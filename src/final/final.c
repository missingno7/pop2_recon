#include <string.h>
#include <stdlib.h>
#include "types.h"
#include "snd.h"
#include "graf.h"
#include "shape.h"
#include "level.h"
#include "heap.h"
#include "char.h"
#include "game.h"
#include "util.h"
#include "draw.h"
#include "custom.h"
#include "add.h"
#include "auto.h"
#include "autoshad.h"
#include "checknis.h"
#include "ctrl.h"
#include "fade.h"
#include "final.h"
#include "load.h"
#include "act.h"
#include "prince.h"
#include "tables.h"
#include "move.h"
#include "misc.h"
#include "sound.h"
#include "top.h"


#pragma pack(1)


void DrawFinlObj(DRAWCELL *object)
{
    if (Room.number == 6 && Level.environment_kind == 6 &&
        (object->info.modifier & 0x1000))
        DrawFinlCrystal(object);

    if ((Room.number == 7 || Room.number == 8) &&
        Level.environment_kind == 6 && (object->info.modifier & 0x1000))
        DrawFinlGargoyle(object);
}

void LoadFinlCode(void)
{
    Pause = 0;
}

void FinlCheckDeadOpp(void)
{
    if (Char.alive == 6) {
        sbyte cell;
        word frame;

        cell = ReadCell(Char.room, Char.column, Char.row);
        frame = Rnd(2);
        if (frame != 0 && cell == 1 && TrobCt < 4) {
            cell = FirstInRow(Char.row) + Char.column;
            if (cell < 22 || cell > 24) {
                Level.cell_kinds[2][cell] = 10;
                Level.cell_info[2][cell].modifier = (frame | 0x200) << 5;
                select_room_arrays(3);
                TriggerFlask(3, cell);
                MarkRedraw(cell);
            }
        }
        ++Char.alive;
    } else if (Char.alive >= 96 && Char.sequence != 239) {
        JumpSeq(239);
        AddSound(197);
        ++Char.alive;
    }
    if (Char.repeat == 0 && Char.alive > 6)
        ++Char.alive;
}

void FinlCheckGenOpp(sbyte count)
{
    sbyte variant;
    sbyte room = -1;

    if (count <= 1) {
        if (count == 1) {
            if (RoomOpp[0].sequence != 238) {
                room = (RoomOpp[0].row == 1) ? 2 : 1;
                if (room == Kid.row)
                    room = -1;
            }
        } else {
            room = (Kid.row == 2) ? 1 : 2;
        }
    }
    if (room >= 0 && (count == 0 || Rnd(20) == 0)) {
        variant = (room == 1) ? 4 : 2;
        if (Rnd(1))
            variant += 4;
        FinlGenerateOpp(variant, room);
    }
}


void FinlGenerateOpp(sbyte variant, sbyte room)
{
    sbyte index;
    OPPONENT *opponent;

    opponent = GetOppInfoIdx(3, index = Level.room_objects[2].opponent_count++);
    opponent->cell = FirstInRow(room) + variant;
    opponent->x = ColEdgeX[variant + 5];
    if (variant < 5)
        opponent->direction = 0;
    else
        opponent->direction = -1;
    opponent->skill = 5;
    if (OppBank[0] == 0) {
        PresetOppBanks(1);
        SetOppBanks();
    }
    opponent->palette_kind = OppBank[0];
    opponent->index = index;
    opponent->behavior = opponent->combat_mode = 0;
    opponent->u.skeleton.row = room;
    opponent->max_life = opponent->life = Rnd(2) + 3;
    Char.slot = index;
    Char.direction = opponent->direction;
    Char.x = opponent->x;
    Char.character_kind = OppCharId[Level.preferred_opponent];
    Char.column = variant;
    Char.row = room;
    Char.behavior = 0;
    Char.room = 3;
    Char.life_delta = Char.life_points = Char.life_initial = opponent->life;
    Char.alive = -1;
    Char.image_bank = 4;
    ResetCharY();
    ClearCharVel();
    Char.sequence_group = 0;
    Char.repeat = 1;
    Char.stance = 0;
    JumpSeq(238);
    Char.alert_delay = 0;
    AnimChar();
    AddSound(197);
    if ((byte)Kid.opponent_index == (byte)-1)
        Kid.opponent_index = Char.slot;
    SaveShad();
}

int FinlGetCutDirToJaffarBurning(void)
{
    int index = RoomOpp[0].sequence == 243 ? 0 : 1;
    int next;

    if (Room.number == 7 && RoomOpp[index].column >= 10)
        next = 1;
    else if (Room.number == 8 && RoomOpp[index].column < 0)
        next = 0;
    else
        next = -1;
    return next;
}


void FinlClipChar(void)
{
    if (Char.room == 3 && Level.environment_kind == 6 &&
        Char.row == 1 && IsDeadPos(Char.frame))
        FChar.clip.bottom = 119;
}


void FinlCutKidToChessRoom(void)
{
    Char.image_bank = 2;
    Char.direction = 0;
    JumpSeq(55);
    Char.stance = 1;
    AddSound(19);
    CutRoom = Char.room = 4;
    Char.column = 5;
    Char.x = ColEdgeX[10] + 14;
    Char.row = 1;
    ResetCharY();
    Char.life_delta = Char.life_points;
    AnimChar();
    Kid = Char;
    ClrAll();
    Dir.action = 0;
}

void FinlFrameFunc(void)
{
    FinlWatchKid();
    FinlWatchOpps();

    if (ambient_state.enabled && !SndOn(current_song + 10000) &&
        Kid.alive < 0 && LevelIdx == NextLevel) {
        int tune = -1;

        switch (Room.number) {
        case 3:
            tune = 269;
            break;
        case 6:
            tune = Level.room_objects[5].opponent_count ? 270 : 268;
            break;
        case 8:
            GameState.escher_room_eight_seen = 1;
            /* Fall through. */
        case 7:
            tune = GameState.escher_room_eight_seen ? 263 : 268;
            break;
        }
        if (tune != -1 && !SndOn(tune + 10000))
            AddSong(tune);
    }
}

void FinlKidEnterPrincessRoom(void)
{
    PlayNIS(5);
    reset_level_overlays();
    LoadLevel(LevelIdx);
    SetPalette(1000, 240, 16, 0);
    SetKidPalette(6);
    FinlCutKidToChessRoom();
}


void FinlSortForeTable(void)
{
    int out, index;
    BFITEM saved;

    index = Ct[1] - 1;
    out = index;
    for (; index >= 0; --index) {
        int shape = CustomRoom->pieces[ForeTable[index].pos.image - CustomRoom->first_shape].shape
                  + CustomRoom->first_shape;
        if ((shape >= 0x1021 && shape <= 0x1024) || (shape >= 0x1058 && shape <= 0x105c)) {
            if (index != out) {
                saved = ForeTable[index];
                _fmemmove(&ForeTable[index], &ForeTable[index + 1], (out - index) * sizeof(BFITEM));
                ForeTable[out] = saved;
            }
            --out;
        }
    }
}


void FinlWatchKid(void)
{
    LoadKid();
    if (Char.room == 2) {
        if (CDRightEdge <= 305 && Char.action != 4)
            FinlKidEnterPrincessRoom();
    } else if (Char.character_kind == 1 && Char.sequence != 71) {
        sbyte room;

        LoadShad(GetKidIdxAsOpp());
        room = RegisterCharInRoom();
        if ((room == 7 || room == 8) && Level.environment_kind == 6 &&
            Kid.room == 5 && Level.environment_kind == 6 && Kid.column <= 4 &&
            (KidFrames[Kid.frame].flags & 0x40)) {
            LoadKid();
            DecLifePts(100);
            JumpSeq(71);
            Kid = Char;
            ChangeMeters();
            CheckMeters();
        }
        LoadKid();
    }
    Kid = Char;
}

void FinlWatchOpps(void)
{
    sbyte i;
    sbyte count = Level.room_objects[Room.number - 1].opponent_count;

    if (count) {
        i = 0;
        if (count > 0)
            do {
                LoadShad(i);
                SaveShad();
            } while (count > ++i);
    }
    if (Room.number == 3 && Level.environment_kind == 6)
        FinlCheckGenOpp(count);
}


void JaffarProg(void)
{
    if (Char.room == 6 && Level.environment_kind == 6) {
        JaffarInCrystalRoomProg();
        return;
    }
    JaffarInEscherRoomProg();
}

void AllOtherJaffarsShimmerAway(void)
{
    CHARREC saved = Char;
    sbyte i = 0;
    sbyte count = Level.room_objects[Opp.room - 1].opponent_count;

    if (count > 0)
        do {
            LoadShad(i);
            if (Char.character_kind == 6 && Char.slot != Opp.slot && Char.frame == 15) {
                JumpSeq(240);
                SaveShad();
            }
        } while (count > ++i);
    Char = saved;
}

void DoJaffCastSpell(void)
{
    int sequence = -1;

    if (Char.frame == 15) {
        if ((Char.x > Opp.x && Char.direction != -1) ||
            (Char.x <= Opp.x && Char.direction != 0))
            sequence = 5;
        else
            sequence = 242;
    } else if (((Char.frame >= 1 && Char.frame <= 14) ||
                (Char.frame >= 49 && Char.frame <= 56)) &&
               Char.sequence != 13 && Char.sequence != 5) {
        sequence = 13;
    }
    if (sequence != -1)
        JumpSeq(sequence);
}


void JaffarAttackInEscherRooms(void)
{
    int move = 1;
    sbyte *stance;
    OPPONENT *raw;

    raw = GetOppInfoIdx(Char.room, Char.slot);
    stance = raw->u.jaffar.stance;
    if (raw->u.jaffar.stance[0] != 2)
        stance[0] = 2;
    if (Char.sequence == 242) {
        if (Char.frame == 281)
            AddSound(62);
        else if (Char.frame == 282 && JaffClearShotAtKid())
            JaffHitKidWithCast();
        move = 0;
    } else if (stance[1] == stance[2]) {
        if (Char.row == Opp.row && JaffClearShotAtKid()) {
            DoJaffCastSpell();
        } else if (Opp.sequence != 241) {
            if (ShadowInRange(stance[1], -1))
                JaffarSetToNextPos((char *)stance);
            else
                JaffarSetToPrevPos((char *)stance);
        }
    }
    if (move && stance[1] != stance[2] && Opp.life_points > 0) {
        if (Char.row == 1 && JaffClearShotAtKid()) {
            DoJaffCastSpell();
            return;
        }
        JaffarGoToPos(stance);
    }
}

void JaffarFleeInEscherRooms(void)
{
    sbyte *stance;
    OPPONENT *raw;

    raw = GetOppInfoIdx(Char.room, Char.slot);
    stance = raw->u.jaffar.stance;
    if (raw->u.jaffar.stance[0] != 1)
        stance[0] = 1;
    if (stance[1] == stance[2]) {
        if (ShadowInRange(stance[1], -1))
            JaffarSetToPrevPos((char *)stance);
        else if (ShadowInRange(stance[1], 1))
            JaffarSetToNextPos((char *)stance);
    }
    if (stance[1] != stance[2])
        JaffarGoToPos(stance);
}

int JaffClearShotAtKid(void)
{
    int clear = 0;

    if (Opp.life_points > 0 && (Opp.room == 7 || Opp.room == 8) &&
        Level.environment_kind == 6 && Opp.sequence != 241 && Char.row == Opp.row &&
        (KidFrames[Char.frame].flags & 0x40) && Char.frame != 109) {
        sbyte position = Char.column;

        if (Char.room == 8)
            position += 10;
        if (Char.row == 0) {
            clear = position <= 6 || position >= 13;
        } else if (Char.row == 1) {
            sbyte shadow = Opp.column;

            if (Opp.room == 8)
                shadow += 10;
            if (position <= 12) {
                if (shadow <= 12)
                    clear = 1;
            } else if (shadow >= 14) {
                clear = 1;
            }
        } else if (Char.row == 2) {
            clear = 1;
        }
    }
    return clear;
}

void JaffarSetToNextPos(char *p)
{
    if (p[1] == 3)
        p[1] = 0;
    else
        ++p[1];
}

void JaffarSetToPrevPos(char *record)
{
    if (record[1] != 0) {
        --record[1];
        return;
    }

    record[1] = 3;
}

typedef struct {
    sbyte room_x;
    byte flags;
    sbyte direction;
    int min_x, max_x;
    sbyte shadow_column[2];
    sbyte shadow_min[2];
    sbyte shadow_max[2];
} PositionRule;

static PositionRule position_rules[4] = {
    {17, 0, -1, 388, 400, {1, 0}, {15, 1}, {18, 18}},
    {14, 2, -1, 280, 304, {2, 1}, {0, 14}, {18, 18}},
    { 5, 2,  0, 312, 336, {1, 2}, {2, 0}, {9, 18}},
    { 6, 0,  0, 360, 368, {0, 1}, {1, 6}, {18, 10}}
};

static int jaffar_motion_disabled = 0;
static byte blink_frame_order[6] = {0, 1, 2, 2, 1, 0};


void JaffarGoToPos(sbyte *stance)
{
    if (!jaffar_motion_disabled) {
        int move;
        switch (stance[1]) {
        case 0:
            move = JaffarGoToPos0(stance);
            break;
        case 1:
            move = JaffarGoToPos1(stance);
            break;
        case 2:
            move = JaffarGoToPos2(stance);
            break;
        case 3:
            move = JaffarGoToPos3(stance);
            break;
        }
        if (move != -1)
            JumpSeq(move);
    }
}

int JaffarHoneInOnExactPos(sbyte *stance)
{
    int sequence = -1;

    if (Char.frame == 15) {
        if (stance[0] == 2 && JaffClearShotAtKid()) {
            DoJaffCastSpell();
        } else {
            const PositionRule *rule = &position_rules[stance[1]];
            int x = Char.x;

            if (rule->direction != Char.direction)
                x += Plus1[rule->direction + 1] * 6;
            if (rule->room_x < 10) {
                if (Char.room == 8)
                    x += 320;
            } else if (Char.room == 7) {
                x -= 320;
            }
            if (x < rule->min_x) {
                if (Char.direction == 0)
                    DoStepFwd(rule->min_x - x + 2);
                else
                    sequence = 5;
            } else if (x > rule->max_x) {
                if (Char.direction == -1)
                    DoStepFwd(x - rule->min_x + 2);
                else
                    sequence = 5;
            } else if (rule->direction != Char.direction) {
                sequence = 5;
            } else {
                stance[2] = stance[1];
            }
        }
    }
    return sequence;
}


int JaffarGoToPos0(sbyte *stance)
{
    sbyte x = Char.column + ((Char.room == 8) ? 1 : 0) * 10;
    int sequence = -1;
    const PositionRule *rule = &position_rules[stance[1]];

    if (stance[2] == 3) {
        if (Char.frame == 15) {
            if (x == 6)
                DoStandJump();
            else if (x > 7 && x < rule->room_x - 1)
                DoStartRun();
            else
                sequence = JaffarHoneInOnExactPos(stance);
        } else if (Char.frame == 7 || Char.frame == 11) {
            if ((x >= 5 && x <= 6) || (x >= 10 && x <= 11)) DoRunJump(4);
            else if (x >= rule->room_x - 1) sequence = 13;
        }
    } else if (Char.row == 2) {
        if (Char.frame == 15) {
            if (Char.direction != 0) goto turn;
            DoJumpUp();
        } else if (Char.frame == 91)
            ClimbUp();
    } else if (Char.row == 1) {
        if (Char.frame == 15) {
            if (Char.direction == 0) {
                if (x < 17) DoStartRun();
                else
                    goto turn;
            } else if (x >= 17)
                DoJumpUp();
            else {
turn:
                sequence = 5;
            }
        } else if (Char.frame == 7 && x >= 17)
            sequence = 13;
        else if (Char.frame == 91) ClimbUp();
        else if (Char.frame == 109) sequence = 49;
    } else {
        sequence = JaffarHoneInOnExactPos(stance);
    }
    return sequence;
}

int JaffarGoToPos1(sbyte *stance)
{
    int sequence = -1; const PositionRule *rule = &position_rules[stance[1]];

    if (stance[2] < stance[1]) {
        if (Char.row == 0) {
            if (Char.direction == 0 && Char.sequence != 5)
                sequence = 5;
            else if (Char.frame == 15)
                Down();
        } else if (Char.row == 1) {
            if (Char.frame >= 87 && Char.frame <= 91) {
                Drop();
            } else if (Char.frame == 15) {
                if (Char.direction != -1)
                    sequence = 5;
                else
                    DoStartRun();
            } else if (Char.frame == 109) {
                sequence = 49;
            }
        } else if (Char.row == 2) {
            if (Char.frame != 109)
                sequence = JaffarHoneInOnExactPos(stance);
            else
                sequence = 49;
        }
    } else {
        sbyte x = Char.column + ((Char.room == 8) ? 1 : 0) * 10;

        if (x == position_rules[2].room_x && Char.frame == 15)
            DoStartRun();
        else if (x >= 6 && x <= 7) {
            DoRunJump(4);
        } else if (x > 10) {
            if (Char.frame == 7 && x >= rule->room_x - 1)
                sequence = 13;
            else
                sequence = JaffarHoneInOnExactPos(stance);
        }
    }
    return sequence;
}


int JaffarGoToPos2(sbyte *stance)
{
    int sequence = -1;

    if (stance[2] < stance[1]) {
        sbyte x = Char.column + ((Char.room == 8) ? 1 : 0) * 10;

        if (x == position_rules[1].room_x && Char.frame == 15) {
            DoStartRun();
        } else if (x >= 11 && ColEdgeX[7] + 36 > Char.x &&
                   Char.sequence != 4 && (Char.frame == 7 || Char.frame == 11)) {
            Char.x = AddCharX(2);
            DoRunJump(4);
        } else if (x <= 6 && Char.frame == 7) {
            sequence = 13;
        } else {
            sequence = JaffarHoneInOnExactPos(stance);
        }
    } else if (Char.row == 0) {
        if (Char.frame == 15) {
            if (Char.direction == 0) sequence = 5;
            else
                Down();
        }
    } else if (Char.row == 1) {
        if (Char.frame >= 87 && Char.frame <= 91) {
            Drop();
        } else if (Char.frame == 15) {
            if (Char.direction == -1) DoStartRun();
            else
                sequence = 5;
        } else if (Char.frame == 109) {
            sequence = 49;
        }
    } else if (Char.row == 2) {
        if (Char.frame == 109) sequence = 49;
        else {
            sequence = JaffarHoneInOnExactPos(stance);
        }
    }
    return sequence;
}

int JaffarGoToPos3(sbyte *stance)
{
    int sequence = -1;
    const PositionRule *rule = &position_rules[stance[1]];
    sbyte x = Char.column + ((Char.room == 8) ? 1 : 0) * 10;

    if (stance[2] == 0) {
        if (Char.frame == 15) {
            if (x > 10) DoStartRun();
            else if (x <= rule->room_x)
                sequence = JaffarHoneInOnExactPos(stance);
        } else if (Char.frame == 7 || Char.frame == 11) {
            if (x <= rule->room_x) sequence = 13;
            else if ((x >= 14 && x <= 15) || (x >= 9 && x <= 10))
                DoRunJump(4);
        }
    } else if (Char.row == 2) {
        if (Char.frame == 15) {
            DoJumpUp();
        } else if (Char.frame == 91) {
            ClimbUp();
        }
    } else if (Char.row == 1) {
        if (Char.frame == 15) {
            if (Char.direction != 0) DoJumpUp();
            else if (x == 6) DoStepFwd(32);
            else
                sequence = 5;
        } else {
            if (Char.frame == 91) ClimbUp();
            else if (Char.frame == 109) sequence = 49;
        }
    } else {
        sequence = JaffarHoneInOnExactPos(stance);
    }
    return sequence;
}


void JaffHitKidWithCast(void)
{
    sbyte slot;

    JumpSeq(244);
    slot = Char.slot;
    SaveShad();
    LoadKid();
    JumpSeq(71);
    AddSound(14);
    DecLifePts(100);
    SetKidDeathMethod(17);
    Kid = Char;
    LoadShadwOpp(slot);
}


void JaffarInCrystalRoomProg(void)
{
    if (Char.alive < 0) {
        sbyte count = Level.room_objects[Char.room - 1].opponent_count;

        if (Opp.character_kind == 1)
            --count;
        if (count == 1 && Char.sequence != 240 && Kid.alive < 0) {
            JumpSeq(240);
        } else if (Char.frame == 15) {
            if (Kid.alive < 0 && Kid.sequence != 44 && Kid.sequence != 6 &&
                Kid.character_kind == 0 && (KidFrames[Kid.frame].flags & 0x40) &&
                Kid.action != 3 && Kid.action != 4) {
                int attack = 0;
                int range = GetOppDist();

                if (Char.direction == Opp.direction) {
                    range -= 15;
                    if (range > 18 && range < 48)
                        attack = 1;
                } else if (Char.direction != Opp.direction) {
                    if (range < 0) {
                        range = -15 - range;
                        if (range > 18 && range < 48) {
                            Char.direction = ~Char.direction;
                            attack = 1;
                        }
                    } else if (Kid.frame == 145) {
                        range -= 15;
                        if (range > 18 && range < 52)
                            attack = 1;
                    }
                }
                if (attack) {
                    Char.x = AddCharX(15);
                    Char.stance = 1;
                    JumpSeq(75);
                }
            }
        } else if (Char.frame == 171) {
            Char.stance = 0;
            JumpSeq(92);
        } else if (Char.frame == 52 && Kid.alive > 0) {
            JumpSeq(197);
            AddSound(262);
        } else if (Char.frame == 109) {
            JumpSeq(49);
        }
    }
}

void JaffarInEscherRoomProg(void)
{
    int flee;
    int attack;
    sbyte slot;
    OPPONENT *opponent;
    sbyte *stance;

    opponent = GetOppInfoIdx(Char.room, Char.slot);
    stance = opponent->u.jaffar.stance;
    if (opponent->u.jaffar.stance[0] == 1 && stance[2] != stance[1])
        flee = 1;
    else
        flee = 0;

    attack = 0;
    if (Opp.character_kind == 0) {
        if (Opp.stance == 1 &&
            !(Char.frame >= 207 && Char.frame <= 210) &&
            Char.frame != 158) {
            if (abs(GetOppDist()) < 62) {
                slot = Char.slot;
                SaveShad();
                LoadKid();
                JumpSeq(241);
                AddSong(265);
                Char.stance = -1;
                Kid = Char;
                LoadShad(slot);
            }
        } else if (Char.row == Opp.row && KidInfo.shadow_timer >= 4) {
            if (Opp.life_points <= 2 && Opp.sequence != 242)
                flee = 0;
            else
                flee = 1;
        } else {
            attack = Opp.stance == -1 ? 1 : 0;
        }
    } else {
        if (Opp.life_points <= 2 && Opp.sequence != 242)
            flee = 0;
        else
            flee = 1;
        if (Opp.life_points <= 2 && Opp.sequence != 242)
            attack = 1;
        else
            attack = 0;
    }

    if (Char.sequence == 243 && Char.frame == 351)
        ++NextLevel;
    if (flee) {
        JaffarFleeInEscherRooms();
        return;
    }
    if (attack)
        JaffarAttackInEscherRooms();
}

void JaffarUpdateOppInfo(OPPONENT *opponent)
{
    sbyte *stance = opponent->u.jaffar.stance;

    if (!((CutRoom == 7 || CutRoom == 8) && Level.environment_kind == 6) &&
        stance[2] != stance[1]) {
        const PositionRule *rule;

        stance[2] = stance[1];
        rule = &position_rules[stance[1]];
        Char.row = rule->flags;
        Char.column = rule->room_x;
        Char.x = rule->min_x + 6;
        if (Char.room == 8) {
            Char.x -= 320;
            Char.column -= 10;
        }
        Char.direction = rule->direction;
        ResetCharY();
        JumpSeq(2);
        AnimChar();
        opponent->cell = IndexCell(Char.column, Char.row);
        opponent->x = Char.x;
        opponent->direction = Char.direction;
        *(int *)((byte *)opponent + 7) = Char.sequence_position;
        *(int *)((byte *)opponent + 5) = Char.sequence_id;
    }
    *(int *)(stance + 4) = Char.y;
}

int ShadowInRange(sbyte position, sbyte direction)
{
    const PositionRule *rule;
    int inside;
    sbyte x;

    if (direction < 0)
        direction = 0;
    else
        direction = 1;
    rule = &position_rules[position];
    if (rule->shadow_column[direction] == Opp.row &&
        (x = Opp.column + (Opp.room == 8 ? 1 : 0) * 10) >= rule->shadow_min[direction] &&
        x <= rule->shadow_max[direction])
        inside = 1;
    else
        inside = 0;
    return inside;
}


void TransferJaffarToEscher2Room(void)
{
    Char.column = 17;
    Char.x = 712;
    Char.row = -3;
    Char.y = -133;
    Char.direction = position_rules[0].direction;
    GetOppInfoIdx(Char.room, Char.slot)->u.jaffar.transition = 0;
    JumpSeq(2);
    AnimChar();
    UpdateOppInfo();
}

void LoadTowerTop(void)
{
    if (!GameState.playing_nis)
        SetPalette(3500, 64, 192, 0);
    if (BPort)
        Graf = BPort;
    FillRect(&GamePlayRect, 139);
}

void LoadChess(void)
{
    SetPalette(3500, 64, 192, 1);
    SetPalette(1000, 240, 16, 0);
    if (Room.number == 4 && Level.environment_kind == 6) {
        PORT *previous = Graf;

        Graf = BPort;
        EraseRect(&GamePlayRect);
        Graf = previous;
        AddSong(268);
    } else if (Room.number == 3 && Level.environment_kind == 6) {
        SetPalette(3000, 224, 16, 0);
        AddSong(269);
    }
}


typedef void (*AnimationCallback)(sbyte cell, void near *clip);


void SetupGuardSmoke(void)
{
    ATTACHFRAME far *table;
    word index;

    if (Char.character_kind == 2 && Char.frame >= 229 && Char.frame <= 254) {
        index = Frame.equipment_image;
        table = h2p(SwordFrames);
        if (table[index].image != -1) {
            FChar.image = table[index].image;
            AddFCharX(table[index].x);
            FChar.y += table[index].y;
            FChar.image_set = 3;
            MakeFCharClipRect();
            AddCharObj(9);
            MarkBufsForRect(MarkFRedraw, &FChar.clip, Char.slot);
        }
    }
}

/* Each frame selects a room image slot. Each shape pair packs two word offsets. */
static byte crystal_frame_slot[] = {
    0, 1, 2, 3, 2, 1, 0, 0
};
static POINT crystal_shapes[] = {
    {10, 310}, {72, 47}, {84, 138},
    {65, 206}, {84, 235}, {130, 14},
    {136, 52}, {164, 128}, {155, 267}
};
static byte crystal_layer[] = {
    1, 0, 0, 0, 0, 1, 1, 1, 1, 0
};
static byte gargoyle_frame_slot[] = {
    0, 1, 2, 4, 3, 4, 2, 1, 0, 0
};
static POINT gargoyle_shapes[] = {
    {67, 36}, {67, 63}, {67, 94},
    {67, 123}, {67, 154}, {131, 113},
    {68, -1}, {126, 25}, {131, 214}
};
static byte gargoyle_layer[] = {
    0, 0, 0, 0, 0, 1, 0, 1, 1, 0
};


static byte crystal_serial;
static byte gargoyle_serial;


void AnimFinlJaffarBlink(void)
{
    word frame;

    ++BlueSpec.modifier;
    frame = BlueSpec.modifier & 15;
    if (frame >= 5 && CustomRoom->kind == 26) {
        frame -= 5;
        if (frame < 6) {
            MarkBufsForRect(MarkMove, &CustomRoom->pieces[2].clip, -1);
            return;
        }
        Trob.state = (byte)-1;
        BlueSpec.modifier = 0;
    }
}

void DrawFinlJaffarBlink(DRAWCELL *object)
{
    word frame = object->info.modifier & 15;

    if ((NewRoom || frame >= 5) && object->pass == 11) {
        if (!NewRoom)
            frame -= 5;
        if (frame < 6)
            DrawCustomRoomImg(blink_frame_order[frame] + 2, 11);
    }
}

void AnimFinlSpider(void)
{
    if (CustomRoom->kind == 25) {
        word state = BlueSpec.modifier;
        int ticks = (state & 0x7f8) >> 3;

        if (state & 0x800) {
            state = Rnd(5);
            if (--ticks > 0 && !NewRoom) {
                state |= 0x800;
            } else {
                ticks = Rnd(60) + 30;
            }
            MarkBufsForRect(MarkMove, &CustomRoom->pieces[6].clip, -1);
        } else if (--ticks <= 0) {
            if (!GameState.fade_active) {
                ticks = Rnd(2) + 1;
                state |= 0x800;
            } else {
                ticks = Rnd(60) + 30;
            }
        }
        state &= 0xf807;
        state |= ticks << 3;
        BlueSpec.modifier = state;
        return;
    }
    Trob.state = (byte)-1;
}


void DrawFinlSpider(DRAWCELL *object)
{
    if (object->pass == 11) {
        int previous = CellClipRect.left;

        CellClipRect.left -= 6;
        DrawCustomRoomImg((object->info.modifier & 7) + 6, 11);
        CellClipRect.left = previous;
    }
}

void LoadCrystal(void)
{
    SetPalette(3500, 64, 192, 2);
    SetPalette(1000, 240, 16, 0);
    SetPalette(25001, 32, 16, 7);
    crystal_serial = 0;
    if (Room.number == 7) {
        SetPalette(3000, 224, 16, gargoyle_serial = 0);
        if (GameState.escher_room_eight_seen)
            AddSong(263);
    } else if (Room.number == 8) {
        gargoyle_serial = 6;
        SetPalette(3000, 224, 16, 0);
        if (!SndOn(263 + 10000) && Level.room_objects[7].opponent_count)
            AddSong(263);
    } else {
        AddSong(270);
    }
}

void AnimFinlCrystal(void)
{
    if (CustomRoom->kind == 28) {
        word state = BlueSpec.modifier; if (state & 0x800) {
            word frame = ((state & 0x7f0) >> 4) + 1;

            if (frame < 7) {
                Rect bounds;
                const POINT *shape;

                state &= 0xf80f; state |= frame << 4;
                shape = &crystal_shapes[state & 15]; OffsetRect(&bounds, &CustomRoom->pieces[4].clip, shape->x, shape->y);
                MarkBufsForRect(MarkMove, &bounds, -1);
            } else {
                state &= 0xf80f; state |= (Rnd(40) + 40) << 4; state &= ~0x800;
            }
        } else {
            int ticks = (state & 0x7f0) >> 4; state &= 0xf80f; if (--ticks <= 0) {
                if (GameState.fade_active && TrobCt)
                    state |= (Rnd(40) + 40) << 4;
                else
                    state |= 0x800;
            } else {
                state |= ticks << 4;
            }
        }
        BlueSpec.modifier = state | 0x1000;
        return;
    }
    Trob.state = (byte)-1;
}

void DrawFinlCrystal(DRAWCELL *object)
{
    int frame;
    byte index, shape;
    CUSTOMPIECE *image;
    Rect bounds;

    if (object->pass != 1 || !(object->info.modifier & 0x800))
        return;
    frame = (short)(object->info.modifier & 0x7f0) >> 4;
    if (--frame < 0)
        return;
    index = crystal_frame_slot[frame] + 4;
    image = &CustomRoom->pieces[index];
    bounds = image->clip;
    shape = object->info.modifier & 15;
    image->pos = crystal_shapes[shape];
    OffsetRect(&image->clip, &image->clip, image->pos.x, image->pos.y);
    image->pass = crystal_layer[shape];
    DrawCustomRoomImg(index, crystal_layer[shape]);
    image->pass = 11;
    image->clip = bounds;
    image->pos.y = 0;
    image->pos.x = 0;
}

void TriggerFinlCrystal(sbyte row, sbyte col)
{
    word serial = crystal_serial++;
    word ticks = Rnd(40);

    BlueS[col].modifier = ((ticks | 0x100) << 4) | serial;
    AddTrob(row, col, 1, 41);
}

void AnimFinlGargoyle(void)
{
    if (CustomRoom->kind == 29 || CustomRoom->kind == 30) {
        word state = BlueSpec.modifier;
        byte serial = (byte)(state & 15);

        if ((serial < 6 && Room.number == 7) || (serial >= 6 && Room.number == 8)) {
            if (state & 0x800) {
                word frame = ((state & 0x7f0) >> 4) + 1;

                if (frame < 9) {
                    Rect bounds;
                    const POINT *shape;

                    state &= 0xf80f;
                    state |= frame << 4;
                    shape = &gargoyle_shapes[serial];
                    OffsetRect(&bounds, &CustomRoom->pieces[2].clip, shape->x, shape->y);
                    MarkBufsForRect(MarkFRedraw, &bounds, -1);
                } else {
                    state &= 0xf80f;
                    state |= (Rnd(40) + 80) << 4;
                    state &= ~0x800;
                }
            } else {
                int ticks = (state & 0x7f0) >> 4;

                state &= 0xf80f;
                if (--ticks <= 0) {
                    if (GameState.fade_active && TrobCt)
                        state |= (Rnd(40) + 80) << 4;
                    else
                        state |= 0x800;
                } else {
                    state |= ticks << 4;
                }
            }
            BlueSpec.modifier = state | 0x1000;
            return;
        }
    }
    Trob.state = (byte)-1;
}

void DrawFinlGargoyle(DRAWCELL *object)
{
    int frame;
    byte index, shape;
    CUSTOMPIECE *image;
    Rect bounds;

    if (object->pass != 1 || !(object->info.modifier & 0x800))
        return;
    frame = (short)(object->info.modifier & 0x7f0) >> 4;
    if (--frame < 0)
        return;
    index = gargoyle_frame_slot[frame] + 2;
    image = &CustomRoom->pieces[index];
    bounds = image->clip;
    shape = object->info.modifier & 15;
    image->pos = gargoyle_shapes[shape];
    OffsetRect(&image->clip, &image->clip, image->pos.x, image->pos.y);
    image->pass = gargoyle_layer[shape];
    DrawCustomRoomImg(index, gargoyle_layer[shape]);
    image->pass = 11;
    image->clip = bounds;
    image->pos.y = 0;
    image->pos.x = 0;
}

void TriggerFinlGargoyle(sbyte row, sbyte col)
{
    word serial = gargoyle_serial++;
    word ticks = Rnd(40);

    BlueS[col].modifier = ((ticks | 0x100) << 4) | serial;
    AddTrob(row, col, 1, 42);
}


#define fireball_half_width MobWidth[12]
#define fireball_height MobHeight[12]


void ATMFireball(void)
{
    int visible = 1;
    int y = Mob.y;

    if (Room.number == Mob.room) {
        if (Mob.x > 449 || Mob.x < 0)
            visible = 0;
    } else if (Mob.room == Room.left) {
        if (Mob.x + fireball_half_width > 320) {
            Mob.x -= 320;
            Mob.room = Room.number;
        } else {
            visible = 0;
        }
    } else if (Mob.room == Room.right) {
        if (Mob.x - fireball_half_width < 320) {
            Mob.x += 320;
            Mob.room = Room.number;
        } else {
            visible = 0;
        }
    } else {
        visible = 0;
    }

    if (visible) {
        sbyte col;
        word image;
        SHAPESIZE size;

        FChar.cell = IndexCell1(col = Mob.x / 32, GetRow(y));
        ++col;
        image = GetFireballImg(Mob.flags);
        if (GetSetImgSize(Pics[2], image, &size)) {
            int saved_y;
            int x = Mob.x;
            Mob.x -= size.width / 2;
            saved_y = y;
            Mob.y = (y += size.height / 2);
            fireball_height = size.height;
            fireball_half_width = size.width;
            AddMobObj(Mob.y);
            MarkMobRedraw(1);
            Mob.y = saved_y;
            Mob.x = x;
        }
    }
}

void CheckFireballHitChar(void)
{
    if (Char.character_kind == 6 && Char.alive < 0 && Char.sequence != 243) {
        int index = 1;
        MOBREC *ball;

        do {
            int offset;

            ball = GetMobTypeItem(12, index);
            if (ball && ball->row == Char.row && !(ball->flags & 8)) {
                if (ball->room == Char.room)
                    offset = 0;
                else if (Char.room == 8 && Level.environment_kind == 6)
                    offset = -320;
                else
                    offset = 320;
                offset += ball->x + ball->auxiliary + 113;
                if (offset + 34 > CDLeftEdge && offset < CDRightEdge) {
                    ball->flags = 8;
                    ball->x = ball->auxiliary > 0 ? CDLeftEdge - 125 : CDRightEdge - 135;
                    DecLifePts(100);
                    JumpSeq(243);
                    AnimChar();
                    AddSound(62);
                    CutRoom = Char.room;
                }
            }
            ++index;
        } while (ball);
    }
}

void CheckFireballHitWall(void)
{
    int x = Mob.x + fireball_half_width - 14;
    sbyte col;

    if (Mob.auxiliary < 0)
        x -= 2 * fireball_half_width;
    col = x / 32;
    if (x < 0)
        --col;
    if (IsWall(ReadCell(Mob.room, col, Mob.row))) {
        Mob.x = col * 32 + 14;
        if (Mob.auxiliary < 0)
            Mob.x += 32;
        Mob.flags = 8;
        AddSound(259);
        ResetSnd(0x281a);
    }
}

int DoThrowFireball(void)
{
    int sequence;
    if (Char.life_points > 2) {
        sequence = 242;

        DecLifePts(2);
        AddSong(266);
    } else {
        sequence = -1;
    }
    return sequence;
}

void DrawObjFireball(byte type)
{
    int image = GetFireballImg(FChar.image);

    AddMid(2, image + 1, FChar.x, FChar.y, 10);
    MidTable[Ct[3] - 1].variant = 8;
    MidTable[Ct[3] - 1].mirrored = 0;
}

int _fastcall GetFireballImg(word state)
{
    word frame = state & 7;
    int number;

    if (state & 8) {
        number = frame;
        number += 308;
    } else {
        number = frame;
        number += 304;
    }
    return number;
}

void MobFireball(void)
{
    if (Mob.velocity >= 0) {
        word frame = Mob.flags & 7;

        if (Mob.flags & 8) {
            if (frame >= 5)
                Mob.velocity = -1;
            else
                ++frame;
        } else {
            if (Mob.auxiliary > 0) {
                if (Mob.auxiliary < 16)
                    Mob.auxiliary += 2;
            } else if (Mob.auxiliary > -16) {
                Mob.auxiliary -= 2;
            }
            Mob.x += Mob.auxiliary;
            if (Mob.room == 0) {
                Mob.velocity = -1;
                ResetSnd(0x2812);
            } else {
                CheckFireballHitWall();
                if (!(Mob.flags & 8)) {
                    if (frame >= 3)
                        frame = 0;
                    else
                        ++frame;
                    AddSound(258);
                }
            }
        }
        if (Mob.velocity == -1) {
            if (Kid.life_points <= 2) {
                SetPalette(2000, 48, 16, 0);
                return;
            }
        } else {
            Mob.flags &= ~7;
            Mob.flags |= frame;
        }
    }
}

void ThrowingFireball(void)
{
    if (Char.frame == 281)
        TriggerFireball();
    AddSound(258);
}

void TriggerFireball(void)
{
    int y;

    Mob.x = AddCharX(27) - 130;
    Mob.y = y = Char.y - 29;
    Mob.room = Char.room;
    Mob.velocity = 0;
    Mob.auxiliary = Plus1[Char.direction + 1] * 8;
    Mob.kind = 12;
    Mob.row = GetRow(y);
    Mob.flags = 2;
    CheckFireballHitWall();
    AddMob();
    AddSound(258);
}


DRAWPROC DrawFinlFunc[] = {
    DrawFinlObj, DrawFinlObj, 0, 0, 0,
    0, 0, 0, 0, 0,
    DrawFlask, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    DrawFinlObj, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, DrawFinlJaffarBlink, 0, 0, 0,
    0, 0, 0, 0, 0,
    DrawFinlSpider, DrawFinlCrystal, DrawFinlGargoyle
};
