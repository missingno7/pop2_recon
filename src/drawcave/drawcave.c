/* DRAWCAVE.C, DOS overlay 6 frame 2a93. */
typedef unsigned char Byte;
typedef signed char CellCoord;
typedef unsigned int Word;

typedef struct DrawCell {
    Byte pass;
    CellCoord column, row;
    Byte foreground;
    Word modifier, background;
} DrawCell;

typedef struct DrawRect {
    int top, left, bottom, right;
} DrawRect;

typedef int (far pascal *AddImage)(Byte, int, CellCoord, CellCoord, Byte, Byte);

#pragma pack(1)
typedef struct BubblePosition { int image, y, flags; } BubblePosition;
typedef struct LavaBubble {
    int active;
    signed char frame, kind;
    int flags, y, layer;
} LavaBubble;
typedef struct LavaInfo { Byte flags; LavaBubble bubbles[10]; } LavaInfo;
typedef struct BackgroundImage { int flags, id, handle; } BackgroundImage;
typedef struct TileNeighbor { Byte kind; Word modifier; } TileNeighbor;
typedef struct ShapeOffsets {
    char before_loose[0xda];
    int loose_x, loose_y;
    char before_loose_front[4];
    int loose_front_y;
    char before_gate[0x420 - 0xe4];
    int gate_y;
    char before_front_gate[4];
    int front_gate_y;
    char before_exit[0x18];
    int exit_y;
    char before_bubble[0x468 - 0x442];
    BubblePosition bubble;
    char before_top_edge[0x496 - 0x46e];
    int top_edge_x, top_edge_y;
} ShapeOffsets;
#pragma pack()

extern CellCoord redraw_column, redraw_row;
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
extern DrawRect clip_rect, GamePlayRect, CaveTorchRect;
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
extern Byte drawn_tile_kind;
extern int draw_pass_mode;
extern signed char *bubble_frames[];
extern TileNeighbor CellLeft;
extern BackgroundImage *background_shapes;
extern LavaInfo *get_lava_info(Byte, CellCoord, CellCoord);
extern void check_background_shape(int);
extern int far *RESOLVE_HANDLE_TO_FAR_PAYLOAD(int);
extern Byte PieceAboveId(CellCoord, CellCoord);
extern Byte PieceAboveRightId(CellCoord, CellCoord);
extern int IS_SIX_KIND(Byte);
extern DrawRect *ComputeOffsetCellRect(DrawRect *, const DrawRect *, int, int);
extern int EncloseRect(const DrawRect far *, const DrawRect far *);
static struct {
    int top_edge_x[7];
    BubblePosition bubble_positions[11];
    int dart_images[7];
    int loose_odd[12];
    int loose_even[12];
} cave_images = {
    {2, 5, 4, 6, 5, 20, 0},
    {
        {160, 17, -6}, {153, 18, -7}, {154, 22, -7}, {153, 20, -6},
        {155, 22, -6}, {156, 23, -7}, {157, 26, -6}, {158, 28, -6},
        {159, 31, -6}, {160, 31, -7}, {161, 35, -9}
    },
    {100, 102, 101, 100, 100, 100, 100},
    {0, 1, 0, 1, 1, 0, 0, 0, 1, 1, 1, 0},
    {0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1}
};

extern ShapeOffsets *shape_offsets;
extern int AddBImgToBTable(Byte, int, CellCoord, CellCoord, Byte, Byte);
extern int add_back_foreground_to_back(Byte, int, CellCoord, CellCoord, Byte, Byte);
extern int add_back_foreground_to_front(Byte, int, CellCoord, CellCoord, Byte, Byte);
extern int add_middle_item(Byte, int, CellCoord, CellCoord, Byte, Byte);
extern Byte PieceRightId(CellCoord, CellCoord);
extern Byte PieceLeftId(CellCoord, CellCoord);
extern int IsWall(Byte);
extern int EmptyRect(const DrawRect far *);
extern int SectRect(DrawRect far *, const DrawRect far *, const DrawRect far *);

void DrawCaveBotEdge(DrawCell *);
void DrawCaveExitMovable(DrawCell *);
void DrawCaveFloor(DrawCell *);
void DrawCaveTopEdge(DrawCell *);
void DrawCaveGateMovable(DrawCell *);
void DrawCaveLavaBubbles(DrawCell *, int);
void DrawCaveSpace(DrawCell *);
void DrawCaveTorch(DrawCell *);
void draw_cave_rubble_pieces(DrawCell *);

void DrawCaveBlock(DrawCell *cell)
{
    Word modifier;
    Word image;
    Byte group;

    modifier = cell->modifier;
    image = (modifier & 15) + 74;
    group = (modifier & 15) / 5;
    switch (cell->pass) {
    case 1:
        if ((image != 78 && image != 83) || cell->column != redraw_column - 1)
            add_middle_item(20, image, cell->column, cell->row, 10, 0);
        DrawCaveBotEdge(cell);
        if (modifier & 0x80)
            add_middle_item(51, group + 88, cell->column, cell->row, 0, 0);
        break;
    case 5:
        if (image != 78 && image != 83) {
            if (!IsWall(PieceRightId(cell->column, cell->row)))
                AddBImgToBTable(20, group + 85, cell->column, cell->row, 10, 0);
        } else {
            if (image == 78)
                image = 32;
            else if (image == 83)
                image = 33;
            else
                image = -1;
            if (image != (Word)-1)
                add_back_foreground_to_back(20, image, cell->column, cell->row, 10, 0);
        }
        if (modifier & 0x80)
            AddBImgToBTable(51, group + 90, cell->column, cell->row, 10, 0);
        break;
    }
}

void DrawCaveBotEdge(DrawCell *cell)
{
    int image;

    if (cell->row != 2 && cell->row == redraw_row - 1 && cell->column == redraw_column) {
        image = (cell->modifier >> 8) & 7;
        if (image)
            add_middle_item(52, image + 7, cell->column, cell->row, 10, 0);
    }
}

void DrawCaveDart(DrawCell *cell)
{
    DrawCell floor;

    floor = *cell;
    floor.modifier &= 0x3f02;
    DrawCaveFloor(&floor);
    if (cell->pass == 1)
        add_middle_item(36, cave_images.dart_images[cell->modifier & 15], cell->column, cell->row, 10, 0);
}

void DrawCaveExit1(DrawCell *cell)
{
    switch (cell->pass) {
    case 0:
        AddBImgToBTable(16, -2, cell->column, cell->row, 10, 0);
        break;
    case 1:
        add_middle_item(16, -2, cell->column, cell->row, 0, 0);
        DrawCaveBotEdge(cell);
        DrawCaveTopEdge(cell);
        break;
    }
}

void DrawCaveExit2(DrawCell *cell)
{
    switch (cell->pass) {
    case 0:
        AddBImgToBTable(17, -2, cell->column, cell->row, 10, 0);
        DrawCaveExitMovable(cell);
        break;
    case 1:
        add_middle_item(17, -2, cell->column, cell->row, 0, 0);
        DrawCaveBotEdge(cell);
        DrawCaveTopEdge(cell);
        break;
    }
}

void DrawCaveExit3(DrawCell *cell)
{
    DrawRect saved;

    if (cell->pass == 0) {
        if (cell->row != redraw_row - 1)
            AddBImgToBTable(0, 1, cell->column, cell->row, 0, 0);
        saved = clip_rect;
        clip_rect.bottom = cell->row * 63 + 66;
        if (!EmptyRect(&clip_rect))
            AddBImgToBTable(33, -2, cell->column, cell->row, 10, 0);
        clip_rect = saved;
    } else if (cell->pass == 1) {
        DrawCaveTopEdge(cell);
    }
}

void DrawCaveExit4(DrawCell *cell)
{
    DrawRect saved;

    if (cell->pass == 0) {
        if (cell->row != redraw_row - 1)
            AddBImgToBTable(0, 2, cell->column, cell->row, 0, 0);
        saved = clip_rect;
        clip_rect.bottom = cell->row * 63 + 66;
        if (!EmptyRect(&clip_rect))
            AddBImgToBTable(35, -2, cell->column, cell->row, 10, 0);
        clip_rect = saved;
    } else if (cell->pass == 1) {
        DrawCaveTopEdge(cell);
    }
}

void DrawCaveExitMovable(DrawCell *cell)
{
    DrawRect saved, doorway;
    AddImage draw;
    unsigned int lift;

    if (cell->pass == 0) {
        saved = clip_rect;
        doorway = GamePlayRect;
        doorway.top = cell->row * 63 + 1;
        if (SectRect(&clip_rect, &clip_rect, &doorway)) {
            lift = cell->modifier & 255;
            shape_offsets->exit_y = -(15 + (lift < 42 ? lift : 42));
            AddBImgToBTable(57, -2, cell->column, cell->row, 10, 0);
            draw = Kid.frame >= 217 && Kid.frame <= 226 ?
                add_back_foreground_to_front : add_back_foreground_to_back;
            draw(57, -2, cell->column, cell->row, 10, 0);
        }
        clip_rect = saved;
    }
}

void DrawCaveFloor(DrawCell *cell)
{
    AddImage draw = 0;
    int image;
    Byte mode;
    Word variant = cell->modifier & 3;

    switch (cell->pass) {
    case 0:
        image = variant + 16;
        mode = 0;
        draw = AddBImgToBTable;
        break;
    case 1:
        image = variant + 25;
        mode = 0;
        draw = add_middle_item;
        break;
    case 2:
        image = variant + 22;
        mode = 10;
        draw = add_back_foreground_to_front;
        break;
    case 5:
        if (IsWall(PieceLeftId(cell->column, cell->row)))
            AddBImgToBTable(54, variant + 19, cell->column, cell->row, 10, 0);
        image = variant + 22;
        mode = 10;
        draw = add_back_foreground_to_back;
        break;
    }
    if (draw) {
        draw(1, image, cell->column, cell->row, mode, 0);
        if (cell->pass == 1) {
            DrawCaveBotEdge(cell);
            DrawCaveTopEdge(cell);
        }
    }
}

void DrawCaveGate(DrawCell *cell)
{
    switch (cell->pass) {
    case 0:
        AddBImgToBTable(4, -2, cell->column, cell->row, 0, 0);
        if (IsWall(PieceLeftId(cell->column, cell->row)))
            AddBImgToBTable(54, 35, cell->column, cell->row, 10, 0);
        break;
    case 1:
        add_middle_item(4, -2, cell->column, cell->row, 0, 0);
        DrawCaveBotEdge(cell);
        DrawCaveTopEdge(cell);
        break;
    case 2:
        add_back_foreground_to_front(4, -2, cell->column, cell->row, 10, 0);
        break;
    case 5:
        add_back_foreground_to_back(4, -2, cell->column, cell->row, 10, 0);
        break;
    }
    DrawCaveGateMovable(cell);
}


void DrawCaveGateMovable(DrawCell *cell)
{
    DrawRect saved, gate;
    AddImage draw;
    unsigned int lift;
    int y;

    saved = clip_rect;
    gate = GamePlayRect;
    gate.top = cell->row * 63;
    if (cell->pass != 1)
        gate.top -= 16;
    if (SectRect(&clip_rect, &clip_rect, &gate)) {
        lift = (cell->modifier & 0xfc) >> 2;
        y = -3 - (lift < 50 ? lift : 50);
        switch (cell->pass) {
        default:
            draw = 0;
            break;
        case 1:
            shape_offsets->front_gate_y = y;
            draw = add_middle_item;
            break;
        case 2:
            shape_offsets->gate_y = y;
            draw = add_back_foreground_to_front;
            break;
        case 5:
            shape_offsets->gate_y = y;
            draw = add_back_foreground_to_back;
            break;
        }
        if (draw)
            draw(55, -2, cell->column, cell->row, 10, 0);
    }
    clip_rect = saved;
}

void DrawCaveGateSquishing(DrawCell *cell)
{
    DrawRect saved, gate;
    unsigned int lift;

    saved = clip_rect;
    gate = GamePlayRect;
    gate.top = cell->row * 63 - 16;
    if (SectRect(&clip_rect, (const DrawRect *)&Kid.bounds, &gate)) {
        lift = (cell->modifier & 0xfc) >> 2;
        shape_offsets->gate_y = -3 - (lift < 50 ? lift : 50);
        add_back_foreground_to_front(55, -2, cell->column, cell->row, 10, 0);
    }
    clip_rect = saved;
}

void DrawCaveLava1(DrawCell *cell)
{
    AddImage draw = 0;
    DrawCell floor;
    LavaInfo *lava;

    floor = *cell;
    ((Byte *)&floor.modifier)[0] = 0;
    lava = get_lava_info(Room.number, cell->column, cell->row);
    switch (cell->pass) {
    case 0:
        DrawCaveFloor(&floor);
        break;
    case 1:
        DrawCaveFloor(&floor);
        break;
    case 2:
        DrawCaveFloor(&floor);
        if (lava && (!(lava->flags & 0x80) || (signed char)(lava->flags & 0x3f) >= 3))
            draw = add_back_foreground_to_front;
        break;
    case 5:
        DrawCaveFloor(&floor);
        if (!draw_pass_mode && lava &&
            (!(lava->flags & 0x80) || (signed char)(lava->flags & 0x3f) >= 3))
            draw = add_back_foreground_to_back;
        break;
    case 11:
        DrawCaveLavaBubbles(cell, 0);
        break;
    }
    if (draw) {
        draw(23, 145, cell->column, cell->row, 0, 0);
        if (cell->pass == 2)
            DrawCaveLavaBubbles(cell, 0);
    }
}

void DrawCaveLava2(DrawCell *cell)
{
    AddImage draw = 0;
    DrawCell floor;
    LavaInfo *lava;

    floor = *cell;
    ((Byte *)&floor.modifier)[0] = 0;
    ++floor.modifier;
    lava = get_lava_info(Room.number, cell->column, cell->row);
    switch (cell->pass) {
    case 0:
        DrawCaveFloor(&floor);
        break;
    case 1:
        DrawCaveFloor(&floor);
        break;
    case 2:
        DrawCaveFloor(&floor);
        if (lava && (!(lava->flags & 0x80) || (signed char)(lava->flags & 0x3f) >= 3))
            draw = add_back_foreground_to_front;
        break;
    case 5:
        DrawCaveFloor(&floor);
        if (lava && (!(lava->flags & 0x80) || (signed char)(lava->flags & 0x3f) >= 3))
            draw = add_back_foreground_to_back;
        break;
    case 11:
        DrawCaveLavaBubbles(cell, 1);
        break;
    }
    if (draw) {
        draw(24, 145, cell->column, cell->row, 0, 0);
        if (cell->pass == 2)
            DrawCaveLavaBubbles(cell, 0);
    }
}

void DrawCaveLavaBubbles(DrawCell *cell, int right)
{
    AddImage draw;
    LavaInfo *lava;
    signed char frame;
    int layer, index;

    if (draw_pass_mode && !right)
        return;
    draw = cell->pass == 2 ? add_back_foreground_to_front : add_back_foreground_to_back;
    lava = get_lava_info(Room.number, cell->column, cell->row);
    if (!lava)
        return;
    frame = lava->flags & 0x3f;
    if (lava->flags & 0x80) {
        draw(right + 23, frame + 146, cell->column, cell->row, 10, 0);
        if (frame < 3)
            return;
    }
    if ((lava->flags & 0x40) && frame >= 6 && frame <= 14) {
        shape_offsets->bubble.y = frame + 20;
        shape_offsets->bubble.flags = -8;
        /* The DOS branch requires both comparisons before the special arm. */
        if (frame == 6 && frame == 14) {
            shape_offsets->bubble.image = 152;
            if (frame == 6)
                shape_offsets->bubble.y += 3;
        } else {
            shape_offsets->bubble.image = 151;
        }
        if (right)
            shape_offsets->bubble.y -= 32;
        draw(59, -2, cell->column, cell->row, 10, 0);
    }
    layer = 0;
    do {
        for (index = 0; index < 10; ++index) {
            LavaBubble *bubble = &lava->bubbles[index];
            if (bubble->active && bubble->layer != layer) {
                signed char picture;

                shape_offsets->bubble.y = bubble->y - (right << 5);
                shape_offsets->bubble.flags = bubble->flags;
                picture = bubble_frames[bubble->kind][bubble->frame];
                shape_offsets->bubble.image = picture + 127;
                draw(59, -2, cell->column, cell->row, 10, 0);
            }
        }
        if (layer == 0) {
            draw(right + 23, 144, cell->column, cell->row, 10, 0);
        } else if ((lava->flags & 0x40) && frame >= 13 && frame <= 23) {
            shape_offsets->bubble = cave_images.bubble_positions[frame - 13];
            if (right)
                shape_offsets->bubble.y -= 32;
            draw(59, -2, cell->column, cell->row, 10, 0);
        }
    } while (++layer < 2);
}

void DrawCaveLoose(DrawCell *cell)
{
    int broken = (cell->modifier & 0x80) >> 7;
    int image = cell->modifier & 15;

    if (image != 13)
        image = cell->column % 2 ?
            ((int far *)cave_images.loose_odd)[image] :
            ((int far *)cave_images.loose_even)[image];
    switch (cell->pass) {
    case 0:
        AddBImgToBTable(11, broken + 64, cell->column, cell->row, 10, 0);
        break;
    case 1:
        if (image == 13)
            break;
        shape_offsets->loose_front_y = image + 5;
        add_middle_item(11, broken + 70, cell->column, cell->row, 10, 0);
        break;
    case 2:
        shape_offsets->loose_x = 16;
        shape_offsets->loose_y = -7;
        add_back_foreground_to_front(11, broken + 68, cell->column, cell->row, 10, 0);
        if (image == 13)
            break;
        shape_offsets->loose_x = 1;
        shape_offsets->loose_y = image + 1;
        add_back_foreground_to_front(11, broken + 66, cell->column, cell->row, 10, 0);
        break;
    case 5:
        shape_offsets->loose_x = 16;
        shape_offsets->loose_y = -7;
        add_back_foreground_to_back(11, broken + 68, cell->column, cell->row, 10, 0);
        if (image == 13)
            break;
        shape_offsets->loose_x = 1;
        shape_offsets->loose_y = image + 1;
        add_back_foreground_to_back(11, broken + 66, cell->column, cell->row, 10, 0);
        break;
    }
}

void DrawCavePillarBot(DrawCell *cell)
{
    DrawCaveFloor(cell);
    if (cell->pass == 1)
        add_middle_item(8, -2, cell->column, cell->row, 10, 0);
}

void DrawCavePillarTop(DrawCell *cell)
{
    switch (cell->pass) {
    case 0:
        ((Byte *)&cell->modifier)[0] &= 0xf8;
        DrawCaveSpace(cell);
        break;
    case 1:
        add_middle_item(9, -2, cell->column, cell->row, 10, 0);
        DrawCaveTopEdge(cell);
        break;
    }
}

void DrawCavePosts(DrawCell *cell)
{
    switch (cell->pass) {
    case 0:
        AddBImgToBTable(3, -2, cell->column, cell->row, 10, 0);
        break;
    case 1:
        add_middle_item(3, -2, cell->column, cell->row, 10, 0);
        DrawCaveBotEdge(cell);
        DrawCaveTopEdge(cell);
        break;
    case 2:
        add_back_foreground_to_front(3, -2, cell->column, cell->row, 10, 0);
        break;
    case 5:
        add_back_foreground_to_back(3, -2, cell->column, cell->row, 10, 0);
        break;
    }
}

void DrawCaveRevPlate(DrawCell *cell)
{
    AddImage draw = 0;

    switch (cell->pass) {
    case 0:
        AddBImgToBTable(34, -2, cell->column, cell->row, 0, 0);
        if (IsWall(PieceLeftId(cell->column, cell->row)))
            draw = AddBImgToBTable;
        break;
    case 1:
        add_middle_item(34, -2, cell->column, cell->row, 0, 0);
        DrawCaveBotEdge(cell);
        break;
    case 2:
        add_back_foreground_to_front(34, -2, cell->column, cell->row, 10, 0);
        if (cell->modifier & 0x0800)
            draw = add_back_foreground_to_front;
        break;
    case 5:
        add_back_foreground_to_back(34, -2, cell->column, cell->row, 10, 0);
        if (cell->modifier & 0x0800)
            draw = add_back_foreground_to_back;
        break;
    }
    if (draw)
        draw(56, -2, cell->column, cell->row, 10, 0);
}

void draw_cave_rubble(DrawCell *cell)
{
    if (!(cell->modifier & 0x0800))
        DrawCaveFloor(cell);
    else
        DrawCaveRevPlate(cell);
    draw_cave_rubble_pieces(cell);
}

void draw_cave_rubble_pieces(DrawCell *cell)
{
    AddImage draw;

    switch (cell->pass) {
    case 1:
        draw = add_middle_item;
        break;
    case 2:
        draw = add_back_foreground_to_front;
        break;
    case 5:
        draw = add_back_foreground_to_back;
        break;
    default:
        draw = 0;
        break;
    }
    if (draw)
        draw(14, -2, cell->column, cell->row, 10, 0);
}

void draw_cave_rubble_torch(DrawCell *cell)
{
    DrawCaveTorch(cell);
    draw_cave_rubble_pieces(cell);
}

void DrawCaveSpace(DrawCell *cell)
{
    volatile Word modifier = cell->modifier;
    int draw_loose = 0;
    int draw_floor;
    DrawCell loose;

    if (cell->pass == 0 || cell->pass == 5) {
        modifier &= 7;
        if (cell->row == redraw_row) { draw_floor = 1; }
        else if (cell->row == redraw_row + 1 &&
                   (IS_SIX_KIND(PieceAboveId(cell->column, cell->row)) ||
                    IS_SIX_KIND(PieceAboveRightId(cell->column, cell->row)))) { draw_floor = 1; }
        else if (redraw_row == -1 && IS_SIX_KIND(drawn_tile_kind)) { draw_floor = 1; }
        else {
            draw_floor = 0;
        }
    }
    switch (cell->pass) {
    case 0:
        if (!draw_floor)
            break;
        if (modifier < 3) {
            AddBImgToBTable(0, modifier + 1, cell->column, cell->row, 0, 0);
            if ((!IS_SIX_KIND(CellLeft.kind) || CellLeft.modifier >= 3) &&
                cell->column == redraw_column)
                AddBImgToBTable(53, modifier + 4, cell->column, cell->row, 10, 0);
        } else {
            draw_loose = 1;
        }
        break;
    case 1:
        DrawCaveTopEdge(cell);
        break;
    case 5:
        if (!draw_floor)
            break;
        if (modifier < 3) {
            if ((!IS_SIX_KIND(CellLeft.kind) || CellLeft.modifier >= 3) &&
                cell->column == redraw_column)
                AddBImgToBTable(53, modifier + 4, cell->column, cell->row, 10, 0);
        } else {
            draw_loose = 1;
        }
        break;
    }
    if (draw_loose) {
        modifier = modifier == 4 ? 0x80 : 0;
        loose = *cell;
        loose.modifier = modifier | 13;
        DrawCaveLoose(&loose);
    }
}

void DrawCaveSpikes(DrawCell *cell)
{
    DrawCell block;

    block = *cell;
    ((Byte *)&block.modifier)[0] &= 0xf0;
    ((Byte *)&block.modifier)[0] |= 0x80;
    switch (cell->pass) {
    case 0:
        DrawCaveBlock(&block);
        break;
    case 1:
        DrawCaveBlock(&block);
        break;
    case 5:
        if (cell->column != 9) {
            DrawCaveBlock(&block);
            AddBImgToBTable(2, -2, cell->column, cell->row, 10, 0);
        }
        break;
    }
}

void DrawCaveTopEdge(DrawCell *cell)
{
    int image;
    int far *height;

    if (cell->column == redraw_column && cell->row == redraw_row) {
        image = (cell->modifier & 0x3800) >> 11;
        if (image) {
            shape_offsets->top_edge_x = cave_images.top_edge_x[image - 1];
            image += 91;
            check_background_shape(image);
            height = RESOLVE_HANDLE_TO_FAR_PAYLOAD(background_shapes[image].handle);
            shape_offsets->top_edge_y = *height - 63;
            add_middle_item(61, image, cell->column, cell->row, 10, 0);
        }
    }
}

void DrawCaveTorch(DrawCell *cell)
{
    DrawCell floor;
    DrawRect bounds;
    int enclosed;

    floor = *cell;
    ((Byte *)&floor.modifier)[0] &= 0xfc;
    ComputeOffsetCellRect(&bounds, &CaveTorchRect, cell->column, cell->row);
    enclosed = EncloseRect(&bounds, &clip_rect);
    switch (cell->pass) {
    case 0:
        if (!enclosed) {
            DrawCaveFloor(&floor);
            AddBImgToBTable(19, -2, cell->column, cell->row, 10, 0);
        }
        AddBImgToBTable(58, (Byte)cell->modifier + 53, cell->column, cell->row, 10, 0);
        break;
    case 5:
        if (enclosed)
            break;
        /* Fall through to the shared floor pass. */
    case 1:
        DrawCaveFloor(&floor);
        break;
    case 2:
        DrawCaveFloor(&floor);
        break;
    }
}

int far _fastcall IsRoundCaveBlock(Word modifier)
{
    int round;

    modifier &= 15;
    if (modifier == 1 || modifier == 4 || modifier == 6 || modifier == 9)
        round = 1;
    else
        round = 0;
    return round;
}

void load_cave_drawing(void)
{
}

extern void far pascal draw_flask(DrawCell near *);
extern void far pascal draw_carpet(DrawCell near *);
extern void far pascal draw_cave_bars(DrawCell near *);
extern void far pascal draw_bridge_floor(DrawCell near *);

typedef void (far pascal *LevelTileProc)(DrawCell near *);
LevelTileProc drawcave_level_spec[] = {
    DrawCaveSpace, DrawCaveFloor, DrawCaveSpikes, DrawCavePosts, DrawCaveGate,
    0, 0, 0, DrawCavePillarBot, DrawCavePillarTop,
    draw_flask, DrawCaveLoose, 0, 0, draw_cave_rubble,
    0, DrawCaveExit1, DrawCaveExit2, draw_carpet, DrawCaveTorch,
    DrawCaveBlock, 0, 0, DrawCaveLava1, DrawCaveLava2,
    0, 0, draw_cave_bars, 0, 0,
    0, 0, draw_cave_rubble_torch, DrawCaveExit3, DrawCaveRevPlate,
    DrawCaveExit4, DrawCaveDart, 0, 0, 0,
    0, 0, 0, 0, draw_bridge_floor
};
