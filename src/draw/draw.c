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
    DrawRect old_clip;
    struct { int dirty_offset, cell, word_offset; } state;

    state.cell = column;
    state.word_offset = state.cell * sizeof(int);

    state.dirty_offset = state.cell * sizeof(DirtyCell);
    if (redraw_whole_cell[column]) {
        redraw_whole_cell[column] = 0;
        redraw_cell_sure();
    } else {
        if (DIRTY_CELL(redraw_back_cells, state.dirty_offset).pending) {
            DIRTY_CELL(redraw_back_cells, state.dirty_offset).pending = 0;
            old_clip = clip_rect;
            if (intersect_rect(&clip_rect, &clip_rect, &DIRTY_CELL(redraw_back_cells, state.dirty_offset).area)) {
                draw_back_image(0);
                draw_back_image(5);
                draw_back_image(11);
                draw_fore_image();
            }
            clip_rect = old_clip;
        }
    }
    if (DIRTY_CELL(redraw_floor_cells, state.dirty_offset).pending) {
        DIRTY_CELL(redraw_floor_cells, state.dirty_offset).pending = 0;
        if (WORD_FLAG(redraw_half_cell, state.word_offset) && compute_half_rect(&old_clip))
            union_rect(&DIRTY_CELL(redraw_floor_cells, state.dirty_offset).area,
                       &DIRTY_CELL(redraw_floor_cells, state.dirty_offset).area, &old_clip);
        old_clip = clip_rect;
        if (intersect_rect(&clip_rect, &clip_rect, &DIRTY_CELL(redraw_floor_cells, state.dirty_offset).area))
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
        if (intersect_rect(&clip_rect, &clip_rect, &DIRTY_CELL(redraw_fore_cells, state.dirty_offset).area))
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
    unsigned char reserved[10];
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
extern TileInfo current_tile_info;
extern signed char current_tile, left_tile;
extern TileInfo left_tile_info;
extern Tile above_tiles[], below_tiles[];
extern int force_full_clip;
extern DrawRect screen_clip;
extern unsigned char level_style, level_number;
extern int back_count, fore_count;
extern DrawEntry far back_queue[];
extern DrawEntry far fore_queue[];

extern int far pascal IsWall(char kind);
extern unsigned char back_image_start_pass(void);
extern void draw_custom_room_images(char pass);
extern void far _fastcall draw_custom_room_image(char image, char pass);
extern void check_foreground_gate(void);
extern int custom_bottom_has_image(void);
extern void prepare_level_room_image(void);
extern int far _fastcall keep_image_in_back(RoomImage *image);
extern int keep_gate_image(RoomImage *image);
extern void draw_horse(unsigned int sheet, unsigned int shape, char pass);
extern unsigned int far _fastcall horse_room_image(Room room, char pass, unsigned int *sheet);

void draw_back_image(unsigned char pass)
{
    unsigned char layer;
    unsigned char priority;
    struct { unsigned long address; TileDrawArgs args; } draw;

    layer = back_image_start_pass();
    if (force_full_clip)
        clip_rect = screen_clip;
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
            draw.args.kind = current_tile;
            draw.args.info = current_tile_info;
            break;
        case 2:
        draw_left:
            draw.args.column = draw_column - 1;
            draw.args.row = draw_row;
            draw.args.kind = left_tile;
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
        ((current_tile != 7 && IsWall(current_tile) &&
          left_tile != 7 && IsWall(left_tile)) ||
         (draw_column != 0 && draw_row != 2));
    if (custom_edge)
        pass = 3;
    else if ((force_full_clip && draw_column == 0 && draw_row != 2) ||
             (room_images && draw_row == 2 && !custom_bottom_has_image()))
        pass = 2;
    else
        pass = 0;
    return pass;
}

void draw_custom_room_images(char pass)
{
    signed char image;
    unsigned int sheet;
    unsigned int draw_images = 0;

    if ((pass == 0 || pass == 5) && force_full_clip &&
        draw_column == 0 && draw_row == 2) {
        draw_images = 1;
        if (drawn_room == 1 && level_number == 14)
            prepare_level_room_image();
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
                } while (++image < room_images->count);
            }
        }
        if ((force_full_clip && pass == 0) || pass == 1) {
            draw_images = horse_room_image(drawn_room, pass, &sheet);
            if (draw_images)
                draw_horse(sheet, draw_images, pass);
        }
    }
}

void far _fastcall draw_custom_room_image(char index, char pass)
{
    RoomImage *image;
    DrawRect clip;
    DrawEntry far *entry;

    image = &room_images->images[index];
    if (image->shape == -1 || image->pass != pass ||
        !intersect_rect(&clip, &image->clip, &clip_rect))
        return;
    if (pass != 0 && pass != 5 && pass != 11) {
        entry = &fore_queue[fore_count++];
        if (level_style == 5 &&
            ((drawn_room != 19 && drawn_room != 16 && drawn_room != 15 &&
              keep_image_in_back(image)) ||
             ((drawn_room == 19 || drawn_room == 16) && keep_gate_image(image)))) {
            entry = &back_queue[back_count++];
            --fore_count;
        }
    } else {
        if (!image->upper)
            return;
        entry = &back_queue[back_count++];
    }
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
            draw.args.kind = left_tile;
            draw.args.info = left_tile_info;
            break;
        case 1:
            draw.args.column = draw_column;
            draw.args.row = draw_row;
            draw.args.kind = current_tile;
            draw.args.info = current_tile_info;
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

extern unsigned int foreground_image_number;
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
    int matches, distance;
    TileDrawArgs args;
    DrawRect saved_clip;
    GateImage *image;

    if (foreground_image_number >= 264 && foreground_image_number <= 266) {
        if (level_style == 3) {
            args.pass = 0;
            args.row = draw_row;
            if (current_tile == 4) {
                args.column = draw_column;
                args.info = current_tile_info;
                draw_special_gate(&args);
            } else if (left_tile == 4) {
                args.column = draw_column - 1;
                args.info = left_tile_info;
                draw_special_gate(&args);
            }
        }
        return;
    }
    if (left_tile != 4)
        return;
    saved_clip = clip_rect;
    examine_first_gate();
    matches = left_tile == 4 && gate_row == draw_row &&
              ((distance = draw_column - gate_column) == 1 || distance == 2) &&
              gate_kind != 2 && gate_kind != 6;
    if (matches) {
        found = 1;
        image = get_gate_image(2, 0);
    } else {
        count = gate_rooms[drawn_room].count;
        for (index = 0; index < count && !matches; ++index) {
            examine_gate(index);
            matches = left_tile == 4 && gate_row == draw_row &&
                      ((distance = draw_column - gate_column) == 1 || distance == 2) &&
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
                intersect_rect(&clip_rect, &clip_rect, &image->clip);
            if (!empty_rect(&clip_rect))
                draw_gate(&args);
        }
    }
    clip_rect = saved_clip;
}

extern _segment room_queue_segment;
extern void prepare_room_queue(void);
extern void draw_back_fore_item(unsigned char pass, unsigned char entry);
extern void draw_middle_item(unsigned int entry);

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
                if (!prepared && ((room_queue_segment :> (DrawEntry _based(void) *)(index * sizeof(DrawEntry)))->sheet)) {
                    prepare_room_queue();
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
    int mode;
    unsigned int flags;
    unsigned int offset;

    offset = index * sizeof(DrawEntry);
    if (pass != 0)
        offset += (unsigned int)&fore_queue[0];
    entry = room_queue_segment :> (DrawEntry _based(void) *)offset;
    mode = 2;
    flags = 0;
    custom = 0;
    table = shape_tables[entry->type];
    if (table->count >= entry->shape) {
        if (entry->type == 1) {
            mode = 3;
            flags = 0x4000;
        } else if (entry->type == 0) {
            mode = 3;
            flags = 0x8000;
        }
        load_shape_ref(table, entry->shape, &ref);
        image = lock_shape(ref.handle);
        if (!image)
            return;
    } else {
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

void draw_middle_item(unsigned int index)
{
    MiddleEntry _based(middle_queue_segment) *entry;
    ShapeRef ref;
    ShapeBitmap far *image;
    signed char type;
    unsigned int mark, flags;
    int mode, flip, x, height, width;

    entry = (MiddleEntry _based(middle_queue_segment) *)
        ((unsigned int)&middle_queue[0] + index * sizeof(MiddleEntry));
    type = entry->type;
    if (entry->shape >= 0x62d7 && entry->shape <= 0x62e2) {
        load_animated_shape(entry->shape - 0x62d7, &ref);
    } else if (level_number == 5 && drawn_room == 10 && type == 4 &&
               entry->shape < room_images->count) {
        load_room_middle_shape(entry->shape, &ref);
    } else {
        if (type == 2 && entry->mode)
            set_shape_variant(entry->mode);
        load_shape_ref(shape_tables[type], entry->shape + 1, &ref);
        if (type == 2 && entry->mode)
            set_shape_variant(2);
    }
    flip = entry->attributes;
    mark = mark_shape(ref.handle);
    image = locate_shape(ref.handle);
    x = entry->x;
    if (entry->palette)
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
    if (entry->palette != ref.palette) {
        if (mode == 1)
            apply_shape_palette(image->data, image->width, image->stride);
        else if (mode == 2)
            apply_custom_shape_palette(image->data, image->width, image->stride);
        else
            apply_shape_scheme(image->data, image->width, image->height, image->stride);
        ((unsigned char *)shape_tables[type])[6 * entry->shape + 9] = entry->palette;
    }
    height = x + image->height;
    if (height > entry->clip.right)
        height = entry->clip.right;
    width = entry->clip.bottom - entry->y;
    if (width > image->width)
        width = image->width;
    update_middle_peel(x, height, entry->y, width, entry->update_peel, 0);
    set_draw_clip(&entry->clip);
    draw_bitmap(image, 0, x, entry->y, flip, mode, flags);
    restore_draw_clip();
    restore_shape(ref.handle, mark);
    release_shape_ref(&ref);
}

typedef struct DrawDestination {
    unsigned char header[24];
    DrawRect clip;
} DrawDestination;

extern DrawDestination *draw_destination;
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
    DrawRect area;
    int width, height;
    char message[150];

    if (!image && !mask) {
        format_draw_error(message, missing_shape_format, x, y, flip, mode);
        report_draw_error(1, message);
    }
    if (image) {
        switch (flip) {
        case 0:
        case 10:
            copy_shape_special(image, x, y, flip);
            break;
        case 2:
        case 3:
        case 8:
            if (mode == 3)
                copy_shape_mode(image, x, y, flip, flags);
            else
                copy_shape(image, x, y, flip);
            break;
        }
        width = image->width;
        height = image->height;
    } else {
        copy_shape_mask(mask, x, y, 0, flip);
        width = mask->width;
        height = mask->height;
    }
    if (!force_full_clip) {
        area.left = x;
        area.right = x + width;
        area.top = y;
        area.bottom = y + height;
        if (intersect_rect(&area, &area, &draw_destination->clip))
            add_dirty_rectangle(&area);
    }
}

extern void clear_fast_draw_state(void);
extern void prepare_fast_objects(void);
extern void prepare_fast_tiles(void);
extern void redraw_room_fast(void);
extern signed char row_to_first_cell(signed char row);

void full_redraw_fast(void)
{
    clear_fast_draw_state();
    prepare_fast_objects();
    prepare_fast_tiles();
    redraw_room_fast();
}

void redraw_room_fast(void)
{
    Room saved_room;

    begin_room_rows();
    draw_cell_objects(30);
    draw_row = 3;
    while (draw_row--) {
        prepare_row();
        prepare_row_images();
        draw_column = 0;
        do {
            select_draw_cell();
            redraw_cell_fast(row_to_first_cell(draw_row) + draw_column);
        } while (++draw_column < 10);
    }
    if (!room_is_custom) {
        saved_room = drawn_room;
        drawn_room = above_room;
        get_rooms();
        begin_room_rows();
        draw_row = 2;
        prepare_row();
        prepare_row_images();
        draw_column = 0;
        do {
            select_draw_cell();
            redraw_top_fast(draw_column);
        } while (++draw_column < 10);
        drawn_room = saved_room;
        get_rooms();
    } else {
        draw_custom_above_row();
    }
    draw_cell_objects(-1);
}

/* Fill the intervening routines before placing the original IsWall body. */
int far pascal IsWall(char kind)
{
    if (kind == 20 || kind == 2 || kind == 7 || kind == 25)
        return 1;
    return 0;
}
