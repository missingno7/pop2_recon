#include "types.h"
#include "system.h"
#include "snd.h"
#include "gameport.h"


int check_game_port(void)
{
    int present = 0;

    if (configuration->control_mode == 2 &&
        (!(Midi.channels & 1) || configuration->midi_mode != 0x20)) {
        poll_game_port();
        joystick_state.center_x = joystick_state.x;
        joystick_state.center_y = joystick_state.y;
        if (joystick_state.center_y > 1 && joystick_state.center_x > 1)
            present = 1;
        if (check_rom_signature() &&
            (joystick_state.center_y >= 450 || joystick_state.center_x >= 450))
            present = 0;
        if (!present)
            configuration->control_mode = present;
    }
    return present;
}
