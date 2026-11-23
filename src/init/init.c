/* INIT.C. DOS platform initialization and startup diagnostics. */
typedef struct
{
    int top, left, bottom, right;
} Rect;

typedef struct
{
    int unknown, video, memory, sound_report;
} Configuration;

typedef int (far pascal *InputPoll)(void);

extern Configuration *configuration;
extern unsigned char active_sound_channels;
extern int initial_setting;
extern int alternate_pool_available;
extern InputPoll poll_input;
extern int message_input(void);

extern void STROKE_PORT_RECT(Rect far *);
extern void frame_rect(Rect far *);
extern void INSETRECT(Rect far *, Rect far *, int, int);
extern void DRAW_NUL_TEXT_IN_RECT(Rect far *, int, int, const char far *);
extern void wait_key(void);
extern int cdecl initialize_sound_drivers(int);
extern void report_memory(int memory_class, unsigned long available);
extern void show_init_message(const char *message);
extern void init_graphics(void);
extern unsigned long choose_memory_pool(int *memory_class);
extern void wait_for_ready(void);
extern int get_sound_startup_flag(void);
extern int SELECT_GRAPHICS_MODE(int mode);
extern void INITIALIZE_GRAPHICS_MODE(int mode);
extern int find_option(const char far *);
typedef unsigned long (far pascal *MemoryDriver)(void);
extern unsigned long extended_pool(void);
extern unsigned long expanded_pool(void);
extern unsigned long query_pool(MemoryDriver driver);
extern void select_pool(MemoryDriver driver);
extern int cdecl sprintf(char *, const char *, ...);
extern char near *cdecl strcat(char near *, const char near *);
extern int cdecl printf(const char *, ...);
extern void cdecl exit(int);

#pragma intrinsic(strcat)

static Rect message_port = { 50, 50, 150, 270 };

static const char *extended_memory_name = "extended";
static const char *expanded_memory_name = "expanded";
static const char *device_filenames[] = {
    "IBMSND.DAT", "DIGISND.DAT", "MIDISND.DAT", 0, "FRAGSND.DAT", "TANDYSND.DAT"
};

void report_memory(int memory_class, unsigned long available)
{
    char message[240];
    const char *kind;

    if (configuration->sound_report && available <= 0x53020UL) {
        if (available) {
            if (memory_class == 1)
                kind = extended_memory_name;
            else
                kind = expanded_memory_name;
            sprintf(message,
                    "Digital sounds will not be played because there is not enough extended or expanded memory available.  "
                    "You need %ld more bytes of %s memory for digital sounds.",
                    0x537f0L - available, kind);
        } else {
            sprintf(message,
                    "Digital sounds will not be played because there is no extended or expanded memory available.");
        }
        strcat(message, "  Press any key to continue.");
        show_init_message(message);
        configuration->sound_report = 0;
    }
}

void show_init_message(const char *message)
{
    Rect rect;
    InputPoll saved_input;

    STROKE_PORT_RECT(&message_port);
    frame_rect(&message_port);
    INSETRECT(&rect, &message_port, 5, 8);
    DRAW_NUL_TEXT_IN_RECT(&rect, 0, 0, message);
    saved_input = poll_input;
    poll_input = message_input;
    wait_key();
    STROKE_PORT_RECT(&message_port);
    poll_input = saved_input;
}

extern const char digital_sound_error[], midi_sound_error[];

void init_sound(void)
{
    initialize_sound_drivers(0);
    if (active_sound_channels & 4)
        show_init_message(digital_sound_error);
    if (active_sound_channels & 8)
        show_init_message(midi_sound_error);
}

void save_initial_setting(void)
{
    initial_setting = configuration->video;
}

extern char config_filename[];
extern char joystick_missing_message[], video_error[], file_error[];
extern InputPoll timer_callback, video_callback, keyboard_callback;
extern int timer_handler(void), video_handler(void), keyboard_handler(void), base_input(void);
extern int draw_handler(void), window_handler(void);
extern char *work_buffer;
extern void far *video_descriptor;
extern int graphics_state, current_port, main_port, midi_ready;
extern char screen_port[], screen_geometry[], joystick_state[], startup_state[];
extern void setup_clock(unsigned long);
extern int open_config(const char *);
extern void close_config(void);
extern void near *cdecl malloc(unsigned int);
extern void start_platform(void);
extern void open_game_data(const char *);
extern void install_interrupt_handlers(void);
extern void wait_for_ticks(int);
extern void enable_graphics(unsigned char);
extern int open_screen_port(int, const char far *);
extern void init_video_driver(void);
extern int make_window(int, const char far *, int, const char far *);
extern void activate_window(int, int);
extern void setup_palette(void);
extern void setup_regions(void);
extern void finish_window(void);
extern void read_configuration(const char **);
extern void initialize_joystick(void);
extern void configure_startup(char *);
extern void prepare_sound(void);
extern int get_sound_resource(int);
extern unsigned long lock_handle(int);
extern int release_resource(unsigned long);
extern void dispose_handle(int);
extern void near *cdecl memset(void near *, int, unsigned int);
#pragma intrinsic(memset)

void init_system(void)
{
    unsigned long available;
    unsigned long sound;
    int memory_class;

    setup_clock(0xffffffUL);
    if (!open_config(config_filename)) {
        close_config();
        printf(file_error, config_filename);
        exit(0);
    }
    save_initial_setting();
    if (!configuration->memory && !find_option("USEHIMEM"))
        available = 0;
    else
        available = choose_memory_pool(&memory_class);

    timer_callback = timer_handler;
    video_callback = video_handler;
    work_buffer = malloc(500);
    *(InputPoll far *)((char far *)video_descriptor + 0x12) = draw_handler;
    keyboard_callback = keyboard_handler;
    poll_input = base_input;
    graphics_state = 0;
    start_platform();
    open_game_data("PRINCE.DAT");
    init_graphics();
    install_interrupt_handlers();
    wait_for_ticks(60);
    report_memory(memory_class, available);
    init_sound();
    if (configuration->sound_report == -1)
        enable_graphics(0);
    current_port = open_screen_port((int)screen_port, screen_geometry);
    init_video_driver();
    current_port = main_port = make_window(-2, screen_geometry, 0, "Prince of Persia II");
    activate_window(main_port, 0);
    *(InputPoll *)((char *)main_port + 0x56) = window_handler;
    setup_palette();
    setup_regions();
    finish_window();
    read_configuration(device_filenames);
    initialize_joystick();
    if (initial_setting != configuration->video) {
        show_init_message(joystick_missing_message);
        initial_setting = 0;
    }
    if (initial_setting != 2)
        memset(joystick_state, 0, 9);
    configure_startup(startup_state);
    prepare_sound();
    wait_for_ready();
    sound = lock_handle(get_sound_resource(10));
    midi_ready = find_option((const char far *)sound) != 0;
    dispose_handle(release_resource(sound));
}

void init_graphics(void)
{
    if (!SELECT_GRAPHICS_MODE(5) && !find_option("NOVIDEODETECT")) {
        printf(video_error);
        exit(0);
    }
    INITIALIZE_GRAPHICS_MODE(5);
}

unsigned long choose_memory_pool(int *memory_class)
{
    unsigned long extended, expanded, available = 0;

    *memory_class = 0;
    if (!find_option("NOHIMEM")) {
        extended = query_pool(extended_pool);
        expanded = query_pool(expanded_pool);
        alternate_pool_available = extended || expanded;
        if (alternate_pool_available) {
            if (expanded < extended) {
                select_pool(extended_pool);
                available = extended;
                *memory_class = 1;
            } else {
                select_pool(expanded_pool);
                available = expanded;
                *memory_class = 1;
            }
        }
    } else {
        alternate_pool_available = 0;
    }
    return available;
}

void wait_for_ready(void)
{
    while (!get_sound_startup_flag())
        ;
}
