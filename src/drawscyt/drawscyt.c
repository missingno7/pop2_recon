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
        if (Level.environment_kind == 4) {
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
    DrawScytheBase draw_base;
    AddImage add_image;

    add_image = 0;
    base = *cell;
    base.modifier = cell->modifier & 0xff00;
    mirrored = cell->modifier & 0x40;
    if (Level.environment_kind == 4)
        draw_base = draw_ruin_floor;
    else
        draw_base = draw_temple_floor;

    switch (cell->pass) {
    case 0: {
        int open_frame;

        draw_base(&base);
        if (cell->modifier & 0x80)
            open_frame = 96;
        else
            open_frame = 95;
        shape_offsets->scythe1_frame = open_frame;
        add_image = AddBImgToBTable;
        break;
    }
    case 1:
        draw_base(&base);
        frame = ScytheImg[cell->modifier & 31];
        if (frame >= 97 && frame <= 99) {
            shape_offsets->scythe1_position = scythe_positions[frame - 92];
            if (mirrored)
                shape_offsets->scythe1_position.image += 9;
            add_image = AddBFImgToFTable;
        }
        break;
    case 2:
        draw_base(&base);
        break;
    case 5:
        draw_base(&base);
        frame = ScytheImg[cell->modifier & 31];
        if (frame >= 97 && frame <= 99) {
            shape_offsets->scythe1_position = scythe_positions[frame - 97];
            if (mirrored)
                shape_offsets->scythe1_position.image += 9;
            add_image = AddBFImgToBTable;
        }
        break;
    }

    if (add_image)
        add_image(12, -2, cell->column, cell->row, 10, 0);
    CheckScytheForRubble(cell);
}

void DrawScythe2(ScytheCell *cell)
{
    ScytheCell base;
    int mirrored;
    int frame;
    DrawScytheBase draw_base;
    AddImage add_image;

    add_image = 0;
    base = *cell;
    base.modifier = cell->modifier & 0xff00;
    mirrored = cell->modifier & 0x40;
    if (Level.environment_kind == 4)
        ++base.modifier;
    if (Level.environment_kind == 4)
        draw_base = draw_ruin_floor;
    else
        draw_base = draw_temple_floor;

    switch (cell->pass) {
    case 0:
        draw_base(&base);
        if (!draw_pass_mode && cell->column - redraw_column + 1 == 0) {
            int open_frame;

            if (cell->modifier & 0x80)
                open_frame = 96;
            else
                open_frame = 95;
            shape_offsets->scythe2_frame = open_frame;
            add_image = AddBImgToBTable;
        }
        break;
    case 1:
        draw_base(&base);
        frame = ScytheImg[cell->modifier & 31];
        if (frame == 100 || frame == 101) {
            if (frame != 101) {
                shape_offsets->scythe2_position = scythe_positions[frame - 92];
                if (mirrored)
                    shape_offsets->scythe2_position.image += 9;
                add_image = AddBFImgToFTable;
            }
        }
        break;
    case 2:
        draw_base(&base);
        break;
    case 5:
        draw_base(&base);
        frame = ScytheImg[cell->modifier & 31];
        if (frame == 100 || frame == 101) {
            shape_offsets->scythe2_position = scythe_positions[frame - 97];
            if (mirrored)
                shape_offsets->scythe2_position.image += 9;
            add_image = AddBFImgToBTable;
        }
        break;
    }

    if (add_image)
        add_image(13, -2, cell->column, cell->row, 10, 0);
    CheckScytheForRubble(cell);
}
