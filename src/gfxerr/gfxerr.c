#include <string.h>
#include <malloc.h>
#include "types.h"
#include "resc.h"
#include "system.h"
#include "graf.h"
#include "level.h"
#include "heap.h"
#include "gfxerr.h"
#include "init.h"
#include "util.h"
#include "window.h"


int confirm_action(const char far *message)
{
    if (show_resource_dialog(-20, 3, message, 1) == 1)
        return 1;
    return 0;
}

int show_message(const char far *message, int replace_port)
{
    return show_resource_dialog(-30, 2, message, replace_port);
}

int confirm_quit(const char far *message)
{
    if (show_resource_dialog(-40, 3, message, 1) == 1)
        return 1;
    return 0;
}

void run_window_item_effect(void *port, int id)
{
    void *saved_port = Graf;
    WINDOWITEM *item;

    Graf = port;
    item = item_by_index(port, id);
    EraseRect(&item->bounds);
    item->action(item, 2, item->argument);
    Graf = saved_port;
}

int run_dialog_preserving_directory(int (*show)(int, int, int), int first, int second, int third)
{
    char directory[66];
    int result;

    get_current_dos_directory(directory);
    default_port.foreground = 0;
    default_port.background = 15;
    result = show(first, second, third);
    set_current_dos_directory(directory);
    default_port.foreground = 15;
    default_port.background = 0;
    discard_queued_events(12);
    return result;
}

Handle allocate_gold_handle(int id)
{
    return hGetRsrc(0x444c4f47UL, id); /* GOLD */
}

int show_resource_dialog(int resource, int item, const char far *message, int replace_port)
{
    void *saved_port = Graf;
    PORT *window;
    char *text;
    int result;
    void *port;

    if (replace_port)
        port = create_dialog_port(resource);
    window = create_window_from_resource(resource);
    _fstrcpy(text = _nmalloc(_fstrlen(message) + 1), message);
    set_window_item_text(window, item, text);
    set_near_record_word20(window, 15);
    result = wait_for_window_result(0);
    _nfree(text);
    dispose_window(window);
    if (replace_port)
        restore_dialog_port(port);
    Graf = saved_port;
    return result;
}

void restore_dialog_port(void *port)
{
    void *saved_port = Graf;

    Graf = &ScreenPort;
    RestoreRect(port);
    Graf = saved_port;
    window_head->needs_update = 0;
}

void *create_dialog_port(int resource)
{
    void *saved_port = Graf;
    Handle handle;
    word saved_flags;
    void *port;
    Rect bounds;

    Graf = &ScreenPort;
    handle = hGetRsrc(0x444c4f47UL, resource);
    saved_flags = hGetState(handle);
    InsetRect(&bounds, hLock(handle), -3, -2);
    port = SaveRect(&bounds);
    hSetState(handle, saved_flags);
    Graf = saved_port;
    return port;
}

void set_mode_flag_value(void)
{
    if (graphics_mode == 2) {
        draw_record_range = 7;
        return;
    }
    draw_record_range = 0;
}

void set_window_item_text(void *port, int id, const void *text)
{
    void *saved_port;
    WINDOWITEM *item;

    saved_port = Graf;
    Graf = port;
    item = item_by_index(port, id);
    ClipRect(&NullRect);
    item->action(item, 9, text);
    ClipRestore();
    Graf = saved_port;
}


void set_near_record_word20(PORT *record,
                                      word value)
{
    record->background = value;
}

word read_queued_key(void)
{
    EVENT event;
    word key;

    discard_queued_events(-9);
    if (read_queued_event(&event, 8))
        key = event.key;
    else
        key = 0;
    return key;
}
