struct Record;
struct window_item {
    char prefix[4];
    void (far pascal *action)(struct window_item *, int, const void *);
    char effect_header[4];
    char effect_name[26];
    const void *argument;
};
struct rectangle {
    int top, left, bottom, right;
};
struct window {
    char prefix[70];
    int needs_update;
};
typedef int (far pascal *dialog_proc)(int, int, int);

extern void *current_port;
extern int foreground_ink;
extern int background_ink;
extern char dialog_port[];
extern struct window *window_head;
extern int graphics_mode;
extern int window_style;
extern struct rectangle screen_clip;

extern struct window_item *far pascal find_window_item(void *, int);
extern void far pascal start_named_effect(const void far *);
extern int far pascal get_current_dos_directory(char far *);
extern int far pascal set_current_dos_directory(const char far *);
extern void far pascal discard_queued_events(int);
extern unsigned far pascal allocate_tagged_handle(unsigned long, int);
extern struct Record *far pascal create_window_from_resource(int);
extern unsigned far cdecl far_string_length(const char far *);
extern void *far cdecl near_heap_alloc(unsigned);
extern char far *far cdecl far_string_copy(char far *, const char far *);
extern int far pascal wait_for_window_result(void (far pascal *)(void));
extern void far cdecl near_heap_free(void *);
extern void far pascal dispose_window(struct Record *);
extern void far pascal dispose_bitmap_port(void *);
extern unsigned far pascal save_handle_flags(unsigned);
extern struct rectangle far *far pascal get_handle_data(unsigned);
extern struct rectangle far *far pascal InsetRect(struct rectangle far *,
                                                  const struct rectangle far *, int, int);
extern void *far pascal allocate_clipped_port(const struct rectangle far *);
extern void far pascal restore_handle_flags(unsigned, unsigned);
/* These two DOS helpers leave and then consume the same saved clip state. */
extern void far pascal push_port_clip(struct rectangle far *);
extern void far pascal pop_port_clip(void);

int far pascal show_resource_dialog(int, int, const char far *, int);
void far pascal restore_dialog_port(void *);
void *far pascal create_dialog_port(int);
void far pascal set_window_item_text(void *, int, const void *);
void far pascal SET_NEAR_RECORD_WORD20(struct Record *, unsigned int);

int far pascal confirm_action(const char far *message)
{
    if (show_resource_dialog(-20, 3, message, 1) == 1)
        return 1;
    return 0;
}

int far pascal show_message(const char far *message, int replace_port)
{
    return show_resource_dialog(-30, 2, message, replace_port);
}

int far pascal confirm_quit(const char far *message)
{
    if (show_resource_dialog(-40, 3, message, 1) == 1)
        return 1;
    return 0;
}

void far pascal run_window_item_effect(void *port, int id)
{
    void *saved_port = current_port;
    struct window_item *item;

    current_port = port;
    item = find_window_item(port, id);
    start_named_effect(item->effect_name);
    item->action(item, 2, item->argument);
    current_port = saved_port;
}

int far pascal run_dialog_preserving_directory(dialog_proc show, int first, int second, int third)
{
    char directory[66];
    int result;

    get_current_dos_directory(directory);
    foreground_ink = 0;
    background_ink = 15;
    result = show(first, second, third);
    set_current_dos_directory(directory);
    foreground_ink = 15;
    background_ink = 0;
    discard_queued_events(12);
    return result;
}

unsigned far pascal allocate_gold_handle(int id)
{
    return allocate_tagged_handle(0x444c4f47UL, id); /* GOLD */
}

int far pascal show_resource_dialog(int resource, int item, const char far *message, int replace_port)
{
    void *saved_port = current_port;
    struct Record *window;
    char *text;
    int result;
    void *port;

    if (replace_port)
        port = create_dialog_port(resource);
    window = create_window_from_resource(resource);
    far_string_copy(text = near_heap_alloc(far_string_length(message) + 1), message);
    set_window_item_text(window, item, text);
    SET_NEAR_RECORD_WORD20(window, 15);
    result = wait_for_window_result(0);
    near_heap_free(text);
    dispose_window(window);
    if (replace_port)
        restore_dialog_port(port);
    current_port = saved_port;
    return result;
}

void far pascal restore_dialog_port(void *port)
{
    void *saved_port = current_port;

    current_port = dialog_port;
    dispose_bitmap_port(port);
    current_port = saved_port;
    window_head->needs_update = 0;
}

void *far pascal create_dialog_port(int resource)
{
    void *saved_port = current_port;
    unsigned handle;
    unsigned saved_flags;
    void *port;
    struct rectangle bounds;

    current_port = dialog_port;
    handle = allocate_tagged_handle(0x444c4f47UL, resource);
    saved_flags = save_handle_flags(handle);
    InsetRect(&bounds, get_handle_data(handle), -3, -2);
    port = allocate_clipped_port(&bounds);
    restore_handle_flags(handle, saved_flags);
    current_port = saved_port;
    return port;
}

void far pascal set_mode_flag_value(void)
{
    if (graphics_mode == 2) {
        window_style = 7;
        return;
    }
    window_style = 0;
}

void far pascal set_window_item_text(void *port, int id, const void *text)
{
    void *saved_port;
    struct window_item *item;

    saved_port = current_port;
    current_port = port;
    item = find_window_item(port, id);
    push_port_clip(&screen_clip);
    item->action(item, 9, text);
    pop_port_clip();
    current_port = saved_port;
}

struct Record {
    unsigned char prefix[32];
    unsigned int word20;
};

void far pascal SET_NEAR_RECORD_WORD20(struct Record *record,
                                      unsigned int value)
{
    record->word20 = value;
}

