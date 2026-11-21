typedef signed char sbyte;
typedef unsigned char byte;

typedef struct {
    int edges[4];
} MoveRect;

typedef struct {
    int dirty;
    MoveRect rect;
} MarkedRect;

typedef void (far *CellMark)(sbyte cell, MoveRect *rect);

extern MarkedRect top_marks[], move_marks[], floor_marks[], foreground_marks[];
extern int half_marks[], redraw_marks[];
extern byte viewed_room, left_room, right_room, upper_room, lower_room;
extern byte marked_room;
extern sbyte marked_cell;

extern int rect_empty(MoveRect far *rect);
extern sbyte room_cell(sbyte column, sbyte row);
extern void get_cell_rect(MoveRect far *out, int column, int row);
extern int intersect_rect(MoveRect far *out, MoveRect far *a, MoveRect far *b);
extern void union_rect(MoveRect far *out, MoveRect far *a, MoveRect far *b);

void mark_rect_buf(MarkedRect *mark, int cell, MoveRect *clip);
void mark_guard_updates(MoveRect *clip, sbyte except);

void mark_top(int cell, MoveRect *clip)
{
    MoveRect rect;

    if (cell <= -1 && cell >= -10) {
        get_cell_rect(&rect, -((cell + 1) % 10), -1);
        if (!clip || intersect_rect(&rect, &rect, clip)) {
            ++cell;
            cell = -cell;
            if (top_marks[cell].dirty)
                union_rect(&top_marks[cell].rect, &top_marks[cell].rect, &rect);
            else {
                top_marks[cell].dirty = 1;
                top_marks[cell].rect = rect;
            }
        }
    }
}

void mark_move(sbyte cell, MoveRect *clip)
{
    if (cell < 30) {
        if (cell >= 0)
            mark_rect_buf(&move_marks[cell], cell, clip);
        else
            mark_top(cell, clip);
        mark_guard_updates(clip, -1);
    }
}

void mark_floor(sbyte cell, MoveRect *clip)
{
    if (cell < 30) {
        if (cell >= 0) {
            mark_rect_buf(&floor_marks[cell], cell, clip);
            return;
        }
        mark_top(cell, clip);
    }
}

void mark_half(sbyte cell)
{
    if (cell < 30) {
        if (cell >= 0) {
            half_marks[cell] = 1;
            return;
        }
        mark_top(cell, 0);
    }
}

void mark_redraw(int cell)
{
    if (cell < 30) {
        if (cell >= 0)
            *(int *)((byte *)redraw_marks + (cell << 1)) = 1;
        else
            mark_top(cell, 0);
    }
}

void mark_cell_redraw(void)
{
    sbyte cell = marked_cell;

    if (viewed_room == marked_room)
        ;
    else if (right_room == marked_room)
        cell -= 9;
    else if (left_room == marked_room)
        cell += 9;
    else if (lower_room == marked_room)
        cell -= 20;
    else if (upper_room == marked_room)
        cell += 20;
    mark_redraw(cell);
}

void mark_foreground(sbyte cell, MoveRect *clip)
{
    if (cell < 30) {
        if (cell >= 0) {
            mark_rect_buf(&foreground_marks[cell], cell, clip);
            return;
        }
        mark_top(cell, clip);
    }
}

void mark_bufs_for_rect(CellMark mark, MoveRect *rect, sbyte except)
{
    struct {
        int top, left, bottom, right;
    } bounds;
    sbyte row, column;

    if (!rect_empty(rect)) {
        if (rect->edges[0] >= 3)
            bounds.top = (rect->edges[0] - 3) / 63;
        else
            bounds.top = -1;
        if (rect->edges[2] >= 3)
            bounds.bottom = (rect->edges[2] - 3) / 63;
        else
            bounds.bottom = -1;
        bounds.left = rect->edges[1] / 32;
        bounds.right = rect->edges[3] / 32;
        for (row = bounds.top; row <= bounds.bottom; ++row) {
            for (column = bounds.left; column <= bounds.right; ++column)
                mark(room_cell(column, row), rect);
        }
        mark_guard_updates(rect, except);
    }
}

#pragma pack(1)
typedef struct {
    sbyte id;
    byte before_rect[26];
    MoveRect rect;
    byte before_update[19];
    int update;
    byte tail[8];
} GuardMark;
#pragma pack()

typedef struct {
    sbyte count;
    byte unused[115];
} RoomGuardCount;

extern GuardMark guard_marks[];
extern RoomGuardCount room_guard_counts[];

void mark_guard_updates(MoveRect *clip, sbyte except)
{
    MoveRect hit;
    sbyte count = room_guard_counts[viewed_room].count;
    int i;

    for (i = 0; i < count; ++i) {
        if (guard_marks[i].id != except)
            guard_marks[i].update |= intersect_rect(&hit, &guard_marks[i].rect, clip);
    }
}

void mark_rect_buf(MarkedRect *mark, int cell, MoveRect *clip)
{
    MoveRect rect;

    get_cell_rect(&rect, cell % 10, cell / 10);
    if (intersect_rect(&rect, &rect, clip)) {
        if (mark->dirty)
            union_rect(&mark->rect, &mark->rect, &rect);
        else {
            mark->dirty = 1;
            mark->rect = rect;
        }
    }
}
