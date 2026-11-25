#include <string.h>
#include "types.h"
#include "resc.h"
#include "snd.h"
#include "graf.h"
#include "shape.h"
#include "level.h"
#include "heap.h"
#include "char.h"
#include "game.h"
#include "draw.h"
#include "custom.h"
#include "dlghelp.h"
#include "act.h"
#include "add.h"
#include "auto.h"
#include "movescyt.h"
#include "autoguar.h"
#include "autoshad.h"
#include "ctrl.h"
#include "fade.h"
#include "drawcave.h"
#include "desert.h"
#include "final.h"
#include "gfxerr.h"
#include "intrface.h"
#include "misc.h"
#include "move.h"
#include "recorder.h"
#include "moveruin.h"
#include "movetemp.h"
#include "prince.h"
#include "rooftops.h"
#include "sound.h"
#include "tables.h"
#include "throne.h"
#include "top.h"
#include "util.h"


int InDoStepFwd = 0;
word Handler(void)
{
    struct {
        byte room;
        sbyte fix_x;
    } local;

    local.fix_x = 0;
    local.room = Cell.room;

    if (Cell.column < 0) {
        local.fix_x = 10;
        if (local.room) {
            local.room = Level.room_links[local.room - 1].left;
            if (local.room) {
                Cell.column += 10;
                local.fix_x = 0;
                Cell.room = local.room;
                Handler();
            }
        }
    } else if (Cell.column >= 10) {
        local.fix_x = -10;
        if (Cell.room) {
            local.room = Level.room_links[Cell.room - 1].right;
            if (local.room) {
                Cell.column -= 10;
                local.fix_x = 0;
                Cell.room = local.room;
                Handler();
            }
        }
    }

    if (Cell.room) {
        if (Cell.row < 0) {
            Cell.row += 3;
            local.room = Level.room_links[Cell.room - 1].above;
            Cell.room = local.room;
            Handler();
        } else if (Cell.row >= 3) {
            Cell.row -= 3;
            local.room = Level.room_links[Cell.room - 1].below;
            Cell.room = local.room;
            Handler();
        }
    }

    if (!local.room && Cell.room)
        Cell.room = local.room;
    if (Cell.column < 0 || Cell.column >= 10)
        Cell.column += local.fix_x;
    return Cell.room;
}

byte ReadCell(byte room, sbyte column, sbyte row)
{
    Cell.room = room;
    Cell.column = column;
    Cell.row = row;
    Cell.room = Handler();
    Cell.index = FirstInRow(Cell.row) + Cell.column;
    if (Cell.room) {
        select_room_arrays(Cell.room);
        Cell.kind = BlueT[Cell.index];
        Cell.modifier = BlueS[Cell.index].modifier;
    } else {
        Cell.modifier = 0;
        Cell.kind = GetUndefineCellId(column, row);
    }
    return Cell.kind;
}

sbyte GetUndefineCellId(sbyte x, sbyte y)
{
    sbyte result;
    switch (Level.environment_kind) {
    case 2:
        if ((Room.number == 4 && Level.level_number == 13) ||
            (Level.level_number == 13 && Room.number == 13))
            result = 0;
        else
            result = 20;
        break;
    case 4:
        if (!Room.above && y == -1) {
            result = 0;
            break;
        }
        if ((Room.number == 27 && Level.level_number == 6) ||
            (Room.number == 16 && Level.level_number == 9)) {
            result = 0;
            break;
        }
        result = 20;
        break;
    case 5:
        result = 0;
        break;
    case 6:
        if (((Room.number == 7 || Room.number == 8) && Level.environment_kind == 6) ||
            (Room.number == 3 && Level.environment_kind == 6 && (x < 0 || x == 9) && y == 2))
            result = 20;
        else
            result = 0;
        break;
    default:
        result = 20;
        break;
    }
    return result;
}

sbyte IndexCell(sbyte column, sbyte row)
{
    sbyte index;

    if (row < 3 && row >= -1 && column < 10 && column >= 0) {
        if (row == -1)
            index = -1 - column;
        else
            index = FirstInRow(row) + column;
    } else {
        index = 30;
    }
    return index;
}

byte IndexCell1(sbyte column, byte row)
{
    sbyte index = IndexCell(column, row);
    sbyte result;

    if (index >= 0)
        result = index;
    else
        result = 30;
    return result;
}


void RereadCells(void)
{
    GetFrameInfo();
    GetBaseCell();
}

void GetBaseCell(void)
{
    Char.column = GetCharCol(GetBaseX());
}

void GetFrameInfo(void)
{
    word adjust = 0;
    FRAMEREC far *frames = h2p(hOppFrames);

    switch (Char.character_kind) {
    case 0:
    case 1:
    case 6:
        Frame = KidFrames[Char.frame];
        break;
    case 2:
    case 4:
    case 10:
    case 12:
        if (Char.frame >= 102 && Char.frame < 107)
            adjust = 70;
        Frame = frames[Char.frame + adjust - 149];
        break;
    case 8:
        if (Char.frame < 183)
            adjust = 43;
        /* Fall through. */
    case 7:
    case 11:
        Frame = frames[Char.frame + adjust - 149];
        break;
    }
}

#define char_direction ((byte)Char.direction)
#define char_x (Char.x)
#define character_row (Char.row)
#define character_room (Char.room)

int AddCharX(int value)
{
    if (char_direction)
        value = -value;
    return value + char_x;
}


int AddFCharX(int value)
{
    if (FChar.direction)
        value = -value;
    FChar.x = FChar.x + value;
    return FChar.x;
}


static int animation_portion;
static int sound_steps;
static FCHARREC saved_fchar;


void AnimChar(void)
{
    int previous_position = Char.sequence_position;
    int reading = 1;
    Handle sequence = hGetSequence(Char.sequence_id);
    int opcode;
    int first, second;
    word status;

    if (!sequence)
        return;

    status = hGetState(sequence);
    hLock(sequence);

    do {
        opcode = GetSeqOp(sequence);
        switch (opcode) {
        case -24:
            if (GetSeqOp(sequence))
                SetPaletteToColorIdx(GetSeqOp(sequence));
            else
                RestoreGamePlayPalette();
            break;
        case -23:
            Char.sequence_position = previous_position - 1 >= 0 ? previous_position - 1 : 0;
            break;
        case -22:
            opcode = GetSeqOp(sequence);
            first = GetSeqOp(sequence);
            second = GetSeqOp(sequence);
            if (Rnd(opcode))
                JumpSeq(first);
            else
                JumpSeq(second);
            break;
        case -21:
            Char.sequence_group = GetSeqOp(sequence);
            break;
        case -19:
            ResetCharY();
            break;
        case -18:
            Char.sequence = 0;
            break;
        case -17:
            RemoveChar();
            reading = 0;
            break;
        case -16:
            ++NextLevel;
            if (Level.level_number == 6)
                KidInfo.entrance_mode = Char.column >= 6 ? 2 : 1;
            break;
        case -15:
            AnimCharTap(GetSeqOp(sequence));
            break;
        case -14:
            opcode = GetSeqOp(sequence);
            if (opcode == 1 || (opcode == 2 && animation_portion == 6)) {
                PotionEffect();
                if (!Char.sequence_position) {
                    hSetState(sequence, status);
                    sequence = hGetSequence(Char.sequence_id);
                    status = hGetState(sequence);
                    hLock(sequence);
                    break;
                }
            } else if (opcode == 3) {
                TriggerJinnee();
            }
            break;
        case -13:
            ShakeFloor = -1;
            break;
        case -12:
            ShakeFloor = 1;
            break;
        case -11:
            SetKidDeathMethod(GetSeqOp(sequence));
            break;
        case -9:
            opcode = GetSeqOp(sequence);
            first = GetSeqOp(sequence);
            if (opcode && Char.horizontal_step < 16 &&
                (!Char.vertical_step || Char.vertical_step < 32)) {
                Char.horizontal_step = Char.horizontal_step + opcode > 16
                    ? 16 : Char.horizontal_step + opcode;
            }
            if (first && Char.vertical_step < 32) {
                Char.vertical_step = Char.vertical_step + first > 32
                    ? 32 : Char.vertical_step + first;
            }
            break;
        case -8:
            opcode = GetSeqOp(sequence);
            if (opcode != 10000)
                Char.horizontal_step = opcode;
            opcode = GetSeqOp(sequence);
            if (opcode != 10000)
                Char.vertical_step = opcode;
            break;
        case -7: Char.action = GetSeqOp(sequence); break;
        case -6: Char.y += GetSeqOp(sequence); break;
        case -5: char_x = AddCharX(GetSeqOp(sequence)); break;
        case -4: ++character_row; break;
        case -3: --character_row; break;
        case -2:
            if (Char.character_kind == 10)
                char_x = AddCharX(25);
            Char.direction = ~Char.direction;
            break;
        case -10:
            if (!WeightlessTimer) {
                GetSeqOp(sequence);
                break;
            }
            /* fall through */
        case -1:
            opcode = GetSeqOp(sequence);
            if (character_row != 0 && Level.level_number == 5 &&
                (character_room == 10 || character_room == 7 || character_room == 12) &&
                opcode == 57 && Char.sequence != 60 && Char.column == 9)
                opcode = 86;
            Char.sequence_id = opcode;
            Char.sequence_position = 0;
            hSetState(sequence, status);
            sequence = hGetSequence(opcode);
            if (!sequence)
                break;
            status = hGetState(sequence);
            hLock(sequence);
            break;
        default:
            Char.frame = opcode;
            reading = 0;
            break;
        }
    } while (reading);

    hSetState(sequence, status);
}

int _fastcall GetSeqOp(Handle sequence)
{
    int opcode;
    int far *entries;

    if (sequence) {
        entries = h2p(sequence);
        opcode = entries[Char.sequence_position]; ++Char.sequence_position;
    }
    return opcode;
}

Handle _fastcall hGetSequence(word id)
{
    return hGetRsrc(0x53455153UL, id);
}

void AnimCharTap(int sound)
{
    switch (sound) {
    case 1:
        ++sound_steps;
        if (Level.environment_kind != 1) {
            AddSound(sound_steps % 2 + 23);
            if (Char.character_kind == 4 && IsExitChasingSkel(character_room))
                char_x = AddCharX(Char.slot - 1);
        }
        /* Fall through. */
    case 0:
        if (!Char.character_kind)
            AlertGuard = 1;
        break;
    default:
        if (sound <= 273) {
            if (sound == 271)
                AddSong(sound);
            else
                AddSound(sound);
        }
        break;
    }
}

void SetKidDeathMethod(byte value)
{
    if (Char.character_kind == 0 || Char.character_kind == 1)
        KidInfo.argument = value;
}

int ResetCharY(void)
{
    Char.y = 63 * character_row + 56;
    if (Char.character_kind == 7 || Char.character_kind == 8)
        Char.y -= 19;
    return Char.y;
}

sbyte FirstInRow(sbyte value)
{
    sbyte result;

    if (value >= 0)
        result = value * 10;
    else
        result = value * 10 + 9;

    return result;
}


void LoadKidwOpp(sbyte slot)
{
    if (slot >= 0 && slot < 5) {
        LoadKid();
        Opp = RoomOpp[slot];
    }
}

void SaveKidwOpp(void)
{
    if (Opp.slot >= 0 && Opp.slot < 5) {
        Kid = Char;
        RoomOpp[Opp.slot] = Opp;
    }
}

void LoadShadwOpp(sbyte slot)
{
    LoadShad(slot);
    Opp = Kid;
}

void SaveShadwOpp(void)
{
    if (Char.slot >= 0 && Char.slot < 5)
        SaveShad();
    Kid = Opp;
}

/* init_fchar_rect (descriptive name; FM Towns inlines it in SetupCarpet, the Mac body is unlabelled):
   copy the play-area rectangle into the 8-byte rectangle stored at byte 9 of the FChar record.
   GamePlayRect (DS:09fe) is the FM Towns global name anchored by matching copies. */
void init_fchar_rect(void)
{
    FChar.clip = GamePlayRect;
}

void ConvertX(int value, int *value_out) {
    if (value < 0)
        --value;
    *value_out = value;
}


void Gravity(void)
{
    if (Char.action == 4 || Char.action == 9) {
        if (WeightlessTimer) {
            ++Char.vertical_step;
            if (Char.vertical_step > 4)
                Char.vertical_step = 4;
        } else {
            Char.vertical_step += 3;
            if (Char.vertical_step > 33)
                Char.vertical_step = 33;
        }
    }
}

void AddCharVel(void)
{
    if (Char.y < 1920) {
        Char.y += Char.vertical_step;
        if (Char.action == 4 || Char.character_kind == 7 || Char.character_kind == 8) {
            char_x = AddCharX(Char.horizontal_step);
            RereadCells();
        }
    }
}


void ClearCharVel(void)
{
    Char.horizontal_step = 0;
    Char.vertical_step = 0;
}

int DecLifePts(sbyte amount)
{
    int lethal = 0;

    if (!(Char.character_kind == 4 && character_row != 0 && Level.level_number == 5 &&
          (character_room == 10 || character_room == 7 || character_room == 12))) {
        if (amount >= Char.life_points) {
            if (-Char.life_points < Char.life_delta)
                Char.life_delta = -Char.life_points;
            lethal = 1;
        } else if (-Char.life_delta < amount) {
            Char.life_delta = -amount;
        } else {
            lethal = Char.life_points + Char.life_delta == 0;
        }
    }
    return lethal;
}

sbyte GetUnderFt(void)
{
    return ReadCell(character_room, Char.column, character_row);
}


int ImHeight, ImWidth, CDLeftEdge, CDRightEdge, LeftEdge, RightEdge, TopEdge, ExitEdge;
Directions Dir, SelDir;


void OnGround(void)
{
    sbyte kind;

    if ((Frame.flags & 0x40) && Char.character_kind != 10) {
        kind = GetUnderFt();
        if (IsWall(kind)) {
            if (kind != 7 || ((Char.frame < 246 || Char.frame > 261) &&
                             Char.character_kind != 11))
                kind = InsideCell();
        }
        if (IsSpace(kind) ||
            (kind == 30 && (Cell.modifier & 15) < 2)) {
            if (Level.environment_kind == 5 && RoofClimbingIntoShip())
                return;
            if (Level.environment_kind == 2 && SlideRockInCell(16))
                return;
            if (Char.character_kind == 11)
                char_x = AddCharX(GetDist() - 42);
            else
                StartFall();
        }
    }
}

void StartFall(void)
{
    word old_frame = Char.frame;
    int adjust_clearance = 0;
    int NeedShadow = -1;
    int sequence;
    int distance;
    struct { int y; } minimum;

    ++character_row;
    if (Char.character_kind == 1)
        CheckShadow();
    if (Char.stance == 1)
        Char.stance = 0;
    if (Char.character_kind == 0 && Level.level_number == 5 && character_room == 10 &&
        ((char_direction == 0 && Dir.forward) ||
         (char_direction == 0xff && Dir.back))) {
        NeedShadow = 198;
        adjust_clearance = 1;
    }

    if (NeedShadow != -1) {
        sequence = NeedShadow;
    } else {
        switch (old_frame) {
        case 9:
            sequence = 7;
            break;
        case 13:
            sequence = 19;
            break;
        case 26:
            sequence = 18;
            adjust_clearance = 1;
            break;
        case 44:
            sequence = 21;
            adjust_clearance = 1;
            break;
        case 81:
            sequence = 7;
            Char.y += 12;
            break;
        default:
            adjust_clearance = 1;
            if (old_frame >= 81 && old_frame < 86) {
                sequence = 19;
            } else if (old_frame >= 150 && old_frame < 180) {
                if (!Char.character_kind) {
                    DroppedOut = adjust_clearance;
                    if (old_frame == 153 || Char.sequence == 56)
                        sequence = 95;
                    else
                        sequence = 81;
                } else {
                    DroppedOut = 0;
                    if (Char.sequence == 86 || Char.sequence == 67 ||
                        Char.sequence == 108)
                        sequence = 83;
                    else
                        sequence = 82;
                }
            } else if (Char.character_kind == 2 && Char.sequence == 100) {
                sequence = 186;
            } else {
                sequence = 7;
            }
            break;
        }
    }

    if (adjust_clearance) {
        if (IsWall(GetCellsInFront(1)) ||
            IsWall(GetBehind()) ||
            !IsSpace(GetAbove()) || !IsSpace(GetAboveInFront()) ||
            !IsSpace(GetAboveBehind())) {
            distance = GetDist();
            if (distance <= 7)
                char_x = AddCharX(distance - 11);
            else if (distance >= 25)
                char_x = AddCharX(distance - 21);
            if (!IsSpace(GetAbove()) || !IsSpace(GetAboveInFront())) {
                minimum.y = 63 * character_row + 1;
                if (minimum.y > Char.y)
                    Char.y = minimum.y;
            }
        }
    }

    if (Level.environment_kind == 1) {
        sequence = 27;
        TriggerAllStonesDown();
        SetKidDeathMethod(8);
    }
    else if (Char.character_kind == 0 &&
             WeightlessTimer && Char.sequence != 228)
    {
        sequence = 228;
    }
    JumpSeq(sequence);
    AnimChar();
    RereadCells();

    if (IsWall(GetUnderFt())) {
        InsideCell();
        return;
    }
    if (IsWall(GetCellsInFront(1))) {
        if (old_frame == 44 && GetDist() < 14) {
            JumpSeq(114);
            AnimChar();
        } else {
            char_x = AddCharX(-2);
        }
        RereadCells();
    }
}


void FallingInUndefinedRoom(void)
{
    Gravity();
    AddCharVel();
    if (Level.environment_kind == 5) {
        if (Char.y > 384) {
            if (Room.number != 19 && Room.number != 16) {
                AddSound(0);
                DecLifePts(100);
                Char.frame = 185;
                SetKidDeathMethod(3);
            }
            Char.y = 384;
            Char.vertical_step = 0;
            Char.action = 1;
            if (Char.character_kind && Kid.opponent_index == Char.slot)
                Kid.opponent_index = GetOppIdxClosestToKid(char_direction);
        } else if (Char.character_kind == 2 && Char.frame != 185 &&
                   !SndOn(0x2729)) {
            AddSound(25);
        }
    } else if (Char.y > 384) {
        KillFallingChar();
    }
}

void KillFallingChar(void)
{
    DecLifePts(100);
    Char.frame = 185;
    SetKidDeathMethod(3);
    Char.y = 384;
    Char.vertical_step = 0;
    Char.action = 1;
    if (Char.character_kind == 1)
        CutToKidRoom();
}

int IsUndefinedRoom(void)
{
    if (character_room == 0 ||
        (((character_room == 3 && Level.environment_kind == 6) ||
          (character_room == 5 && Level.environment_kind == 6)) &&
         character_row >= 11))
        return 1;
    return 0;
}

void FallOn(void)
{
    int position;
    if (Dir.action && Char.vertical_step < 32 && Char.alive < 0 &&
        63 * character_row + 56 <= Char.y + 25) {
        position = char_x;
        char_x = AddCharX(-16);
        RereadCells();
        if (!GrabTest()) {
            char_x = position;
            return;
        }
        position = GetDist();
        position += 4;
        if (char_direction == 0xff)
            position -= 7;
        else
            position -= 9;
        char_x = AddCharX(position);
        ResetCharY();
        Char.vertical_step = 0;
        JumpSeq(15);
        AnimChar();
        AddSound(11);
        Stunned = 12;
        Screamed = 0;
    }
}

int GrabTest(void)
{
    sbyte above = GetAbove();
    word modifier = Cell.modifier;
    sbyte ahead = GetAboveInFront();
    int result = IsLedge(above, ahead, modifier, Cell.modifier);

    if (Level.environment_kind == 5 && !result)
        return RoofGrabShip();
    return result;
}

sbyte InsideCell(void)
{
    int distance;
    sbyte ahead;

    if (Char.sequence_group != 4 && Char.character_kind != 11 &&
        !(character_room == 3 && Level.environment_kind == 6 && Char.column < 0)) {
        distance = GetDist();
        ahead = GetCellsInFront(1);
        if (distance < 11 && !IsWall(ahead))
            distance += 11;
        else
            distance -= 32;
        char_x = AddCharX(distance);
        RereadCells();
    }
    return GetUnderFt();
}

sbyte GetCellsInFront(sbyte distance)
{
    return ReadCell(character_room,
                    Char.column + Plus1[Char.direction + 1] * distance,
                    character_row);
}

sbyte GetBehind(void)
{
    return ReadCell(character_room, Char.column + Minus1[Char.direction + 1],
                    character_row);
}

extern sbyte Offset;

int GetBaseX(void)
{
    return AddCharX((sbyte)(Frame.x - (Frame.flags & 31)));
}

int GetDist(void)
{
    return GetDist1(GetBaseX());
}

int GetDist1(int x)
{
    int distance;
    GetCharCol(x);
    if (!char_direction)
        distance = 31 - Offset;
    else
        distance = Offset;
    return distance;
}

sbyte GetCharCol(int x)
{
    return GetCol(x - 14);
}

sbyte GetCol(int x)
{
    int shifted = x - 4;
    sbyte col;

    Offset = shifted % 32;
    col = shifted / 32 - 4;
    if (x < 0)
        --col;
    return col;
}

sbyte GetRow(int value)
{
    int shifted = value - 3;
    sbyte result = shifted / 63;

    if (shifted <= 0)
        --result;

    return result;
}

sbyte GetCellCol(void)
{
    sbyte col = Cell.column;

    if (Room.number != Cell.room && Room.below != Cell.room && Room.above != Cell.room) {
        if (Room.left == Cell.room || Room.below_left == Cell.room ||
            Room.above_left == Cell.room)
            col -= 10;
        else if (Room.right == Cell.room || Room.below_right == Cell.room ||
                 Room.above_right == Cell.room)
            col += 10;
    }

    return col;
}


JoyState Joy;

int CtrlPlayer(void)
{
    int result;
    result = PlayerCtrl();
    if (result != -2 || Char.alive < 0 || !IsDeadPos(Char.frame))
        return result;

    if (BackToLife) {
        ResetSnd(0);
        Kid.life_delta = Kid.life_initial;
        LoadKid();
        JumpSeq(2);
        char_x += 16;
        AnimChar();
        RereadCells();
        StartKid1();
    }

    if (Char.character_kind != 0 && Char.character_kind != 1)
        return result;
    if (SndOn(current_effect + 10000) &&
        current_effect != 4 && current_effect != 7 &&
        current_effect != 54)
        return result;

    TimeRequest = 0;
    if (Char.alive < 0 || Char.alive >= 6) {
        if (Char.alive == 6) {
            AddKidDeathSong(KidInfo.argument);
        } else {
            if (Char.alive != 7 || SndOn(current_effect + 10000))
                return result;
            if (SndOn(current_song + 10000))
                return result;
            if (!Minutes) {
                YouLose();
                result = -1;
            } else if (MsgTimerStart != 600) {
                MsgTimer = MsgTimerStart = 600;
                DrawRestartMessage();
            }
        }
    }
    ++Char.alive;
    return result;
}

int PlayerCtrl(void)
{
    int result;
    if (Char.alive < 0 && !Char.life_points)
        Char.alive = 0;
    if (Stunned)
        --Stunned;
    LoadSelect();
    Input();
    if (LevelIdx != NextLevel || !character_room || Char.sequence == 59)
        Joy.y = Joy.x = 0;
    ClrJoystick();
    UserCtrl();
    result = CheckForEndOfRecording();
    SaveSelect();
    return result;
}

void ShadCtrl(void)
{
    if (Char.alive >= 0) {
        if (Char.alive >= 6) {
            AddOppDeathSong();
            if (Kid.opponent_index == Char.slot)
                Kid.opponent_index = -1;
        } else if (Char.repeat) {
            if (Char.alive < 20)
                ++Char.alive;
            else
                Char.repeat = 0;
        }
    } else if (!Char.life_points) {
        Char.alive = 0;
    }
    AutoCtrl();
    if (GameState.recording_mode)
        CheckRecordedDirForChar();
    if (Char.character_kind != 1 && Char.character_kind != 12)
        GenCtrl();
}

void UserCtrl(void)
{
    byte direction = char_direction;
    if (!direction)
        FaceJoystick();
    if (Invert)
        InvertJoystick();
    if (GameState.recording_mode)
        CheckRecordedDirForChar();
    GenCtrl();
    if (!direction)
        FaceJoystick();
    if (Invert)
        InvertJoystick();
    if (Char.sequence == 237 && Char.frame == 185 && character_room == 9 &&
        Level.level_number == 8)
        PlayThroneRoomNIS();
}

void FaceJoystick(void)
{
    sbyte temporary;
    Joy.x = -Joy.x;
    temporary = Dir.forward;
    Dir.forward = Dir.back;
    Dir.back = temporary;
}

void InvertJoystick(void)
{
    sbyte temporary;
    Joy.y = -Joy.y;
    temporary = Dir.up;
    Dir.up = Dir.down;
    Dir.down = temporary;
}


byte ClrAll(void)
{
    sbyte zero = 0;
    Dir.down = zero;
    Dir.up = zero;
    Dir.forward = zero;
    Dir.back = zero;
    return 1;
}

/* SaveSelect: remember the current selection (FM Towns CTRL.C public; anchored by PlayerCtrl
   call sites in both builds). Dir/SelDir are the FM Towns globals at DOS DS:628c/DS:626a. */
void SaveSelect(void)
{
    SelDir = Dir;
}

/* LoadSelect: restore the remembered selection before input polling (FM Towns CTRL.C). */
void LoadSelect(void)
{
    Dir = SelDir;
}


void InitInput(void) { memset(&SelDir, 0, sizeof(SelDir)); }

void ClrJoystick(void)
{
    if (Dir.forward >= 0) {
        if (Joy.x == -1) {
            if (!Dir.forward)
                Dir.forward = -1;
        } else
            Dir.forward = 0;
    }
    if (Dir.back >= 0) {
        if (Joy.x == 1) {
            if (!Dir.back)
                Dir.back = -1;
        } else
            Dir.back = 0;
    }
    if (Dir.up >= 0) {
        if (Joy.y == -1) {
            if (!Dir.up)
                Dir.up = -1;
        } else
            Dir.up = 0;
    }
    if (Dir.down >= 0) {
        if (Joy.y == 1) {
            if (!Dir.down)
                Dir.down = -1;
        } else
            Dir.down = 0;
    }
    if (Joy.button == -1) {
        if (Dir.action != 1)
            Dir.action = -1;
    } else if (Joy.button == -2) {
        if (Dir.action != 2)
            Dir.action = -2;
    } else
        Dir.action = 0;
}


int IsLedge(sbyte above, sbyte ahead, word above_modifier,
                           word ahead_modifier)
{
    int result;
    if (!IsSpace(above) || (Level.environment_kind == 2 && (above_modifier & 0x80)) ||
        ((ahead == 11 || ahead == 15) && (ahead_modifier & 15)) ||
        !IsFloor(ahead) ||
        (ahead == 4 && !(ahead_modifier & 255) && char_direction == 255) ||
        (Level.environment_kind == 5 && character_room == 15 && character_row != 0))
        result = 0;
    else
        result = 1;
    return result;
}

sbyte GetAbove(void)
{
    return ReadCell(character_room, Char.column, character_row - 1);
}

sbyte GetAboveBehind(void)
{
    return ReadCell(character_room, Char.column + Minus1[Char.direction + 1],
                    character_row - 1);
}

sbyte GetAboveInFront(void)
{
    return ReadCell(character_room, Char.column + Plus1[Char.direction + 1],
                    character_row - 1);
}

/* Semantic name only; this external names the independently witnessed DS byte. */

int FaceDx(int value)
{
    if (!char_direction)
        value = -value;
    return value;
}


void RemoveObj(int portion)
{
    word modifier;
    int index;
    animation_portion = portion;
    Dir.action = 1;
    if (Level.environment_kind == 3)
        modifier = Cell.modifier & 0x3f00;
    else if (Level.environment_kind == 4)
        modifier = Cell.modifier & 0x0f00;
    else
        modifier = 0;
    if (CustomRoom)
        modifier |= 0xc000;
    index = 30 * Cell.room + Cell.index;
    (&Level.cell_kinds[0][0])[index - 30] = 1;
    (&Level.cell_info[0][0])[index - 30].modifier = modifier;
    MarkCellRedraw();
}


void ClipChar(void)
{
    int clearance = 1;
    word frame = Char.frame;
    int action = Char.action;
    sbyte row = character_row;
    int flying;
    sbyte tile;
    struct { int top; Rect bounds; } workspace;

    init_fchar_rect();
    MakeFCharClipRect();
    if (IsCharNonViewable()) {
        FChar.clip = NullRect;
        return;
    }
    if (Char.character_kind == 0 &&
        Char.frame >= 217 && Char.frame <= 226 &&
        63 * character_row + 41 > Char.y) {
        tile = ReadCell(character_room, Char.column, character_row);
        if (ComputeOffsetCellRect(&workspace.bounds, LevelExitClipRect[Level.environment_kind], (tile == 16) + Char.column, character_row)) {
            FChar.clip.top = workspace.bounds.top;
            FChar.clip.right = 320;
        }
        return;
    }
    if (Char.action == 9) {
        if (Level.environment_kind == 5)
            RoofClipChar();
        return;
    }

    flying = Char.character_kind == 7 || Char.character_kind == 8;
    tile = ReadCell(character_room, Cell.first_col, Cell.top_row);
    if (IsSpace(tile) ||
        (Level.environment_kind == 3 && tile == 20 && IsRoundCaveBlock(Cell.modifier)) ||
        (Cell.room == 1 && Level.level_number == 14) ||
        ((Cell.room == 7 || Cell.room == 8) && Level.environment_kind == 6 && Char.sequence != 14)) {
        clearance = 0;
    } else if (action != 0 || (frame != 79 && frame != 81)) {
        tile = ReadCell(character_room, Cell.last_col, Cell.top_row);
        if (IsSpace(tile) || (tile == 20 && Level.environment_kind == 3 && IsRoundCaveBlock(Cell.modifier)))
            clearance = 0;
    }

    if (clearance && Level.environment_kind != 5 && Level.environment_kind != 1 &&
        !(character_room == 16 && Level.level_number == 9) &&
        !(character_room == 27 && Level.level_number == 6) &&
        !(character_room == 3 && Level.environment_kind == 6) &&
        !(character_room == 6 && Level.environment_kind == 6)) {
        if ((sbyte)((tile = row + 1) - 1)) {
            int height = 63 * tile - 60;
            clearance = height >= FChar.y ? 0 : height - 15 >= TopEdge ? 0 : clearance;
        }
        if (clearance && !flying) {
            if ((workspace.top = 63 * tile - 60) > FChar.clip.top)
                FChar.clip.top = TopEdge = workspace.top;
        }
    }
    switch (Level.environment_kind) {
    case 1: DsrtClipChar(); break;
    case 4: RuinsClipChar(); break;
    case 5: RoofClipChar(); break;
    case 6: FinlClipChar(); break;
    }
}


void MakeFCharClipRect(void)
{
    SHAPESIZE size;
    if (FChar.image != -1 && GetSetImgSize(Pics[FChar.image_set], FChar.image, &size)) {
        FChar.clip.top = FChar.y - size.height;
        FChar.clip.bottom = FChar.y + 1;
        if (!FChar.direction) {
            FChar.clip.right = FChar.x;
            FChar.clip.left = FChar.x - size.width;
        } else {
            FChar.clip.left = FChar.x;
            FChar.clip.right = FChar.x + size.width;
        }
        return;
    }
    FChar.clip = NullRect;
}

void UnevenFloor(void)
{
    if (Char.character_kind != 11 && Char.character_kind != 7 && Char.character_kind != 8 && GetUnderFt() == 6 &&
        (Frame.flags & 0x40))
        Char.y = 63 * character_row + 57;
}

void IndexChar(void)
{
    word frame = Char.frame;
    int action = Char.action;
    if (action == 1) {
        Cell.row = Cell.bottom_row;
        Cell.column = Cell.first_col;
    } else {
        Cell.row = character_row;
        Cell.column = Char.column;
    }
    if ((frame >= 135 && frame < 149) || action == 2 || action == 3 || action == 4 || action == 6)
        Cell.column += Minus1[Char.direction + 1];
    FChar.cell = IndexCell1(Cell.column, Cell.row);
}


void PotionEffect(void)
{
    if (!animation_portion)
        return;
    if (Char.character_kind != 0 &&
        (Char.character_kind != 1 || animation_portion != 1))
        return;
    if (animation_portion == -1) {
        Char.stance = 0;
    } else {
        switch (--animation_portion) {
        case 0:
            if (Char.life_initial != Char.life_points)
                Char.life_delta = 1;
            AddSong(101);
            break;
        case 1:
            BoostMeter();
            AddSong(101);
            break;
        case 2: GoWeightless(); break;
        case 3:
            ToggleInvertState();
            break;
        case 4:
            if (Invert) {
                ToggleInvertState();
                break;
            }
            if (DecLifePts(1))
                JumpSeq(71);
            AddSound(14);
            break;
        case 5:
            AddSound(14);
            JumpSeq(111);
            break;
        }
    }
    animation_portion = 0;
}

int Static(void)
{
    int result;
    if (IsDeadPos(Char.frame) || Char.frame == 0 || Char.frame == 15 ||
        Char.frame == 109 || Char.frame == 166 || Char.frame == 171 || Char.frame == 229)
        result = 1;
    else
        result = 0;
    return result;
}

int IsCharNonViewable(void)
{
    int result;
    if (Char.sequence_group == 4 || Char.sequence_group == 5 || Char.sequence_group == 6 ||
        Char.sequence_group == 7 || Char.sequence_group == 10 || Char.sequence_group == 12 ||
        (Level.environment_kind == 5 && character_room != 19 && character_room != 16 &&
         character_room != 15 && IsDeadPos(Char.frame)))
        result = 1;
    else
        result = 0;
    return result;
}

int IsDeadPos(int frame)
{
    int dead;
    if (frame == 185)
        dead = 1;
    else {
        switch (Char.character_kind) {
        case 0:
            dead = frame == 242 || frame == 243 || frame == 185 || frame == 271 || frame == 266;
            if (!dead && Level.level_number == 5 && character_room == 3)
                dead = Char.alive >= 0 && Char.sequence_group == 10;
            break;
        case 2:
            if (!Level.preferred_opponent) {
                dead = frame == 228;
                break;
            }
            dead = frame == 245 || frame == 248 || frame == 251 || frame == 252 ||
                   frame == 253 || frame == 254 || frame == 255;
            break;
        case 4:
            dead = frame == 206 || frame == 207;
            break;
        case 10:
            dead = frame == 213;
            break;
        case 11:
            dead = frame == 273;
            break;
        default:
            dead = 0;
            break;
        }
    }
    return dead;
}

void RemoveChar(void)
{
    if (character_room == 6 && Level.environment_kind == 6 &&
        Char.sequence == 240 && Kid.alive < 0)
        TransferJaffarToEscher2Room();
    VanishChar();
    character_room = 0;
}


void VanishChar(void)
{
    DrawOppMeter(Char.slot, 0, Char.life_initial);
    char_direction = 86;
    Char.action = 0;
    Char.alive = 0;
    Char.life_points = 0;
    if (Char.slot != 0 && Kid.opponent_index == Char.slot)
        Kid.opponent_index = GetOppIdxClosestToKid(1);
}

void SaveFChar(void) { saved_fchar = FChar; }
void RestoreFChar(void) { FChar = saved_fchar; }


void GuardCtrl(void)
{
    if (Char.frame == 166 && Dir.down < 0) {
        if (Dir.forward < 0) {
            if (!IsAdvancableId(GetBehind(), Cell.modifier))
                char_x = AddCharX(5);
            DoEngarde();
        } else {
            Dir.down = 1;
            JumpSeq(80);
        }
    }
}

#define opponent_direction (Opp.direction)
#define opponent_x (Opp.x)
#define opponent_row (Opp.row)
#define opponent_room (Opp.room)
#define opponent_life (Opp.life_points)
int GetOppDist(void)
{
    int distance;
    if (opponent_life && opponent_room == character_room && opponent_row == character_row &&
        Opp.direction != 86) {
        distance = opponent_x - char_x;
        if (char_direction)
            distance = -distance;
        if (distance >= 0 && char_direction != (byte)Opp.direction)
            distance += 13;
    } else {
        distance = 999;
    }
    return distance;
}

static struct slot_error_messages {
    char save[27];
    char load[27];
} slot_errors = {
    "Error in SaveShad(). Quit?",
    "Error in LoadShad(). Quit?"
};


void SaveShad(void)
{
    if (Char.slot >= 0 && Char.slot < 5) {
        RoomOpp[Char.slot] = Char;
        return;
    }
    if (confirm_quit((char far *)slot_errors.save))
        abort_with_message(1, 0);
}

void LoadShad(sbyte slot)
{
    if (slot >= 0 && slot < 5)
        Char = RoomOpp[slot];
    else if (confirm_quit((char far *)slot_errors.load))
        abort_with_message(1, 0);
}

void LoadKid(void)
{
    Char = Kid;
}

/* The frame's final routines follow the Kid-to-Char copy. */
