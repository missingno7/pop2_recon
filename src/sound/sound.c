#include "types.h"
#include "level.h"
#include "char.h"
#include "game.h"
extern signed char LastOppTypeLoaded;

#pragma pack(1)

typedef unsigned int SoundHandle;
typedef unsigned char far *SoundData;

typedef struct {
    unsigned char mode;
    unsigned char priority;
    unsigned char attribute;
} SoundRule;

extern LEVELREC Level;
extern struct { unsigned char channels, volume; } Midi;
int CueSnd = -1, CueSong = -1;
int current_effect = -1, current_song = -1;
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

extern CHARREC Char;
extern CHARREC Kid;
extern CHARREC RoomOpp[5];
extern SoundRule Snd[];
extern AmbientState ambient_state;
extern ROOMREC Room;

extern int WeightlessTimer;
SoundHandle sound_handles[273];
extern char ambient_on_message[], ambient_off_message[], music_unavailable_message[];

extern int SndOn(int resource);
extern int OppToFight(void);
extern void *GetOppGenPtOnRow(unsigned char room, signed char column);
extern void LoadShad(signed char sound);
extern SoundData MATERIALIZE_HANDLE(SoundHandle handle);
extern void hFree(SoundHandle handle);
extern SoundHandle hGetSnd(int resource);
extern SoundHandle hPurge(SoundHandle handle);
extern SoundData h2p(SoundHandle handle);
extern int find_option(const char far *option);
extern void LoadKid(void);
extern unsigned RND(unsigned limit);
extern signed char FIRSTINROW(signed char room);
extern void ResetSnd(int resource);
extern void process_handle(SoundHandle handle);
extern int IsExitChasingSkel(signed char room);
extern void SaveOptionsToDisk(void);
extern void *memset(void *, int, unsigned int);
#pragma intrinsic(memset)

void AddSong(int song);
void AddSound(int effect);
int GetLeaveLevelSongId(void);
SoundData CheckSnd(int song);
void UnloadSounds(int first, int last);
void LoadSounds(int first, int last);
int _fastcall TimeForNewAmbient(AmbientState *state);
int SpecialMusic(int special);
void play_song(int song);
int should_replace_effect(int effect, int playing);
void UpdateAmbient(AmbientState *state, int song);

void AddSFX(void);
void AddOppDeathSong(void);
void AddKidDeathSong(signed char kind);
void _fastcall AddKidScream(unsigned char room);
void AddLeaveLevelSong(void);
void change_song_range(signed char range);
void InitAmbient(AmbientState *state);

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

void AddOppDeathSong(void)
{
    int allowed;
    int song;

    if (Char.repeat) {
        switch (Char.character_kind) {
        case 2:
            allowed = Char.alive >= 6 ? 1 : 0;
            song = 209;
            break;
        case 7:
        case 8:
            allowed = !SndOn(10088);
            song = 147;
            break;
        case 10:
            allowed = !SndOn(10195);
            song = 192;
            break;
        case 11:
            Char.repeat = 0;
        default:
            allowed = Kid.alive < 0 && Char.sequence == 120 &&
                      Char.alive > 4 && !SndOn(10076) &&
                      !SndOn(10097) && !SndOn(10098) &&
                      Kid.alive < 0 ? 1 : 0;
            song = 74;
            break;
        }
        if (allowed) {
            if (!OppToFight() && !GetOppGenPtOnRow(Char.room, Char.row) && Level.environment_kind != 5)
                AddSong(song);
            Char.repeat = 0;
        }
    }
}

void AddSFX(void)
{
    signed char sound = Kid.opponent_index;

    if (sound != -1) {
        LoadShad(sound);
        if ((Kid.frame == 167 || Char.frame == 167) &&
            Char.character_kind != 7 && Char.character_kind != 8)
            AddSound(12);
    }
}

void AddSong(int song)
{
    if (CueSong == -1) {
        CheckSnd(song);
        CueSong = song;
    }
}

void AddSound(int effect)
{
    if ((CueSnd == -1 || Snd[effect].priority <= Snd[CueSnd].priority) &&
        !WeightlessTimer && Char.character_kind != 1)
        CueSnd = effect;
}

SoundData CheckSnd(int song)
{
    SoundData data;

    data = sound_handles[song] ? MATERIALIZE_HANDLE(sound_handles[song]) : (SoundData)0;
    if (!data) {
        if (sound_handles[song])
            hFree(sound_handles[song]);
        if ((sound_handles[song] = hGetSnd(song + 10000)) != 0)
            data = h2p(sound_handles[song] = hPurge(sound_handles[song]));
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

void InitAmbient(AmbientState *state)
{
    memset(state, 0, sizeof(*state));
    state->enabled = Midi.channels & 2;
}

int IsLeaveLevelMusicOn(void)
{
    int playing;
    int song = GetLeaveLevelSongId();

    if (song != -1) {
        song += 10000;
        playing = SndOn(song);
    } else {
        playing = 0;
    }
    return playing;
}

void change_song_range(signed char range)
{
    if (range != LastOppTypeLoaded && LastOppTypeLoaded != -1 && track_first[LastOppTypeLoaded] != -1)
        UnloadSounds(track_first[range], track_last[range]);
    if (range != -1 && track_first[range] != -1)
        LoadSounds(track_first[range], track_last[range]);
}

void LoadSounds(int first, int last)
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
                        hFree(*handle);
                    *handle = hGetSnd(song + 10000);
                }
                if (*handle)
                    *handle = hPurge(*handle);
                ++handle;
            } while (++song <= last);
        }
}

void PlayAmbient(AmbientState *state)
{
    int special, song, first, count;
    signed char cue, cell;
    int *songs;

    if (Kid.room) {
        LoadKid();
        if (TimeForNewAmbient(state) && ambient_first[Level.environment_kind] != 0) {
            special = OppToFight();
            if (special && ambient_special[Level.environment_kind] == -1)
                special = 0;
            song = SpecialMusic(special);
            if (song == -1) {
                if (!special) {
                    cell = FIRSTINROW(Char.row) + Char.column;
                    cue = Level.ambient_cues[Char.room - 1][cell];
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

void PlaySounds(void)
{
    if (!find_option(no_sounds)) {
        if (should_replace_effect(CueSnd, current_effect)) {
            ResetSnd(current_effect + 10000);
            play_song(current_effect = CueSnd);
        }
        CueSnd = -1;
        if (CueSong != -1) {
            ResetSnd(current_song + 10000);
            play_song(current_song = CueSong);
            UpdateAmbient(&ambient_state, CueSong);
            CueSong = -1;
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
        ((Midi.channels & 3) || current_song == -1 || !SndOn(current_song + 10000))) {
        if (!SndOn(playing + 10000)) {
            replace = 1;
        } else {
            if (((Snd[playing].mode == 1 && playing != effect) || Snd[playing].mode == 2) &&
                Snd[playing].priority >= Snd[effect].priority)
                replace = 1;
        }
    }
    return replace;
}

int SpecialMusic(int special)
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

int SpecialNoAmbientCase(void)
{
    int blocked = 0;

    if (Kid.alive > 0)
        blocked = 1;
    if (IsLeaveLevelMusicOn() ||
        (Level.preferred_opponent == 2 && IsExitChasingSkel(RoomOpp[0].room) &&
         (RoomOpp[0].sequence == 88 || RoomOpp[0].sequence == 102 || RoomOpp[0].sequence == 216)) ||
        (Kid.room == 16 && Level.level_number == 9) ||
        (Kid.room == 9 && Level.level_number == 8 && Kid.column >= 8))
        blocked = 1;
    return blocked;
}

int _fastcall TimeForNewAmbient(AmbientState *state)
{
    int change = 0;
    int special;
    union {
        signed char value;
        int word;
    } cue;

    if (state->enabled && Char.alive < 0 && !SpecialNoAmbientCase()) {
        if (!SndOn(state->song + 10000)) {
            change = 1;
        } else if (state->cue != -1 || (Level.level_number == 8 && SndOn(10255))) {
            special = OppToFight();
            if (special && ambient_special[Level.environment_kind] == -1)
                special = 0;
            if (special) {
                if (state->cue < ambient_special[Level.environment_kind])
                    change = 1;
            } else if (!special) {
                cue.value = ambient_special[Level.environment_kind];
                if (cue.value <= state->cue && cue.value != -1 && !GetOppGenPtOnRow(Char.room, Char.row) &&
                    (Level.level_number != 8 || !SndOn(10255)))
                    change = 1;
            }
        }
    }
    return change;
}

char *toggle_ambient(void)
{
    char *message;
    if (Midi.channels & 2) {
        ambient_state.enabled = !ambient_state.enabled;
        message = ambient_state.enabled ? ambient_on_message : ambient_off_message;
        if (!ambient_state.enabled && ambient_state.song && CueSong != ambient_state.song)
            ResetSnd(ambient_state.song + 10000);
        SaveOptionsToDisk();
    } else {
        message = music_unavailable_message;
    }
    return message;
}

void UpdateAmbient(AmbientState *state, int song)
{
    SoundData data = (SoundData)CheckSnd(song);

    if (state->enabled && (*data & 2)) {
        state->song = song;
        state->cue = -1;
    }
}

void UnloadSounds(int first, int last)
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
            if (*handle && !SndOn(song + 10000)) {
                hFree(*handle);
                *handle = 0;
            }
            ++handle;
        } while (++song <= last);
    }
}
