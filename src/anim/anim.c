/* ANIM.C, overlay 2. Definitions follow DOS code order. Built with line numbers (/Zd) like the other
   library modules: the original relocation order fixes where its line records end. */
#include "types.h"
#include "char.h"

typedef struct
{
    void far *pixels;
    Rect bounds;
    int row_bytes;
    int flags;
} Port;

typedef union {
    unsigned int word;
    struct {
        unsigned int ink : 4;
        unsigned int move_rate : 4;
        unsigned int frame_rate : 4;
        unsigned int playback : 4;
    } bits;
} ElementFlags;

typedef struct
{
    int shape, first, last;
    ElementFlags flags;
    signed char move_ticks, frame_ticks;
    long x, y;
    long dx, dy;
    long ddx, ddy;
} Element;

typedef struct Animation Animation;
typedef int (far pascal *AnimationCallback)(Animation *, int);

struct Animation
{
    Port port;
    Rect source;
    unsigned char reserved_18[20];
    int destination;
    Port background;
    Rect bounds, dirty;
    const unsigned char far *commands;
    union {
        int repeats;
        unsigned char wait_level;
        unsigned long deadline;
    } timing;
    int skip_draw;
    unsigned int frame_index, delay;
    unsigned long deadline_base;
    AnimationCallback callbacks[4];
    int loops[4];
    Element elements[32];
    int sound_flag;
    int handle_count;
    unsigned long fade;
    int handles[1];
};

typedef struct
{
    int height, width;
    int sound;
    unsigned char depth;
} ShapeHeader;

extern int Graf;
extern unsigned int allocated_banks;
extern union {
    unsigned long count;
    struct {
        unsigned int low, high;
    } words;
} TimeVal;
extern void MEASURE_TEXT(int, int, const Rect far *, const Rect far *, int);
extern void QuitFadeTo(unsigned long);
extern int fade_to_step(unsigned long);
extern void dispose_handle(int);
extern void release_port(void *);
extern void release_buffer(void far *);
extern void cdecl free(void *);

Animation *anim_draw(Animation *animation)
{
    int saved_port = Graf;

    Graf = animation->destination;
    MEASURE_TEXT(Graf, animation, &animation->bounds, &animation->source, 0);
    Graf = saved_port;
    return animation;
}

void anim_free(Animation *animation)
{
    int i;

    if (animation->fade)
        QuitFadeTo(animation->fade);
    for (i = 0; i < animation->handle_count; ++i) {
        if (animation->handles[i])
            dispose_handle(animation->handles[i]);
    }
    release_port(animation);
    release_buffer(animation->background.pixels);
    if (animation->sound_flag)
        allocated_banks &= ~animation->sound_flag;
    free(animation);
}

Animation *draw_frame(Animation *animation);
Animation *near advance_animation_frame(Animation *animation);
void near paint_element(Animation *animation, Element *element);
void near accumulate_element_bounds(Animation *animation, Element *element);
void near reset_element(Element *element);

extern void play_sound(int id);
extern void dispatch_sound(int);
extern int query_sound(int);
extern unsigned char midi_cue;
int high_color_shape_boundary = 0;

/* byte-offset access for scanning an unparsed animation resource. */
#define WORD_AT(p, n) (((const int far *)&(p)[(n)])[0])
/* Command operands begin immediately after the opcode. */
#define EVENT_WORD(p, i) (((const int far *)((p) + 1))[(i)])
#define EVENT_LONG(p, i) (((const long far *)((p) + 1))[(i)])
#define CLOCK_WORDS() (TimeVal.words.low | ((unsigned long)TimeVal.words.high << 16))

int anim_idle(Animation *animation)
{
    int status = 0;
    int previous_port = Graf;
    const unsigned char far *event;
    int idx;
    ElementFlags saved_flags;
    void far *saved_pixels;
    Element *current_element;

    Graf = animation;

    for (;;) {
        event = animation->commands;
        switch (*event) {
        case 0:
            if (animation->deadline_base + animation->delay <= CLOCK_WORDS())
                status = 1;
            break;
        case 1:
            if (animation->deadline_base + animation->delay <= CLOCK_WORDS()) {
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
                animation->timing.deadline = ((long)EVENT_WORD(event, 0) << 16 >> 16) + CLOCK_WORDS();
                if (animation->deadline_base)
                    animation->deadline_base += EVENT_WORD(event, 0);
            } else if (animation->timing.deadline <= TimeVal.count) {
                animation->timing.deadline = 0;
                animation->commands += 3;
            }
            break;
        case 3:
            if (animation->deadline_base + animation->delay <= CLOCK_WORDS()) {
                play_sound(EVENT_WORD(event, 0));
                animation->commands += 3;
            }
            break;
        case 4:
            dispatch_sound(EVENT_WORD(event, 0));
            animation->commands += 3;
            break;
        case 5:
            if (!query_sound(EVENT_WORD(event, 0)))
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
            draw_frame(animation);
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
                        *(unsigned *)&animation->commands = event + EVENT_WORD(event, 1) + 3;
                    else
                        animation->commands += 5;
                } else {
                    animation->loops[idx] = EVENT_WORD(event, 0);
                    *(unsigned *)&animation->commands = event + EVENT_WORD(event, 1) + 3;
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
                if ((signed char)event[1] > 0) {
                    Graf = animation->destination;
                    ((int *)&current_element->x)[1] += animation->bounds.left;
                    ((int *)&current_element->y)[1] += animation->bounds.top;
                    paint_element(animation, current_element);
                    ((int *)&current_element->x)[1] -= animation->bounds.left;
                    ((int *)&current_element->y)[1] -= animation->bounds.top;
                    Graf = animation;
                }
                if (!event[1]) {
                    paint_element(animation, current_element);
                } else {
                    saved_pixels = animation->port.pixels;
                    animation->port.pixels = animation->background.pixels;
                    if (animation->port.pixels)
                        paint_element(animation, current_element);
                    animation->port.pixels = saved_pixels;
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

extern void near *cdecl _nmalloc(unsigned int);
extern unsigned int cdecl _nmsize(void *);
extern void near *cdecl memset(void near *, int, unsigned int);
#pragma intrinsic(memset)
extern void far *cdecl _fmemcpy(void far *, const void far *, unsigned int);

extern int SAVE_HANDLE_FLAGS(int);
extern void RESTORE_HANDLE_FLAGS(int, int);
extern ShapeHeader far *resolve_handle_to_far_payload(int);
extern ShapeHeader far *fetch_locked_shape_list(int);
extern void load_shape_list_banks(int);
extern void CLEAR_HANDLE_OWNED(int);
extern void MARK_FIXED(int);
extern int hNewShpItem1(int, int, int);
extern int create_port(void *, const Rect far *);
extern void SET_ORIGIN(int, int);
extern void far *ALLOCATE_BUFFER(unsigned long);
static unsigned char animation_command_codes[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 0x10, 0x14, 0x20, 0x40, 0x60, 0x80, 0xa0, 0xc0, 0xe0
};
static unsigned char animation_operand_lengths[] = {
    0, 4, 2, 2, 2, 2, 1, 0, 2, 4, 4, 4, 4, 4, 8, 8, 1
};
char *cp;
int TGAMode, ForceShapes;
extern int cdecl sprintf(char *, const char *, ...);
extern void abort_with_message(int, const char far *);

Animation *anim_new(const unsigned char far *commands, int sheet, int first_shape,
                    int last_shape, int save_background)
{
    int old_entry = SAVE_HANDLE_FLAGS(sheet);
    int count = last_shape - first_shape + 1;
    int ei, previous_port;
    unsigned char used_entries[512];
    Animation *anim_record;
    ShapeHeader far *sheet_info;
    int idx;
    const unsigned char far *command_cursor;
    unsigned char operation;
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
        sheet_info = fetch_locked_shape_list(sheet);
        if (sheet_info) {
            TGAMode = sheet_info->depth <= 16;
            if (sheet_info->sound == 0) {
                load_shape_list_banks(sheet);
                anim_record->sound_flag = sheet_info->sound;
            }
            for (count = 0; count < anim_record->handle_count; ++count) {
                if (used_entries[count]) {
                    if (!(anim_record->handles[count] = hNewShpItem1(sheet, count + first_shape,
                            TGAMode == 0 ? 1 : 2))) {
                        if (anim_record->sound_flag != 0) sheet_info->sound = 0;
                        goto fail;
                    }
                } else {
                    anim_record->handles[count] = 0;
                }
            }
            if (anim_record->sound_flag != 0) sheet_info->sound = 0;
            CLEAR_HANDLE_OWNED(sheet);
            MARK_FIXED(sheet);
        }
        {
            if (!create_port(anim_record, _fmemcpy(&anim_record->bounds, commands, sizeof(Rect)))) goto fail;
            Graf = anim_record;
            SET_ORIGIN(0, 0);
            anim_record->dirty = anim_record->source;
            anim_record->commands = &commands[8];
            if (save_background) {
                anim_record->background = anim_record->port;
                anim_record->background.pixels = ALLOCATE_BUFFER(
                    (long)(anim_record->port.bounds.bottom - anim_record->port.bounds.top) *
                    anim_record->port.row_bytes);
                if (!anim_record->background.pixels) goto fail;
                MEASURE_TEXT(&anim_record->background, anim_record->destination,
                          &anim_record->source, &anim_record->bounds, 0);
            }
        }
        draw_frame(anim_record);
        goto restore;
fail:
        anim_free(anim_record);
        anim_record = 0;
restore:
        Graf = previous_port;
    }
    RESTORE_HANDLE_FLAGS(sheet, old_entry);
    return anim_record;
}

extern Rect far *OFFSETRECT(Rect far *, const Rect far *, int, int);
extern void PUSH_PORT_CLIP(Rect far *);
extern void SET_RECORD_RANGE(Rect far *, int);
extern void POP_PORT_CLIP(void);
int ForceFuncParam;

Animation *near advance_animation_frame(Animation *animation)
{
    struct { Rect box; int reserved; } region; /* Extra stack word of unknown purpose. */
    Element *cursor;
    Element *element;
    int count;
    signed char tick;

    ++animation->frame_index; animation->deadline_base = TimeVal.count;
    if (!animation->skip_draw)
        draw_frame(animation);
    animation->skip_draw = 0;
    OFFSETRECT(&region.box, &animation->dirty, animation->bounds.left, animation->bounds.top);
    MEASURE_TEXT(animation->destination, animation, &region.box, &animation->dirty, 0);
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

Animation *draw_frame(Animation *animation)
{
    int i;
    Element *element;

    PUSH_PORT_CLIP(&animation->dirty);
    if (animation->background.pixels) {
        MEASURE_TEXT(animation, &animation->background,
                  &animation->source, &animation->source, 0);
    } else {
        SET_RECORD_RANGE(&animation->source, *(int *)((char *)animation->destination + 0x20));
    }
    i = 0; element = animation->elements;
    do {
        paint_element(animation, element);
        if (i && animation->frame_index > 1 && animation->fade && fade_to_step(animation->fade)) {
            QuitFadeTo(animation->fade); animation->fade = 0;
        }
        ++element; } while (++i < 32);
    POP_PORT_CLIP();
    if (ForceFuncParam && animation->callbacks[0])
        animation->callbacks[0](animation, ForceFuncParam);
    return animation;
}

extern void PAINTRLESHAPE_ASM(const ShapeHeader far *, int, int, int);
extern void PAINT_PLAIN(const ShapeHeader far *, int, int, int);

void near paint_element(Animation *animation, Element *element)
{
    if (element->shape) {
        if (!TGAMode && high_color_shape_boundary < element->shape)
            PAINTRLESHAPE_ASM(resolve_handle_to_far_payload(animation->handles[element->shape - 1]),
                              (int)(element->x >> 16), (int)(element->y >> 16),
                              element->flags.word & 15);
        else
            PAINT_PLAIN(resolve_handle_to_far_payload(animation->handles[element->shape - 1]),
                        (int)(element->x >> 16), (int)(element->y >> 16),
                        element->flags.word & 15);
    }
}

extern int SECTRECT(Rect far *, const Rect far *, const Rect far *);
extern int EMPTYRECT(const Rect far *);
extern void UNIONRECT(Rect far *, const Rect far *, const Rect far *);

void near accumulate_element_bounds(Animation *animation, Element *element)
{
    Rect rect;

    if (element->shape) {
        rect.left = (int)(element->x >> 16);
        rect.right = rect.left +
                     resolve_handle_to_far_payload(animation->handles[element->shape - 1])->width;
        rect.top = (int)(element->y >> 16);
        rect.bottom = rect.top +
                      resolve_handle_to_far_payload(animation->handles[element->shape - 1])->height;
        if (SECTRECT(&rect, &rect, &animation->source)) {
            if (EMPTYRECT(&animation->dirty))
                animation->dirty = rect;
            else
                UNIONRECT(&animation->dirty, &animation->dirty, &rect);
        }
    }
}

void near reset_element(Element *element)
{
    memset(element, 0, sizeof(*element));
    element->flags.word = (element->flags.word & 0xfff2) | 2;
    element->y = 0x80000000UL;
    element->x = 0x80000000UL;
}

extern int check_wait_function(void);
extern void fade_out_and_restore_palette(void);
extern void DISPATCH_RESOURCE_POINTER(int, int);
extern void wait_sound(int);
extern const unsigned char far *LOAD_RESOURCE_PINNED(unsigned long, int);
extern int hGetNMapShpList(int, int, int);
extern int HGETRSRC(unsigned long, int);
extern int hGetSnd(int);
extern void CLEAR_PAYLOAD_HEADER(const unsigned char far *);
extern int query_sound_payload(const void far *);
extern char ScreenPort[];

int play_anim(int sheet_override, int scene, int scene_offset, int save_background,
              AnimationCallback callback, int restore, unsigned int flags, int interruptible)
{
    int scene_id;
    int palette_list, count;
    int sheet;
    const unsigned char far *commands;
    Animation *animation;
    int palettes[20];
    int result, aborted = 0;
    char message[80];

    scene_id = scene + scene_offset; commands = LOAD_RESOURCE_PINNED(0x00534352UL, scene_id);
    if (sheet_override) sheet = sheet_override;
    else
        sheet = hGetNMapShpList(scene, -1, 1);
    animation = anim_new(commands, sheet, 1, resolve_handle_to_far_payload(sheet)->width, save_background);
    animation->callbacks[0] = callback;
    sprintf(message, "AnimNew(%d)", scene);
    if (!animation) abort_with_message(1, message);

    ForceShapes = 0;
    palette_list = HGETRSRC(0x0050534cUL, scene_id);
    count = *(int far *)resolve_handle_to_far_payload(palette_list);
    for (result = 0; result < count; ++result)
        palettes[result] = hGetSnd(((int far *)resolve_handle_to_far_payload(palette_list) + 1)[result]);
    dispose_handle(palette_list);
    do {
        result = anim_idle(animation);
        if (interruptible) {
            if (result == 2 || check_wait_function()) {
                fade_out_and_restore_palette(); DISPATCH_RESOURCE_POINTER(0, 0);
                aborted = 1;
                break;
            }
            if (result == 2) result = 0;
        }
        if (animation->fade && fade_to_step(animation->fade)) {
            QuitFadeTo(animation->fade);
            animation->fade = 0;
        }
    } while (!result);
    if (restore) {
        if (save_background) {
            Rect rect;
            MEASURE_TEXT(ScreenPort, &animation->background, &animation->bounds,
                      OFFSETRECT(&rect, &animation->bounds, -animation->bounds.left,
                                  -animation->bounds.top), 0);
        } else {
            SET_RECORD_RANGE((Rect far *)commands, 0);
        }
    }
    anim_free(animation);
    if (!sheet_override) dispose_handle(sheet);
    CLEAR_PAYLOAD_HEADER(commands);
    if (flags & 2) wait_sound(0);
    if (flags & 1) DISPATCH_RESOURCE_POINTER(0, 0);
    for (result = 0; result < count; ++result)
        if (palettes[result] && !query_sound_payload(resolve_handle_to_far_payload(palettes[result]))) dispose_handle(palettes[result]);
    return aborted;
}

void swap_record_pairs(Animation *animation)
{
    void far *pixels = animation->port.pixels;

    animation->port.pixels = animation->background.pixels;
    animation->background.pixels = pixels;
}
