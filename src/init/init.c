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
extern int backing_driver_active;
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
extern unsigned long ems_driver(void);
extern unsigned long xms_driver(void);
extern unsigned long query_callback_u32(MemoryDriver driver);
extern void initialize_backing_driver(MemoryDriver driver);
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
typedef int (far pascal *CatalogError)(unsigned int *);
typedef void (far pascal *ResourceError)(const char far *);
typedef void (far pascal *WindowAction)(void *, int);
extern CatalogError catalog_callback;
extern InputPoll allocation_handler;
extern ResourceError resource_error_callback;
extern int load_resource_error(unsigned int *);
extern int report_fatal_memory(void), release_discardable_images(void), Input(void);
extern void report_resource_path(const char far *);
extern void ignore_window_action(void *, int);
extern char *interface_buffer;
extern void far *heap_state;
extern int source_port, Graf, main_port, option_flags;
extern char ScreenPort[], screen_bounds[], joystick_state[];

typedef struct {
    unsigned char enabled;
    signed char cue;
    int song, delay, reserved;
} AmbientState;
extern AmbientState ambient_state;
extern void initialize_dos_heap(unsigned long);
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
extern void detect_gameport(void);
extern void init_ambient(AmbientState *);
extern void set_default_palette(void);
extern int hGetTxt4(int);
extern unsigned long resolve_handle_to_far_payload(int);
extern int owner_of_payload(unsigned long);
extern void dispose_handle(int);
extern void near *cdecl memset(void near *, int, unsigned int);
#pragma intrinsic(memset)

void init_system(void)
{
    unsigned long available;
    unsigned long sound;
    int memory_class;

    initialize_dos_heap(0xffffffUL);
    if (!load_sound_settings(config_filename)) {
        release_heap_segment();
        printf(file_error, config_filename);
        exit(0);
    }
    save_initial_setting();
    if (!configuration->memory && !find_option("USEHIMEM"))
        available = 0;
    else
        available = choose_memory_pool(&memory_class);

    catalog_callback = load_resource_error;
    allocation_handler = report_fatal_memory;
    interface_buffer = _nmalloc(500);
    *(InputPoll far *)((char far *)heap_state + 0x12) = release_discardable_images;
    resource_error_callback = report_resource_path;
    poll_input = Input;
    source_port = 0;
    snapshot_null_area();
    open_verified_resource("PRINCE.DAT");
    init_graphics();
    install_interrupt_handlers();
    install_default_timer(60);
    report_memory(memory_class, available);
    init_sound();
    if (configuration->sound_report == -1)
        set_sound_level(0);
    Graf = init_port((int)ScreenPort, screen_bounds);
    set_mode_flag_value();
    Graf = main_port = create_window(-2, screen_bounds, 0, "Prince of Persia II");
    set_window_event_state(main_port, 0);
    *(WindowAction *)((char *)main_port + 0x56) = ignore_window_action;
    update_message_display();
    install_dos_vectors();
    initialize_window_draw_port();
    open_level_resources(device_filenames);
    detect_gameport();
    if (initial_setting != configuration->video) {
        show_init_message(joystick_missing_message);
        initial_setting = 0;
    }
    if (initial_setting != 2)
        memset(joystick_state, 0, 9);
    init_ambient(&ambient_state);
    set_default_palette();
    wait_for_ready();
    sound = resolve_handle_to_far_payload(hGetTxt4(10));
    option_flags = find_option((const char far *)sound) != 0;
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
                *memory_class = 1;
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

void wait_for_ready(void)
{
    while (!get_sound_startup_flag())
        ;
}
