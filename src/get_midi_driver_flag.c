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
