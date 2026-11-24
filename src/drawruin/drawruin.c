/* DRAWRUIN.C, DOS overlay 8 code frame 2a74. */
#include "types.h"
#include "level.h"
#include "char.h"

#pragma pack(1)
typedef struct RuinCell {
    byte pass;
    sbyte column, row;
    byte foreground;
    unsigned int modifier;
    byte background, background_modifier;
} RuinCell;
#pragma pack()

typedef struct BackImage {
    byte placement[10];
    Rect clip;
} BackImage;

typedef int (far pascal *AddImage)(byte, int, sbyte, sbyte, byte, byte);

extern int NewRoom;
extern Rect CellClipRect;
extern BackImage far *far _fastcall get_back_table_image_row(int image, byte column, byte row);
extern int SECTRECT(Rect far *, const Rect far *, const Rect far *);
extern void UNIONRECT(Rect far *, const Rect far *, const Rect far *);
extern int AddBImgToBTable(byte, int, sbyte, sbyte, byte, byte);
extern int AddBFImgToBTable(byte, int, sbyte, sbyte, byte, byte);
extern int AddBFImgToFTable(byte, int, sbyte, sbyte, byte, byte);
extern int add_middle_item(byte, int, sbyte, sbyte, byte, byte);
extern byte PieceRightId(sbyte, sbyte);

void draw_ruin_flaw(RuinCell *cell);

void draw_ruin_back_wall(RuinCell *cell)
{
    int image;
    byte background_modifier, tile, layer;
    int fallback;
    BackImage far *shape;
    Rect overlap;

    background_modifier = cell->background_modifier & 15;
    image = (*(unsigned int *)&cell->background) & 31;
    if (image) {
        if (!NewRoom) {
            shape = get_back_table_image_row(image, cell->column - 1, cell->row);
            if (shape && SECTRECT(&overlap, &shape->clip, &CellClipRect)) {
                fallback = 0;
                UNIONRECT(&shape->clip, &shape->clip, &CellClipRect);
                if (image >= 1 && image <= 5) {
                    shape = get_back_table_image_row(7, cell->column - 1, cell->row);
                    if (shape && SECTRECT(&overlap, &shape->clip, &CellClipRect))
                        UNIONRECT(&shape->clip, &shape->clip, &CellClipRect);
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
                if (image == 5 && NewRoom)
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
        if (NewRoom)
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
        if (NewRoom)
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
    byte layer = 10;
    int flaw = 1;

    switch (cell->pass) {
    case 0:
        if (NewRoom) {
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
    byte flip_flaw;
    ShapePosition front_flaw, back_flaw;
    char before_gate[0x420 - 0x163];
    int gate_y;
    char before_exit[0x440 - 0x422];
    int exit_y;
    ShapePosition exit;
} RuinPositions;
#pragma pack()

typedef struct MovingExit {
    byte reserved[2];
    signed char height;
} MovingExit;

extern RuinPositions *Pieces;
extern Rect GamePlayRect;
extern ROOMREC Room;

extern CHARREC Kid;
extern signed char FirstInRow(byte, sbyte);
extern MovingExit *get_trob_room_cell(int);

void draw_ruin_exit_movable(RuinCell *cell);

void draw_ruin_exit_right(RuinCell *cell)
{
    AddImage add_image = 0;
    byte layer = 10;
    int flaw = 0;

    switch (cell->pass) {
    case 0:
        if (NewRoom) {
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
        if (!NewRoom)
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
    Rect saved_clip, door_clip;

    if (cell->pass == 0) {
        saved_clip = CellClipRect;
        door_clip = GamePlayRect;
        door_clip.top = cell->row * 63 - 5;
        if (SECTRECT(&CellClipRect, &CellClipRect, &door_clip)) {
            moving = get_trob_room_cell(cell->column + FirstInRow(Room.number, cell->row));
            if (moving && moving->height >= 4)
                AddBImgToBTable(17, -2, cell->column, cell->row, 10, 0);
            Pieces->exit_y = -(int)(19U + ((cell->modifier & 0xff) < 42 ? cell->modifier & 0xff : 42));
            AddBImgToBTable(57, -2, cell->column, cell->row, 10, 0);
            add_image = Kid.frame >= 217 && Kid.frame <= 226 ? AddBFImgToFTable : AddBFImgToBTable;
            Pieces->exit = exit_positions[1];
            add_image(57, -2, cell->column, cell->row, 10, 0);
        }
        CellClipRect = saved_clip;
    }
}

extern sbyte ColNo, RowNo;
extern int Ct[5];

#pragma pack(1)
typedef struct DrawEntry {
    byte type, sheet;
    int shape, x, y;
    sbyte column, row;
    Rect clip;
    byte flip, attributes;
} DrawEntry;
#pragma pack()
extern DrawEntry far ForeTable[];

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
            flaw = ((byte *)&flags)[1];
            if (flaw != 2 || !NewRoom) {
                int emitted;
                Pieces->back_flaw = ruin_tables.rows.flaw_positions[flaw + 1];
                if (flaw == 3 && !NewRoom &&
                    ColNo == cell->column && RowNo == cell->row)
                    emitted = 0;
                else
                    emitted = AddBFImgToFTable(18, -2, cell->column, cell->row, 10, 0);
                if (emitted && flaw == 4)
                    ForeTable[Ct[1] - 1].clip = GamePlayRect;
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
        draw_ruin_flaw(cell);
        break;
    case 2:
        AddBFImgToFTable(1, (cell->modifier & 3) + 22, cell->column, cell->row, 10, 0);
        break;
    case 5:
        AddBFImgToBTable(1, (cell->modifier & 3) + 22, cell->column, cell->row, 10, 0);
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

extern LEVELREC Level;
extern void draw_throne_room_gate_in_fore(void);

void draw_ruin_gate_movable(RuinCell *cell)
{
    AddImage add_image;
    Rect gate_clip, saved_clip;

    saved_clip = CellClipRect;
    gate_clip = GamePlayRect;
    gate_clip.top = cell->row * 63 - 10;
    if (SECTRECT(&CellClipRect, &CellClipRect, &gate_clip)) {
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
            if (Room.number == 9 && (Level.level_number) == 8) {
                if (cell->pass == 2)
                    draw_throne_room_gate_in_fore();
            } else {
                add_image(55, -2, cell->column, cell->row, 10, 0);
            }
        }
    }
    CellClipRect = saved_clip;
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
    byte layer;
    int image;
    AddImage add_image = 0;
    byte tile = 5 + ((cell->modifier & 0x800) != 0);

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
    byte layer;
    int image;
    AddImage add_image = 0;
    byte tile = 5 + ((cell->modifier & 0x800) != 0);

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
        image += 36;
        break;
    case 5:
        add_image = AddBFImgToBTable;
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

extern byte ObjId;
extern int visible_above_piece(sbyte, sbyte);
extern int is_six_kind(byte);
extern int Pause;

void draw_ruin_space(RuinCell *cell)
{
    int back_wall;

    if (cell->pass == 0) {
        back_wall = 0;
        if (cell->row == RowNo)
            back_wall = 1;
        else if (cell->row - RowNo == 1)
            back_wall = visible_above_piece(cell->column, cell->row);
        if (!back_wall && RowNo == -1 && is_six_kind(ObjId))
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

extern CELLREC Cell;
extern CHARREC Char;
extern byte ReadCell(sbyte, sbyte, sbyte);
extern byte get_cells_ahead(byte);
extern int is_visible_room_cell(sbyte, sbyte);
extern void mark_cell_redraw(void);

void ruin_set_crevice_revealed(int reveal);

void ruin_change_crevices(int reveal, sbyte room, sbyte column, sbyte row)
{
    sbyte stop;
    byte tile;

    stop = column + 3 >= 10 ? column + 3 : 10;
    column = column - 2 <= 0 ? column - 2 : 0;
    do {
        tile = ReadCell(room, column, row);
    } while (stop != ++column && tile != 7);
    if (tile == 7) {
        while (ReadCell((Cell.room), Cell.column - 1, Cell.row) == 7)
            ;
        ReadCell((Cell.room), Cell.column + 1, Cell.row);
        do
            ruin_set_crevice_revealed(reveal);
        while (ReadCell((Cell.room), Cell.column + 1, Cell.row) == 7);
    }
}

void ruin_check_crevices(void)
{
    if (Char.sequence != 17 && get_cells_ahead(1) == 7 && (Cell.modifier & 3) != 3)
        ruin_change_crevices(1, Char.room, Char.column, Char.row);
}

void ruin_set_crevice_revealed(int reveal)
{
    int changed;
    unsigned int flags;

    flags = ((LEVELCELL *)Level.cell_info)[Cell.room * 30 + Cell.index - 30].modifier;
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
        if (is_visible_room_cell((Cell.room), Cell.index))
            mark_cell_redraw();
        if (!(flags & 3) && Cell.column != 9) {
            ReadCell((Cell.room), Cell.column + 1, Cell.row);
            mark_cell_redraw();
            ReadCell((Cell.room), Cell.column - 1, Cell.row);
        }
        ((LEVELCELL *)Level.cell_info)[Cell.room * 30 + Cell.index - 30].modifier = flags;
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
