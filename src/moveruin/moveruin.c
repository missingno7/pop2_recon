/* MOVERUIN.C, DOS overlay 8 code frame 2a4e. */
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
#include "moveruin.h"
#include "prince.h"
#include "sound.h"
#include "tables.h"
#include "top.h"


void RuinsClipChar(void)
{
    int clip_lower;
    sbyte column, left_column;
    int previous_cell_remaining, current_cell_offset;

    clip_lower = Char.character_kind == 7 || Char.character_kind == 8;
    column = GetCol(CDRightEdge);
    if (IsWall(ReadCell(Char.room, column, Char.row))) {
        left_column = ReadCell(Char.room, column, Cell.top_row);
        if (IsWall(left_column) && left_column != 7 && Char.room == Cell.room)
            FChar.clip.right = Cell.column * 32;
    }
    if (clip_lower && Char.sequence != 137 && Char.sequence != 138 &&
        Char.sequence != 143 && Char.sequence > 144 &&
        Char.sequence != 143 && Char.sequence != 144) {
        left_column = GetCol(CDLeftEdge);
        if (GetCol(CDRightEdge) != left_column) {
            previous_cell_remaining = 32 - (CDLeftEdge - 130) % 32;
            current_cell_offset = (CDRightEdge - 32) % 32;
            if (previous_cell_remaining > current_cell_offset)
                --column;
        }
        if (!IsSpace(ReadCell(Char.room, column, Cell.top_row)))
            FChar.clip.top = 63 * ((sbyte)Cell.bottom_row) + 3;
    }
}

void BreakTotalLoose(void)
{
    int saved_x;

    Mob.x = (Cell.index % 10) * 32;
    Mob.y = 63 * Cell.row + 66;
    Mob.room = Cell.room;
    Mob.velocity = Mob.auxiliary = 0;
    Mob.kind = 3;
    Mob.row = Cell.row;
    Mob.flags = 0;
    AddMob();
    MakeSpace(Cell.room, Cell.index);
    saved_x = Mob.x;
    if (Room.left == Mob.room)
        Mob.x -= 320;
    MarkMobRedraw(0);
    Mob.x = saved_x;
}

void CutKidInsideLevel6(void)
{
    sbyte start_room;

    Char.direction = 0;
    JumpSeq(5);
    start_room = 3;
    Char.room = start_room;
    CutRoom = start_room;
    Char.column = 4;
    Char.x = ColEdgeX[9] + 14;
    Char.row = 2;
    ResetCharY();
    AnimChar();
    Kid = Char;
    Entrance();
    ClrAll();
    Dir.action = 0;
}

void LoadRNStart6(void)
{
    SetPalette(3500, 64, 160, 1);
}

void UnloadRNStart6(void)
{
    if (Kid.alive < 0 && Minutes)
        SetPalette(3500, 64, 160, 0);
}

void LoadRuinsMoveCode(void)
{
}

void LoadPlateauRoomFunc(void)
{
    if (Kid.column >= 9) {
        if (Room.number == 16 && Level.level_number == 9) {
            AddSong(92);
            return;
        }
    } else if (Room.number == 2 && Level.level_number == 9 && !GameState.plateau_loaded
               && Kid.column < 5) {
        AddSong(91);
        GameState.plateau_loaded = 1;
    }
}
