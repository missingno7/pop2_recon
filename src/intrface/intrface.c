#include "types.h"
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

Rect ScreenPortRect = { 0, 0, 200, 320 };
unsigned int PollUser0(void);
void StdRsrcVerify(FarName);

WaitFunction WaitFunc = PollUser0;
ResourceError resource_error = StdRsrcVerify;
static const char resource_error_message[] = "StdRsrcError().";
static const char resource_verify_message[] = "StdRsrcVerify().";

extern void ResetSound(const void far *);
extern void reset_sound_drivers(void);
extern void reset_timer_hook(void);
extern void restore_dos_vectors(void);
extern void restore_interrupt_vectors(void);
extern void shutdown_graphics_mode(void);
extern void shutdown_backing_driver(void);
extern void release_heap_segment(void);
extern void show_message(FarName, int);
extern unsigned int read_queued_key(void);
extern int SndOn(int);
extern void CloseMap(ResourceMap);
extern ResourceMap OpenMap(FarName);
extern struct ResourceMapData far *h2p(ResourceMap);

void ResetAll(void);
ResourceMap GetRsrcMapHnd(const char *);

void abort_with_message(int status, FarName message)
{
    if (interface_buffer)
        _nfree(interface_buffer);
    ResetAll();
    if (message)
        printf(message_format, message);
    exit(status);
}

void ResetAll(void)
{
    if (TimeVal) {
        ResetSound(0L);
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

int CheckWaitFunc(void)
{
    int result;
    if (WaitFunc)
        result = WaitFunc();
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
    ResetAll();
    exit(1);
}

unsigned int PollUser0(void)
{
    unsigned int key = read_queued_key();
    if (key == 0x1000 || key == 0x11) {
        abort_with_message(0, (FarName)0);
        key = 0;
    }
    return key;
}

int WaitTimer(int timer)
{
    int key;
    do {
        key = CheckWaitFunc();
    } while (Timer[timer] && !key);
    return key;
}

void WaitTimerNoPoll(int timer)
{
    while (Timer[timer])
        ;
}

void Wait(int timer, int ticks)
{
    Timer[timer] = ticks;
    WaitTimer(timer);
}

int WaitSnd(int sound)
{
    int key = 0;
    if (SndOn(sound)) {
        do {
            if (key)
                break;
            key = CheckWaitFunc();
        } while (SndOn(sound));
    }
    return key;
}

int WaitKey(void)
{
    int key;
    do {
        key = CheckWaitFunc();
    } while (!key);
    return key;
}

void CloseRsrcFile(const char *name)
{
    ResourceMap map = GetRsrcMapHnd(name);
    if (map)
        CloseMap(map);
}

ResourceMap OpenRsrcVerify(const char *name)
{
    ResourceMap map = GetRsrcMapHnd(name);
    if (!map) {
        map = OpenMap(name);
        if (!map)
            resource_error(name);
    }
    return map;
}

ResourceMap GetRsrcMapHnd(const char *name)
{
    ResourceMap map = SHAPE_METADATA_LIST;
    int found = name == (const char *)0;
    struct ResourceMapData far *data;
    if (!found) {
        while (map) {
            data = h2p(map);
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
            OpenRsrcVerify(names[0]);
    }
    if ((Midi.channels & 1) && names[1])
        OpenRsrcVerify(names[1]);
    if (Midi.channels & 2) {
        if (configuration->level != 0x20 && configuration->level != 0x29) {
            if (names[2])
                OpenRsrcVerify(names[2]);
        } else {
            if (names[5])
                OpenRsrcVerify(names[5]);
        }
    }
}

void close_level_resources(const char **names)
{
    int index;
    if (!SndOn(0)) {
        for (index = 0; index < 5; ++index) {
            if (names[index])
                CloseRsrcFile(names[index]);
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
