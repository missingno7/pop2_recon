typedef int (far pascal *SoundDriver)(void);
typedef void (far pascal *Completion)(int, int, int);

typedef struct SoundSettings {
    unsigned char reserved1[6];
    int digital_mode;
    int midi_mode;
    unsigned char reserved2[4];
    unsigned char digital_parameters[6];
    unsigned char midi_parameters[6];
} SoundSettings;

extern SoundSettings *configuration;
extern int digital_mode, midi_mode;
extern int midi_driver_flag;
extern SoundDriver digital_driver, midi_driver;
extern void far *startup_presets;
extern volatile int sound_startup_finished;

extern SoundDriver far cdecl load_sound_driver(const char far *path);
extern void far *far cdecl read_dos_file_to_buffer(const char far *path);
extern int far pascal probe_first_sound_driver(const void far *parameters);
extern int far pascal probe_second_sound_driver(const void far *parameters);
extern int far pascal configure_sound_devices(void far *parameters);
extern int far pascal start_sound_payload(void far *payload, Completion completion);
void far pascal finish_sound_startup(int unused1, int unused2, int unused3);

static struct SoundFiles {
    char digital[9];
    char midi[9];
    char presets[12];
} sound_files = { "DIGI.DRV", "MIDI.DRV", "PRESETS.DEF" };

void far cdecl initialize_sound_drivers(int wait_for_sound)
{
    const void far *digital_parameters;
    const void far *midi_parameters;
    int first_driver_ready;

    digital_mode = configuration->digital_mode;
    midi_mode = configuration->midi_mode;
    digital_parameters = configuration->digital_parameters;
    midi_parameters = configuration->midi_parameters;

    digital_driver = load_sound_driver(digital_mode ? sound_files.digital : (char *)0);
    midi_driver = load_sound_driver(midi_mode ? sound_files.midi : (char *)0);

    if (!probe_second_sound_driver(digital_parameters) && digital_mode != -1) {
        digital_mode = 0;
    }

    first_driver_ready = probe_first_sound_driver(midi_parameters);
    if (!first_driver_ready && midi_mode != -1) {
        midi_mode = first_driver_ready;
    }

    if (digital_mode == 0 && midi_mode > 32)
        midi_driver_flag = 1;
    startup_presets = midi_mode ? read_dos_file_to_buffer(sound_files.presets) : 0;

    if (midi_mode >= 40) {
        configure_sound_devices(0);
        sound_startup_finished = 1;
        if (startup_presets && first_driver_ready) {
            sound_startup_finished = 0;
            if (start_sound_payload(startup_presets, finish_sound_startup) && wait_for_sound) {
                if (!sound_startup_finished)
                    do { } while (!sound_startup_finished);
                return;
            }
        }
    } else {
        configure_sound_devices(startup_presets);
        sound_startup_finished = 1;
    }
}

extern void far *startup_presets;
extern volatile int sound_startup_finished;

extern void far cdecl release_driver_payload(void far *payload);

void far pascal finish_sound_startup(int unused1, int unused2, int unused3)
{
    release_driver_payload(startup_presets);
    sound_startup_finished = 1;
}

int far pascal get_sound_startup_flag(void)
{
    return sound_startup_finished;
}

/* This flag is set by the DOS sound-driver initialization code. */
extern int midi_driver_flag;

int far pascal get_midi_driver_flag(void)
{
    return midi_driver_flag;
}
