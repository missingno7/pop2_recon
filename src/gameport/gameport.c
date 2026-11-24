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
extern unsigned char active_sound_channels;
extern GameportState gameport_state;
extern void POLL_GAME_PORT(void);
extern int CHECK_ROM_SIGNATURE(void);

int check_game_port(void)
{
    int present = 0;

    if (configuration->control_mode == 2 &&
        (!(active_sound_channels & 1) || configuration->midi_type != 0x20)) {
        POLL_GAME_PORT();
        gameport_state.center_y = gameport_state.y;
        gameport_state.center_x = gameport_state.x;
        if (gameport_state.center_x > 1 && gameport_state.center_y > 1)
            present = 1;
        if (CHECK_ROM_SIGNATURE() &&
            (gameport_state.center_x >= 450 || gameport_state.center_y >= 450))
            present = 0;
        if (!present)
            configuration->control_mode = present;
    }
    return present;
}
