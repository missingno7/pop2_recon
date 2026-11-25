#include <string.h>
#include <ctype.h>
#include "types.h"
#include "resc.h"
#include "system.h"
#include "snd.h"
#include "graf.h"
#include "level.h"
#include "heap.h"
#include "char.h"
#include "dlghelp.h"


typedef struct {
    POINT position;
    int state[2];
} Pen;


void show_edit_caret(EDITFIELD far *field)
{
    paint_edit_caret(field, 1);
}

void update_edit_caret(EDITFIELD far *field)
{
    Handle saved_font;

    paint_edit_caret(field, 0);
    saved_font = Graf->font;
    hSetFont(field->font);
    field->x = measure_counted_text(field->text, field->caret) + field->clip.left;
    hSetFont(saved_font);
}

void hide_edit_caret(EDITFIELD far *field)
{
    paint_edit_caret(field, 0);
}

void paint_edit_caret(EDITFIELD far *field, int visible)
{
    word saved_mode;
    Pen pen;

    copy_current_port_origin(&pen);
    ClipRect(&field->clip);
    MoveTo(field->x, field->y);
    if (!visible) {
        field->visible = visible;
        saved_mode = Graf->foreground;
        Graf->foreground = field->port->background;
    } else {
        field->visible = 1;
    }
    draw_relative_axis_segment(0, field->clip.bottom - field->clip.top + 1);
    if (!visible) {
        Graf->foreground = saved_mode;
        if (field->paint)
            field->paint(field);
        else
            StringRect(&field->bounds, field->style, 1, field->text);
    }
    ClipRestore();
    set_relative_pen_state(&pen);
}

void frame_edit_field(EDITFIELD far *field, word mode)
{
    Rect rect;
    word saved_mode = Graf->foreground;

    Graf->foreground = mode;
    InsetRect(&rect, &field->clip, -1, -1);
    FrameRect(&rect);
    Graf->foreground = saved_mode;
}

void clear_flags_of_far_block(void far *block)
{
    clear_payload_header(block);
}

void blink_edit_caret(EDITFIELD far *field)
{
    if ((timer_rate >> 1) < TimeVal - field->last_blink) {
        paint_edit_caret(field, field->visible = !field->visible);
        field->last_blink = TimeVal;
    }
}

int handle_edit_key(EDITFIELD far *field, int key)
{
    byte far *font;
    int rejected, changed, result;
    word i;

    result = changed = rejected = 0;
    if (key) {
        if (!(key & 0xff00)) {
            paint_edit_caret(field, 0);
            font = h2p(field->font);
            if (isprint(key)
                && measure_character(key) != *(int far *)(font + 8)) {
                if (field->capacity == field->length
                    || field->clip.right - field->clip.left
                       < measure_character(key) + measure_nul_terminated_text(field->text)) {
                    rejected = 1;
                } else {
                    for (i = field->length + 1; i > field->caret; --i) {
                        field->text[i] = field->text[i - 1];
                    }
                    field->text[field->caret] = (char)key;
                    ++field->length;
                    ++field->caret;
                    update_edit_caret(field);
                    changed = 1;
                }
            }
        }
    }
    if (!rejected && !changed) {
        switch (key) {
        default:
            result = key;
            break;
        case 8:
            if (field->caret) {
                delete_edit_character(field, --field->caret);
                update_edit_caret(field);
                changed = 1;
            } else
                rejected = 1;
            break;
        case 0x4700:
        case 0x4800:
        case 0x4900:
            field->caret = 0;
            update_edit_caret(field);
            break;
        case 0x4b00:
            if (field->caret) {
                --field->caret;
                update_edit_caret(field);
            } else
                rejected = 1;
            break;
        case 0x4d00:
            if (field->caret != field->length) {
                ++field->caret;
                update_edit_caret(field);
            } else
                rejected = 1;
            break;
        case 0x4f00:
        case 0x5000:
        case 0x5100:
            field->caret = field->length;
            update_edit_caret(field);
            break;
        case 0x5300:
            if (field->caret != field->length) {
                delete_edit_character(field, field->caret);
                changed = 1;
            } else
                rejected = 1;
            break;
        }
    }

    if (changed)
        redraw_edit_text(field);
    else if (rejected)
        SetSnd(-2);

    if (key)
        paint_edit_caret(field, 1);
    return result;
}

void delete_edit_character(EDITFIELD far *field, word index)
{
    for (; index < field->length; ++index) {
        field->text[index] = field->text[index + 1];
    }
    --field->length;
}

void click_edit_field(EDITFIELD far *field, void *point)
{
    POINT position;
    word index;
    int width;
    register int total;

    paint_edit_caret(field, 0);
    clamp_point_to_rect(&position, point, &field->bounds);
    position.x -= field->bounds.left;
    if (measure_counted_text(field->text, field->length) <= position.x)
        position.x = measure_counted_text(field->text, field->length);

    total = 0;
    for (index = 0; index < field->length; ++index) {
        width = measure_character(field->text[index]);
        if ((int)((word)width >> 1) + total > position.x)
            break;
        total += width;
    }
    field->caret = index;
    update_edit_caret(field);
    paint_edit_caret(field, 1);
}

EDITFIELD far * create_edit_field(Rect far *bounds, Rect far *clip,
                                             char far *text, word capacity)
{
    EDITFIELD far *field = allocate_pinned_payload(sizeof(EDITFIELD));


    field->font = (field->port = Graf)->font;
    move_huge_bytes(&field->bounds, bounds, (long)sizeof(Rect));
    move_huge_bytes(&field->clip, clip, (long)sizeof(Rect));
    field->capacity = capacity;
    field->style = -1;
    field->selection_end = 0;
    field->selection_start = 0;
    field->caret = 0;
    field->x = bounds->left - 1;
    field->y = clip->top;
    field->visible = 0;
    field->last_blink = TimeVal;
    field->paint = 0;
    replace_edit_text(field, text);
    return field;
}

void replace_edit_text(EDITFIELD far *field, char far *text)
{
    field->text = text;
    field->length = _fstrlen(text);
    field->caret = 0;
    update_edit_caret(field);
}

void redraw_edit_text(EDITFIELD far *field)
{
    Pen pen;

    copy_current_port_origin(&pen);
    ClipRect(&field->clip);
    EraseRect(&field->bounds);
    MoveTo(field->bounds.left, field->bounds.bottom);
    if (field->paint)
        field->paint(field);
    else
        StringRect(&field->bounds, field->style, 1, field->text);
    ClipRestore();
    set_relative_pen_state(&pen);
}
