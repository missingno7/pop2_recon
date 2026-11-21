extern signed char sound_volume;
extern void far pascal set_sound_volume(signed char level);
extern void far pascal set_options_from_disk(void);

int far pascal toggle_sound(void)
{
    int enabled;

    if (sound_volume == 0) {
        set_sound_volume(15);
        enabled = 1;
    } else {
        set_sound_volume(0);
        enabled = 0;
    }
    set_options_from_disk();
    return enabled;
}
