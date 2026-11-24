typedef struct {
    int unused;
    int control_mode;
    int memory;
    unsigned int digital_type, midi_type;
} GameOptions;

#pragma pack(1)
typedef struct {
    int x, y;
    int center_x, center_y;
    signed char button;
} GameportState;
#pragma pack()

extern GameOptions *configuration;
extern struct { unsigned char channels, volume; } Midi;
extern GameportState joystick_state;
extern void POLL_GAME_PORT(void);
extern int CHECK_ROM_SIGNATURE(void);

int check_game_port(void)
{
    int present = 0;

    if (configuration->control_mode == 2 &&
        (!(Midi.channels & 1) || configuration->midi_type != 0x20)) {
        POLL_GAME_PORT();
        joystick_state.center_y = joystick_state.y;
        joystick_state.center_x = joystick_state.x;
        if (joystick_state.center_x > 1 && joystick_state.center_y > 1)
            present = 1;
        if (CHECK_ROM_SIGNATURE() &&
            (joystick_state.center_x >= 450 || joystick_state.center_y >= 450))
            present = 0;
        if (!present)
            configuration->control_mode = present;
    }
    return present;
}
