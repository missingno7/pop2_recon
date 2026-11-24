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

extern Port *Graf;
extern unsigned long TimeVal;
extern Word timer_rate;
extern unsigned char near cdecl _ctype[];
#define isprint(c) ((_ctype + 1)[(c)] & 0x57)
extern void far pascal COPY_CURRENT_PORT_ORIGIN(Pen far *);
extern void far pascal PUSH_PORT_CLIP(Rect far *);
extern void far pascal MoveTo(int, int);
extern void far pascal DRAW_RELATIVE_AXIS_SEGMENT(int, int);
extern void far pascal DRAW_NUL_TEXT_IN_RECT(Rect far *, int, int, char far *);
extern void far pascal POP_PORT_CLIP(void);
extern void far pascal SET_RELATIVE_PEN_STATE(Pen far *);
extern Word far pascal hSetFont(Word);
extern int far pascal measure_counted_text(char far *, Word);
extern void far pascal InsetRect(Rect far *, Rect far *, int, int);
extern void far pascal frame_rect(Rect far *);
extern void far pascal CLEAR_PAYLOAD_HEADER(void far *block);
extern void far * far pascal RESOLVE_HANDLE_TO_FAR_PAYLOAD(Word);
extern int far pascal MEASURE_CHARACTER(unsigned char);
extern int far pascal MEASURE_NUL_TERMINATED_TEXT(char far *);
extern void far pascal play_sound(int);
extern void far pascal CLAMP_POINT_TO_RECT(Point far *, void far *, Rect far *);
extern EditField far * far pascal ALLOCATE_PINNED_PAYLOAD(long);
extern void far pascal MOVE_HUGE_BYTES(void far *, const void far *, long);
extern Word far cdecl _fstrlen(const char far *);
extern void far pascal STROKE_PORT_RECT(Rect far *);

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
    saved_font = Graf->font;
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

    COPY_CURRENT_PORT_ORIGIN(&pen);
    PUSH_PORT_CLIP(&field->clip);
    MoveTo(field->x, field->y);
    if (!visible) {
        field->visible = visible;
        saved_mode = Graf->pen_mode;
        Graf->pen_mode = field->port->erase_mode;
    } else {
        field->visible = 1;
    }
    DRAW_RELATIVE_AXIS_SEGMENT(0, field->clip.bottom - field->clip.top + 1);
    if (!visible) {
        Graf->pen_mode = saved_mode;
        if (field->paint)
            field->paint(field);
        else
            DRAW_NUL_TEXT_IN_RECT(&field->bounds, field->style, 1, field->text);
    }
    POP_PORT_CLIP();
    SET_RELATIVE_PEN_STATE(&pen);
}

void far pascal frame_edit_field(EditField far *field, Word mode)
{
    Rect rect;
    Word saved_mode = Graf->pen_mode;

    Graf->pen_mode = mode;
    InsetRect(&rect, &field->clip, -1, -1);
    frame_rect(&rect);
    Graf->pen_mode = saved_mode;
}

/* Forward the far pointer to the separately reviewed block-flag helper. */
void far pascal CLEAR_FLAGS_OF_FAR_BLOCK(void far *block)
{
    CLEAR_PAYLOAD_HEADER(block);
}

void far pascal blink_edit_caret(EditField far *field)
{
    if ((timer_rate >> 1) < TimeVal - field->last_blink) {
        paint_edit_caret(field, field->visible = !field->visible);
        field->last_blink = TimeVal;
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
            font = RESOLVE_HANDLE_TO_FAR_PAYLOAD(field->font);
            if (isprint(key)
                && MEASURE_CHARACTER(key) != *(int far *)(font + 8)) {
                if (field->capacity == field->length
                    || field->clip.right - field->clip.left
                       < MEASURE_CHARACTER(key) + MEASURE_NUL_TERMINATED_TEXT(field->text)) {
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
        play_sound(-2);

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
    CLAMP_POINT_TO_RECT(&position, point, &field->bounds);
    position.x -= field->bounds.left;
    if (measure_counted_text(field->text, field->length) <= position.x)
        position.x = measure_counted_text(field->text, field->length);

    total = 0;
    index = 0;
    if (index < field->length) {
        do {
            width = MEASURE_CHARACTER(field->text[index]);
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
    EditField far *field = ALLOCATE_PINNED_PAYLOAD(sizeof(EditField));

    int zero;

    field->font = (field->port = Graf)->font;
    MOVE_HUGE_BYTES(&field->bounds, bounds, (long)sizeof(Rect));
    MOVE_HUGE_BYTES(&field->clip, clip, (long)sizeof(Rect));
    field->capacity = capacity;
    field->style = -1;
    zero = 0;
    field->selection_end = zero;
    field->selection_start = zero;
    field->caret = zero;
    field->x = bounds->left - 1;
    field->y = clip->top;
    field->visible = zero;
    field->last_blink = TimeVal;
    field->paint = 0;
    replace_edit_text(field, text);
    return field;
}

void far pascal replace_edit_text(EditField far *field, char far *text)
{
    field->text = text;
    field->length = _fstrlen(text);
    field->caret = 0;
    update_edit_caret(field);
}

void far pascal redraw_edit_text(EditField far *field)
{
    Pen pen;

    COPY_CURRENT_PORT_ORIGIN(&pen);
    PUSH_PORT_CLIP(&field->clip);
    STROKE_PORT_RECT(&field->bounds);
    MoveTo(field->bounds.left, field->bounds.bottom);
    if (field->paint)
        field->paint(field);
    else
        DRAW_NUL_TEXT_IN_RECT(&field->bounds, field->style, 1, field->text);
    POP_PORT_CLIP();
    SET_RELATIVE_PEN_STATE(&pen);
}
