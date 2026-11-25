#include "types.h"
#include "snd.h"
typedef void (*Completion)(int, int, int);


static volatile int sound_startup_finished;


static struct SoundFiles {
    char digital[9];
    char midi[9];
    char presets[12];
} sound_files = { "DIGI.DRV", "MIDI.DRV", "PRESETS.DEF" };
SOUNDDRIVER midi_driver = 0, digital_driver = 0;
int midi_mode = 0, digital_mode = 0;
int midi_driver_flag = 0;
static void far *startup_presets = 0;
CONFIG *configuration = 0;

void cdecl initialize_sound_drivers(int wait_for_sound)
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

static volatile int sound_startup_finished;


void finish_sound_startup(void far *payload, word status)
{
    release_driver_payload(startup_presets);
    sound_startup_finished = 1;
}

int get_sound_startup_flag(void)
{
    return sound_startup_finished;
}


int get_midi_driver_flag(void)
{
    return midi_driver_flag;
}
