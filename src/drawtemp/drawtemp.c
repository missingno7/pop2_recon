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

extern int draw_pass_mode;
extern DrawRect clip_rect, exit_clip;
extern int row_edges[];
extern ShapeOffsets *shape_offsets;
extern Byte drawn_room;
extern unsigned int kid_frame;
extern BackImage far *far _fastcall get_back_table_image_row(int image, Byte column, Byte row);
extern int intersect_rect(DrawRect far *out, const DrawRect far *a, const DrawRect far *b);
extern void union_rect(DrawRect far *out, const DrawRect far *a, const DrawRect far *b);
extern int test_back_neighbor(CellCoord column, CellCoord row);
extern int is_rectangle_empty(const DrawRect far *rect);
extern int AddBImgToBTable(Byte, int, CellCoord, CellCoord, Byte, Byte);
extern int AddBFImgToBTable(Byte, int, CellCoord, CellCoord, Byte, Byte);
extern int add_foreground_image(Byte, int, CellCoord, CellCoord, Byte, Byte);
extern int AddBFImgToFTable(Byte, int, CellCoord, CellCoord, Byte, Byte);
extern Byte PieceLeftId(CellCoord, CellCoord);
extern int IsWall(Byte);
extern Byte PieceBelowId(CellCoord, CellCoord);
extern Byte PieceRightId(CellCoord, CellCoord);
extern signed char FirstInRow(CellCoord);
extern MovingExit *find_moving_exit(Byte room, int tile);

extern int loose_odd[], loose_even[];
extern signed char redraw_row;
extern Byte redraw_below_tile;
extern int is_six_kind(Byte);
extern DrawRect torch_bounds;
extern DrawRect *ComputeOffsetCellRect(DrawRect *, const DrawRect *, int, int);
extern int EncloseRect(const DrawRect far *, const DrawRect far *);
extern int temple_draw_state;

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
    struct { int edge; DrawRect overlap; } clipped;
    int saved_top;

    modifier = cell->background_modifier & 15;
    image = cell->background & 31;
    if (image) {
        if (image == 20) {
            image = 19;
            paired = 1;
        } else {
            paired = 0;
        }

        if (!draw_pass_mode) {
            shape = get_back_table_image_row(image, cell->column - 1, cell->row);
            if (shape && intersect_rect(&clipped.overlap, &shape->clip, &clip_rect)) {
                fallback = 0;
                union_rect(&shape->clip, &shape->clip, &clip_rect);
                if (paired) {
                    shape = get_back_table_image_row(image + 1, cell->column - 1, cell->row);
                    if (shape && intersect_rect(&clipped.overlap, &shape->clip, &clip_rect))
                        union_rect(&shape->clip, &shape->clip, &clip_rect);
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
            saved_top = clip_rect.top;
            if (cell->row >= 0 && !test_back_neighbor(cell->column, cell->row)) {
                clipped.edge = row_edges[cell->row];
                if (clipped.edge > clip_rect.left) {
                    clip_rect.top = clipped.edge < saved_top ? saved_top : clipped.edge;
                    if (is_rectangle_empty(&clip_rect))
                        fallback = 0;
                }
            }
            if (fallback) {
                modifier += 36;
                AddBImgToBTable(modifier, image, cell->column, cell->row, 0, 0);
                if (paired)
                    AddBFImgToBTable(modifier, -2, cell->column, cell->row, 0, 0);
            }
            clip_rect.top = saved_top;
        }
    }
}

#pragma optimize("g", off)
void draw_temple_block(TempleCell *cell)
{
    int image;
    Byte left;
    AddImage add_image;

    switch (cell->pass) {
    case 0:
        if (draw_pass_mode)
            draw_temple_back_wall(cell);
        break;
    case 1:
        image = (cell->modifier & 7) + 24;
        if (image > 25) {
            if (!draw_pass_mode) {
                shape_offsets->block_y = -32;
                image -= 2;
            } else {
                image = -1;
            }
        } else {
            shape_offsets->block_y = 0;
        }
        if (image != -1)
            add_foreground_image(20, image, cell->column, cell->row, 10, 0);
        image = (CELL_INFO_WORD(cell, 0) & 7) + 24;
        left = PieceLeftId(cell->column, cell->row);
        if (image <= 25 && IsWall(left))
            add_foreground_image(20, 61, cell->column, cell->row, 0, 0);
        if (IsWall(PieceBelowId(cell->column, cell->row))) {
            image = IsWall(left) ? 63 : 62;
            add_foreground_image(20, image, cell->column, cell->row, 0, 0);
        }
        break;
    case 2:
    case 5:
        image = (cell->modifier & 7) + 24;
        if ((image == 24 || image > 25) && !IsWall(PieceRightId(cell->column, cell->row))) {
            add_image = cell->pass == 5 ? AddBFImgToBTable : AddBFImgToFTable;
            add_image(20, -2, cell->column, cell->row, 10, 0);
        }
        break;
    }
}
#pragma optimize("g", on)

#pragma optimize("eg", off)
void draw_temple_exit_left(TempleCell *cell)
{
    Byte layer;
    unsigned long add_image = 0;
    switch (cell->pass) {
    case 0:
        if (draw_pass_mode)
            draw_temple_back_wall(cell);
        break;
    case 1:
        add_image = (unsigned long)add_foreground_image;
        layer = 0;
        break;
    case 2:
        add_image = (unsigned long)AddBFImgToFTable;
        goto alternate_layer;
    case 5:
        add_image = (unsigned long)AddBFImgToBTable;
    alternate_layer:
        layer = 10;
        break;
    }
    if (add_image)
        ((AddImage)add_image)(16, -2, cell->column, cell->row, layer, 0);
}
#pragma optimize("eg", on)

#pragma optimize("eg", off)
void draw_temple_exit_right(TempleCell *cell)
{
    Byte layer;
    unsigned long add_image = 0;
    switch (cell->pass) {
    case 0:
        if (draw_pass_mode) {
            draw_temple_back_wall(cell);
            draw_temple_exit_movable(cell);
        }
        break;
    case 1:
        add_image = (unsigned long)add_foreground_image;
        layer = 0;
        break;
    case 2:
        add_image = (unsigned long)AddBFImgToFTable;
        goto alternate_layer;
    case 5:
        add_image = (unsigned long)AddBFImgToBTable;
    alternate_layer:
        layer = 10;
        break;
    }
    if (add_image) {
        ((AddImage)add_image)(17, -2, cell->column, cell->row, layer, 0);
        if (cell->pass == 5)
            draw_temple_exit_movable(cell);
    }
}
#pragma optimize("eg", on)

#pragma optimize("e", off)
void draw_temple_exit_movable(TempleCell *cell)
{
    AddImage add_image;
    DrawRect door_clip, saved_clip;
    MovingExit *moving;
    if (cell->pass == 5) {
        saved_clip = clip_rect;
        door_clip = exit_clip;
        door_clip.top = cell->row * 63 - 5;
        if (intersect_rect(&clip_rect, &clip_rect, &door_clip)) {
            moving = find_moving_exit(drawn_room, cell->column + FirstInRow(cell->row));
            if (moving && moving->height >= 4)
                AddBImgToBTable(17, -2, cell->column, cell->row, 0, 0);
            shape_offsets->exit_y = -(19 + ((CELL_INFO_WORD(cell, 0) & 255) < 42 ? (CELL_INFO_WORD(cell, 0) & 255) : 42));
            AddBImgToBTable(57, -2, cell->column, cell->row, 10, 0);
            add_image = kid_frame >= 217 && kid_frame <= 226 ? AddBFImgToFTable : AddBFImgToBTable;
            add_image(57, -2, cell->column, cell->row, 10, 0);
        }
        clip_rect = saved_clip;
    }
}
#pragma optimize("e", on)

#pragma optimize("eg", off)
void draw_temple_floor(TempleCell *cell)
{
    Byte layer;
    int detail = 0;
    unsigned long add_image = 0;
    switch (cell->pass) {
    case 0:
        draw_temple_back_wall(cell);
        break;
    case 1:
        add_image = (unsigned long)add_foreground_image;
        layer = 0;
        break;
    case 2:
        add_image = (unsigned long)AddBFImgToFTable;
        goto alternate_layer;
    case 5:
        add_image = (unsigned long)AddBFImgToBTable;
    alternate_layer:
        layer = 10;
        detail = cell->modifier & 3;
        break;
    }
    if (add_image) {
        ((AddImage)add_image)(1, -2, cell->column, cell->row, layer, 0);
        if (detail)
            draw_temple_floor_detail(cell);
    }
}
#pragma optimize("eg", on)



#pragma optimize("g", off)
void draw_temple_floor_detail(TempleCell *cell)
{
    struct ShapePosition saved;
    AddImage add_image = cell->pass == 5 ? AddBFImgToBTable :
                         cell->pass == 2 ? AddBFImgToFTable : 0;
    if (add_image) {
        saved = shape_offsets->floor_detail;
        shape_offsets->floor_detail = shape_offsets->floor;
        ((AddImage)add_image)(1, -2, cell->column, cell->row, 0, 0);
        shape_offsets->floor_detail = saved;
    }
}

#pragma optimize("g", on)
#pragma optimize("e", off)
void draw_temple_gate(TempleCell *cell)
{
    unsigned long draw = 0;
    switch (cell->pass) {
    case 0:
        draw_temple_back_wall(cell);
        break;
    case 1:
        draw = (unsigned long)add_foreground_image;
        break;
    case 2:
        draw = (unsigned long)AddBFImgToFTable;
        break;
    case 5:
        draw = (unsigned long)AddBFImgToBTable;
        break;
    }
    if (draw) {
        ((AddImage)draw)(4, -2, cell->column, cell->row, 10, 0);
        draw_temple_gate_movable(cell);
    }
}
#pragma optimize("e", on)

void draw_temple_gate_movable(TempleCell *cell)
{
    AddImage draw;
    DrawRect gate_clip, saved_clip;
    unsigned int height;

    saved_clip = clip_rect;
    gate_clip = exit_clip;
    gate_clip.top = cell->row * 63 - 10;
    if (intersect_rect(&clip_rect, &clip_rect, &gate_clip)) {
        height = (cell->modifier & 252) >> 2;
        shape_offsets->gate_y = -(4 + (height < 50 ? height : 50));
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
    clip_rect = saved_clip;
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
        if (image && image != 13) {
            add_foreground_image(11, image + 35, cell->column, cell->row, 10, 0);
        } else if (!image) {
            goto floor_only;
        }
        break;
    case 2:
        if (image && image != 13) {
            AddBFImgToFTable(11, image + 33, cell->column, cell->row, 10, 0);
        } else if (!image) {
            goto floor_only;
        }
        break;
    case 5:
        if (image && image != 13) {
            AddBFImgToBTable(11, image + 33, cell->column, cell->row, 10, 0);
        } else if (!image) {
            goto floor_only;
        }
        break;
    floor_only:
        draw_temple_floor(&floor_cell);
        break;
    }
}

#pragma optimize("eg", off)
void draw_temple_pillar_bottom(TempleCell *cell)
{
    unsigned long draw = 0;
    switch (cell->pass) {
    case 0:
        draw_temple_back_wall(cell);
        break;
    case 1:
        draw = (unsigned long)add_foreground_image;
        break;
    case 5:
        draw = (unsigned long)AddBImgToBTable;
        break;
    }
    if (draw)
        ((AddImage)draw)(8, -2, cell->column, cell->row, 10, 0);
}
#pragma optimize("eg", on)

#pragma optimize("eg", off)
void draw_temple_pillar_top(TempleCell *cell)
{
    unsigned long draw = 0;
    switch (cell->pass) {
    case 0:
        draw_temple_back_wall(cell);
        break;
    case 1:
        draw = (unsigned long)add_foreground_image;
        break;
    case 5:
        draw = (unsigned long)AddBImgToBTable;
        break;
    }
    if (draw)
        ((AddImage)draw)(9, -2, cell->column, cell->row, 10, 0);
}
#pragma optimize("eg", on)

#pragma optimize("eg", off)
void draw_temple_down_plate(TempleCell *cell)
{
    Byte tile;
    int image;
    int pressed = CELL_INFO_WORD(cell, 0) & 0x0800;
    unsigned long draw = 0;

    switch (cell->pass) {
    case 0:
        draw_temple_back_wall(cell);
        break;
    case 1:
        tile = pressed ? 6 : 5;
        image = 33;
        draw = (unsigned long)add_foreground_image;
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
        draw = (unsigned long)(cell->pass == 5 ? AddBFImgToBTable : AddBFImgToFTable);
        break;
    }
    if (draw)
        ((AddImage)draw)(tile, image, cell->column, cell->row, 10, 0);
}
#pragma optimize("eg", on)

#pragma optimize("eg", off)
void draw_temple_up_plate(TempleCell *cell)
{
    int image;
    Byte tile = 5 + ((CELL_INFO_WORD(cell, 0) & 0x0800) != 0);
    unsigned long draw = 0;

    switch (cell->pass) {
    case 0:
        draw_temple_back_wall(cell);
        break;
    case 1:
        draw = (unsigned long)add_foreground_image;
        image = 32;
        break;
    case 2:
    case 5:
        draw = (unsigned long)(cell->pass == 5 ? AddBFImgToBTable : AddBFImgToFTable);
        image = 30;
        break;
    }
    if (draw)
        ((AddImage)draw)(tile, image, cell->column, cell->row, 10, 0);
}
#pragma optimize("eg", on)

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
        if (!image) {
            draw = add_foreground_image;
            image = 44;
        } else if (!draw_pass_mode || image == 1) {
            draw = add_foreground_image;
            shape_offsets->posts_y = 0;
            if (image == 2)
                shape_offsets->posts_y -= 32;
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
            if (shape_offsets->posts_y == 0 && PieceLeftId(cell->column, cell->row) != 3) {
                saved_y = shape_offsets->block_overlay_y;
                shape_offsets->block_overlay_y -= 54;
                add_foreground_image(20, 38, cell->column, cell->row, 10, 0);
                shape_offsets->block_overlay_y = saved_y;
            }
            shape_offsets->posts_y = -1;
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
    unsigned int pass = cell->pass;
    unsigned long draw = pass == 1 ? (unsigned long)add_foreground_image :
                         pass == 2 ? (unsigned long)AddBFImgToFTable :
                         pass == 5 ? (unsigned long)AddBFImgToBTable : 0;
    if (draw)
        ((AddImage)draw)(14, -2, cell->column, cell->row, 10, 0);
}

void draw_temple_space(TempleCell *cell)
{
    int draw;
    if (cell->pass == 0) {
        draw = 0;
        if (cell->row == redraw_row)
            draw = 1;
        else if (cell->row - redraw_row == 1)
            draw = test_back_neighbor(cell->column, cell->row);
        if (!draw && redraw_row == -1 && is_six_kind(redraw_below_tile))
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
        ComputeOffsetCellRect(&torch_clip, &torch_bounds, cell->column, cell->row);
        if (!EncloseRect(&torch_clip, &clip_rect)) {
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
            goto floor_only;
        break;
    case 1:
    case 2:
    case 5:
        if (!state) {
        floor_only:
            draw_temple_floor(cell);
        }
        break;
    }
}

void LoadTempleDrawCode(void)
{
    temple_draw_state = 0;
}

