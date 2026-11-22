/* DRAWSCYT.C, DOS overlay 7. */
typedef unsigned char Byte;
typedef signed char CellCoord;

typedef struct ScytheCell {
    Byte pass;
    CellCoord column, row;
    Byte foreground;
    unsigned int modifier, background;
} ScytheCell;

typedef struct ScythePosition {
    int image, x, y;
} ScythePosition;

#pragma pack(1)
typedef struct ShapeOffsets {
    char before_scythe1[0xe5];
    int scythe1_frame;
    char before_scythe1_position[4];
    ScythePosition scythe1_position;
    char before_scythe2[7];
    int scythe2_frame;
    char before_scythe2_position[4];
    ScythePosition scythe2_position;
} ShapeOffsets;
#pragma pack()

typedef void (far pascal *DrawScytheBase)(ScytheCell *);
typedef int (far pascal *AddImage)(Byte, int, CellCoord, CellCoord, Byte, Byte);

typedef struct ScytheDispatch {
    DrawScytheBase base;
    unsigned long add;  /* Stored 16:16 address of the selected AddImage callback. */
} ScytheDispatch;

extern Byte level_type;
extern void draw_ruin_rubble_pieces(ScytheCell *);
extern void draw_temple_rubble_pieces(ScytheCell *);
extern void draw_ruin_floor(ScytheCell *);
extern void draw_temple_floor(ScytheCell *);
extern int AddBImgToBTable(Byte, int, CellCoord, CellCoord, Byte, Byte);
extern int AddBFImgToFTable(Byte, int, CellCoord, CellCoord, Byte, Byte);
extern int AddBFImgToBTable(Byte, int, CellCoord, CellCoord, Byte, Byte);
extern ShapeOffsets *shape_offsets;
/* Consecutive six-byte records starting with scythe image 97. */
static ScythePosition scythe_positions[] = {
    {97, 22, -33}, {98, 28, -33}, {99, 41, -33},
    {100, 24, -33}, {101, 43, -36}, {102, 7, -24},
    {103, 15, -23}, {104, 48, -23}, {105, 42, -29}
};
extern int ScytheImg[];
extern int draw_pass_mode;
extern CellCoord redraw_column;

void CheckScytheForRubble(ScytheCell *cell)
{
    ScytheCell base;

    if (cell->modifier & 0x20) {
        if (level_type == 4) {
            base = *cell;
            base.modifier = 0;
            draw_ruin_rubble_pieces(&base);
        } else {
            draw_temple_rubble_pieces(cell);
        }
    }
}

void DrawScythe1(ScytheCell *cell)
{
    ScytheCell base;
    int mirrored;
    int frame;
    ScytheDispatch dispatch;

    dispatch.add = 0;
    base = *cell;
    base.modifier = cell->modifier & 0xff00;
    mirrored = cell->modifier & 0x40;
    if (level_type == 4)
        dispatch.base = draw_ruin_floor;
    else
        dispatch.base = draw_temple_floor;

    switch (cell->pass) {
    case 0: {
        int open_frame;

        dispatch.base(&base);
        if (cell->modifier & 0x80)
            open_frame = 96;
        else
            open_frame = 95;
        shape_offsets->scythe1_frame = open_frame;
        dispatch.add = (unsigned long)AddBImgToBTable;
        break;
    }
    case 1:
        dispatch.base(&base);
        frame = ScytheImg[cell->modifier & 31];
        if (frame >= 97 && frame <= 99) {
            shape_offsets->scythe1_position = scythe_positions[frame - 92];
            if (mirrored)
                shape_offsets->scythe1_position.image += 9;
            dispatch.add = (unsigned long)AddBFImgToFTable;
        }
        break;
    case 2:
        dispatch.base(&base);
        break;
    case 5:
        dispatch.base(&base);
        frame = ScytheImg[cell->modifier & 31];
        if (frame >= 97 && frame <= 99) {
            shape_offsets->scythe1_position = scythe_positions[frame - 97];
            if (mirrored)
                shape_offsets->scythe1_position.image += 9;
            dispatch.add = (unsigned long)AddBFImgToBTable;
        }
        break;
    }

    if (dispatch.add)
        ((AddImage)dispatch.add)(12, -2, cell->column, cell->row, 10, 0);
    CheckScytheForRubble(cell);
}

void DrawScythe2(ScytheCell *cell)
{
    ScytheCell base;
    int mirrored;
    int frame;
    ScytheDispatch dispatch;

    dispatch.add = 0;
    base = *cell;
    base.modifier = cell->modifier & 0xff00;
    mirrored = cell->modifier & 0x40;
    if (level_type == 4)
        ++base.modifier;
    if (level_type == 4)
        dispatch.base = draw_ruin_floor;
    else
        dispatch.base = draw_temple_floor;

    switch (cell->pass) {
    case 0:
        dispatch.base(&base);
        if (!draw_pass_mode && cell->column - redraw_column + 1 == 0) {
            int open_frame;

            if (cell->modifier & 0x80)
                open_frame = 96;
            else
                open_frame = 95;
            shape_offsets->scythe2_frame = open_frame;
            dispatch.add = (unsigned long)AddBImgToBTable;
        }
        break;
    case 1:
        dispatch.base(&base);
        frame = ScytheImg[cell->modifier & 31];
        if (frame == 100 || frame == 101) {
            if (frame != 101) {
                shape_offsets->scythe2_position = scythe_positions[frame - 92];
                if (mirrored)
                    shape_offsets->scythe2_position.image += 9;
                dispatch.add = (unsigned long)AddBFImgToFTable;
            }
        }
        break;
    case 2:
        dispatch.base(&base);
        break;
    case 5:
        dispatch.base(&base);
        frame = ScytheImg[cell->modifier & 31];
        if (frame == 100 || frame == 101) {
            shape_offsets->scythe2_position = scythe_positions[frame - 97];
            if (mirrored)
                shape_offsets->scythe2_position.image += 9;
            dispatch.add = (unsigned long)AddBFImgToBTable;
        }
        break;
    }

    if (dispatch.add)
        ((AddImage)dispatch.add)(13, -2, cell->column, cell->row, 10, 0);
    CheckScytheForRubble(cell);
}
