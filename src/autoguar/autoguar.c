/* AUTOGUAR.C. Opening routines in original order. */
#include <stdlib.h>
#include "types.h"
#include "load.h"
#include "graf.h"
#include "shape.h"
#include "level.h"
#include "dissolve.h"
#include "heap.h"
#include "checknis.h"
#include "gameport.h"
#include "char.h"
#include "autoguar.h"
#include "game.h"
#include "draw.h"
#include "act.h"
#include "add.h"
#include "auto.h"
#include "gfxerr.h"
#include "diag.h"
#include "moveruin.h"
#include "collide.h"
#include "bridge.h"
#include "ctrl.h"
#include "snd.h"
#include "fade.h"
#include "move.h"
#include "drawruin.h"
#include "prince.h"
#include "misc.h"
#include "sound.h"
#include "tables.h"
#include "util.h"
#include "top.h"


void unused_guard_entry(void)
{
}

void AutoFwd(void)
{
    Dir.forward = Joy.x = -1;
}

void AutoBack(void)
{
    Dir.back = -1;
    Joy.x = 1;
}

void AutoUp(void)
{
    Dir.up = Joy.y = -1;
}

void AutoDown(void)
{
    Dir.down = -1;
    Joy.y = 1;
}

void AutoRunAway(void)
{
    Dir.down = -1;
    AutoBack();
}

void AutoEnGarde(void)
{
    Dir.down = -1;
    AutoFwd();
}

void AutoPress(byte value)
{
    Joy.button = value;
    Dir.action = value;
}


void CheckMultipleDeadBodies(void)
{
    int sequence;
    if (Char.frame == 185) {
        sequence = GetOppDeadSeq(Level.preferred_opponent);
        if (sequence != 0) {
            JumpSeq(sequence);
            AnimChar();
        }
        if (Char.alive < 20)
            ++Char.alive;
    }
}

int GetOppDeadSeq(byte kind)
{
    int count;
    int sequence_result;
    int sequence;

    sequence = 0;
    sequence_result = sequence;
    if (kind == 7) {
        sequence = 191;
        count = 4;
    } else if (kind == 0 && Level.environment_kind != 6) {
        sequence = 195;
        count = 1;
    }

    if (sequence && Char.slot % (count + 1))
        sequence_result = sequence + Char.slot % (count + 1) - 1;

    return sequence_result;
}

void DoOppStartRun(void)
{
    JumpSeq(Char.character_kind == 4 ? 102 : 84);
    Char.stance = 0;
    Char.alert_delay = 0;
}


int DoOppTumbleSeq(void)
{
    sbyte tile, live, count, index;
    int result;
    CHARREC saved;

    if (Level.environment_kind == 5)
        result = Char.room == 19 || Char.room == 16 ||
            (IsSpace(GetBehind()) && Char.direction == -1);
    else
        result = 0;

    if (!result && IsOppGenPtWithTumbleOn(Char.room)) {
        live = GetTotalDeadOpps();
        result = (sbyte)Rnd(3) <= live;
        if (!result && live != 0) {
            saved = Char;
            count = Level.room_objects[Room.number - 1].opponent_count;
            for (index = 0; index < count; ++index) {
                if (result)
                    break;
                if (index != saved.slot) {
                    LoadShad(index);
                    if (Char.row == saved.row &&
                        Char.column == saved.column &&
                        IsDeadPos(Char.frame))
                        result = 1;
                }
            }
            Char = saved;
        }
        if (result) {
            if (Opp.direction == Char.direction)
                result = 0;
            else {
                tile = GetUnderFt();
                if (tile == 3 || tile == 8 || tile == 4)
                    result = 0;
                else {
                    tile = GetBehind();
                    if (tile == 3 || tile == 8)
                        result = 0;
                }
            }
        }
    }
    return result;
}

void FollowKid(void)
{
    sbyte tile;
    int stop;

    if (Opp.action == 2 || Opp.action == 6)
        return;

    stop = 0;
    tile = GetCellsInFront(1);
    if (GetBarrCode(tile)) {
        stop = 1;
    } else if (!IsSpace(tile)) {
        AutoFwd();
    } else {
        ++Cell.row;
        tile = ReadCell(Cell.room, Cell.column, Cell.row);
        if ((IsImpassable(tile) && Char.character_kind != 10) ||
            GetBarrCode(tile) || IsSpace(tile) ||
            Char.row - Opp.row + 1 != 0) {
            stop = 1;
        } else {
            if (Level.background_overlays[0] != stop &&
                ReadCell(Cell.room, Cell.column - 1, Cell.row) == 2)
                stop = 1;
            if (!stop)
                AutoFwd();
        }
    }

    if (stop) {
        DroppedOut = 0;
        AutoBack();
    }
}

sbyte GetTotalDeadOpps(void)
{
    sbyte index, limit, live;

    live = 0;
    limit = Level.room_objects[Room.number - 1].opponent_count;
    for (index = 0; index < limit; ++index) {
        if (RoomOpp[index].alive >= 0)
            ++live;
    }
    return live;
}


void InRange(int distance)
{
    int horizontal, vertical;

    if (Opp.stance == 1 && Opp.direction != Char.direction) {
        GenFight(distance); return;
    }

    if (Refract == 0) {
        GetCharStrikeRanges(&vertical, &horizontal);
        if (horizontal > distance) {
            AutoPress(254);
            return;
        }
        if (!Char.row || Level.level_number != 5 ||
            (Char.room != 10 && Char.room != 7 && Char.room != 12) ||
            IsOKForBrgSkelToAdvance())
            AutoFwd();
    }
    return;
}

void OppAlert(void)
{
    int distance;

    if (Opp.alive < 0) {
        distance = GetOppDist();
        if (Opp.row == Char.row && distance < 0 && distance >= -18) {
            if (Char.alert_delay >= 2 && Char.frame == 166)
                AutoEnGarde();
        } else if (AlertGuard) {
            if (distance < 0 && distance < -4)
                AutoDown();
        } else if (distance >= 0 && Char.alert_delay != 0)
            AutoEnGarde();
    }
}


void OppChasing(void)
{
    sbyte tile;
    OPPONENT *opponent;
    int nearest, kid_distance;
    int sequence;

    if (!((Char.frame >= 194 && Char.frame <= 199 &&
           (Char.frame & 1)) || Char.frame == 212)) {
        if (Char.behavior >= 1 && (Char.frame == 192 || Char.frame == 196)) {
            DoOppRunJump(); return;
        }
        return;
    }

    tile = GetOppIdxClosestToKid(~Char.direction);
    if (tile != Char.slot)
        nearest = abs(Char.x - RoomOpp[tile].x);
    else
        nearest = 121;

    tile = GetCellsInFront(1);
    kid_distance = abs(Char.x - Kid.x);
    if ((Kid.frame >= 1 && Kid.frame <= 14) ||
        (Kid.frame >= 49 && Kid.frame <= 56) ||
        (Kid.frame >= 34 && Kid.frame <= 44)) {
        if (Kid.direction != Char.direction)
            kid_distance -= 32;
        else
            kid_distance += 32;
    } else if (Kid.stance != 1) {
        kid_distance += 32;
    }

    opponent = GetOppInfoIdx(Char.room, Char.slot);
    if ((Kid.row == Char.row || opponent->u.skeleton.row == Kid.row) &&
        IsAdvancableId(tile, Cell.modifier)) {
        if ((Kid.row == Char.row &&
             !(Opp.direction == Char.direction &&
               ((Opp.frame >= 1 && Opp.frame <= 14) ||
                (Opp.frame >= 49 && Opp.frame <= 56) ||
                (Opp.frame >= 34 && Opp.frame <= 44)) &&
               Opp.action != 7) &&
             (nearest <= 120 || kid_distance <= 120)) ||
            (Char.direction == -1 && Kid.column > Char.column) ||
            (Char.direction == 0 && Kid.column < Char.column)) {
            Char.alert_delay = 2;
            sequence = 101;
        } else if (Kid.alive < 0 && Kid.sequence_group != 13) {
            if (IsExitChasingSkel(Char.room) && kid_distance <= 80) {
                sequence = 87;
                Char.stance = 1;
            } else
                sequence = -1;
        } else {
            Char.alert_delay = 0;
            sequence = 101;
        }
    } else {
        kid_distance = GetDist();
        if ((Kid.row == Char.row || opponent->u.skeleton.row == Kid.row) &&
            !(Char.behavior >= 1 && kid_distance < 27) &&
            Char.behavior >= 1 && tile != 4)
            sequence = -1;
        else {
            Char.alert_delay = 0;
            sequence = 101;
        }
    }

    if (sequence != -1) {
        JumpSeq(sequence);
        tile = GetBehind();
        if (Char.frame == 212 && !IsAdvancableId(tile, Cell.modifier))
            Char.x = AddCharX(8);
    }
}


void DoOppRunJump(void)
{
    sbyte tile1, tile2, start, steps_ahead;
    OPPONENT *opponent;
    int attempt, selected;

    if (tile1 = GetCellsInFront(1), IsSpace(tile2 = GetCellsInFront(2))) {
        if (IsSpace(tile1))
            start = 1;
        else {
            start = 2;
            tile1 = tile2;
        }
        steps_ahead = 0;
        if (IsSpace(tile1)) {
            while (steps_ahead <= 4) {
                ++steps_ahead;
                tile1 = ReadCell(Char.room, Char.column +
                                  Plus1[Char.direction + 1] * (steps_ahead + start),
                                  Char.row);
                if (!IsSpace(tile1))
                    break;
            }
        }

        if (steps_ahead < 4 || (steps_ahead == 4 && !Rnd(Char.behavior - 1)) ||
            (Char.room == 13 && Level.level_number == 13 && Kid.sequence_group != 13))
            attempt = 1;
        else {
            opponent = GetOppInfoIdx(Char.room, Char.slot);
            attempt = opponent && opponent->u.skeleton.row != Char.row;
        }

        selected = attempt ? DoRunJump(100) : 0;
        if (!selected) {
            JumpSeq(101);
            Char.alert_delay = 2;
        }
    }
}


void EnGarde(void)
{
    sbyte tile;
    int upper, lower;
    word frame = Char.frame;
    word pose;
    int distance;

    if (Char.alert_delay == 1 && Char.alert_delay == 2) {
        WaitingEngarde();
    } else if (frame != 166 && frame >= 150) {
        if (Char.alert_delay == 0) {
            if (DroppedOut)
                FollowKid();
            else if (Char.character_kind != 4) {
                AutoRunAway();
                return;
            }
        } else {
            pose = Opp.frame;
            distance = GetOppDist();
            if (distance >= 27 && pose >= 102 && pose < 118 && Opp.action == 5)
                return;

            GetCharStrikeRanges(&upper, &lower);
            if (lower + 12 > distance) {
                if (upper > distance) {
                    if (Char.row && Level.level_number == 5 &&
                        (Char.room == 10 || Char.room == 7 || Char.room == 12) &&
                        BrgSkelMustAdvance()) {
                        AutoFwd();
                        return;
                    }
                    tile = GetBehind();
                    if (IsAdvancableId(tile, Cell.modifier) && distance > 0) {
                        AutoBack();
                        return;
                    }
                    if (!Char.row || Level.level_number != 5 ||
                        (Char.room != 10 && Char.room != 7 && Char.room != 12) ||
                        IsOKForBrgSkelToAdvance()) {
                        AutoFwd();
                        return;
                    }
                } else {
                    InRange(distance);
                    return;
                }
            } else if (!Refract) {
                if (Opp.direction != Char.direction) {
                    if (pose >= 7 && pose < 15) {
                        if (distance < 71)
                            AutoPress(254);
                        return;
                    }
                    if (pose >= 34 && pose < 44) {
                        if (distance < 94)
                            AutoPress(254);
                        return;
                    }
                }
                NoJump();
            }
        }
    }
}


#define RUNNING_FRAME(frame) \
    (((frame) >= 1 && (frame) <= 14) || \
     ((frame) >= 49 && (frame) <= 56) || \
     ((frame) >= 34 && (frame) <= 44))


static const word strike_chance[12] =
    {75, 100, 75, 75, 75, 50, 100, 220, 0, 60, 40, 60};
static const word quick_strike_chance[12] =
    {0, 0, 0, 5, 5, 175, 20, 10, 0, 255, 255, 150};


void WaitingEngarde(void)
{
    sbyte slot, tile;
    int spacing, distance, can_stand;

    slot = GetOppIdxClosestToKid(~Char.direction);
    if (slot == -1)
        slot = GetOppIdxClosestToKid(1);
    if (slot == -1)
        return;
    do { if (Char.slot != slot) {
        can_stand = 0;
        tile = GetCellsInFront(1);
        if (IsAdvancableId(tile, Cell.modifier) &&
            IsAdvancableId(tile = GetCellsInFront(2), Cell.modifier) &&
            IsAdvancableId(tile = GetCellsInFront(3), Cell.modifier))
            can_stand = 1;

        distance = abs(Char.x - RoomOpp[slot].x);
        spacing = GetDistFromClosestOpp();
        if (spacing + 24 >= distance &&
            (RoomOpp[slot].direction != Char.direction || RoomOpp[slot].sequence != 84)) {
            if (can_stand && Opp.direction == Char.direction &&
                RUNNING_FRAME(Opp.frame) && Opp.action != 7 && Char.behavior &&
                ((Char.x < Opp.x && Opp.direction == 0) ||
                 (Char.x > Opp.x && Opp.direction == -1))) {
                DoOppStartRun();
                return;
            }

            if (distance < spacing)
                break;
            return;
        } else {
            if (!can_stand || Char.sequence == 84)
                return;
            if (((RoomOpp[slot].sequence == 84 && RoomOpp[slot].direction == Char.direction) ||
                 (Opp.direction == Char.direction && RUNNING_FRAME(Opp.frame) && Opp.action != 7)) &&
                Char.behavior)
                DoOppStartRun();
            else
                AutoFwd();
            return;
        }
    } else {
        distance = abs(Kid.x - Char.x);
        if (distance >= 64)
            return;
        if (distance < 32)
            tile = 1;
        else
            tile = 2;
        tile = GetCellsInFront(tile);
        if (!IsImpassable(tile) && !IsSpace(tile) &&
            (tile != 4 || (byte)Cell.modifier >= 112))
            return;
    } } while (0);
    if (IsAdvancableId(GetBehind(), Cell.modifier))
        AutoBack();
    return;
}

int GetDistFromClosestOpp(void)
{
    int spacing = 96;
    sbyte index, count;

    count = Level.room_objects[Room.number - 1].opponent_count;
    index = Char.slot + 1;
    if (count > index) {
        do {
            if (abs(Char.x - RoomOpp[index].x) < 24)
                spacing += 24;
        } while (++index < count);
    }
    return spacing;
}

void NoJump(void)
{
    sbyte front, further;

    front = GetCellsInFront(1);
    further = GetCellsInFront(2);
    if (IsSpace(front) && IsSpace(further)) {
        if (!(Char.row && Level.level_number == 5 &&
              (Char.room == 10 || Char.room == 7 || Char.room == 12) &&
              BrgSkelMustAdvance()))
            AutoBack();
        return;
    }

    if (Opp.direction == Char.direction && RUNNING_FRAME(Opp.frame) &&
        Opp.action != 7 && Char.behavior &&
        (!Char.row || Level.level_number != 5 ||
         (Char.room != 10 && Char.room != 7 && Char.room != 12))) {
        DoOppStartRun();
        return;
    }
    MaybeAdvance();
}

static const word block_chance[12] =
    {0, 150, 150, 200, 200, 255, 200, 250, 0, 255, 255, 255};
static const word quick_block_chance[12] =
    {0, 75, 75, 100, 100, 145, 100, 250, 0, 145, 255, 175};
static const word advance_chance[12] =
    {255, 200, 200, 200, 255, 255, 200, 0, 0, 255, 100, 100};


void GenFight(int distance)
{
    int upper, lower;

    GetCharStrikeRanges(&upper, &lower);
    if (upper <= distance && distance < lower &&
        !(Char.row && Level.level_number == 5 &&
          (Char.room == 10 || Char.room == 7 || Char.room == 12) &&
          BrgSkelMustAdvance())) {
        MaybeBlock();
        if (Refract == 0)
            MaybeStrike();
    } else {
        MaybeAdvance();
    }
}

void MaybeAdvance(void)
{
    byte skill;

    if (Char.row && Level.level_number == 5 &&
        (Char.room == 10 || Char.room == 7 || Char.room == 12) &&
        BrgSkelMustAdvance()) {
        AutoFwd();
        return;
    }
    if ((skill = Level.room_objects[Char.room - 1].opponents[Char.slot].skill) && GuardTimer)
        return;
    if (Char.row && Level.level_number == 5 &&
        (Char.room == 10 || Char.room == 7 || Char.room == 12) &&
        !IsOKForBrgSkelToAdvance())
        return;

    if (advance_chance[skill] > Rnd(255))
        AutoFwd();
}

void MaybeBlock(void)
{
    byte skill;

    if (Char.character_kind == 10 ||
        (Opp.frame != 152 && Opp.frame != 153 && Opp.frame != 162))
        return;
    skill = Level.room_objects[Char.room - 1].opponents[Char.slot].skill;

    if (JustBlocked) {
        if (quick_block_chance[skill] > Rnd(255))
            AutoUp();
    } else if (block_chance[skill] > Rnd(255))
        AutoUp();
}

void MaybeStrike(void)
{
    byte skill;

    if (Opp.frame == 169 || Opp.frame == 151)
        return;
    skill = Level.room_objects[Char.room - 1].opponents[Char.slot].skill;
    if (Char.character_kind != 7 && Char.character_kind != 8 &&
        (Char.frame == 161 || Char.frame == 150)) {
        if (quick_strike_chance[skill] > Rnd(255))
            AutoPress(254);
    } else if (strike_chance[skill] > Rnd(255))
        AutoPress(254);
}


int OpponentClose(sbyte slot, sbyte column, sbyte row, sbyte radius)
{
    sbyte tile, limit, step;
    int clear = 0;
    OPPONENT *opponent = GetOppInfoIdx(Room.number, slot);

    if (opponent && Char.alive >= 0 && Char.sequence_group != 5 && !Kid.character_kind &&
        (!opponent->u.skeleton.timer || IsExitChasingSkel(Char.room))) {
        if (Char.column > column) {
            limit = Char.column + 1;
            step = 1;
        } else {
            limit = Char.column - 1;
            step = -1;
        }

        if (Char.row == row && column - radius <= Char.column && column + radius >= Char.column) {
            do {
                tile = ReadCell(Char.room, column, row);
                if ((!IsImpassable(tile) || Char.character_kind == 10) &&
                    !IsWall(tile) && (tile != 4 || !GateBarr()))
                    clear = 1;
                else
                    clear = 0;
                column += step;
            } while (clear && limit != column);
        }
    }
    return clear;
}


void OpponentRise(void)
{
    sbyte index, count, radius;
    int started;
    OPPONENT *opponent;

    if (IsExitChasingSkel(Kid.room))
        radius = 8;
    else if (Level.preferred_opponent == 2)
        radius = 4;
    else
        radius = 5;

    count = Level.room_objects[Room.number - 1].opponent_count; index = 0;
    if (count > 0) {
        do {
            LoadShad(index);
            if (Kid.alive < 0 && Kid.action != 4 && Kid.action != 3 &&
                Kid.action != 5 && OpponentClose(index, Kid.column, Kid.row, radius)) {
                opponent = GetOppInfoIdx(Char.room, Char.slot);
                if (Level.preferred_opponent == 2) {
                    if (opponent->u.skeleton.shake_delay != -1) {
                        started = opponent->u.skeleton.timer; if (SkelDoneShaking(Char.slot)) {
                            JumpSeq(88);
                            started = 1;
                            Char.character_kind = 4;
                        } else {
                            if (!started && Char.sequence != 112)
                                JumpSeq(112);
                            started = 0;
                        }
                    }
                } else {
                    JumpSeq(106);
                    started = 1;
                    Char.character_kind = 10;
                }
                AnimChar();
                if (started) {
                    SetOppLifePts(opponent);
                    Char.alive = (byte)-1;
                    Char.stance = 1;
                    Char.alert_delay = 3;
                    if (Char.character_kind == 10) {
                        sbyte facing;
                        if (Char.x >= Kid.x)
                            facing = -1;
                        else
                            facing = 0;
                        Char.direction = facing;
                    }
                    AlertGuard = 0;
                    *(volatile int near *)&Refract = 0;
                    ClearCharVel();
                    if (Kid.opponent_index == -1)
                        Kid.opponent_index = Char.slot;
                }
            }
            SaveShad();
        } while (++index < count);
    }
}

void SkelProg(void)
{
    int active;
    OPPONENT *opponent;

    if (Char.alive < 0) {
        active = 1;
        if (Char.row && Level.level_number == 5 &&
            (Char.room == 10 || Char.room == 7 || Char.room == 12))
            active = BrgSkelProg();
        else if (Kid.alive >= 5 && Char.character_kind == 4 && Char.sequence != 120) {
            BonesFall();
            active = 0;
        }

        if (active) {
            if (Char.sequence == 88 && Char.frame == 177) {
                SkelFaceKid();
                return;
            }
            if (active) {
                GuardProg();
                return;
            }
        }
    } else {
        opponent = GetOppInfoIdx(Char.room, Char.slot);
        if (opponent && opponent->u.skeleton.timer > 0)
            --opponent->u.skeleton.timer;
    }
}


void BonesFall(void)
{
    int distance;

    JumpSeq(120);
    AddSound(76);
    Char.repeat = 1;
    Char.stance = 0;
    if (Char.alive < 0)
        Char.alive = 0;

    if (Char.direction == 0 && IsSpace(GetBehind())) {
        distance = 32 - GetDist();
        if (distance <= 35) {
            distance -= 35;
            Char.x = AddCharX(-distance);
        }
    }
    SaveShad();
    DrawOppMeter(Char.slot, 0, Char.life_initial);
    SetSkelTimer(Char.slot, 120);
    SetSkelShakes(Char.slot);
    LoadShad(Char.slot);
}

int IsExitChasingSkel(byte value)
{
    int result = 0;

    if (Level.level_number == 4 && value >= 22 && value <= 28 && value != 23) {
        result = 1;
    }
    return result;
}

void SetSkelShakes(sbyte id)
{
    OPPONENT *opponent = GetOppInfoIdx(Room.number, id);
    if (opponent) {
        if (opponent->skill)
            opponent->u.skeleton.shakes = (opponent->skill + 1) % 2;
        else
            opponent->u.skeleton.shakes = 3;
    }
}

void SetSkelTimer(sbyte id, int duration)
{
    OPPONENT *opponent;
    if (Level.preferred_opponent == 2 || RoomOpp[0].character_kind == 10) {
        opponent = GetOppInfoIdx(Room.number, id);
        if (opponent)
            opponent->u.skeleton.timer = duration;
    }
}

void ShakeBones(byte room, sbyte row)
{
    sbyte index, count;
    CHARREC saved;
    OPPONENT *opponent;

    saved = Char;
    index = 0;
    count = Level.room_objects[room - 1].opponent_count;
    if (count > 0) {
        do {
            LoadShad(index);
            if (Char.row == row && Char.frame == 206 && Char.sequence_group != 5) {
                opponent = GetOppInfoIdx(Char.room, Char.slot);
                if (opponent->u.skeleton.shake_delay == 0) {
                    JumpSeq(112);
                    SaveShad();
                }
            }
        } while (++index < count);
    }
    Char = saved;
}


OPPONENT *SkelAddOpponent(OPPONENT *opponent)
{
    int distance;

    if (opponent->u.skeleton.shake_delay == 1) {
        opponent->action = 63;
        Char.stance = 1;
    } else {
        if (Char.slot == 0 && opponent->u.skeleton.shake_delay == 0)
            opponent->action = 112;
        else
            opponent->action = 119;
        opponent->u.skeleton.timer = opponent->u.skeleton.shake_delay;
        SetSkelShakes(Char.slot);
        Char.stance = 0;

        if ((Char.direction == -1 && IsSpace(GetCellsInFront(1))) ||
            (Char.direction == 0 && IsSpace(GetBehind()))) {
            distance = GetDist();
            if (Char.direction == 0)
                distance = 32 - distance;
            if (distance <= 35) {
                distance = 35 - distance;
                if (Char.direction == -1)
                    distance = -distance;
                Char.x = AddCharX(distance);
            }
        }
    }
    opponent->saved_sequence = 0;
    Char.image_bank = 8;
    return opponent;
}

void SkelDoImpale(void)
{
    if (DecLifePts(1)) {
        StabKillChar();
        return;
    }
    JumpSeq(126);
    AddSound(72);
}


#define random_below Rnd

int SkelDoneShaking(sbyte id)
{
    int result;
    int reset_timer;
    OPPONENT *opponent = GetOppInfoIdx(Room.number, id);

    if (opponent) {
        result = 0;
        reset_timer = result;
        if (opponent->u.skeleton.shake_delay != -1) {
            if (IsExitChasingSkel(Char.room)) {
                ReadCell(22, 4, 2);
                if (IsExitOpen() && Kid.room == Char.room &&
                    Kid.x < Char.x) {
                    if (!SndOn(10106))
                        AddSong(106);
                    result = 1;
                } else {
                    reset_timer = opponent->u.skeleton.timer == 0 && Char.sequence == 112;
                }
            } else if (opponent->u.skeleton.shakes != 0) {
                --opponent->u.skeleton.shakes;
                reset_timer = 1;
            } else {
                result = 1;
            }
        }
        if (reset_timer)
            SetSkelTimer(id, random_below(30) + 15);
    }
    return result;
}


void SkelFaceKid(void)
{
    if (Kid.x > Char.x) {
        if (Char.direction == 0)
            return;
        Char.direction = 0;
    } else {
        if (Char.direction == -1)
            return;
        Char.direction = -1;
    }
    Char.x = AddCharX(10);
}

void SetupFlameSword(void)
{
    ATTACHFRAME far *table = h2p(SwordFrames);
    AddFCharX(table[Frame.equipment_image].x);
    FChar.y += table[Frame.equipment_image].y;

    if (FChar.image == 4 || FChar.image == 19 || FChar.image == 20) {
        if (Char.flame_offset != 0)
            FChar.image = 18 - Char.flame_offset;
        if (Char.flame_offset <= -4) {
            Char.flame_offset = 0;
            return;
        }
        --Char.flame_offset;
    }
}


int FlameSwordBlocked(void)
{
    int sequence = FlameSwordDecLifePts(1) ? -1 : 103;
    AddSound(12);
    ResetCharY();
    return sequence;
}

int FlameSwordDecLifePts(sbyte points)
{
    int result;
    if (DecLifePts(points)) {
        FlameSwordFall();
        result = 1;
    } else {
        result = 0;
    }
    return result;
}

void FlameSwordFall(void)
{
    JumpSeq(107);
    SetSkelTimer(Char.slot, 480);
    AddSong(192);
    Char.alive = 0;
    Char.repeat = 1;
}


typedef void (*SmokeDrawer)(void);

#define smoke_table_handle SwordFrames

void FlameSwordFloatY(void)
{
    int row_y;
    int amount, drift, dy;

    if (FChar.image == 4 || FChar.image == 19 || FChar.image == 20) {
        amount = 4;
        if ((row_y = Char.row * 63) + 54 >= Char.y) {
            drift = amount = 1;
        } else {
            drift = -2;
            if (Char.row * 63 + 58 <= Char.y)
                amount = 1;
        }

        dy = random_below(amount) + drift;
        Char.y += dy;
        FChar.y += dy;
        OffsetRect(&FChar.clip, &FChar.clip, 0, dy);
    }
}

void HasJinneeRunAway(void)
{
    if (Char.column < -1 || Char.column > 10)
        RemoveChar();
}

void SetupJinneeSmoke(void)
{
    ATTACHFRAME far *table;
    word frame;

    if (Frame.equipment_image >= 171 && Frame.equipment_image <= 199) {
        frame = Frame.equipment_image;
        table = h2p(smoke_table_handle);
        if (table[frame].image != -1) {
            FChar.image = table[frame].image;
            AddFCharX(table[frame].x);
            FChar.y += table[frame].y;
            FChar.image_set = 3;
            MakeFCharClipRect();
            AddCharObj(9);
            MarkBufsForRect(MarkFRedraw, &FChar.clip, Char.slot);
        }
    }
}

void TriggerJinnee(void)
{
    OPPONENT *opponent;
    sbyte column;
    sbyte row;
    int x;

    Kid = Char;
    x = 85 * Plus1[Kid.direction + 1] + Kid.x;
    column = GetCol(x);
    row = Kid.row;
    opponent = GetOppInfoIdx(Kid.room,
                            Char.slot = Level.room_objects[Kid.room - 1].opponent_count++);
    Char.character_kind = 12;
    opponent->kind = 9;
    Char.room = Kid.room;
    Char.row = row;
    opponent->cell = FirstInRow(row) + column;
    ResetCharY();
    Char.x = opponent->x = x;
    Char.column = column;
    Char.direction = opponent->direction = -1;
    column = 0;
    opponent->behavior = column;
    Char.behavior = column;
    opponent->behavior_type = column;
    Char.opponent_index = column;
    Char.alert_delay = column;
    Char.previous_frame = 0;
    Char.sequence_group = Char.previous_y = Char.previous_x = 0;
    Char.image_bank = 4;
    Char.stance = Char.repeat = 1;
    opponent->action = 180;
    opponent->saved_sequence = 0;
    JumpSeq(180);
    AddSong(196);
    opponent->max_life = 6;
    SetOppLifePts(opponent);
    opponent->skill = 10;
    Char.alive = -1;
    AlertGuard = 0;
    JustBlocked = 0;
    Refract = 0;
    ClearCharVel();
    Char.action = 1;
    CheckLoadedOppSet(opponent->kind);
    SaveShad();
    LoadKid();
}
