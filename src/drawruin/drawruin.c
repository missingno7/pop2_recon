/* DRAWRUIN.C, DOS overlay 8 code frame 2a74. */
#include "types.h"
#include "graf.h"
#include "shape.h"
#include "level.h"
#include "char.h"
#include "game.h"
#include "draw.h"
#include "add.h"
#include "ctrl.h"
#include "drawruin.h"
#include "drawscyt.h"
#include "fade.h"
#include "misc.h"
#include "move.h"
#include "prince.h"
#include "tables.h"
#include "throne.h"
#include "top.h"


typedef int (*AddImage)(byte, int, sbyte, sbyte, byte, byte);


void DrawRuinBackWall(DRAWCELL *cell)
{
    int image;
    byte background_modifier, tile, layer;
    int fallback;
    BFITEM far *shape;
    Rect overlap;

    background_modifier = ((byte *)&cell->info.background)[1] & 15;
    image = cell->info.background & 31;
    if (image) {
        if (!NewRoom) {
            shape = GetBackTableImgRow(image, cell->column - 1, cell->row);
            if (shape && SectRect(&overlap, &shape->clip, &CellClipRect)) {
                fallback = 0;
                UnionRect(&shape->clip, &shape->clip, &CellClipRect);
                if (image >= 1 && image <= 5) {
                    shape = GetBackTableImgRow(7, cell->column - 1, cell->row);
                    if (shape && SectRect(&overlap, &shape->clip, &CellClipRect))
                        UnionRect(&shape->clip, &shape->clip, &CellClipRect);
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

void DrawRuinBlock(DRAWCELL *cell)
{
    int image;

    switch (cell->pass) {
    case 0:
        if (NewRoom)
            DrawRuinBackWall(cell);
        break;
    case 5:
        image = (cell->info.modifier & 12) >> 2;
        if (image != 2 && PieceRightId(cell->column, cell->row) != 20) {
            image = image != 3 ? image + 12 : 53;
            AddBImgToBTable(20, image, cell->column, cell->row, 10, 0);
        }
        /* Fall through to the block and its flaw. */
    case 1:
        AddFImgToFTable(20, (cell->info.modifier & 3) + 14, cell->column, cell->row, 0, 0);
        DrawRuinFlaw(cell);
        break;
    }
}

void DrawRuinBones(DRAWCELL *cell)
{
    AddImage add_image = 0;

    switch (cell->pass) {
    case 0:
        DrawRuinBackWall(cell);
        break;
    case 1:
        add_image = AddFImgToFTable;
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
        DrawRuinFlaw(cell);
    }
}


static union RuinTables {
    struct {
        int crevice_flags[4];
        int loose_odd[12], loose_even[12];
        IMAGEPOS flaw_positions[15], exit_positions[2];
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

void DrawRuinCrevice(DRAWCELL *cell)
{
    int pattern, revealed;
    DRAWCELL block;

    pattern = cell->info.modifier & 3;
    revealed = cell->info.modifier & 0x80;
    switch (cell->pass) {
    case 0:
        if (NewRoom)
            DrawRuinBackWall(cell);
        break;
    case 1:
        if (revealed) {
            AddFImgToFTable(7, pattern + 58, cell->column, cell->row, 10, 0);
        } else {
            block = *cell;
            block.info.modifier &= ~3;
            block.info.modifier |= ruin_tables.rows.crevice_flags[pattern];
            DrawRuinBlock(&block);
        }
        DrawRuinFlaw(cell);
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
            block.info.modifier |= pattern ? 8 : 12;
            DrawRuinBlock(&block);
        }
        break;
    }
}

void DrawRuinExit1(DRAWCELL *cell)
{
    AddImage add_image = 0;
    byte layer = 10;
    int flaw = 1;

    switch (cell->pass) {
    case 0:
        if (NewRoom) {
            DrawRuinBackWall(cell);
            add_image = AddBImgToBTable;
        }
        break;
    case 1:
        add_image = AddFImgToFTable;
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
        DrawRuinFlaw(cell);
}


void DrawRuinExit2(DRAWCELL *cell)
{
    AddImage add_image = 0;
    byte layer = 10;
    int flaw = 0;

    switch (cell->pass) {
    case 0:
        if (NewRoom) {
            DrawRuinBackWall(cell);
        } else {
            Pieces[57].back_fore = exit_positions[0];
            AddBFImgToBTable(57, -2, cell->column, cell->row, 10, 0);
            add_image = 0;
        }
        break;
    case 1:
        add_image = AddFImgToFTable;
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
        DrawRuinFlaw(cell);
    if (cell->pass == 0)
        DrawRuinExitMovable(cell);
}

void DrawRuinExitMovable(DRAWCELL *cell)
{
    AddImage add_image;
    TROBREC *moving;
    Rect saved_clip, door_clip;

    if (cell->pass == 0) {
        saved_clip = CellClipRect;
        door_clip = GamePlayRect;
        door_clip.top = cell->row * 63 - 5;
        if (SectRect(&CellClipRect, &CellClipRect, &door_clip)) {
            moving = GetTrobRoomCell(Room.number, FirstInRow(cell->row) + cell->column);
            if (moving && moving->state >= 4)
                AddBImgToBTable(17, -2, cell->column, cell->row, 10, 0);
            Pieces[57].back.y = -(int)(19 + ((cell->info.modifier & 0xff) < 42 ? cell->info.modifier & 0xff : 42));
            AddBImgToBTable(57, -2, cell->column, cell->row, 10, 0);
            add_image = Kid.frame >= 217 && Kid.frame <= 226 ? AddBFImgToFTable : AddBFImgToBTable;
            Pieces[57].back_fore = exit_positions[1];
            add_image(57, -2, cell->column, cell->row, 10, 0);
        }
        CellClipRect = saved_clip;
    }
}


void DrawRuinFlaw(DRAWCELL *cell)
{
    int flaw, flags;

    if (cell->pass == 5) {
        flaw = cell->info.modifier >> 8 & 15;
        if (flaw == 11 || flaw == 12) {
            Pieces[18].back = ruin_tables.rows.flaw_positions[flaw - 11];
            Pieces[18].peel = flaw == 12 ? 1 : 0;
            AddBImgToBTable(18, -2, cell->column, cell->row, 10, 0);
        } else {
            flaw = cell->info.modifier & 0x3000;
            if (flaw) {
                flaw >>= 12;
                Pieces[18].back_fore = ruin_tables.rows.flaw_positions[flaw + 11];
                AddBFImgToBTable(18, -2, cell->column, cell->row, 10, 0);
            }
        }
    } else if (cell->pass == 1) {
        flags = cell->info.modifier & 0xf00;
        if (flags && (word)flags < 0xb00) {
            flaw = ((byte *)&flags)[1];
            if (flaw != 2 || !NewRoom) {
                int emitted;
                Pieces[18].back_fore = ruin_tables.rows.flaw_positions[flaw + 1];
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

void DrawRuinFloor(DRAWCELL *cell)
{
    switch (cell->pass) {
    case 0:
        DrawRuinBackWall(cell);
        break;
    case 1:
        AddFImgToFTable(1, (cell->info.modifier & 3) + 26, cell->column, cell->row, 0, 0);
        DrawRuinFlaw(cell);
        break;
    case 2:
        AddBFImgToFTable(1, (cell->info.modifier & 3) + 22, cell->column, cell->row, 10, 0);
        break;
    case 5:
        AddBFImgToBTable(1, (cell->info.modifier & 3) + 22, cell->column, cell->row, 10, 0);
        DrawRuinFlaw(cell);
        break;
    }
}


void DrawRuinGate(DRAWCELL *cell)
{
    AddImage add_image = 0;

    switch (cell->pass) {
    case 0:
        DrawRuinBackWall(cell);
        break;
    case 1:
        add_image = AddFImgToFTable;
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
        DrawRuinGateMovable(cell);
        DrawRuinFlaw(cell);
    }
}


void DrawRuinGateMovable(DRAWCELL *cell)
{
    AddImage add_image;
    Rect gate_clip, saved_clip;

    saved_clip = CellClipRect;
    gate_clip = GamePlayRect;
    gate_clip.top = cell->row * 63 - 10;
    if (SectRect(&CellClipRect, &CellClipRect, &gate_clip)) {
        word height = (cell->info.modifier & 0xfc) >> 2;
        Pieces[55].back_fore.y = -(4 + (height < 50 ? height : 50));
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
            if (Room.number == 9 && Level.level_number == 8) {
                if (cell->pass == 2)
                    DrawThroneRoomGateInFore();
            } else {
                add_image(55, -2, cell->column, cell->row, 10, 0);
            }
        }
    }
    CellClipRect = saved_clip;
}

void DrawRuinLoose(DRAWCELL *cell)
{
    int image;
    DRAWCELL floor;
    image = cell->info.modifier & 15;
    if (image != 13)
        image = cell->column % 2 ? ruin_tables.words[LOOSE_ODD_WORD + image]
                                 : ruin_tables.words[LOOSE_EVEN_WORD + image];
    if (!image) {
        floor = *cell;
        floor.info.modifier &= ~3;
    }
    switch (cell->pass) {
    case 0:
        if (image != 13) {
            if (image)
                DrawRuinBackWall(cell);
            else
                DrawRuinFloor(&floor);
        } else {
            DrawRuinBackWall(cell);
        }
        break;
    case 1: {
        DRAWCELL *tile = cell;
        if (image == 13) {
            DrawRuinFlaw(tile);
            break;
        }
        if (!image) {
            DrawRuinFloor(&floor);
            break;
        }
        AddFImgToFTable(11, image + 43, tile->column, tile->row, 10, 0);
        DrawRuinFlaw(tile);
        break;
    }
    case 2:
        if (image != 13) {
            if (image) {
                DRAWCELL *tile = cell;
                AddBFImgToFTable(11, image + 41, tile->column, tile->row, 10, 0);
            } else {
                DrawRuinFloor(&floor);
            }
        }
        break;
    case 5: {
        DRAWCELL *tile = cell;
        if (image == 13) {
            DrawRuinFlaw(tile);
            break;
        }
        if (!image) {
            DrawRuinFloor(&floor);
            break;
        }
        AddBFImgToBTable(11, image + 41, tile->column, tile->row, 10, 0);
        DrawRuinFlaw(tile);
        break;
    }
    }
}

void DrawRuinPillarBot(DRAWCELL *cell)
{
    AddImage add_image = 0;

    switch (cell->pass) {
    case 0:
        DrawRuinBackWall(cell);
        add_image = AddBImgToBTable;
        break;
    case 1:
        add_image = AddFImgToFTable;
        break;
    }
    if (add_image) {
        add_image(8, -2, cell->column, cell->row, 10, 0);
        DrawRuinFlaw(cell);
    }
}

void DrawRuinPillarTop(DRAWCELL *cell)
{
    AddImage add_image;

    switch (cell->pass) {
    case 0:
        DrawRuinBackWall(cell);
        add_image = AddBImgToBTable;
        break;
    case 1:
        add_image = AddFImgToFTable;
        break;
    default:
        add_image = 0;
        break;
    }
    if (add_image) {
        add_image(9, -2, cell->column, cell->row, 10, 0);
        DrawRuinFlaw(cell);
    }
}

void DrawRuinDownPlate(DRAWCELL *cell)
{
    byte layer;
    int image;
    AddImage add_image = 0;
    byte tile = 5 + ((cell->info.modifier & 0x800) != 0);

    switch (cell->pass) {
    case 0:
        DrawRuinBackWall(cell);
        break;
    case 1:
        image = 52;
        layer = 0;
        add_image = AddFImgToFTable;
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

void DrawRuinUpPlate(DRAWCELL *cell)
{
    byte layer;
    int image;
    AddImage add_image = 0;
    byte tile = 5 + ((cell->info.modifier & 0x800) != 0);

    switch (cell->pass) {
    case 0:
        DrawRuinBackWall(cell);
        break;
    case 1:
        add_image = AddFImgToFTable;
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

void DrawRuinPosts(DRAWCELL *cell)
{
    AddImage add_image = 0;
    int image = cell->info.modifier & 3;

    switch (cell->pass) {
    case 0:
        DrawRuinBackWall(cell);
        break;
    case 1:
        add_image = AddFImgToFTable;
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
        DrawRuinFlaw(cell);
    }
}


void DrawRuinRubble(DRAWCELL *cell)
{
    DRAWCELL floor;
    word flags;

    if (cell->info.modifier & 0x40) {
        floor = *cell;
        floor.info.modifier = (flags = cell->info.modifier & ~0x40);
        if (flags & 4) {
            DrawRuinUpPlate(&floor);
            cell->info.modifier &= ~0x800;
        } else if (flags & 8) {
            DrawRuinDownPlate(&floor);
            cell->info.modifier &= ~0x800;
        } else {
            DrawRuinFloor(&floor);
        }
    } else if (cell->pass == 0) {
        DrawRuinBackWall(cell);
    }
    DrawRuinRubblePieces(cell);
    DrawRuinFlaw(cell);
}

void DrawRuinRubblePieces(DRAWCELL *cell)
{
    AddImage add_image = 0;
    int image = (cell->info.modifier & 0x30) >> 4;

    switch (cell->pass) {
    case 1:
        image += 33;
        add_image = AddFImgToFTable;
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
        DrawRuinFlaw(cell);
    }
}


void DrawRuinSpace(DRAWCELL *cell)
{
    int back_wall;

    if (cell->pass == 0) {
        back_wall = 0;
        if (cell->row == RowNo)
            back_wall = 1;
        else if (cell->row - RowNo == 1)
            back_wall = VisibleAbovePiece(cell->column, cell->row);
        if (!back_wall && RowNo == -1 && IsSpace(ObjId))
            back_wall = 1;
        if (back_wall)
            DrawRuinBackWall(cell);
    }
    DrawRuinFlaw(cell);
}

void LoadRuinsDrawCode(void)
{
    Pause = 0;
}


void RuinChangeCrevices(int reveal, sbyte room, sbyte column, sbyte row)
{
    sbyte stop;
    byte tile;

    stop = column + 3 >= 10 ? column + 3 : 10;
    column = column - 2 <= 0 ? column - 2 : 0;
    do {
        tile = ReadCell(room, column, row);
    } while (stop != ++column && tile != 7);
    if (tile == 7) {
        while (ReadCell(Cell.room, Cell.column - 1, Cell.row) == 7)
            ;
        ReadCell(Cell.room, Cell.column + 1, Cell.row);
        do
            RuinSetCreviceRevealed(reveal);
        while (ReadCell(Cell.room, Cell.column + 1, Cell.row) == 7);
    }
}

void RuinCheckCrevices(void)
{
    if (Char.sequence != 17 && GetCellsInFront(1) == 7 && (Cell.modifier & 3) != 3)
        RuinChangeCrevices(1, Char.room, Char.column, Char.row);
}

void RuinSetCreviceRevealed(int reveal)
{
    int changed;
    word flags;

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
        if (IsVisibleRoomCell(Cell.room, Cell.index))
            MarkCellRedraw();
        if (!(flags & 3) && Cell.column != 9) {
            ReadCell(Cell.room, Cell.column + 1, Cell.row);
            MarkCellRedraw();
            ReadCell(Cell.room, Cell.column - 1, Cell.row);
        }
        ((LEVELCELL *)Level.cell_info)[Cell.room * 30 + Cell.index - 30].modifier = flags;
    }
}


DRAWPROC DrawRuinFunc[] = {
    DrawRuinSpace, DrawRuinFloor, 0, DrawRuinPosts, DrawRuinGate,
    DrawRuinUpPlate, DrawRuinDownPlate, DrawRuinCrevice, DrawRuinPillarBot, DrawRuinPillarTop,
    DrawFlask, DrawRuinLoose, DrawScythe1, DrawScythe2, DrawRuinRubble,
    DrawRuinFloor, DrawRuinExit1, DrawRuinExit2, 0, 0,
    DrawRuinBlock, DrawRuinBones, DrawSword
};
