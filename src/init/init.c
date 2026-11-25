/* INIT.C. DOS platform initialization and startup diagnostics. */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include "types.h"
#include "resc.h"
#include "system.h"
#include "snd.h"
#include "graf.h"
#include "shape.h"
#include "level.h"
#include "heap.h"
#include "char.h"
#include "game.h"
#include "draw.h"
#include "diag.h"
#include "english.h"
#include "fade.h"
#include "gameport.h"
#include "gfxerr.h"
#include "init.h"
#include "intrface.h"
#include "load.h"
#include "misc.h"
#include "prince.h"
#include "sound.h"
#include "util.h"
#include "window.h"


typedef unsigned long (*MemoryDriver)(void);


static Rect message_port = { 50, 50, 150, 270 };

static const char *extended_memory_name = "extended";
static const char *expanded_memory_name = "expanded";
static const char *device_filenames[] = {
    "IBMSND.DAT", "DIGISND.DAT", "MIDISND.DAT", 0, 0, 0
};

void show_init_message(const char *message)
{
    Rect rect;
    int (*saved_input)(void);

    EraseRect(&message_port);
    FrameRect(&message_port);
    InsetRect(&rect, &message_port, 5, 8);
    StringRect(&rect, 0, 0, message);
    saved_input = WaitFunc;
    WaitFunc = PollUser0;
    WaitKey();
    EraseRect(&message_port);
    WaitFunc = saved_input;
}


void init_sound(void)
{
    initialize_sound_drivers(0);
    if (configuration->digital_mode && !(Midi.channels & 1))
        show_init_message(digital_sound_error);
    if (configuration->midi_mode && !(Midi.channels & 2))
        show_init_message(midi_sound_error);
}

void save_initial_setting(void)
{
    GameState.control_mode = configuration->control_mode;
}


PORT ScreenPort;


unsigned long choose_memory_pool(int *memory_class)
{
    unsigned long extended, expanded, available = 0;

    *memory_class = 0;
    if (!find_option("NOHIMEM")) {
        extended = query_callback_u32(ems_driver);
        expanded = query_callback_u32(xms_driver);
        backing_driver_active = extended || expanded;
        if (backing_driver_active) {
            if (expanded < extended) {
                initialize_backing_driver(ems_driver);
                available = extended;
                *memory_class = 2;
            } else {
                initialize_backing_driver(xms_driver);
                available = expanded;
                *memory_class = 1;
            }
        }
    } else {
        backing_driver_active = 0;
    }
    return available;
}

void InitSystem(void)
{
    unsigned long available;
    unsigned long sound;
    int memory_class;
    char message[100];

    HeapInit(0xffffffUL);
    available = expand_memory_limits();
    if (available < 0x41eb0UL) {
        sprintf(message, "\nThis machine needs to have %ld more bytes of base memory.\n\n",
                0x42298UL - available);
        printf(message);
        exit(1);
    }
    if (!load_sound_settings(config_filename)) {
        release_heap_segment();
        printf(file_error, config_filename);
        exit(0);
    }
    configuration->control_mode = 2;
    save_initial_setting();
    if (find_option("USEHIMEM"))
        choose_memory_pool(&memory_class);

    catalog_callback = load_resource_error;
    allocation_handler = report_fatal_memory;
    interface_buffer = _nmalloc(500);
    heap_state->release_discardable = release_discardable_images;
    resource_error = report_resource_path;
    WaitFunc = Input;
    BPort = 0;
    snapshot_null_area();
    OpenRsrcVerify("PRINCE.DAT");
    init_graphics();
    install_interrupt_handlers();
    install_default_timer(60);
    init_sound();
    if (configuration->digital_mode == -1)
        set_sound_level(0);
    Graf = InitPort(&ScreenPort, &ScreenPortRect);
    set_mode_flag_value();
    Graf = main_port = create_window(-2, &ScreenPortRect, 0, "Prince of Persia II");
    set_window_event_state(main_port, 0);
    ((WINDOW *)main_port)->action = ignore_window_action;
    update_message_display();
    install_dos_vectors();
    dos_vectors_installed = 1;
    initialize_window_draw_port();
    open_level_resources(device_filenames);
    check_game_port();
    if (configuration->control_mode != GameState.control_mode)
        GameState.control_mode = 0;
    if (GameState.control_mode != 2)
        memset(&joystick_state, 0, sizeof(joystick_state));
    InitAmbient(&ambient_state);
    ResetStdPalette();
    wait_for_ready();
    sound = (unsigned long)h2p(hGetTxt4(10));
    frame_diagnostics = find_option((const char far *)sound) != 0;
    hFree(p2h((void far *)sound));
}

void init_graphics(void)
{
    if (!select_graphics_mode(5) && !find_option("NOVIDEODETECT")) {
        printf(video_error);
        exit(0);
    }
    initialize_graphics_mode(5);
}

void wait_for_ready(void)
{
    while (!get_sound_startup_flag())
        ;
}
