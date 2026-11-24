extern signed char previous_track_range;
#pragma pack(1)
typedef struct {
    unsigned char number;                             /* 00 */
    unsigned char left, right, above, below;           /* 01-04 */
    unsigned char above_left, above_right;             /* 05-06 */
    unsigned char below_left, below_right;             /* 07-08 */
    unsigned char custom;                             /* 09 */
} ROOMREC;

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
    signed char ambient_cues[32][30];                 /* 2b39-2ef8 */
} LEVELREC;
#pragma pack()

#pragma pack(1)

typedef unsigned int SoundHandle;
typedef unsigned char far *SoundData;

typedef struct {
    unsigned char mode;
    unsigned char priority;
    unsigned char attribute;
} SoundRule;

typedef struct {
    unsigned char enabled;
    signed char cue;
    int song;
    int delay;
    int reserved;
} AmbientState;

static int death_songs[] = {
    100, 103, 73, 102, 104, 150, 145, 149, 40,
    100, 208, 0, 191, 207, 50, 51, 58, 264
};

static int track_first[] = {-1, 191, 72, -1, -1, 86, 86, -1, 69, 196, 0};
static int track_last[] = {-1, 195, 76, -1, -1, 90, 90, -1, 71, 199, 0};

int background_scene_group[] = {-1, 37, 204, 100, 145, 246, 256};
int background_scene_number[] = {-1, 46, 245, 142, 190, 249, 272};

static int level5_songs[] = {246};
static int level5_song_counts[] = {3};
static int level3_songs[] = {107, 123, 139};
static int level3_song_counts[] = {15, 15, 3};
static int level4_songs[] = {155, 171, 187};
static int level4_song_counts[] = {15, 15, 3};
static int level2_songs[] = {210, 226, 242};
static int level2_song_counts[] = {15, 15, 3};
static int extra_ambient_songs[] = {256};
static int extra_ambient_song_counts[] = {16};

static int *ambient_first[] = {0, 0, level2_songs, level3_songs, level4_songs,
                              level5_songs, 0};
static int *ambient_count[] = {0, 0, level2_song_counts, level3_song_counts,
                              level4_song_counts, level5_song_counts, 0};
static signed char ambient_special[] = {-1, 1, 2, 2, 2, -1, -1};

int background_intro_group[] = {77, 97, -1, 83, 80, 53, 94, -1};
int background_intro_number[] = {79, 99, -1, 85, 82, 55, 95, -1};

static char no_load_sound[] = "NOLOADSOUND";
static char no_sounds[] = "NOSOUNDS";

extern int current_effect, current_song;
extern LEVELREC Level;
extern unsigned char active_sound_channels;
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
extern CHARREC Char;
extern CHARREC Kid;
extern CHARREC RoomOpp[5];
extern SoundRule sound_rules[];
extern AmbientState ambient_state;
extern ROOMREC Room;

extern int sound_inhibit, ambient_last_song, pending_song;
extern SoundHandle sound_handles[];
extern char ambient_on_message[], ambient_off_message[], music_unavailable_message[];

extern int query_sound(int resource);
extern int OppToFight(void);
extern void *find_active_generator(unsigned char room, signed char column);
extern void load_actor_slot(signed char sound);
extern SoundData MATERIALIZE_HANDLE(SoundHandle handle);
extern void DISPOSE_HANDLE(SoundHandle handle);
extern SoundHandle hGetSnd(int resource);
extern SoundHandle MARK_PURGEABLE(SoundHandle handle);
extern SoundData RESOLVE_HANDLE_TO_FAR_PAYLOAD(SoundHandle handle);
extern int find_option(const char far *option);
extern void COPY_KID_TO_CHAR(void);
extern unsigned RND(unsigned limit);
extern signed char FIRSTINROW(signed char room);
extern void dispatch_sound(int resource);
extern int pending_effect;
extern void process_handle(SoundHandle handle);
extern int IsExitChasingSkel(signed char room);
extern void SaveOptionsToDisk(void);
extern void *memset(void *, int, unsigned int);
#pragma intrinsic(memset)

void AddSong(int song);
void AddSound(int effect);
int GetLeaveLevelSongId(void);
SoundData load_song(int song);
void release_song_range(int first, int last);
void load_song_range(int first, int last);
int _fastcall should_change_ambient(AmbientState *state);
int select_special_ambient(int special);
void play_song(int song);
int should_replace_effect(int effect, int playing);
void remember_ambient_song(AmbientState *state, int song);

void AddKidDeathSong(signed char kind)
{
    AddSong(death_songs[kind]);
}

void AddLeaveLevelSong(void)
{
    int song = GetLeaveLevelSongId();

    if (song != -1)
        AddSong(song);
}

void _fastcall AddKidScream(unsigned char room)
{
    if ((Char.character_kind == 0 && Level.environment_kind != 5) ||
        (Level.environment_kind == 5 && room != 19 && room != 16 && room != 15))
        AddSound(1);
}

void add_opponent_death_song(void)
{
    int allowed;
    int song;

    if (Char.field_0f) {
        switch (Char.character_kind) {
        case 2:
            allowed = Char.alive >= 6 ? 1 : 0;
            song = 209;
            break;
        case 7:
        case 8:
            allowed = !query_sound(10088);
            song = 147;
            break;
        case 10:
            allowed = !query_sound(10195);
            song = 192;
            break;
        case 11:
            Char.field_0f = 0;
        default:
            allowed = Kid.alive < 0 && Char.sequence == 120 &&
                      Char.alive > 4 && !query_sound(10076) &&
                      !query_sound(10097) && !query_sound(10098) &&
                      Kid.alive < 0 ? 1 : 0;
            song = 74;
            break;
        }
        if (allowed) {
            if (!OppToFight() && !find_active_generator(Char.room, Char.row) && Level.environment_kind != 5)
                AddSong(song);
            Char.field_0f = 0;
        }
    }
}

void add_actor_sfx(void)
{
    signed char sound = Kid.opponent_index;

    if (sound != -1) {
        load_actor_slot(sound);
        if ((Kid.frame == 167 || Char.frame == 167) &&
            Char.character_kind != 7 && Char.character_kind != 8)
            AddSound(12);
    }
}

void AddSong(int song)
{
    if (pending_song == -1) {
        load_song(song);
        pending_song = song;
    }
}


void AddSound(int effect)
{
    if ((pending_effect == -1 || sound_rules[effect].priority <= sound_rules[pending_effect].priority) &&
        !sound_inhibit && Char.character_kind != 1)
        pending_effect = effect;
}

SoundData load_song(int song)
{
    SoundData data;

    data = sound_handles[song] ? MATERIALIZE_HANDLE(sound_handles[song]) : (SoundData)0;
    if (!data) {
        if (sound_handles[song])
            DISPOSE_HANDLE(sound_handles[song]);
        if ((sound_handles[song] = hGetSnd(song + 10000)) != 0)
            data = RESOLVE_HANDLE_TO_FAR_PAYLOAD(sound_handles[song] = MARK_PURGEABLE(sound_handles[song]));
    }
    return data;
}

int GetLeaveLevelSongId(void)
{
    int song;

    switch (Level.environment_kind) {
    case 1:
        song = -1;
        break;
    case 2:
        song = 30;
        break;
    case 4:
        song = 29;
        break;
    default:
        song = 28;
        break;
    }
    return song;
}

void init_ambient(AmbientState *state)
{
    memset(state, 0, sizeof(*state));
    state->enabled = active_sound_channels & 2;
}

int is_leave_level_music_on(void)
{
    int playing;
    int song = GetLeaveLevelSongId();

    if (song != -1) {
        song += 10000;
        playing = query_sound(song);
    } else {
        playing = 0;
    }
    return playing;
}

void change_song_range(signed char range)
{
    if (range != previous_track_range && previous_track_range != -1 && track_first[previous_track_range] != -1)
        release_song_range(track_first[range], track_last[range]);
    if (range != -1 && track_first[range] != -1)
        load_song_range(track_first[range], track_last[range]);
}

void load_song_range(int first, int last)
{
    int song;
    SoundHandle *handle;
    int start;

    if (!find_option(no_load_sound))
        if (first != -1) {
            start = first;
            song = start;
            if (first > last)
                return;
            handle = &sound_handles[song];
            do {
                if (*handle)
                    MATERIALIZE_HANDLE(*handle);
                if (!*handle || !MATERIALIZE_HANDLE(*handle)) {
                    if (*handle)
                        DISPOSE_HANDLE(*handle);
                    *handle = hGetSnd(song + 10000);
                }
                if (*handle)
                    *handle = MARK_PURGEABLE(*handle);
                ++handle;
            } while (++song <= last);
        }
}

void play_ambient(AmbientState *state)
{
    int special, song, first, count;
    signed char cue, cell;
    int *songs;

    if (Kid.room) {
        COPY_KID_TO_CHAR();
        if (should_change_ambient(state) && ambient_first[Level.environment_kind] != 0) {
            special = OppToFight();
            if (special && ambient_special[Level.environment_kind] == -1)
                special = 0;
            song = select_special_ambient(special);
            if (song == -1) {
                if (!special) {
                    cell = FIRSTINROW(Char.row) + Char.column;
                    cue = Level.ambient_cues[(unsigned char)Char.room - 1][cell];
                } else {
                    cue = ambient_special[Level.environment_kind];
                }
                songs = ambient_first[Level.environment_kind];
                first = songs[cue];
                count = ambient_count[Level.environment_kind][cue];
                song = songs[cue] + RND(count);
                if (state->song == song) {
                    if (first + count != song)
                        ++song;
                    else
                        song = first;
                }
            } else {
                cue = -1;
            }
            play_song(song);
            state->cue = cue;
            state->song = song;
        }
    }
}

void play_sounds(void)
{
    if (!find_option(no_sounds)) {
        if (should_replace_effect(pending_effect, current_effect)) {
            dispatch_sound(current_effect + 10000);
            play_song(current_effect = pending_effect);
        }
        pending_effect = -1;
        if (pending_song != -1) {
            dispatch_sound(current_song + 10000);
            play_song(current_song = pending_song);
            remember_ambient_song(&ambient_state, pending_song);
            pending_song = -1;
        }
    }
}

void play_song(int song)
{
    if (!sound_handles[song] || !MATERIALIZE_HANDLE(sound_handles[song]))
        sound_handles[song] = hGetSnd(song + 10000);
    process_handle(sound_handles[song]);
}

int should_replace_effect(int effect, int playing)
{
    int replace = 0;

    if (effect != -1 &&
        ((active_sound_channels & 3) || current_song == -1 || !query_sound(current_song + 10000))) {
        if (!query_sound(playing + 10000)) {
            replace = 1;
        } else {
            if (((sound_rules[playing].mode == 1 && playing != effect) || sound_rules[playing].mode == 2) &&
                sound_rules[playing].priority >= sound_rules[effect].priority)
                replace = 1;
        }
    }
    return replace;
}

int select_special_ambient(int special)
{
    int song = -1;

    if (special && Level.level_number == 5 && (Room.number == 10 || Room.number == 7 || Room.number == 12))
        song = 64;
    else if (special && RoomOpp[0].character_kind == 12)
        song = 198;
    else if (special && RoomOpp[0].character_kind == 10)
        song = 199;
    else if (Kid.character_kind == 1 && Kid.life_points > 2)
        song = 60;
    else if (Level.level_number == 13 && (Room.number == 4 || Room.number == 29))
        song = 61;
    return song;
}

int special_no_ambient_case(void)
{
    int blocked = 0;

    if (Kid.alive > 0)
        blocked = 1;
    if (is_leave_level_music_on() ||
        (Level.preferred_opponent == 2 && IsExitChasingSkel(RoomOpp[0].room) &&
         (RoomOpp[0].sequence == 88 || RoomOpp[0].sequence == 102 || RoomOpp[0].sequence == 216)) ||
        (Kid.room == 16 && Level.level_number == 9) ||
        (Kid.room == 9 && Level.level_number == 8 && Kid.column >= 8))
        blocked = 1;
    return blocked;
}

int _fastcall should_change_ambient(AmbientState *state)
{
    int change = 0;
    int special;
    union {
        signed char value;
        int word;
    } cue;

    if (state->enabled && Char.alive < 0 && !special_no_ambient_case()) {
        if (!query_sound(state->song + 10000)) {
            change = 1;
        } else if (state->cue != -1 || (Level.level_number == 8 && query_sound(10255))) {
            special = OppToFight();
            if (special && ambient_special[Level.environment_kind] == -1)
                special = 0;
            if (special) {
                if (state->cue < ambient_special[Level.environment_kind])
                    change = 1;
            } else if (!special) {
                cue.value = ambient_special[Level.environment_kind];
                if (cue.value <= state->cue && cue.value != -1 && !find_active_generator(Char.room, Char.row) &&
                    (Level.level_number != 8 || !query_sound(10255)))
                    change = 1;
            }
        }
    }
    return change;
}

char *toggle_ambient(void)
{
    char *message;
    if (active_sound_channels & 2) {
        ambient_state.enabled = !ambient_state.enabled;
        message = ambient_state.enabled ? ambient_on_message : ambient_off_message;
        if (!ambient_state.enabled && ambient_last_song && pending_song != ambient_last_song)
            dispatch_sound(ambient_last_song + 10000);
        SaveOptionsToDisk();
    } else {
        message = music_unavailable_message;
    }
    return message;
}

void remember_ambient_song(AmbientState *state, int song)
{
    SoundData data = (SoundData)load_song(song);

    if (state->enabled && (*data & 2)) {
        state->song = song;
        state->cue = -1;
    }
}

void release_song_range(int first, int last)
{
    int song;
    SoundHandle *handle;
    int start;

    if (first != -1) {
        start = first;
        song = start;
        if (first > last)
            return;
        handle = &sound_handles[song];
        do {
            if (*handle && !query_sound(song + 10000)) {
                DISPOSE_HANDLE(*handle);
                *handle = 0;
            }
            ++handle;
        } while (++song <= last);
    }
}
