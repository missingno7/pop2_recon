/* LOAD.C, resident resource loading and overlay selection. */
typedef unsigned char Byte;
typedef signed char SByte;
typedef unsigned Handle;
typedef struct {
    int top, left, bottom, right;
} Rect;
typedef struct {
    Byte reserved[12];
    SByte sword_kind;
} KidRecord;

/* Resource openers replace the initial name pointers with file handles. */
int resource_files[3] = {
    (int)"PRINCE.DAT", (int)"KID.DAT", (int)"SEQUENCE.DAT"
};
int LevelRsrcFiles[7] = {
    0, (int)"DESERT.DAT", (int)"TEMPLE.DAT", (int)"CAVERNS.DAT",
    (int)"RUINS.DAT", (int)"ROOFTOPS.DAT", (int)"FINAL.DAT"
};

int BGSetMarkedPurgeable = 0;
const int Level1stOverlay[7] = { -1, -1, 78, 99, 89, -1, -1 };

static const int temple_image_first[] = { 0, 22, 29, 33, 38, 40, 42, 45, 49, 90, -1 };
static const int temple_image_last[] = { 20, 27, 30, 34, 38, 40, 42, 45, 62, 125 };
static const int cavern_image_first[] = { 0, 15, 28, 33, 40, 44, 51, 63, 73, 99, -1 };
static const int cavern_image_last[] = { 5, 23, 29, 36, 41, 49, 51, 66, 90, 125 };
static const int ruin_image_first[] = { 0, 18, 21, 29, 35, 41, 48, 50, 52, 89, -1 };
static const int ruin_image_last[] = { 16, 18, 24, 31, 37, 42, 48, 50, 79, 113 };

static const int *background_image_first[7] = {
    0, 0, temple_image_first, cavern_image_first, ruin_image_first, 0, 0
};
static const int *background_image_last[7] = {
    0, 0, temple_image_last, cavern_image_last, ruin_image_last, 0, 0
};


extern void near *BPort;
extern Rect GamePlayRect;
extern int background_intro_group[], background_intro_number[];
extern unsigned overlay_output;
extern Handle Pics[];
extern int CustomRoom;
extern Rect ScreenPortRect;
extern char background_overlay_file[];
extern int extra_kid_overlay;
typedef void (far pascal *LevelFrameProc)(void);

extern void far pascal DsrtFrameFunc(void);
extern void far pascal check_temple_event(void);
extern void far pascal CaveFrameFunc(void);
extern void far pascal RoofFrameFunc(void);
extern void far pascal FinlFrameFunc(void);

LevelFrameProc LevelFrameFunc[7] = {
    0, DsrtFrameFunc, check_temple_event, CaveFrameFunc, 0, RoofFrameFunc, FinlFrameFunc
};

SByte LastOppTypeLoaded = -1;
static SByte loaded_sword = 0;
static int opponent_resource_files[11] = {
    (int)"GUARD.DAT", (int)"FLAME.DAT", (int)"SKELETON.DAT", (int)"GUARD.DAT",
    0, (int)"HEAD.DAT", (int)"HEAD.DAT", (int)"BIRD.DAT", (int)"HEAD.DAT",
    (int)"JINNEE.DAT", 0
};

static int guard_overlay_first[] = { 31, 47, 58, 74, -1 };
static int guard_overlay_last[] = { 46, 57, 73, 85 };
static int skeleton_overlay_first[] = { 33, 49, -1 };
static int skeleton_overlay_last[] = { 48, 52 };
static int head_overlay_first[] = { 0, -1 };
static int head_overlay_last[] = { 116 };
static int bird_overlay_first[] = { 40, 56, 67, 95, -1 };
static int bird_overlay_last[] = { 55, 66, 94, 98 };

int *Opp1stRange[11] = {
    guard_overlay_first, 0, skeleton_overlay_first, 0, 0, head_overlay_first, 0,
    bird_overlay_first, 0, 0, 0
};
static int *opponent_overlay_last[11] = {
    guard_overlay_last, 0, skeleton_overlay_last, 0, 0, head_overlay_last, 0,
    bird_overlay_last, 0, 0, 0
};

static int sword_overlay_first[1] = { 47 };
static int sword_overlay_last[1] = { 57 };
int Need1stImg[8] = { 115, 127, 90, 95, 100, 79, 91, 105 };
int NeedLastImg[8] = { 126, 161, 91, 114, 103, 90, 94, 108 };
static const int kid_overlay_first[11] = { 263, 223, 241, 244, 249, 269, 222, 271, 280, 290, 300 };
static const int kid_overlay_last[11] = { 268, 240, 243, 248, 262, 270, 222, 279, 289, 298, 313 };
static const int kid_image_first[9] = { 15, 44, 99, 120, 133, 160, 219, 222, -1 };
static const int kid_image_last[8] = { 32, 79, 111, 129, 144, 215, 220, 299 };
static const char map_option[] = "GAMEPLAY\0%s%s";

typedef struct {
    signed char argument, entrance_mode, sword_kind, death_countdown;
    int field_4, shadow_timer;
} KIDINFO;
extern KIDINFO KidInfo;
extern Handle Pieces;
Handle SwordFrames;
extern Handle hOppFrames, hHeadChompFrame;
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
    int field_34;                      /* 34 */
    int previous_mode;                 /* 36 */
    unsigned char behavior;            /* 38 */
    signed char opponent_index;        /* 39 */
    unsigned char field_3a;            /* 3a */
    unsigned char unknown_3b[5];        /* 3b..3f */
} CHARREC;
#pragma pack()
extern int *LevelDrawFuncs[];
extern void far *LevelDrawGateMoveFuncs[];
extern int *DrawFunc;
extern void far *DrawGateMoveFunc;
extern SByte LevelIdx;
extern int *BlueS;
extern Byte *BlueT;
extern int *Graf;
extern int ScreenPort[];
extern int saved_background_set;
extern char resource_path_format[];

extern void near * far pascal AllocBackPort(const Rect far *);
extern void far pascal CloseRsrcFile(int);
extern void far pascal OpenRsrcVerify(int);
extern Handle far pascal LoadShapeList(int, unsigned, unsigned, int);
extern void far pascal UnloadShapeList(Handle);
extern void far pascal OverlayShapeList(Handle, unsigned, int, int, int);
extern int far pascal IsOverlayLoaded(Handle, int);
extern void far pascal UnloadOverlay(Handle, int, int);
extern void far pascal PrinceMarkRangesPurgeable(Handle, const int *, const int *);
extern void far pascal LoadSounds(int, int);
extern void far pascal UnloadSounds(int, int);
extern void far pascal change_song_range(SByte);
extern int far pascal SetPalette(unsigned, int, Byte, Byte);
extern void * far cdecl _nmalloc(unsigned);
extern void far cdecl _nfree(void *);
extern void * far cdecl memset(void *, int, unsigned);
#pragma intrinsic(memset)
extern void far pascal RemoveRebirthPt(void);
extern int far pascal IsLevelStartWithNISScreen(void);
extern void far pascal EraseRect(const Rect far *);
extern void far pascal InitDsrtGlobals(void);
extern void * far pascal find_option(const char far *);
extern void far pascal ReadRsrc(int, int, int, const char far *, long);
extern void far pascal SetRebirthPtLevelStatus(void);
extern void far pascal SaveGamePlayPalette(void);
extern void far pascal LoadDsrtCode(void);
extern void far pascal TempFrameFunc(void);
extern void far pascal LoadTempleDrawCode(void);
extern void far pascal load_cave_drawing(void);
extern void far pascal cave_overlay_entry(void);
extern void far pascal LoadRuinsDrawCode(void);
extern void far pascal LoadRuinsMoveCode(void);
extern void far pascal LoadRoofCode(void);
extern void far pascal LoadFinlCode(void);
extern void far pascal unused_head_entry(void);
extern void far pascal unused_guard_entry(void);
extern Handle far pascal nGetRsrc(unsigned long, unsigned);
extern void far pascal hFree(Handle);
extern Handle far pascal HGETRSRC(unsigned long, unsigned);
extern void far pascal RELEASE_TAGGED_RESOURCE(unsigned long, unsigned);
extern unsigned long far * far pascal h2p(Handle);
extern void far pascal select_room_arrays(Byte);
extern void far _fastcall PresetOppBanks(Byte);
extern Handle far pascal hGetSnd(unsigned);
extern void far pascal hGetState(Handle);
extern const char far * far pascal hLock(Handle);
extern char far * far cdecl _fstrcpy(char far *, const char far *);
extern void far cdecl _splitpath(char *, char *, char *, char *, char *);
extern int far cdecl sprintf(char *, const char *, ...);
extern void far pascal abort_with_message(int, const char far *);
extern void far pascal ZeroPeels(void);
extern void far pascal UnloadCustomRoomData(void);

void far pascal UnloadSet(int set);
void far pascal LoadBGOverlays(void);
void far pascal LoadSet(int set, unsigned id, unsigned limit, int count, int mode);
void far pascal UnloadLevel(int first);
void far pascal LoadLevelMap(SByte map);
void far pascal LoadKidSet(void);
void far pascal LoadKidSetOverlays(void);
void far pascal LoadOppSet(SByte);
void far pascal SetKidSwordKind(void);
void far pascal LoadSwordSet(SByte kind);
void far pascal LoadPiecesData(void);
void far pascal switch_background_overlay(Byte current, Byte previous);
void far pascal LoadCodeOverlays(Byte background_kind, Byte opponent_kind);
void far pascal SetKidPalette(Byte kind);
void far pascal SetPrinceLevelSpecPtrs(Byte kind);
void far pascal SetInitials(void);
void far pascal UnloadPiecesData(void);
Handle far pascal LoadSwordFrames(Byte kind);
int far pascal NeedSwordOverlay(SByte index);

void far pascal OpenLevelRsrcFiles(Byte current, Byte previous)
{
    if (current != previous) {
        if (previous) {
            if (previous == 6 || current == 6)
                UnloadSet(2);
            CloseRsrcFile(LevelRsrcFiles[previous]);
        }
        OpenRsrcVerify(LevelRsrcFiles[current]);
    }
}

int far pascal NeedKidSetOverlay(int index);
void far pascal UnloadOppOverlays(SByte previous, SByte current);
void far pascal LoadOppOverlays(SByte kind);
void far pascal report_resource_path(const char far *path);

void far pascal CheckBPort(void)
{
    if (BPort == 0)
        BPort = AllocBackPort(&GamePlayRect);
}

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
    int sword_overlays[3];                           /* 1858 */
    int flask_flag;                                   /* 185e */
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
extern LEVELREC Level;
extern CHARREC Char;

void far pascal CheckLoadedOppSet(SByte kind)
{
    if (kind != LastOppTypeLoaded && Level.preferred_opponent != 5 &&
        Level.preferred_opponent != 6 && Char.character_kind != 11 &&
        Char.character_kind != 0 && Char.character_kind != 6)
        LoadOppSet(kind);
}
void far pascal LoadBGSet(int palette)
{
    unsigned limit = 0x3ff0;

    if (Level.environment_kind == 5)
        limit = 0x7ff0;
    else if (Level.environment_kind == 6)
        limit = 0xfff0;

    LoadSet(4, 3500, limit, 1, 0);
    if (palette)
        SetPalette(3500, 64, 160, 0);
    LoadBGOverlays();
    BGSetMarkedPurgeable = 0;
    if (background_image_first[Level.environment_kind])
        PrinceMarkRangesPurgeable(Pics[4], background_image_first[Level.environment_kind],
                             background_image_last[Level.environment_kind]);
}

void far pascal LoadBGOverlays(void)
{
    int i;

    for (i = 0; i < 8; ++i) {
        if (Level.background_overlays[i]) {
            if (Need1stImg[i] != -1)
                OverlayShapeList(Pics[4], 3700, Need1stImg[i] - 1,
                                   NeedLastImg[i] - 1, 1);
            if (background_intro_group[i] != -1)
                LoadSounds(background_intro_group[i], background_intro_number[i]);
        }
    }
}

void far pascal LoadFlaskSet(void)
{
    if (Level.flask_flag) {
        Byte *entries = _nmalloc(35);
        int i;

        memset(entries, 2, 35);
        for (i = 2; i <= 21; ++i)
            entries[i] = 1;
        LoadSet(1, 3000, 0x4000, (int)entries, 0);
        _nfree(entries);
        if (Level.environment_kind != 6)
            SetPalette(3000, 224, 16, 0);
    } else {
        UnloadSet(1);
    }
}

extern int NextLevel, Resume;
void far pascal LoadLevel(SByte number)
{
    int starts_with_nis;
    Byte previous;

    UnloadLevel(3);
    previous = Level.environment_kind;
    if (number != -2 && number != -1) {
        if (LevelIdx != number) {
            if (Resume == 0)
                RemoveRebirthPt();
            LevelIdx = number;
        }
    }
    NextLevel = LevelIdx;
    CheckBPort();
    LoadLevelMap(LevelIdx);
    starts_with_nis = IsLevelStartWithNISScreen();
    if (!starts_with_nis)
        EraseRect(&ScreenPortRect);
    LoadFlaskSet();
    OpenLevelRsrcFiles(Level.environment_kind, previous);
    LoadBGSet(!starts_with_nis);
    LoadKidSet();
    LoadKidSetOverlays();
    LoadOppSet(Level.preferred_opponent);
    SetKidSwordKind();
    LoadSwordSet(KidInfo.sword_kind);
    LoadSounds(0, 26);
    LoadPiecesData();
    if (previous != Level.environment_kind) {
        switch_background_overlay(Level.environment_kind, previous);
        SetKidPalette(Level.environment_kind);
    }
    LoadCodeOverlays(Level.environment_kind, Level.preferred_opponent);
    if (number != -2 && number != -1 && Level.environment_kind == 1)
        InitDsrtGlobals();
    SetPrinceLevelSpecPtrs(Level.environment_kind);
}

void far pascal LoadLevelMap(SByte map)
{
    int id = map + 1999;
    if (find_option(map_option))
        id += 20;
    ReadRsrc(0, 0, id, (const char far *)&Level, 12025L);
    SetInitials();
    SetRebirthPtLevelStatus();
}

void far pascal LoadLevelMinimum(SByte number)
{
    LoadLevelMap(number);
    SetKidSwordKind();
    LoadSwordSet(KidInfo.sword_kind);
    SaveGamePlayPalette();
    Graf = ScreenPort;
    EraseRect(&ScreenPortRect);
}

extern int background_scene_group[], background_scene_number[];

void far pascal switch_background_overlay(Byte current, Byte previous)
{
    if (previous != current && background_scene_group[previous] != -1)
        UnloadSounds(background_scene_group[previous], background_scene_number[previous]);
    if (background_scene_group[Level.environment_kind] != -1)
        LoadSounds(background_scene_group[Level.environment_kind], background_scene_number[Level.environment_kind]);
}

void far pascal LoadCodeOverlays(Byte background_kind, Byte opponent_kind)
{
    switch (background_kind) {
    case 1:
        LoadDsrtCode();
        break;
    case 2:
        TempFrameFunc();
        LoadTempleDrawCode();
        break;
    case 3:
        load_cave_drawing();
        cave_overlay_entry();
        break;
    case 4:
        LoadRuinsDrawCode();
        LoadRuinsMoveCode();
        break;
    case 5:
        LoadRoofCode();
        break;
    case 6:
        LoadFinlCode();
        break;
    }
    if (opponent_kind == 5 || opponent_kind == 6 || opponent_kind == 8)
        unused_head_entry();
    else
        unused_guard_entry();
}

void far pascal LoadPiecesData(void)
{
    UnloadPiecesData();
    Pieces = nGetRsrc(0x50494543UL, 3500);
}

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

void far pascal LoadSwordSet(SByte kind)
{
    unsigned id;
    int mode;
    SByte i;

    if (loaded_sword != kind) {
        if (SwordFrames) {
            hFree(SwordFrames);
            SwordFrames = 0;
        }
        UnloadSet(0);
    }
    if (SwordFrames == 0)
        SwordFrames = LoadSwordFrames(kind);

    id = 1000;
    if (kind == 2)
        id = 1200;
    mode = Room.number == 1 && Level.level_number == 14 ? 0 : 1;
    LoadSet(0, id, 0x8000, 2, mode);
    if (kind == 1) {
        for (i = 0; i < 1; ++i) {
            if (NeedSwordOverlay(i)) {
                if (!IsOverlayLoaded(Pics[0], sword_overlay_first[i]))
                    OverlayShapeList(Pics[0], 1100, sword_overlay_first[i],
                                       sword_overlay_last[i], 2);
            } else {
                UnloadOverlay(Pics[0], sword_overlay_first[i], sword_overlay_last[i]);
            }
        }
    }
    loaded_sword = kind;
}

Handle far pascal LoadSwordFrames(Byte kind)
{
    unsigned id = 1000;
    if (kind == 2)
        id = 1200;
    return HGETRSRC(0x4652414dUL, id);
}

int far _fastcall GetSwordFrameImg(unsigned index)
{
    int image;

    if (index)
        image = (int)h2p(SwordFrames)[index];
    else
        image = -1;
    return image;
}

void far pascal SetPrinceLevelSpecPtrs(Byte kind)
{
    DrawFunc = LevelDrawFuncs[kind];
    DrawGateMoveFunc = LevelDrawGateMoveFuncs[kind];
}

void far _fastcall clear_special_sequence_flag(int i)
{
    int *flags = &BlueS[2 * i];
    switch ((unsigned)BlueT[i]) {
    case 7:
        *flags &= ~0x80;
        break;
    }
}

void far pascal SetInitials(void)
{
    Byte room;
    int index;
    for (room = 1; room <= Level.room_count; ++room) {
        select_room_arrays(room);
        Room.left = Level.room_links[room - 1].left;
        Room.right = Level.room_links[room - 1].right;
        for (index = 0; index < 30; ++index)
            clear_special_sequence_flag(index);
    }
}

void far pascal LoadSet(int set, unsigned id, unsigned limit, int count, int mode)
{
    if (Pics[set] == 0)
        Pics[set] = LoadShapeList(id, limit, count, mode);
}

void far pascal LoadKidSet(void)
{
    Byte *entries;
    int i, j;

    if (Level.environment_kind == 6)
        OpenRsrcVerify(LevelRsrcFiles[6]);
    entries = _nmalloc(314);
    memset(entries, 0, 314);
    for (i = 131; i <= 132; ++i)
        entries[i] = 1;
    for (j = 216; j <= 218; ++j)
        entries[j] = 1;
    LoadSet(2, 25001U, 2, (int)entries, 0);
    _nfree(entries);
    PrinceMarkRangesPurgeable(Pics[2], kid_image_first, kid_image_last);
}

void far pascal LoadKidSetOverlays(void)
{
    int i;
    for (i = 0; i < 11; ++i) {
        if (NeedKidSetOverlay(i)) {
            if (!IsOverlayLoaded(Pics[2], kid_overlay_first[i]))
                OverlayShapeList(Pics[2], 24601, kid_overlay_first[i],
                                   kid_overlay_last[i], 0);
        } else {
            UnloadOverlay(Pics[2], kid_overlay_first[i], kid_overlay_last[i]);
        }
    }
}

int far pascal NeedKidSetOverlay(int index)
{
    int needed;
    switch (index) {
    default:
        needed = 0;
        break;
    case 0:
        needed = Level.background_overlays[0];
        break;
    case 1:
        needed = Level.environment_kind == 4 || Level.environment_kind == 2;
        break;
    case 2:
        needed = Level.environment_kind == 3 || Level.preferred_opponent == 5 || Level.preferred_opponent == 6;
        break;
    case 3:
        needed = Level.background_overlays[3];
        break;
    case 5:
        needed = Level.preferred_opponent == 5 || Level.preferred_opponent == 6;
        break;
    case 6:
        needed = Level.level_number == 5;
        break;
    case 8:
        needed = Level.level_number == 13;
        break;
    case 9:
        needed = Level.level_number == 13 || Level.environment_kind == 6;
        break;
    case 4:
    case 7:
    case 10:
        needed = Level.environment_kind == 6;
        break;
    }
    return needed;
}

void far pascal SetKidPalette(Byte kind)
{
    SetPalette(25001, 16, 16, (Byte)(kind - 1));
}

int far pascal NeedOppOverlay(SByte kind, int index)
{
    int needed = 0;
    if (kind != -1 && Opp1stRange[kind]) {
        int count;
        for (count = -1; Opp1stRange[kind][count] != -1; ++count)
            ;
        if (count >= index) {
            if (index < 3)
                needed = Level.sword_overlays[index];
            else if (kind == 7 || Level.environment_kind == 6)
                needed = 1;
        }
    }
    return needed;
}

int far pascal NeedSwordOverlay(SByte index)
{
    int needed; /* The original sets this local only on the success path. */
    switch ((int)index) {
    case 0:
        if (Level.sword_overlays[0])
            needed = 1;
        break;
    }
    return needed;
}

void far pascal LoadOppSet(SByte kind)
{
    if (kind == -1 || kind != LastOppTypeLoaded || Pics[3] == 0) {
        UnloadSet(3);
        if (hOppFrames) {
            hFree(hOppFrames);
            hOppFrames = 0;
        }
        if (hHeadChompFrame) {
            hFree(hHeadChompFrame);
            hHeadChompFrame = 0;
        }
        if (LastOppTypeLoaded != -1 && kind != LastOppTypeLoaded)
            CloseRsrcFile(opponent_resource_files[LastOppTypeLoaded]);
    }
    if (LastOppTypeLoaded != -1 && Opp1stRange[LastOppTypeLoaded])
        UnloadOppOverlays(LastOppTypeLoaded, kind);

    if (kind != -1) {
        if (kind == LastOppTypeLoaded && Pics[3]) {
            if (kind == LastOppTypeLoaded)
                LoadOppOverlays(kind);
        } else {
            OpenRsrcVerify(opponent_resource_files[kind]);
            LoadSet(3, 750, 4, 2, 0);
            SetPalette(750, 32, 16, 0);
            if (Opp1stRange[kind]) {
                LoadOppOverlays(kind);
                PrinceMarkRangesPurgeable(Pics[3], Opp1stRange[kind],
                                     opponent_overlay_last[kind]);
            }
            change_song_range(kind);
            hOppFrames = HGETRSRC(0x4652414dUL, 750);
            if ((kind == 5 || kind == 6) && hHeadChompFrame == 0)
                hHeadChompFrame = HGETRSRC(0L, 755);
        }
    }
    LastOppTypeLoaded = kind;
}

signed char OppBank[2];
void far pascal LoadOppOverlays(SByte kind)
{
    int *last_base = opponent_overlay_last[kind];
    int *first_base = Opp1stRange[kind];
    if (first_base && kind != 5 && kind != 6) {
        int i = 0;
        if (*first_base == -1)
            return;
        do {
            if (NeedOppOverlay(kind, i)) {
                if (!IsOverlayLoaded(Pics[3], first_base[i]))
                    OverlayShapeList(Pics[3], 850, first_base[i], last_base[i], 2);
                if (i == 0 && (kind == 0 || kind == 5 || kind == 6)) {
                    if (!(Byte)OppBank[0])
                        PresetOppBanks(1);
                    if (!(Byte)OppBank[1])
                        PresetOppBanks(2);
                }
            } else {
                UnloadOverlay(Pics[3], first_base[i], last_base[i]);
            }
            ++i;
        } while (first_base[i] != -1);
    }
}


extern CHARREC Kid;

void far pascal PreloadRoomData(Byte room)
{
    int visited[64];
    SByte tile;

    memset(visited, 0, sizeof(visited));
    tile = 0;
    do {
        const Byte *object_code = &Level.cell_kinds[room - 1][tile];
        const LEVELCELL *cell = &Level.cell_info[room - 1][tile];
        int object = *object_code;
        unsigned info = cell->modifier;
        if (visited[*object_code] == 0) {
            visited[object] = 1;
            switch (object) {
            case 2:
                LoadSounds(background_intro_group[0], background_intro_number[0]);
                break;
            case 5:
            case 6:
            case 34:
                LoadSounds(3, 3);
                break;
            case 10:
                if (((Byte)info & 0xe0) == 0xc0) {
                    if (LastOppTypeLoaded != 9)
                        LoadOppSet(9);
                } else {
                    visited[object] = 0;
                }
                /* fall through */
            case 4:
                if (Level.environment_kind == 3)
                    LoadSounds(7, 9);
                else
                    LoadSounds(4, 6);
                break;
            case 11:
                LoadSounds(20, 20);
                LoadSounds(2, 2);
                break;
            case 13:
                LoadSounds(background_intro_group[3], background_intro_number[3]);
                break;
            case 23:
                LoadSounds(background_intro_group[1], background_intro_number[1]);
                break;
            case 25:
                LoadSounds(background_intro_group[5], background_intro_number[5]);
                break;
            case 26:
                LoadSounds(background_intro_group[6], background_intro_number[6]);
                break;
            case 36:
                LoadSounds(background_intro_group[4], background_intro_number[4]);
                break;
            }
        }
    } while (++tile < 30);
    if (Level.room_objects[room - 1].opponent_count) {
        change_song_range(Level.preferred_opponent);
        LoadSounds(12, 14);
    }
    if (Kid.vertical_step > 0) {
        if (Kid.vertical_step < 33)
            hGetSnd(10016);
        else
            hGetSnd(10000);
        if (Kid.vertical_step < 31)
            hGetSnd(10001);
    }
}

int far pascal load_resource_error(Handle resource)
{
    char path[120];
    char drive[4], directory[256], basename[256], extension[256];

    hGetState(resource);
    _fstrcpy(path, hLock(resource) + 16);
    _splitpath(path, drive, directory, basename, extension);
    sprintf(path, map_option + sizeof("GAMEPLAY"), basename, extension);
    report_resource_path(path);
    return 1;
}

void far pascal report_resource_path(const char far *path)
{
    char message[100];

    sprintf(message, resource_path_format, path);
    abort_with_message(1, message);
}

extern KidRecord far *hRebirthSaved;

void far pascal SetKidSwordKind(void)
{
    if (hRebirthSaved)
        KidInfo.sword_kind = hRebirthSaved->sword_kind;
    else if (Level.level_number == 6)
        KidInfo.sword_kind = -1;
    else if (Level.level_number == 7 || Level.level_number == 8)
        KidInfo.sword_kind = 2;
    else
        KidInfo.sword_kind = 1;
}

void far pascal UnloadFrom(int first)
{
    int i;
    ZeroPeels();
    for (i = first; i < 5; ++i)
        UnloadSet(i);
    if (first <= 4 && CustomRoom)
        UnloadCustomRoomData();
}

void far pascal UnloadLevel(int first)
{
    int i;
    UnloadFrom(first);
    for (i = 0; i < 8; ++i) {
        if (Level.background_overlays[i])
            UnloadSounds(background_intro_group[i], background_intro_number[i]);
    }
    if (first <= 3 && hOppFrames) {
        if (hOppFrames) {
            hFree(hOppFrames);
            hOppFrames = 0;
        }
        if (hHeadChompFrame) {
            RELEASE_TAGGED_RESOURCE(0L, 755);
            hHeadChompFrame = 0;
        }
        if (LastOppTypeLoaded != -1) {
            CloseRsrcFile(opponent_resource_files[LastOppTypeLoaded]);
            LastOppTypeLoaded = -1;
        }
    }
    if (first <= 0 && SwordFrames) {
        hFree(SwordFrames);
        SwordFrames = 0;
        loaded_sword = 0;
    }
    UnloadPiecesData();
    UnloadCustomRoomData();
}

void far pascal UnloadOppOverlays(SByte previous, SByte current)
{
    if (Pics[3] && Level.preferred_opponent != 5 && Level.preferred_opponent != 6) {
        int *last_base = opponent_overlay_last[previous];
        int *first_base = Opp1stRange[previous];
        if (first_base) {
            int i = 0;
            if (*first_base == -1)
                return;
            do {
                if (current != previous || !NeedOppOverlay(current, i))
                    UnloadOverlay(Pics[3], first_base[i], last_base[i]);
                ++i;
            } while (first_base[i] != -1);
        }
    }
}

void far pascal UnloadPiecesData(void)
{
    if (Pieces) {
        _nfree((void *)Pieces);
        Pieces = 0;
    }
}

void far pascal UnloadSet(int set)
{
    if (Pics[set]) {
        UnloadShapeList(Pics[set]);
        Pics[set] = 0;
    }
}
