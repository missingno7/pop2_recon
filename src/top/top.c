/* TOP.C candidate, DOS frame 0e56. Definitions follow original order.
 * Address comments identify the observed operands behind external declarations. */
extern signed char LevelIdx;                          /* ds:0a18 */
#pragma pack(1)
struct char_bounds { int top, left, bottom, right; };
typedef struct {
    signed char slot;                  /* 00 */
    signed char direction;             /* 01 */
    int x, y;                          /* 02, 04 */
    signed char character_kind;        /* 06 */
    unsigned int frame;                /* 07 */
    signed char column, row;           /* 09, 0a */
    signed char action;                /* 0b */
    signed char horizontal_step;       /* 0c */
    signed char vertical_step;         /* 0d */
    signed char room;                  /* 0e */
    signed char field_0f;              /* 0f */
    signed char stance;                /* 10 */
    signed char alive;                 /* 11 */
    signed char life_points;           /* 12 */
    signed char life_initial;          /* 13 */
    signed char life_delta;            /* 14 */
    int sequence_position;             /* 15 */
    int sequence_id;                   /* 17 */
    unsigned int sequence;             /* 19 */
    struct char_bounds bounds;                    /* 1b: top, left, bottom, right */
    signed char alert_delay;           /* 23 */
    int sequence_group;                /* 24 */
    int previous_x, previous_y;        /* 26, 28 */
    unsigned int previous_frame;       /* 2a */
    struct char_bounds previous_bounds;           /* 2c: top, left, bottom, right */
    volatile int field_34;                      /* 34 */
    int previous_mode;                 /* 36 */
    unsigned char behavior;            /* 38 */
    signed char opponent_index;        /* 39 */
    unsigned char field_3a;            /* 3a */
    unsigned char unknown_3b[5];        /* 3b..3f */
} CHARREC;
struct GetOppInfoIdx { char prefix[7]; int active; };
typedef void (far pascal *frame_proc)(void);

extern CHARREC Opp;                    /* ds:5c40 */
extern CHARREC Char;                     /* ds:5c00 */
extern CHARREC RoomOpp[5];                /* ds:5cc0 */
#pragma pack(1)
typedef struct {
    unsigned int modifier, unknown_02;
} LEVELCELL;

typedef struct {
    unsigned char left, right, above, below;
} ROOMLINKS;

typedef struct {
    signed char opponent_count;
    unsigned char opponent_data[5][23];
} ROOMOBJECTS;

typedef struct {
    signed char room, cell;
} REBIRTHPOINT;

typedef struct {
    signed char count;
    unsigned char spawn_flags;
    unsigned char header[2];
    unsigned char points[3][10];
} ROOMGENERATORS;

typedef struct {
    unsigned char cell_kinds[32][30];                  /* 0000 */
    LEVELCELL cell_info[32][30];                       /* 03c0 */
    unsigned char unknown_12c0[0x500];                 /* 12c0 */
    ROOMLINKS room_links[32];                          /* 17c0 */
    unsigned char room_count;                         /* 1840 */
    unsigned char unknown_1841[4];
    unsigned char environment_kind;                  /* 1845 */
    unsigned char unknown_1846;
    signed char level_number;                         /* 1847 */
    int background_overlays[8];                       /* 1848 */
    unsigned char unknown_1858[4];
    int actor_phase;                                  /* 185c */
    unsigned char unknown_185e[2];
    signed char entry_room, entry_cell, entry_facing;    /* 1860-1862 */
    unsigned char unknown_1863[3];
    unsigned char preferred_opponent;                /* 1866 */
    ROOMOBJECTS room_objects[32];                      /* 1867 */
    REBIRTHPOINT rebirth_points[2];                    /* 26e7 */
    unsigned char unknown_26eb[0x0e];
    ROOMGENERATORS generators[32];                     /* 26f9 */
    unsigned char unknown_2b39[0x3c0];                 /* 2b39-2ef8 */
} LEVELREC;
#pragma pack()

extern frame_proc LevelFrameFunc[];             /* ds:06d4 */
extern int far *hRebirthSaved;                      /* ds:5bfc */
signed char StartLevel;                   /* ds:6ccc */
signed char CutRoom;                   /* ds:6ccd */
signed char OrigLifePts;                /* ds:6cd1 */
extern int Resume;                            /* ds:5e00 */
typedef struct {
    signed char argument, entrance_mode, sword_kind, death_countdown;
    int field_4, shadow_timer;
} KIDINFO;
extern KIDINFO KidInfo;
extern int BPort;                      /* ds:5e0c */
extern int Player;                           /* ds:5e0e */
extern int RedrawFlag;                            /* ds:5e18 */
extern int DoBlackOut;                           /* ds:5e1a */
extern int BlackOut;                         /* ds:5e1c */
typedef struct {
    unsigned char enabled;
    signed char cue;
    int song;
    int delay;
    int reserved;
} AmbientState;
extern AmbientState ambient_state;                           /* ds:5e22 */
unsigned Minutes;                 /* ds:5e2a */
int RestartLevel;                        /* ds:5e2c */
int TimeRequest;                             /* ds:5e2e */
#pragma pack(1)
typedef struct {
    unsigned char number;                             /* 00 */
    unsigned char left, right, above, below;           /* 01-04 */
    unsigned char above_left, above_right;             /* 05-06 */
    unsigned char below_left, below_right;             /* 07-08 */
    unsigned char custom;                             /* 09 */
} ROOMREC;
#pragma pack()
extern ROOMREC Room;
extern LEVELREC Level;
int BackToLife;                           /* ds:5e3e */
int MsgTimer;                    /* ds:5e40 */
int MsgTimerStart;                       /* ds:5e42 */
int SixtiethsCtr;                        /* ds:5e44 */
int NextLevel;                             /* ds:5e46 */
int CutPlan;                           /* ds:5e5e */
unsigned long FrameNum;                             /* ds:5e60 */
int Invert;                         /* ds:5e8e */
int WeightlessTimer;                       /* ds:5e90 */
extern volatile int Timer[4];                             /* ds:264e */
typedef struct {
    int control_mode, fade_active, playing_nis, recording_mode, skip_opening, field_0a;
    int plateau_loaded, throne_song_added, throne_nis_played, escher_room_eight_seen, nis_room_restored;
} GAMESTATE;
extern GAMESTATE GameState;
int CutTimer;                              /* ds:6a4a */
int GuardTimer, Refract, JustBlocked, CutDir;
extern int NewRoom;                          /* ds:6266 */
extern int Graf;                           /* ds:25c0 */
extern int CustomRoom;                          /* ds:0228 */
extern int NeedShadow;                       /* ds:0a16 */
extern int current_effect;                         /* ds:0902 */
extern int CueSnd;                             /* ds:08fe */
extern int CueSong;                             /* ds:0900 */
extern int frame_diagnostics;                           /* ds:1142 */
extern int nis_skip_requested;                           /* ds:1504 */
int ShakeFloor;                             /* ds:6296 */
int AlertGuard;                      /* ds:6298 */
int DroppedOut;                           /* ds:629a */
int Screamed;                            /* ds:629c */
int Stunned;                           /* ds:629e */
extern int MobCt;                      /* ds:62de */
extern int TrobCt;                     /* ds:67cc */
extern char quote_index;                            /* ds:01d6 */
#pragma pack(1)
typedef struct {
    signed char kind;                                 /* 00 */
    unsigned int modifier;                            /* 01 */
    signed char index;                                /* 03 */
    unsigned char room;                               /* 04 */
    signed char column, row;                          /* 05-06 */
    signed char first_col, last_col;                   /* 07-08 */
    unsigned char top_row, bottom_row;                 /* 09-0a */
} CELLREC;
#pragma pack()
CELLREC Cell;
extern int ColEdgeX[20];
extern char Ct[10];
extern unsigned char *BlueT;                   /* ds:6294 */
static char level_option[] = "LEVEL";
struct char_bounds GamePlayRect = { 0, 0, 192, 320 };
extern char ScreenPortRect[];                         /* ds:1fe0 */

extern void CheckBPort(void);              /* far:0a5c:004e */
extern int find_option(char far *);                  /* far:1101:29aa */
extern int CheckPlayNIS(int level, int previous_level);             /* far:0284:000e */
extern char poll_and_clear_pending_state(void);             /* far:0000:0210 */
extern void LoadLevel(char level);                       /* far:0a5c:01b4 */
extern void LoadLevelMinimum(char);               /* far:0a5c:02f4 */
extern void InitCollisionDetection(void);        /* far:2802:0848 */
extern void InitInput(void);                        /* far:02cc:1404 */
extern void StartPlayingRecordedGame(void);                  /* far:0da4:0252 */
extern void CheckRecordedGamePlayer(void);                 /* far:0da4:0122 */
extern void ZeroPeels(void);                     /* far:0169:0794 */
extern void reset_actor_alert(void);                   /* far:2203:0d21 */
extern void reset_overlay_state(void);                   /* far:2203:0d0d */
extern void reset_shadow_overlay_state(void);                   /* far:2203:0d17 */
extern void reset_barrier_state(void);                   /* far:2203:0d2b */
extern int SetRebirthPtKidStatus(void);                 /* far:052d:1234 */
extern void JumpSeq(int);                    /* far:25da:0016 */
extern void AnimChar(void);               /* far:02cc:0394 */
extern void ResetCharY(void);                   /* far:02cc:07a2 */
extern void ClearCharVel(void);                    /* far:02cc:0944 */
extern void select_room_arrays(char);                       /* far:0f77:0002 */
extern void CloseExit(char, char);               /* far:0b41:16fa */
extern void hGetSnd(int);                        /* far:1101:807a */
extern int PrepCut(int);                  /* far:0000:0dd4 */
extern int IsLevelStartWithNISScreen(void);                         /* far:0284:0096 */
extern void EraseRect(char far *);          /* far:1101:4d90 */
extern void ClrMessage(int);                    /* far:078a:213a */
extern void DrawKidMeter(int, int);                /* far:078a:24ee */
extern void ClearKeyboard(void);                     /* far:0000:125a */
extern void FillRect(char far *, int);         /* far:1101:6650 */
extern void DoSure(void);                  /* far:078a:0000 */
extern void CheckRoomSpecific(void);                   /* far:04a6:003e */
extern void DrawAll(void);                 /* far:078a:13b2 */
extern void UnloadCustomRoomShapes(int);                 /* far:04a6:0784 */
extern void DoFast(void);                        /* far:078a:12e4 */
extern void InvertBitMap(char far *);            /* far:1ead:01a2 */
extern void SetOppBanks(void);              /* far:2344:0f4e */
extern void PrinceCopy(char far *);               /* far:0000:1268 */
extern void RestoreGamePlayPalette(void);                 /* far:078a:293a */
extern int IsLeaveLevelMusicOn(void);                         /* far:0dd7:02ce */
extern int SndOn(int);                  /* far:1101:8444 */
extern void ResetSnd(int);                        /* far:1101:83f0 */
extern void RemoveRebirthPt(void);                /* far:052d:109e */
extern void WaitTimerNoPoll(int);                    /* far:1de8:015a */
extern void limit_frame_rate(void);                 /* far:107d:000c */
extern void AnimMobs(void);                /* far:0b41:1a42 */
extern void AnimTrobs(void);                    /* far:0b41:000c */
extern void OpponentRise(void);              /* far:2c3e:0f6c */
extern void CheckOppGenPts(unsigned char);   /* far:2344:0a52 */
extern void CheckStrike(void);              /* far:2344:1f0e */
extern void CheckStab(void);              /* far:2344:198e */
extern void AddSFX(void);                     /* far:0dd7:016c */
extern void ChangeMeters(void);              /* far:0000:0f70 */
extern void CutCheck(void);               /* far:2344:1088 */
extern void CutOpponent(void);               /* far:2344:0fae */
extern void KeepTime(void);                      /* far:0000:0cbc */
extern void LoadKid(void);                         /* far:02cc:1d94 */
extern int IsUndefinedRoom(void);            /* far:02cc:0eb6 */
extern signed char GetOppIdxClosestToKid(signed char);       /* far:2344:08f0 */
extern void LoadKidwOpp(signed char);               /* far:02cc:07fc */
extern void RereadCells(void);           /* far:02cc:027e */
extern int CtrlPlayer(void);                       /* far:02cc:116e */
extern void Gravity(void);                  /* far:02cc:08d6 */
extern void AddCharVel(void);                       /* far:02cc:090e */
extern void SetupChar(void);                   /* far:0169:09ee */
extern void GetCharEdges(void);                 /* far:2802:09ca */
extern void CheckBarr(void);                    /* far:2802:000e */
extern void Collisions(void);                    /* far:2802:0402 */
extern void CheckGateForKid(void);                  /* far:2802:0d56 */
extern void CheckFloor(void);                    /* far:0b41:06f8 */
extern void CheckPress(void);                     /* far:0b41:075e */
extern void CheckSpikes(void);                 /* far:101f:01ae */
extern void CheckTrapDoor(void);                /* far:2a4e:0e44 */
extern void CheckImpale(void);                  /* far:101f:00a0 */
extern void CheckScythe(void);                /* far:29cf:00b2 */
extern void CheckScytheHit(void);                 /* far:29cf:0118 */
extern void CheckDartHit(void);                /* far:29cf:01f4 */
extern void CheckLavaBoil(void);                 /* far:29cf:06e8 */
extern void AddSound(int);                    /* far:0dd7:01ce */
extern void FallingInUndefinedRoom(void);        /* far:02cc:0dda */
extern void LoadShad(char);                      /* far:02cc:1d42 */
extern void LoadShadwOpp(char);              /* far:02cc:086a */
extern void ShadCtrl(void);                   /* far:02cc:12e4 */
extern void GetRooms(void);                 /* far:078a:0024 */
extern void CheckMultipleDeadBodies(void);                      /* far:2c3e:0052 */
extern void EnemyCollide(void);              /* far:2802:0e82 */
extern void CheckGateForOpp(void);              /* far:2802:0c94 */
extern void CheckFireballHitChar(void);                 /* far:29cf:1b60 */
extern char GetRow(int);                      /* far:02cc:1102 */
extern void HasJinneeRunAway(void);                 /* far:2c3e:1674 */
extern void SaveShad(void);                       /* far:02cc:1cf6 */
extern void RemoveOppFromRoom(unsigned char, int);       /* far:2344:07bc */
extern void LoadCustomRoomData(char);                  /* far:04a6:02b0 */
extern void DrawMeters(void);               /* far:078a:2590 */
extern void CopyDRects(void);                    /* far:078a:218e */
extern void ToggleInvertState(void);                 /* far:0000:12de */
extern void PlaySounds(void);                       /* far:0dd7:04d0 */
extern void PlayAmbient(AmbientState *);              /* far:0dd7:03cc */
extern void DrawRestartMessage(void);                     /* far:078a:20a8 */
extern int SetSnd(unsigned);                    /* far:1101:842c */
extern void MarkBufsForRect(void far *, struct char_bounds *, char); /* far:0b41:0f62 */
extern void MarkFRedraw(void);        /* far:0b41:0f1e */
extern int IndexCell(char, char);               /* far:02cc:020a */
extern void MarkHalf(int);                 /* far:0b41:0e60 */
extern void MarkFloor(int, struct char_bounds *);           /* far:0b41:0e1c */
extern char FirstInRow(char);                     /* far:02cc:07c6 */
extern void SaveRebirthPtStatus(int);               /* far:052d:0fc4 */
extern void ShakeEm(char, char);            /* far:0b41:190c */
extern int GetOppDist(void);                  /* far:02cc:1cac */
extern int AddCharX(int);                     /* far:02cc:0360 */
extern struct GetOppInfoIdx *GetOppInfoIdx(int, int); /* far:2344:08b4 */
extern void InitLavaGlobals(void);                  /* far:29cf:08c6 */
extern void InitBridgeGlobals(void);            /* far:29cf:0c1c */
extern int IsWall(char);                  /* far:078a:28fe */
extern int IsImpassable(char);                 /* far:078a:2830 */
extern int IsSpace(char);               /* far:078a:28c8 */
extern void SaveGamePlayPalette(void);               /* far:078a:29a6 */
extern void *memset(void *, int, unsigned);
#pragma intrinsic(memset)
int cdecl abs(int);
#pragma intrinsic(abs)
int Restart(signed char requested);
void reset_level_overlays(void);
void StartKid(void);
void StartKid1(void);
void StartKidSpecialEntrance(void);
void Entrance(void);
int FirstFrame(signed char requested);
void DoCleanCut(void);
int MainLoop(void);
int NextFrame(void);
int DoKid(void);
extern CHARREC Kid;                      /* ds:5c80 */
void DoShad(void);
int FrameAdv(void);
void ResetFrameVars(void);
void QuickFG(void);
void QuickFloor(void);
void CheckRebirthPts(void);
void ShakeLoose(void);
void MiscTimers(void);
void FirstGuard(void);
void InitGuards(void);
void InitLevelSpecific(void);
void CheckAlert(void);
void CheckLevelFrameFunc(void);
signed char ResetLevel(void);
void YouLose(void);


int Start(signed char requested)
{
    CheckBPort();
    reset_level_overlays();
    TimeRequest = BackToLife = MsgTimer = MsgTimerStart = 0;
    if (!Resume) {
        Minutes = 75;
        SixtiethsCtr = 719;
        OrigLifePts = 3;
        if (frame_diagnostics && find_option(level_option)) {
            signed char health = Level.level_number;
            health = health < 3 ? 3 : health;
            OrigLifePts = health > 12 ? 12 : health;
        }
    }
    return requested = Restart(requested);
}

int Restart(signed char requested)
{
    int reload;
    signed char previous_request;

    if (requested) do {
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
    } while (requested);
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
    signed char repeat;

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
    Char.field_34 = 0;
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
        Level.cell_info[31][24].modifier = (unsigned) ((unsigned char *)&Level.cell_info[31][24].modifier)[1] << 8;
    } else {
        Level.entry_room = 32;
        Level.entry_cell = 24;
        Level.cell_info[3][25].modifier = (unsigned) ((unsigned char *)&Level.cell_info[3][25].modifier)[1] << 8;
    }
}

void Entrance(void)
{
    signed char cell;

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

int FirstFrame(signed char requested)
{
    int result;
    CutRoom = Kid.room;
    result = PrepCut(requested != -2 ? 1 : 0);
    if (result == -2) {
        if (!IsLevelStartWithNISScreen())
            EraseRect(ScreenPortRect);
        else
            ClrMessage(1);
        MsgTimer = MsgTimerStart = 0;
        DoCleanCut();
        DrawKidMeter(Kid.life_points, Kid.life_initial);
        Timer[1] = 5;
        if (Timer[1]) {
            do {
                ClearKeyboard();
            } while (Timer[1]);
        }
    }
    return result;
}

void DoCleanCut(void)
{
    CutPlan = 0;
    if (BlackOut) {
        FillRect((char *)&GamePlayRect, 12);
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
            InvertBitMap((char *)BPort);
        ClearKeyboard();
        SetOppBanks();
        PrinceCopy((char *)&GamePlayRect);
        RestoreGamePlayPalette();
        ClearKeyboard();
        if (Invert)
            InvertBitMap((char *)BPort);
        ClearKeyboard();
    }
    CutTimer = 2;
}

int MainLoop(void)
{
    signed char request;

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
    if (((signed char)Level.preferred_opponent) == 2 || RoomOpp[0].character_kind == 10)
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
    signed char opponent;
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
    signed char index, count;

    if (!Room.number)
        return;
    index = 0;
    count = Level.room_objects[Room.number - 1].opponent_count;

    if (count > 0) do {
        LoadShad(index);
        if (Char.direction == 0x56 || IsUndefinedRoom() ||
            (Char.y > 254 && !Level.room_links[(unsigned char)Char.room - 1].below)) {
            FallingInUndefinedRoom();
        } else {
            unsigned char old_room;
            LoadShadwOpp(index);
            RereadCells();
            ShadCtrl();
            old_room = Room.number;
            if (old_room != (unsigned char)Char.room) {
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
            if (old_room != (unsigned char)Char.room) {
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
        if (BlackOut) EraseRect((char *)&GamePlayRect);
        if (Invert)
            InvertBitMap((char *)BPort);
        CopyDRects();
        if (Invert) {
            InvertBitMap((char *)BPort);
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
            if ((unsigned)MsgTimer < 120) {
                phase = (unsigned)MsgTimer % 12;
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
    signed char index, count;

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
        MarkBufsForRect((void far *)MarkFRedraw, &Char.bounds, -1);
}

void QuickFloor(void)
{
    signed char left_edge;
    int floor = Char.action;
    unsigned seq = Char.frame;

    if (seq >= 135 && seq < 145 && !Char.direction) {
        MarkHalf(IndexCell(Cell.last_col, ((signed char)Cell.top_row)));
        return;
    }
    if (floor != 2 && floor != 3 && floor != 4 && floor != 6 && seq != 79 && seq != 81) {
        if ((Char.character_kind == 7 || Char.character_kind == 8) &&
            (Char.sequence == 137 || Char.sequence == 138 || Char.sequence == 143 ||
             (int)Char.sequence <= 144 || Char.sequence == 143 || Char.sequence == 144)) {
            if ((Cell.column = Cell.last_col) >= Cell.first_col) do {
                MarkFloor(IndexCell(Cell.column, ((signed char)Cell.top_row)), &Char.bounds);
                left_edge = Cell.first_col;
                --Cell.column;
            } while (Cell.column >= left_edge);
            return;
        }
        return;
    }
    if ((Cell.column = Cell.last_col) >= Cell.first_col) do {
        if (floor != 2)
            MarkFloor(IndexCell(Cell.column, ((signed char)Cell.bottom_row)), &Char.bounds);
        if (((signed char)Cell.bottom_row) != ((signed char)Cell.top_row) &&
            !(Char.sequence == 28 && (Char.room == 7 || Char.room == 8) && Level.environment_kind == 6))
            MarkFloor(IndexCell(Cell.column, ((signed char)Cell.top_row)), &Char.bounds);
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
                (!hRebirthSaved || *hRebirthSaved - index != 1))
                SaveRebirthPtStatus(index + 1);
        }
    }
}

void ShakeLoose(void)
{
    if (ShakeFloor) {
        ShakeEm(Char.room, (signed char)(Char.row - (ShakeFloor >= 1 ? 1 : 0)));
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
    unsigned char room, limit;
    int index;
    struct GetOppInfoIdx *guard;

    room = 1;
    if (Level.room_count >= 1) {
        do {
            index = 0;
            do {
                guard = GetOppInfoIdx(room, index++);
                if (guard)
                    guard->active = 0;
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

extern char ReadCell(char room, char column, char row);              /* far:02cc:00f0 */

void CheckAlert(void)
{
    signed char swap, index, left, right, kind, second, first, count;
    unsigned int sequence;

    if (Kid.room) {
        sequence = Kid.frame;
        first = GetOppIdxClosestToKid(0);
        second = GetOppIdxClosestToKid(-1);
        count = Level.room_objects[(unsigned char)Kid.room - 1].opponent_count;
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
                            !(Char.character_kind == 11 && kind == 7 && ((*(unsigned char *)&Cell.modifier) & 0x80))) {
                            Char.alert_delay = 0;
                        } else if ((kind == 4 && (*(unsigned char *)&Cell.modifier) < 0x70) ||
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

signed char ResetLevel(void)
{
    RestartLevel = 0;
    if (LevelIdx == NextLevel) {
        SaveGamePlayPalette();
        EraseRect((char *)&GamePlayRect);
        if (StartLevel)
            return LevelIdx;
        return 0;
    }
    return NextLevel;
}

extern int PlayNIS(int);                   /* far:0284:0226 */

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
struct char_bounds MsgRect = { 193, 98, 202, 235 };
int NeedShadow = 0;
signed char LevelIdx = 0;
int unused_flag = 1;                                /* never referenced */
