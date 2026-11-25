#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <io.h>
#include "types.h"
#include "resc.h"
#include "system.h"
#include "snd.h"
#include "graf.h"
#include "level.h"
#include "heap.h"
#include "english.h"
#include "gfxerr.h"
#include "intrface.h"


struct ResourceMapData {
    Handle next;
    char reserved[14];
    char name[1];
};


int dos_vectors_installed;
void *interface_buffer;


int backing_driver_active;


static const char message_format[] = "%s\n";

Rect ScreenPortRect = { 0, 0, 200, 320 };


int (*WaitFunc)(void) = PollUser0;
void (*resource_error)(const char far *) = StdRsrcVerify;
static const char resource_error_message[] = "StdRsrcError().";
static const char resource_verify_message[] = "StdRsrcVerify().";


void abort_with_message(int status, const char far *message)
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
    abort_with_message(1, 0L);
}

void abort_program(void)
{
    ResetAll();
    exit(1);
}

word PollUser0(void)
{
    word key = read_queued_key();
    if (key == 0x1000 || key == 0x11) {
        abort_with_message(0, 0L);
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

int Wait(int timer, word ticks)
{
    Timer[timer] = ticks;
    WaitTimer(timer);
}

int WaitSnd(int sound)
{
    int key = 0;
    while (SndOn(sound)) {
        if (key)
            break;
        key = CheckWaitFunc();
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
    Handle map = GetRsrcMapHnd(name);
    if (map)
        CloseMap(map);
}

word OpenRsrcVerify(const char *name)
{
    Handle map = GetRsrcMapHnd(name);
    if (!map) {
        map = OpenMap(name);
        if (!map)
            resource_error(name);
    }
    return map;
}

Handle GetRsrcMapHnd(const char *name)
{
    Handle map = shape_metadata_list;
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
        if (configuration->midi_mode != 0x20 && configuration->midi_mode != 0x29) {
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

int StdRsrcError(const char far *name)
{
    abort_with_message(1, resource_error_message);
    return 1;
}

void StdRsrcVerify(const char far *name)
{
    abort_with_message(1, resource_verify_message);
}

int file_exists(const char *name)
{
    int file = open(name, 0, 0);
    close(file);
    return file != -1;
}
