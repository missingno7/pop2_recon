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
extern int add_foreground_image(Byte, int, CellCoord, CellCoord, Byte, Byte);
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
        add_foreground_image(20, (cell->modifier & 3) + 14, cell->column, cell->row, 0, 0);
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
        add_image = add_foreground_image;
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
            add_foreground_image(7, pattern + 58, cell->column, cell->row, 10, 0);
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
        add_image = add_foreground_image;
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

extern RuinPositions *shape_offsets;
extern DrawRect door_clip_rect;
extern Byte drawn_room;
extern unsigned int kid_frame;
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
            shape_offsets->exit = exit_positions[0];
            AddBFImgToBTable(57, -2, cell->column, cell->row, 10, 0);
            add_image = 0;
        }
        break;
    case 1:
        add_image = add_foreground_image;
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
        door_clip = door_clip_rect;
        door_clip.top = cell->row * 63 - 5;
        if (SECTRECT(&clip_rect, &clip_rect, &door_clip)) {
            moving = get_trob_room_cell(cell->column + FirstInRow(drawn_room, cell->row));
            if (moving && moving->height >= 4)
                AddBImgToBTable(17, -2, cell->column, cell->row, 10, 0);
            shape_offsets->exit_y = -(int)(19U + ((cell->modifier & 0xff) < 42 ? cell->modifier & 0xff : 42));
            AddBImgToBTable(57, -2, cell->column, cell->row, 10, 0);
            add_image = kid_frame >= 217 && kid_frame <= 226 ? AddBFImgToFTable : AddBFImgToBTable;
            shape_offsets->exit = exit_positions[1];
            add_image(57, -2, cell->column, cell->row, 10, 0);
        }
        clip_rect = saved_clip;
    }
}

extern CellCoord hidden_flaw_column, hidden_flaw_row;
extern int last_front_table_slot;
extern _segment front_table_segment;

#pragma pack(1)
typedef struct FrontImage {
    char drawing[10];
    DrawRect clip;
    char tail[2];
} FrontImage;
typedef struct FrontImageBank {
    char before_images[0xa14];
    FrontImage images[1];
} FrontImageBank;
#pragma pack()


void draw_ruin_flaw(RuinCell *cell)
{
    int flaw, flags;

    if (cell->pass == 5) {
        flaw = cell->modifier >> 8 & 15;
        if (flaw == 11 || flaw == 12) {
            shape_offsets->front_flaw = ruin_tables.rows.flaw_positions[flaw - 11];
            shape_offsets->flip_flaw = flaw == 12 ? 1 : 0;
            AddBImgToBTable(18, -2, cell->column, cell->row, 10, 0);
        } else {
            flaw = cell->modifier & 0x3000;
            if (flaw) {
                flaw >>= 12;
                shape_offsets->back_flaw = ruin_tables.rows.flaw_positions[flaw + 11];
                AddBFImgToBTable(18, -2, cell->column, cell->row, 10, 0);
            }
        }
    } else if (cell->pass == 1) {
        flags = cell->modifier & 0xf00;
        if (flags && (unsigned)flags < 0xb00) {
            flaw = ((Byte *)&flags)[1];
            if (flaw != 2 || !draw_pass_mode) {
                int emitted;
                shape_offsets->back_flaw = ruin_tables.rows.flaw_positions[flaw + 1];
                if (flaw == 3 && !draw_pass_mode &&
                    hidden_flaw_column == cell->column && hidden_flaw_row == cell->row)
                    emitted = 0;
                else
                    emitted = AddBFImgToFTable(18, -2, cell->column, cell->row, 10, 0);
                if (emitted && flaw == 4)
                    ((FrontImageBank far *)(front_table_segment :>
                        (void _based(void) *)(last_front_table_slot * sizeof(FrontImage))))->images[0].clip =
                        door_clip_rect;
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
        add_foreground_image(1, (cell->modifier & 3) + 26, cell->column, cell->row, 0, 0);
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
        add_image = add_foreground_image;
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

extern Byte level_number;
extern void draw_throne_room_gate_in_fore(void);

void draw_ruin_gate_movable(RuinCell *cell)
{
    AddImage add_image;
    DrawRect gate_clip, saved_clip;

    saved_clip = clip_rect;
    gate_clip = door_clip_rect;
    gate_clip.top = cell->row * 63 - 10;
    if (SECTRECT(&clip_rect, &clip_rect, &gate_clip)) {
        unsigned int height = (cell->modifier & 0xfc) >> 2;
        shape_offsets->gate_y = -(4 + (height < 50 ? height : 50));
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
            if (drawn_room == 9 && level_number == 8) {
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
    RuinCell *draw_cell;
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
        if (image == 13 || image)
            draw_ruin_back_wall(cell);
        else {
    draw_floor:
            draw_ruin_floor(&floor);
        }
        break;
    case 1:
        draw_cell = cell;
        if (image != 13) {
            if (!image)
                goto draw_floor;
            add_foreground_image(11, image + 43, draw_cell->column, draw_cell->row, 10, 0);
        }
        goto finish_flaw;
    case 2:
        draw_cell = cell;
        if (image != 13) {
            if (!image)
                goto draw_floor;
            AddBFImgToFTable(11, image + 41, draw_cell->column, draw_cell->row, 10, 0);
        }
        break;
    case 5:
        draw_cell = cell;
        if (!image)
            goto draw_floor;
        if (image != 13) {
            AddBFImgToBTable(11, image + 41, draw_cell->column, draw_cell->row, 10, 0);
        }
    finish_flaw:
        draw_ruin_flaw(cell);
        break;
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
        add_image = add_foreground_image;
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
        add_image = add_foreground_image;
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
        add_image = add_foreground_image;
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
        add_image = add_foreground_image;
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

#pragma optimize("g", off)
void draw_ruin_posts(RuinCell *cell)
{
    AddImage add_image = 0;
    int image = cell->modifier & 3;

    switch (cell->pass) {
    case 0:
        draw_ruin_back_wall(cell);
        break;
    case 1:
        add_image = add_foreground_image;
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
#pragma optimize("g", on)

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

#pragma optimize("g", off)
void draw_ruin_rubble_pieces(RuinCell *cell)
{
    AddImage add_image = 0;
    int image = (cell->modifier & 0x30) >> 4;

    switch (cell->pass) {
    case 1:
        image += 33;
        add_image = add_foreground_image;
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
#pragma optimize("g", on)

extern Byte below_tile_kind;
extern int visible_above_piece(CellCoord, CellCoord);
extern int is_six_kind(Byte);
extern int ruins_draw_state;

void draw_ruin_space(RuinCell *cell)
{
    int back_wall;

    if (cell->pass == 0) {
        back_wall = 0;
        if (cell->row == hidden_flaw_row)
            back_wall = 1;
        else if (cell->row - hidden_flaw_row == 1)
            back_wall = visible_above_piece(cell->column, cell->row);
        if (!back_wall && hidden_flaw_row == -1 && is_six_kind(below_tile_kind))
            back_wall = 1;
        if (back_wall)
            draw_ruin_back_wall(cell);
    }
    draw_ruin_flaw(cell);
}

void LoadRuinsDrawCode(void)
{
    ruins_draw_state = 0;
}

extern CellCoord tile_room, tile_column, tile_row, tile_index;
extern CellCoord actor_room, actor_column, actor_row;
extern int actor_frame;
extern Byte tile_modifier;
extern Byte ReadCell(CellCoord, CellCoord, CellCoord);
extern Byte get_cells_ahead(Byte);
extern int is_visible_room_cell(CellCoord, CellCoord);
extern void mark_cell_redraw(void);
typedef struct CellState {
    unsigned int flags, other;
} CellState;
extern CellState cell_states[];

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
        while (ReadCell(tile_room, tile_column - 1, tile_row) == 7)
            ;
        ReadCell(tile_room, tile_column + 1, tile_row);
        do
            ruin_set_crevice_revealed(reveal);
        while (ReadCell(tile_room, tile_column + 1, tile_row) == 7);
    }
}

void ruin_check_crevices(void)
{
    if (actor_frame != 17 && get_cells_ahead(1) == 7 && (tile_modifier & 3) != 3)
        ruin_change_crevices(1, actor_room, actor_column, actor_row);
}

void ruin_set_crevice_revealed(int reveal)
{
    int changed;
    unsigned int flags;

    flags = cell_states[(Byte)tile_room * 30 + tile_index].flags;
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
        if (is_visible_room_cell(tile_room, tile_index))
            mark_cell_redraw();
        if (!(flags & 3) && tile_column != 9) {
            ReadCell(tile_room, tile_column + 1, tile_row);
            mark_cell_redraw();
            ReadCell(tile_room, tile_column - 1, tile_row);
        }
        cell_states[(Byte)tile_room * 30 + tile_index].flags = flags;
    }
}
