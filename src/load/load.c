/* LOAD.C, resident resource loading and overlay selection.
 * Declarations outside the verified claims retain the working TU hypotheses.
 * See notes.md for the exact claims and the unresolved draft extents.
 */
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
typedef struct {
    Byte x, y;
    int flags;
} LevelStart;

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


extern void near *back_port;
extern Rect back_port_bounds;
extern Byte background;
extern int background_overlay_flags[8];
extern int background_intro_group[], background_intro_number[];
static int flask_enabled;
extern unsigned overlay_output;
extern Handle sprite_sets[];
extern Byte level_number;
extern int sound_available;
extern int active_map, resume_mode;
extern char level_effect[];
extern char map_archive[], background_overlay_file[];
extern Byte opponent, start_room_count;
extern int extra_kid_overlay;
static int sword_overlay_enabled[3];
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
static int background_overlay_first[8] = { 115, 127, 90, 95, 100, 79, 91, 105 };
static const int background_overlay_last[8] = { 126, 161, 91, 114, 103, 90, 94, 108 };
static const int kid_overlay_first[11] = { 263, 223, 241, 244, 249, 269, 222, 271, 280, 290, 300 };
static const int kid_overlay_last[11] = { 268, 240, 243, 248, 262, 270, 222, 279, 289, 298, 313 };
static const int kid_image_first[9] = { 15, 44, 99, 120, 133, 160, 219, 222, -1 };
static const int kid_image_last[8] = { 32, 79, 111, 129, 144, 215, 220, 299 };
static const char map_option[] = "GAMEPLAY\0%s%s";

extern SByte sword_kind;
extern Handle level_pieces, sword_frames, opponent_frames, opponent_effect_frames;
extern Byte play_state, room_feature, music_number;
extern KidRecord far * volatile saved_kid;
extern int *level_start_table[];
extern void far *level_sequence_table[];
extern int *level_starts;
extern void far *level_sequences;
extern int *sequence_flags;
extern Byte *sequence_opcodes;
extern LevelStart room_starts[];
extern Byte start_x, start_y;
extern unsigned long room_object_info[];
extern Byte room_trigger_data[];
extern int *room_init_ptr;
extern int room_init_data[];
extern Byte opponent_overlay_one, opponent_overlay_two;
extern SByte requested_map;
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
extern void far pascal load_opponent_sound(SByte);
extern void far pascal load_resource_palette(unsigned, int, Byte, Byte);
extern void * far cdecl _nmalloc(unsigned);
extern void far cdecl _nfree(void *);
extern void * far cdecl memset(void *, int, unsigned);
#pragma intrinsic(memset)
extern void far pascal RemoveRebirthPt(void);
extern int far pascal IsLevelStartWithNISScreen(void);
extern void far pascal STROKE_PORT_RECT(const char far *);
extern void far pascal reset_desert_variant(void);
extern void * far pascal find_option(const char far *);
extern void far pascal LOAD_RESOURCE_INTO(int, int, int, const char far *, long);
extern void far pascal SetRebirthPtLevelStatus(void);
extern void far pascal save_game_play_palette(void);
extern void far pascal load_overlay_one(void);
extern void far pascal load_overlay_two(void);
extern void far pascal load_overlay_three(void);
extern void far pascal load_overlay_four(void);
extern void far pascal load_overlay_five(void);
extern void far pascal load_overlay_six(void);
extern void far pascal load_overlay_seven(void);
extern void far pascal load_overlay_eight(void);
extern void far pascal load_overlay_nine(void);
extern void far pascal load_overlay_ten(void);
extern void far pascal load_overlay_eleven(void);
extern Handle far pascal LOAD_RESOURCE_COPY(unsigned long, unsigned);
extern void far pascal DISPOSE_HANDLE(Handle);
extern Handle far pascal HGETRSRC(unsigned long, unsigned);
extern void far pascal RELEASE_TAGGED_RESOURCE(unsigned long, unsigned);
extern unsigned long far * far pascal resolve_handle_to_far_payload(Handle);
extern void far pascal enter_room(Byte);
extern void far pascal MOVETO(int, int);
extern void far _fastcall load_opponent_extra(Byte);
extern Handle far pascal hGetSnd(unsigned);
extern void far pascal SAVE_HANDLE_FLAGS(Handle);
extern const char far * far pascal FETCH_LOCKED_SHAPE_LIST(Handle);
extern char far * far cdecl copy_far_string(char far *, const char far *);
extern void far cdecl split_path(char *, char *, char *, char *, char *);
extern int far cdecl sprintf(char *, const char *, ...);
extern void far pascal abort_with_message(int, const char far *);
extern void far pascal clear_peels(void);
extern void far pascal UnloadCustomRoomData(void);

void far pascal unload_set(register int set);
void far pascal check_background_overlays(Byte kind);
void far pascal load_set(register int set, unsigned id, unsigned limit, int count, int mode);
void far pascal unload_level(int first);
void far pascal load_level_map(SByte map);
void far pascal load_kid_set(void);
void far pascal load_kid_set_overlays(void);
void far pascal load_opponent_set(SByte kind);
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
            if (previous == 6)
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
    if (back_port == 0)
        back_port = AllocBackPort(&back_port_bounds);
}

void far pascal load_background_set(int palette)
{
    unsigned limit = 0x3ff0;

    if (background == 5)
        limit = 0x7ff0;
    else if (background == 6)
        limit = 0xfff0;

    load_set(4, 3500, limit, 1, 0);
    if (palette)
        load_resource_palette(3500, 64, 160, 0);
    check_background_overlays(background);
    background_image_count = 0;
    if (background_image_first[background])
        PrinceMarkRangesPurgeable(sprite_sets[4], background_image_first[background],
                             background_image_last[background]);
}

void far pascal check_background_overlays(Byte kind)
{
    int i;

    for (i = 0; i < 8; ++i) {
        if (background_overlay_flags[i]) {
            if (background_overlay_first[i] != -1)
                overlay_shape_list(sprite_sets[4], 3700, background_overlay_first[i] - 1,
                                   background_overlay_last[i] - 1, 1);
            if (background_intro_group[i] != -1)
                load_song_range(background_intro_group[i], background_intro_number[i]);
        }
    }
}

void far pascal load_flask_set(void)
{
    if (flask_enabled) {
        Byte *entries = _nmalloc(35);
        int i;

        memset(entries, 2, 35);
        for (i = 2; i <= 21; ++i)
            entries[i] = 1;
        load_set(1, 3000, 0x4000, (int)entries, 0);
        _nfree(entries);
        load_resource_palette(3000, 224, 16, 0);
    } else {
        unload_set(1);
    }
}

void far pascal load_level(SByte number)
{
    Byte previous;

    unload_level(3);
    previous = background;
    if (number != -2 && number != -1) {
        if (number != requested_map) {
            if (resume_mode == 0)
                RemoveRebirthPt();
            requested_map = number;
        }
    }
    active_map = requested_map;
    CheckBPort();
    load_level_map(requested_map);
    if (!IsLevelStartWithNISScreen())
        STROKE_PORT_RECT(level_effect);
    load_flask_set();
    open_level_resource_files(background, previous);
    load_background_set(!IsLevelStartWithNISScreen());
    load_kid_set();
    load_kid_set_overlays();
    load_opponent_set(opponent);
    set_kid_sword_kind();
    load_sword_set(sword_kind);
    load_song_range(0, 26);
    load_pieces_data();
    if (background != previous) {
        switch_background_overlay(background, previous);
        set_kid_palette(background);
    }
    load_code_overlays(background, opponent);
    if (number != -2 && number != -1 && background == 1)
        reset_desert_variant();
    set_level_spec_ptrs(background);
}

void far pascal load_level_map(SByte map)
{
    int id = map + 1999;
    if (find_option(map_option))
        id += 20;
    LOAD_RESOURCE_INTO(0, 0, id, map_archive, 12025L);
    set_initial_room_state();
    SetRebirthPtLevelStatus();
}

void far pascal load_minimum_level(SByte number)
{
    load_level_map(number);
    set_kid_sword_kind();
    load_sword_set(sword_kind);
    save_game_play_palette();
    room_init_ptr = room_init_data;
    STROKE_PORT_RECT(level_effect);
}

extern int background_scene_group[], background_scene_number[];

void far pascal switch_background_overlay(Byte current, Byte previous)
{
    if (previous != current && background_scene_group[previous] != -1)
        release_song_range(background_scene_group[previous], background_scene_number[previous]);
    if (background_scene_group[background] != -1)
        load_song_range(background_scene_group[background], background_scene_number[background]);
}

void far pascal load_code_overlays(Byte background_kind, Byte opponent_kind)
{
    switch (background_kind) {
    case 1:
        load_overlay_one();
        break;
    case 2:
        load_overlay_two();
        load_overlay_three();
        break;
    case 3:
        load_overlay_four();
        load_overlay_five();
        break;
    case 4:
        load_overlay_six();
        load_overlay_seven();
        break;
    case 5:
        load_overlay_eight();
        break;
    case 6:
        load_overlay_nine();
        break;
    }
    if (opponent_kind == 5 || opponent_kind == 6 || opponent_kind == 8)
        load_overlay_ten();
    else
        load_overlay_eleven();
}

void far pascal load_pieces_data(void)
{
    unload_pieces_data();
    level_pieces = LOAD_RESOURCE_COPY(0x50494543UL, 3500);
}

void far pascal load_sword_set(SByte kind)
{
    unsigned id;
    int mode;
    SByte i;

    if (loaded_sword != kind) {
        if (sword_frames) {
            DISPOSE_HANDLE(sword_frames);
            sword_frames = 0;
        }
        unload_set(0);
    }
    if (sword_frames == 0)
        sword_frames = load_sword_frames(kind);

    id = 1000;
    if (kind == 2)
        id = 1200;
    mode = play_state == 1 && level_number == 14 ? 0 : 1;
    load_set(0, id, 0x8000, 2, mode);
    if (kind == 1) {
        for (i = 0; i < 1; ++i) {
            if (need_sword_overlay(i)) {
                if (!IsOverlayLoaded(sprite_sets[0], sword_overlay_first[i]))
                    overlay_shape_list(sprite_sets[0], 1100, sword_overlay_first[i],
                                       sword_overlay_last[i], 2);
            } else {
                unload_overlay(sprite_sets[0], sword_overlay_first[i], sword_overlay_last[i]);
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
        image = (int)resolve_handle_to_far_payload(sword_frames)[index];
    else
        image = -1;
    return image;
}

void far pascal set_level_spec_ptrs(Byte kind)
{
    level_starts = level_start_table[kind];
    level_sequences = level_sequence_table[kind];
}

void far _fastcall clear_special_sequence_flag(int i)
{
    int *flags = &sequence_flags[2 * i];
    switch ((unsigned)sequence_opcodes[i]) {
    case 7:
        *flags &= ~0x80;
        break;
    }
}

void far pascal set_initial_room_state(void)
{
    Byte room;
    int index;
    for (room = 1; room <= start_room_count; ++room) {
        enter_room(room);
        start_x = room_starts[room].x;
        start_y = room_starts[room].y;
        for (index = 0; index < 30; ++index)
            clear_special_sequence_flag(index);
    }
}

void far pascal load_set(register int set, unsigned id, unsigned limit, int count, int mode)
{
    if (sprite_sets[set] == 0)
        sprite_sets[set] = load_shape_list(id, limit, count, mode);
}

void far pascal load_kid_set(void)
{
    Byte *entries;
    int i, j;

    if (background == 6)
        open_verified_resource(level_resource_files[6]);
    entries = _nmalloc(314);
    memset(entries, 0, 314);
    for (i = 131; i <= 132; ++i)
        entries[i] = 1;
    for (j = 216; j <= 218; ++j)
        entries[j] = 1;
    load_set(2, 25001U, 2, (int)entries, 0);
    _nfree(entries);
    PrinceMarkRangesPurgeable(sprite_sets[2], kid_image_first, kid_image_last);
}

void far pascal load_kid_set_overlays(void)
{
    int i;
    for (i = 0; i < 11; ++i) {
        if (need_kid_overlay(i)) {
            if (!IsOverlayLoaded(sprite_sets[2], kid_overlay_first[i]))
                overlay_shape_list(sprite_sets[2], 24601, kid_overlay_first[i],
                                   kid_overlay_last[i], 0);
        } else {
            unload_overlay(sprite_sets[2], kid_overlay_first[i], kid_overlay_last[i]);
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
        needed = background_overlay_flags[0];
        break;
    case 1:
        needed = background == 4 || background == 2;
        break;
    case 2:
        needed = background == 3 || opponent == 5 || opponent == 6;
        break;
    case 3:
        needed = background_overlay_flags[3];
        break;
    case 5:
        needed = opponent == 5 || opponent == 6;
        break;
    case 6:
        needed = level_number == 5;
        break;
    case 8:
        needed = level_number == 13;
        break;
    case 9:
        needed = level_number == 13 || background == 6;
        break;
    case 4:
    case 7:
    case 10:
        needed = background == 6;
        break;
    }
    return needed;
}

void far pascal set_kid_palette(Byte kind)
{
    load_resource_palette(25001, 16, 16, (Byte)(kind - 1));
}

int far pascal need_opponent_overlay(SByte kind, int index)
{
    int needed = 0;
    if (kind != -1 && opponent_overlay_first[kind]) {
        int count = -1;
        if (opponent_overlay_first[kind][-1] != count) {
            int *first = ((int * volatile *)opponent_overlay_first)[*(volatile SByte *)&kind] - 1;
            do {
                ++count;
                ++first;
                } while (*first != -1);
        }
        if (count >= index) {
            if (index < 3)
                needed = sword_overlay_enabled[index];
            else if (kind == 7 || background == 6)
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
        if (sword_overlay_enabled[0])
            needed = 1;
        break;
    }
    return needed;
}

void far pascal load_opponent_set(SByte kind)
{
    MOVETO(50, 100);
    if (kind == -1 || kind != loaded_opponent || sprite_sets[3] == 0) {
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
        if (kind == loaded_opponent && sprite_sets[3]) {
            if (kind == loaded_opponent)
                load_opponent_overlays(kind);
        } else {
            open_verified_resource(opponent_resource_files[kind]);
            load_set(3, 750, 4, 2, 0);
            load_resource_palette(750, 32, 16, 0);
            if (opponent_overlay_first[kind]) {
                load_opponent_overlays(kind);
                PrinceMarkRangesPurgeable(sprite_sets[3], opponent_overlay_first[kind],
                                     opponent_overlay_last[kind]);
            }
            load_opponent_sound(kind);
            opponent_frames = HGETRSRC(0x4652414dUL, 750);
            if ((kind == 5 || kind == 6) && opponent_effect_frames == 0)
                opponent_effect_frames = HGETRSRC(0L, 755);
        }
    }
    loaded_opponent = kind;
}

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
                if (!IsOverlayLoaded(sprite_sets[3], first_base[i]))
                    overlay_shape_list(sprite_sets[3], 850, first_base[i], last_base[i], 2);
                if (i == 0 && (kind == 0 || kind == 5 || kind == 6)) {
                    if (!opponent_overlay_one)
                        load_opponent_extra(1);
                    if (!opponent_overlay_two)
                        load_opponent_extra(2);
                }
            } else {
                unload_overlay(sprite_sets[3], first_base[i], last_base[i]);
            }
            ++i;
        } while (first_base[i] != -1);
    }
}

extern Byte room_objects[];

typedef struct {
    unsigned flags, actor;
} RoomObjectInfo;

void far pascal preload_room_data(Byte room)
{
    int visited[64];
    SByte tile;

    memset(visited, 0, sizeof(visited));
    tile = 0;
    do {
        int index = tile + (unsigned)(Byte)30 * room;
        const Byte *object_code = &room_objects[index];
        const RoomObjectInfo *object_info = &((RoomObjectInfo *)room_object_info)[index];
        unsigned object = *object_code;
        unsigned info = object_info->flags;
        int *flag = &visited[object];
        if (*flag == 0) { visited[object] = 1;
            switch (object) {
            case 2: load_song_range(background_intro_group[0], background_intro_number[0]); break;
            case 10:
                if (((Byte)info & 0xe0) == 0xc0 && loaded_opponent != 9) load_opponent_set(9);
                /* fall through */
            case 23: load_song_range(background_intro_group[1], background_intro_number[1]); break;
            case 13: load_song_range(background_intro_group[3], background_intro_number[3]); break;
            case 25: load_song_range(background_intro_group[5], background_intro_number[5]); break;
            case 26: load_song_range(background_intro_group[6], background_intro_number[6]); break;
            case 36: load_song_range(background_intro_group[4], background_intro_number[4]); break;
            }
        }
    } while (++tile < 30);
    if (room_trigger_data[(unsigned)(Byte)116 * room]) load_opponent_sound(opponent);
    if ((SByte)music_number > 0) {
        if ((SByte)music_number < 33) hGetSnd(10016);
        else hGetSnd(10000);
        if ((SByte)music_number < 31) hGetSnd(10001);
    }
}

int far pascal load_resource_error(Handle resource)
{
    char path[120];
    char drive[4], directory[256], basename[256], extension[256];

    SAVE_HANDLE_FLAGS(resource);
    copy_far_string(path, FETCH_LOCKED_SHAPE_LIST(resource) + 16);
    split_path(path, drive, directory, basename, extension);
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

void far pascal set_kid_sword_kind(void)
{
    if (saved_kid)
        sword_kind = saved_kid->sword_kind;
    else if (level_number == 6)
        sword_kind = -1;
    else if (level_number == 7 ||
             (level_number == 8 && room_feature != 9 && saved_kid == 0))
        sword_kind = 2;
    else
        sword_kind = 1;
}

void far pascal unload_from(int first)
{
    int i;
    clear_peels();
    for (i = first; i < 5; ++i)
        unload_set(i);
    if (first <= 4 && sound_available)
        UnloadCustomRoomData();
}

void far pascal unload_level(int first)
{
    int i;
    unload_from(first);
    for (i = 0; i < 8; ++i) {
        if (background_overlay_flags[i])
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
    if (first <= 0 && sword_frames) {
        DISPOSE_HANDLE(sword_frames);
        sword_frames = 0;
        loaded_sword = 0;
    }
    unload_pieces_data();
    UnloadCustomRoomData();
}

void far pascal unload_opponent_overlays(SByte previous, SByte current)
{
    if (sprite_sets[3] && opponent != 5 && opponent != 6) {
        int *last_base = opponent_overlay_last[previous];
        int *first_base = opponent_overlay_first[previous];
        if (first_base) {
            int i = 0;
            if (*first_base == -1)
                return;
            do {
                if (current != previous || !need_opponent_overlay(current, i))
                    unload_overlay(sprite_sets[3], first_base[i], last_base[i]);
                ++i;
            } while (first_base[i] != -1);
        }
    }
}

void far pascal unload_pieces_data(void)
{
    if (level_pieces) {
        _nfree((void *)level_pieces);
        level_pieces = 0;
    }
}

void far pascal unload_set(register int set)
{
    if (sprite_sets[set]) {
        UnloadShapeList(sprite_sets[set]);
        sprite_sets[set] = 0;
    }
}
