/* ANIM.C, overlay 2. Definitions follow DOS code order. Built with line numbers (/Zd) like the other
   library modules: the original relocation order fixes where its line records end. */
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include "types.h"
#include "resc.h"
#include "system.h"
#include "snd.h"
#include "graf.h"
#include "shape.h"
#include "level.h"
#include "heap.h"
#include "char.h"
#include "fade.h"
#include "anim.h"
#include "hnewshpi.h"
#include "init.h"
#include "intrface.h"
#include "xlatgraf.h"


typedef int (*ANIMATIONCALLBACK)(ANIMATION *, int);


ANIMATION *AnimDraw(ANIMATION *animation)
{
    PORT *saved_port = Graf;

    Graf = animation->destination;
    CopyBits(Graf, animation, &animation->background.rect, &animation->port.visible, 0);
    Graf = saved_port;
    return animation;
}

void AnimFree(ANIMATION *animation)
{
    int i;

    if (animation->fade)
        QuitFadeTo(animation->fade);
    for (i = 0; i < animation->handle_count; ++i) {
        if (animation->handles[i])
            hFree(animation->handles[i]);
    }
    FreePort(&animation->port);
    release_buffer(animation->background.map.pixels);
    if (animation->banks)
        BankStatus &= ~animation->banks;
    free(animation);
}


void near paint_element(ANIMATION *animation, ELEMENT *element);
void near accumulate_element_bounds(ANIMATION *animation, ELEMENT *element);
void near reset_element(ELEMENT *element);


int RLEShapeCutOff = 0;

/* byte-offset access for scanning an unparsed animation resource. */
#define WORD_AT(p, n) (((const int far *)&(p)[(n)])[0])
/* Command operands begin immediately after the opcode. */
#define EVENT_WORD(p, i) (((const int far *)((p) + 1))[(i)])
#define EVENT_LONG(p, i) (((const long far *)((p) + 1))[(i)])

int AnimIdle(ANIMATION *animation)
{
    int status = 0;
    PORT *previous_port = Graf;
    const byte far *event;
    int idx;
    ELEMENTFLAGS saved_flags;
    void far *saved_pixels;
    ELEMENT *current_element;

    Graf = &animation->port;

    for (;;) {
        event = animation->commands;
        switch (*event) {
        case 0:
            if (animation->deadline_base + animation->delay <= TimeVal)
                status = 1;
            break;
        case 1:
            if (animation->deadline_base + animation->delay <= TimeVal) {
                advance_animation_frame(animation);
                if (!animation->timing.repeats) {
                    animation->timing.repeats = EVENT_WORD(event, 0);
                    animation->delay = EVENT_WORD(event, 1);
                }
                if (!--animation->timing.repeats)
                    animation->commands += 5;
            }
            break;
        case 2:
            if (!animation->timing.deadline) {
                animation->timing.deadline = EVENT_WORD(event, 0) + TimeVal;
                if (animation->deadline_base)
                    animation->deadline_base += EVENT_WORD(event, 0);
            } else if (animation->timing.deadline <= TimeVal) {
                animation->timing.deadline = 0;
                animation->commands += 3;
            }
            break;
        case 3:
            if (animation->deadline_base + animation->delay <= TimeVal) {
                SetSnd(EVENT_WORD(event, 0));
                animation->commands += 3;
            }
            break;
        case 4:
            ResetSnd(EVENT_WORD(event, 0));
            animation->commands += 3;
            break;
        case 5:
            if (!SndOn(EVENT_WORD(event, 0)))
                animation->commands += 3;
            break;
        case 6:
            if (!animation->timing.wait_level)
                animation->timing.wait_level = event[1];
            if (!midi_cue || animation->timing.wait_level <= midi_cue) {
                animation->timing.wait_level = 0;
                animation->commands += 2;
            }
            break;
        case 7:
            DrawFrame(animation);
            animation->skip_draw = 1;
            ++animation->commands;
            break;
        default:
            if ((*event & ~3) == 0x10) {
                idx = *event & 3;
                if (animation->callbacks[idx] && animation->callbacks[idx](animation, EVENT_WORD(event, 0)))
                    status = 2;
                animation->commands += 3;
                continue;
            }
            if ((*event & ~3) == 0x14) {
                idx = *event & 3;
                if (animation->loops[idx]) {
                    if (--animation->loops[idx])
                        *(word *)&animation->commands = event + EVENT_WORD(event, 1) + 3;
                    else
                        animation->commands += 5;
                } else {
                    animation->loops[idx] = EVENT_WORD(event, 0);
                    *(word *)&animation->commands = event + EVENT_WORD(event, 1) + 3;
                }
                continue;
            }
            current_element = &animation->elements[*event & 31];
            switch (*event & ~31) {
            case 0x20:
                saved_flags = current_element->flags;
                current_element->flags.word &= EVENT_WORD(event, 0);
                current_element->flags.word |= EVENT_WORD(event, 1);
                if (current_element->flags.bits.move_rate != saved_flags.bits.move_rate)
                    current_element->move_ticks = 0;
                if (current_element->flags.bits.frame_rate != saved_flags.bits.frame_rate)
                    current_element->frame_ticks = 0;
                animation->commands += 5;
                continue;
            case 0x40:
                accumulate_element_bounds(animation, current_element);
                current_element->x += (long)EVENT_WORD(event, 0) << 16;
                current_element->y += (long)EVENT_WORD(event, 1) << 16;
                accumulate_element_bounds(animation, current_element);
                animation->commands += 5;
                continue;
            case 0x60:
                accumulate_element_bounds(animation, current_element);
                current_element->x = (long)EVENT_WORD(event, 0) << 16;
                current_element->y = (long)EVENT_WORD(event, 1) << 16;
                accumulate_element_bounds(animation, current_element);
                animation->commands += 5;
                continue;
            case 0x80:
                accumulate_element_bounds(animation, current_element);
                current_element->last = EVENT_WORD(event, 1);
                current_element->shape = current_element->first = EVENT_WORD(event, 0);
                if (current_element->shape)
                    accumulate_element_bounds(animation, current_element);
                else
                    reset_element(current_element);
                animation->commands += 5;
                continue;
            case 0xa0:
                current_element->dx = EVENT_LONG(event, 0);
                current_element->dy = EVENT_LONG(event, 1);
                animation->commands += 9;
                continue;
            case 0xc0:
                current_element->ddx = EVENT_LONG(event, 0);
                current_element->ddy = EVENT_LONG(event, 1);
                animation->commands += 9;
                continue;
            case 0xe0:
                if ((sbyte)event[1] > 0) {
                    Graf = animation->destination;
                    ((int *)&current_element->x)[1] += animation->background.rect.left;
                    ((int *)&current_element->y)[1] += animation->background.rect.top;
                    paint_element(animation, current_element);
                    ((int *)&current_element->x)[1] -= animation->background.rect.left;
                    ((int *)&current_element->y)[1] -= animation->background.rect.top;
                    Graf = &animation->port;
                }
                if (!event[1]) {
                    paint_element(animation, current_element);
                } else {
                    saved_pixels = animation->port.map.pixels;
                    animation->port.map.pixels = animation->background.map.pixels;
                    if (animation->port.map.pixels)
                        paint_element(animation, current_element);
                    animation->port.map.pixels = saved_pixels;
                }
                animation->commands += 2;
                continue;
            default:
                status = 1;
                break;
            }
            break;
        }
        break;
    }

    Graf = previous_port;
    return status;
}


static byte animation_command_codes[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 0x10, 0x14, 0x20, 0x40, 0x60, 0x80, 0xa0, 0xc0, 0xe0
};
static byte animation_operand_lengths[] = {
    0, 4, 2, 2, 2, 2, 1, 0, 2, 4, 4, 4, 4, 4, 8, 8, 1
};
char *cp;
int TGAMode, ForceShapes;


ANIMATION *AnimNew(const byte far *commands, Handle sheet, int first_shape,
                    int last_shape, int save_background)
{
    int old_entry = hGetState(sheet);
    int count = last_shape - first_shape + 1;
    int ei;
    PORT *previous_port;
    byte used_entries[512];
    ANIMATION *anim_record;
    SHAPELISTRSRC far *sheet_info;
    int idx;
    const byte far *command_cursor;
    byte operation;
    char warning[80];

    anim_record = _nmalloc((0x260 + count) * 2);
    if (anim_record) {
        memset(anim_record, 0, _nmsize(anim_record));
        previous_port = anim_record->destination = Graf;
        ei = 0;
        while (ei < 32) {
            reset_element(&anim_record->elements[ei]);
            ++ei;
        }
        idx = 0;
        do {
            used_entries[idx] = ForceShapes;
        } while (++idx < (int)sizeof(used_entries));
        if (!ForceShapes) {
            command_cursor = commands + sizeof(Rect);
            while ((operation = *command_cursor) != 0) {
                for (idx = 0; idx < 8; ++idx) {
                    if (operation == animation_command_codes[idx])
                        goto found_command;
                }
                idx = 8;
                if ((operation & ~3) == animation_command_codes[idx] ||
                    (idx = 9, (operation & ~3) == animation_command_codes[idx]))
                    goto found_command;
                if ((operation & ~31) == 0x80 && (idx = WORD_AT(command_cursor, 1)) != 0) {
                    for (; idx <= WORD_AT(command_cursor, 3); ++idx) {
                        if (last_shape < idx) {
                            sprintf(warning, "AnimNew(): Cast member %d out of range", idx);
                            abort_with_message(1, warning);
                        } else {
                            used_entries[idx - 1] = 1;
                        }
                    }
                }
                for (idx = 10; idx < 17; ++idx) {
                    if (animation_command_codes[idx] == (operation & ~31)) {
                    found_command:
                        command_cursor = &command_cursor[animation_operand_lengths[idx] + 1];
                    }
                }
            }
        }
        anim_record->handle_count = count;
        sheet_info = hLock(sheet);
        if (sheet_info) {
            TGAMode = sheet_info->palette.colors <= 16;
            if (sheet_info->palette.banks == 0) {
                hMapShpList(sheet);
                anim_record->banks = sheet_info->palette.banks;
            }
            for (count = 0; count < anim_record->handle_count; ++count) {
                if (used_entries[count]) {
                    if (!(anim_record->handles[count] = hNewShpItem1(sheet, count + first_shape,
                            TGAMode == 0 ? 1 : 2))) {
                        if (anim_record->banks != 0) sheet_info->palette.banks = 0;
                        goto fail;
                    }
                } else {
                    anim_record->handles[count] = 0;
                }
            }
            if (anim_record->banks != 0) sheet_info->palette.banks = 0;
            hUnlock(sheet);
            hNoPurge(sheet);
        }
        {
            if (!BackPort(&anim_record->port, _fmemcpy(&anim_record->background.rect, commands, sizeof(Rect)))) goto fail;
            Graf = &anim_record->port;
            SetOrigin(0, 0);
            anim_record->dirty = anim_record->port.visible;
            anim_record->commands = &commands[8];
            if (save_background) {
                anim_record->background.map = anim_record->port.map;
                anim_record->background.map.pixels = allocate_buffer(
                    (long)(anim_record->port.map.bounds.bottom - anim_record->port.map.bounds.top) *
                    anim_record->port.map.row_bytes);
                if (!anim_record->background.map.pixels) goto fail;
                CopyBits(&anim_record->background, anim_record->destination,
                          &anim_record->port.visible, &anim_record->background.rect, 0);
            }
        }
        DrawFrame(anim_record);
        goto restore;
fail:
        AnimFree(anim_record);
        anim_record = 0;
restore:
        Graf = previous_port;
    }
    hSetState(sheet, old_entry);
    return anim_record;
}


int ForceFuncParam;

ANIMATION *near advance_animation_frame(ANIMATION *animation)
{
    struct { Rect box; int reserved; } region; /* Extra stack word of unknown purpose. */
    ELEMENT *cursor;
    ELEMENT *element;
    int count;
    sbyte tick;

    ++animation->frame_index; animation->deadline_base = TimeVal;
    if (!animation->skip_draw)
        DrawFrame(animation);
    animation->skip_draw = 0;
    OffsetRect(&region.box, &animation->dirty, animation->background.rect.left, animation->background.rect.top);
    CopyBits(animation->destination, animation, &region.box, &animation->dirty, 0);
    memset(&animation->dirty, 0, sizeof(Rect));
    cursor = animation->elements; count = 32;
    do {
        element = cursor;
        if (element->shape) {
            if (element->first != element->last) {
                tick = element->frame_ticks++;
                if (((element->flags.word >> 8) & 15) <= tick) {
                    element->frame_ticks = 0; accumulate_element_bounds(animation, element);
                    element->shape += (element->flags.word & 0x1000) ? -1 : 1;
                    if (element->flags.word & 0x2000) {
                        if (element->shape < element->first) {
                            element->shape = element->first + 1; element->flags.word &= ~0x1000;
                        } else if (element->shape > element->last) {
                            element->shape = element->last - 1; element->flags.word |= 0x1000;
                        }
                    } else if (element->shape < element->first) { element->shape = element->last;
                    } else if (element->shape > element->last) { element->shape = element->first;
                    }
                    accumulate_element_bounds(animation, element);
                }
            }
            tick = element->move_ticks++;
            if (((element->flags.word >> 4) & 15) <= tick) {
                element->move_ticks = 0;
                if (element->dx || element->dy) {
                    accumulate_element_bounds(animation, element);
                    element->x += element->dx; element->y += element->dy;
                    accumulate_element_bounds(animation, element);
                }
                element->dx += element->ddx; element->dy += element->ddy;
            }
        }
        ++cursor;
    } while (--count);
    return animation;
}

ANIMATION *DrawFrame(ANIMATION *animation)
{
    int i;
    ELEMENT *element;

    ClipRect(&animation->dirty);
    if (animation->background.map.pixels) {
        CopyBits(animation, &animation->background,
                  &animation->port.visible, &animation->port.visible, 0);
    } else {
        FillRect(&animation->port.visible, *(int *)((char *)animation->destination + 0x20));
    }
    i = 0; element = animation->elements;
    do {
        paint_element(animation, element);
        if (i && animation->frame_index > 1 && animation->fade && FadeToStep(animation->fade)) {
            QuitFadeTo(animation->fade); animation->fade = 0;
        }
        ++element; } while (++i < 32);
    ClipRestore();
    if (ForceFuncParam && animation->callbacks[0])
        animation->callbacks[0](animation, ForceFuncParam);
    return animation;
}


void near paint_element(ANIMATION *animation, ELEMENT *element)
{
    if (element->shape) {
        if (!TGAMode && RLEShapeCutOff < element->shape)
            PaintRLEShape_asm(h2p(animation->handles[element->shape - 1]),
                              (int)(element->x >> 16), (int)(element->y >> 16),
                              element->flags.word & 15);
        else
            PaintShape(h2p(animation->handles[element->shape - 1]),
                        (int)(element->x >> 16), (int)(element->y >> 16),
                        element->flags.word & 15);
    }
}


void near accumulate_element_bounds(ANIMATION *animation, ELEMENT *element)
{
    Rect rect;

    if (element->shape) {
        rect.left = (int)(element->x >> 16);
        rect.right = rect.left +
                     ((SHAPE far *)h2p(animation->handles[element->shape - 1]))->width;
        rect.top = (int)(element->y >> 16);
        rect.bottom = rect.top +
                      ((SHAPE far *)h2p(animation->handles[element->shape - 1]))->height;
        if (SectRect(&rect, &rect, &animation->port.visible)) {
            if (EmptyRect(&animation->dirty))
                animation->dirty = rect;
            else
                UnionRect(&animation->dirty, &animation->dirty, &rect);
        }
    }
}

void near reset_element(ELEMENT *element)
{
    memset(element, 0, sizeof(*element));
    element->flags.word = (element->flags.word & 0xfff2) | 2;
    element->y = 0x80000000UL;
    element->x = 0x80000000UL;
}


int PlayAnim(Handle shapes, int scene, int scene_offset, int save_background,
              ANIMATIONCALLBACK callback, int restore, int flags, int interruptible)
{
    int scene_id;
    Handle palette_list;
    int count;
    Handle sheet;
    const byte far *commands;
    ANIMATION *animation;
    Handle palettes[20];
    int result, aborted = 0;
    char message[80];

    scene_id = scene + scene_offset; commands = load_resource_pinned(0x00534352UL, scene_id);
    if (shapes) sheet = shapes;
    else
        sheet = hGetNMapShpList(scene, -1, 1);
    animation = AnimNew(commands, sheet, 1, ((SHAPELISTRSRC far *)h2p(sheet))->count, save_background);
    animation->callbacks[0] = callback;
    sprintf(message, "AnimNew(%d)", scene);
    if (!animation) abort_with_message(1, message);

    ForceShapes = 0;
    palette_list = hGetRsrc(0x0050534cUL, scene_id);
    count = *(int far *)h2p(palette_list);
    for (result = 0; result < count; ++result)
        palettes[result] = hGetSnd(((int far *)h2p(palette_list) + 1)[result]);
    hFree(palette_list);
    do {
        result = AnimIdle(animation);
        if (interruptible) {
            if (result == 2 || CheckWaitFunc()) {
                ForceFadeOut(); ResetSound(0L);
                aborted = 1;
                break;
            }
            if (result == 2) result = 0;
        }
        if (animation->fade && FadeToStep(animation->fade)) {
            QuitFadeTo(animation->fade);
            animation->fade = 0;
        }
    } while (!result);
    if (restore) {
        if (save_background) {
            Rect rect;
            CopyBits(&ScreenPort, &animation->background, &animation->background.rect,
                      OffsetRect(&rect, &animation->background.rect, -animation->background.rect.left,
                                  -animation->background.rect.top), 0);
        } else {
            FillRect((Rect far *)commands, 0);
        }
    }
    AnimFree(animation);
    if (!shapes) hFree(sheet);
    clear_payload_header(commands);
    if (flags & 2) WaitSnd(0);
    if (flags & 1) ResetSound(0L);
    for (result = 0; result < count; ++result)
        if (palettes[result] && !query_sound_payload(h2p(palettes[result]))) hFree(palettes[result]);
    return aborted;
}

void SwapBackGround(ANIMATION *animation)
{
    void far *pixels = animation->port.map.pixels;

    animation->port.map.pixels = animation->background.map.pixels;
    animation->background.map.pixels = pixels;
}
