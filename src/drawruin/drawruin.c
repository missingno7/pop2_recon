/* DRAWRUIN.C, DOS overlay 8 code frame 2a74. */
typedef unsigned char Byte;
typedef signed char CellCoord;

#pragma pack(1)
typedef struct RuinCell {
    Byte pass;
    CellCoord column, row;
    Byte foreground;
    unsigned int modifier;
    Byte background, background_modifier;
} RuinCell;
#pragma pack()

typedef struct DrawRect {
    int top, left, bottom, right;
} DrawRect;

typedef struct BackImage {
    Byte placement[10];
    DrawRect clip;
} BackImage;

typedef int (far pascal *AddImage)(Byte, int, CellCoord, CellCoord, Byte, Byte);

extern int draw_pass_mode;
extern DrawRect clip_rect;
extern BackImage far *far _fastcall get_back_table_image_row(int image, Byte column, Byte row);
extern int SECTRECT(DrawRect far *, const DrawRect far *, const DrawRect far *);
extern void UNIONRECT(DrawRect far *, const DrawRect far *, const DrawRect far *);
extern int AddBImgToBTable(Byte, int, CellCoord, CellCoord, Byte, Byte);
extern int AddBFImgToBTable(Byte, int, CellCoord, CellCoord, Byte, Byte);
extern int AddBFImgToFTable(Byte, int, CellCoord, CellCoord, Byte, Byte);
extern int add_middle_item(Byte, int, CellCoord, CellCoord, Byte, Byte);
extern Byte PieceRightId(CellCoord, CellCoord);

void draw_ruin_flaw(RuinCell *cell);

void draw_ruin_back_wall(RuinCell *cell)
{
    int image;
    Byte background_modifier, tile, layer;
    int fallback;
    BackImage far *shape;
    DrawRect overlap;

    background_modifier = cell->background_modifier & 15;
    image = (*(unsigned int *)&cell->background) & 31;
    if (image) {
        if (!draw_pass_mode) {
            shape = get_back_table_image_row(image, cell->column - 1, cell->row);
            if (shape && SECTRECT(&overlap, &shape->clip, &clip_rect)) {
                fallback = 0;
                UNIONRECT(&shape->clip, &shape->clip, &clip_rect);
                if (image >= 1 && image <= 5) {
                    shape = get_back_table_image_row(7, cell->column - 1, cell->row);
                    if (shape && SECTRECT(&overlap, &shape->clip, &clip_rect))
                        UNIONRECT(&shape->clip, &shape->clip, &clip_rect);
                }
            } else {
                shape = 0;
                fallback = 1;
            }
        } else {
            fallback = 0;
            shape = 0;
        }

        if (!shape && (fallback || !background_modifier ||
                       (cell->column == -1 && background_modifier <= 3) || cell->row == 3)) {
            if (image >= 1 && image <= 5) {
                tile = image == 5 && (background_modifier == 2 || background_modifier == 3 ||
                                      background_modifier == 6 || background_modifier == 7)
                     ? background_modifier - 2 : background_modifier;
                tile += 36;
                AddBImgToBTable(tile, 7, cell->column, cell->row, 0, 0);
                if (image == 5 && draw_pass_mode)
                    AddBImgToBTable(tile, 7, cell->column + 2, cell->row, 0, 0);
                layer = 10;
            } else {
                layer = 0;
            }
            if (image)
                AddBImgToBTable(background_modifier + 36, image, cell->column, cell->row, layer, 0);
        }
    }
}

void draw_ruin_block(RuinCell *cell)
{
    int image;

    switch (cell->pass) {
    case 0:
        if (draw_pass_mode)
            draw_ruin_back_wall(cell);
        break;
    case 5:
        image = (cell->modifier & 12) >> 2;
        if (image != 2 && PieceRightId(cell->column, cell->row) != 20) {
            image = image != 3 ? image + 12 : 53;
            AddBImgToBTable(20, image, cell->column, cell->row, 10, 0);
        }
        /* Fall through to the block and its flaw. */
    case 1:
        add_middle_item(20, (cell->modifier & 3) + 14, cell->column, cell->row, 0, 0);
        draw_ruin_flaw(cell);
        break;
    }
}

void draw_ruin_bones(RuinCell *cell)
{
    AddImage add_image = 0;

    switch (cell->pass) {
    case 0:
        draw_ruin_back_wall(cell);
        break;
    case 1:
        add_image = add_middle_item;
        break;
    case 2:
        add_image = AddBFImgToFTable;
        break;
    case 5:
        add_image = AddBFImgToBTable;
        break;
    }
    if (add_image) {
        add_image(21, -2, cell->column, cell->row, 10, 0);
        draw_ruin_flaw(cell);
    }
}

typedef struct ShapePosition {
    int x, y, flags;
} ShapePosition;

static union RuinTables {
    struct {
        int crevice_flags[4];
        int loose_odd[12], loose_even[12];
        ShapePosition flaw_positions[15], exit_positions[2];
    } rows;
    int words[79];
} ruin_tables = {
    {
        { 0, 2, 2, 0 },
        { 0, 1, 0, 2, 2, 0, 0, 0, 1, 1, 1, 0 },
        { 0, 2, 0, 1, 1, 0, 0, 2, 2, 2, 0, 2 },
        {
            { 76, 0, -32 },
            { 77, 22, -7 },
            { 67, 0, -3 },
            { 67, -32, -3 },
            { 68, -17, -6 },
            { 69, -8, 18 },
            { 70, 4, -32 },
            { 71, -16, -5 },
            { 72, 1, -4 },
            { 73, 4, 31 },
            { 74, 2, -25 },
            { 75, 4, -27 },
            { 78, -27, -7 },
            { 79, -16, -20 },
            { 80, 7, -31 }
        },
        {
            { 62, -15, -16 },
            { 65, 6, -60 }
        }
    }
};
#define exit_positions (ruin_tables.rows.exit_positions)

/* Loose selectors index the shared word storage across the full nibble. */
enum {
    LOOSE_ODD_WORD = sizeof ruin_tables.rows.crevice_flags / sizeof(int),
    LOOSE_EVEN_WORD = LOOSE_ODD_WORD + sizeof ruin_tables.rows.loose_odd / sizeof(int)
};

void draw_ruin_crevice(RuinCell *cell)
{
    int pattern, revealed;
    RuinCell block;

    pattern = cell->modifier & 3;
    revealed = cell->modifier & 0x80;
    switch (cell->pass) {
    case 0:
        if (draw_pass_mode)
            draw_ruin_back_wall(cell);
        break;
    case 1:
        if (revealed) {
            add_middle_item(7, pattern + 58, cell->column, cell->row, 10, 0);
        } else {
            block = *cell;
            block.modifier &= ~3;
            block.modifier |= ruin_tables.rows.crevice_flags[pattern];
            draw_ruin_block(&block);
        }
        draw_ruin_flaw(cell);
        break;
    case 2:
        if (revealed)
            AddBFImgToFTable(7, pattern + 54, cell->column, cell->row, 10, 0);
        break;
    case 5:
        if (revealed) {
            AddBFImgToBTable(7, pattern + 54, cell->column, cell->row, 10, 0);
        } else {
            block = *cell;
            block.modifier |= pattern ? 8 : 12;
            draw_ruin_block(&block);
        }
        break;
    }
}

void draw_ruin_exit_left(RuinCell *cell)
{
    AddImage add_image = 0;
    Byte layer = 10;
    int flaw = 1;

    switch (cell->pass) {
    case 0:
        if (draw_pass_mode) {
            draw_ruin_back_wall(cell);
            add_image = AddBImgToBTable;
        }
        break;
    case 1:
        add_image = add_middle_item;
        layer = 0;
        break;
    case 2:
        add_image = AddBFImgToFTable;
        flaw = 0;
        break;
    case 5:
        add_image = AddBFImgToBTable;
        break;
    default:
        flaw = 0;
        break;
    }
    if (add_image) {
        add_image(16, -2, cell->column, cell->row, layer, 0);
    }
    if (flaw)
        draw_ruin_flaw(cell);
}

#pragma pack(1)
typedef struct RuinPositions {
    char before_flaw[0x156];
    Byte flip_flaw;
    ShapePosition front_flaw, back_flaw;
    char before_gate[0x420 - 0x163];
    int gate_y;
    char before_exit[0x440 - 0x422];
    int exit_y;
    ShapePosition exit;
} RuinPositions;
#pragma pack()

typedef struct MovingExit {
    Byte reserved[2];
    signed char height;
} MovingExit;

extern RuinPositions *Pieces;
extern DrawRect GamePlayRect;
#pragma pack(1)
typedef struct {
    unsigned char number;                             /* 00 */
    unsigned char left, right, above, below;           /* 01-04 */
    unsigned char above_left, above_right;             /* 05-06 */
    unsigned char below_left, below_right;             /* 07-08 */
    unsigned char custom;                             /* 09 */
} ROOMREC;
#pragma pack()
extern ROOMREC Room;
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
extern signed char FirstInRow(Byte, CellCoord);
extern MovingExit *get_trob_room_cell(int);

void draw_ruin_exit_movable(RuinCell *cell);

void draw_ruin_exit_right(RuinCell *cell)
{
    AddImage add_image = 0;
    Byte layer = 10;
    int flaw = 0;

    switch (cell->pass) {
    case 0:
        if (draw_pass_mode) {
            draw_ruin_back_wall(cell);
        } else {
            Pieces->exit = exit_positions[0];
            AddBFImgToBTable(57, -2, cell->column, cell->row, 10, 0);
            add_image = 0;
        }
        break;
    case 1:
        add_image = add_middle_item;
        layer = 0;
        flaw = 1;
        break;
    case 2:
        add_image = AddBFImgToFTable;
        break;
    case 5:
        if (!draw_pass_mode)
            add_image = AddBFImgToBTable;
        flaw = 1;
        break;
    }
    if (add_image)
        add_image(17, -2, cell->column, cell->row, layer, 0);
    if (flaw)
        draw_ruin_flaw(cell);
    if (cell->pass == 0)
        draw_ruin_exit_movable(cell);
}

void draw_ruin_exit_movable(RuinCell *cell)
{
    AddImage add_image;
    MovingExit *moving;
    DrawRect saved_clip, door_clip;

    if (cell->pass == 0) {
        saved_clip = clip_rect;
        door_clip = GamePlayRect;
        door_clip.top = cell->row * 63 - 5;
        if (SECTRECT(&clip_rect, &clip_rect, &door_clip)) {
            moving = get_trob_room_cell(cell->column + FirstInRow(Room.number, cell->row));
            if (moving && moving->height >= 4)
                AddBImgToBTable(17, -2, cell->column, cell->row, 10, 0);
            Pieces->exit_y = -(int)(19U + ((cell->modifier & 0xff) < 42 ? cell->modifier & 0xff : 42));
            AddBImgToBTable(57, -2, cell->column, cell->row, 10, 0);
            add_image = Kid.frame >= 217 && Kid.frame <= 226 ? AddBFImgToFTable : AddBFImgToBTable;
            Pieces->exit = exit_positions[1];
            add_image(57, -2, cell->column, cell->row, 10, 0);
        }
        clip_rect = saved_clip;
    }
}

extern CellCoord hidden_flaw_column, hidden_flaw_row;
extern int last_front_table_slot;

#pragma pack(1)
typedef struct DrawEntry {
    Byte type, sheet;
    int shape, x, y;
    CellCoord column, row;
    DrawRect clip;
    Byte flip, attributes;
} DrawEntry;
#pragma pack()
extern DrawEntry far fore_queue[];


void draw_ruin_flaw(RuinCell *cell)
{
    int flaw, flags;

    if (cell->pass == 5) {
        flaw = cell->modifier >> 8 & 15;
        if (flaw == 11 || flaw == 12) {
            Pieces->front_flaw = ruin_tables.rows.flaw_positions[flaw - 11];
            Pieces->flip_flaw = flaw == 12 ? 1 : 0;
            AddBImgToBTable(18, -2, cell->column, cell->row, 10, 0);
        } else {
            flaw = cell->modifier & 0x3000;
            if (flaw) {
                flaw >>= 12;
                Pieces->back_flaw = ruin_tables.rows.flaw_positions[flaw + 11];
                AddBFImgToBTable(18, -2, cell->column, cell->row, 10, 0);
            }
        }
    } else if (cell->pass == 1) {
        flags = cell->modifier & 0xf00;
        if (flags && (unsigned)flags < 0xb00) {
            flaw = ((Byte *)&flags)[1];
            if (flaw != 2 || !draw_pass_mode) {
                int emitted;
                Pieces->back_flaw = ruin_tables.rows.flaw_positions[flaw + 1];
                if (flaw == 3 && !draw_pass_mode &&
                    hidden_flaw_column == cell->column && hidden_flaw_row == cell->row)
                    emitted = 0;
                else
                    emitted = AddBFImgToFTable(18, -2, cell->column, cell->row, 10, 0);
                if (emitted && flaw == 4)
                    fore_queue[last_front_table_slot - 1].clip = GamePlayRect;
            }
        }
    }
}



void draw_ruin_floor(RuinCell *cell)
{
    switch (cell->pass) {
    case 0:
        draw_ruin_back_wall(cell);
        break;
    case 1:
        add_middle_item(1, (cell->modifier & 3) + 26, cell->column, cell->row, 0, 0);
        goto finish_flaw;
    case 2:
        AddBFImgToFTable(1, (cell->modifier & 3) + 22, cell->column, cell->row, 10, 0);
        break;
    case 5:
        AddBFImgToBTable(1, (cell->modifier & 3) + 22, cell->column, cell->row, 10, 0);
    finish_flaw:
        draw_ruin_flaw(cell);
        break;
    }
}

void draw_ruin_gate_movable(RuinCell *cell);

void draw_ruin_gate(RuinCell *cell)
{
    AddImage add_image = 0;

    switch (cell->pass) {
    case 0:
        draw_ruin_back_wall(cell);
        break;
    case 1:
        add_image = add_middle_item;
        break;
    case 2:
        add_image = AddBFImgToFTable;
        break;
    case 5:
        add_image = AddBFImgToBTable;
        break;
    }
    if (add_image) {
        add_image(4, -2, cell->column, cell->row, 10, 0);
        draw_ruin_gate_movable(cell);
        draw_ruin_flaw(cell);
    }
}

#pragma pack(1)
typedef struct {
    unsigned int modifier, unknown_02;
} LEVELCELL;

typedef struct {
    unsigned char left, right, above, below;
} ROOMLINKS;

typedef struct {
    signed char opponent_count;
    unsigned char opponent_data[5][23];
} ROOMOBJECTS;

typedef struct {
    signed char room, cell;
} REBIRTHPOINT;

typedef struct {
    signed char count;
    unsigned char spawn_flags;
    unsigned char header[2];
    unsigned char points[3][10];
} ROOMGENERATORS;

typedef struct {
    unsigned char cell_kinds[32][30];                  /* 0000 */
    LEVELCELL cell_info[32][30];                       /* 03c0 */
    unsigned char unknown_12c0[0x500];                 /* 12c0 */
    ROOMLINKS room_links[32];                          /* 17c0 */
    unsigned char room_count;                         /* 1840 */
    unsigned char unknown_1841[4];
    unsigned char environment_kind;                  /* 1845 */
    unsigned char unknown_1846;
    signed char level_number;                         /* 1847 */
    int background_overlays[8];                       /* 1848 */
    unsigned char unknown_1858[4];
    int actor_phase;                                  /* 185c */
    unsigned char unknown_185e[2];
    signed char entry_room, entry_cell, entry_facing;    /* 1860-1862 */
    unsigned char unknown_1863[3];
    unsigned char preferred_opponent;                /* 1866 */
    ROOMOBJECTS room_objects[32];                      /* 1867 */
    REBIRTHPOINT rebirth_points[2];                    /* 26e7 */
    unsigned char unknown_26eb[0x0e];
    ROOMGENERATORS generators[32];                     /* 26f9 */
    unsigned char unknown_2b39[0x3c0];                 /* 2b39-2ef8 */
} LEVELREC;
#pragma pack()
extern LEVELREC Level;
extern void draw_throne_room_gate_in_fore(void);

void draw_ruin_gate_movable(RuinCell *cell)
{
    AddImage add_image;
    DrawRect gate_clip, saved_clip;

    saved_clip = clip_rect;
    gate_clip = GamePlayRect;
    gate_clip.top = cell->row * 63 - 10;
    if (SECTRECT(&clip_rect, &clip_rect, &gate_clip)) {
        unsigned int height = (cell->modifier & 0xfc) >> 2;
        Pieces->gate_y = -(4 + (height < 50 ? height : 50));
        switch (cell->pass) {
        case 2:
            add_image = AddBFImgToFTable;
            break;
        case 5:
            add_image = AddBFImgToBTable;
            break;
        default:
            add_image = 0;
            break;
        }
        if (add_image) {
            if (Room.number == 9 && ((Byte)Level.level_number) == 8) {
                if (cell->pass == 2)
                    draw_throne_room_gate_in_fore();
            } else {
                add_image(55, -2, cell->column, cell->row, 10, 0);
            }
        }
    }
    clip_rect = saved_clip;
}

void draw_ruin_loose(RuinCell *cell)
{
    int image;
    RuinCell floor;
    image = cell->modifier & 15;
    if (image != 13)
        image = cell->column % 2 ? ruin_tables.words[LOOSE_ODD_WORD + image]
                                 : ruin_tables.words[LOOSE_EVEN_WORD + image];
    if (!image) {
        floor = *cell;
        floor.modifier &= ~3;
    }
    switch (cell->pass) {
    case 0:
        if (image != 13) {
            if (image)
                draw_ruin_back_wall(cell);
            else
                draw_ruin_floor(&floor);
        } else {
            draw_ruin_back_wall(cell);
        }
        break;
    case 1: {
        RuinCell *tile = cell;
        if (image == 13) {
            draw_ruin_flaw(tile);
            break;
        }
        if (!image) {
            draw_ruin_floor(&floor);
            break;
        }
        add_middle_item(11, image + 43, tile->column, tile->row, 10, 0);
        draw_ruin_flaw(tile);
        break;
    }
    case 2:
        if (image != 13) {
            if (image) {
                RuinCell *tile = cell;
                AddBFImgToFTable(11, image + 41, tile->column, tile->row, 10, 0);
            } else {
                draw_ruin_floor(&floor);
            }
        }
        break;
    case 5: {
        RuinCell *tile = cell;
        if (image == 13) {
            draw_ruin_flaw(tile);
            break;
        }
        if (!image) {
            draw_ruin_floor(&floor);
            break;
        }
        AddBFImgToBTable(11, image + 41, tile->column, tile->row, 10, 0);
        draw_ruin_flaw(tile);
        break;
    }
    }
}

void draw_ruin_pillar_bottom(RuinCell *cell)
{
    AddImage add_image = 0;

    switch (cell->pass) {
    case 0:
        draw_ruin_back_wall(cell);
        add_image = AddBImgToBTable;
        break;
    case 1:
        add_image = add_middle_item;
        break;
    }
    if (add_image) {
        add_image(8, -2, cell->column, cell->row, 10, 0);
        draw_ruin_flaw(cell);
    }
}

void draw_ruin_pillar_top(RuinCell *cell)
{
    AddImage add_image;

    switch (cell->pass) {
    case 0:
        draw_ruin_back_wall(cell);
        add_image = AddBImgToBTable;
        break;
    case 1:
        add_image = add_middle_item;
        break;
    default:
        add_image = 0;
        break;
    }
    if (add_image) {
        add_image(9, -2, cell->column, cell->row, 10, 0);
        draw_ruin_flaw(cell);
    }
}

void draw_ruin_down_plate(RuinCell *cell)
{
    Byte layer;
    int image;
    AddImage add_image = 0;
    Byte tile = 5 + ((cell->modifier & 0x800) != 0);

    switch (cell->pass) {
    case 0:
        draw_ruin_back_wall(cell);
        break;
    case 1:
        image = 52;
        layer = 0;
        add_image = add_middle_item;
        break;
    case 5:
        add_image = AddBFImgToBTable;
        /* Shared back and front plate image. */
    case 2:
        if (!add_image)
            add_image = AddBFImgToFTable;
        image = 51;
        layer = 10;
        break;
    }
    if (add_image) {
        add_image(tile, image, cell->column, cell->row, layer, 0);
    }
}

void draw_ruin_up_plate(RuinCell *cell)
{
    Byte layer;
    int image;
    AddImage add_image = 0;
    Byte tile = 5 + ((cell->modifier & 0x800) != 0);

    switch (cell->pass) {
    case 0:
        draw_ruin_back_wall(cell);
        break;
    case 1:
        add_image = add_middle_item;
        image = 50;
        layer = 0;
        break;
    case 5:
        add_image = AddBFImgToBTable;
        /* Shared back and front plate image. */
    case 2:
        if (!add_image)
            add_image = AddBFImgToFTable;
        image = 49;
        layer = 10;
        break;
    }
    if (add_image) {
        add_image(tile, image, cell->column, cell->row, layer, 0);
    }
}

void draw_ruin_posts(RuinCell *cell)
{
    AddImage add_image = 0;
    int image = cell->modifier & 3;

    switch (cell->pass) {
    case 0:
        draw_ruin_back_wall(cell);
        break;
    case 1:
        add_image = add_middle_item;
        image += 39;
        break;
    case 2:
        add_image = AddBFImgToFTable;
        goto offset_back_image;
    case 5:
        add_image = AddBFImgToBTable;
    offset_back_image:
        image += 36;
        break;
    }
    if (add_image) {
        add_image(3, image, cell->column, cell->row, 10, 0);
        draw_ruin_flaw(cell);
    }
}

void draw_ruin_rubble_pieces(RuinCell *cell);

void draw_ruin_rubble(RuinCell *cell)
{
    RuinCell floor;
    unsigned int flags;

    if (cell->modifier & 0x40) {
        floor = *cell;
        floor.modifier = (flags = cell->modifier & ~0x40);
        if (flags & 4) {
            draw_ruin_up_plate(&floor);
            cell->modifier &= ~0x800;
        } else if (flags & 8) {
            draw_ruin_down_plate(&floor);
            cell->modifier &= ~0x800;
        } else {
            draw_ruin_floor(&floor);
        }
    } else if (cell->pass == 0) {
        draw_ruin_back_wall(cell);
    }
    draw_ruin_rubble_pieces(cell);
    draw_ruin_flaw(cell);
}

void draw_ruin_rubble_pieces(RuinCell *cell)
{
    AddImage add_image = 0;
    int image = (cell->modifier & 0x30) >> 4;

    switch (cell->pass) {
    case 1:
        image += 33;
        add_image = add_middle_item;
        break;
    case 2:
        image += 30;
        add_image = AddBFImgToFTable;
        break;
    case 5:
        image += 30;
        add_image = AddBFImgToBTable;
        break;
    }
    if (add_image) {
        add_image(14, image, cell->column, cell->row, 10, 0);
        draw_ruin_flaw(cell);
    }
}

extern Byte drawn_tile_kind;
extern int visible_above_piece(CellCoord, CellCoord);
extern int is_six_kind(Byte);
extern int Pause;

void draw_ruin_space(RuinCell *cell)
{
    int back_wall;

    if (cell->pass == 0) {
        back_wall = 0;
        if (cell->row == hidden_flaw_row)
            back_wall = 1;
        else if (cell->row - hidden_flaw_row == 1)
            back_wall = visible_above_piece(cell->column, cell->row);
        if (!back_wall && hidden_flaw_row == -1 && is_six_kind(drawn_tile_kind))
            back_wall = 1;
        if (back_wall)
            draw_ruin_back_wall(cell);
    }
    draw_ruin_flaw(cell);
}

void LoadRuinsDrawCode(void)
{
    Pause = 0;
}

#pragma pack(1)
typedef struct {
    signed char kind;                                 /* 00 */
    unsigned int modifier;                            /* 01 */
    signed char index;                                /* 03 */
    unsigned char room;                               /* 04 */
    signed char column, row;                          /* 05-06 */
    signed char first_col, last_col;                   /* 07-08 */
    unsigned char top_row, bottom_row;                 /* 09-0a */
} CELLREC;
#pragma pack()
extern CELLREC Cell;
extern CHARREC Char;
extern Byte ReadCell(CellCoord, CellCoord, CellCoord);
extern Byte get_cells_ahead(Byte);
extern int is_visible_room_cell(CellCoord, CellCoord);
extern void mark_cell_redraw(void);

void ruin_set_crevice_revealed(int reveal);

void ruin_change_crevices(int reveal, CellCoord room, CellCoord column, CellCoord row)
{
    CellCoord stop;
    Byte tile;

    stop = column + 3 >= 10 ? column + 3 : 10;
    column = column - 2 <= 0 ? column - 2 : 0;
    do {
        tile = ReadCell(room, column, row);
    } while (stop != ++column && tile != 7);
    if (tile == 7) {
        while (ReadCell(((CellCoord)Cell.room), Cell.column - 1, Cell.row) == 7)
            ;
        ReadCell(((CellCoord)Cell.room), Cell.column + 1, Cell.row);
        do
            ruin_set_crevice_revealed(reveal);
        while (ReadCell(((CellCoord)Cell.room), Cell.column + 1, Cell.row) == 7);
    }
}

void ruin_check_crevices(void)
{
    if (Char.sequence != 17 && get_cells_ahead(1) == 7 && ((*(Byte *)&Cell.modifier) & 3) != 3)
        ruin_change_crevices(1, Char.room, Char.column, Char.row);
}

void ruin_set_crevice_revealed(int reveal)
{
    int changed;
    unsigned int flags;

    flags = ((LEVELCELL *)Level.cell_info)[(Byte)Cell.room * 30 + Cell.index - 30].modifier;
    if (reveal) {
        changed = (flags & 0x80) == 0;
        if (changed)
            flags |= 0x80;
    } else {
        changed = flags & 0x80;
        if (changed)
            flags &= ~0x80;
    }
    if (changed) {
        if (is_visible_room_cell(((CellCoord)Cell.room), Cell.index))
            mark_cell_redraw();
        if (!(flags & 3) && Cell.column != 9) {
            ReadCell(((CellCoord)Cell.room), Cell.column + 1, Cell.row);
            mark_cell_redraw();
            ReadCell(((CellCoord)Cell.room), Cell.column - 1, Cell.row);
        }
        ((LEVELCELL *)Level.cell_info)[(Byte)Cell.room * 30 + Cell.index - 30].modifier = flags;
    }
}

extern void far pascal draw_flask(RuinCell near *);
extern void far pascal DrawScythe1(RuinCell near *);
extern void far pascal DrawScythe2(RuinCell near *);
extern void far pascal draw_sword(RuinCell near *);

typedef void (far pascal *LevelTileProc)(RuinCell near *);
LevelTileProc drawruin_level_spec[] = {
    draw_ruin_space, draw_ruin_floor, 0, draw_ruin_posts, draw_ruin_gate,
    draw_ruin_up_plate, draw_ruin_down_plate, draw_ruin_crevice, draw_ruin_pillar_bottom, draw_ruin_pillar_top,
    draw_flask, draw_ruin_loose, DrawScythe1, DrawScythe2, draw_ruin_rubble,
    draw_ruin_floor, draw_ruin_exit_left, draw_ruin_exit_right, 0, 0,
    draw_ruin_block, draw_ruin_bones, draw_sword
};
