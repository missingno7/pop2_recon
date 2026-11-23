/* DRAW.C in original routine order. The verification receipt identifies the exact entries. */
typedef unsigned char Room;
typedef struct RoomLinks {
    Room left, right, above, below;
} RoomLinks;

extern unsigned char redraw_flags[10];
extern Room viewed_room, left_room, right_room, above_room, below_room;
extern Room above_left_room, above_right_room, below_left_room, below_right_room;
extern char room_is_custom;
extern RoomLinks room_links[];
extern char draw_row, draw_column;

extern void clear_peels(void);
extern void init_fchar_rect(void);
extern void ZeroBuffers(void);
extern void select_room(Room room);
extern int IsCustomizedRoom(Room room);
extern void SetSideRoomInfo(void);
extern void prepare_row(void);
extern void GetBelowAry(void);
extern void select_draw_cell(void);
extern void draw_custom_above_row(void);
extern void redraw_room_sure(void);
extern void redraw_cell_sure(void);

void * cdecl memset(void *, int, unsigned int);
#pragma intrinsic(memset)

void full_redraw(void)
{
    clear_peels();
    memset(redraw_flags, 0, sizeof redraw_flags);
    init_fchar_rect();
    redraw_room_sure();
    ZeroBuffers();
}

void get_rooms(void)
{
    above_left_room = above_right_room = below_left_room = below_right_room = 0;
    if (viewed_room) {
        select_room(viewed_room);
        left_room = room_links[viewed_room].left;
        right_room = room_links[viewed_room].right;
        above_room = room_links[viewed_room].above;
        below_room = room_links[viewed_room].below;

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
        room_is_custom = IsCustomizedRoom(viewed_room);
    } else {
        left_room = right_room = above_room = below_room = 0;
    }
}

void redraw_room_sure(void)
{
    int saved_room;

    SetSideRoomInfo();
    draw_row = 3;
    while (draw_row--) {
        prepare_row();
        GetBelowAry();
        draw_column = 0;
        do {
            select_draw_cell();
            redraw_cell_sure();
        } while (++draw_column < 10);
    }
    if (room_is_custom == 0) {
        saved_room = viewed_room;
        viewed_room = above_room;
        get_rooms();
        below_room = (Room)saved_room;
        SetSideRoomInfo();
        draw_row = 2;
        prepare_row();
        GetBelowAry();
        draw_row = -1;
        draw_column = 0;
        do {
            select_draw_cell();
            redraw_cell_sure();
        } while (++draw_column < 10);
        viewed_room = (Room)saved_room;
        get_rooms();
    } else {
        draw_custom_above_row();
    }
}

extern void draw_back_image(unsigned char pass);
extern void draw_fore_image(void);

void redraw_cell_sure(void)
{
    draw_back_image(0);
    draw_back_image(5);
    draw_back_image(11);
    draw_fore_image();
}

/* The following definitions are drafts. Several later same-file callees still need bodies. */
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

extern int SECTRECT(DrawRect far *out, const DrawRect far *a,
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
    DrawRect old_clip;
    struct { int dirty_offset, cell, word_offset; } state;

    state.cell = column;
    state.word_offset = state.cell * sizeof(int);

    if (WORD_FLAG(redraw_whole_cell, state.word_offset)) {
        WORD_FLAG(redraw_whole_cell, state.word_offset) = 0;
        redraw_cell_sure();
    } else {
        state.dirty_offset = state.cell * sizeof(DirtyCell);
        if (DIRTY_CELL(redraw_back_cells, state.dirty_offset).pending) {
            DIRTY_CELL(redraw_back_cells, state.dirty_offset).pending = 0;
            old_clip = clip_rect;
            if (SECTRECT(&clip_rect, &clip_rect, &DIRTY_CELL(redraw_back_cells, state.dirty_offset).area)) {
                draw_back_image(0);
                draw_back_image(5);
                draw_back_image(11);
                draw_fore_image();
            }
            clip_rect = old_clip;
        }
    }
    state.dirty_offset = state.cell * sizeof(DirtyCell);
    if (DIRTY_CELL(redraw_floor_cells, state.dirty_offset).pending) {
        DIRTY_CELL(redraw_floor_cells, state.dirty_offset).pending = 0;
        if (WORD_FLAG(redraw_half_cell, state.word_offset) && compute_half_rect(&old_clip))
            union_rect(&DIRTY_CELL(redraw_floor_cells, state.dirty_offset).area,
                       &DIRTY_CELL(redraw_floor_cells, state.dirty_offset).area, &old_clip);
        old_clip = clip_rect;
        if (SECTRECT(&clip_rect, &clip_rect, &DIRTY_CELL(redraw_floor_cells, state.dirty_offset).area))
            draw_floor_buffer();
        clip_rect = old_clip;
    } else if (WORD_FLAG(redraw_half_cell, state.word_offset)) {
        WORD_FLAG(redraw_half_cell, state.word_offset) = 0;
        draw_half_cell();
    }
    if (redraw_cell_objects[state.cell]) {
        draw_cell_objects(column);
        redraw_cell_objects[state.cell] = 0;
    }
    if (DIRTY_CELL(redraw_fore_cells, state.dirty_offset).pending) {
        DIRTY_CELL(redraw_fore_cells, state.dirty_offset).pending = 0;
        if (SECTRECT(&clip_rect, &clip_rect, &DIRTY_CELL(redraw_fore_cells, state.dirty_offset).area))
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

/* Packed tile and image records used by the drawing passes. */
#pragma pack(1)
typedef union TileInfo {
    unsigned long bits;
    unsigned int words[2];
    unsigned char bytes[4];
} TileInfo;

typedef struct Tile {
    signed char kind;
    TileInfo info;
} Tile;

typedef struct TileDrawArgs {
    char pass;
    signed char column, row, kind;
    TileInfo info;
} TileDrawArgs;

typedef struct RoomImage {
    unsigned char type;
    int y, x;
    char pass, flip;
    int shape, upper;
    DrawRect clip;
    unsigned char attributes, sheet;
    unsigned char reserved[4];
} RoomImage;

typedef struct RoomImages {
    signed char count;
    unsigned char flags;
    int first_shape;
    unsigned char reserved[8];
    int floor_image;
    int background_shape;
    unsigned char reserved_tail[12];
    RoomImage images[1];
} RoomImages;

typedef struct DrawEntry {
    unsigned char type, sheet;
    int shape, x, y;
    signed char column, row;
    DrawRect clip;
    unsigned char flip, attributes;
} DrawEntry;
#pragma pack()

typedef void (far pascal *TileDrawer)(TileDrawArgs *args);
extern TileDrawer *tile_drawers;
extern RoomImages *room_images;
extern TileInfo BlueSpec;
extern signed char drawn_tile_kind, CellLeft;
extern TileInfo left_tile_info;
extern Tile above_tiles[], below_tiles[];
extern int force_full_clip;
extern DrawRect GamePlayRect;
extern unsigned char level_style, level_number;
extern int back_count, fore_count;
extern DrawEntry far back_queue[];
extern DrawEntry far fore_queue[];

extern int far pascal IsWall(char kind);
extern unsigned char back_image_start_pass(void);
extern void draw_custom_room_images(char pass);
extern void far _fastcall draw_custom_room_image(char image, char pass);
extern void check_foreground_gate(void);
extern int NeedDrawRoomBelowCustom(void);
extern void load_tower_top(void);
extern int far _fastcall keep_image_in_back(RoomImage *image);
extern int draw_pillar_in_back(RoomImage *image);
extern void draw_horse(unsigned int sheet, unsigned int shape, char pass);
extern unsigned int far _fastcall horse_room_image(Room room, char pass, unsigned int *sheet);

void draw_back_image(unsigned char pass)
{
    unsigned char layer;
    unsigned char priority;
    struct { unsigned long address; TileDrawArgs args; } draw;

    layer = back_image_start_pass();
    if (force_full_clip)
        clip_rect = GamePlayRect;
    draw.args.pass = pass;
    do {
        switch (layer) {
        case 0:
            draw.args.column = draw_column - 1;
            draw.args.row = draw_row + 1;
            draw.args.kind = below_tiles[draw_column].kind;
            draw.args.info = below_tiles[draw_column].info;
            if (!force_full_clip || draw_column == 0 || draw_column == 9 ||
                level_style == 3 || draw.args.kind == 4 || level_number == 13)
                break;
            if (level_style == 2 && (draw.args.kind == 19 || draw.args.kind == 9))
                break;
            layer += 2;
            break;
        case 1:
            draw.args.row = draw_row + 1;
            draw.args.column = draw_column;
            draw.args.kind = below_tiles[draw_column + 1].kind;
            draw.args.info = below_tiles[draw_column + 1].info;
            if (!IsWall(draw.args.kind))
                break;
            ++layer;
            goto draw_left;
        case 3:
            draw_custom_room_images(pass);
            draw.args.column = draw_column;
            draw.args.row = draw_row;
            draw.args.kind = drawn_tile_kind;
            draw.args.info = BlueSpec;
            break;
        case 2:
        draw_left:
            draw.args.column = draw_column - 1;
            draw.args.row = draw_row;
            draw.args.kind = CellLeft;
            draw.args.info = left_tile_info;
            break;
        }
        priority = (draw.args.info.words[0] >> 14) & 3;
        if (!room_images || !priority || priority == pass || (priority == 2 && pass == 5)) {
            draw.address = (unsigned long)tile_drawers[draw.args.kind];
            if (draw.address)
                ((TileDrawer)draw.address)(&draw.args);
        }
    } while (++layer < 4);
}

unsigned char back_image_start_pass(void)
{
    unsigned char pass;
    int custom_edge;

    custom_edge = force_full_clip &&
        ((drawn_tile_kind != 7 && IsWall(drawn_tile_kind) &&
          CellLeft != 7 && IsWall(CellLeft)) ||
         (draw_column != 0 && draw_row != 2));
    if (custom_edge)
        pass = 3;
    else if ((force_full_clip && draw_column == 0 && draw_row != 2) ||
             (room_images && draw_row == 2 && !NeedDrawRoomBelowCustom()))
        pass = 2;
    else
        pass = 0;
    return pass;
}

void draw_custom_room_images(char pass)
{
    volatile signed char image;
    unsigned int sheet;
    unsigned int draw_images = 0;

    if ((pass == 0 || pass == 5) && force_full_clip &&
        draw_column == 0 && draw_row == 2) {
        draw_images = 1;
        if (viewed_room == 1 && level_number == 14)
            load_tower_top();
    } else if ((pass == 0 && !force_full_clip && room_images &&
                room_images->background_shape) ||
               ((pass == 1 || pass == 2) &&
                ((draw_column == 9 && draw_row == 0) || !force_full_clip))) {
        draw_images = 1;
    }
    if (draw_images) {
        if (room_images) {
            image = 0;
            if (room_images->count > 0) {
                do {
                    draw_custom_room_image(image, pass);
                    ++image;
                } while (room_images->count > image);
            }
        }
        if ((force_full_clip && pass == 0) || pass == 1) {
            draw_images = horse_room_image(viewed_room, pass, &sheet);
            if (draw_images)
                draw_horse(sheet, draw_images, pass);
        }
    }
}


/* The foreground queue follows 130 background entries in the shared segment. */
typedef struct DrawQueueStorage {
    DrawEntry background[130];
    DrawEntry foreground[100];
} DrawQueueStorage;
void far _fastcall draw_custom_room_image(char index, char pass)
{
    RoomImage *image;
    DrawRect clip;
    DrawEntry far *entry;

    image = &room_images->images[index];
    if (image->shape == -1 || image->pass != pass ||
        !SECTRECT(&clip, &image->clip, &clip_rect))
        return;
    if (pass != 0 && pass != 5 && pass != 11) {
        entry = ((_segment)fore_queue) :>
            (DrawEntry _based(void) *)&((DrawQueueStorage near *)0)->foreground[fore_count++];
        if (level_style != 5)
            goto queued;
        if ((level_style == 5 && viewed_room != 19 && viewed_room != 16 &&
             viewed_room != 15 && keep_image_in_back(image)) ||
            ((viewed_room == 19 || viewed_room == 16) && draw_pillar_in_back(image))) {
            entry = &back_queue[(*(int *)redraw_flags)++];
            --fore_count;
        }
    } else {
        if (!image->upper)
            return;
        entry = &back_queue[(*(int *)redraw_flags)++];
    }
queued:
    entry->y = image->y;
    entry->x = image->x;
    entry->shape = image->shape;
    entry->type = 4;
    entry->column = draw_column;
    entry->row = draw_row;
    entry->clip = clip;
    entry->flip = image->flip;
    entry->sheet = image->sheet;
}

void draw_fore_image(void)
{
    unsigned char layer, priority;
    int skip_left;
    struct { unsigned long address; TileDrawArgs args; } draw;

    check_foreground_gate();
    if (force_full_clip && draw_column)
        skip_left = 1;
    else
        skip_left = 0;
    layer = skip_left != 0;
    draw.args.pass = 1;
    do {
        switch (layer) {
        case 0:
            draw.args.column = draw_column - 1;
            draw.args.row = draw_row;
            draw.args.kind = CellLeft;
            draw.args.info = left_tile_info;
            break;
        case 1:
            draw.args.column = draw_column;
            draw.args.row = draw_row;
            draw.args.kind = drawn_tile_kind;
            draw.args.info = BlueSpec;
            if (skip_left && level_style != 3)
                ++layer;
            break;
        case 2:
            draw.args.row = draw_row - 1;
            draw.args.column = draw_column;
            draw.args.kind = above_tiles[draw_column + 1].kind;
            draw.args.info = above_tiles[draw_column + 1].info;
            break;
        }
        priority = (draw.args.info.words[0] >> 14) & 3;
        if (!room_images || !priority || priority == 1 || priority == 2) {
            draw.address = (unsigned long)tile_drawers[draw.args.kind];
            if (draw.address)
                ((TileDrawer)draw.address)(&draw.args);
        }
    } while (++layer < 3 && (layer < 2 || draw_row != -1));
    draw_custom_room_images(1);
}

#pragma pack(1)
typedef struct GateRoom {
    unsigned char header[37];
    signed char count;
    unsigned char contents[78];
} GateRoom;

typedef struct GateImage {
    unsigned char header[11];
    DrawRect clip;
} GateImage;
#pragma pack()

#pragma pack(1)
struct char_bounds { int top, left, bottom, right; };
typedef struct {
    signed char slot;                  /* 00 */
    signed char direction;             /* 01 */
    int x, y;                          /* 02, 04 */
    signed char character_kind;        /* 06 */
    unsigned int frame;                /* 07 */
    signed char column, row;           /* 09, 0a */
    signed char action;                /* 0b */
    signed char horizontal_step;       /* 0c */
    signed char vertical_step;         /* 0d */
    signed char room;                  /* 0e */
    signed char field_0f;              /* 0f */
    signed char stance;                /* 10 */
    signed char alive;                 /* 11 */
    signed char life_points;           /* 12 */
    signed char life_initial;          /* 13 */
    signed char life_delta;            /* 14 */
    int sequence_position;             /* 15 */
    int sequence_id;                   /* 17 */
    unsigned int sequence;             /* 19 */
    struct char_bounds bounds;                    /* 1b: top, left, bottom, right */
    signed char alert_delay;           /* 23 */
    int sequence_group;                /* 24 */
    int previous_x, previous_y;        /* 26, 28 */
    unsigned int previous_frame;       /* 2a */
    struct char_bounds previous_bounds;           /* 2c: top, left, bottom, right */
    int field_34;                      /* 34 */
    int previous_mode;                 /* 36 */
    unsigned char behavior;            /* 38 */
    signed char opponent_index;        /* 39 */
    unsigned char field_3a;            /* 3a */
    unsigned char unknown_3b[5];        /* 3b..3f */
} CHARREC;
#pragma pack()

extern CHARREC Kid;
extern GateRoom gate_rooms[];
extern unsigned char gate_number, gate_kind;
extern signed char gate_column, gate_row;
extern TileDrawer draw_gate;
extern void draw_special_gate(TileDrawArgs *args);
extern void examine_first_gate(void);
extern void examine_gate(char index);
extern GateImage *get_gate_image(char type, unsigned char index);
extern int empty_rect(DrawRect far *rect);

void check_foreground_gate(void)
{
    unsigned char found;
    signed char index, count;
    int matches;
    TileDrawArgs args;
    struct {
        volatile int distance;
        DrawRect saved_clip;
    } state;
    GateImage *image;

    if (Kid.frame >= 264 && Kid.frame <= 266) {
        if (level_style == 3) {
            args.pass = 0;
            args.row = draw_row;
            if (drawn_tile_kind == 4) {
                args.column = draw_column;
                args.info = BlueSpec;
                draw_special_gate(&args);
            } else if (CellLeft == 4) {
                args.column = draw_column - 1;
                args.info = left_tile_info;
                draw_special_gate(&args);
            }
        }
        return;
    }
    if (CellLeft != 4)
        return;
    state.saved_clip = clip_rect;
    examine_first_gate();
    matches = CellLeft == 4 && gate_row == draw_row &&
              ((state.distance = draw_column - gate_column) - 1 == 0 || state.distance == 2) &&
              gate_kind != 2 && gate_kind != 6;
    if (matches) {
        found = 1;
        image = get_gate_image(2, 0);
    } else {
        count = gate_rooms[viewed_room].count;
        for (index = 0; index < count && !matches; ++index) {
            examine_gate(index);
            matches = CellLeft == 4 && gate_row == draw_row &&
                      ((state.distance = draw_column - gate_column) - 1 == 0 || state.distance == 2) &&
                      gate_kind != 2 && gate_kind != 6;
        }
        found = matches;
        if (found)
            image = get_gate_image(3, gate_number);
    }
    if (found) {
        args.pass = 2;
        args.column = draw_column - 1;
        args.row = draw_row;
        args.info = left_tile_info;
        if (draw_gate) {
            if (force_full_clip)
                clip_rect.top = 63 * args.row + 3;
            else if (image)
                SECTRECT(&clip_rect, &clip_rect, &image->clip);
            if (!empty_rect(&clip_rect))
                draw_gate(&args);
        }
    }
    clip_rect = state.saved_clip;
}

extern _segment room_queue_segment;
extern DrawEntry _based(room_queue_segment) back_entries[];
extern unsigned char _based(room_queue_segment) back_entry_modes[];
extern void SaveCustomRoomPeels(void);
extern void draw_back_fore_item(unsigned char pass, unsigned char entry);
extern void draw_middle_item(int entry);

void dump_draw_table(unsigned char pass)
{
    unsigned char index, count;
    int prepared;

    if (!force_full_clip || !room_images)
        prepared = 1;
    else
        prepared = 0;
    index = 0;
    count = redraw_flags[2 * pass];
    if (count) {
        do {
            if (pass == 3) {
                draw_middle_item(index);
            } else {
                if (!prepared && back_entry_modes[index * sizeof(DrawEntry)]) {
                    SaveCustomRoomPeels();
                    prepared = 1;
                }
                draw_back_fore_item(pass, index);
            }
        } while (++index < count);
    }
}

#pragma pack(1)
typedef struct ShapeBitmap {
    int width, height, stride;
    unsigned char data[1];
} ShapeBitmap;

typedef struct ShapeRef {
    unsigned char kind, reserved, separate_mask, palette;
    unsigned int handle;
} ShapeRef;

typedef struct ShapeTable {
    unsigned int reserved[2];
    int count;
} ShapeTable;

typedef struct MiddleEntry {
    int x, y;
    signed char type;
    int shape;
    DrawRect clip;
    unsigned char attributes;
    int mode;
    unsigned char palette, update_peel;
} MiddleEntry;
#pragma pack()

extern ShapeTable *shape_tables[];
extern MiddleEntry far middle_queue[];
extern _segment middle_queue_segment;
extern MiddleEntry _based(middle_queue_segment) middle_entries[];
extern ShapeBitmap far *lock_shape(unsigned int handle);
extern ShapeBitmap far *locate_shape(unsigned int handle);
extern unsigned int mark_shape(unsigned int handle);
extern void restore_shape(unsigned int handle, unsigned int mark);
extern void load_shape_ref(ShapeTable *table, int image, ShapeRef far *reference);
extern void release_shape_ref(ShapeRef far *reference);
extern void customize_entry_clip(RoomImage *image, DrawRect far *rect);
extern ShapeBitmap far *load_custom_shape(RoomImage *image);
extern ShapeBitmap far *far _fastcall load_room_middle_shape(int image, ShapeRef *ref);
extern ShapeBitmap far *far _fastcall load_animated_shape(int index, ShapeRef *ref);
extern ShapeBitmap far *far _fastcall load_horse_shape(int id, ShapeRef *ref);
extern void prepare_animated_shapes(void);
extern void set_shape_variant(int variant);
extern void remember_shape(int shape);
extern void apply_shape_palette(unsigned char far *data, int width, int stride);
extern void apply_custom_shape_palette(unsigned char far *data, int width, int stride);
extern void apply_shape_scheme(unsigned char far *data, int width, int height, int stride);
extern void set_draw_clip(DrawRect far *rect);
extern void restore_draw_clip(void);
extern void far _fastcall update_middle_peel(int x, int height, int y, int width, char mode, char flags);
extern void draw_bitmap(ShapeBitmap far *image, ShapeBitmap far *mask, int x, int y,
                        int flip, int mode, unsigned int flags);

void draw_back_fore_item(unsigned char pass, unsigned char index)
{
    DrawEntry far *entry;
    ShapeTable *table;
    ShapeBitmap far *image;
    ShapeBitmap far *mask;
    RoomImage *custom;
    ShapeRef ref;
    int mode, image_number;
    unsigned char initial_type;
    unsigned int flags;
    if (pass == 0)
        entry = room_queue_segment :> (DrawEntry _based(void) *)&back_entries[index];
    else
        entry = room_queue_segment :> (DrawEntry _based(void) *)&fore_queue[index];
    mode = 2;
    custom = 0;
    initial_type = entry->type;
    table = shape_tables[initial_type];
    image_number = entry->shape;
    if (table->count >= image_number) {
        if (initial_type == 1) {
            mode = 3;
            flags = 0x4000;
        } else if (initial_type == 0) {
            mode = 3;
            flags = 0x8000;
        } else {
            flags = 0;
        }
        load_shape_ref(table, image_number, &ref);
        image = lock_shape(ref.handle);
        if (!image)
            return;
    } else {
        flags = 0;
        if (!room_images && (entry->shape < 0x6370 || entry->shape > 0x6372))
            return;
        if (room_images) {
            custom = &room_images->images[entry->shape - room_images->first_shape];
            if (custom->sheet == 1)
                customize_entry_clip(custom, &entry->clip);
        }
        if (entry->shape >= 0x6370 && entry->shape <= 0x6372) {
            image = load_horse_shape(entry->shape, &ref);
            entry->shape = 0;
        } else if (entry->shape >= 0x62d7 && entry->shape <= 0x62e2) {
            prepare_animated_shapes();
            image = load_animated_shape(entry->shape - 0x62d7, &ref);
            ref.palette = entry->attributes;
        } else {
            if (room_images) {
                image = load_custom_shape(custom);
                ref.palette = entry->attributes;
            }
        }
        ref.separate_mask = 0;
    }
    if (entry->attributes != ref.palette) {
        if (ref.separate_mask)
            apply_shape_palette(image->data, image->width, image->stride);
        else if (ref.kind == 2)
            apply_shape_scheme(image->data, image->width, image->height, image->stride);
        else if (ref.kind == 1)
            apply_custom_shape_palette(image->data, image->width, image->stride);
        if (entry->shape < 0x6370 || entry->shape > 0x6372)
            ((unsigned char *)shape_tables[entry->type])[6 * entry->shape + 3] = entry->attributes;
    }
    if (ref.separate_mask) {
        mask = image;
        image = 0;
    } else {
        mask = 0;
    }
    set_draw_clip(&entry->clip);
    draw_bitmap(image, mask, entry->x, entry->y, entry->flip, mode, flags);
    restore_draw_clip();
    if (shape_tables[entry->type]->count > entry->shape)
        release_shape_ref(&ref);
    else if (custom && custom->pass == 0 && custom->attributes == 0)
        remember_shape(custom->upper);
}

void draw_middle_item(int index)
{
    MiddleEntry _based(middle_queue_segment) *entry;
    ShapeRef ref;
    ShapeBitmap far *image;
    signed char type;
    unsigned char far *palette;
    unsigned int mark, flags;
    int mode, flip, x, height, width;

    entry = &middle_entries[index];
    type = middle_queue[index].type;
    if (middle_queue[index].shape >= 0x62d7 && middle_queue[index].shape <= 0x62e2) {
        load_animated_shape(entry->shape - 0x62d7, &ref);
    } else if (level_number == 5 && viewed_room == 10 && type == 4 &&
               entry->shape < room_images->count) {
        load_room_middle_shape(entry->shape, &ref);
    } else {
        if (type == 2 && entry->mode)
            set_shape_variant(entry->mode);
        load_shape_ref(shape_tables[type], entry->shape + 1, &ref);
        if (type == 2 && entry->mode)
            set_shape_variant(2);
    }
    flip = middle_entries[index].attributes;
    mark = mark_shape(ref.handle);
    image = locate_shape(ref.handle);
    x = entry->x;
    palette = &entry->palette;
    if (*palette)
        x -= image->height;
    if (type == 2) {
        flags = 0;
        mode = 1;
    } else if (type == 4) {
        flags = 0;
        mode = 2;
    } else {
        if (type == 0)
            flags = 0x8000;
        else if (type == 1)
            flags = 0x4000;
        else
            flags = entry->mode;
        mode = 3;
    }
    if (*palette != ref.palette) {
        if (mode == 1)
            apply_shape_palette(image->data, image->width, image->stride);
        else if (mode == 2)
            apply_custom_shape_palette(image->data, image->width, image->stride);
        else
            apply_shape_scheme(image->data, image->width, image->height, image->stride);
        ((unsigned char *)shape_tables[type])[6 * entry->shape + 9] = *palette;
    }
    entry = &middle_entries[index];
    height = x + image->height;
    if (height > entry->clip.right)
        height = entry->clip.right;
    width = entry->clip.bottom - entry->y;
    if (width > image->width)
        width = image->width;
    update_middle_peel(x, height, entry->y, width, entry->update_peel, 0);
    set_draw_clip(&middle_queue[index].clip);
    draw_bitmap(image, 0, x, entry->y, flip, mode, flags);
    restore_draw_clip();
    restore_shape(ref.handle, mark);
    release_shape_ref(&ref);
}

typedef struct DrawDestination {
    unsigned char header[24];
    DrawRect clip;
} DrawDestination;

extern DrawDestination *Graf;
extern char missing_shape_format[];
extern int cdecl format_draw_error(char *out, const char *fmt, ...);
extern void report_draw_error(int severity, const char far *message);
extern void copy_shape(ShapeBitmap far *image, int x, int y, int flip);
extern void copy_shape_mode(ShapeBitmap far *image, int x, int y, int flip, unsigned int flags);
extern void copy_shape_special(ShapeBitmap far *image, int x, int y, int flip);
extern void copy_shape_mask(ShapeBitmap far *mask, int x, int y, int color, int flip);
extern void add_dirty_rectangle(DrawRect *rect);

void draw_bitmap(ShapeBitmap far *image, ShapeBitmap far *mask, int x, int y,
                 int flip, int mode, unsigned int flags)
{
    typedef struct BitmapSize {
        int width, height;
    } BitmapSize;
    DrawRect area;
    BitmapSize size;
    char message[150];

    if (!image && !mask) {
        format_draw_error(message, missing_shape_format, x, y, flip, mode);
        report_draw_error(1, message);
    }
    if (image) {
        switch (flip) {
        case 0:
        case 10:
            if (mode == 2) {
                copy_shape_special(image, x, y, flip);
                break;
            }
            if (mode == 3) {
                copy_shape_mode(image, x, y, flip, flags);
                break;
            }
        case 2:
        case 3:
        case 8:
            copy_shape(image, x, y, flip);
            break;
        }
        size = *(BitmapSize far *)image;
    } else {
        copy_shape_mask(mask, x, y, 0, flip);
        size = *(BitmapSize far *)mask;
    }
    if (!force_full_clip) {
        area.right = area.left = x;
        area.right += size.height;
        area.bottom = area.top = y;
        area.bottom += size.width;
        if (SECTRECT(&area, &area, &Graf->clip))
            add_dirty_rectangle(&area);
    }
}

extern void add_mobs(void);
extern void add_characters(void);
extern void mark_peel_front_buffers(void);
extern void redraw_room_fast(void);
extern signed char FirstInRow(signed char row);

void full_redraw_fast(void)
{
    add_mobs();
    add_characters();
    mark_peel_front_buffers();
    redraw_room_fast();
}

void redraw_room_fast(void)
{
    Room saved_room;

    SetSideRoomInfo();
    draw_cell_objects(30);
    draw_row = 3;
    goto next_row;
paint_row:
    prepare_row();
    GetBelowAry();
    draw_column = 0;
next_cell:
    select_draw_cell();
    redraw_cell_fast((signed char)(FirstInRow(draw_row) + draw_column));
    if (++draw_column < 10)
        goto next_cell;
next_row:
    if (draw_row--)
        goto paint_row;
    if (!room_is_custom) {
        saved_room = viewed_room;
        viewed_room = above_room;
        get_rooms();
        SetSideRoomInfo();
        draw_row = 2;
        prepare_row();
        GetBelowAry();
        draw_column = 0;
        do {
            select_draw_cell();
            redraw_top_fast(draw_column);
        } while (++draw_column < 10);
        viewed_room = saved_room;
        get_rooms();
    } else {
        draw_custom_above_row();
    }
    draw_cell_objects(-1);
}

extern DrawDestination *room_destination;
extern DrawDestination ScreenPort;
extern int draw_room_frame;
extern int dirty_rectangle_count;
extern DrawRect dirty_rectangles[40];
extern char dirty_rectangle_error[];
extern void SET_RECORD_RANGE(DrawRect far *area, int style);
extern void sort_back_table(void);
extern void sort_fore_table(void);
extern void prepare_room_tables(void);
extern DrawRect far *INSETRECT(DrawRect far *out, const DrawRect far *area, int x, int y);
extern void report_rect_error(const char far *message, int severity);

void draw_all(void)
{
    dirty_rectangle_count = 0;
    Graf = room_destination;
    if (draw_room_frame)
        SET_RECORD_RANGE(&GamePlayRect, 12);
    if (level_style == 4 || level_style == 2 || level_style == 5)
        sort_back_table();
    if ((viewed_room == 1 && level_number == 14) ||
        (viewed_room == 22 && level_number == 10))
        sort_fore_table();
    prepare_room_tables();
    dump_draw_table(0);
    dump_draw_table(3);
    dump_draw_table(1);
    Graf = &ScreenPort;
}

void add_dirty_rectangle(DrawRect *rect)
{
    DrawRect expanded;
    DrawRect *entry;
    int index = 0;

    if (dirty_rectangle_count > index) {
        entry = dirty_rectangles;
        do {
            INSETRECT(&expanded, rect, -1, -1);
            if (SECTRECT(&expanded, &expanded, entry))
                goto merge;
            ++entry;
            ++index;
        } while (dirty_rectangle_count > index);
    }
    if (dirty_rectangle_count >= 40) {
        report_rect_error(dirty_rectangle_error, 1);
        goto done;
    }
    goto append;
merge:
    entry = &dirty_rectangles[index];
    union_rect(entry, entry, rect);
    goto done;
append:
    dirty_rectangles[dirty_rectangle_count++] = *rect;
done:
    return;
}

extern int compute_slide_rock_rect(DrawRect *area, Room room, signed char row);
extern void mark_foreground(signed char cell, DrawRect *area);

extern struct LooseItemDrawer *loose_item_drawer;

void draw_half_cell(void)
{
    unsigned char priority;
    DrawRect area;
    struct {
        unsigned long address;
        DrawRect old_clip, row_clip;
        TileDrawArgs args;
    } draw;

    if (!compute_half_rect(&area))
        return;
    if (level_style == 2 && compute_slide_rock_rect(&draw.row_clip, viewed_room, draw_row) &&
        SECTRECT(&draw.row_clip, &draw.row_clip, &area))
        area.top = draw.row_clip.bottom - 1;
    draw.old_clip = clip_rect;
    clip_rect = area;
    draw.args.pass = 2;
    draw.args.column = draw_column;
    draw.args.row = draw_row;
    draw.args.info = BlueSpec;
    priority = (draw.args.info.words[0] >> 14) & 3;
    if (!room_is_custom || priority == 2) {
        draw.address = (unsigned long)((TileDrawer *)loose_item_drawer)[(unsigned char)drawn_tile_kind];
        if (draw.address) {
            ((TileDrawer)draw.address)(&draw.args);
            mark_foreground((signed char)(FirstInRow(draw_row) + draw_column), &area);
        }
    }
    if (room_images && room_images->floor_image)
        draw_custom_room_images(2);
    clip_rect = draw.old_clip;
}

extern CHARREC Char;
extern CHARREC RoomOpp[5];
typedef struct GuardMeterRoom {
    unsigned char present;
    unsigned char rest[115];
} GuardMeterRoom;

extern GuardMeterRoom guard_meter_rooms[];
static const DrawRect _based(_segname("_CONST")) half_tile_rectangles[10] = {
    {57, 0, 63, 16},
    {57, 0, 63, 16},
    {57, 0, 63, 17},
    {57, 0, 63, 20},
    {57, 0, 63, 15},
    {57, 0, 63, 16},
    {57, 0, 63, 19},
    {59, 0, 63, 14},
    {59, 0, 63, 13},
    {59, 0, 63, 13},
};
extern DrawRect *ComputeOffsetCellRect(DrawRect *area, DrawRect *source, int column, int row);

int compute_half_rect(DrawRect *area)
{
    signed char index;
    unsigned int image;
    struct { int byte_offset; signed char count; DrawRect source; } state;

    if (Kid.frame >= 135 && Kid.frame < 145) {
        image = Kid.frame;
    } else {
        image = 0;
        index = 0;
        state.count = guard_meter_rooms[(Room)Char.room].present;
        if (state.count > 0) {
            do {
                if (image)
                    break;
                state.byte_offset = index * sizeof(CHARREC);
                if (((CHARREC *)((char *)RoomOpp + state.byte_offset))->character_kind == 1 &&
                    ((CHARREC *)((char *)RoomOpp + state.byte_offset))->frame >= 135 &&
                    ((CHARREC *)((char *)RoomOpp + state.byte_offset))->frame < 145)
                    image = ((CHARREC *)((char *)RoomOpp + state.byte_offset))->frame;
                ++index;
            } while (index < *(volatile signed char *)&state.count);
        }
    }
    if (image) {
        state.source = half_tile_rectangles[image - 135];
        if (drawn_tile_kind == 6)
            ++state.source.bottom;
        if ((Char.room == 7 || Char.room == 8) && level_style == 6)
            state.source.top += 3;
        area = ComputeOffsetCellRect(area, &state.source, draw_column, draw_row);
    }
    return (int)area;
}

extern int far pascal IS_SIX_KIND(char kind);

void draw_floor_buffer(void)
{
    DrawRect old_clip, row_clip;
    TileDrawArgs args;
    unsigned char priority;
    int floor_edge, floor_y;
    struct { TileDrawer *current_draw, *left_draw; } callbacks;

    old_clip = clip_rect;
    floor_y = 63 * draw_row + 53;
    if (floor_y > clip_rect.top)
        clip_rect.top = floor_y;
    floor_edge = 0;
    if (level_style == 2) {
        if (compute_slide_rock_rect(&row_clip, viewed_room, draw_row) &&
            SECTRECT(&row_clip, &row_clip, &clip_rect)) {
            clip_rect.top = row_clip.bottom - 1;
            floor_edge = 1;
        } else if (compute_slide_rock_rect(&row_clip, viewed_room, (signed char)(draw_row + 1)) &&
                   SECTRECT(&row_clip, &row_clip, &clip_rect)) {
            floor_edge = 1;
        }
    }
    if (IS_SIX_KIND(CellLeft)) {
        priority = (left_tile_info.words[0] >> 14) & 3;
        if (!room_is_custom || priority == 2 || !priority) {
            callbacks.left_draw = &tile_drawers[(unsigned char)CellLeft];
            if (*callbacks.left_draw) {
                args.pass = 2;
                args.column = draw_column - 1;
                args.row = draw_row;
                args.kind = CellLeft;
                args.info = left_tile_info;
                (*callbacks.left_draw)(&args);
            }
        }
        priority = (BlueSpec.words[0] >> 14) & 3;
        if (!room_is_custom || priority == 2 || !priority) {
            callbacks.current_draw = &tile_drawers[(unsigned char)drawn_tile_kind];
            if (*callbacks.current_draw) {
                args.pass = 2;
                args.column = draw_column;
                args.row = draw_row;
                args.kind = drawn_tile_kind;
                args.info = BlueSpec;
                (*callbacks.current_draw)(&args);
                mark_foreground((signed char)(FirstInRow(draw_row) + draw_column), &clip_rect);
            }
        }
        if (room_images && room_images->floor_image)
            draw_custom_room_images(2);
    } else if (floor_edge && IS_SIX_KIND(drawn_tile_kind) && IsWall(CellLeft)) {
        priority = (left_tile_info.words[0] >> 14) & 3;
        if (!room_is_custom || priority == 2) {
            callbacks.left_draw = &tile_drawers[(unsigned char)CellLeft];
            if (*callbacks.left_draw) {
                args.pass = 2;
                args.column = draw_column - 1;
                args.row = draw_row;
                args.kind = CellLeft;
                args.info = left_tile_info;
                (*callbacks.left_draw)(&args);
            }
        }
    }
    clip_rect = old_clip;
}

#pragma pack(1)
typedef struct CellObject {
    int x, y;
    unsigned char set;
    int image;
    unsigned char object_id;
    signed char cell;
    unsigned char flags, kind;
    DrawRect clip;
    signed char status;
    int depth;
    unsigned char type;
} CellObject;
#pragma pack()

extern CellObject cell_objects[];
extern signed char sorted_objects[];
extern signed char cell_object_total;
extern int sorted_object_total;
extern int equal_rect(DrawRect far *first, DrawRect far *second);
extern void sort_cell_objects(void);
extern void draw_cell_object(signed char index);

void draw_cell_objects(char column)
{
    struct {
        int byte_offset;
        DrawRect original_clip, object_clip, merged_clip;
        signed char count, index;
    } state;

    state.count = cell_object_total;
    if (!state.count)
        return;
    state.original_clip = clip_rect;
    state.merged_clip = clip_rect;
    sorted_object_total = 0;
    if ((state.index = state.count - 1) >= 0) {
        do {
            state.byte_offset = state.index * sizeof(CellObject);
            if (((CellObject *)((char *)cell_objects + state.byte_offset))->status == column ||
                (((CellObject *)((char *)cell_objects + state.byte_offset))->status >= 0 &&
                 SECTRECT(&state.object_clip, &state.merged_clip,
                                &((CellObject *)((char *)cell_objects + state.byte_offset))->clip))) {
                sorted_objects[sorted_object_total++] = state.index;
                union_rect(&state.object_clip, &state.merged_clip,
                           &((CellObject *)((char *)cell_objects + state.byte_offset))->clip);
                if (!equal_rect(&state.object_clip, &state.merged_clip)) {
                    state.index = state.count;
                    sorted_object_total = 0;
                    state.merged_clip = state.object_clip;
                }
            }
        } while (--state.index >= 0);
    }
    if (sorted_object_total) {
        clip_rect = state.merged_clip;
        sort_cell_objects();
        state.index = 0;
        if (sorted_object_total > 0) {
            do {
                int index;
                unsigned char object = sorted_objects[state.index];
                index = state.index;
                draw_cell_object(object);
                cell_objects[sorted_objects[index]].status = -2;
            } while (++state.index < sorted_object_total);
        }
    }
    clip_rect = state.original_clip;
}

extern int compare_objects(int first, int second);
extern unsigned char far _fastcall object_priority(unsigned char type);
static unsigned char object_priority_low[16] = {
    127, 127, 127, 100, 127, 50, 127, 127, 127, 100, 127, 127, 127, 200, 200, 0
};
static unsigned char object_priority_high[13] = {
    127, 127, 127, 127, 200, 60, 225, 50, 200, 80, 210, 210, 60
};
extern unsigned char load_draw_object(signed char index);
extern void draw_loose_object(unsigned char type);

void sort_cell_objects(void)
{
    int swapped, index, last;

    last = sorted_object_total - 1;
    do {
        index = 0;
        swapped = 0;
        if (last > 0) {
            do {
                int next = index + 1;
                if (compare_objects(index, next)) {
                    int value = sorted_objects[index];
                    sorted_objects[index] = sorted_objects[index + 1];
                    sorted_objects[index + 1] = (signed char)value;
                    swapped = 1;
                }
                index = next;
            } while (index < last);
        }
    } while (swapped);
}

int compare_objects(int first, int second)
{
    volatile int left_offset, right_offset;
    int left_id, right_id;
    unsigned char left_type, right_type;
    unsigned char right_priority, left_priority;
    volatile int swap;
    int left_entry = first, right_entry = second;

#define SORTED_AT(offset) (*(CellObject *)((char *)cell_objects + (offset)))
    left_offset = sorted_objects[left_entry] * sizeof(CellObject);
    left_id = *(int *)&SORTED_AT(left_offset).object_id;
    left_type = SORTED_AT(left_offset).kind;
    right_offset = sorted_objects[right_entry] * sizeof(CellObject);
    right_id = *(int *)&SORTED_AT(right_offset).object_id;
    right_type = SORTED_AT(right_offset).kind;
    if (left_type == right_type) {
        if (SORTED_AT(left_offset).y != SORTED_AT(right_offset).y)
            goto less_y;
        if (SORTED_AT(left_offset).x > SORTED_AT(right_offset).x)
            goto before;
        goto after;
    }
    if (left_type != 7 && left_type != 8 &&
        (right_type == 7 || right_type == 8) &&
        (right_id == 181 || right_id == 166)) {
        swap = 1;
        goto done;
    }
    if ((left_type == 7 || left_type == 8) &&
        right_type != 7 && right_type != 8 &&
        (left_id == 181 || left_id == 166)) {
        swap = 0;
        goto done;
    }
    left_priority = object_priority(left_type);
    right_priority = object_priority(right_type);
    if (left_priority != right_priority) {
        if (left_priority < right_priority)
            goto before;
        goto after;
    }
    left_offset = sorted_objects[left_entry] * sizeof(CellObject);
    right_offset = sorted_objects[right_entry] * sizeof(CellObject);
    if (SORTED_AT(left_offset).y == SORTED_AT(right_offset).y) {
        if (SORTED_AT(left_offset).x > SORTED_AT(right_offset).x)
            goto before;
        goto after;
    }
    if ((left_type & 0x80) || (right_type & 0x80)) {
        if (SORTED_AT(left_offset).y < SORTED_AT(right_offset).y)
            goto before;
        goto after;
    }
less_y:
    if (SORTED_AT(left_offset).y <= SORTED_AT(right_offset).y)
        goto after;
before:
    swap = 1;
    goto done;
after:
    swap = 0;
done:
#undef SORTED_AT
    return swap;
}

unsigned char far _fastcall object_priority(unsigned char type)
{
    unsigned char result;

    if (type & 0x80)
        result = object_priority_high[type & 0x7f];
    else
        result = object_priority_low[type];
    return result;
}

GateImage *get_gate_image(char set, unsigned char type)
{
    CellObject *match = 0;
    int index = 0;
    int total = *(int *)&cell_object_total;
    unsigned char *selector;

    if (total > 0) {
        selector = &cell_objects[0].set;
        do {
            if (match)
                break;
            if (*selector == (unsigned char)set && selector[6] == type)
                match = (CellObject *)(selector - 4);
            selector += sizeof(CellObject);
        } while (++index < total);
    }
    return (GateImage *)match;
}

extern signed char FChar, active_object_flags;
extern int active_object_x, active_object_y, active_object_image, active_object_offset;
extern unsigned char active_object_set, active_object_type;
extern DrawRect active_object_clip;
extern int active_object_depth, counter_tick, middle_count;
extern int far _fastcall add_middle_image(unsigned char set, int image, int x, int y, unsigned char order);
extern void draw_overlay_object_13(unsigned char type);
extern void draw_overlay_object_84(unsigned char type);
extern void draw_overlay_object_86(unsigned char type);
extern void draw_overlay_object_8a(unsigned char type);
extern void draw_overlay_object_8b(unsigned char type);
extern void draw_overlay_object_8c(unsigned char type);

void draw_cell_object(signed char index)
{
    unsigned char type = load_draw_object(index);

    switch (type) {
    case 0:
        if (active_object_image != -1 &&
            (counter_tick == 0 || counter_tick % 2 != 0)) {
            add_middle_image(active_object_set, active_object_image + 1,
                             active_object_offset, active_object_y, 10);
            if (active_object_depth) {
                middle_entries[middle_count - 1].mode = active_object_depth;
            }
        }
        break;
    case 1: case 2: case 3:
    case 5: case 6: case 7: case 8: case 9: case 10: case 14:
        if (add_middle_image(active_object_set, active_object_image + 1,
                             active_object_offset, active_object_y, 10)) {
            middle_queue[middle_count - 1].mode = active_object_depth;
            middle_queue[middle_count - 1].update_peel = active_object_type;
        }
        break;
    case 13:
        draw_overlay_object_13(type);
        break;
    case 0x80: case 0x81: case 0x83:
        draw_loose_object(type);
        break;
    case 0x82:
        add_middle_image(active_object_set, 103, active_object_offset, active_object_y, 10);
        return;
    case 0x84: case 0x85:
        draw_overlay_object_84(type);
        return;
    case 0x86: case 0x87: case 0x88: case 0x89:
        draw_overlay_object_86(type);
        return;
    case 0x8a:
        draw_overlay_object_8a(type);
        return;
    case 0x8b:
        draw_overlay_object_8b(type);
        return;
    case 0x8c:
        draw_overlay_object_8c(type);
        break;
    }
}

void draw_loose_object(unsigned char type)
{
    int vertical_gap, horizontal_gap, left_shape, right_shape;

    switch (level_style) {
    case 2:
        left_shape = 21;
        right_shape = 22;
        vertical_gap = horizontal_gap = 0;
        break;
    case 3:
        left_shape = (type & 127) + 66;
        right_shape = (type & 127) + 70;
        horizontal_gap = 1;
        vertical_gap = 4;
        active_object_clip.bottom += 5;
        break;
    case 4:
        left_shape = 22;
        right_shape = 26;
        vertical_gap = horizontal_gap = 0;
        break;
    default:
        left_shape = 0;
        break;
    }
    if (left_shape) {
        active_object_flags = -1;
        add_middle_image(4, left_shape, active_object_offset + horizontal_gap,
                         active_object_y, 10);
        add_middle_image(4, right_shape, active_object_offset,
                         active_object_y + vertical_gap, 10);
    }
}

unsigned char load_draw_object(signed char index)
{
    CellObject *item = &cell_objects[index];

    FChar = item->status;
    active_object_offset = active_object_x = item->x;
    active_object_y = item->y;
    active_object_image = item->image;
    active_object_set = item->set;
    active_object_flags = item->flags;
    active_object_clip = item->clip;
    active_object_depth = item->depth;
    active_object_type = item->type;
    return item->kind;
}

void set_object_cell(signed char index)
{
    cell_objects[index].status = FChar;
    if (FChar < 30)
        redraw_cell_objects[FChar] = 1;
}

extern _segment fore_queue_segment;
extern void far *far cdecl move_far_memory(void far *out, const void far *src, unsigned count);
extern void sort_fore_overlay(void);

void sort_back_table(void)
{
    int source, dest, first_offset, sheet_offset;
    DrawEntry item;

    source = dest = 0;
    if (back_count > 0) {
        first_offset = 0;
        sheet_offset = 1;
        do {
            if (!*(room_queue_segment :> (unsigned char _based(void) *)sheet_offset)) {
                if (source != dest) {
                    item = *(room_queue_segment :> (DrawEntry _based(void) *)(sheet_offset - 1));
                    move_far_memory(room_queue_segment :> (void _based(void) *)(first_offset + sizeof(DrawEntry)),
                                    room_queue_segment :> (void _based(void) *)first_offset,
                                    (source - dest) * sizeof(DrawEntry));
                    *(room_queue_segment :> (DrawEntry _based(void) *)first_offset) = item;
                }
                first_offset += sizeof(DrawEntry);
                ++dest;
            }
            sheet_offset += sizeof(DrawEntry);
        } while (++source < back_count);
    }
}

void sort_fore_table(void)
{
    int source, dest, first_offset, shape_offset, shape;
    DrawEntry item;

    shape = 0;
    if (level_style == 6) {
        if ((viewed_room == 1 && level_number == 14) ||
            (viewed_room == 22 && level_number == 10))
            shape = 0x6372;
        else
            shape = -1;
    }
    if (shape == -1) {
        sort_fore_overlay();
        return;
    }
    if (shape) {
        source = dest = 0;
        if (fore_count > 0) {
            first_offset = (int)&fore_queue[0];
            shape_offset = (int)&fore_queue[0].shape;
            do {
                if (*(fore_queue_segment :> (int _based(void) *)shape_offset) == shape) {
                    if (source != dest) {
                        item = *(((_segment)fore_queue) :> (DrawEntry _based(void) *)(shape_offset - 2));
                        move_far_memory(((_segment)fore_queue) :> (void _based(void) *)(first_offset + sizeof(DrawEntry)),
                                        ((_segment)fore_queue) :> (void _based(void) *)first_offset,
                                        (source - dest) * (int)sizeof(DrawEntry));
                        *(((_segment)fore_queue) :> (DrawEntry _based(void) *)first_offset) = item;
                    }
                    first_offset += sizeof(DrawEntry);
                    ++dest;
                }
                shape_offset += sizeof(DrawEntry);
            } while (++source < fore_count);
        }
    }
}

extern DrawRect message_rect;
extern int suppress_restart, restart_style, message_timeout, message_counter;
extern char restart_key_message2[];
extern char restart_key_message1[];
extern void clear_message(int reset_timer);
extern void clear_life_bar(void);
extern void DRAW_NUL_TEXT_IN_RECT(DrawRect far *area, int color, int mode, const char far *text);
extern void STROKE_PORT_RECT(DrawRect far *area);
extern void PrinceCopy(DrawRect far *area);
extern char *cdecl strcpy(char *out, const char *in);
extern char *cdecl strupr(char *str);
#pragma intrinsic(strcpy)

void draw_message(const char *message)
{
    char text[100];

    strcpy(text, message);
    strupr(text);
    clear_message(1);
    DRAW_NUL_TEXT_IN_RECT(&message_rect, 0, 1, text);
}

void draw_restart_message(void)
{
    DrawRect area;

    if (!suppress_restart) {
        clear_life_bar();
        INSETRECT(&area, &message_rect, -message_rect.left, 0);
        area.right = 320;
        if (restart_style == 2)
            DRAW_NUL_TEXT_IN_RECT(&area, 0, 1, restart_key_message2);
        else
            DRAW_NUL_TEXT_IN_RECT(&area, 0, 1, restart_key_message1);
    }
}

void clear_life_bar(void)
{
    DrawRect area;

    INSETRECT(&area, &message_rect, -message_rect.left, 0);
    area.right = 320;
    STROKE_PORT_RECT(&area);
}

void clear_message(int reset_timer)
{
    DrawRect area;

    if (message_counter == 600) {
        INSETRECT(&area, &message_rect, -message_rect.left, 0);
        area.right = 320;
        SET_RECORD_RANGE(&area, 0);
    } else {
        SET_RECORD_RANGE(&message_rect, 0);
    }
    if (reset_timer)
        message_timeout = message_counter = 0;
}

void copy_dirty_rectangles(void)
{
    while (dirty_rectangle_count--)
        PrinceCopy(&dirty_rectangles[dirty_rectangle_count]);
    dirty_rectangle_count = 0;
}

#pragma pack(1)
typedef struct HorseShapeHeader {
    int mode;
    unsigned char palette;
    unsigned char payload[96];
} HorseShapeHeader;
#pragma pack()

extern ShapeRef horse_shape_ref;
extern int suppress_horse;
extern unsigned int load_resource_shape(unsigned long type, int shape);
extern unsigned int derive_resource_id(int shape);
extern unsigned int create_shape(HorseShapeHeader far *out, HorseShapeHeader far *header, int count);
extern unsigned int retain_shape(unsigned int handle);
extern ShapeBitmap far *probe_shape(unsigned int handle);
extern void free_shape(unsigned int handle);
static const int _based(_segname("_CONST")) horse_level9_origin[] = {22, 159};
static const int _based(_segname("_CONST")) horse_level10_origin[] = {102, 119};
extern int horse_level14_origin[];

ShapeBitmap far *far _fastcall load_horse_shape(int id, ShapeRef *ref)
{
    HorseShapeHeader header;
    ShapeBitmap far *result;
    unsigned int handle, new_handle;
    HorseShapeHeader far *metadata;

    if (!horse_shape_ref.handle || !probe_shape(horse_shape_ref.handle)) {
        if (horse_shape_ref.handle)
            free_shape(horse_shape_ref.handle);
        handle = load_resource_shape(0x53484150UL, id);
        if (handle) {
            metadata = (HorseShapeHeader far *)lock_shape(derive_resource_id(0x61a9));
            header = *(HorseShapeHeader far *)((char far *)metadata + 4);
            header.palette = 0xf0;
            header.mode = -2;
            new_handle = create_shape((HorseShapeHeader far *)locate_shape(handle), &header, 1);
            free_shape(handle);
            /* The first word is kind/reserved, the second mask/palette. */
            *(unsigned int *)&horse_shape_ref.kind = 1;
            *(unsigned int *)&horse_shape_ref.separate_mask = 0;
            horse_shape_ref.handle = retain_shape(new_handle);
        }
    }
    *ref = horse_shape_ref;
    if (horse_shape_ref.handle)
        result = lock_shape(horse_shape_ref.handle);
    else
        result = 0;
    return result;
}

unsigned int far _fastcall horse_room_image(Room room, char pass, unsigned int *shape)
{
    unsigned int origin = 0;

    if (pass == 0) {
        if (room == 2 && level_number == 9) {
            origin = (unsigned int)horse_level9_origin;
            *shape = 0x6371;
            goto chosen;
        }
    } else if (pass == 1 && !suppress_horse) {
        if (room == 1 && level_number == 14) {
            origin = (unsigned int)horse_level14_origin;
            *shape = 0x6372;
            goto chosen;
        }
        if (room == 22 && level_number == 10) {
            origin = (unsigned int)horse_level10_origin;
            *shape = 0x6370;
        }
    }
chosen:
    return origin;
}

void draw_horse(unsigned int shape, unsigned int origin, char pass)
{
    DrawEntry far *entry;
    int *point;

    if (!shape)
        return;
    point = (int *)origin;
    if (point) {
        if (pass == 0)
            entry = &back_queue[(*(int *)redraw_flags)++];
        else
            entry = ((_segment)fore_queue) :>
                (DrawEntry _based(void) *)&((DrawQueueStorage near *)0)->foreground[fore_count++];
        entry->attributes = 0;
        entry->y = point[0];
        entry->x = point[1];
        entry->shape = shape;
        entry->type = 4;
        entry->column = draw_column;
        entry->row = draw_row;
        entry->clip = clip_rect;
        entry->flip = 10;
        entry->sheet = drawn_tile_kind;
    }
}


typedef struct LooseItemDrawer {
    TileDrawer base, item;
} LooseItemDrawer;

extern LooseItemDrawer *loose_item_drawer;
extern int *middle_draw_state;
extern void add_middle_item(unsigned char sheet, int image, char column, char row,
                            unsigned char color, unsigned char flags);
extern unsigned char flask_bubble_color(unsigned int attributes);

void draw_flask(TileDrawArgs *args)
{
    int color, tall;
    unsigned char bubble, flags;
    TileDrawArgs base;

    if (loose_item_drawer->item && !room_images) {
        base = *args;
        base.info.bytes[0] = 0;
        loose_item_drawer->item(&base);
    }
    if (args->pass != 1)
        return;
    color = args->info.words[0] & 0xe0;
    tall = color < 0xa0 && color >= 0x40 ? 1 : 0;
    add_middle_item(10, tall + 1, args->column, args->row, 10, 0);
    if (color) {
        color = args->info.words[0] & 0x1f;
        if (color == 28 || (color >= 10 && color <= 13) || color - 14 >= 10)
            return;
        bubble = flask_bubble_color(args->info.words[0]);
        if (color > 13) {
            int first_bubble = color - 14;
            color = first_bubble;
            flags = 1;
        } else {
            flags = 0;
        }
        *(int *)((char *)middle_draw_state + 0x485) = tall < 1 ? -14 : -17;
        add_middle_item(60, color + 3, args->column, args->row, bubble, flags);
        add_middle_item(60, color + 13, args->column, args->row, bubble + 1, flags);
    }
}

unsigned char flask_bubble_color(unsigned int attributes)
{
    unsigned char color;

    switch ((unsigned int)attributes & 0xe0) {
    default:
        color = 0xec;
        break;
    case 0x20:
    case 0x40:
        color = 0xea;
        break;
    case 0xa0:
        color = 0xee;
        break;
    }
    return color;
}

extern int meter_sheet;
extern signed char meter_phase, meter_limit;
extern DrawRect meter_strip;
extern void blit_meter_shape(int table, int image, int x, int y, int flags, int mode);
extern void load_kid_with_opponent(signed char guard);
extern CHARREC Opp;
extern void draw_opponent_meter(signed char guard, int life, int max_life);

void draw_kid_meter(int life, int max_life)
{
    int x = 2, index, visible, mode = 1;
    struct { DrawRect rect; volatile int blank_start, full_cells; } state;

    state.blank_start = 0;
    if ((visible = life) > 0) {
        state.full_cells = visible;
        index = visible;
        do {
            blit_meter_shape(meter_sheet, 216, x, 193, 0, mode);
            x += 8;
        } while (--index);
    }
    if (life < max_life) {
        index = max_life - life;
        do {
            blit_meter_shape(meter_sheet, 217, x, 193, 0, mode);
            x += 8;
        } while (--index);
    }
    if (max_life > 1 && meter_limit < meter_phase) {
        state.rect.top = 193;
        state.rect.left = x;
        state.rect.bottom = meter_strip.bottom;
        state.rect.right = 98;
        STROKE_PORT_RECT(&state.rect);
    }
}

void draw_meters(void)
{
    if (Kid.opponent_index >= 0 && Kid.opponent_index < 5) {
        load_kid_with_opponent(Kid.opponent_index);
        draw_opponent_meter(Opp.slot, Opp.life_points, Opp.life_initial);
    }
    draw_kid_meter(Kid.life_points, Kid.life_initial);
}

extern unsigned char guard_kind, guard_level_room, guard_action;
extern int game_tick_counter;
extern signed char find_guard_for_meter(unsigned char slot);
extern void select_guard_for_meter(signed char guard);

void draw_opponent_meter(signed char guard, int life, int max_life)
{
    DrawRect rect;
    int count, x, half;
    volatile int full_cells, origin;

    if (guard == -1) {
        guard = find_guard_for_meter(1);
        if (guard == -1 && guard_meter_rooms[viewed_room].present)
            guard = 0;
    }
    if (guard != -1)
        select_guard_for_meter(guard);
    if (guard == -1 || !guard_meter_rooms[viewed_room].present ||
        guard_kind == 10 || guard_kind == 11 || guard_kind == 0 ||
        (guard_kind == 4 && level_number == 5 && guard_level_room == 1 &&
         (guard_action == 10 || guard_action == 7 || guard_action == 12))) {
        rect.top = 193; rect.left = 218;
        rect.bottom = meter_strip.bottom; rect.right = meter_strip.right;
        STROKE_PORT_RECT(&rect);
    } else if (game_tick_counter != 600) {
        origin = 308;
        if (guard_kind == 7 || guard_kind == 8) {
            half = life % 2;
            life /= 2;
        } else {
            half = 0;
        }
        x = origin;
        if (life > 0) {
            full_cells = life;
            count = full_cells;
            do {
                blit_meter_shape(meter_sheet, 131, x, 193, 0, 1);
                x -= 10;
            } while (--count);
        }
        if ((guard_kind == 7 || guard_kind == 8) && half) {
            blit_meter_shape(meter_sheet, 132, x, 193, 0, 1);
            x -= 10;
        }
        rect.top = 193;
        rect.bottom = meter_strip.bottom;
        if (life < 8) {
            count = 8 - life;
            do {
                rect.left = x;
                rect.right = x + 10;
                STROKE_PORT_RECT(&rect);
                x -= 10;
            } while (--count);
        }
    }
}

extern unsigned char sword_kind;

void draw_sword(TileDrawArgs *args)
{
    TileDrawArgs base;
    unsigned char kind;

    if (loose_item_drawer->item && (Kid.room != 9 || level_number != 8)) {
        base = *args;
        base.info.bytes[0] &= 0xfc;
        loose_item_drawer->item(&base);
    }
    if (args->pass == 1) {
        kind = args->info.bytes[0] & 3;
        add_middle_item(22, kind == sword_kind || (sword_kind == 255 && kind == 1) ? 11 : 46,
                        args->column, args->row, 10, 0);
    }
}

extern int far pascal IS_SIX_KIND(char kind);

int is_open_tile(char kind)
{
    if (!IsWall(kind) && !IS_SIX_KIND(kind))
        return 1;
    return 0;
}

int far pascal IsImpassable(char kind)
{
    if (kind == 11 || kind == 15 || kind == 26 || kind == 12 ||
        kind == 13 || kind == 23 || kind == 24)
        return 1;
    return 0;
}

int is_walkable_tile(char kind)
{
    if (is_open_tile(kind) && !IsImpassable(kind) && kind != 6 && kind != 34)
        return 1;
    return 0;
}

int far pascal IsMobFloor(char kind)
{
    if (kind == 11 || kind == 26 || kind == 15)
        return 1;
    return 0;
}

int far pascal IS_SIX_KIND(char kind)
{
    if (kind == 0 || kind == 9 || kind == 33 || kind == 35 || kind == 27 || kind == 37)
        return 1;
    return 0;
}

int far pascal IsWall(char kind)
{
    if (kind == 20 || kind == 2 || kind == 7 || kind == 25)
        return 1;
    return 0;
}


extern int set_palette(int resource, int first, unsigned char count, unsigned char palette);
extern unsigned char far *saved_palette, far *saved_text_palette;
extern unsigned char far *ALLOCATE_PINNED_PAYLOAD(long bytes);
extern void CLEAR_PAYLOAD_HEADER(unsigned char far *palette);
extern void READ_DAC_BLOCK(int first, int count, unsigned char far *palette);
extern void WRITE_DAC_BLOCK(int first, int count, const unsigned char far *palette, int update);
extern int EMPTYRECT(DrawRect far *clip);
extern unsigned int HGETRSRC(unsigned long type, int resource);
extern unsigned char get_total_pals(int resource);
extern void far *RESOLVE_HANDLE_TO_FAR_PAYLOAD(unsigned int resource);
extern void DISPOSE_HANDLE(unsigned int resource);
extern void far *far cdecl copy_palette_bytes(void far *dst, const void far *src, unsigned int bytes);

void set_default_palette(void)
{
    set_palette(10, 0, 16, 0);
}

void restore_game_play_palette(void)
{
    if (saved_palette) {
        WRITE_DAC_BLOCK(16, 240, saved_palette, 1);
        CLEAR_PAYLOAD_HEADER(saved_palette);
        saved_palette = 0;
    }
    if (saved_text_palette) {
        WRITE_DAC_BLOCK(0, 16, saved_text_palette, 0);
        CLEAR_PAYLOAD_HEADER(saved_text_palette);
        saved_text_palette = 0;
    }
}

void save_game_play_palette(void)
{
    DrawRect rect;

    if (!saved_palette) {
        saved_palette = ALLOCATE_PINNED_PAYLOAD(720L);
        READ_DAC_BLOCK(16, 240, saved_palette);
        WRITE_DAC_BLOCK(16, 240, 0, 1);
        if (!EMPTYRECT((DrawRect *)&Kid.previous_bounds) &&
            SECTRECT(&rect, (DrawRect *)&Kid.previous_bounds, &GamePlayRect))
            STROKE_PORT_RECT(&rect);
    }
}

typedef struct PaletteColor {
    unsigned char red, green, blue;
} PaletteColor;

void fill_palette_with_color(unsigned char color)
{
    PaletteColor far *palette;
    PaletteColor far *next;
    int remaining;

    palette = (PaletteColor far *)ALLOCATE_PINNED_PAYLOAD(768L);
    READ_DAC_BLOCK(color, 1, (unsigned char far *)palette);
    next = palette + 1;
    remaining = 255;
    do {
        *next = *palette;
        ++next;
    } while (--remaining);
    if (!saved_palette) {
        saved_palette = ALLOCATE_PINNED_PAYLOAD(720L);
        READ_DAC_BLOCK(16, 240, saved_palette);
    }
    if (!saved_text_palette) {
        saved_text_palette = ALLOCATE_PINNED_PAYLOAD(48L);
        READ_DAC_BLOCK(0, 16, saved_text_palette);
    }
    WRITE_DAC_BLOCK(0, 256, (unsigned char far *)palette, 1);
    CLEAR_PAYLOAD_HEADER((unsigned char far *)palette);
}

int set_palette(int resource, int first, unsigned char count, unsigned char palette)
{
    unsigned char total;
    register unsigned int handle;
    unsigned char far *volatile colors;
    int success;

    total = get_total_pals(resource);
    handle = HGETRSRC(0x50414c53UL, resource);
    if (handle && total > palette) {
        colors = (unsigned char far *)RESOLVE_HANDLE_TO_FAR_PAYLOAD(handle);
        if (first < 16) {
            if (saved_palette) {
                total = count < 16 - first ? count : 16 - first;
                if (count - total > 0)
                    copy_palette_bytes(saved_palette, colors + (count * palette + total) * 3,
                                       (count - total) * 3);
            } else {
                total = count;
            }
            WRITE_DAC_BLOCK(first, total, colors + count * palette * 3, 1);
        } else if (saved_palette) {
            copy_palette_bytes(saved_palette + first * 3 - 48, colors + count * palette * 3, count * 3);
        } else {
            WRITE_DAC_BLOCK(first, count, colors + count * palette * 3, 1);
        }
        DISPOSE_HANDLE(handle);
        success = 1;
    } else {
        success = 0;
    }
    return success;
}

unsigned char get_total_pals(int resource)
{
    unsigned int handle;
    int total;

    handle = HGETRSRC(0x50414c43UL, resource);
    if (handle) {
        total = *(int far *)RESOLVE_HANDLE_TO_FAR_PAYLOAD(handle);
        DISPOSE_HANDLE(handle);
    } else {
        total = 0;
    }
    return (unsigned char)total;
}

extern unsigned int *kid_meter_shapes;

void set_shape_variant(int variant)
{
    ((int far *)RESOLVE_HANDLE_TO_FAR_PAYLOAD(*kid_meter_shapes))[2] = variant;
}

extern char PieceAboveId(Room room, unsigned char piece);
extern char PieceAboveRightId(Room room, unsigned char piece);

int visible_above_piece(Room room, unsigned char piece)
{
    int visible = 0;
    char above, upper_left;

    above = PieceAboveId(room, piece);
    if (IS_SIX_KIND(above) || above == 5 || above == 6 ||
        (above == 3 && level_number == 13)) {
        visible = 1;
    } else {
        upper_left = PieceAboveRightId(room, piece);
        if (IS_SIX_KIND(upper_left) || upper_left == 5 || upper_left == 6 ||
            (above == 3 && level_number == 13))
            visible = 1;
    }
    return visible;
}
