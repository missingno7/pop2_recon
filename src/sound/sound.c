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

extern int pending_effect, pending_song, current_effect, current_song;
extern unsigned char level, scene, location_kind, sound_flags;
extern signed char opponent_kind, opponent_pending, stage, opponent_state;
extern signed char kid_room, kid_room_offset, kid_section, kid_sound, ambient_enabled;
extern signed char ambient_actor_room;
extern AmbientState ambient_state;
extern signed char previous_track_range, ambient_room, ambient_table[];
extern int kid_action, opponent_action, opponent_sequence, ambient_actor_action;
extern int sound_inhibit, ambient_last_song;
extern int track_first[], track_last[];
extern int near *ambient_first[];
extern int near *ambient_count[];
extern signed char ambient_special[];
extern SoundRule sound_rules[];
extern SoundHandle sound_handles[];
extern char no_load_sound[], no_sounds[];
extern char ambient_on_message[], ambient_off_message[], music_unavailable_message[];

extern int sound_playing(int resource);
extern int OppToFight(void);
extern void *find_active_generator(unsigned char room, signed char column);
extern void queue_actor_sound(signed char sound);
extern SoundData get_sound_data(SoundHandle handle);
extern void release_sound(SoundHandle handle);
extern SoundHandle read_sound(int resource);
extern SoundHandle prepare_sound(SoundHandle handle);
extern SoundData lock_sound_data(SoundHandle handle);
extern int command_option(const char far *option);
extern void advance_sound(void);
extern int random_below(int limit);
extern signed char map_room_row(signed char room);
extern void stop_sound(int resource);
extern void start_sound(SoundHandle handle);
extern int check_ambient_actor(signed char room);
extern void update_sound_menu(void);
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
    if ((opponent_kind == 0 && level != 5) ||
        (level == 5 && room != 19 && room != 16 && room != 15))
        AddSound(1);
}

void add_opponent_death_song(void)
{
    int allowed;
    int song;

    if (opponent_pending) {
        switch (opponent_kind) {
        case 2:
            allowed = stage >= 6 ? 1 : 0;
            song = 209;
            break;
        case 7:
        case 8:
            allowed = !sound_playing(10088);
            song = 147;
            break;
        case 10:
            allowed = !sound_playing(10195);
            song = 192;
            break;
        case 11:
            opponent_pending = 0;
        default:
            allowed = opponent_state < 0 && opponent_sequence == 120 &&
                      stage > 4 && !sound_playing(10076) &&
                      !sound_playing(10097) && !sound_playing(10098) &&
                      opponent_state < 0 ? 1 : 0;
            song = 74;
            break;
        }
        if (allowed) {
            if (!OppToFight() && !find_active_generator(kid_section, kid_room) && level != 5)
                AddSong(song);
            opponent_pending = 0;
        }
    }
}

void add_actor_sfx(void)
{
    signed char sound = kid_sound;

    if (sound != -1) {
        queue_actor_sound(sound);
        if ((opponent_action == 167 || kid_action == 167) &&
            opponent_kind != 7 && opponent_kind != 8)
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
    int old = pending_effect;

    if ((old == -1 || sound_rules[effect].priority <= sound_rules[old].priority) &&
        !sound_inhibit)
        pending_effect = effect;
}

SoundData load_song(int song)
{
    SoundData data;
    SoundHandle *handle = &sound_handles[song];

    data = *handle ? get_sound_data(*handle) : (SoundData)0;
    if ((unsigned long)data == 0) {
        if (*handle)
            release_sound(*handle);
        *handle = read_sound(song + 10000);
        if (*handle) {
            SoundHandle prepared = prepare_sound(*(*(SoundHandle *volatile *)&handle));
            *handle = prepared;
            data = lock_sound_data(prepared);
        }
    }
    return data;
}

int GetLeaveLevelSongId(void)
{
    int song;

    switch (level) {
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
    state->enabled = sound_flags & 2;
}

int is_leave_level_music_on(void)
{
    int playing;
    int song = GetLeaveLevelSongId();

    if (song != -1) {
        song += 10000;
        playing = sound_playing(song);
    } else {
        playing = 0;
    }
    return playing;
}

void change_song_range(signed char range)
{
    signed char prior;
    if ((prior = previous_track_range) != range && prior != -1 &&
        track_first[prior] != -1)
        release_song_range(track_first[range], track_last[range]);
    if (range != -1) {
        int index = range * 2;
        if (*(int *)((char *)track_first + index) != -1)
            load_song_range(*(int *)((char *)track_first + index), *(int *)((char *)track_last + index));
    }
}

void load_song_range(int first, int last)
{
    int song;
    SoundHandle *handle;

    if (!command_option(no_load_sound) && first != -1) {
        song = first;
        if (first > last)
            return;
        handle = &sound_handles[song];
        do {
            if (*handle)
                get_sound_data(*handle);
            if (!*handle || !get_sound_data(*handle)) {
                if (*handle)
                    release_sound(*handle);
                *handle = read_sound(song + 10000);
            }
            if (*handle)
                *handle = prepare_sound(*handle);
            ++handle;
        } while (++song <= last);
    }
}

void play_ambient(AmbientState *state)
{
    int special, song, first, count;
    int *song_table;
    signed char cue;

    if (ambient_enabled) {
        advance_sound();
        if (should_change_ambient(state) && ambient_first[level] != 0) {
            special = OppToFight();
            if (special && ambient_special[level] == -1)
                special = 0;
            song = select_special_ambient(special);
            if (song != -1) {
                cue = -1;
            } else {
                cue = !special ? ambient_table[(signed char)(map_room_row(kid_room) + kid_room_offset) +
                                              30 * (unsigned char)kid_section] : ambient_special[level];
                song_table = ambient_first[level];
                first = song_table[cue];
                count = ambient_count[level][cue];
                song = random_below(count) + song_table[cue];
                if (state->song == song) {
                    if (first + count != song)
                        ++song;
                    else
                        song = first;
                }
            }
            play_song(song);
            state->cue = cue;
            state->song = song;
        }
    }
}

void play_sounds(void)
{
    if (!command_option(no_sounds)) {
        if (should_replace_effect(pending_effect, current_effect)) {
            stop_sound(current_effect + 10000);
            play_song(current_effect = pending_effect);
        }
        pending_effect = -1;
        if (pending_song != -1) {
            stop_sound(current_song + 10000);
            play_song(current_song = pending_song);
            remember_ambient_song(&ambient_state, pending_song);
            pending_song = -1;
        }
    }
}

void play_song(int song)
{
    SoundHandle *handle = &sound_handles[song];

    if (!*handle || !get_sound_data(*handle))
        *handle = read_sound(song + 10000);
    start_sound(*(*(SoundHandle *volatile *)&handle));
}

int should_replace_effect(int effect, int playing)
{
    int replace = 0;

    if (effect != -1 &&
        ((sound_flags & 3) || current_song == -1 || !sound_playing(current_song + 10000))) {
        if (!sound_playing(playing + 10000) ||
            (((sound_rules[playing].mode == 1 && playing == effect) || sound_rules[playing].mode == 2) &&
             sound_rules[playing].priority >= sound_rules[effect].priority))
            replace = 1;
    }
    return replace;
}

int select_special_ambient(int special)
{
    int song = -1;
    if (special && scene == 5 && (ambient_room == 10 || ambient_room == 7 || ambient_room == 12))
        song = 64;
    return song;
}

int special_no_ambient_case(AmbientState *state)
{
    int blocked = 0;
    if (is_leave_level_music_on() ||
        (location_kind == 2 && check_ambient_actor(ambient_actor_room) &&
         (ambient_actor_action == 88 || ambient_actor_action == 102 || ambient_actor_action == 216)) ||
        (ambient_enabled == 16 && scene == 9))
        blocked = 1;
    return blocked;
}

int _fastcall should_change_ambient(AmbientState *state)
{
    int change = 0;
    int special;
    signed char cue;

    if (state->enabled && stage < 0 && !special_no_ambient_case(state)) {
        if (!sound_playing(state->song + 10000)) {
            change = 1;
        } else if (state->cue != -1) {
            special = OppToFight();
            if (special && ambient_special[level] == -1)
                special = 0;
            if (special && ambient_special[level] > state->cue)
                change = 1;
            else if (!special) {
                cue = ambient_special[level];
                if (cue <= state->cue && cue != -1 && !find_active_generator(kid_section, kid_room))
                    change = 1;
            }
        }
    }
    return change;
}

char *toggle_ambient(void)
{
    char *message;
    if (sound_flags & 2) {
        ambient_state.enabled = !ambient_state.enabled;
        message = ambient_state.enabled ? ambient_on_message : ambient_off_message;
        if (!ambient_state.enabled && ambient_last_song && pending_song != ambient_last_song)
            stop_sound(ambient_last_song + 10000);
        update_sound_menu();
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

    if (first != -1) {
        song = first;
        if (first > last)
            return;
        handle = &sound_handles[song];
        do {
            if (*handle && !sound_playing(song + 10000)) {
                release_sound(*handle);
                *handle = 0;
            }
            ++handle;
        } while (++song <= last);
    }
}
