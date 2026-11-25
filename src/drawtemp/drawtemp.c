/* DRAWTEMP.C, DOS overlay 9 frame 2b4b. */
#include "types.h"
#include "graf.h"
#include "shape.h"
#include "level.h"
#include "char.h"
#include "game.h"
#include "draw.h"
#include "add.h"
#include "altar.h"
#include "ctrl.h"
#include "drawscyt.h"
#include "drawtemp.h"
#include "fade.h"
#include "lgraf.h"
#include "misc.h"
#include "move.h"
#include "prince.h"
#include "tables.h"
#include "top.h"


typedef int (*AddImage)(byte, int, sbyte, sbyte, byte, byte);


static int loose_odd[12] = { 0, 1, 0, 2, 2, 0, 0, 0, 1, 1, 1, 0 };
static int loose_even[12] = { 0, 2, 2, 0, 1, 1, 0, 2, 2, 2, 0, 1 };


void DrawTempBackWall(DRAWCELL *cell)
{
    int image;
    byte modifier;
    int paired, fallback;
    BFITEM far *shape;
    Rect overlap;
    int saved_top;

    modifier = (cell->info.background & 0x0f00) >> 8;
    image = cell->info.background & 31;
    if (image) {
        if (image == 20) {
            image = 19;
            paired = 1;
        } else {
            paired = 0;
        }

        if (!NewRoom) {
            shape = GetBackTableImgRow(image, cell->column - 1, cell->row);
            if (shape && SectRect(&overlap, &shape->clip, &CellClipRect)) {
                fallback = 0;
                UnionRect(&shape->clip, &shape->clip, &CellClipRect);
                if (paired) {
                    shape = GetBackTableImgRow(image + 1, cell->column - 1, cell->row);
                    if (shape) {
                        if (SectRect(&overlap, &shape->clip, &CellClipRect))
                            UnionRect(&shape->clip, &shape->clip, &CellClipRect);
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
            if (cell->row >= 0 && !VisibleAbovePiece(cell->column, cell->row)) {
                if (Row2PixelY[cell->row] > CellClipRect.left) {
                    CellClipRect.top = Row2PixelY[cell->row] < saved_top ? saved_top : Row2PixelY[cell->row];
                    if (EmptyRect(&CellClipRect))
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

void DrawTempBlock(DRAWCELL *cell)
{
    int image;
    byte left;
    AddImage add_image;

    switch (cell->pass) {
    case 0:
        if (NewRoom)
            DrawTempBackWall(cell);
        break;
    case 1:
        image = (cell->info.modifier & 7) + 24;
        if (image > 25) {
            if (!NewRoom) {
                Pieces[20].fore.x = -32;
                image -= 2;
            } else {
                image = -1;
            }
        } else {
            Pieces[20].fore.x = 0;
        }
        if (image != -1)
            AddFImgToFTable(20, image, cell->column, cell->row, 10, 0);
        image = (cell->info.modifier & 7) + 24;
        left = PieceLeftId(cell->column, cell->row);
        if (image <= 25 && IsWall(left))
            AddFImgToFTable(20, 61, cell->column, cell->row, 0, 0);
        if (IsWall(PieceBelowId(cell->column, cell->row))) {
            image = IsWall(left) ? 63 : 62;
            AddFImgToFTable(20, image, cell->column, cell->row, 0, 0);
        }
        break;
    case 2:
    case 5:
        image = (cell->info.modifier & 7) + 24;
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

void DrawTempExit1(DRAWCELL *cell)
{
    byte layer;
    AddImage add_image = 0;
    switch (cell->pass) {
    case 0:
        if (NewRoom)
            DrawTempBackWall(cell);
        break;
    case 1:
        add_image = AddFImgToFTable;
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

void DrawTempExit2(DRAWCELL *cell)
{
    byte layer;
    AddImage add_image = 0;
    switch (cell->pass) {
    case 0:
        if (NewRoom) {
            DrawTempBackWall(cell);
            DrawTempExitMovable(cell);
        }
        break;
    case 1:
        add_image = AddFImgToFTable;
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
            DrawTempExitMovable(cell);
    }
}

void DrawTempExitMovable(DRAWCELL *cell)
{
    AddImage add_image;
    Rect door_clip, saved_clip;
    TROBREC *moving;
    if (cell->pass == 5) {
        saved_clip = CellClipRect;
        door_clip = GamePlayRect;
        door_clip.top = cell->row * 63 - 5;
        if (SectRect(&CellClipRect, &CellClipRect, &door_clip)) {
            moving = GetTrobRoomCell(Room.number, FirstInRow(cell->row) + cell->column);
            if (moving && moving->state >= 4)
                AddBImgToBTable(17, -2, cell->column, cell->row, 0, 0);
            Pieces[57].back.y = -(19 + ((cell->info.modifier & 255) < 42 ? (cell->info.modifier & 255) : 42));
            AddBImgToBTable(57, -2, cell->column, cell->row, 10, 0);
            add_image = Kid.frame >= 217 && Kid.frame <= 226 ? AddBFImgToFTable : AddBFImgToBTable;
            add_image(57, -2, cell->column, cell->row, 10, 0);
        }
        CellClipRect = saved_clip;
    }
}

void DrawTempFloor(DRAWCELL *cell)
{
    byte layer;
    int detail = 0;
    AddImage add_image = 0;
    switch (cell->pass) {
    case 0:
        DrawTempBackWall(cell);
        break;
    case 1:
        add_image = AddFImgToFTable;
        layer = 0;
        break;
    case 2:
        add_image = AddBFImgToFTable;
        layer = 10;
        detail = cell->info.modifier & 3;
        break;
    case 5:
        add_image = AddBFImgToBTable;
        layer = 10;
        detail = cell->info.modifier & 3;
        break;
    }
    if (add_image) {
        add_image(1, -2, cell->column, cell->row, layer, 0);
        if (detail)
            DrawTempFloorPillarBase(cell);
    }
}

void DrawTempFloorPillarBase(DRAWCELL *cell)
{
    IMAGEPOS saved;
    AddImage add_image;

    if (cell->pass == 5)
        add_image = AddBFImgToBTable;
    else if (cell->pass == 2)
        add_image = AddBFImgToFTable;
    else
        add_image = 0;
    if (add_image) {
        saved = Pieces[1].back_fore;
        Pieces[1].back_fore = Pieces[1].back;
        add_image(1, -2, cell->column, cell->row, 0, 0);
        Pieces[1].back_fore = saved;
    }
}

void DrawTempGate(DRAWCELL *cell)
{
    AddImage draw = 0;
    switch (cell->pass) {
    case 0:
        DrawTempBackWall(cell);
        break;
    case 1:
        draw = AddFImgToFTable;
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
        DrawTempGateMovable(cell);
    }
}

void DrawTempGateMovable(DRAWCELL *cell)
{
    AddImage draw;
    Rect gate_clip, saved_clip;
    word height;

    saved_clip = CellClipRect;
    gate_clip = GamePlayRect;
    gate_clip.top = cell->row * 63 - 10;
    if (SectRect(&CellClipRect, &CellClipRect, &gate_clip)) {
        height = (cell->info.modifier & 252) >> 2;
        Pieces[55].back_fore.y = -(4 + (height < 50 ? height : 50));
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

void DrawTempLoose(DRAWCELL *cell)
{
    int image;
    DRAWCELL floor_cell;
    image = cell->info.modifier & 15;
    if (image != 13)
        image = cell->column % 2 ? loose_odd[image] : loose_even[image];
    if (!image) {
        floor_cell = *cell;
        floor_cell.info.modifier &= ~3;
    }
    switch (cell->pass) {
    case 0:
        DrawTempBackWall(cell);
        break;
    case 1:
        if (image == 13)
            break;
        if (image)
            AddFImgToFTable(11, image + 35, cell->column, cell->row, 10, 0);
        else
            DrawTempFloor(&floor_cell);
        break;
    case 2:
        if (image == 13)
            break;
        if (image)
            AddBFImgToFTable(11, image + 33, cell->column, cell->row, 10, 0);
        else
            DrawTempFloor(&floor_cell);
        break;
    case 5:
        if (image == 13)
            break;
        if (image)
            AddBFImgToBTable(11, image + 33, cell->column, cell->row, 10, 0);
        else
            DrawTempFloor(&floor_cell);
        break;
    }
}

void DrawTempPillarBot(DRAWCELL *cell)
{
    AddImage draw = 0;
    switch (cell->pass) {
    case 0:
        DrawTempBackWall(cell);
        break;
    case 1:
        draw = AddFImgToFTable;
        break;
    case 5:
        draw = AddBImgToBTable;
        break;
    }
    if (draw)
        draw(8, -2, cell->column, cell->row, 10, 0);
}

void DrawTempPillarTop(DRAWCELL *cell)
{
    AddImage draw = 0;
    switch (cell->pass) {
    case 0:
        DrawTempBackWall(cell);
        break;
    case 1:
        draw = AddFImgToFTable;
        break;
    case 5:
        draw = AddBImgToBTable;
        break;
    }
    if (draw)
        draw(9, -2, cell->column, cell->row, 10, 0);
}

void DrawTempDownPlate(DRAWCELL *cell)
{
    byte tile;
    int image;
    int pressed = cell->info.modifier & 0x0800;
    AddImage draw = 0;

    switch (cell->pass) {
    case 0:
        DrawTempBackWall(cell);
        break;
    case 1:
        tile = pressed ? 6 : 5;
        image = 33;
        draw = AddFImgToFTable;
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

void DrawTempUpPlate(DRAWCELL *cell)
{
    int image;
    byte tile = 5 + ((cell->info.modifier & 0x0800) != 0);
    AddImage draw = 0;

    switch (cell->pass) {
    case 0:
        DrawTempBackWall(cell);
        break;
    case 1:
        draw = AddFImgToFTable;
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

void DrawTempPosts(DRAWCELL *cell)
{
    AddImage draw = 0;
    int image = cell->info.modifier & 3;
    int saved_y;

    switch (cell->pass) {
    case 0:
        DrawTempBackWall(cell);
        break;
    case 1:
        if (!image) { draw = AddFImgToFTable; image = 44; }
        else if (!NewRoom || image == 1) {
            draw = AddFImgToFTable;
            Pieces[3].fore.x = 0;
            if (image == 2)
                Pieces[3].fore.x -= 32;
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
            if (Pieces[3].fore.x == 0 && PieceLeftId(cell->column, cell->row) != 3) {
                saved_y = Pieces[20].fore.y;
                Pieces[20].fore.y -= 54;
                AddFImgToFTable(20, 38, cell->column, cell->row, 10, 0);
                Pieces[20].fore.y = saved_y;
            }
            Pieces[3].fore.x = 0;
        }
    }
}

void DrawTempRubble(DRAWCELL *cell)
{
    if (cell->pass == 0)
        DrawTempBackWall(cell);
    else if ((cell->pass == 5 || cell->pass == 2) && (cell->info.modifier & 3))
        DrawTempFloorPillarBase(cell);
    DrawTempRubblePieces(cell);
}

void DrawTempRubblePieces(DRAWCELL *cell)
{
    AddImage draw;
    switch (cell->pass) {
    case 1:
        draw = AddFImgToFTable;
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

void DrawTempSpace(DRAWCELL *cell)
{
    int draw;
    if (cell->pass == 0) {
        draw = 0;
        if (cell->row == RowNo)
            draw = 1;
        else if (cell->row - RowNo == 1)
            draw = VisibleAbovePiece(cell->column, cell->row);
        if (!draw && RowNo == -1 && IsSpace(ObjId))
            draw = 1;
        if (draw)
            DrawTempBackWall(cell);
    }
}

void DrawTempSpikes(DRAWCELL *cell)
{
    DRAWCELL block;
    switch (cell->pass) {
    case 1:
        block = *cell;
        block.info.modifier &= ~7;
        DrawTempBlock(&block);
        break;
    case 5:
        if (cell->column != 9) {
            block = *cell;
            block.info.modifier &= ~7;
            DrawTempBlock(&block);
            AddBImgToBTable(2, -2, cell->column, cell->row, 0, 0);
        }
        break;
    }
}

void DrawTempRubbleTorch(DRAWCELL *cell)
{
    DrawTempTorch(cell);
    DrawTempRubblePieces(cell);
}

void DrawTempTorch(DRAWCELL *cell)
{
    int frame = (byte)cell->info.modifier;
    DRAWCELL floor_cell = *cell;
    Rect torch_clip;
    floor_cell.info.background = floor_cell.info.modifier = 0;
    switch (cell->pass) {
    case 0:
        ComputeOffsetCellRect(&torch_clip, &TempTorchRect, cell->column, cell->row);
        if (!EncloseRect(&torch_clip, &CellClipRect)) {
            DrawTempFloor(&floor_cell);
            AddBImgToBTable(19, -2, cell->column, cell->row, 0, 0);
        }
        break;
    case 1:
    case 2:
        DrawTempFloor(&floor_cell);
        break;
    case 5:
        DrawTempFloor(&floor_cell);
        AddBFImgToBTable(19, frame + 51, cell->column, cell->row, 0, 0);
        break;
    }
}

void DrawTempTrapDoor(DRAWCELL *cell)
{
    int state = cell->info.modifier & 31;
    switch (cell->pass) {
    case 0:
        if (state)
            DrawTempBackWall(cell);
        else
            DrawTempFloor(cell);
        break;
    case 1:
    case 2:
    case 5:
        if (!state)
            DrawTempFloor(cell);
        break;
    }
}

void LoadTempleDrawCode(void)
{
    Pause = 0;
}


DRAWPROC DrawTempFunc[] = {
    DrawTempSpace, DrawTempFloor, DrawTempSpikes, DrawTempPosts, DrawTempGate,
    DrawTempUpPlate, DrawTempDownPlate, 0, DrawTempPillarBot, DrawTempPillarTop,
    DrawFlask, DrawTempLoose, DrawScythe1, DrawScythe2, DrawTempRubble,
    0, DrawTempExit1, DrawTempExit2, 0, DrawTempTorch,
    DrawTempBlock, 0, 0, 0, 0,
    DrawTempBlock, DrawTempTrapDoor, 0, 0, 0,
    0, 0, DrawTempRubbleTorch, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, DrawTempSacredFlame
};
