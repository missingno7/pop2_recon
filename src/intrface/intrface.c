typedef unsigned int ResourceMap;
typedef const char far *FarName;
typedef int (far pascal *WaitFunction)(void);
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

extern void cdecl free(void *);
extern int cdecl printf(const char *, ...);
extern void cdecl exit(int);
extern char far *cdecl far_find_string(FarName, FarName);
extern int cdecl open(const char *, int, ...);
extern int cdecl close(int);

extern void *interface_buffer;
extern void far *video_state;
extern int music_enabled;
extern int resource_state;
extern int file_state;
extern WaitFunction wait_function;
extern ResourceError resource_error;
extern volatile int timers[];
extern ResourceMap resource_map_list;
extern volatile unsigned char resource_flags;
extern struct LevelState *level_state;
extern const char system_error_message[];

static const char message_format[] = "%s\n";
extern const char resource_error_message[];
extern const char resource_verify_message[];

extern void set_screen_mode(int, int);
extern void restore_screen(void);
extern void release_video(void);
extern void reset_input(void);
extern void reset_music(void);
extern void reset_resources(void);
extern void reset_file_state(void);
extern void restore_system(void);
extern void show_system_error(FarName, int);
extern int read_input(void);
extern int input_active(int);
extern int sound_playing(int);
extern void close_resource_map(ResourceMap);
extern ResourceMap open_resource_map(FarName);
extern struct ResourceMapData far *lock_resource_map(ResourceMap);
extern int get_midi_driver_flag(void);

void reset_interface(void);
ResourceMap find_resource_map(const char *);

void abort_with_message(int status, register FarName message)
{
    free(interface_buffer);
    reset_interface();
    if (message)
        printf(message_format, message);
    exit(status);
}

void reset_interface(void)
{
    if (video_state) {
        set_screen_mode(0, 0);
        restore_screen();
        release_video();
        reset_input();
    }
    if (music_enabled)
        reset_music();
    if (resource_state)
        reset_resources();
    if (file_state)
        reset_file_state();
    restore_system();
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
    show_system_error(system_error_message, 0);
    abort_with_message(1, (FarName)0);
}

void abort_program(void)
{
    reset_interface();
    exit(1);
}

unsigned int poll_user(void)
{
    register unsigned int key = read_input();
    if (key == 0x1000 || key == 0x11) {
        abort_with_message(0, (FarName)0);
        key = 0;
    }
    if (!key) {
        if (input_active(1))
            key = 0xe100;
        else if (input_active(2))
            key = 0xe200;
    }
    return key;
}

#pragma optimize("l", on)
int wait_timer(int timer)
{
    int key;
    do {
        key = check_wait_function();
    } while (timers[timer] && !key);
    return key;
}

void wait_timer_no_poll(int timer)
{
    while (timers[timer])
        ;
}
#pragma optimize("l", off)

void wait_ticks(int timer, int ticks)
{
    timers[timer] = ticks;
    wait_timer(timer);
}

int wait_sound(register int sound)
{
    register int key = 0;
    if (sound_playing(sound)) {
        do {
            if (key)
                break;
            key = check_wait_function();
        } while (sound_playing(sound));
    }
    return key;
}

int wait_key(void)
{
    register int key;
    do {
        key = check_wait_function();
    } while (!key);
    return key;
}

void close_resource_file(const char *name)
{
    ResourceMap map = find_resource_map(name);
    if (map)
        close_resource_map(map);
}

ResourceMap open_verified_resource(const char *name)
{
    ResourceMap map = find_resource_map(name);
    if (!map) {
        map = open_resource_map(name);
        if (!map)
            resource_error(name);
    }
    return map;
}

ResourceMap find_resource_map(const char *name)
{
    ResourceMap map = resource_map_list;
    int found = name == (const char *)0;
    struct ResourceMapData far *data;
    if (!found) {
        while (map) {
            data = lock_resource_map(map);
            if (far_find_string(data->name, name))
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

    if (!(resource_flags & 2) || !(resource_flags & 1)) {
        if (names[0])
            open_verified_resource(names[0]);
    }
    if ((resource_flags & 1) && names[1])
        open_verified_resource(names[1]);
    if (resource_flags & 2) {
        if (level_state->level != 0x20 && level_state->level != 0x29) {
            if (names[2])
                open_verified_resource(names[2]);
        } else {
            if (names[5])
                open_verified_resource(names[5]);
        }
        if (!(resource_flags & 1) && get_midi_driver_flag()) {
            if (names[0])
                close_resource_file(names[0]);
            if (names[3])
                open_verified_resource(names[3]);
            if (names[4])
                open_verified_resource(names[4]);
        }
    }
}

#pragma optimize("l", on)
void close_level_resources(const char **names)
{
    int index;
    if (!sound_playing(0)) {
        for (index = 0; index < 5; ++index) {
            if (names[index])
                close_resource_file(names[index]);
        }
    }
}
#pragma optimize("l", off)

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
