typedef unsigned int ResourceMap;
typedef const char far *FarName;
typedef unsigned int (far pascal *WaitFunction)(void);
typedef void (far pascal *ResourceError)(FarName);

struct ResourceMapData {
    ResourceMap next;
    char reserved[14];
    char name[1];
};

struct LevelState {
    char reserved[8];
    int level;
};

extern void cdecl _nfree(void *);
extern int cdecl printf(const char *, ...);
extern void cdecl exit(int);
extern char far *cdecl _fstrstr(FarName, FarName);
extern int cdecl open(const char *, int, ...);
extern int cdecl close(int);

int dos_vectors_installed;
void *interface_buffer;
extern unsigned long TimeVal;
extern int interrupt_vectors_installed;
extern int graphics_mode;
int backing_driver_active;
extern volatile int Timer[4];
extern ResourceMap SHAPE_METADATA_LIST;
extern struct { unsigned char channels, volume; } Midi;
extern struct LevelState *configuration;
extern const char system_error[];

static const char message_format[] = "%s\n";
struct screen_port_rect { int top, left, bottom, right; };
struct screen_port_rect ScreenPortRect = { 0, 0, 200, 320 };
unsigned int poll_user(void);
void StdRsrcVerify(FarName);

WaitFunction wait_function = poll_user;
ResourceError resource_error = StdRsrcVerify;
static const char resource_error_message[] = "StdRsrcError().";
static const char resource_verify_message[] = "StdRsrcVerify().";

extern void dispatch_resource_pointer(const void far *);
extern void reset_sound_drivers(void);
extern void reset_timer_hook(void);
extern void restore_dos_vectors(void);
extern void restore_interrupt_vectors(void);
extern void shutdown_graphics_mode(void);
extern void shutdown_backing_driver(void);
extern void release_heap_segment(void);
extern void show_message(FarName, int);
extern unsigned int read_queued_key(void);
extern int query_sound(int);
extern void close_resource_catalog(ResourceMap);
extern ResourceMap MOUNT_RESOURCE_CATALOG(FarName);
extern struct ResourceMapData far *RESOLVE_HANDLE_TO_FAR_PAYLOAD(ResourceMap);

void reset_interface(void);
ResourceMap find_resource_map(const char *);

void abort_with_message(int status, FarName message)
{
    if (interface_buffer)
        _nfree(interface_buffer);
    reset_interface();
    if (message)
        printf(message_format, message);
    exit(status);
}

void reset_interface(void)
{
    if (TimeVal) {
        dispatch_resource_pointer(0L);
        reset_sound_drivers();
        reset_timer_hook();
    }
    if (dos_vectors_installed) restore_dos_vectors();
    if (interrupt_vectors_installed) restore_interrupt_vectors();
    if (graphics_mode) shutdown_graphics_mode();
    if (backing_driver_active)
        shutdown_backing_driver();
    release_heap_segment();
}

int check_wait_function(void)
{
    int result;
    if (wait_function)
        result = wait_function();
    else
        result = 0;
    return result;
}

void report_system_error(void)
{
    show_message(system_error, 0);
    abort_with_message(1, (FarName)0);
}

void abort_program(void)
{
    reset_interface();
    exit(1);
}

unsigned int poll_user(void)
{
    unsigned int key = read_queued_key();
    if (key == 0x1000 || key == 0x11) {
        abort_with_message(0, (FarName)0);
        key = 0;
    }
    return key;
}

int wait_timer(int timer)
{
    int key;
    do {
        key = check_wait_function();
    } while (Timer[timer] && !key);
    return key;
}

void wait_timer_no_poll(int timer)
{
    while (Timer[timer])
        ;
}

void wait_ticks(int timer, int ticks)
{
    Timer[timer] = ticks;
    wait_timer(timer);
}

int wait_sound(int sound)
{
    int key = 0;
    if (query_sound(sound)) {
        do {
            if (key)
                break;
            key = check_wait_function();
        } while (query_sound(sound));
    }
    return key;
}

int wait_key(void)
{
    int key;
    do {
        key = check_wait_function();
    } while (!key);
    return key;
}

void close_resource_file(const char *name)
{
    ResourceMap map = find_resource_map(name);
    if (map)
        close_resource_catalog(map);
}

ResourceMap open_verified_resource(const char *name)
{
    ResourceMap map = find_resource_map(name);
    if (!map) {
        map = MOUNT_RESOURCE_CATALOG(name);
        if (!map)
            resource_error(name);
    }
    return map;
}

ResourceMap find_resource_map(const char *name)
{
    ResourceMap map = SHAPE_METADATA_LIST;
    int found = name == (const char *)0;
    struct ResourceMapData far *data;
    if (!found) {
        while (map) {
            data = RESOLVE_HANDLE_TO_FAR_PAYLOAD(map);
            if (_fstrstr(data->name, name))
                found = 1;
            else
                map = data->next;
            if (found)
                break;
        }
    }
    return map;
}

void open_level_resources(const char **names)
{

    if (!(Midi.channels & 2) || !(Midi.channels & 1)) {
        if (names[0])
            open_verified_resource(names[0]);
    }
    if ((Midi.channels & 1) && names[1])
        open_verified_resource(names[1]);
    if (Midi.channels & 2) {
        if (configuration->level != 0x20 && configuration->level != 0x29) {
            if (names[2])
                open_verified_resource(names[2]);
        } else {
            if (names[5])
                open_verified_resource(names[5]);
        }
    }
}

void close_level_resources(const char **names)
{
    int index;
    if (!query_sound(0)) {
        for (index = 0; index < 5; ++index) {
            if (names[index])
                close_resource_file(names[index]);
        }
    }
}

int StdRsrcError(FarName name)
{
    abort_with_message(1, resource_error_message);
    return 1;
}

void StdRsrcVerify(FarName name)
{
    abort_with_message(1, resource_verify_message);
}

int file_exists(const char *name)
{
    int file = open(name, 0, 0);
    close(file);
    return file != -1;
}
