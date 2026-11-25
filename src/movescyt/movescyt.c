/* MOVESCYT.C, DOS scythe movement and collision. */
#include "types.h"
#include "graf.h"
#include "shape.h"
#include "level.h"
#include "char.h"
#include "game.h"
#include "draw.h"
#include "act.h"
#include "ctrl.h"
#include "fade.h"
#include "move.h"
#include "movescyt.h"
#include "prince.h"
#include "sound.h"
#include "tables.h"
#include "top.h"


static Rect redraw_bounds = {26, -24, 43, 52};
static Rect first_blade = {26, 20, 33, 57};
static Rect second_blade = {26, 46, 33, 89};
int ScytheImg[20] = {
    0, 100, 97, 98, 97, 98, 97, 98, 97, 97,
    97, 97, 97, 97, 97, 97, 99, 100, 101, 0
};


void AnimScythe(void)
{
    int frame = BlueSpec.modifier & 31;
    if (frame < 19) {
        ++BlueSpec.modifier;
        if (ScytheImg[frame] != ScytheImg[frame + 1])
            RedrawScythe(0);
        if (frame == 0)
            AddSound(84);
        else if (frame == 15)
            AddSound(83);
        else if (frame >= 16 && (BlueSpec.modifier & 0x40) && !(BlueSpec.modifier & 0x80)) {
            BlueSpec.modifier |= 0x80;
            RedrawScythe(1);
        }
    } else {
        BlueSpec.modifier &= ~31;
        Trob.state = -1;
    }
    ReadCell(Trob.room, Trob.cell % 10 - 1, Trob.cell / 10);
    *(byte near *)&Cell.modifier = 0;
    Level.cell_info[Cell.room - 1][Cell.index].modifier =
        (Cell.modifier |= (byte)BlueSpec.modifier);
}

void CheckScythe(void)
{
    if (Char.character_kind != 7 && Char.character_kind != 8 && Char.character_kind != 11 &&
        Char.alive < 0 && Char.row * 63 + 40 <= Char.y) {
        sbyte kind = ReadCell(Char.room, Char.column, Char.row);
        if ((kind == 12 || kind == 13) && ColEdgeX[Char.column + 5] + 18 < Char.x)
            TriggerScythe(Cell.room, Cell.index);
    }
}

void CheckScytheHit(void)
{
    int done = 0;
    sbyte column, kind;
    Rect bounds;
    if (Char.character_kind != 0 && Char.character_kind != 2)
        return;
    if (Char.sequence == 44)
        return;
    column = Char.column - 1;
    while (column <= Char.column + 1) {
        kind = ReadCell(Char.room, column, Char.row);
        if (kind == 12 || kind == 13) {
            if (GetScytheHitType(Cell.modifier) &&
                ComputeBladeHitRect(&bounds) &&
                SectRect(&bounds, &bounds, (const Rect far *)&Char.bounds) &&
                (Char.frame < 267 || Char.frame > 271)) {
                if (Char.character_kind == 0) {
                    JumpSeq(113);
                    SetKidDeathMethod(5);
                } else {
                    JumpSeq(190);
                    Char.alive = 1;
                }
                AnimChar();
                DecLifePts(100);
                AddSound(85);
                BlueS[Cell.index].modifier = (Cell.modifier |= 0x40);
                if (kind == 12)
                    ReadCell(Char.room, column + 1, Char.row);
                else
                    ReadCell(Char.room, column - 1, Char.row);
                BlueS[Cell.index].modifier = (Cell.modifier |= 0x40);
            }
            done = 1;
        }
        ++column;
        if (done)
            break;
    }
}

int ComputeBladeHitRect(Rect *bounds)
{
    int valid = 1;
    int phase = Cell.modifier & 31;
    if (phase == 2)
        UnionRect(bounds, &first_blade, &second_blade);
    else if (phase == 16)
        *bounds = first_blade;
    else if (phase == 17)
        *bounds = second_blade;
    else
        valid = 0;
    if (valid)
        OffsetRect(bounds, bounds, (Cell.column + (Cell.kind == 13 ? 1 : 0)) << 5,
                   63 * Cell.row);
    return valid;
}

int GetScytheHitType(word value)
{
    word low5 = value & 31;

    if (low5 == 2 || (low5 >= 16 && low5 <= 17)) {
        low5 = 1;
    } else {
        low5 = 0;
    }

    return low5;
}

void RedrawScythe(int bloody)
{
    Rect bounds;
    if (CheckTrobVis()) {
        bounds = redraw_bounds;
        if (bloody) {
            bounds.right += 6;
            --bounds.top;
        }
        if (ComputeTrobMoveRect(&bounds, &bounds))
            MarkBufsForRect(MarkMove, &bounds, -1);
    } else {
        Trob.state = 1;
    }
}

int ScytheBarrToStandUp(void)
{
    int blocked = 0;
    sbyte kind = ReadCell(Char.room, Char.column, Char.row);
    if (kind == 12 && ScytheBladeExtended(Cell.modifier)) {
        blocked = 1;
    } else if (kind == 13) {
        ReadCell(Char.room, Char.column - 1, Char.row);
        if (ScytheBladeExtended(Cell.modifier) &&
            ColEdgeX[Char.column + 5] + 9 >= LeftEdge)
            blocked = 1;
    }
    return blocked;
}

int ScytheBladeExtended(word value)
{
    word low5 = value & 31;
    return low5 >= 3 && low5 <= 15;
}
void TriggerScythe(byte room, sbyte index)
{
    if (Cell.kind == 12) {
        ReadCell(room, Cell.column + 1, Cell.row);
        room = Cell.room;
        index = Cell.index;
    }
    if (!(Level.cell_info[room - 1][index].modifier & 31))
        AddTrob(room, index, 1, 13);
}
