#include <string.h>
#include <stdlib.h>
#include "types.h"
#include "level.h"
#include "snd.h"
#include "system.h"
#include "shape.h"
#include "util.h"
#include "graf.h"
#include "char.h"
#include "movescyt.h"
#include "draw.h"
#include "game.h"
#include "movecave.h"
#include "act.h"
#include "add.h"
#include "auto.h"
#include "autoguar.h"
#include "autoshad.h"
#include "checknis.h"
#include "collide.h"
#include "ctrl.h"
#include "movetemp.h"
#include "dlghelp.h"
#include "dialog.h"
#include "fade.h"
#include "intrface.h"
#include "custom.h"
#include "diag.h"
#include "lgraf.h"
#include "movespik.h"
#include "load.h"
#include "anim.h"
#include "final.h"
#include "misc.h"
#include "prince.h"
#include "sound.h"
#include "move.h"
#include "nis.h"
#include "recorder.h"
#include "drawruin.h"
#include "tables.h"
#include "top.h"

int Minutes;
int RestartLevel;
int TimeRequest;
int BackToLife;
int MsgTimer;
int MsgTimerStart;
word SixtiethsCtr;
int NextLevel;
int CutPlan;
unsigned long FrameNum;
int Invert;
int WeightlessTimer;
int CutTimer;
int GuardTimer;
int Refract;
int JustBlocked;
int CutDir;
int ShakeFloor;
int AlertGuard;
int DroppedOut;
int Screamed;
int Stunned;
CELLREC Cell;
sbyte StartLevel;
byte CutRoom;
sbyte OrigLifePts;

#pragma pack(1)

typedef void (*frame_proc)(void);





















static char level_option[] = "LEVEL";
Rect GamePlayRect = { 0, 0, 192, 320 };



              /* far:0a5c:004e */
                  /* far:1101:29aa */
             /* far:0284:000e */
             /* far:0000:0210 */
                       /* far:0a5c:01b4 */
               /* far:0a5c:02f4 */
        /* far:2802:0848 */
                        /* far:02cc:1404 */
                  /* far:0da4:0252 */
                 /* far:0da4:0122 */
                     /* far:0169:0794 */
                   /* far:2203:0d21 */
                   /* far:2203:0d0d */
                   /* far:2203:0d17 */
                   /* far:2203:0d2b */
                 /* far:052d:1234 */
                    /* far:25da:0016 */
               /* far:02cc:0394 */
                   /* far:02cc:07a2 */
                    /* far:02cc:0944 */
                       /* far:0f77:0002 */
               /* far:0b41:16fa */
                        /* far:1101:807a */
                  /* far:0000:0dd4 */
                         /* far:0284:0096 */
          /* far:1101:4d90 */
                    /* far:078a:213a */
                /* far:078a:24ee */
                     /* far:0000:125a */
         /* far:1101:6650 */
                  /* far:078a:0000 */
                   /* far:04a6:003e */
                 /* far:078a:13b2 */
                 /* far:04a6:0784 */
                        /* far:078a:12e4 */
            /* far:1ead:01a2 */
              /* far:2344:0f4e */
               /* far:0000:1268 */
                 /* far:078a:293a */
                         /* far:0dd7:02ce */
                  /* far:1101:8444 */
                        /* far:1101:83f0 */
                /* far:052d:109e */
                    /* far:1de8:015a */
                 /* far:107d:000c */
                /* far:0b41:1a42 */
                    /* far:0b41:000c */
              /* far:2c3e:0f6c */
   /* far:2344:0a52 */
              /* far:2344:1f0e */
              /* far:2344:198e */
                     /* far:0dd7:016c */
              /* far:0000:0f70 */
               /* far:2344:1088 */
               /* far:2344:0fae */
                      /* far:0000:0cbc */
                         /* far:02cc:1d94 */
            /* far:02cc:0eb6 */
       /* far:2344:08f0 */
               /* far:02cc:07fc */
           /* far:02cc:027e */
                       /* far:02cc:116e */
                  /* far:02cc:08d6 */
                       /* far:02cc:090e */
                   /* far:0169:09ee */
                 /* far:2802:09ca */
                    /* far:2802:000e */
                    /* far:2802:0402 */
                  /* far:2802:0d56 */
                    /* far:0b41:06f8 */
                     /* far:0b41:075e */
                 /* far:101f:01ae */
                /* far:2a4e:0e44 */
                  /* far:101f:00a0 */
                /* far:29cf:00b2 */
                 /* far:29cf:0118 */
                /* far:29cf:01f4 */
                 /* far:29cf:06e8 */
                    /* far:0dd7:01ce */
        /* far:02cc:0dda */
                      /* far:02cc:1d42 */
              /* far:02cc:086a */
                   /* far:02cc:12e4 */
                 /* far:078a:0024 */
                      /* far:2c3e:0052 */
              /* far:2802:0e82 */
              /* far:2802:0c94 */
                 /* far:29cf:1b60 */
                      /* far:02cc:1102 */
                 /* far:2c3e:1674 */
                       /* far:02cc:1cf6 */
       /* far:2344:07bc */
                  /* far:04a6:02b0 */
               /* far:078a:2590 */
                    /* far:078a:218e */
                 /* far:0000:12de */
                       /* far:0dd7:04d0 */
              /* far:0dd7:03cc */
                     /* far:078a:20a8 */
                    /* far:1101:842c */
 /* far:0b41:0f62 */
        /* far:0b41:0f1e */
               /* far:02cc:020a */
                 /* far:0b41:0e60 */
           /* far:0b41:0e1c */
                     /* far:02cc:07c6 */
               /* far:052d:0fc4 */
            /* far:0b41:190c */
                  /* far:02cc:1cac */
                     /* far:02cc:0360 */
 /* far:2344:08b4 */
                  /* far:29cf:08c6 */
            /* far:29cf:0c1c */
                  /* far:078a:28fe */
                 /* far:078a:2830 */
               /* far:078a:28c8 */
               /* far:078a:29a6 */




int Start(sbyte requested)
{
    CheckBPort();
    reset_level_overlays();
    TimeRequest = BackToLife = MsgTimer = MsgTimerStart = 0;
    if (!Resume) {
        Minutes = 75;
        SixtiethsCtr = 719;
        OrigLifePts = 3;
        if (frame_diagnostics && find_option(level_option)) {
            sbyte health = Level.level_number;
            health = health < 3 ? 3 : health;
            OrigLifePts = health > 12 ? 12 : health;
        }
    }
    return requested = Restart(requested);
}

int Restart(sbyte requested)
{
    int reload;
    sbyte previous_request;

    while (requested) {
        if (requested == -1 || requested > 14)
            break;
        if (!Resume) {
            reload = CheckPlayNIS(LevelIdx, requested);
            if (nis_skip_requested) {
                previous_request = StartLevel;
                StartLevel = requested;
                requested = poll_and_clear_pending_state();
                StartLevel = previous_request;
                reload = 1;
            }
            if (requested != LevelIdx)
                reload = 1;
        } else {
            reload = 1;
        }

        if (reload) {
            reset_level_overlays();
            LoadLevel(requested);
        } else {
            LoadLevelMinimum(LevelIdx);
        }
        InitGuards();
        InitCollisionDetection();
        InitInput();
        InitLevelSpecific();
        Room.number = 0;
        NeedShadow = Stunned = TrobCt = MobCt = 0;
        CueSong = CueSnd = -1;
        Kid.life_delta = 0;
        RoomOpp[0].direction = 0x56;
        StartKid();
        Entrance();
        requested = FirstFrame(requested);
        if (requested != -1) {
            if (GameState.recording_mode)
                StartPlayingRecordedGame();
            requested = MainLoop();
            if (requested != -1)
                ZeroPeels();
            if (GameState.recording_mode) {
                CheckRecordedGamePlayer();
                requested = LevelIdx = 0;
                NextLevel = 0;
            }
        }
    }
    return requested;
}

void reset_level_overlays(void)
{
    reset_actor_alert();
    reset_overlay_state();
    reset_shadow_overlay_state();
    reset_barrier_state();
}

void StartKid(void)
{
    int seq;
    sbyte repeat;

    Char.character_kind = 0;
    if (Level.level_number == 7)
        StartKidSpecialEntrance();
    CutRoom = Char.room = Level.entry_room;
    Char.column = Level.entry_cell % 10;
    Char.row = Level.entry_cell / 10;
    Char.direction = ~Level.entry_facing;
    seq = SetRebirthPtKidStatus();
    Char.x = ColEdgeX[Char.column + 5] + 14;
    Char.life_initial = Char.life_points = OrigLifePts;
    repeat = 0;
    if (seq == -1) {
        if (Level.environment_kind == 5 && Level.entry_room == 4) {
            seq = 4;
            repeat = 9;
        } else if (Level.environment_kind == 1) {
            seq = 124;
        } else if (Level.entry_room == 27 && Level.level_number == 6) {
            seq = 2;
            Char.x = 240;
        } else if (Level.entry_room == 22 && Level.level_number == 10) {
            seq = 2;
            Char.x = 310;
        } else if (Level.level_number == 14 && Char.room == 1 && Level.level_number == 14) {
            seq = 2;
            Char.x = 306;
        } else {
            seq = 5;
        }
    }
    JumpSeq(seq);
    if (repeat) {
        do {
            AnimChar();
        } while (--repeat);
    }
    StartKid1();
    KidInfo.death_countdown = -1;
    KidInfo.shadow_timer = 0;
    Player = 1;
}

void StartKid1(void)
{
    Char.slot = 10;
    Char.character_kind = 0;
    Char.alert_delay = 3;
    Char.sequence_group = 0;
    *(volatile int *)&Char.image_bank = 0;
    Char.opponent_index = Char.alive = -1;
    ResetCharY();
    ClearCharVel();
    DroppedOut = Screamed = WeightlessTimer = ShakeFloor = 0;
    Char.stance = KidInfo.sword_kind == -1 ? -1 : 0;
    AnimChar();
    Kid = Char;
}

void StartKidSpecialEntrance(void)
{
    if (KidInfo.entrance_mode == 2) {
        Level.entry_room = 4;
        Level.entry_cell = 25;
        Level.cell_info[31][24].modifier = (word) ((byte *)&Level.cell_info[31][24].modifier)[1] << 8;
    } else {
        Level.entry_room = 32;
        Level.entry_cell = 24;
        Level.cell_info[3][25].modifier = (word) ((byte *)&Level.cell_info[3][25].modifier)[1] << 8;
    }
}

void Entrance(void)
{
    sbyte cell;

    if (Kid.room == Level.entry_room || Level.level_number == 7 || Level.level_number == 6) {
        select_room_arrays(Kid.room);
        cell = 0;
        while (BlueT[cell] != 17) {
            if (++cell >= 30)
                return;
        }
        CloseExit(Kid.room, cell);
        hGetSnd(10009);
    }
}

int FirstFrame(sbyte requested)
{
    int result;
    CutRoom = Kid.room;
    result = PrepCut(requested != -2 ? 1 : 0);
    if (result == -2) {
        if (!IsLevelStartWithNISScreen())
            EraseRect(&ScreenPortRect);
        else
            ClrMessage(1);
        MsgTimer = MsgTimerStart = 0;
        DoCleanCut();
        DrawKidMeter(Kid.life_points, Kid.life_initial);
        Timer[1] = 5;
        while (Timer[1]) {
            ClearKeyboard();
        }
    }
    return result;
}

void DoCleanCut(void)
{
    CutPlan = 0;
    if (BlackOut) {
        FillRect(&GamePlayRect, 12);
    } else {
        NewRoom = 1;
        ClearKeyboard();
        DoSure();
        ClearKeyboard();
        Graf = BPort;
        CheckRoomSpecific(); DrawAll();
        ClearKeyboard();
        if (CustomRoom)
            UnloadCustomRoomShapes(0);
        ClearKeyboard();
        BlackOut = NewRoom = 0;
        memset(Ct, 0, sizeof(Ct));
        DoFast();
        ClearKeyboard();
        DrawAll();
        ClearKeyboard();
        if (Invert)
            InvertBitMap(&BPort->map);
        ClearKeyboard();
        SetOppBanks();
        PrinceCopy(&GamePlayRect);
        RestoreGamePlayPalette();
        ClearKeyboard();
        if (Invert)
            InvertBitMap(&BPort->map);
        ClearKeyboard();
    }
    CutTimer = 2;
}

int MainLoop(void)
{
    sbyte request;

    do {
        ResetFrameVars();
        request = NextFrame();
        if (request == -2) {
            if (RestartLevel) {
                request = ResetLevel();
            } else if (LevelIdx != NextLevel && !IsLeaveLevelMusicOn() &&
                       (!SndOn(current_effect + 10000) ||
                        current_effect == 4 || current_effect == 7 || current_effect == 54)) {
                if (NextLevel == -1 || (Level.level_number != 9 && Level.level_number != 5))
                    ResetSnd(0);
                OrigLifePts = Kid.life_initial;
                request = NextLevel;
                RemoveRebirthPt();
            } else {
                request = FrameAdv();
                if (request == -2) {
                    if (Timer[1]) {
                        if (GameState.fade_active)
                            --GameState.fade_active;
                        WaitTimerNoPoll(1);
                    } else if (GameState.fade_active < 20) {
                        ++GameState.fade_active;
                    }
                }
            }
        }
        limit_frame_rate();
    } while (request == -2);
    return request;
}

int NextFrame(void)
{
    int result;

    AnimMobs();
    AnimTrobs();
    if (Level.preferred_opponent == 2 || RoomOpp[0].character_kind == 10)
        OpponentRise();
    CheckOppGenPts(Room.number);
    CheckAlert();
    result = DoKid();
    if (!result) {
        DoShad();
        if (!BackToLife && Room.number) {
            CheckStrike();
            CheckStab();
        }
        AddSFX();
        CheckRebirthPts();
        CheckLevelFrameFunc();
        ChangeMeters();
        CutCheck();
        result = PrepCut(0);
        if (result == -2) {
            CutOpponent();
            KeepTime();
            if (!Minutes) {
                YouLose();
                StartLevel = 0;
                result = -1;
            }
            return result;
        }
    } else if (result != -1) {
        result = -2;
        DoShad();
        CheckLevelFrameFunc();
        ChangeMeters();
    }
    return result;
}


int DoKid(void)
{
    sbyte opponent;
    int saved_result;
    int result;

    LoadKid();
    if (!IsUndefinedRoom()) {
        if ((opponent = Char.opponent_index) == -1) {
            if ((opponent = GetOppIdxClosestToKid(1)) == -1)
                opponent = 0;
        }
        LoadKidwOpp(opponent);
        RereadCells();
        result = CtrlPlayer();
        if (result != -1) {
            if (!RestartLevel) {
                AnimChar();
                if (Char.frame) {
                    Gravity();
                    AddCharVel();
                    SetupChar();
                    RereadCells();
                    GetCharEdges();
                    FirstGuard();
                    CheckBarr();
                    Collisions();
                    CheckGateForKid();
                    CheckFloor();
                    CheckPress();
                    switch (Level.environment_kind) {
                    case 2:
                        CheckSpikes();
                        CheckTrapDoor();
                        if (!BackToLife)
                            CheckImpale();
                        /* fall through */
                    case 4:
                        CheckScythe();
                        if (!BackToLife)
                            CheckScytheHit();
                        break;
                    case 3:
                        CheckSpikes();
                        if (!BackToLife) {
                            CheckDartHit();
                            CheckImpale();
                            CheckLavaBoil();
                        }
                        break;
                    }
                    ShakeLoose();
                }
                result = 0;
            } else {
                result = 1;
            }
        }
        saved_result = result;
        if (KidInfo.death_countdown >= 0) {
            if (!KidInfo.death_countdown && Kid.alive < 0 && Kid.action != 3 && Kid.action != 4)
                AddSound(26);
            --KidInfo.death_countdown;
        }
    } else {
        saved_result = CtrlPlayer();
        FallingInUndefinedRoom();
    }
    Kid = Char;
    {
        int *status = &saved_result;
        return *status;
    }
}

void DoShad(void)
{
    sbyte index, count;

    if (!Room.number)
        return;
    index = 0;
    count = Level.room_objects[Room.number - 1].opponent_count;

    if (count > 0) do {
        LoadShad(index);
        if (Char.direction == 0x56 || IsUndefinedRoom() ||
            (Char.y > 254 && !Level.room_links[Char.room - 1].below)) {
            FallingInUndefinedRoom();
        } else {
            byte old_room;
            LoadShadwOpp(index);
            RereadCells();
            ShadCtrl();
            old_room = Room.number;
            if (old_room != Char.room) {
                Room.number = Char.room;
                GetRooms();
            }
            AnimChar();
            if (Char.character_kind == 2)
                CheckMultipleDeadBodies();
            if ((Char.x >= 34 && Char.x <= 545) ||
                (Char.row && Level.level_number == 5 &&
                 (Char.room == 10 || Char.room == 7 || Char.room == 12)) ||
                ((Char.room == 7 || Char.room == 8) && Level.environment_kind == 6)) {
                Gravity(); AddCharVel();
                SetupChar(); RereadCells();
                GetCharEdges();
                if (Char.action != 9) {
                    EnemyCollide(); CheckGateForOpp();
                    CheckFloor(); CheckPress();
                    switch (Level.environment_kind) {
                    case 2:
                        CheckSpikes();
                        CheckTrapDoor();
                        CheckImpale();
                        /* fall through */
                    case 4:
                        CheckScythe();
                        CheckScytheHit();
                        break;
                    case 3:
                        CheckSpikes();
                        CheckDartHit();
                        CheckImpale();
                        CheckLavaBoil();
                        break;
                    case 6:
                        CheckFireballHitChar();
                        break;
                    }
                } else {
                    Char.row = GetRow(Char.y);
                }
            }
            if (Char.character_kind == 12)
                HasJinneeRunAway();
            if (old_room != Char.room) {
                Room.number = old_room;
                GetRooms();
            }
        }
        if (Level.environment_kind == 6 && count != Level.room_objects[Room.number - 1].opponent_count) {
            --count;
            --index;
        } else {
            SaveShad();
        }
    } while (++index < count);

    index = 0;
    if (count > 0) do {
        if (RoomOpp[index].direction == 0x56) {
            LoadShad(index);
            RemoveOppFromRoom(Room.number, index);
            if (Char.slot < Kid.opponent_index)
                --Kid.opponent_index;
            else if (Char.slot == Kid.opponent_index)
                Kid.opponent_index = GetOppIdxClosestToKid(Char.direction);
            --index;
            --count;
        }
    } while (++index < count);
    AlertGuard = 0;
}

int FrameAdv(void)
{
    int result, phase;

    ++FrameNum;
    result = -2;
    if (DoBlackOut) {
        DoCleanCut();
        DoBlackOut = 0;
    } else if (CutPlan) {
        Room.number = CutRoom;
        DoCleanCut();
    } else if (RedrawFlag) {
        RedrawFlag = 0;
        LoadCustomRoomData(Room.number);
        DoCleanCut();
        DrawMeters();
    } else {
        memset(Ct, 0, sizeof(Ct)); DoFast(); DrawAll();
        if (BlackOut) EraseRect(&GamePlayRect);
        if (Invert)
            InvertBitMap(&BPort->map);
        CopyDRects();
        if (Invert) {
            InvertBitMap(&BPort->map);
            if (Invert == 1)
                ToggleInvertState();
            else if (Kid.alive < 0)
                --Invert;
        }
    }
    PlaySounds();
    PlayAmbient(&ambient_state);
    if (MsgTimer == 1) {
        if (MsgTimerStart == 36 || MsgTimerStart == 600) {
            StartLevel = 0;
            result = -1;
        } else {
            ClrMessage(1);
        }
    } else if (MsgTimer && MsgTimerStart != 1188) {
        if (NeedShadow != -1)
            --MsgTimer;
        if (MsgTimerStart == 600) {
            if (Room.number == 4 && Level.level_number == 13 && !NeedShadow)
                NeedShadow = 1;
            if ((word)MsgTimer < 120) {
                phase = (word)MsgTimer % 12;
                if (phase > 3) {
                    ClrMessage(0);
                } else if (phase == 3) {
                    DrawRestartMessage();
                    SetSnd(-2);
                }
            }
        }
    }
    return result;
}

void ResetFrameVars(void)
{
    sbyte index, count;

    Timer[1] = Kid.stance == 1 ? 6 : 5;
    Kid.life_delta = 0;
    Kid.previous_bounds = Kid.bounds;
    if (Kid.sequence_group == 8 && (Kid.sequence_id != 52 || Kid.sequence_position))
        Kid.sequence_group = 0;
    index = 0;
    count = Level.room_objects[Room.number - 1].opponent_count;
    if (count > 0) do {
        LoadShad(index);
        Char.life_delta = 0;
        Char.previous_bounds = Char.bounds;
        if (Char.sequence_group == 8)
            Char.sequence_group = 0;
        SaveShad();
    } while (++index < count);
    MiscTimers();
}

void QuickFG(void)
{
    if (Char.slot == 10)
        MarkBufsForRect(MarkFRedraw, &Char.bounds, -1);
}

void QuickFloor(void)
{
    sbyte left_edge;
    int floor = Char.action;
    word seq = Char.frame;

    if (seq >= 135 && seq < 145 && !Char.direction) {
        MarkHalf(IndexCell(Cell.last_col, Cell.top_row));
        return;
    }
    if (floor != 2 && floor != 3 && floor != 4 && floor != 6 && seq != 79 && seq != 81) {
        if ((Char.character_kind == 7 || Char.character_kind == 8) &&
            (Char.sequence == 137 || Char.sequence == 138 || Char.sequence == 143 ||
             Char.sequence <= 144 || Char.sequence == 143 || Char.sequence == 144)) {
            if ((Cell.column = Cell.last_col) >= Cell.first_col) do {
                MarkFloor(IndexCell(Cell.column, Cell.top_row), &Char.bounds);
                left_edge = Cell.first_col;
                --Cell.column;
            } while (Cell.column >= left_edge);
            return;
        }
        return;
    }
    if ((Cell.column = Cell.last_col) >= Cell.first_col) do {
        if (floor != 2)
            MarkFloor(IndexCell(Cell.column, Cell.bottom_row), &Char.bounds);
        if (Cell.bottom_row != Cell.top_row &&
            !(Char.sequence == 28 && (Char.room == 7 || Char.room == 8) && Level.environment_kind == 6))
            MarkFloor(IndexCell(Cell.column, Cell.top_row), &Char.bounds);
        left_edge = Cell.first_col;
        --Cell.column;
    } while (Cell.column >= left_edge);
}


void CheckRebirthPts(void)
{
    int index;

    if (Kid.room && Kid.action != 4 && Kid.alive < 0 && abs(Kid.life_delta) < Kid.life_points &&
        Kid.character_kind != 1 && (Kid.room != 9 || Level.level_number != 8 || KidInfo.sword_kind == 1)) {
        for (index = 0; index < 2; ++index) {
            if (Kid.room == Level.rebirth_points[index].room && FirstInRow(Kid.row) + Kid.column == Level.rebirth_points[index].cell &&
                (!hRebirthSaved || hRebirthSaved->point_id - index != 1))
                SaveRebirthPtStatus(index + 1);
        }
    }
}

void ShakeLoose(void)
{
    if (ShakeFloor) {
        ShakeEm(Char.room, (sbyte)(Char.row - (ShakeFloor >= 1 ? 1 : 0)));
        ShakeFloor = 0;
    }
}

void MiscTimers(void)
{
    if (BackToLife > 0)
        --BackToLife;
    if (WeightlessTimer)
        --WeightlessTimer;
}

void FirstGuard(void)
{
    if (Opp.direction != 0x56 && Opp.alert_delay >= 2 && Char.stance != 1 &&
        Opp.direction != Char.direction && Opp.stance && Opp.action < 2 &&
        Opp.character_kind != 11 && Opp.character_kind != 7 && Opp.character_kind != 8 &&
        !(Opp.character_kind == 12 && Opp.frame >= 179 && Opp.frame <= 235) &&
        !(Opp.character_kind == 4 && Opp.frame == 206) &&
        !(Opp.character_kind == 2 && Opp.sequence == 110) &&
        Char.alive < 0 && abs(GetOppDist()) <= 15) {
        ResetCharY();
        Char.vertical_step = 0;
        if (Char.frame >= 246 && Char.frame <= 261) {
            Char.x = AddCharX(-6);
            return;
        }
        JumpSeq(47);
        AnimChar();
    }
}

void InitGuards(void)
{
    byte room, limit;
    int index;
    OPPONENT *guard;

    room = 1;
    if (Level.room_count >= 1) {
        do {
            index = 0;
            do {
                guard = GetOppInfoIdx(room, index++);
                if (guard)
                    guard->saved_sequence = 0;
            } while (guard);
            limit = Level.room_count;
            ++room;
        } while (room <= limit);
    }
}

void InitLevelSpecific(void)
{
    if (Level.environment_kind == 3) {
        InitLavaGlobals();
        if (Level.level_number == 5)
            InitBridgeGlobals();
    } else if (Level.environment_kind == 4) {
        GameState.throne_nis_played = GameState.throne_song_added = GameState.plateau_loaded = 0;
    } else if (Level.environment_kind == 6) {
        GameState.escher_room_eight_seen = 0;
    }
}

              /* far:02cc:00f0 */

void CheckAlert(void)
{
    sbyte swap, index, left, right, kind, second, first, count;
    word sequence;

    if (Kid.room) {
        sequence = Kid.frame;
        first = GetOppIdxClosestToKid(0);
        second = GetOppIdxClosestToKid(-1);
        count = Level.room_objects[Kid.room - 1].opponent_count;
        index = 0;
        if (count > 0) do {
            LoadShad(index);
            if (!sequence || (sequence >= 217 && sequence < 226) || Kid.alive >= 0 || Char.alive >= 0 ||
                Char.direction == 0x56 || (Kid.row != Char.row && Char.character_kind != 7 && Char.character_kind != 8) ||
                (Char.character_kind == 11 && Char.sequence_group == 6) || (Kid.character_kind == 1 && Level.environment_kind == 2)) {
                Char.alert_delay = 0;
            } else {
                Char.alert_delay = 3;
                left = Kid.column;
                right = Char.column;
                if (left > right) {
                    swap = left;
                    left = right;
                    right = swap;
                }
                if (ReadCell(Kid.room, right, Kid.row) == 4)
                    --right;
                if (left < right) {
                    if (left <= right) do {
                        if (Char.alert_delay != 3)
                            break;
                        kind = ReadCell(Kid.room, left, Kid.row);
                        if (IsWall(kind) &&
                            !(Char.character_kind == 11 && kind == 7 && (Cell.modifier & 0x80))) {
                            Char.alert_delay = 0;
                        } else if ((kind == 4 && (byte)Cell.modifier < 0x70) ||
                                   (IsImpassable(kind) && Char.character_kind != 10) ||
                                   (IsSpace(kind) && Char.character_kind != 10 &&
                                    Char.character_kind != 7 && Char.character_kind != 8)) {
                            Char.alert_delay = 1;
                        }
                    } while (right >= ++left);
                }
                if (Char.alert_delay == 3 && Char.character_kind != 11) {
                    if (index != second && index != first)
                        Char.alert_delay = 2;
                    else if (Char.character_kind == 10)
                        Char.stance = 1;
                }
            }
            SaveShad();
        } while (++index < count);
    }
}

void CheckLevelFrameFunc(void)
{
    unsigned long address = (unsigned long)LevelFrameFunc[Level.environment_kind];
    if (address)
        ((frame_proc)address)();
}

sbyte ResetLevel(void)
{
    RestartLevel = 0;
    if (LevelIdx == NextLevel) {
        SaveGamePlayPalette();
        EraseRect(&GamePlayRect);
        if (StartLevel)
            return LevelIdx;
        return 0;
    }
    return NextLevel;
}

                   /* far:0284:0226 */

void YouLose(void)
{
    int result;

    ResetSnd(0);
    result = PlayNIS(28);
    Minutes = 0;
    quote_index = 8;
    if (result == 2)
        reset_level_overlays();
}

const int TopRowRect[4] = { 0, 0, 3, 320 };
Rect MsgRect = { 193, 98, 202, 235 };
int NeedShadow = 0;
sbyte LevelIdx = 0;
int unused_flag = 1;                                /* never referenced */
