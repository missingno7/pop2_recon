#include <string.h>
#include <stdlib.h>
#include "types.h"
#include "graf.h"
#include "shape.h"
#include "level.h"
#include "auto.h"
#include "heap.h"
#include "act.h"
#include "char.h"
#include "game.h"
#include "draw.h"
#include "add.h"
#include "autohead.h"
#include "moveruin.h"
#include "collide.h"
#include "ctrl.h"
#include "fade.h"
#include "anim.h"
#include "load.h"
#include "move.h"
#include "movescyt.h"
#include "movetemp.h"
#include "prince.h"
#include "sound.h"
#include "tables.h"
#include "nis.h"
#include "top.h"
#include "util.h"
#include "movecave.h"
#define KIND_SAME_ROW (Room.number)
#define KIND_ROW_ABOVE (Room.left)
#define KIND_ROW_BELOW (Room.right)

struct collision_row {
    byte cell[10];
};

static byte cur_rooms[10], old_rooms[10], bottom_rooms[10], up_rooms[10];
byte ColDetLastFrame[10];
byte ColDetThisFrame[10];
static byte lower_edges[10], above_edges[10];
static sbyte cur_row, from_col, stop_col;
static sbyte old_row;
static sbyte left_barrier;
static sbyte right_block_col;


static int coll_x;


#define cell_flags (*(byte *)&Cell.modifier)


void CheckBarr(void)
{
    sbyte col;

    right_block_col = left_barrier = -1;
    if (Char.action != 7 && Char.sequence != 78 && Char.sequence != 70 &&
        (!Level.background_overlays[5] || !GetSlideRockOnRoomRow(Char.room, Char.row) || Char.sequence_group != 4)) {
        cur_row = Char.row;
        InitColDetBufs();
        old_row = cur_row;
        if (GetCharCol(CDRightEdge) + 4 <= 11)
            stop_col = GetCharCol(CDRightEdge) + 4;
        else
            stop_col = 11;
        from_col = GetCharCol(CDLeftEdge) - 2;
        GetColDetData(cur_row, cur_rooms, ColDetThisFrame);
        GetColDetData(cur_row + 1, bottom_rooms, lower_edges);
        GetColDetData(cur_row - 1, up_rooms, above_edges);
        if (Level.background_overlays[5]) {
            GetSlideRockDetData(cur_row, cur_rooms, ColDetThisFrame);
            GetSlideRockDetData(cur_row + 1, bottom_rooms, lower_edges);
            GetSlideRockDetData(cur_row - 1, up_rooms, above_edges);
        }
        for (col = 9; col >= 0; --col) {
            if (cur_rooms[col] != 255 && cur_rooms[col] == old_rooms[col]) {
                if (!(ColDetLastFrame[col] & 0x0f) && (ColDetThisFrame[col] & 0x0f))
                    right_block_col = col;
                if (!(ColDetLastFrame[col] & 0xf0) && (ColDetThisFrame[col] & 0xf0))
                    left_barrier = col;
            }
        }
    }

    if (Char.character_kind == 0 && right_block_col == -1 && left_barrier == -1 &&
        WeightlessTimer && Char.action == 4)
        CheckWeightlessBarr();
}

void CheckWeightlessBarr(void)
{
    if ((int)(63 * Char.row) - 4 > Char.y - ImHeight &&
        !IsSpace(ReadCell(Char.room, Char.column, Char.row - 1))) {
        Char.x = AddCharX(-4);
        Char.horizontal_step = 0;
        JumpSeq(229);
    }
}

void InitColDetBufs(void)
{
    struct collision_row *rooms, *edges;

    if (cur_row == old_row || (cur_row - old_row) == -3 || (cur_row - old_row) == 3) {
        rooms = (struct collision_row *)cur_rooms;
        edges = (struct collision_row *)ColDetThisFrame;
    } else if ((cur_row - old_row) == -1 || (cur_row - old_row) == 2) {
        rooms = (struct collision_row *)up_rooms;
        edges = (struct collision_row *)above_edges;
    } else {
        rooms = (struct collision_row *)bottom_rooms;
        edges = (struct collision_row *)lower_edges;
    }
    *(struct collision_row *)old_rooms = *rooms;
    *(struct collision_row *)ColDetLastFrame = *edges;
    memset(cur_rooms, 255, sizeof(cur_rooms));
    memset(up_rooms, 255, sizeof(up_rooms));
    memset(bottom_rooms, 255, sizeof(bottom_rooms));
}

int GetBarrCode(byte cell)
{
    int code;

    switch (cell) {
    case 2:
        if (cell_flags < 5 && InDoStepFwd) {
            code = 3;
        } else
            code = 4;
        break;
    case 4:
        if (Level.environment_kind == 3)
            code = 5;
        else if (KIND_SAME_ROW == 9 && Level.level_number == 8)
            code = 8;
        else
            code = 1;
        break;
    case 7:
        if (ImHeight <= 18) {
            code = 2;
        } else
            code = 4;
        break;
    case 12:
        code = 6;
        break;
    case 20:
    case 25:
        code = 4;
        break;
    default:
        code = 0;
        break;
    }
    return code;
}

void GetColDetData(sbyte row, byte *rooms, byte *edges)
{
    sbyte col;
    int left, right;
    byte flags;

    coll_x = ColEdgeX[from_col + 5] + 14;
    for (col = from_col; col <= stop_col; ++col) {
        left = GetLeftBarr(Char.room, col, row);
        right = GetRightBarr(Char.room, col, row);
        flags = CDRightEdge > left ? 0x0f : 0;
        if (right > CDLeftEdge)
            flags |= 0xf0;
        edges[Cell.column] = flags;
        rooms[Cell.column] = Cell.room;
        coll_x += 32;
    }
}

int GetLeftBarr(byte room, sbyte col, sbyte row)
{
    int code = GetBarrCode(ReadCell(room, col, row));
    int edge;

    if (code)
        edge = BarL[code] + coll_x;
    else
        edge = ColEdgeX[19];
    return edge;
}

int GetRightBarr(byte room, sbyte col, sbyte row)
{
    int code = GetBarrCode(ReadCell(room, col, row));
    int edge;

    if (code)
        edge = coll_x - BarR[code] + 31;
    else
        edge = 0;
    return edge;
}

void Collisions(void)
{
    if (Char.action != 2 && Char.action != 6 && (Char.frame < 135 || Char.frame >= 145)) {
        if (right_block_col >= 0)
            LeftCollide();
        else if (left_barrier >= 0)
            RightCollide();
    }
}

void RightCollide(void)
{
    int barrier;

    if ((Char.stance == 1 || Char.direction != 0) && CheckCollide1(left_barrier)) {
        barrier = GetRightBarr(Cell.room, Cell.column, Cell.row);
        if (barrier <= 0 && Level.background_overlays[5])
            barrier = GetSlideRockBarrDist(0, Cell.room, Cell.row);
        barrier -= CDLeftEdge;
        Collide(barrier, 0);
    }
}

void LeftCollide(void)
{
    int barrier;

    if ((Char.stance == 1 || Char.direction != -1) && CheckCollide1(right_block_col)) {
        barrier = GetLeftBarr(Cell.room, Cell.column, Cell.row);
        if (ColEdgeX[19] <= barrier && Level.background_overlays[5])
            barrier = GetSlideRockBarrDist(-1, Cell.room, Cell.row);
        barrier -= CDRightEdge;
        Collide(barrier, -1);
    }
}

int CheckCollide1(sbyte col)
{
    sbyte row = Char.row;

    if (row < 0)
        row += 3;
    if (row >= 3)
        row -= 3;
    return CheckCollide(ReadCell(cur_rooms[col], col, row));
}

int CheckCollide(byte cell)
{
    int blocked;

    switch (cell) {
    case 4:
        blocked = GateBarr();
        break;
    case 7:
        blocked = CreviceBarr();
        break;
    case 10:
        blocked = 0;
        break;
    case 12:
        blocked = Scythe1Barr();
        break;
    default:
        blocked = 1;
        break;
    }
    if (blocked && Char.character_kind == 11)
        blocked = 0;
    if (blocked)
        coll_x = AdjustRoom(Cell.room, ColEdgeX[Cell.column + 5]) + 14;
    return blocked;
}

int AdjustRoom(byte kind, int value)
{
    /* The first matching kind leaves the signed offset unchanged. */
    if (KIND_SAME_ROW == kind) {
        /* unchanged */
    } else if (KIND_ROW_ABOVE == kind || Room.below_left == kind) {
        value -= 320;
    } else if (KIND_ROW_BELOW == kind || Room.below_right == kind) {
        value += 320;
    }
    return value;
}

void Collide(int displacement, sbyte direction)
{
    byte cell;

    if (Char.alive >= 0 || Char.frame == 242 || Char.frame == 243)
        return;
    Char.x += displacement;
    cell = Cell.kind;
    if (direction == -1) {
        if (IsWall(cell))
            cell = ReadCell(Cell.room, --Cell.column, Cell.row);
    } else if (IsWall(cell)) {
        ++Cell.column;
        if (Cell.room == 0 && Cell.column == 10) {
            Cell.room = Char.room;
            Cell.column = 0;
        }
        cell = ReadCell(Cell.room, Cell.column, Cell.row);
    }
    if (IsSpace(cell) && (Level.environment_kind != 2 || !SlideRockInCell(8)))
        AirBump();
    else
        GroundBump(direction);
}

void AirBump(void)
{
    Char.x = AddCharX(-8);
    if (Char.action == 4)
        Char.horizontal_step = 0;
    else {
        JumpSeq(45);
        AnimChar();
    }
    BumpSound();
    RereadCells();
}

void GroundBump(sbyte direction)
{
    int sequence;
    int boundary;
    word frame;

    if (Char.stance != 1 && 63 * Char.row - Char.y + 56 >= 15) {
        AirBump();
        return;
    }
    ResetCharY();
    if (Char.vertical_step >= 22) {
        Char.x = AddCharX(-10), RereadCells();
        return;
    }
    Char.vertical_step = 0;
    if (Char.alive == 0)
        return;
    if (Char.stance == 1) {
        boundary = GetSlideRockX(Char.room, Char.row);
        if (direction == Char.direction) {
            if (Char.sequence_id == 55 && Char.frame == 207)
                Char.x = AddCharX(10);
            Char.x = AddCharX(2), sequence = 65;
        } else {
            sequence = 64;
            if (IsStrikingPos(Char.character_kind, Char.frame)) {
                if (Cell.kind == 4 && cell_flags > 64)
                    sequence = -1;
                else
                    Char.x = AddCharX(16);
            }
        }
        if (boundary != 0 && boundary > Char.x) {
            sequence = 93, Char.stance = 0;
        }
    } else {
        frame = Char.frame;
        if (frame >= 246 && frame <= 261) {
            Char.x = AddCharX(-3), sequence = 121, Char.repeat = 0;
        } else if (frame == 24 || frame == 25 ||
                   (frame >= 40 && frame < 43) || (frame >= 102 && frame < 107)) {
            sequence = 46;
        } else {
            sequence = 47;
        }
    }
    if (sequence != -1) {
        JumpSeq(sequence);
        AnimChar();
        RereadCells();
    }
    if (sequence != 64)
        BumpSound();
    else if (++sequence != 0 && Char.sequence != 56)
        AddSound(12);
}

void BumpSound(void)
{
    if (Char.character_kind != 1)
        AlertGuard = 1;
}

void InitCollisionDetection(void)
{
    memset(old_rooms, 255, sizeof(old_rooms));
    memset(cur_rooms, 255, sizeof(cur_rooms));
    memset(bottom_rooms, 255, sizeof(bottom_rooms));
    memset(up_rooms, 255, sizeof(up_rooms));
    old_row = -1;
}

int CreviceBarr(void)
{
    if (Char.character_kind == 7 || Char.character_kind == 8 || ImHeight > 18 || (cell_flags & 3) == 3)
        return 1;
    return 0;
}

int GateBarr(void)
{
    int height, clearance;
    int result;

    if (Level.environment_kind == 1) {
        if ((sbyte)(Cell.modifier & 31) < 19)
            result = 1;
        else
            result = 0;
    } else if (Char.character_kind == 1) {
        result = 0;
    } else {
        height = Char.bounds.bottom - Char.bounds.top;
        if (Char.character_kind == 7)
            height += 26;
        else if (Char.character_kind == 8)
            height += 19;
        clearance = ((Cell.modifier & 252) >> 2) + 6;
        result = height + 7 <= clearance ? 0 : 1;
    }
    return result;
}


int GateBarrToStandUp(void)
{
    int position = 0;
    word opening;

    if ((Char.room != 9 || Level.level_number != 8) &&
        (ReadCell(Char.room, Char.column, Char.row) == 4 ||
         GetCellsInFront(1) == 4 || GetBehind() == 4)) {
        opening = Cell.modifier & 255;
        position = AddCharX(-6) - 14;
        if (opening != 0 && opening < 200 && ColEdgeX[Cell.column + 5] + 12 <= position &&
            ColEdgeX[Cell.column + 1 + 5] + 4 >= position)
            position = 1;
        else
            position = 0;
    }
done:
    return position;
}


int Scythe1Barr(void)
{
    int blocked;

    if (Char.character_kind != 1 && Char.character_kind != 7 && Char.character_kind != 8 && Char.character_kind != 11 &&
        ImHeight > 25 && ScytheBladeExtended(Cell.modifier)) {
        blocked = 1;
    } else {
        blocked = 0;
    }
    return blocked;
}


void GetCharEdges(void)
{
    Rect previous, bounds;
    SHAPESIZE size;
    int sword_width;

    previous = Char.bounds;
    Char.bounds = NullRect;
    if (FChar.image == -1 || !GetSetImgSize(Pics[FChar.image_set], FChar.image, &size))
        return;
    ImWidth = size.width + 1;
    ImHeight = size.height;
    sword_width = GetSwordWidth();
    LeftEdge = FChar.x + 130;
    LeftEdge -= Char.direction == 0 ? ImWidth : sword_width;
    CDLeftEdge = LeftEdge;
    ImWidth += sword_width;
    CDRightEdge = RightEdge = LeftEdge + ImWidth;
    TopEdge = FChar.y - ImHeight + 1;
    if (TopEdge >= 192)
        TopEdge = 0;
    bounds.top = TopEdge;
    bounds.bottom = FChar.y + 1;
    if (Char.direction == -1) {
        bounds.left = FChar.x;
        bounds.right = FChar.x + size.width;
    } else {
        bounds.left = FChar.x - size.width;
        bounds.right = FChar.x;
    }
    Char.bounds = bounds;
    Cell.top_row = GetRow(TopEdge);
    Cell.bottom_row = GetRow(FChar.y);
    if (Cell.bottom_row == 255)
        Cell.bottom_row = 3;
    if (GetCol(LeftEdge) < 0)
        Cell.first_col = 0;
    else
        Cell.first_col = GetCol(LeftEdge);
    if (GetCol(RightEdge) <= 9)
        Cell.last_col = GetCol(RightEdge);
    else
        Cell.last_col = 9;
    if (Frame.flags & 32) {
        CDLeftEdge += 9;
        CDRightEdge -= 9;
    }
}

int GetSwordWidth(void)
{
    int width = 0;
    ATTACHFRAME far *frames;

    if (Char.stance == 1 && Char.character_kind != 7 && Char.character_kind != 8) {
        GetFrameInfo();
        if (Frame.equipment_image != width) {
            frames = h2p(SwordFrames);
            width = frames[Frame.equipment_image].x;
            width = width - 2 < 0 ? 0 : width - 2;
        }
    }
    return width;
}


sbyte StepTo = 0;


int GetFwdDist(void)
{
    byte cell;
    int distance;

    GetBaseCell();
    SetupChar();
    GetCharEdges();
    cell = GetUnderFt();
    if (GetBarrCode(cell)) {
        distance = GetBarrDistFromFace(cell, Char.room, Char.column);
        if (distance >= 0)
            goto barrier;
    }
    if (cell == 24)
        goto facing_left;
    cell = GetCellsInFront(1);
    if (GetBarrCode(cell)) {
        distance = GetBarrDistFromFace(cell, Char.room, Char.column + Plus1[Char.direction + 1]);
        if (distance >= 0)
            goto barrier;
    }
    if ((IsSpace(cell) || IsImpassable(cell) || cell == 30) && cell != 24) {
        StepTo = 0;
        distance = GetDist();
        goto done;
    }
    if (cell == 6 || cell == 22 || cell == 2 || cell == 34)
        goto partial;
    if (cell != 24)
        goto fallback;
    goto facing_left;
partial:
    if ((distance = GetDist()) == 0)
        goto fallback;
    --distance;
    StepTo = 0;
    goto done;
facing_left:
    if (Char.direction == -1) {
        distance = GetDist();
        if (Char.column == Cell.column)
            distance -= 15;
        else
            distance += 17;
        goto done;
    }
    goto fallback;
barrier:
    if (distance < 32) {
        StepTo = 1;
        goto done;
    }
fallback:
    StepTo = 2;
    distance = 28;
done:
    return distance;
}


void CheckGateForOpp(void)
{
    int left, right;

    byte cell = GetUnderFt();

    if (Char.alive >= 0)
        return;
    if (cell != 4 && GetBehind() != 4 && GetCellsInFront(1) != 4)
        return;
    if ((Cell.room == 9 && Level.level_number == 8) || Char.character_kind == 12 || !GateBarr())
        return;
    coll_x = ColEdgeX[Cell.column + 5] + 14;
    left = GetLeftBarr(Cell.room, Cell.column, Cell.row);
    right = GetRightBarr(Cell.room, Cell.column, Cell.row);
    if (Room.number != Cell.room) {
        if (Room.left == Cell.room) {
            left -= 320;
            right -= 320;
        } else if (Room.right == Cell.room) {
            left += 320;
            right += 320;
        } else {
            left = 999;
            right = 0;
        }
    }
    if (left < CDRightEdge && right > CDLeftEdge)
        PushCharAwayFromGate();
}

void CheckGateForKid(void)
{
    byte cell = GetUnderFt();

    if (Char.alive >= 0)
        return;
    if (Char.sequence == 118 || (cell != 4 && GetBehind() != 4 && GetCellsInFront(1) != 4))
        return;
    if (Cell.room == 9 && Level.level_number == 8)
        return;
    if ((ColDetThisFrame[Cell.column] & ColDetLastFrame[Cell.column]) != 255 || !GateBarr())
        return;
    if (Char.frame != 109 && Cell.modifier != 0 && Char.stance != 1) {
        if (Char.sequence != 50)
            JumpSeq(50);
    } else if (Level.environment_kind == 3 && Cell.modifier != 0 && Char.stance != 1) {
        SquishChar();
    } else {
        BumpSound();
        PushCharAwayFromGate();
    }
}

void PushCharAwayFromGate(void)
{
    int face, back;

    if (GetBarrDistFromFace(4, Cell.room, Cell.column) >= 0)
        face = GetBarrDistFromFace(4, Cell.room, Cell.column);
    else
        face = -GetBarrDistFromFace(4, Cell.room, Cell.column);
    if (GetBarrDistFromBack(4, Cell.room, Cell.column) >= 0)
        back = GetBarrDistFromBack(4, Cell.room, Cell.column);
    else
        back = -GetBarrDistFromBack(4, Cell.room, Cell.column);
    Char.x = AddCharX(back > face ? -face : back);
}


void EnemyCollide(void)
{
    int back;
    int airborne, distance, front, sequence;
    byte cell;

    airborne = Char.character_kind == 7 || Char.character_kind == 8;
    if (Char.action != ((Char.character_kind == 7 || Char.character_kind == 8) ? 0 : 1) &&
        ((Char.character_kind != 7 && Char.character_kind != 8) || Char.sequence == 164))
        return;
    if ((Char.alive >= 0 && !airborne) || Char.stance != 1 || Char.sequence == 70)
        return;
    cell = GetUnderFt();
    if (!IsWall(cell) && (cell != 4 || !GateBarr())) {
        cell = GetCellsInFront(1);
        if (!IsWall(cell) && (cell != 4 || !GateBarr()))
            cell = 0;
    }
    if (!cell)
        return;
    SetupChar();
    GetCharEdges();
    if (!CheckCollide(cell))
        return;
    sequence = -1;
    back = GetBarrDistFromBack(Cell.kind, Cell.room, Cell.column);
    front = GetBarrDistFromFace(Cell.kind, Cell.room, Cell.column);
    distance = abs(front) < abs(back) ? front : back;
    if (airborne)
        sequence = HeadEnemyCollide(front);
    if (sequence == -1 && distance < 5 && -ImWidth <= distance) {
        if (cell == 4 && distance > 0)
            distance = -distance;
        Char.x = AddCharX(-distance);
        if (distance >= 0 && cell == 4 && !airborne)
            sequence = 64;
        else if (!airborne || (Char.sequence != 145 && Char.sequence != 164 && Char.sequence_group != 1))
            sequence = (Char.character_kind == 7 || Char.character_kind == 8) ? 158 : 65;
    }
    if (sequence != -1) {
        JumpSeq(sequence);
        AnimChar();
        RereadCells();
    }
}

int GetBarrDistFromFace(byte cell, byte room, sbyte col)
{
    int code, distance;

    if (cell == 4 && !GateBarr())
        distance = -1;
    else {
        coll_x = AdjustRoom(room, ColEdgeX[col + 5] + 14);
        code = GetBarrCode(cell);
        if (!code)
            distance = -1;
        else if (Char.direction != 0)
            distance = BarR[code] - coll_x + CDLeftEdge - 31;
        else
            distance = BarL[code] - CDRightEdge + coll_x;
    }
    return distance;
}

int GetBarrDistFromBack(byte cell, byte room, sbyte col)
{
    int code, distance;

    coll_x = AdjustRoom(room, ColEdgeX[col + 5] + 14);
    code = GetBarrCode(cell);
    if (!code)
        distance = 99;
    else if (Char.direction >= 0)
        distance = BarR[code] - coll_x + CDLeftEdge - 31;
    else
        distance = BarL[code] - CDRightEdge + coll_x;
    return distance;
}


void reset_barrier_state(void)
{
    Pause = 0;
}
