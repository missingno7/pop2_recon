/* DRAW.C, resident frame 078a. Six functions in original order. */
typedef unsigned char Room;
typedef struct RoomLinks {
    Room left, right, above, below;
} RoomLinks;

extern unsigned char redraw_flags[10];
extern Room drawn_room, left_room, right_room, above_room, below_room;
extern Room above_left_room, above_right_room, below_left_room, below_right_room;
extern char room_is_custom;
extern RoomLinks room_links[];
extern char draw_row, draw_column;

extern void begin_full_redraw(void);
extern void prepare_full_redraw(void);
extern void finish_full_redraw(void);
extern void select_room(Room room);
extern int check_custom_room(Room room);
extern void begin_room_rows(void);
extern void prepare_row(void);
extern void prepare_row_images(void);
extern void select_draw_cell(void);
extern void draw_custom_above_row(void);
extern void redraw_room_sure(void);
extern void redraw_cell_sure(void);

void * cdecl memset(void *, int, unsigned int);
#pragma intrinsic(memset)

void full_redraw(void)
{
    begin_full_redraw();
    memset(redraw_flags, 0, sizeof redraw_flags);
    prepare_full_redraw();
    redraw_room_sure();
    finish_full_redraw();
}

void get_rooms(void)
{
    above_left_room = above_right_room = below_left_room = below_right_room = 0;
    if (drawn_room) {
        select_room(drawn_room);
        left_room = room_links[drawn_room].left;
        right_room = room_links[drawn_room].right;
        above_room = room_links[drawn_room].above;
        below_room = room_links[drawn_room].below;

        if (above_room) {
            above_left_room = room_links[above_room].left;
            above_right_room = room_links[above_room].right;
        } else {
            if (left_room)
                above_left_room = room_links[left_room].above;
            if (right_room)
                above_right_room = room_links[right_room].above;
        }
        if (below_room) {
            below_left_room = room_links[below_room].left;
            below_right_room = room_links[below_room].right;
        } else {
            if (left_room)
                below_left_room = room_links[left_room].below;
            if (right_room)
                below_right_room = room_links[right_room].below;
        }
        room_is_custom = check_custom_room(drawn_room);
    } else {
        left_room = right_room = above_room = below_room = 0;
    }
}

void redraw_room_sure(void)
{
    int saved_room;

    begin_room_rows();
    draw_row = 3;
    while (draw_row--) {
        prepare_row();
        prepare_row_images();
        draw_column = 0;
        do {
            select_draw_cell();
            redraw_cell_sure();
        } while (++draw_column < 10);
    }
    if (room_is_custom == 0) {
        saved_room = drawn_room;
        drawn_room = above_room;
        get_rooms();
        below_room = (Room)saved_room;
        begin_room_rows();
        draw_row = 2;
        prepare_row();
        prepare_row_images();
        draw_row = -1;
        draw_column = 0;
        do {
            select_draw_cell();
            redraw_cell_sure();
        } while (++draw_column < 10);
        drawn_room = (Room)saved_room;
        get_rooms();
    } else {
        draw_custom_above_row();
    }
}

/* The remaining routines are drafts. Later same-file callees are still declarations. */
extern void draw_back_image(char pass);
extern void draw_fore_image(void);

void redraw_cell_sure(void)
{
    draw_back_image(0);
    draw_back_image(5);
    draw_back_image(11);
    draw_fore_image();
}

typedef struct DrawRect {
    int top, left, bottom, right;
} DrawRect;

typedef struct DirtyCell {
    int pending;
    DrawRect area;
} DirtyCell;

extern DrawRect clip_rect;
extern int redraw_whole_cell[], redraw_half_cell[];
extern unsigned char redraw_cell_objects[];
extern DirtyCell redraw_back_cells[], redraw_floor_cells[];
extern DirtyCell redraw_fore_cells[], redraw_top_cells[];

extern int intersect_rect(DrawRect far *out, const DrawRect far *a,
                          const DrawRect far *b);
extern void union_rect(DrawRect far *out, const DrawRect far *a,
                       const DrawRect far *b);
extern int compute_half_rect(DrawRect *area);
extern void draw_half_cell(void);
extern void draw_floor_buffer(void);
extern void draw_cell_objects(char column);

#define DIRTY_CELL(table, byte_offset) \
    (*(DirtyCell *)((char *)(table) + (byte_offset)))
#define WORD_FLAG(table, byte_offset) \
    (*(int *)((char *)(table) + (byte_offset)))

void redraw_cell_fast(char column)
{
    int dirty_offset;
    int cell = column;
    int word_offset = cell * sizeof(int);
    DrawRect old_clip;

    if (WORD_FLAG(redraw_whole_cell, word_offset)) {
        WORD_FLAG(redraw_whole_cell, word_offset) = 0;
        redraw_cell_sure();
    } else {
        dirty_offset = cell * sizeof(DirtyCell);
        if (DIRTY_CELL(redraw_back_cells, dirty_offset).pending) {
            DIRTY_CELL(redraw_back_cells, dirty_offset).pending = 0;
            old_clip = clip_rect;
            if (intersect_rect(&clip_rect, &clip_rect, &DIRTY_CELL(redraw_back_cells, dirty_offset).area)) {
                draw_back_image(0);
                draw_back_image(5);
                draw_back_image(11);
                draw_fore_image();
            }
            clip_rect = old_clip;
        }
    }
    dirty_offset = cell * sizeof(DirtyCell);
    if (DIRTY_CELL(redraw_floor_cells, dirty_offset).pending) {
        DIRTY_CELL(redraw_floor_cells, dirty_offset).pending = 0;
        if (WORD_FLAG(redraw_half_cell, word_offset) && compute_half_rect(&old_clip))
            union_rect(&DIRTY_CELL(redraw_floor_cells, dirty_offset).area,
                       &DIRTY_CELL(redraw_floor_cells, dirty_offset).area, &old_clip);
        old_clip = clip_rect;
        if (intersect_rect(&clip_rect, &clip_rect, &DIRTY_CELL(redraw_floor_cells, dirty_offset).area))
            draw_floor_buffer();
        clip_rect = old_clip;
    } else if (WORD_FLAG(redraw_half_cell, word_offset)) {
        WORD_FLAG(redraw_half_cell, word_offset) = 0;
        draw_half_cell();
    }
    if (redraw_cell_objects[cell]) {
        draw_cell_objects(column);
        redraw_cell_objects[cell] = 0;
    }
    if (DIRTY_CELL(redraw_fore_cells, dirty_offset).pending) {
        DIRTY_CELL(redraw_fore_cells, dirty_offset).pending = 0;
        if (intersect_rect(&clip_rect, &clip_rect, &DIRTY_CELL(redraw_fore_cells, dirty_offset).area))
            draw_fore_image();
    }
}

void redraw_top_fast(char column)
{
    int dirty_offset = column * sizeof(DirtyCell);
    char old_row;
    DrawRect old_clip;

    if (DIRTY_CELL(redraw_top_cells, dirty_offset).pending) {
        DIRTY_CELL(redraw_top_cells, dirty_offset).pending = 0;
        old_row = draw_row;
        old_clip = clip_rect;
        clip_rect = DIRTY_CELL(redraw_top_cells, dirty_offset).area;
        draw_row = -1;
        draw_back_image(0);
        draw_back_image(5);
        draw_back_image(11);
        draw_fore_image();
        draw_row = old_row;
        clip_rect = old_clip;
    }
}
