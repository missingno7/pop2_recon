/* INIT.C. Only INIT_SOUND, SAVE_INITIAL_SETTING and WAIT_FOR_READY are verified. */
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
extern unsigned char configuration_flags;
extern int initial_setting;
extern char *memory_class_a, *memory_class_b;
extern char message_port[];
extern char video_option[];
extern char memory_option[];
extern int alternate_pool_available;
extern InputPoll poll_input;
extern int message_input(void);

extern void start_drawing(char far *);
extern void frame_rect(char far *);
extern void inset_rect(Rect far *, char far *, int, int);
extern void draw_string_at(Rect far *, int, int, const char far *);
extern void pause_for_input(void);
extern int cdecl sound_setup(int);
extern void report_memory(int memory_class, unsigned long available);
extern void show_init_message(const char *message);
extern void init_graphics(void);
extern unsigned long choose_memory_pool(int *memory_class);
extern void wait_for_ready(void);
extern int wait_status(void);
extern int check_video_mode(int mode);
extern void set_video_mode(int mode);
extern int find_option(const char far *);
extern unsigned long query_pool(unsigned long pool);
extern char far extended_pool[];
extern char far expanded_pool[];
extern void select_pool(unsigned long pool);
extern int cdecl sprintf(char *, const char *, ...);
extern char near *cdecl strcat(char near *, const char near *);
extern int cdecl printf(const char *, ...);
extern void cdecl exit(int);

#pragma intrinsic(strcat)

extern char memory_format[], memory_fallback[], memory_suffix[];

void report_memory(int memory_class, unsigned long available)
{
    char message[240];
    char *kind;

    if (configuration->sound_report && available <= 0x53020UL) {
        if (available) {
            if (memory_class == 1)
                kind = memory_class_a;
            else
                kind = memory_class_b;
            sprintf(message, memory_format, 0x537f0L - available, kind);
        } else {
            sprintf(message, memory_fallback);
        }
        strcat(message, memory_suffix);
        show_init_message(message);
        configuration->sound_report = 0;
    }
}

void show_init_message(const char *message)
{
    Rect rect;
    InputPoll saved_input;

    start_drawing(message_port);
    frame_rect(message_port);
    inset_rect(&rect, message_port, 5, 8);
    draw_string_at(&rect, 0, 0, message);
    saved_input = poll_input;
    poll_input = message_input;
    pause_for_input();
    start_drawing(message_port);
    poll_input = saved_input;
}

static const struct
{
    char digital[86];
    char midi[73];
} sound_messages = {
    "The digital sound card selected did not initialize.  There will be no digital sounds.",
    "The midi sound card selected did not initialize.  There will be no music."
};

void init_sound(void)
{
    sound_setup(0);
    if (configuration_flags & 4)
        show_init_message(sound_messages.digital);
    if (configuration_flags & 8)
        show_init_message(sound_messages.midi);
}

void save_initial_setting(void)
{
    initial_setting = configuration->video;
}

extern char himem_option[], data_filename[], window_title[], config_filename[];
extern char device_filenames[];
extern char video_error[], file_error[];
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
extern void init_resources(void);
extern void wait_for_ticks(int);
extern void enable_graphics(unsigned char);
extern int open_screen_port(int, const char far *);
extern void init_video_driver(void);
extern int make_window(int, const char far *, int, const char far *);
extern void activate_window(int, int);
extern void setup_palette(void);
extern void setup_regions(void);
extern void finish_window(void);
extern void read_configuration(const char *);
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
    if (!configuration->memory && !find_option(himem_option))
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
    open_game_data(data_filename);
    init_graphics();
    init_resources();
    wait_for_ticks(60);
    report_memory(memory_class, available);
    init_sound();
    if (configuration->sound_report == -1)
        enable_graphics(0);
    current_port = open_screen_port((int)screen_port, screen_geometry);
    init_video_driver();
    main_port = current_port = make_window(-2, screen_geometry, 0, window_title);
    activate_window(main_port, 0);
    *(InputPoll *)((char *)main_port + 0x56) = window_handler;
    setup_palette();
    setup_regions();
    finish_window();
    read_configuration(device_filenames);
    initialize_joystick();
    if (initial_setting != configuration->video) {
        show_init_message(video_error);
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
    if (!check_video_mode(5) && !find_option(video_option)) {
        printf(video_error);
        exit(0);
    }
    set_video_mode(5);
}

unsigned long choose_memory_pool(int *memory_class)
{
    unsigned long extended, expanded, available = 0;

    *memory_class = 0;
    if (!find_option(memory_option)) {
        extended = query_pool((unsigned long)extended_pool);
        expanded = query_pool((unsigned long)expanded_pool);
        alternate_pool_available = extended || expanded;
        if (alternate_pool_available) {
            if (expanded < extended) {
                select_pool((unsigned long)extended_pool);
                available = extended;
                *memory_class = 1;
            } else {
                select_pool((unsigned long)expanded_pool);
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
    while (!wait_status())
        ;
}
