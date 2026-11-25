#include <stdlib.h>
#include "types.h"
#include "snd.h"
#include "graf.h"
#include "shape.h"
#include "level.h"
#include "heap.h"
#include "char.h"
#include "game.h"
#include "movescyt.h"
#include "draw.h"
#include "act.h"
#include "add.h"
#include "autohead.h"
#include "auto.h"
#include "fade.h"
#include "collide.h"
#include "ctrl.h"
#include "moveruin.h"
#include "move.h"
#include "prince.h"
#include "sound.h"
#include "tables.h"
#include "top.h"
#include "util.h"

#pragma pack(1)


typedef struct {
    sbyte x, y;
} NeckPoint;
#pragma pack()


static const sbyte first_chomp[12] = {
    1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 99
};
static const sbyte last_chomp[12] = {
    1, 1, 2, 2, 2, 3, 3, 3, 3, 4, 4, 99
};
static const int flee_length[12] = {
    70, 65, 60, 55, 50, 45, 40, 35, 30, 25, 20, 15
};
typedef int COL_EDGE_X[20];                           /* DS:0d7c */


void unused_head_entry(void)
{
}

void AlignHeadOnKidNeck(void)
{
    sbyte neck_x;
    SHAPESIZE bounds;
    OBJITEM *body;
    NeckPoint far *neck;

    body = GetObjTableType(2, 0);
    if (body == 0)
        return;

    neck = hLock(hHeadChompFrame);
    if (Kid.direction != Char.direction)
        neck_x = neck[body->image].y;
    else {
        GetSetImgSize(Pics[3], FChar.image, &bounds);
        neck_x = -((Char.character_kind == 7 ? 10 : 16) + neck[body->image].y + bounds.width);
    }
    if (Char.character_kind == 8)
        neck_x += 6;
    if (Char.direction == 0)
        neck_x = -neck_x;
    FChar.x = body->x + neck_x;
    FChar.y = body->y + ((sbyte far *)neck)[body->image * 2];
    hUnlock(hHeadChompFrame);
    Char.x = FChar.x + 130;
}

/* FM Towns CharCanSeeOpp: the same statements and the same order of locals. */
#define ABS(x) ((x) < 0 ? -(x) : (x))

int CharCanSeeOpp(void)
{
    sbyte end_column, end_row, column, row, columns, rows, step_column, step_row, straight_error, diagonal, error, tile;
    int clear = 1;
    int result;

    if (Opp.column == Char.column && Opp.row == Char.row) {
        result = 4;
    } else {
        row = Char.row * 2;
        column = Char.column * 2;
        if ((Char.x + Frame.x - 122) % 32 > 16)
            ++column;
        end_row = Opp.row * 2 + 1;
        end_column = Opp.column * 2;
        if ((Opp.x - 130) % 32 > 16)
            ++end_column;

        columns = ABS(end_column - column);
        rows = ABS(end_row - row);
        step_column = end_column > column ? 1 : -1;
        step_row = end_row > row ? 1 : -1;

        if (columns > rows) {
            error = rows * 2 - columns;
            straight_error = rows * 2;
            diagonal = (rows - columns) * 2;
            do {
                column += step_column;
                if ((step_column > 0 && column % 2 == 0) ||
                    (step_column < 0 && column % 2 != 0)) {
                    tile = ReadCell(Char.room, column / 2, row / 2);
                    clear = !IsWall(tile);
                }
                if (clear) {
                    if (error < 0)
                        error += straight_error;
                    else {
                        if ((step_row > 0 && row % 2 != 0) ||
                            (step_row < 0 && row % 2 == 0)) {
                            if (step_row < 0)
                                tile = ReadCell(Char.room, column / 2, (row - 1) / 2);
                            else
                                tile = ReadCell(Char.room, column / 2, row / 2);
                            clear = IsSpace(tile);
                        }
                        row += step_row;
                        error += diagonal;
                    }
                }
            } while (--columns && clear);
        } else {
            error = columns * 2 - rows;
            straight_error = columns * 2;
            diagonal = (columns - rows) * 2;
            do {
                if ((step_row > 0 && row % 2 != 0) ||
                    (step_row < 0 && row % 2 == 0)) {
                    if (step_row < 0)
                        tile = ReadCell(Char.room, column / 2, (row - 1) / 2);
                    else
                        tile = ReadCell(Char.room, column / 2, row / 2);
                    clear = IsSpace(tile);
                }
                if (clear) {
                    row += step_row;
                    if (error < 0)
                        error += straight_error;
                    else {
                        column += step_column;
                        if ((step_column > 0 && column % 2 == 0) ||
                            (step_column < 0 && column % 2 != 0)) {
                            tile = ReadCell(Char.room, column / 2, row / 2);
                            clear = !IsWall(tile);
                        }
                        error += diagonal;
                    }
                }
            } while (--rows && clear);
        }
        if (clear) {
            if (!IsCharClosestToKid())
                result = 4;
            else if (Opp.row != Char.row)
                result = step_row > 0 ? 2 : 3;
            else
                result = step_column > 0 ? 1 : 0;
        } else
            result = -1;
    }
    return result;
}


int ChompAgain(HEADPLAN *plan)
{
    byte room_skill = Level.room_objects[Char.room - 1].opponents[Char.slot].skill;
    int result;
    sbyte column;

    if (plan->column < first_chomp[room_skill])
        result = 1;
    else if (last_chomp[room_skill] <= (column = plan->column))
        result = 0;
    else
        result = Rnd(1);
    return result;
}

OPPONENT *HeadAddOpponent(OPPONENT *opponent)
{
    if (opponent->kind == 0) {
        opponent->kind = 5;
        Char.character_kind = OppCharId[5];
        ResetCharY();
    }
    if (Char.character_kind == 11) {
        if (Char.alive < 0) {
            if (opponent->u.head.mode <= 0) {
                opponent->action = 170;
                opponent->saved_sequence = 0;
                opponent->u.head.mode = -1;
            } else {
                opponent->action = 175;
                opponent->saved_sequence = 0;
                Char.sequence_group = 6;
            }
            Char.stance = 1;
            AddSound(71);
        } else
            Char.stance = 0;
    } else {
        Char.x = ColEdgeX[Char.column + 5] + 30;
        Char.direction = -1;
        opponent->action = 130;
        opponent->saved_sequence = 0;
        opponent->u.head.mode = Char.stance = 0;
    }
    return opponent;
}

void HeadAttackProg(HEADPLAN *plan)
{
    byte dx, dy;
    int direction, vertical_sequence;

    if (Char.sequence == 140 || Char.sequence == 152 ||
        Char.sequence == 156 || Char.sequence == 162)
        return;
    if (Char.sequence >= 147 && Char.sequence <= 151) {
        HeadFocusOnNeck(plan);
        return;
    }
    if (Char.frame != 150 && Char.frame != 165 && Char.frame != 166 &&
        Char.frame != 158 && Char.frame != 173)
        return;
    direction = CharCanSeeOpp();
    if (direction != -1) {
        plan->state.attack.direction = 1;
        {
            int delta = Char.column - Opp.column;
            dx = delta < 0 ? -delta : delta;
        }
        {
            int delta = Char.row - Opp.row;
            dy = delta < 0 ? -delta : delta;
        }
        if (dy == 0 && dx < 2) {
            vertical_sequence = Char.x < Opp.x ? 1 : 0;
            if (dx != 0 && !HeadClearInDir(vertical_sequence)) {
                if (Char.sequence != 166 && YWithinMiddleOfCell(Char.y, 15))
                    JumpSeq(166);
                return;
            }
            if (Char.sequence != 147)
                JumpSeq(147);
        } else if (dx == 0 && dy == 1 && HeadClearInDir(direction)) {
            if (Opp.row < Char.row)
                vertical_sequence = 143;
            else
                vertical_sequence = 144;
            if (vertical_sequence != Char.sequence)
                JumpSeq(vertical_sequence);
        } else if (direction == 4) {
            if (dx || dy) {
                if (Char.sequence != 166 && YWithinMiddleOfCell(Char.y, 15))
                    JumpSeq(166);
            }
        } else {
            plan->row = Opp.row;
            plan->column = Opp.column;
            if ((direction == 3 || direction == 2) && !HeadClearInDir(direction))
                direction = Char.x < Opp.x ? 1 : 0;
            if (HeadClearInDir(direction)) {
                if (direction == 3)
                    direction = 143;
                else if (direction == 2)
                    direction = 144;
                else {
                    direction = 142;
                    HeadFaceKid();
                    ResetCharY();
                }
            } else {
                direction = 166;
                ClearCharVel();
            }
            if (Char.sequence != direction)
                JumpSeq(direction);
        }
    } else {
        if (plan->state.attack.direction != 0) {
            plan->state.attack.direction = 0;
            plan->column = Opp.column;
            plan->row = Opp.row;
        }
        HeadGoToDest(plan);
    }
}

int HeadBlocked(void)
{
    int sequence;
    if (DecLifePts(1))
        sequence = 154;
    else {
        AddSound(87);
        HeadSetToFleeMode();
        sequence = 153;
    }
    return sequence;
}

int HeadClearInDir(int direction)
{
    sbyte tile;
    int clear = 0;
    switch (direction) {
    case 0:
        if (Char.column < -1 || JustStartVerticalSeq())
            break;
        tile = ReadCell(Char.room, Char.column - 1, Char.row);
        if (IsWall(tile))
            break;
        if (tile == 4 && GateBarr())
            break;
        clear = 1;
        break;
    case 1:
        if (Char.column > 10 || JustStartVerticalSeq())
            break;
        tile = ReadCell(Char.room, Char.column + 1, Char.row);
        if (IsWall(tile))
            break;
        if (tile == 4 && GateBarr())
            break;
        if (ReadCell(Char.room, Char.column, Char.row) != 4 || !GateBarr())
            clear = 1;
        break;
    case 2:
        if (Char.row != 2 && XWithinMiddleOfCell(Char.x, 8))
            clear = IsSpace(ReadCell(Char.room, Char.column, Char.row));
        break;
    case 3:
        if (Char.row != 0 && XWithinMiddleOfCell(Char.x, 8))
            clear = IsSpace(ReadCell(Char.room, Char.column, Char.row - 1));
        break;
    }
    return clear;
}

void HeadDoneAttackingKid(HEADPLAN *plan)
{
    HeadGetPeaceful(plan);
    Char.sequence_group = 0;
}

int HeadEnemyCollide(int minimum_gap)
{
    int sequence = -1;
    int gap;

    if (Char.sequence_group != 1) {
        gap = GetBarrDistFromFace(Cell.kind, Cell.room, Cell.column);
        if (gap < 5 && minimum_gap <= gap && gap < 5 && -ImWidth <= gap) {
            Char.x = AddCharX(-(gap + 14));
            if (Char.alive < 0 && Char.sequence != 153)
                sequence = 157;
            else if (Char.sequence != 164) {
                if (Char.frame <= 191) {
                    sequence = 164;
                    Char.x = ColEdgeX[GetCellCol() + 5] + 14;
                    if (Char.direction == -1)
                        Char.x += 32;
                } else
                    JumpSeq(145);
                DecLifePts(100);
            }
        }
    }
    return sequence;
}

void HeadFaceKid(void)
{
    sbyte column = Char.column;
    if (column < Opp.column) {
        if (Char.direction != 0) {
            Char.x = AddCharX(-14);
            Char.direction = 0;
        }
    } else if (Char.direction != -1) {
        Char.x = AddCharX(-14);
        Char.direction = -1;
    }
}

void HeadFleeProg(HEADPLAN *plan)
{
    if (Char.sequence != 153) {
        if (Char.sequence == 159) {
            if (plan->column == Char.column && XWithinMiddleOfCell(Char.x, 2)) {
                if (Char.row == Opp.row && abs(Char.column - Opp.column) < 3)
                    HeadGetAngry(plan);
                else {
                    JumpSeq(160);
                    plan->state.flee_timer = flee_length[Level.room_objects[Char.room - 1].opponents[Char.slot].skill];
                }
            }
        } else if (Char.row == Opp.row && abs(Char.column - Opp.column) < 3 &&
                   Opp.life_points && (Opp.sequence != 70 || (Opp.frame != 0 && Opp.frame < 223)))
            HeadGetAngry(plan);
        else if (--plan->state.flee_timer == 0) {
            if (Opp.life_points && (Opp.sequence != 70 || (Opp.frame != 0 && Opp.frame < 223)) &&
                CharCanSeeOpp() != -1) {
                HeadGetAngry(plan);
                JumpSeq(162);
            } else {
                HeadGetPeaceful(plan);
                JumpSeq(163);
            }
        }
        if (!plan->sound_played && !SndOn(10087)) {
            plan->sound_played = 1;
            AddSong(146);
        }
    } else if (Char.sequence_group == 2) {
        Char.alert_delay = 0;
        JumpSeq(159);
    }
}

void HeadFocusOnNeck(HEADPLAN *plan)
{
    int low, high, distance;
    if (Char.sequence == 147 || Char.sequence == 144 || Char.sequence == 143) {
        if (Opp.stance == 1) {
            low = -25;
            high = 0;
        } else {
            low = -3;
            high = 30;
        }
        distance = GetOppDist();
        if (Char.row == Opp.row && low <= distance && high >= distance) {
            ClearCharVel();
            JumpSeq(Rnd(1) + 150);
            plan->mode = 3;
            plan->column = 1;
        } else {
            int close_limit = Opp.stance == 1 ? -35 : -13;
            if ((Char.row != Opp.row || close_limit > distance) && Char.sequence == 147) {
                ClearCharVel();
                JumpSeq(156);
            }
            if (distance - 64 <= high && Char.row == Opp.row && Char.sequence != 144)
                AddSound(90);
        }
    }
}

void HeadGetAngry(HEADPLAN *plan)
{
    plan->mode = 1;
    plan->row = Opp.row;
    plan->column = Opp.column;
    plan->state.attack.direction = 1;
    JumpSeq(140);
    ClearCharVel();
    HeadFaceKid();
    Char.alert_delay = 3;
    Char.stance = 1;
    if (Opp.opponent_index == -1) {
        SaveShad();
        DrawOppMeter(Char.slot, Char.life_points, Char.life_initial);
        LoadShad(Char.slot);
        Opp.opponent_index = Char.slot;
    }
    AddSound(87);
}

void HeadGetPeaceful(HEADPLAN *plan)
{
    plan->mode = 0;
    Char.stance = 0;
    ClearCharVel();
    JumpSeq(139);
    Char.column = GetCharCol(GetBaseX());
    Char.x = ColEdgeX[Char.column + 5] + 30;
    ResetCharY();
    Char.alert_delay = 0;
}

int HeadGoToDest(HEADPLAN *plan)
{
    int clear = 0;
    int sequence;
    int direction;
    if (Char.column != plan->column || plan->row != Char.row) {
        if (plan->row != Char.row) {
            if (plan->row > Char.row) {
                direction = 2;
                sequence = 144;
            } else {
                direction = 3;
                sequence = 143;
            }
            clear = HeadClearInDir(direction);
            if (clear && Char.sequence != sequence)
                JumpSeq(sequence);
        }
        if (!clear && Char.column != plan->column) {
            if (plan->column > Char.column)
                sequence = 1;
            else
                sequence = 0;
            clear = HeadClearInDir(sequence);
            if (clear && Char.sequence != 142)
                JumpSeq(142);
        }
    }
    if (!clear)
        HeadGetPeaceful(plan);
    return clear;
}

void HeadLockedProg(HEADPLAN *plan)
{
    int lost = 0;
    OPPONENT *record;
    if ((Char.sequence == 150 && Char.frame == 167 && Char.sequence_group != 1) ||
        (Char.sequence == 151 && Char.frame == 166)) {
        if (ChompAgain(plan)) {
            if (Opp.alive < 0) {
                ++plan->column;
                JumpSeq(150 + (Char.sequence == 150));
            } else {
                record = GetOppInfoIdx(Char.room, Char.slot);
                if (record && record->skill >= 11)
                    JumpSeq(165);
            }
        } else {
            JumpSeq(152);
            Char.x = AddCharX(-8);
            lost = 1;
        }
    }
    if (lost || Char.sequence == 158 || Char.sequence == 157) {
        plan->mode = 1;
        plan->column = Opp.column;
        plan->row = Opp.row;
    }
}

void HeadOnAlert(void)
{
}

void HeadPeacefulProg(void)
{
    int direction, sequence;
    if (Char.frame != 150)
        return;
    direction = Rnd(3);
    if (direction == 3) {
        if (Char.sequence >= 131 && Char.sequence <= 133)
            direction = 0;
        else if (Char.sequence >= 134 && Char.sequence <= 136)
            direction = 1;
        else
            direction = -1;
    } else if (direction == 2)
        direction += Rnd(1);
    if (direction != -1 && HeadClearInDir(direction)) {
        if (direction == 3)
            sequence = 137;
        else if (direction == 2)
            sequence = 138;
        else if (direction == 0)
            sequence = Rnd(2) + 131;
        else
            sequence = Rnd(2) + 134;
        JumpSeq(sequence);
    }
}

void HeadProg(void)
{
    OPPONENT *record;
    HEADPLAN *plan;
    if (Char.alive >= 0)
        return;
    record = GetOppInfoIdx(Char.room, Char.slot);
    if (!record)
        return;
    plan = &record->u.head;
    LoadShadwOpp(Char.slot);
    GetFrameInfo();
    if (Frame.image != -1) {
        FChar.image = Frame.image;
        GetCharEdges();
    }
    switch (plan->mode) {
    case 0:
        if (Opp.life_points && (Opp.sequence != 70 || (Opp.frame != 0 && Opp.frame < 223)) &&
            (Char.frame == 150 || Char.frame == 165 || Char.frame == 166 ||
             Char.frame == 158 || Char.frame == 173) && CharCanSeeOpp() != -1)
            HeadGetAngry(plan);
        else
            HeadPeacefulProg();
        break;
    case 1:
        if (Opp.life_points && (Opp.sequence != 70 || (Opp.frame != 0 && Opp.frame < 223)))
            HeadAttackProg(plan);
        else
            HeadDoneAttackingKid(plan);
        break;
    case 2:
        HeadFleeProg(plan);
        break;
    case 3:
        if (Opp.life_points && (Opp.sequence != 70 || (Opp.frame != 0 && Opp.frame < 223)))
            HeadLockedProg(plan);
        else
            HeadDoneAttackingKid(plan);
        break;
    }
    SaveShadwOpp();
}

int HeadSetToFleeMode(void)
{
    byte skill;
    sbyte column;
    int clear, edge, initial_column;
    OPPONENT *record;

    skill = Level.room_objects[Char.room - 1].opponents[Char.slot].skill;
    if (IsAbleToFleeInRoom(Char.room) && Rnd(skill / 3 + 1) == 0 &&
        ((Char.direction == 0 && Char.column >= 3) ||
         (Char.direction == -1 && Char.column + 3 <= 10))) {
        clear = 1;
        initial_column = *(volatile sbyte *)&column = Char.column;
        edge = Char.direction == 0 ? -1 : 10;
        if (initial_column != edge)
            do {
                if (!clear)
                    break;
                skill = ReadCell(Char.room, column, Char.row);
                if (GetBarrCode(skill) && (skill != 4 || GateBarr()))
                    clear = 0;
                column += Minus1[Char.direction + 1];
            } while (column != edge);
        if (!clear) {
            column += Plus1[Char.direction + 1] * 2;
            if (abs(column - Char.column) >= 3)
                clear = 1;
        } else
            column += Plus1[Char.direction + 1];
        if (clear) {
            Char.stance = 0;
            Char.alert_delay = 1;
            record = GetOppInfoIdx(Char.room, Char.slot);
            record->u.head.mode = 2;
            record->u.head.column = column;
            record->u.head.row = Char.row;
            record->u.head.state.flee_timer = -1;
            record->u.head.sound_played = 0;
            SaveShad();
            DrawOppMeter(Char.slot, 0, Char.life_initial);
            LoadShad(Char.slot);
            if (Char.slot == Opp.opponent_index)
                Opp.opponent_index = -1;
        }
    } else
        clear = 0;
    return clear;
}

int IsAbleToFleeInRoom(byte room)
{
    sbyte slot, count;
    int room_number = room;
    int available;

    count = Level.room_objects[room_number - 1].opponent_count;
    if (count > 1) {
        available = 1;
        slot = 0;
        do {
            if (!(slot < count))
                break;
            {
                int can_flee = GetOppInfoIdx(room, slot)->u.head.mode != 2;
                available = can_flee;
                ++slot;
                if (!can_flee)
                    break;
            }
        } while (1);
    } else
        available = 0;
    return available;
}


int IsHeadLockedOn(byte room, int slot)
{
    OPPONENT *record;
    int result;
    record = GetOppInfoIdx(room, slot);
    result = record != 0 && record->u.head.mode == 3;
    return result;
}

int JustStartVerticalSeq(void)
{
    int baseline = Char.row * 63 + 37;
    int result;
    if (Char.sequence != 144) {
        if (Char.sequence == 143)
            result = Char.y < baseline;
        else
            result = 0;
        return result;
    } else if (Char.y > baseline)
        result = 1;
    else
        result = 0;
    return result;
}

int XWithinMiddleOfCell(int first, int second)
{
    int result;
    first = (first - 144) % 32;
    result = first >= 16 - second && 16 + second >= first;
    return result;
}

int YWithinMiddleOfCell(int value, int half_width)
{
    int result;
    value = (value - 3) % 63;
    result = value >= 31 - half_width && 31 + half_width >= value;
    return result;
}

void SnakeProg(void)
{
    OPPONENT *record;
    if (Char.alive < 0) {
        record = GetOppInfoIdx(Char.room, Char.slot);
        if (record) {
            LoadShadwOpp(Char.slot);
            if (record->u.head.mode < 0) {
                if (SnakeSlitheringTowardOpp())
                    SnakeAttack();
                else
                    SnakeGoIntoHole(record);
            } else
                SnakeComeOutOfHole(record);
            SaveShadwOpp();
        }
    }
}

void SnakeAttack(void)
{
    int hit = 0;
    int distance = GetOppDist();
    sbyte tile;
    if (distance >= 13 && distance <= 45) {
        if (Char.frame == 278 || Char.frame == 279 ||
            (Char.sequence == 176 &&
             ((Opp.frame >= 1 && Opp.frame <= 14) || (Opp.frame >= 49 && Opp.frame <= 56))))
            hit = 1;
        else if (Char.sequence != 176 && Char.sequence != 173) {
            JumpSeq(176);
            AddSound(71);
        }
    } else if (Char.sequence == 176 &&
               ((Opp.frame >= 1 && Opp.frame <= 14) || (Opp.frame >= 49 && Opp.frame <= 56)) &&
               distance > 13 && distance < 50)
        hit = 1;
    else if (Char.sequence == 176 && Opp.frame >= 135 && Opp.frame <= 149 &&
             distance > 13 && distance < 45)
        hit = 1;
    else if ((Char.frame == 278 || Char.frame == 279) &&
             (distance < 13 || distance > 49)) {
        tile = GetCellsInFront(1);
        if (IsFloor(tile) || (tile == 7 && (Cell.modifier & 128)))
            JumpSeq(177);
    }
    if (hit && Char.sequence != 173) {
        Char.x = AddCharX(distance - 32);
        JumpSeq(173);
    }
}

void SnakeComeOutOfHole(OPPONENT *record)
{
    sbyte tile;
    sbyte left, right;
    if (--record->u.head.mode > 0)
        return;
    record->u.head.mode = -1;
    left = Get1stWallColInDir(Char.room, Char.column, Char.row, -1);
    right = Get1stWallColInDir(Char.room, Char.column, Char.row, 1);
    tile = GetUnderFt();
    if (Opp.room == Char.room && Char.row == Opp.row &&
        Char.column > left && Char.column < right) {
        if (Char.column <= Opp.column &&
            (!IsWall(tile) || IsFloor(tile) || (tile == 7 && (Cell.modifier & 128))))
            Char.direction = 0;
        else
            Char.direction = -1;
    } else
        Char.direction = ~Char.direction;
    if (IsWall(tile) && !IsFloor(tile) && (tile != 7 || !(Cell.modifier & 128))) {
        JumpSeq(170);
        Char.x = AddCharX(38);
    } else {
        JumpSeq(172);
    }
    Char.sequence_group = 0;
}

void SnakeGoIntoHole(OPPONENT *record)
{
    sbyte tile;
    int distance;
    if (Opp.alive >= 0) {
        if (Char.sequence != 176 && Char.frame != 278 && Char.frame != 279)
            JumpSeq(176);
        return;
    }
    if (Char.sequence != 171 && Char.sequence != 172 && Char.sequence_group != 9) {
        GetUnderFt();
        if ((((byte *)&Cell.modifier)[1] & 15) == 12) {
            distance = (Char.x - 144) % 32;
            if (distance >= 14 && distance <= 18) {
                distance = 16 - distance;
                if (Char.direction == -1)
                    distance = -distance;
                else
                    distance += 8;
                Char.x = AddCharX(distance);
                JumpSeq(171);
            }
        } else if (Char.sequence != 176 && Char.sequence != 168 &&
                   Char.frame != 278 && Char.frame != 279) {
            tile = GetCellsInFront(1);
            if (IsFloor(tile) || (tile == 7 && (Cell.modifier & 128)))
                return;
            if (!IsWall(tile))
                JumpSeq(176);
            else {
                if (tile != 7) {
                    if (Char.direction == 0) {
                        Char.sequence_group = 9;
                        return;
                    }
                    if (Char.frame != 254)
                        return;
                }
                JumpSeq(168);
            }
        } else {
            tile = GetCellsInFront(1);
            if (IsFloor(tile) || (tile == 7 && (Cell.modifier & 128)))
                JumpSeq(177);
        }
    } else {
        if (Char.sequence_group == 9) {
            tile = GetUnderFt();
            distance = GetDist();
            if (IsWall(tile) && distance < 8) {
                Char.frame = 264;
                JumpSeq(168);
            }
        }
        if (Char.frame == 264) {
            Char.sequence_group = 6;
            record->u.head.mode = Char.alert_delay == 2 ? 6 : Rnd(60) + 36;
        }
    }
}

int SnakeSlitheringTowardOpp(void)
{
    int result = 0;
    if (Char.sequence != 171 && Char.alert_delay >= 2) {
        if ((Char.direction == 0 && Char.column <= Opp.column) ||
            (Char.direction == -1 && Char.column >= Opp.column))
            result = 1;
    }
    return result;
}

void SnakeUpdateOppInfo(OPPONENT *record)
{
    sbyte tile;
    if (Char.sequence == 171 || Char.sequence_group == 9) {
        Char.sequence_group = 6;
        record->u.head.mode = Rnd(60) + 36;
    } else {
        tile = GetCellsInFront(1);
        if (!IsFloor(tile) && (tile != 7 || !(Cell.modifier & 128)) &&
            (Char.sequence == 176 || Char.sequence == 168 || Char.sequence == 170)) {
            record->direction = ~Char.direction;
            record->x = AddCharX(-32);
            return;
        }
    }
}

Handle hHeadChompFrame = 0;
