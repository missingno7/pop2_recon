/* AUTOGUAR.C. Opening routines in original order. */
#include "types.h"
#include "level.h"
#include "char.h"
#include "game.h"

extern signed char Plus1[2];

extern JoyState Joy;

extern Directions Dir;

void far pascal OppAlert(void);
void far pascal AutoBack(void);
void far pascal AutoDown(void);
void far pascal AutoEnGarde(void);
void far pascal AutoFwd(void);
void far pascal AutoRunAway(void);
void far pascal AutoUp(void);
int far pascal FlameSwordBlocked(void);
void far pascal OppChasing(void);
void far pascal InRange(int distance);
void far pascal FollowKid(void);
void far pascal HasJinneeRunAway(void);
void far pascal CheckMultipleDeadBodies(void);
int far pascal DoOppTumbleSeq(void);
void far pascal FlameSwordFloatY(void);
void far pascal EnGarde(void);

void far pascal unused_guard_entry(void)
{
}

void far pascal AutoFwd(void)
{
    Dir.forward = Joy.x = -1;
}

void far pascal AutoBack(void)
{
    Dir.back = -1;
    Joy.x = 1;
}

void far pascal AutoUp(void)
{
    Dir.up = Joy.y = -1;
}

void far pascal AutoDown(void)
{
    Dir.down = -1;
    Joy.y = 1;
}

void far pascal AutoRunAway(void)
{
    Dir.down = -1;
    AutoBack();
}

void far pascal AutoEnGarde(void)
{
    Dir.down = -1;
    AutoFwd();
}

void far pascal AutoPress(unsigned char value)
{
    Joy.button = value;
    Dir.action = value;
}

extern void far pascal CheckLoadedOppSet(signed char kind);
extern CHARREC Opp, Kid, Char;

extern LEVELREC Level;
extern void far pascal JumpSeq(int sequence);
extern void far pascal AnimChar(void);
int far pascal GetOppDeadSeq(unsigned char kind);

extern void far pascal OFFSETRECT(void far *destination, void far *source, int dx, int dy);
void far pascal CheckMultipleDeadBodies(void)
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

int far pascal GetOppDeadSeq(unsigned char kind)
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

void far pascal DoOppStartRun(void)
{
    JumpSeq(Char.character_kind == 4 ? 102 : 84);
    Char.stance = 0;
    Char.alert_delay = 0;
}

extern ROOMREC Room;
extern CELLREC Cell;
extern CHARREC RoomOpp[5];
extern int DroppedOut;

extern signed char far pascal GetBehind(void);
extern signed char far pascal GetUnderFt(void);
extern signed char far pascal GetCellsInFront(signed char distance);
extern int far pascal IsSpace(signed char tile);
extern int far pascal IsImpassable(signed char tile);
extern int far pascal GetBarrCode(signed char tile);
extern signed char far pascal ReadCell(unsigned char room, signed char column, signed char row);
extern int far pascal IsOppGenPtWithTumbleOn(unsigned char room);
extern int far pascal Rnd(int limit);
extern void far pascal LoadShad(signed char id);
extern int far pascal IsDeadPos(int frame);
extern void far pascal LoadKid(void);
signed char far pascal GetTotalDeadOpps(void);

int far pascal DoOppTumbleSeq(void)
{
    signed char tile, live, count, index;
    int result;
    CHARREC saved;

    if (Level.environment_kind == 5)
        result = Char.room == 19 || Char.room == 16 ||
            (IsSpace(GetBehind()) && Char.direction == -1);
    else
        result = 0;

    if (!result && IsOppGenPtWithTumbleOn(Char.room)) {
        live = GetTotalDeadOpps();
        result = (signed char)Rnd(3) <= live;
        if (!result && live != 0) {
            saved = Char;
            count = Level.room_objects[Room.number - 1].opponent_count;
            index = 0;
            if (index < count) {
                do {
                    if (result)
                        break;
                    if (index != saved.slot) {
                        LoadShad(index);
                        if (Char.row == saved.row &&
                            Char.column == saved.column &&
                            IsDeadPos(Char.frame))
                            result = 1;
                    }
                } while (++index < count);
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

void far pascal FollowKid(void)
{
    signed char tile;
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

signed char far pascal GetTotalDeadOpps(void)
{
    signed char index, limit, live;

    live = 0;
    limit = Level.room_objects[Room.number - 1].opponent_count;
    index = 0;
    if (index < limit) {
        do {
            if (RoomOpp[index].alive >= 0)
                ++live;
        } while (++index < limit);
    }
    return live;
}

extern int Refract, AlertGuard;
extern void far pascal GenFight(int distance);
extern void far _fastcall GetCharStrikeRanges(int near *, int near *);
extern int far pascal IsOKForBrgSkelToAdvance(void);
extern int far pascal GetOppDist(void);

void far pascal InRange(int distance)
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

void far pascal OppAlert(void)
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

#pragma pack(1)
typedef struct {
    signed char cell;
    int x;
    signed char facing;
    unsigned char state;
    int sequence;
    int position;
    unsigned char reserved[4];
    signed char motion, secondary_motion;
    unsigned char kind, health;
    int shake_delay;
    int timer;
    signed char row;
    unsigned char shakes;
} Opponent;
#pragma pack()

extern signed char far pascal GetOppIdxClosestToKid(signed char facing);
extern Opponent near *far pascal GetOppInfoIdx(int room, int id);
extern int far pascal IsAdvancableId(signed char tile, int modifier);
extern int far pascal GetDist(void);
extern int far pascal AddCharX(int amount);
extern int far pascal IsExitChasingSkel(unsigned char room);
void far pascal DoOppRunJump(void);

extern int abs(int value);
#pragma intrinsic(abs)

void far pascal OppChasing(void)
{
    signed char tile;
    Opponent near *opponent;
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
    if ((Kid.row == Char.row || opponent->row == Kid.row) &&
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
        if ((Kid.row == Char.row || opponent->row == Kid.row) &&
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

extern int far pascal DoRunJump(int id);

void far pascal DoOppRunJump(void)
{
    signed char tile1, tile2, start, steps_ahead;
    Opponent near *opponent;
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
            attempt = opponent && opponent->row != Char.row;
        }

        selected = attempt ? DoRunJump(100) : 0;
        if (!selected) {
            JumpSeq(101);
            Char.alert_delay = 2;
        }
    }
}

extern void far pascal NoJump(void);
extern void far pascal WaitingEngarde(void);
extern int far pascal BrgSkelMustAdvance(void);
extern int Refract;

void far pascal EnGarde(void)
{
    signed char tile;
    int upper, lower;
    unsigned int frame = Char.frame;
    unsigned int pose;
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

extern int far pascal GetDistFromClosestOpp(void);

extern void far pascal MaybeAdvance(void);
#define RUNNING_FRAME(frame) \
    (((frame) >= 1 && (frame) <= 14) || \
     ((frame) >= 49 && (frame) <= 56) || \
     ((frame) >= 34 && (frame) <= 44))

extern int far pascal IsExitOpen(void);
static const unsigned int strike_chance[12] =
    {75, 100, 75, 75, 75, 50, 100, 220, 0, 60, 40, 60};
static const unsigned int quick_strike_chance[12] =
    {0, 0, 0, 5, 5, 175, 20, 10, 0, 255, 255, 150};
extern int far pascal IsWall(signed char tile);
extern int far pascal GateBarr(void);
extern void far pascal SaveShad(void);
extern void far pascal SetOppLifePts(Opponent near *opponent);
extern void far pascal ClearCharVel(void);
extern int far pascal SndOn(int sequence);
extern void far pascal AddSong(int request);

void far pascal WaitingEngarde(void)
{
    signed char slot, tile;
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
            (tile != 4 || (unsigned char)Cell.modifier >= 112))
            return;
    } } while (0);
    if (IsAdvancableId(GetBehind(), Cell.modifier))
        AutoBack();
    return;
}

int far pascal GetDistFromClosestOpp(void)
{
    int spacing = 96;
    signed char index, count;

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

void far pascal NoJump(void)
{
    signed char front, further;

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

static const unsigned int block_chance[12] =
    {0, 150, 150, 200, 200, 255, 200, 250, 0, 255, 255, 255};
static const unsigned int quick_block_chance[12] =
    {0, 75, 75, 100, 100, 145, 100, 250, 0, 145, 255, 175};
static const unsigned int advance_chance[12] =
    {255, 200, 200, 200, 255, 255, 200, 0, 0, 255, 100, 100};
extern int JustBlocked, Refract, GuardTimer;
void far pascal MaybeBlock(void);
void far pascal MaybeStrike(void);

void far pascal GenFight(int distance)
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

void far pascal MaybeAdvance(void)
{
    unsigned char skill;

    if (Char.row && Level.level_number == 5 &&
        (Char.room == 10 || Char.room == 7 || Char.room == 12) &&
        BrgSkelMustAdvance()) {
        AutoFwd();
        return;
    }
    if ((skill = Level.room_objects[Char.room - 1].opponent_data[Char.slot][4]) && GuardTimer)
        return;
    if (Char.row && Level.level_number == 5 &&
        (Char.room == 10 || Char.room == 7 || Char.room == 12) &&
        !IsOKForBrgSkelToAdvance())
        return;

    if (advance_chance[skill] > Rnd(255))
        AutoFwd();
}

void far pascal MaybeBlock(void)
{
    unsigned char skill;

    if (Char.character_kind == 10 ||
        (Opp.frame != 152 && Opp.frame != 153 && Opp.frame != 162))
        return;
    skill = Level.room_objects[Char.room - 1].opponent_data[Char.slot][4];

    if (JustBlocked) {
        if (quick_block_chance[skill] > Rnd(255))
            AutoUp();
    } else if (block_chance[skill] > Rnd(255))
        AutoUp();
}

void far pascal MaybeStrike(void)
{
    unsigned char skill;

    if (Opp.frame == 169 || Opp.frame == 151)
        return;
    skill = Level.room_objects[Char.room - 1].opponent_data[Char.slot][4];
    if (Char.character_kind != 7 && Char.character_kind != 8 &&
        (Char.frame == 161 || Char.frame == 150)) {
        if (quick_strike_chance[skill] > Rnd(255))
            AutoPress(254);
    } else if (strike_chance[skill] > Rnd(255))
        AutoPress(254);
}

int far pascal SkelDoneShaking(signed char id);
void far pascal BonesFall(void);
void far pascal SkelFaceKid(void);

int far pascal OpponentClose(signed char slot, signed char column, signed char row, signed char radius)
{
    signed char tile, limit, step;
    int clear = 0;
    Opponent near *opponent = GetOppInfoIdx(Room.number, slot);

    if (opponent && Char.alive >= 0 && Char.sequence_group != 5 && !Kid.character_kind &&
        (!opponent->timer || IsExitChasingSkel(Char.room))) {
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

extern void far pascal RemoveChar(void);
extern void far pascal MarkFRedraw(void);
extern unsigned int SwordFrames;
extern int far pascal BrgSkelProg(void);
extern void far pascal GuardProg(void);

void far pascal OpponentRise(void)
{
    signed char index, count, radius;
    int started;
    Opponent near *opponent;

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
                    if (opponent->shake_delay != -1) {
                        started = opponent->timer; if (SkelDoneShaking(Char.slot)) {
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
                    Char.alive = (unsigned char)-1;
                    Char.stance = 1;
                    Char.alert_delay = 3;
                    if (Char.character_kind == 10) {
                        signed char facing;
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

void far pascal SkelProg(void)
{
    int active;
    Opponent near *opponent;

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
        if (opponent && opponent->timer > 0)
            --opponent->timer;
    }
}

extern Opponent near *far pascal GetOppInfoIdx(int room, int id);
extern void far pascal LoadShad(signed char id);
extern void far pascal JumpSeq(int sequence);
extern void far pascal SaveShad(void);

extern int far pascal IsSpace(signed char tile);
extern int far pascal GetDist(void);
extern int far pascal AddCharX(int amount);
extern void far pascal AddSound(int id);
extern void far pascal DrawOppMeter(signed char id, int flags, int health);
extern int far pascal DecLifePts(signed char amount);
extern void far pascal StabKillChar(void);
void far pascal SetSkelTimer(signed char id, int duration);
void far pascal SetSkelShakes(signed char id);

void far pascal BonesFall(void)
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

int far pascal IsExitChasingSkel(unsigned char value)
{
    int result = 0;

    if (Level.level_number == 4 && value >= 22 && value <= 28 && value != 23) {
        result = 1;
    }
    return result;
}

void far pascal SetSkelShakes(signed char id)
{
    Opponent near *opponent = GetOppInfoIdx(Room.number, id);
    if (opponent) {
        if (opponent->state)
            opponent->shakes = (opponent->state + 1) % 2;
        else
            opponent->shakes = 3;
    }
}

void far pascal SetSkelTimer(signed char id, int duration)
{
    Opponent near *opponent;
    if (Level.preferred_opponent == 2 || RoomOpp[0].character_kind == 10) {
        opponent = GetOppInfoIdx(Room.number, id);
        if (opponent)
            opponent->timer = duration;
    }
}

void far pascal ShakeBones(unsigned char room, signed char row)
{
    signed char index, count;
    CHARREC saved;
    Opponent near *opponent;

    saved = Char;
    index = 0;
    count = Level.room_objects[room - 1].opponent_count;
    if (count > 0) {
        do {
            LoadShad(index);
            if (Char.row == row && Char.frame == 206 && Char.sequence_group != 5) {
                opponent = GetOppInfoIdx(Char.room, Char.slot);
                if (opponent->shake_delay == 0) {
                    JumpSeq(112);
                    SaveShad();
                }
            }
        } while (++index < count);
    }
    Char = saved;
}

extern int far pascal IsSpace(signed char tile);
extern int far pascal GetDist(void);
extern int far pascal AddCharX(int amount);
extern void far pascal AddSound(int id);

Opponent near *far pascal SkelAddOpponent(Opponent near *opponent)
{
    int distance;

    if (opponent->shake_delay == 1) {
        opponent->sequence = 63;
        Char.stance = 1;
    } else {
        if (Char.slot == 0 && opponent->shake_delay == 0)
            opponent->sequence = 112;
        else
            opponent->sequence = 119;
        opponent->timer = opponent->shake_delay;
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
    opponent->position = 0;
    Char.image_bank = 8;
    return opponent;
}

void far pascal SkelDoImpale(void)
{
    if (DecLifePts(1)) {
        StabKillChar();
        return;
    }
    JumpSeq(126);
    AddSound(72);
}

extern signed char far pascal ReadCell(unsigned char room, signed char column, signed char row);
#define random_below Rnd

int far pascal SkelDoneShaking(signed char id)
{
    int result;
    int reset_timer;
    Opponent near *opponent = GetOppInfoIdx(Room.number, id);

    if (opponent) {
        result = 0;
        reset_timer = result;
        if (opponent->shake_delay != -1) {
            if (IsExitChasingSkel(Char.room)) {
                ReadCell(22, 4, 2);
                if (IsExitOpen() && Kid.room == Char.room &&
                    Kid.x < Char.x) {
                    if (!SndOn(10106))
                        AddSong(106);
                    result = 1;
                } else {
                    reset_timer = opponent->timer == 0 && Char.sequence == 112;
                }
            } else if (opponent->shakes != 0) {
                --opponent->shakes;
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

typedef struct {
    int frame;
    signed char x;
    signed char y;
} SpriteFrame;

extern FRAMEREC Frame;
extern FCHARREC FChar;
extern int far pascal AddCharX(int delta);
extern int far pascal AddFCharX(int delta);
extern SpriteFrame far *far pascal h2p(unsigned int handle);

void far pascal SkelFaceKid(void)
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

void far pascal SetupFlameSword(void)
{
    SpriteFrame far *table = h2p(SwordFrames);
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

extern void far pascal AddSound(int sound);
extern void far pascal ResetCharY(void);
extern int far pascal DecLifePts(signed char points);
extern void far pascal JumpSeq(int sequence);
extern void far pascal SetSkelTimer(signed char id, int duration);
extern void far pascal AddSong(int effect);
int far pascal FlameSwordDecLifePts(signed char points);
void far pascal FlameSwordFall(void);

int far pascal FlameSwordBlocked(void)
{
    int sequence = FlameSwordDecLifePts(1) ? -1 : 103;
    AddSound(12);
    ResetCharY();
    return sequence;
}

int far pascal FlameSwordDecLifePts(signed char points)
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

void far pascal FlameSwordFall(void)
{
    JumpSeq(107);
    SetSkelTimer(Char.slot, 480);
    AddSong(192);
    Char.alive = 0;
    Char.repeat = 1;
}

extern int AlertGuard, JustBlocked, Refract;

extern signed char far pascal GetCol(int x);
extern signed char far pascal FirstInRow(signed char row);
extern void far pascal ResetCharY(void);
extern void far pascal JumpSeq(int sequence);
extern void far pascal AddSong(int effect);
extern void far pascal SetOppLifePts(Opponent near *opponent);
extern void far pascal ClearCharVel(void);
extern void far pascal SaveShad(void);

extern SpriteFrame far *far pascal h2p(unsigned int handle);
extern int far pascal AddFCharX(int x);
extern void far pascal MakeFCharClipRect(void);
extern void far _fastcall AddCharObj(unsigned char style);
typedef void (far pascal *SmokeDrawer)(void);
extern void far pascal MarkBufsForRect(SmokeDrawer draw, void near *clip, signed char id);
#define smoke_table_handle SwordFrames

void far pascal FlameSwordFloatY(void)
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
        OFFSETRECT(&FChar.clip.top, &FChar.clip.top, 0, dy);
    }
}

void far pascal HasJinneeRunAway(void)
{
    if (Char.column < -1 || Char.column > 10)
        RemoveChar();
}

void far pascal SetupJinneeSmoke(void)
{
    SpriteFrame far *table;
    unsigned int frame;

    if (Frame.equipment_image >= 171 && Frame.equipment_image <= 199) {
        frame = Frame.equipment_image;
        table = h2p(smoke_table_handle);
        if (table[frame].frame != -1) {
            FChar.image = table[frame].frame;
            AddFCharX(table[frame].x);
            FChar.y += table[frame].y;
            FChar.image_set = 3;
            MakeFCharClipRect();
            AddCharObj(9);
            MarkBufsForRect(MarkFRedraw, &FChar.clip.top, Char.slot);
        }
    }
}

void far pascal TriggerJinnee(void)
{
    Opponent near *opponent;
    signed char column;
    signed char row;
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
    Char.direction = opponent->facing = -1;
    column = 0;
    opponent->motion = column;
    Char.behavior = column;
    opponent->secondary_motion = column;
    Char.opponent_index = column;
    Char.alert_delay = column;
    Char.previous_frame = 0;
    Char.sequence_group = Char.previous_y = Char.previous_x = 0;
    Char.image_bank = 4;
    Char.stance = Char.repeat = 1;
    opponent->sequence = 180;
    opponent->position = 0;
    JumpSeq(180);
    AddSong(196);
    opponent->health = 6;
    SetOppLifePts(opponent);
    opponent->state = 10;
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
