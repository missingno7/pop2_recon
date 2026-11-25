/* Sounds and songs of the game. */
typedef byte far *SoundData;

extern int CueSnd;
extern int CueSong;
extern int current_effect;
extern int current_song;
extern int background_scene_group[];
extern int background_scene_number[];
extern int background_intro_group[];
extern int background_intro_number[];
extern Handle sound_handles[273];

void AddKidDeathSong(sbyte kind);
void _fastcall AddKidScream(byte room);
void AddLeaveLevelSong(void);
void AddOppDeathSong(void);
void AddSFX(void);
void AddSong(int song);
void AddSound(int effect);
void change_song_range(sbyte range);
SoundData CheckSnd(int song);
int GetLeaveLevelSongId(void);
void InitAmbient(AmbientState *state);
int IsLeaveLevelMusicOn(void);
void LoadSounds(int first, int last);
void play_song(int song);
void PlayAmbient(AmbientState *state);
void PlaySounds(void);
int should_replace_effect(int effect, int playing);
int SpecialMusic(int special);
int SpecialNoAmbientCase(void);
int _fastcall TimeForNewAmbient(AmbientState *state);
sbyte *toggle_ambient(void);
void UnloadSounds(int first, int last);
void UpdateAmbient(AmbientState *state, int song);
