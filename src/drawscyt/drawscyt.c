/* DRAWSCYT.C, DOS overlay 7. */
#include "types.h"
#include "graf.h"
#include "shape.h"
#include "level.h"
#include "char.h"
#include "game.h"
#include "draw.h"
#include "add.h"
#include "drawruin.h"
#include "drawscyt.h"
#include "drawtemp.h"
#include "fade.h"
#include "movescyt.h"
#include "prince.h"
#include "tables.h"


typedef void (*DrawScytheBase)(DRAWCELL *);
typedef int (*AddImage)(byte, int, sbyte, sbyte, byte, byte);


/* Consecutive six-byte records starting with scythe image 97. */
static IMAGEPOS scythe_positions[] = {
    {97, 22, -33}, {98, 28, -33}, {99, 41, -33},
    {100, 24, -33}, {101, 43, -36}, {102, 7, -24},
    {103, 15, -23}, {104, 48, -23}, {105, 42, -29}
};


void CheckScytheForRubble(DRAWCELL *cell)
{
    DRAWCELL base;

    if (cell->info.modifier & 0x20) {
        if (Level.environment_kind == 4) {
            base = *cell;
            base.info.modifier = 0;
            DrawRuinRubblePieces(&base);
        } else {
            DrawTempRubblePieces(cell);
        }
    }
}

void DrawScythe1(DRAWCELL *cell)
{
    DRAWCELL base;
    int mirrored;
    int frame;
    DrawScytheBase draw_base;
    AddImage add_image;

    add_image = 0;
    base = *cell;
    base.info.modifier = cell->info.modifier & 0xff00;
    mirrored = cell->info.modifier & 0x40;
    if (Level.environment_kind == 4)
        draw_base = DrawRuinFloor;
    else
        draw_base = DrawTempFloor;

    switch (cell->pass) {
    case 0: {
        int open_frame;

        draw_base(&base);
        if (cell->info.modifier & 0x80)
            open_frame = 96;
        else
            open_frame = 95;
        Pieces[12].back.image = open_frame;
        add_image = AddBImgToBTable;
        break;
    }
    case 1:
        draw_base(&base);
        frame = ScytheImg[cell->info.modifier & 31];
        if (frame >= 97 && frame <= 99) {
            Pieces[12].back_fore = scythe_positions[frame - 92];
            if (mirrored)
                Pieces[12].back_fore.image += 9;
            add_image = AddBFImgToFTable;
        }
        break;
    case 2:
        draw_base(&base);
        break;
    case 5:
        draw_base(&base);
        frame = ScytheImg[cell->info.modifier & 31];
        if (frame >= 97 && frame <= 99) {
            Pieces[12].back_fore = scythe_positions[frame - 97];
            if (mirrored)
                Pieces[12].back_fore.image += 9;
            add_image = AddBFImgToBTable;
        }
        break;
    }

    if (add_image)
        add_image(12, -2, cell->column, cell->row, 10, 0);
    CheckScytheForRubble(cell);
}

void DrawScythe2(DRAWCELL *cell)
{
    DRAWCELL base;
    int mirrored;
    int frame;
    DrawScytheBase draw_base;
    AddImage add_image;

    add_image = 0;
    base = *cell;
    base.info.modifier = cell->info.modifier & 0xff00;
    mirrored = cell->info.modifier & 0x40;
    if (Level.environment_kind == 4)
        ++base.info.modifier;
    if (Level.environment_kind == 4)
        draw_base = DrawRuinFloor;
    else
        draw_base = DrawTempFloor;

    switch (cell->pass) {
    case 0:
        draw_base(&base);
        if (!NewRoom && cell->column - ColNo + 1 == 0) {
            int open_frame;

            if (cell->info.modifier & 0x80)
                open_frame = 96;
            else
                open_frame = 95;
            Pieces[13].back.image = open_frame;
            add_image = AddBImgToBTable;
        }
        break;
    case 1:
        draw_base(&base);
        frame = ScytheImg[cell->info.modifier & 31];
        if (frame == 100 || frame == 101) {
            if (frame != 101) {
                Pieces[13].back_fore = scythe_positions[frame - 92];
                if (mirrored)
                    Pieces[13].back_fore.image += 9;
                add_image = AddBFImgToFTable;
            }
        }
        break;
    case 2:
        draw_base(&base);
        break;
    case 5:
        draw_base(&base);
        frame = ScytheImg[cell->info.modifier & 31];
        if (frame == 100 || frame == 101) {
            Pieces[13].back_fore = scythe_positions[frame - 97];
            if (mirrored)
                Pieces[13].back_fore.image += 9;
            add_image = AddBFImgToBTable;
        }
        break;
    }

    if (add_image)
        add_image(13, -2, cell->column, cell->row, 10, 0);
    CheckScytheForRubble(cell);
}
