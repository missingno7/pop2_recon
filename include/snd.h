/* Sound devices, and the settings SETUP stores for them. */
typedef struct {                        /* the settings file */
    int unknown_00;
    int control_mode, memory;
    int digital_mode, midi_mode;        /* sound devices; -1: to be found */
    byte unknown_0a[4];
    byte digital_parameters[6];
    byte midi_parameters[6];
} CONFIG;

typedef struct {
    byte channels, volume;              /* 1: digital sound, 2: music */
} MIDISTATE;

#pragma pack(1)
typedef struct {
    int y, x, center_y, center_x;
    sbyte button;
} JOYSTICK;
#pragma pack()

typedef int (*SOUNDDRIVER)(void);

extern MIDISTATE Midi;
extern byte midi_cue;
extern word digital_rate;
extern word digital_type;
extern JOYSTICK joystick_state;

int configure_sound_devices(void far *parameters);
void flag_sound_payload(void far *payload);
SOUNDDRIVER far cdecl load_sound_driver(const char far *path);
int probe_first_sound_driver(const void far *parameters);
int probe_second_sound_driver(const void far *parameters);
int query_sound_payload(const void far *payload);
void far * _cdecl read_dos_file_to_buffer(const char far *path);
void _cdecl release_driver_payload(void far *payload);
void reset_sound_drivers(void);
void ResetSound(const void far *payload);
void set_sound_level(byte level);
int start_sound_payload(const void far *payload, void (*completion)(void far *, word));

extern SOUNDDRIVER midi_driver;
extern SOUNDDRIVER digital_driver;
extern int midi_mode;
extern int digital_mode;
extern int midi_driver_flag;
extern CONFIG *configuration;

Handle convert_sound_handle(Handle handle);
char *find_option(const char far *name);
void finish_sound_startup(void far *payload, word status);
void flag_sound_resource(word id);
int get_midi_driver_flag(void);
int get_sound_startup_flag(void);
Handle hGetSnd(word id);
void cdecl initialize_sound_drivers(int wait_for_sound);
int load_sound_settings(sbyte *path);
int process_handle(Handle handle);
void ResetSnd(word id);
int SetSnd(word id);
int SndOn(int id);
void sound_payload_callback(void far *payload, word status);
