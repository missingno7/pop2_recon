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
int level_resource_files[7] = {
    0, (int)"DESERT.DAT", (int)"TEMPLE.DAT", (int)"CAVERNS.DAT",
    (int)"RUINS.DAT", (int)"ROOFTOPS.DAT", (int)"FINAL.DAT"
};

int background_image_count = 0;
const int background_image_limits[7] = { -1, -1, 78, 99, 89, -1, -1 };

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
extern int custom_room;
extern Rect ScreenPortRect;
extern char background_overlay_file[];
extern int extra_kid_overlay;
typedef void (far pascal *LevelFrameProc)(void);

extern void far pascal desert_frame(void);
extern void far pascal check_temple_event(void);
extern void far pascal cave_frame(void);
extern void far pascal roof_frame(void);
extern void far pascal run_final_frame(void);

LevelFrameProc level_frame_procs[7] = {
    0, desert_frame, check_temple_event, cave_frame, 0, roof_frame, run_final_frame
};

SByte loaded_opponent = -1;
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

int *opponent_overlay_first[11] = {
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
extern Handle opponent_frames, opponent_effect_frames;
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
extern int *level_start_table[];
extern void far *level_sequence_table[];
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
extern void far pascal close_resource_file(int);
extern void far pascal open_verified_resource(int);
extern Handle far pascal load_shape_list(int, unsigned, unsigned, int);
extern void far pascal UnloadShapeList(Handle);
extern void far pascal overlay_shape_list(Handle, unsigned, int, int, int);
extern int far pascal IsOverlayLoaded(Handle, int);
extern void far pascal unload_overlay(Handle, int, int);
extern void far pascal PrinceMarkRangesPurgeable(Handle, const int *, const int *);
extern void far pascal load_song_range(int, int);
extern void far pascal release_song_range(int, int);
extern void far pascal change_song_range(SByte);
extern int far pascal set_palette(unsigned, int, Byte, Byte);
extern void * far cdecl _nmalloc(unsigned);
extern void far cdecl _nfree(void *);
extern void * far cdecl memset(void *, int, unsigned);
#pragma intrinsic(memset)
extern void far pascal RemoveRebirthPt(void);
extern int far pascal IsLevelStartWithNISScreen(void);
extern void far pascal STROKE_PORT_RECT(const Rect far *);
extern void far pascal reset_desert_variant(void);
extern void * far pascal find_option(const char far *);
extern void far pascal LOAD_RESOURCE_INTO(int, int, int, const char far *, long);
extern void far pascal SetRebirthPtLevelStatus(void);
extern void far pascal save_game_play_palette(void);
extern void far pascal LoadDsrtCode(void);
extern void far pascal temple_frame(void);
extern void far pascal LoadTempleDrawCode(void);
extern void far pascal load_cave_drawing(void);
extern void far pascal cave_overlay_entry(void);
extern void far pascal LoadRuinsDrawCode(void);
extern void far pascal LoadRuinsMoveCode(void);
extern void far pascal LoadRoofCode(void);
extern void far pascal load_final_code(void);
extern void far pascal unused_head_entry(void);
extern void far pascal unused_guard_entry(void);
extern Handle far pascal LOAD_RESOURCE_COPY(unsigned long, unsigned);
extern void far pascal DISPOSE_HANDLE(Handle);
extern Handle far pascal HGETRSRC(unsigned long, unsigned);
extern void far pascal RELEASE_TAGGED_RESOURCE(unsigned long, unsigned);
extern unsigned long far * far pascal resolve_handle_to_far_payload(Handle);
extern void far pascal select_room_arrays(Byte);
extern void far _fastcall reserve_opponent_bank(Byte);
extern Handle far pascal hGetSnd(unsigned);
extern void far pascal SAVE_HANDLE_FLAGS(Handle);
extern const char far * far pascal FETCH_LOCKED_SHAPE_LIST(Handle);
extern char far * far cdecl _fstrcpy(char far *, const char far *);
extern void far cdecl _splitpath(char *, char *, char *, char *, char *);
extern int far cdecl sprintf(char *, const char *, ...);
extern void far pascal abort_with_message(int, const char far *);
extern void far pascal clear_peels(void);
extern void far pascal UnloadCustomRoomData(void);

void far pascal unload_set(int set);
void far pascal check_background_overlays(void);
void far pascal load_set(int set, unsigned id, unsigned limit, int count, int mode);
void far pascal unload_level(int first);
void far pascal load_level_map(SByte map);
void far pascal load_kid_set(void);
void far pascal load_kid_set_overlays(void);
void far pascal load_opponent_set(SByte);
void far pascal set_kid_sword_kind(void);
void far pascal load_sword_set(SByte kind);
void far pascal load_pieces_data(void);
void far pascal switch_background_overlay(Byte current, Byte previous);
void far pascal load_code_overlays(Byte background_kind, Byte opponent_kind);
void far pascal set_kid_palette(Byte kind);
void far pascal set_level_spec_ptrs(Byte kind);
void far pascal set_initial_room_state(void);
void far pascal unload_pieces_data(void);
Handle far pascal load_sword_frames(Byte kind);
int far pascal need_sword_overlay(SByte index);

void far pascal open_level_resource_files(Byte current, Byte previous)
{
    if (current != previous) {
        if (previous) {
            if (previous == 6 || current == 6)
                unload_set(2);
            close_resource_file(level_resource_files[previous]);
        }
        open_verified_resource(level_resource_files[current]);
    }
}

int far pascal need_kid_overlay(int index);
void far pascal unload_opponent_overlays(SByte previous, SByte current);
void far pascal load_opponent_overlays(SByte kind);
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

void far pascal load_changed_opponent_set(SByte kind)
{
    if (kind != loaded_opponent && Level.preferred_opponent != 5 &&
        Level.preferred_opponent != 6 && Char.character_kind != 11 &&
        Char.character_kind != 0 && Char.character_kind != 6)
        load_opponent_set(kind);
}
void far pascal load_background_set(int palette)
{
    unsigned limit = 0x3ff0;

    if (Level.environment_kind == 5)
        limit = 0x7ff0;
    else if (Level.environment_kind == 6)
        limit = 0xfff0;

    load_set(4, 3500, limit, 1, 0);
    if (palette)
        set_palette(3500, 64, 160, 0);
    check_background_overlays();
    background_image_count = 0;
    if (background_image_first[Level.environment_kind])
        PrinceMarkRangesPurgeable(Pics[4], background_image_first[Level.environment_kind],
                             background_image_last[Level.environment_kind]);
}

void far pascal check_background_overlays(void)
{
    int i;

    for (i = 0; i < 8; ++i) {
        if (Level.background_overlays[i]) {
            if (Need1stImg[i] != -1)
                overlay_shape_list(Pics[4], 3700, Need1stImg[i] - 1,
                                   NeedLastImg[i] - 1, 1);
            if (background_intro_group[i] != -1)
                load_song_range(background_intro_group[i], background_intro_number[i]);
        }
    }
}

void far pascal load_flask_set(void)
{
    if (Level.flask_flag) {
        Byte *entries = _nmalloc(35);
        int i;

        memset(entries, 2, 35);
        for (i = 2; i <= 21; ++i)
            entries[i] = 1;
        load_set(1, 3000, 0x4000, (int)entries, 0);
        _nfree(entries);
        if (Level.environment_kind != 6)
            set_palette(3000, 224, 16, 0);
    } else {
        unload_set(1);
    }
}

extern int NextLevel, Resume;
void far pascal load_level(SByte number)
{
    int starts_with_nis;
    Byte previous;

    unload_level(3);
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
    load_level_map(LevelIdx);
    starts_with_nis = IsLevelStartWithNISScreen();
    if (!starts_with_nis)
        STROKE_PORT_RECT(&ScreenPortRect);
    load_flask_set();
    open_level_resource_files(Level.environment_kind, previous);
    load_background_set(!starts_with_nis);
    load_kid_set();
    load_kid_set_overlays();
    load_opponent_set(Level.preferred_opponent);
    set_kid_sword_kind();
    load_sword_set(KidInfo.sword_kind);
    load_song_range(0, 26);
    load_pieces_data();
    if (previous != Level.environment_kind) {
        switch_background_overlay(Level.environment_kind, previous);
        set_kid_palette(Level.environment_kind);
    }
    load_code_overlays(Level.environment_kind, Level.preferred_opponent);
    if (number != -2 && number != -1 && Level.environment_kind == 1)
        reset_desert_variant();
    set_level_spec_ptrs(Level.environment_kind);
}

void far pascal load_level_map(SByte map)
{
    int id = map + 1999;
    if (find_option(map_option))
        id += 20;
    LOAD_RESOURCE_INTO(0, 0, id, (const char far *)&Level, 12025L);
    set_initial_room_state();
    SetRebirthPtLevelStatus();
}

void far pascal load_minimum_level(SByte number)
{
    load_level_map(number);
    set_kid_sword_kind();
    load_sword_set(KidInfo.sword_kind);
    save_game_play_palette();
    Graf = ScreenPort;
    STROKE_PORT_RECT(&ScreenPortRect);
}

extern int background_scene_group[], background_scene_number[];

void far pascal switch_background_overlay(Byte current, Byte previous)
{
    if (previous != current && background_scene_group[previous] != -1)
        release_song_range(background_scene_group[previous], background_scene_number[previous]);
    if (background_scene_group[Level.environment_kind] != -1)
        load_song_range(background_scene_group[Level.environment_kind], background_scene_number[Level.environment_kind]);
}

void far pascal load_code_overlays(Byte background_kind, Byte opponent_kind)
{
    switch (background_kind) {
    case 1:
        LoadDsrtCode();
        break;
    case 2:
        temple_frame();
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
        load_final_code();
        break;
    }
    if (opponent_kind == 5 || opponent_kind == 6 || opponent_kind == 8)
        unused_head_entry();
    else
        unused_guard_entry();
}

void far pascal load_pieces_data(void)
{
    unload_pieces_data();
    Pieces = LOAD_RESOURCE_COPY(0x50494543UL, 3500);
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

void far pascal load_sword_set(SByte kind)
{
    unsigned id;
    int mode;
    SByte i;

    if (loaded_sword != kind) {
        if (SwordFrames) {
            DISPOSE_HANDLE(SwordFrames);
            SwordFrames = 0;
        }
        unload_set(0);
    }
    if (SwordFrames == 0)
        SwordFrames = load_sword_frames(kind);

    id = 1000;
    if (kind == 2)
        id = 1200;
    mode = Room.number == 1 && Level.level_number == 14 ? 0 : 1;
    load_set(0, id, 0x8000, 2, mode);
    if (kind == 1) {
        for (i = 0; i < 1; ++i) {
            if (need_sword_overlay(i)) {
                if (!IsOverlayLoaded(Pics[0], sword_overlay_first[i]))
                    overlay_shape_list(Pics[0], 1100, sword_overlay_first[i],
                                       sword_overlay_last[i], 2);
            } else {
                unload_overlay(Pics[0], sword_overlay_first[i], sword_overlay_last[i]);
            }
        }
    }
    loaded_sword = kind;
}

Handle far pascal load_sword_frames(Byte kind)
{
    unsigned id = 1000;
    if (kind == 2)
        id = 1200;
    return HGETRSRC(0x4652414dUL, id);
}

int far _fastcall get_sword_frame_image(unsigned index)
{
    int image;

    if (index)
        image = (int)resolve_handle_to_far_payload(SwordFrames)[index];
    else
        image = -1;
    return image;
}

void far pascal set_level_spec_ptrs(Byte kind)
{
    DrawFunc = level_start_table[kind];
    DrawGateMoveFunc = level_sequence_table[kind];
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

void far pascal set_initial_room_state(void)
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

void far pascal load_set(int set, unsigned id, unsigned limit, int count, int mode)
{
    if (Pics[set] == 0)
        Pics[set] = load_shape_list(id, limit, count, mode);
}

void far pascal load_kid_set(void)
{
    Byte *entries;
    int i, j;

    if (Level.environment_kind == 6)
        open_verified_resource(level_resource_files[6]);
    entries = _nmalloc(314);
    memset(entries, 0, 314);
    for (i = 131; i <= 132; ++i)
        entries[i] = 1;
    for (j = 216; j <= 218; ++j)
        entries[j] = 1;
    load_set(2, 25001U, 2, (int)entries, 0);
    _nfree(entries);
    PrinceMarkRangesPurgeable(Pics[2], kid_image_first, kid_image_last);
}

void far pascal load_kid_set_overlays(void)
{
    int i;
    for (i = 0; i < 11; ++i) {
        if (need_kid_overlay(i)) {
            if (!IsOverlayLoaded(Pics[2], kid_overlay_first[i]))
                overlay_shape_list(Pics[2], 24601, kid_overlay_first[i],
                                   kid_overlay_last[i], 0);
        } else {
            unload_overlay(Pics[2], kid_overlay_first[i], kid_overlay_last[i]);
        }
    }
}

int far pascal need_kid_overlay(int index)
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

void far pascal set_kid_palette(Byte kind)
{
    set_palette(25001, 16, 16, (Byte)(kind - 1));
}

int far pascal need_opponent_overlay(SByte kind, int index)
{
    int needed = 0;
    if (kind != -1 && opponent_overlay_first[kind]) {
        int count;
        for (count = -1; opponent_overlay_first[kind][count] != -1; ++count)
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

int far pascal need_sword_overlay(SByte index)
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

void far pascal load_opponent_set(SByte kind)
{
    if (kind == -1 || kind != loaded_opponent || Pics[3] == 0) {
        unload_set(3);
        if (opponent_frames) {
            DISPOSE_HANDLE(opponent_frames);
            opponent_frames = 0;
        }
        if (opponent_effect_frames) {
            DISPOSE_HANDLE(opponent_effect_frames);
            opponent_effect_frames = 0;
        }
        if (loaded_opponent != -1 && kind != loaded_opponent)
            close_resource_file(opponent_resource_files[loaded_opponent]);
    }
    if (loaded_opponent != -1 && opponent_overlay_first[loaded_opponent])
        unload_opponent_overlays(loaded_opponent, kind);

    if (kind != -1) {
        if (kind == loaded_opponent && Pics[3]) {
            if (kind == loaded_opponent)
                load_opponent_overlays(kind);
        } else {
            open_verified_resource(opponent_resource_files[kind]);
            load_set(3, 750, 4, 2, 0);
            set_palette(750, 32, 16, 0);
            if (opponent_overlay_first[kind]) {
                load_opponent_overlays(kind);
                PrinceMarkRangesPurgeable(Pics[3], opponent_overlay_first[kind],
                                     opponent_overlay_last[kind]);
            }
            change_song_range(kind);
            opponent_frames = HGETRSRC(0x4652414dUL, 750);
            if ((kind == 5 || kind == 6) && opponent_effect_frames == 0)
                opponent_effect_frames = HGETRSRC(0L, 755);
        }
    }
    loaded_opponent = kind;
}

signed char OppBank[2];
void far pascal load_opponent_overlays(SByte kind)
{
    int *last_base = opponent_overlay_last[kind];
    int *first_base = opponent_overlay_first[kind];
    if (first_base && kind != 5 && kind != 6) {
        int i = 0;
        if (*first_base == -1)
            return;
        do {
            if (need_opponent_overlay(kind, i)) {
                if (!IsOverlayLoaded(Pics[3], first_base[i]))
                    overlay_shape_list(Pics[3], 850, first_base[i], last_base[i], 2);
                if (i == 0 && (kind == 0 || kind == 5 || kind == 6)) {
                    if (!(Byte)OppBank[0])
                        reserve_opponent_bank(1);
                    if (!(Byte)OppBank[1])
                        reserve_opponent_bank(2);
                }
            } else {
                unload_overlay(Pics[3], first_base[i], last_base[i]);
            }
            ++i;
        } while (first_base[i] != -1);
    }
}


extern CHARREC Kid;

void far pascal preload_room_data(Byte room)
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
                load_song_range(background_intro_group[0], background_intro_number[0]);
                break;
            case 5:
            case 6:
            case 34:
                load_song_range(3, 3);
                break;
            case 10:
                if (((Byte)info & 0xe0) == 0xc0) {
                    if (loaded_opponent != 9)
                        load_opponent_set(9);
                } else {
                    visited[object] = 0;
                }
                /* fall through */
            case 4:
                if (Level.environment_kind == 3)
                    load_song_range(7, 9);
                else
                    load_song_range(4, 6);
                break;
            case 11:
                load_song_range(20, 20);
                load_song_range(2, 2);
                break;
            case 13:
                load_song_range(background_intro_group[3], background_intro_number[3]);
                break;
            case 23:
                load_song_range(background_intro_group[1], background_intro_number[1]);
                break;
            case 25:
                load_song_range(background_intro_group[5], background_intro_number[5]);
                break;
            case 26:
                load_song_range(background_intro_group[6], background_intro_number[6]);
                break;
            case 36:
                load_song_range(background_intro_group[4], background_intro_number[4]);
                break;
            }
        }
    } while (++tile < 30);
    if (Level.room_objects[room - 1].opponent_count) {
        change_song_range(Level.preferred_opponent);
        load_song_range(12, 14);
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

    SAVE_HANDLE_FLAGS(resource);
    _fstrcpy(path, FETCH_LOCKED_SHAPE_LIST(resource) + 16);
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

void far pascal set_kid_sword_kind(void)
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

void far pascal unload_from(int first)
{
    int i;
    clear_peels();
    for (i = first; i < 5; ++i)
        unload_set(i);
    if (first <= 4 && custom_room)
        UnloadCustomRoomData();
}

void far pascal unload_level(int first)
{
    int i;
    unload_from(first);
    for (i = 0; i < 8; ++i) {
        if (Level.background_overlays[i])
            release_song_range(background_intro_group[i], background_intro_number[i]);
    }
    if (first <= 3 && opponent_frames) {
        if (opponent_frames) {
            DISPOSE_HANDLE(opponent_frames);
            opponent_frames = 0;
        }
        if (opponent_effect_frames) {
            RELEASE_TAGGED_RESOURCE(0L, 755);
            opponent_effect_frames = 0;
        }
        if (loaded_opponent != -1) {
            close_resource_file(opponent_resource_files[loaded_opponent]);
            loaded_opponent = -1;
        }
    }
    if (first <= 0 && SwordFrames) {
        DISPOSE_HANDLE(SwordFrames);
        SwordFrames = 0;
        loaded_sword = 0;
    }
    unload_pieces_data();
    UnloadCustomRoomData();
}

void far pascal unload_opponent_overlays(SByte previous, SByte current)
{
    if (Pics[3] && Level.preferred_opponent != 5 && Level.preferred_opponent != 6) {
        int *last_base = opponent_overlay_last[previous];
        int *first_base = opponent_overlay_first[previous];
        if (first_base) {
            int i = 0;
            if (*first_base == -1)
                return;
            do {
                if (current != previous || !need_opponent_overlay(current, i))
                    unload_overlay(Pics[3], first_base[i], last_base[i]);
                ++i;
            } while (first_base[i] != -1);
        }
    }
}

void far pascal unload_pieces_data(void)
{
    if (Pieces) {
        _nfree((void *)Pieces);
        Pieces = 0;
    }
}

void far pascal unload_set(int set)
{
    if (Pics[set]) {
        UnloadShapeList(Pics[set]);
        Pics[set] = 0;
    }
}
