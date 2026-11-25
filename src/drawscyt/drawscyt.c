/* DRAWSCYT.C, DOS overlay 7. */
#include "types.h"
#include "level.h"

typedef struct ScytheCell {
    byte pass;
    sbyte column, row;
    byte foreground;
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
typedef int (far pascal *AddImage)(byte, int, sbyte, sbyte, byte, byte);

extern LEVELREC Level;
extern void DrawRuinRubblePieces(ScytheCell *);
extern void DrawTempRubblePieces(ScytheCell *);
extern void DrawRuinFloor(ScytheCell *);
extern void DrawTempFloor(ScytheCell *);
extern int AddBImgToBTable(byte, int, sbyte, sbyte, byte, byte);
extern int AddBFImgToFTable(byte, int, sbyte, sbyte, byte, byte);
extern int AddBFImgToBTable(byte, int, sbyte, sbyte, byte, byte);
extern ShapeOffsets *Pieces;
/* Consecutive six-byte records starting with scythe image 97. */
static ScythePosition scythe_positions[] = {
    {97, 22, -33}, {98, 28, -33}, {99, 41, -33},
    {100, 24, -33}, {101, 43, -36}, {102, 7, -24},
    {103, 15, -23}, {104, 48, -23}, {105, 42, -29}
};
extern int ScytheImg[];
extern int NewRoom;
extern sbyte ColNo;

void CheckScytheForRubble(ScytheCell *cell)
{
    ScytheCell base;

    if (cell->modifier & 0x20) {
        if (Level.environment_kind == 4) {
            base = *cell;
            base.modifier = 0;
            DrawRuinRubblePieces(&base);
        } else {
            DrawTempRubblePieces(cell);
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
        draw_base = DrawRuinFloor;
    else
        draw_base = DrawTempFloor;

    switch (cell->pass) {
    case 0: {
        int open_frame;

        draw_base(&base);
        if (cell->modifier & 0x80)
            open_frame = 96;
        else
            open_frame = 95;
        Pieces->scythe1_frame = open_frame;
        add_image = AddBImgToBTable;
        break;
    }
    case 1:
        draw_base(&base);
        frame = ScytheImg[cell->modifier & 31];
        if (frame >= 97 && frame <= 99) {
            Pieces->scythe1_position = scythe_positions[frame - 92];
            if (mirrored)
                Pieces->scythe1_position.image += 9;
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
            Pieces->scythe1_position = scythe_positions[frame - 97];
            if (mirrored)
                Pieces->scythe1_position.image += 9;
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
        draw_base = DrawRuinFloor;
    else
        draw_base = DrawTempFloor;

    switch (cell->pass) {
    case 0:
        draw_base(&base);
        if (!NewRoom && cell->column - ColNo + 1 == 0) {
            int open_frame;

            if (cell->modifier & 0x80)
                open_frame = 96;
            else
                open_frame = 95;
            Pieces->scythe2_frame = open_frame;
            add_image = AddBImgToBTable;
        }
        break;
    case 1:
        draw_base(&base);
        frame = ScytheImg[cell->modifier & 31];
        if (frame == 100 || frame == 101) {
            if (frame != 101) {
                Pieces->scythe2_position = scythe_positions[frame - 92];
                if (mirrored)
                    Pieces->scythe2_position.image += 9;
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
            Pieces->scythe2_position = scythe_positions[frame - 97];
            if (mirrored)
                Pieces->scythe2_position.image += 9;
            add_image = AddBFImgToBTable;
        }
        break;
    }

    if (add_image)
        add_image(13, -2, cell->column, cell->row, 10, 0);
    CheckScytheForRubble(cell);
}
