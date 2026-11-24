/* INIT.C. DOS platform initialization and startup diagnostics. */
typedef struct
{
    int top, left, bottom, right;
} Rect;

typedef struct
{
    int unknown, video, memory, sound_report, midi_sound_mode;
} Configuration;

typedef int (far pascal *InputPoll)(void);

extern Configuration *configuration;
extern unsigned char active_sound_channels;
typedef struct {
    int control_mode, fade_active, playing_nis, recording_mode, skip_opening, field_0a;
    int plateau_loaded, throne_song_added, throne_nis_played, escher_room_eight_seen, nis_room_restored;
} GAMESTATE;
extern GAMESTATE GameState;
extern int backing_driver_active;
extern InputPoll wait_function;
extern int poll_user(void);

extern void STROKE_PORT_RECT(Rect far *);
extern void frame_rect(Rect far *);
extern void INSETRECT(Rect far *, Rect far *, int, int);
extern void DRAW_NUL_TEXT_IN_RECT(Rect far *, int, int, const char far *);
extern void wait_key(void);
extern int cdecl initialize_sound_drivers(int wait_for_sound);
extern void show_init_message(const char *message);
extern void init_graphics(void);
extern unsigned long choose_memory_pool(int *memory_class);
extern void wait_for_ready(void);
extern int get_sound_startup_flag(void);
extern int SELECT_GRAPHICS_MODE(int mode);
extern void INITIALIZE_GRAPHICS_MODE(int mode);
extern int find_option(const char far *);
typedef unsigned long (far pascal *MemoryDriver)(void);
extern unsigned long ems_driver(void);
extern unsigned long xms_driver(void);
extern unsigned long query_callback_u32(MemoryDriver driver);
extern void initialize_backing_driver(MemoryDriver driver);
extern int cdecl sprintf(char *, const char *, ...);
extern int cdecl printf(const char *, ...);
extern void cdecl exit(int);

static Rect message_port = { 50, 50, 150, 270 };

static const char *extended_memory_name = "extended";
static const char *expanded_memory_name = "expanded";
static const char *device_filenames[] = {
    "IBMSND.DAT", "DIGISND.DAT", "MIDISND.DAT", 0, 0, 0
};

void show_init_message(const char *message)
{
    Rect rect;
    InputPoll saved_input;

    STROKE_PORT_RECT(&message_port);
    frame_rect(&message_port);
    INSETRECT(&rect, &message_port, 5, 8);
    DRAW_NUL_TEXT_IN_RECT(&rect, 0, 0, message);
    saved_input = wait_function;
    wait_function = poll_user;
    wait_key();
    STROKE_PORT_RECT(&message_port);
    wait_function = saved_input;
}

extern const char digital_sound_error[], midi_sound_error[];

void init_sound(void)
{
    initialize_sound_drivers(0);
    if (configuration->sound_report && !(active_sound_channels & 1))
        show_init_message(digital_sound_error);
    if (configuration->midi_sound_mode && !(active_sound_channels & 2))
        show_init_message(midi_sound_error);
}

void save_initial_setting(void)
{
    GameState.control_mode = configuration->video;
}

extern char config_filename[];
extern char video_error[], file_error[];
typedef int (far pascal *CatalogError)(unsigned int *);
typedef void (far pascal *ResourceError)(const char far *);
typedef void (far pascal *WindowAction)(void *, int);
extern CatalogError catalog_callback;
extern InputPoll allocation_handler;
extern ResourceError resource_error;
extern int load_resource_error(unsigned int *);
extern int report_fatal_memory(void), release_discardable_images(void), Input(void);
extern void report_resource_path(const char far *);
extern void ignore_window_action(void *, int);
extern char *interface_buffer;
extern void far *heap_state;
extern int BPort, Graf, main_port, frame_diagnostics;
extern int window_draw_ready;
extern char ScreenPort[], ScreenPortRect[], joystick_state[];

typedef struct {
    unsigned char enabled;
    signed char cue;
    int song, delay, reserved;
} AmbientState;
extern AmbientState ambient_state;
extern void initialize_dos_heap(unsigned long);
extern unsigned long expand_memory_limits(void);
extern int load_sound_settings(const char *);
extern void release_heap_segment(void);
extern void near *cdecl _nmalloc(unsigned int);
extern void snapshot_null_area(void);
extern void open_verified_resource(const char *);
extern void install_interrupt_handlers(void);
extern void install_default_timer(int);
extern void set_sound_level(unsigned char);
extern int init_port(int, const char far *);
extern void set_mode_flag_value(void);
extern int create_window(int, const char far *, int, const char far *);
extern void set_window_event_state(int, int);
extern void update_message_display(void);
extern void install_dos_vectors(void);
extern void initialize_window_draw_port(void);
extern void open_level_resources(const char **);
extern void check_game_port(void);
extern void init_ambient(AmbientState *);
extern void set_default_palette(void);
extern int hGetTxt4(int);
extern unsigned long resolve_handle_to_far_payload(int);
extern int owner_of_payload(unsigned long);
extern void dispose_handle(int);
extern void near *cdecl memset(void near *, int, unsigned int);
#pragma intrinsic(memset)

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

void init_system(void)
{
    unsigned long available;
    unsigned long sound;
    int memory_class;
    char message[100];

    initialize_dos_heap(0xffffffUL);
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
    configuration->video = 2;
    save_initial_setting();
    if (find_option("USEHIMEM"))
        choose_memory_pool(&memory_class);

    catalog_callback = load_resource_error;
    allocation_handler = report_fatal_memory;
    interface_buffer = _nmalloc(500);
    *(InputPoll far *)((char far *)heap_state + 0x12) = release_discardable_images;
    resource_error = report_resource_path;
    wait_function = Input;
    BPort = 0;
    snapshot_null_area();
    open_verified_resource("PRINCE.DAT");
    init_graphics();
    install_interrupt_handlers();
    install_default_timer(60);
    init_sound();
    if (configuration->sound_report == -1)
        set_sound_level(0);
    Graf = init_port((int)ScreenPort, ScreenPortRect);
    set_mode_flag_value();
    Graf = main_port = create_window(-2, ScreenPortRect, 0, "Prince of Persia II");
    set_window_event_state(main_port, 0);
    *(WindowAction *)((char *)main_port + 0x56) = ignore_window_action;
    update_message_display();
    install_dos_vectors();
    window_draw_ready = 1;
    initialize_window_draw_port();
    open_level_resources(device_filenames);
    check_game_port();
    if (configuration->video != GameState.control_mode)
        GameState.control_mode = 0;
    if (GameState.control_mode != 2)
        memset(joystick_state, 0, 9);
    init_ambient(&ambient_state);
    set_default_palette();
    wait_for_ready();
    sound = resolve_handle_to_far_payload(hGetTxt4(10));
    frame_diagnostics = find_option((const char far *)sound) != 0;
    dispose_handle(owner_of_payload(sound));
}

void init_graphics(void)
{
    if (!SELECT_GRAPHICS_MODE(5) && !find_option("NOVIDEODETECT")) {
        printf(video_error);
        exit(0);
    }
    INITIALIZE_GRAPHICS_MODE(5);
}

void wait_for_ready(void)
{
    while (!get_sound_startup_flag())
        ;
}
