typedef unsigned int Word;
typedef struct {
    int top, left, bottom, right;
} Rect;

typedef struct {
    int y, x;
} Point;

typedef struct {
    Point position;
    int state[2];
} Pen;

typedef struct {
    char reserved[0x20];
    Word erase_mode;
    char pen[6];
    Word pen_mode;
    Word font;
} Port;

typedef struct EditField EditField;

struct EditField {
    Port *port;
    Word font;
    Rect bounds;
    Rect clip;
    int style;
    Word selection_start;
    Word selection_end;
    Word caret;
    int y, x;
    int visible;
    unsigned long last_blink;
    char far *text;
    Word capacity;
    Word length;
    void (far pascal *paint)(EditField far *);
};

extern Port *current_port;
extern unsigned long clock_ticks;
extern Word blink_interval;
extern unsigned char near cdecl _ctype[];
#define isprint(c) ((_ctype + 1)[(c)] & 0x57)
extern void far pascal copy_pen(Pen far *);
extern void far pascal push_clip(Rect far *);
extern void far pascal MoveTo(int, int);
extern void far pascal draw_cursor_line(int, int);
extern void far pascal draw_text(Rect far *, int, int, char far *);
extern void far pascal pop_clip(void);
extern void far pascal restore_pen(Pen far *);
extern Word far pascal hSetFont(Word);
extern int far pascal measure_counted_text(char far *, Word);
extern void far pascal InsetRect(Rect far *, Rect far *, int, int);
extern void far pascal frame_rect(Rect far *);
extern void far pascal CLEAR_FAR_BLOCK_FLAGS(void far *block);
extern void far * far pascal resolve_handle(Word);
extern int far pascal measure_char(unsigned char);
extern int far pascal measure_text(char far *);
extern void far pascal report_option(int);
extern void far pascal clamp_point(Point far *, void far *, Rect far *);
extern EditField far * far pascal alloc_far_block(long);
extern void far pascal move_far_bytes(void far *, const void far *, long);
extern Word far cdecl far_string_length(const char far *);
extern void far pascal start_rect_effect(Rect far *);

void far pascal paint_edit_caret(EditField far *, int);
void far pascal delete_edit_character(EditField far *, Word);
void far pascal replace_edit_text(EditField far *, char far *);
void far pascal redraw_edit_text(EditField far *);

void far pascal show_edit_caret(EditField far *field)
{
    paint_edit_caret(field, 1);
}

void far pascal update_edit_caret(EditField far *field)
{
    Word saved_font;

    paint_edit_caret(field, 0);
    saved_font = current_port->font;
    hSetFont(field->font);
    field->x = measure_counted_text(field->text, field->caret) + field->clip.left;
    hSetFont(saved_font);
}

void far pascal hide_edit_caret(EditField far *field)
{
    paint_edit_caret(field, 0);
}

void far pascal paint_edit_caret(EditField far *field, int visible)
{
    Word saved_mode;
    Pen pen;

    copy_pen(&pen);
    push_clip(&field->clip);
    MoveTo(field->x, field->y);
    if (!visible) {
        field->visible = visible;
        saved_mode = current_port->pen_mode;
        current_port->pen_mode = field->port->erase_mode;
    } else {
        field->visible = 1;
    }
    draw_cursor_line(0, field->clip.bottom - field->clip.top + 1);
    if (!visible) {
        current_port->pen_mode = saved_mode;
        if (field->paint)
            field->paint(field);
        else
            draw_text(&field->bounds, field->style, 1, field->text);
    }
    pop_clip();
    restore_pen(&pen);
}

void far pascal frame_edit_field(EditField far *field, Word mode)
{
    Rect rect;
    Word saved_mode = current_port->pen_mode;

    current_port->pen_mode = mode;
    InsetRect(&rect, &field->clip, -1, -1);
    frame_rect(&rect);
    current_port->pen_mode = saved_mode;
}

/* Forward the far pointer to the separately reviewed block-flag helper. */
void far pascal CLEAR_FLAGS_OF_FAR_BLOCK(void far *block)
{
    CLEAR_FAR_BLOCK_FLAGS(block);
}

void far pascal blink_edit_caret(EditField far *field)
{
    if ((blink_interval >> 1) < clock_ticks - field->last_blink) {
        paint_edit_caret(field, field->visible = !field->visible);
        field->last_blink = clock_ticks;
    }
}

int far pascal handle_edit_key(EditField far *field, int key)
{
    unsigned char far *font;
    int rejected, changed, result;
    Word i;

    result = changed = rejected = 0;
    if (key) {
        if (!(key & 0xff00)) {
            paint_edit_caret(field, 0);
            font = resolve_handle(field->font);
            if (isprint(key)
                && measure_char(key) != *(int far *)(font + 8)) {
                if (field->capacity == field->length
                    || field->clip.right - field->clip.left
                       < measure_char(key) + measure_text(field->text)) {
                    rejected = 1;
                } else {
                    i = field->length + 1;
                    if (i > field->caret) {
                        do {
                            field->text[i] = field->text[i - 1];
                            --i;
                        } while (i > field->caret);
                    }
                    field->text[field->caret] = (char)key;
                    ++field->length;
                    ++field->caret;
                    update_edit_caret(field);
                    changed = 1;
                    goto dispatch;
                }
            }
        }
    }
dispatch:
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
        report_option(-2);

    if (key)
        paint_edit_caret(field, 1);
    return result;
}

void far pascal delete_edit_character(EditField far *field, Word index)
{
    if (index < field->length) {
        do {
            field->text[index] = field->text[index + 1];
        } while (++index < field->length);
    }
    --field->length;
}

void far pascal click_edit_field(EditField far *field, void *point)
{
    Point position;
    Word index;
    int width;
    register int total;

    paint_edit_caret(field, 0);
    clamp_point(&position, point, &field->bounds);
    position.x -= field->bounds.left;
    if (measure_counted_text(field->text, field->length) <= position.x)
        position.x = measure_counted_text(field->text, field->length);

    total = 0;
    index = 0;
    if (index < field->length) {
        do {
            width = measure_char(field->text[index]);
            if ((int)((Word)width >> 1) + total > position.x)
                break;
            total += width;
        } while (++index < field->length);
    }
    field->caret = index;
    update_edit_caret(field);
    paint_edit_caret(field, 1);
}

EditField far * far pascal create_edit_field(Rect far *bounds, Rect far *clip,
                                             char far *text, Word capacity)
{
    EditField far *field = alloc_far_block(sizeof(EditField));

    int zero;

    field->font = (field->port = current_port)->font;
    move_far_bytes(&field->bounds, bounds, (long)sizeof(Rect));
    move_far_bytes(&field->clip, clip, (long)sizeof(Rect));
    field->capacity = capacity;
    field->style = -1;
    zero = 0;
    field->selection_end = zero;
    field->selection_start = zero;
    field->caret = zero;
    field->x = bounds->left - 1;
    field->y = clip->top;
    field->visible = zero;
    field->last_blink = clock_ticks;
    field->paint = 0;
    replace_edit_text(field, text);
    return field;
}

void far pascal replace_edit_text(EditField far *field, char far *text)
{
    field->text = text;
    field->length = far_string_length(text);
    field->caret = 0;
    update_edit_caret(field);
}

void far pascal redraw_edit_text(EditField far *field)
{
    Pen pen;

    copy_pen(&pen);
    push_clip(&field->clip);
    start_rect_effect(&field->bounds);
    MoveTo(field->bounds.left, field->bounds.bottom);
    if (field->paint)
        field->paint(field);
    else
        draw_text(&field->bounds, field->style, 1, field->text);
    pop_clip();
    restore_pen(&pen);
}
