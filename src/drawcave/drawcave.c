/* DRAWCAVE.C, DOS overlay 6 frame 2a93. */
#include "types.h"
#include "graf.h"
#include "shape.h"
#include "level.h"
#include "heap.h"
#include "char.h"
#include "game.h"
#include "draw.h"
#include "movecave.h"
#include "add.h"
#include "bridge.h"
#include "custcvca.h"
#include "drawcave.h"
#include "fade.h"
#include "lgraf.h"
#include "misc.h"
#include "move.h"
#include "prince.h"
#include "tables.h"
#include "top.h"


typedef int (*AddImage)(byte, int, sbyte, sbyte, byte, byte);


static struct {
    int top_edge_x[7];
    IMAGEPOS bubble_positions[11];
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


void DrawCaveBlock(DRAWCELL *cell)
{
    word modifier;
    word image;
    byte group;

    modifier = cell->info.modifier;
    image = (modifier & 15) + 74;
    group = (modifier & 15) / 5;
    switch (cell->pass) {
    case 1:
        if ((image != 78 && image != 83) || cell->column != ColNo - 1)
            AddFImgToFTable(20, image, cell->column, cell->row, 10, 0);
        DrawCaveBotEdge(cell);
        if (modifier & 0x80)
            AddFImgToFTable(51, group + 88, cell->column, cell->row, 0, 0);
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
            if (image != (word)-1)
                AddBFImgToBTable(20, image, cell->column, cell->row, 10, 0);
        }
        if (modifier & 0x80)
            AddBImgToBTable(51, group + 90, cell->column, cell->row, 10, 0);
        break;
    }
}

void DrawCaveBotEdge(DRAWCELL *cell)
{
    int image;

    if (cell->row != 2 && cell->row == RowNo - 1 && cell->column == ColNo) {
        image = (cell->info.modifier >> 8) & 7;
        if (image)
            AddFImgToFTable(52, image + 7, cell->column, cell->row, 10, 0);
    }
}

void DrawCaveDart(DRAWCELL *cell)
{
    DRAWCELL floor;

    floor = *cell;
    floor.info.modifier &= 0x3f02;
    DrawCaveFloor(&floor);
    if (cell->pass == 1)
        AddFImgToFTable(36, cave_images.dart_images[cell->info.modifier & 15], cell->column, cell->row, 10, 0);
}

void DrawCaveExit1(DRAWCELL *cell)
{
    switch (cell->pass) {
    case 0:
        AddBImgToBTable(16, -2, cell->column, cell->row, 10, 0);
        break;
    case 1:
        AddFImgToFTable(16, -2, cell->column, cell->row, 0, 0);
        DrawCaveBotEdge(cell);
        DrawCaveTopEdge(cell);
        break;
    }
}

void DrawCaveExit2(DRAWCELL *cell)
{
    switch (cell->pass) {
    case 0:
        AddBImgToBTable(17, -2, cell->column, cell->row, 10, 0);
        DrawCaveExitMovable(cell);
        break;
    case 1:
        AddFImgToFTable(17, -2, cell->column, cell->row, 0, 0);
        DrawCaveBotEdge(cell);
        DrawCaveTopEdge(cell);
        break;
    }
}

void DrawCaveExit3(DRAWCELL *cell)
{
    Rect saved;

    if (cell->pass == 0) {
        if (cell->row != RowNo - 1)
            AddBImgToBTable(0, 1, cell->column, cell->row, 0, 0);
        saved = CellClipRect;
        CellClipRect.bottom = cell->row * 63 + 66;
        if (!EmptyRect(&CellClipRect))
            AddBImgToBTable(33, -2, cell->column, cell->row, 10, 0);
        CellClipRect = saved;
    } else if (cell->pass == 1) {
        DrawCaveTopEdge(cell);
    }
}

void DrawCaveExit4(DRAWCELL *cell)
{
    Rect saved;

    if (cell->pass == 0) {
        if (cell->row != RowNo - 1)
            AddBImgToBTable(0, 2, cell->column, cell->row, 0, 0);
        saved = CellClipRect;
        CellClipRect.bottom = cell->row * 63 + 66;
        if (!EmptyRect(&CellClipRect))
            AddBImgToBTable(35, -2, cell->column, cell->row, 10, 0);
        CellClipRect = saved;
    } else if (cell->pass == 1) {
        DrawCaveTopEdge(cell);
    }
}

void DrawCaveExitMovable(DRAWCELL *cell)
{
    Rect saved, doorway;
    AddImage draw;
    word lift;

    if (cell->pass == 0) {
        saved = CellClipRect;
        doorway = GamePlayRect;
        doorway.top = cell->row * 63 + 1;
        if (SectRect(&CellClipRect, &CellClipRect, &doorway)) {
            lift = cell->info.modifier & 255;
            Pieces[57].back.y = -(15 + (lift < 42 ? lift : 42));
            AddBImgToBTable(57, -2, cell->column, cell->row, 10, 0);
            draw = Kid.frame >= 217 && Kid.frame <= 226 ?
                AddBFImgToFTable : AddBFImgToBTable;
            draw(57, -2, cell->column, cell->row, 10, 0);
        }
        CellClipRect = saved;
    }
}

void DrawCaveFloor(DRAWCELL *cell)
{
    AddImage draw = 0;
    int image;
    byte mode;
    word variant = cell->info.modifier & 3;

    switch (cell->pass) {
    case 0:
        image = variant + 16;
        mode = 0;
        draw = AddBImgToBTable;
        break;
    case 1:
        image = variant + 25;
        mode = 0;
        draw = AddFImgToFTable;
        break;
    case 2:
        image = variant + 22;
        mode = 10;
        draw = AddBFImgToFTable;
        break;
    case 5:
        if (IsWall(PieceLeftId(cell->column, cell->row)))
            AddBImgToBTable(54, variant + 19, cell->column, cell->row, 10, 0);
        image = variant + 22;
        mode = 10;
        draw = AddBFImgToBTable;
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

void DrawCaveGate(DRAWCELL *cell)
{
    switch (cell->pass) {
    case 0:
        AddBImgToBTable(4, -2, cell->column, cell->row, 0, 0);
        if (IsWall(PieceLeftId(cell->column, cell->row)))
            AddBImgToBTable(54, 35, cell->column, cell->row, 10, 0);
        break;
    case 1:
        AddFImgToFTable(4, -2, cell->column, cell->row, 0, 0);
        DrawCaveBotEdge(cell);
        DrawCaveTopEdge(cell);
        break;
    case 2:
        AddBFImgToFTable(4, -2, cell->column, cell->row, 10, 0);
        break;
    case 5:
        AddBFImgToBTable(4, -2, cell->column, cell->row, 10, 0);
        break;
    }
    DrawCaveGateMovable(cell);
}

void DrawCaveGateMovable(DRAWCELL *cell)
{
    Rect saved, gate;
    AddImage draw;
    word lift;
    int y;

    saved = CellClipRect;
    gate = GamePlayRect;
    gate.top = cell->row * 63;
    if (cell->pass != 1)
        gate.top -= 16;
    if (SectRect(&CellClipRect, &CellClipRect, &gate)) {
        lift = (cell->info.modifier & 0xfc) >> 2;
        y = -3 - (lift < 50 ? lift : 50);
        switch (cell->pass) {
        default:
            draw = 0;
            break;
        case 1:
            Pieces[55].fore.y = y;
            draw = AddFImgToFTable;
            break;
        case 2:
            Pieces[55].back_fore.y = y;
            draw = AddBFImgToFTable;
            break;
        case 5:
            Pieces[55].back_fore.y = y;
            draw = AddBFImgToBTable;
            break;
        }
        if (draw)
            draw(55, -2, cell->column, cell->row, 10, 0);
    }
    CellClipRect = saved;
}

void DrawCaveGateSquishing(DRAWCELL *cell)
{
    Rect saved, gate;
    word lift;

    saved = CellClipRect;
    gate = GamePlayRect;
    gate.top = cell->row * 63 - 16;
    if (SectRect(&CellClipRect, (const Rect *)&Kid.bounds, &gate)) {
        lift = (cell->info.modifier & 0xfc) >> 2;
        Pieces[55].back_fore.y = -3 - (lift < 50 ? lift : 50);
        AddBFImgToFTable(55, -2, cell->column, cell->row, 10, 0);
    }
    CellClipRect = saved;
}

void DrawCaveLava1(DRAWCELL *cell)
{
    AddImage draw = 0;
    DRAWCELL floor;
    LAVAINFO *lava;

    floor = *cell;
    ((byte *)&floor.info.modifier)[0] = 0;
    lava = GetLavaInfoPtr(Room.number, cell->column, cell->row);
    switch (cell->pass) {
    case 0:
        DrawCaveFloor(&floor);
        break;
    case 1:
        DrawCaveFloor(&floor);
        break;
    case 2:
        DrawCaveFloor(&floor);
        if (lava && (!(lava->frame & 0x80) || (sbyte)(lava->frame & 0x3f) >= 3))
            draw = AddBFImgToFTable;
        break;
    case 5:
        DrawCaveFloor(&floor);
        if (!NewRoom && lava &&
            (!(lava->frame & 0x80) || (sbyte)(lava->frame & 0x3f) >= 3))
            draw = AddBFImgToBTable;
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

void DrawCaveLava2(DRAWCELL *cell)
{
    AddImage draw = 0;
    DRAWCELL floor;
    LAVAINFO *lava;

    floor = *cell;
    ((byte *)&floor.info.modifier)[0] = 0;
    ++floor.info.modifier;
    lava = GetLavaInfoPtr(Room.number, cell->column, cell->row);
    switch (cell->pass) {
    case 0:
        DrawCaveFloor(&floor);
        break;
    case 1:
        DrawCaveFloor(&floor);
        break;
    case 2:
        DrawCaveFloor(&floor);
        if (lava && (!(lava->frame & 0x80) || (sbyte)(lava->frame & 0x3f) >= 3))
            draw = AddBFImgToFTable;
        break;
    case 5:
        DrawCaveFloor(&floor);
        if (lava && (!(lava->frame & 0x80) || (sbyte)(lava->frame & 0x3f) >= 3))
            draw = AddBFImgToBTable;
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

void DrawCaveLavaBubbles(DRAWCELL *cell, int right)
{
    AddImage draw;
    LAVAINFO *lava;
    sbyte frame;
    int layer, index;

    if (NewRoom && !right)
        return;
    draw = cell->pass == 2 ? AddBFImgToFTable : AddBFImgToBTable;
    lava = GetLavaInfoPtr(Room.number, cell->column, cell->row);
    if (!lava)
        return;
    frame = lava->frame & 0x3f;
    if (lava->frame & 0x80) {
        draw(right + 23, frame + 146, cell->column, cell->row, 10, 0);
        if (frame < 3)
            return;
    }
    if ((lava->frame & 0x40) && frame >= 6 && frame <= 14) {
        Pieces[59].back_fore.x = frame + 20;
        Pieces[59].back_fore.y = -8;
        /* The DOS branch requires both comparisons before the special arm. */
        if (frame == 6 && frame == 14) {
            Pieces[59].back_fore.image = 152;
            if (frame == 6)
                Pieces[59].back_fore.x += 3;
        } else {
            Pieces[59].back_fore.image = 151;
        }
        if (right)
            Pieces[59].back_fore.x -= 32;
        draw(59, -2, cell->column, cell->row, 10, 0);
    }
    for (layer = 0; layer < 2; ++layer) {
        for (index = 0; index < 10; ++index) {
            LAVABUBBLE *bubble = &lava->bubbles[index];
            if (bubble->active && bubble->layer != layer) {
                sbyte picture;

                Pieces[59].back_fore.x = bubble->pos.x - (right << 5);
                Pieces[59].back_fore.y = bubble->pos.y;
                picture = BubbleScript[bubble->kind][bubble->frame];
                Pieces[59].back_fore.image = picture + 127;
                draw(59, -2, cell->column, cell->row, 10, 0);
            }
        }
        if (layer == 0) {
            draw(right + 23, 144, cell->column, cell->row, 10, 0);
        } else if ((lava->frame & 0x40) && frame >= 13 && frame <= 23) {
            Pieces[59].back_fore = cave_images.bubble_positions[frame - 13];
            if (right)
                Pieces[59].back_fore.x -= 32;
            draw(59, -2, cell->column, cell->row, 10, 0);
        }
    }
}

void DrawCaveLoose(DRAWCELL *cell)
{
    int broken = (cell->info.modifier & 0x80) >> 7;
    int image = cell->info.modifier & 15;

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
        Pieces[11].fore.y = image + 5;
        AddFImgToFTable(11, broken + 70, cell->column, cell->row, 10, 0);
        break;
    case 2:
        Pieces[11].back_fore.x = 16;
        Pieces[11].back_fore.y = -7;
        AddBFImgToFTable(11, broken + 68, cell->column, cell->row, 10, 0);
        if (image == 13)
            break;
        Pieces[11].back_fore.x = 1;
        Pieces[11].back_fore.y = image + 1;
        AddBFImgToFTable(11, broken + 66, cell->column, cell->row, 10, 0);
        break;
    case 5:
        Pieces[11].back_fore.x = 16;
        Pieces[11].back_fore.y = -7;
        AddBFImgToBTable(11, broken + 68, cell->column, cell->row, 10, 0);
        if (image == 13)
            break;
        Pieces[11].back_fore.x = 1;
        Pieces[11].back_fore.y = image + 1;
        AddBFImgToBTable(11, broken + 66, cell->column, cell->row, 10, 0);
        break;
    }
}

void DrawCavePillarBot(DRAWCELL *cell)
{
    DrawCaveFloor(cell);
    if (cell->pass == 1)
        AddFImgToFTable(8, -2, cell->column, cell->row, 10, 0);
}

void DrawCavePillarTop(DRAWCELL *cell)
{
    switch (cell->pass) {
    case 0:
        ((byte *)&cell->info.modifier)[0] &= 0xf8;
        DrawCaveSpace(cell);
        break;
    case 1:
        AddFImgToFTable(9, -2, cell->column, cell->row, 10, 0);
        DrawCaveTopEdge(cell);
        break;
    }
}

void DrawCavePosts(DRAWCELL *cell)
{
    switch (cell->pass) {
    case 0:
        AddBImgToBTable(3, -2, cell->column, cell->row, 10, 0);
        break;
    case 1:
        AddFImgToFTable(3, -2, cell->column, cell->row, 10, 0);
        DrawCaveBotEdge(cell);
        DrawCaveTopEdge(cell);
        break;
    case 2:
        AddBFImgToFTable(3, -2, cell->column, cell->row, 10, 0);
        break;
    case 5:
        AddBFImgToBTable(3, -2, cell->column, cell->row, 10, 0);
        break;
    }
}

void DrawCaveRevPlate(DRAWCELL *cell)
{
    AddImage draw = 0;

    switch (cell->pass) {
    case 0:
        AddBImgToBTable(34, -2, cell->column, cell->row, 0, 0);
        if (IsWall(PieceLeftId(cell->column, cell->row)))
            draw = AddBImgToBTable;
        break;
    case 1:
        AddFImgToFTable(34, -2, cell->column, cell->row, 0, 0);
        DrawCaveBotEdge(cell);
        break;
    case 2:
        AddBFImgToFTable(34, -2, cell->column, cell->row, 10, 0);
        if (cell->info.modifier & 0x0800)
            draw = AddBFImgToFTable;
        break;
    case 5:
        AddBFImgToBTable(34, -2, cell->column, cell->row, 10, 0);
        if (cell->info.modifier & 0x0800)
            draw = AddBFImgToBTable;
        break;
    }
    if (draw)
        draw(56, -2, cell->column, cell->row, 10, 0);
}

void DrawCaveRubble(DRAWCELL *cell)
{
    if (!(cell->info.modifier & 0x0800))
        DrawCaveFloor(cell);
    else
        DrawCaveRevPlate(cell);
    DrawCaveRubblePieces(cell);
}

void DrawCaveRubblePieces(DRAWCELL *cell)
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

void DrawCaveRubbleTorch(DRAWCELL *cell)
{
    DrawCaveTorch(cell);
    DrawCaveRubblePieces(cell);
}

void DrawCaveSpace(DRAWCELL *cell)
{
    volatile word modifier = cell->info.modifier;
    int draw_loose = 0;
    int draw_floor;
    DRAWCELL loose;

    if (cell->pass == 0 || cell->pass == 5) {
        modifier &= 7;
        if (cell->row == RowNo) { draw_floor = 1; }
        else if (cell->row == RowNo + 1 &&
                   (IsSpace(PieceAboveId(cell->column, cell->row)) ||
                    IsSpace(PieceAboveRightId(cell->column, cell->row)))) { draw_floor = 1; }
        else if (RowNo == -1 && IsSpace(ObjId)) { draw_floor = 1; }
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
            if ((!IsSpace(CellLeft.kind) || CellLeft.info.modifier >= 3) &&
                cell->column == ColNo)
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
            if ((!IsSpace(CellLeft.kind) || CellLeft.info.modifier >= 3) &&
                cell->column == ColNo)
                AddBImgToBTable(53, modifier + 4, cell->column, cell->row, 10, 0);
        } else {
            draw_loose = 1;
        }
        break;
    }
    if (draw_loose) {
        modifier = modifier == 4 ? 0x80 : 0;
        loose = *cell;
        loose.info.modifier = modifier | 13;
        DrawCaveLoose(&loose);
    }
}

void DrawCaveSpikes(DRAWCELL *cell)
{
    DRAWCELL block;

    block = *cell;
    ((byte *)&block.info.modifier)[0] &= 0xf0;
    ((byte *)&block.info.modifier)[0] |= 0x80;
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

void DrawCaveTopEdge(DRAWCELL *cell)
{
    int image;
    int far *height;

    if (cell->column == ColNo && cell->row == RowNo) {
        image = (cell->info.modifier & 0x3800) >> 11;
        if (image) {
            Pieces[61].fore.x = cave_images.top_edge_x[image - 1];
            image += 91;
            CheckBGSetImg(image);
            height = h2p(Pics[4]->items[image - 1].image);
            Pieces[61].fore.y = *height - 63;
            AddFImgToFTable(61, image, cell->column, cell->row, 10, 0);
        }
    }
}

void DrawCaveTorch(DRAWCELL *cell)
{
    DRAWCELL floor;
    Rect bounds;
    int enclosed;

    floor = *cell;
    ((byte *)&floor.info.modifier)[0] &= 0xfc;
    ComputeOffsetCellRect(&bounds, &CaveTorchRect, cell->column, cell->row);
    enclosed = EncloseRect(&bounds, &CellClipRect);
    switch (cell->pass) {
    case 0:
        if (!enclosed) {
            DrawCaveFloor(&floor);
            AddBImgToBTable(19, -2, cell->column, cell->row, 10, 0);
        }
        AddBImgToBTable(58, (byte)cell->info.modifier + 53, cell->column, cell->row, 10, 0);
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

int _fastcall IsRoundCaveBlock(word modifier)
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


DRAWPROC DrawCaveFunc[] = {
    DrawCaveSpace, DrawCaveFloor, DrawCaveSpikes, DrawCavePosts, DrawCaveGate,
    0, 0, 0, DrawCavePillarBot, DrawCavePillarTop,
    DrawFlask, DrawCaveLoose, 0, 0, DrawCaveRubble,
    0, DrawCaveExit1, DrawCaveExit2, DrawCarpet, DrawCaveTorch,
    DrawCaveBlock, 0, 0, DrawCaveLava1, DrawCaveLava2,
    0, 0, DrawCaveBars, 0, 0,
    0, 0, DrawCaveRubbleTorch, DrawCaveExit3, DrawCaveRevPlate,
    DrawCaveExit4, DrawCaveDart, 0, 0, 0,
    0, 0, 0, 0, DrawBrgFloor
};
