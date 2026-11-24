/* DRAWTEMP.C, DOS overlay 9 frame 2b4b. */
typedef unsigned char Byte;
typedef signed char CellCoord;

typedef struct TempleCell {
    Byte pass;
    CellCoord column, row;
    Byte foreground, modifier, flags, background, background_modifier;
} TempleCell;

/* Each tile longword has two packed views shared with the draw dispatcher. */
#define CELL_INFO_WORD(cell, i) (((unsigned int *)&(cell)->modifier)[(i)])

typedef struct DrawRect {
    int top, left, bottom, right;
} DrawRect;

typedef struct BackImage {
    Byte placement[10];
    DrawRect clip;
} BackImage;

typedef int (far pascal *AddImage)(Byte, int, CellCoord, CellCoord, Byte, Byte);

typedef struct ShapePosition {
    int x, y, flags;
} ShapePosition;

#pragma pack(1)
typedef struct ShapeOffsets {
    char pad0[0x14];
    ShapePosition floor, floor_detail;
    char pad1[0x48 - 0x20];
    int posts_y;
    char pad2[0x18b - 0x4a];
    int block_y, block_overlay_y;
    char pad3[0x420 - 0x18f];
    int gate_y;
    char pad4[0x440 - 0x422];
    int exit_y;
} ShapeOffsets;
#pragma pack()

typedef struct MovingExit {
    Byte reserved[2];
    signed char height;
} MovingExit;

extern int NewRoom;
extern DrawRect CellClipRect, GamePlayRect;
extern int Row2PixelY[];
extern ShapeOffsets *Pieces;
extern Byte Room;
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
extern BackImage far *far _fastcall get_back_table_image_row(int image, Byte column, Byte row);
extern int SECTRECT(DrawRect far *out, const DrawRect far *a, const DrawRect far *b);
extern void UNIONRECT(DrawRect far *out, const DrawRect far *a, const DrawRect far *b);
extern int visible_above_piece(CellCoord column, CellCoord row);
extern int EMPTYRECT(const DrawRect far *rect);
extern int AddBImgToBTable(Byte, int, CellCoord, CellCoord, Byte, Byte);
extern int AddBFImgToBTable(Byte, int, CellCoord, CellCoord, Byte, Byte);
extern int add_middle_item(Byte, int, CellCoord, CellCoord, Byte, Byte);
extern int AddBFImgToFTable(Byte, int, CellCoord, CellCoord, Byte, Byte);
extern Byte PieceLeftId(CellCoord, CellCoord);
extern int IsWall(Byte);
extern Byte PieceBelowId(CellCoord, CellCoord);
extern Byte PieceRightId(CellCoord, CellCoord);
extern signed char FirstInRow(Byte, CellCoord);
extern MovingExit *get_trob_room_cell(int tile);

static int loose_odd[12] = { 0, 1, 0, 2, 2, 0, 0, 0, 1, 1, 1, 0 };
static int loose_even[12] = { 0, 2, 2, 0, 1, 1, 0, 2, 2, 2, 0, 1 };
extern signed char RowNo;
extern Byte ObjId;
extern int is_six_kind(Byte);
extern DrawRect TempTorchRect;
extern DrawRect *ComputeOffsetCellRect(DrawRect *, const DrawRect *, int, int);
extern int EncloseRect(const DrawRect far *, const DrawRect far *);
extern int Pause;

void draw_temple_exit_movable(TempleCell *cell);
void draw_temple_gate_movable(TempleCell *cell);
void draw_temple_rubble_pieces(TempleCell *cell);
void draw_temple_torch(TempleCell *cell);
void draw_temple_floor_detail(TempleCell *cell);

void draw_temple_back_wall(TempleCell *cell)
{
    int image;
    Byte modifier;
    int paired, fallback;
    BackImage far *shape;
    DrawRect overlap;
    int saved_top;

    modifier = (CELL_INFO_WORD(cell, 1) & 0x0f00) >> 8;
    image = CELL_INFO_WORD(cell, 1) & 31;
    if (image) {
        if (image == 20) {
            image = 19;
            paired = 1;
        } else {
            paired = 0;
        }

        if (!NewRoom) {
            shape = get_back_table_image_row(image, cell->column - 1, cell->row);
            if (shape && SECTRECT(&overlap, &shape->clip, &CellClipRect)) {
                fallback = 0;
                UNIONRECT(&shape->clip, &shape->clip, &CellClipRect);
                if (paired) {
                    shape = get_back_table_image_row(image + 1, cell->column - 1, cell->row);
                    if (shape) {
                        if (SECTRECT(&overlap, &shape->clip, &CellClipRect))
                            UNIONRECT(&shape->clip, &shape->clip, &CellClipRect);
                    }
                }
            } else {
                shape = 0;
                fallback = 1;
            }
        } else {
            fallback = 0;
            shape = 0;
        }

        if (!shape && (fallback || !modifier ||
                       (cell->column == -1 && modifier <= 3) || cell->row == 3)) {
            fallback = 1;
            saved_top = CellClipRect.top;
            if (cell->row >= 0 && !visible_above_piece(cell->column, cell->row)) {
                if (Row2PixelY[cell->row] > CellClipRect.left) {
                    CellClipRect.top = Row2PixelY[cell->row] < saved_top ? saved_top : Row2PixelY[cell->row];
                    if (EMPTYRECT(&CellClipRect))
                        fallback = 0;
                }
            }
            if (fallback) {
                modifier += 36;
                AddBImgToBTable(modifier, image, cell->column, cell->row, 0, 0);
                if (paired)
                    AddBFImgToBTable(modifier, -2, cell->column, cell->row, 0, 0);
            }
            CellClipRect.top = saved_top;
        }
    }
}

void draw_temple_block(TempleCell *cell)
{
    int image;
    Byte left;
    AddImage add_image;

    switch (cell->pass) {
    case 0:
        if (NewRoom)
            draw_temple_back_wall(cell);
        break;
    case 1:
        image = (cell->modifier & 7) + 24;
        if (image > 25) {
            if (!NewRoom) {
                Pieces->block_y = -32;
                image -= 2;
            } else {
                image = -1;
            }
        } else {
            Pieces->block_y = 0;
        }
        if (image != -1)
            add_middle_item(20, image, cell->column, cell->row, 10, 0);
        image = (CELL_INFO_WORD(cell, 0) & 7) + 24;
        left = PieceLeftId(cell->column, cell->row);
        if (image <= 25 && IsWall(left))
            add_middle_item(20, 61, cell->column, cell->row, 0, 0);
        if (IsWall(PieceBelowId(cell->column, cell->row))) {
            image = IsWall(left) ? 63 : 62;
            add_middle_item(20, image, cell->column, cell->row, 0, 0);
        }
        break;
    case 2:
    case 5:
        image = (cell->modifier & 7) + 24;
        if ((image == 24 || image > 25) && !IsWall(PieceRightId(cell->column, cell->row))) {
            if (cell->pass == 5)
                add_image = AddBFImgToBTable;
            else
                add_image = AddBFImgToFTable;
            add_image(20, -2, cell->column, cell->row, 10, 0);
        }
        break;
    }
}

void draw_temple_exit_left(TempleCell *cell)
{
    Byte layer;
    AddImage add_image = 0;
    switch (cell->pass) {
    case 0:
        if (NewRoom)
            draw_temple_back_wall(cell);
        break;
    case 1:
        add_image = add_middle_item;
        layer = 0;
        break;
    case 2:
        add_image = AddBFImgToFTable;
        layer = 10;
        break;
    case 5:
        add_image = AddBFImgToBTable;
        layer = 10;
        break;
    }
    if (add_image)
        add_image(16, -2, cell->column, cell->row, layer, 0);
}

void draw_temple_exit_right(TempleCell *cell)
{
    Byte layer;
    AddImage add_image = 0;
    switch (cell->pass) {
    case 0:
        if (NewRoom) {
            draw_temple_back_wall(cell);
            draw_temple_exit_movable(cell);
        }
        break;
    case 1:
        add_image = add_middle_item;
        layer = 0;
        break;
    case 2:
        add_image = AddBFImgToFTable;
        layer = 10;
        break;
    case 5:
        add_image = AddBFImgToBTable;
        layer = 10;
        break;
    }
    if (add_image) {
        add_image(17, -2, cell->column, cell->row, layer, 0);
        if (cell->pass == 5)
            draw_temple_exit_movable(cell);
    }
}

void draw_temple_exit_movable(TempleCell *cell)
{
    AddImage add_image;
    DrawRect door_clip, saved_clip;
    MovingExit *moving;
    if (cell->pass == 5) {
        saved_clip = CellClipRect;
        door_clip = GamePlayRect;
        door_clip.top = cell->row * 63 - 5;
        if (SECTRECT(&CellClipRect, &CellClipRect, &door_clip)) {
            moving = get_trob_room_cell(cell->column + FirstInRow(Room, cell->row));
            if (moving && moving->height >= 4)
                AddBImgToBTable(17, -2, cell->column, cell->row, 0, 0);
            Pieces->exit_y = -(19 + ((CELL_INFO_WORD(cell, 0) & 255) < 42 ? (CELL_INFO_WORD(cell, 0) & 255) : 42));
            AddBImgToBTable(57, -2, cell->column, cell->row, 10, 0);
            add_image = Kid.frame >= 217 && Kid.frame <= 226 ? AddBFImgToFTable : AddBFImgToBTable;
            add_image(57, -2, cell->column, cell->row, 10, 0);
        }
        CellClipRect = saved_clip;
    }
}

void draw_temple_floor(TempleCell *cell)
{
    Byte layer;
    int detail = 0;
    AddImage add_image = 0;
    switch (cell->pass) {
    case 0:
        draw_temple_back_wall(cell);
        break;
    case 1:
        add_image = add_middle_item;
        layer = 0;
        break;
    case 2:
        add_image = AddBFImgToFTable;
        layer = 10;
        detail = cell->modifier & 3;
        break;
    case 5:
        add_image = AddBFImgToBTable;
        layer = 10;
        detail = cell->modifier & 3;
        break;
    }
    if (add_image) {
        add_image(1, -2, cell->column, cell->row, layer, 0);
        if (detail)
            draw_temple_floor_detail(cell);
    }
}

void draw_temple_floor_detail(TempleCell *cell)
{
    struct ShapePosition saved;
    AddImage add_image;

    if (cell->pass == 5)
        add_image = AddBFImgToBTable;
    else if (cell->pass == 2)
        add_image = AddBFImgToFTable;
    else
        add_image = 0;
    if (add_image) {
        saved = Pieces->floor_detail;
        Pieces->floor_detail = Pieces->floor;
        add_image(1, -2, cell->column, cell->row, 0, 0);
        Pieces->floor_detail = saved;
    }
}

void draw_temple_gate(TempleCell *cell)
{
    AddImage draw = 0;
    switch (cell->pass) {
    case 0:
        draw_temple_back_wall(cell);
        break;
    case 1:
        draw = add_middle_item;
        break;
    case 2:
        draw = AddBFImgToFTable;
        break;
    case 5:
        draw = AddBFImgToBTable;
        break;
    }
    if (draw) {
        draw(4, -2, cell->column, cell->row, 10, 0);
        draw_temple_gate_movable(cell);
    }
}

void draw_temple_gate_movable(TempleCell *cell)
{
    AddImage draw;
    DrawRect gate_clip, saved_clip;
    unsigned int height;

    saved_clip = CellClipRect;
    gate_clip = GamePlayRect;
    gate_clip.top = cell->row * 63 - 10;
    if (SECTRECT(&CellClipRect, &CellClipRect, &gate_clip)) {
        height = (CELL_INFO_WORD(cell, 0) & 252) >> 2;
        Pieces->gate_y = -(4 + (height < 50 ? height : 50));
        switch (cell->pass) {
        case 2:
            draw = AddBFImgToFTable;
            break;
        case 5:
            draw = AddBFImgToBTable;
            break;
        default:
            draw = 0;
            break;
        }
        if (draw)
            draw(55, -2, cell->column, cell->row, 10, 0);
    }
    CellClipRect = saved_clip;
}

void draw_temple_loose(TempleCell *cell)
{
    int image;
    TempleCell floor_cell;
    image = cell->modifier & 15;
    if (image != 13)
        image = cell->column % (signed char)2 ? loose_odd[image] : loose_even[image];
    if (!image) {
        floor_cell = *cell;
        floor_cell.modifier &= ~3;
    }
    switch (cell->pass) {
    case 0:
        draw_temple_back_wall(cell);
        break;
    case 1:
        if (image == 13)
            break;
        if (image)
            add_middle_item(11, image + 35, cell->column, cell->row, 10, 0);
        else
            draw_temple_floor(&floor_cell);
        break;
    case 2:
        if (image == 13)
            break;
        if (image)
            AddBFImgToFTable(11, image + 33, cell->column, cell->row, 10, 0);
        else
            draw_temple_floor(&floor_cell);
        break;
    case 5:
        if (image == 13)
            break;
        if (image)
            AddBFImgToBTable(11, image + 33, cell->column, cell->row, 10, 0);
        else
            draw_temple_floor(&floor_cell);
        break;
    }
}

void draw_temple_pillar_bottom(TempleCell *cell)
{
    AddImage draw = 0;
    switch (cell->pass) {
    case 0:
        draw_temple_back_wall(cell);
        break;
    case 1:
        draw = add_middle_item;
        break;
    case 5:
        draw = AddBImgToBTable;
        break;
    }
    if (draw)
        draw(8, -2, cell->column, cell->row, 10, 0);
}

void draw_temple_pillar_top(TempleCell *cell)
{
    AddImage draw = 0;
    switch (cell->pass) {
    case 0:
        draw_temple_back_wall(cell);
        break;
    case 1:
        draw = add_middle_item;
        break;
    case 5:
        draw = AddBImgToBTable;
        break;
    }
    if (draw)
        draw(9, -2, cell->column, cell->row, 10, 0);
}

void draw_temple_down_plate(TempleCell *cell)
{
    Byte tile;
    int image;
    int pressed = CELL_INFO_WORD(cell, 0) & 0x0800;
    AddImage draw = 0;

    switch (cell->pass) {
    case 0:
        draw_temple_back_wall(cell);
        break;
    case 1:
        tile = pressed ? 6 : 5;
        image = 33;
        draw = add_middle_item;
        break;
    case 2:
    case 5:
        if (pressed) {
            tile = 6;
            image = 31;
        } else {
            tile = 1;
            image = -2;
        }
        draw = cell->pass == 5 ? AddBFImgToBTable : AddBFImgToFTable;
        break;
    }
    if (draw)
        draw(tile, image, cell->column, cell->row, 10, 0);
}

void draw_temple_up_plate(TempleCell *cell)
{
    int image;
    Byte tile = 5 + ((CELL_INFO_WORD(cell, 0) & 0x0800) != 0);
    AddImage draw = 0;

    switch (cell->pass) {
    case 0:
        draw_temple_back_wall(cell);
        break;
    case 1:
        draw = add_middle_item;
        image = 32;
        break;
    case 2:
    case 5:
        draw = cell->pass == 5 ? AddBFImgToBTable : AddBFImgToFTable;
        image = 30;
        break;
    }
    if (draw)
        draw(tile, image, cell->column, cell->row, 10, 0);
}

void draw_temple_posts(TempleCell *cell)
{
    AddImage draw = 0;
    int image = CELL_INFO_WORD(cell, 0) & 3;
    int saved_y;

    switch (cell->pass) {
    case 0:
        draw_temple_back_wall(cell);
        break;
    case 1:
        if (!image) { draw = add_middle_item; image = 44; }
        else if (!NewRoom || image == 1) {
            draw = add_middle_item;
            Pieces->posts_y = 0;
            if (image == 2)
                Pieces->posts_y -= 32;
            image = 26;
        }
        break;
    case 2:
        if (!image) {
            draw = AddBFImgToFTable;
            image = 43;
        }
        break;
    case 5:
        if (!image) {
            draw = AddBFImgToBTable;
            image = 43;
        }
        break;
    }
    if (draw) {
        draw(3, image, cell->column, cell->row, 10, 0);
        if (image == 26) {
            if (Pieces->posts_y == 0 && PieceLeftId(cell->column, cell->row) != 3) {
                saved_y = Pieces->block_overlay_y;
                Pieces->block_overlay_y -= 54;
                add_middle_item(20, 38, cell->column, cell->row, 10, 0);
                Pieces->block_overlay_y = saved_y;
            }
            Pieces->posts_y = 0;
        }
    }
}

void draw_temple_rubble(TempleCell *cell)
{
    if (cell->pass == 0)
        draw_temple_back_wall(cell);
    else if ((cell->pass == 5 || cell->pass == 2) && (cell->modifier & 3))
        draw_temple_floor_detail(cell);
    draw_temple_rubble_pieces(cell);
}

void draw_temple_rubble_pieces(TempleCell *cell)
{
    AddImage draw;
    switch (cell->pass) {
    case 1:
        draw = add_middle_item;
        break;
    case 2:
        draw = AddBFImgToFTable;
        break;
    case 5:
        draw = AddBFImgToBTable;
        break;
    default:
        draw = 0;
        break;
    }
    if (draw)
        draw(14, -2, cell->column, cell->row, 10, 0);
}

void draw_temple_space(TempleCell *cell)
{
    int draw;
    if (cell->pass == 0) {
        draw = 0;
        if (cell->row == RowNo)
            draw = 1;
        else if (cell->row - RowNo == 1)
            draw = visible_above_piece(cell->column, cell->row);
        if (!draw && RowNo == -1 && is_six_kind(ObjId))
            draw = 1;
        if (draw)
            draw_temple_back_wall(cell);
    }
}

void draw_temple_spikes(TempleCell *cell)
{
    TempleCell block;
    switch (cell->pass) {
    case 1:
        block = *cell;
        block.modifier &= ~7;
        draw_temple_block(&block);
        break;
    case 5:
        if (cell->column != 9) {
            block = *cell;
            block.modifier &= ~7;
            draw_temple_block(&block);
            AddBImgToBTable(2, -2, cell->column, cell->row, 0, 0);
        }
        break;
    }
}

void draw_temple_rubble_torch(TempleCell *cell)
{
    draw_temple_torch(cell);
    draw_temple_rubble_pieces(cell);
}

void draw_temple_torch(TempleCell *cell)
{
    int frame = cell->modifier;
    TempleCell floor_cell = *cell;
    DrawRect torch_clip;
    CELL_INFO_WORD(&floor_cell, 1) = CELL_INFO_WORD(&floor_cell, 0) = 0;
    switch (cell->pass) {
    case 0:
        ComputeOffsetCellRect(&torch_clip, &TempTorchRect, cell->column, cell->row);
        if (!EncloseRect(&torch_clip, &CellClipRect)) {
            draw_temple_floor(&floor_cell);
            AddBImgToBTable(19, -2, cell->column, cell->row, 0, 0);
        }
        break;
    case 1:
    case 2:
        draw_temple_floor(&floor_cell);
        break;
    case 5:
        draw_temple_floor(&floor_cell);
        AddBFImgToBTable(19, frame + 51, cell->column, cell->row, 0, 0);
        break;
    }
}

void draw_temple_trapdoor(TempleCell *cell)
{
    int state = cell->modifier & 31;
    switch (cell->pass) {
    case 0:
        if (state)
            draw_temple_back_wall(cell);
        else
            draw_temple_floor(cell);
        break;
    case 1:
    case 2:
    case 5:
        if (!state)
            draw_temple_floor(cell);
        break;
    }
}

void LoadTempleDrawCode(void)
{
    Pause = 0;
}

extern void far pascal draw_flask(TempleCell near *);
extern void far pascal DrawScythe1(TempleCell near *);
extern void far pascal DrawScythe2(TempleCell near *);
extern void far pascal draw_temp_sacred_flame(TempleCell near *);

typedef void (far pascal *LevelTileProc)(TempleCell near *);
LevelTileProc drawtemp_level_spec[] = {
    draw_temple_space, draw_temple_floor, draw_temple_spikes, draw_temple_posts, draw_temple_gate,
    draw_temple_up_plate, draw_temple_down_plate, 0, draw_temple_pillar_bottom, draw_temple_pillar_top,
    draw_flask, draw_temple_loose, DrawScythe1, DrawScythe2, draw_temple_rubble,
    0, draw_temple_exit_left, draw_temple_exit_right, 0, draw_temple_torch,
    draw_temple_block, 0, 0, 0, 0,
    draw_temple_block, draw_temple_trapdoor, 0, 0, 0,
    0, 0, draw_temple_rubble_torch, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, draw_temp_sacred_flame
};
