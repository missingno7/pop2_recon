struct dialog_rect {
    int left, top, right, bottom;
};

struct dialog_point {
    int x, y;
};

struct dialog_range {
    int first, last;
};

struct dialog_port {
    char private_words[0x20];
    int mode;
    char saved_state[6];
    int foreground;
};

struct dialog_text {
    char header[0x0c];
    struct dialog_rect bounds;
    char reserved[0x12];
    char far *text;
};

extern struct dialog_port *current_port;

extern int far pascal hGetFont(int);
extern void far pascal release_handle(int);
extern char * far cdecl _itoa(int, char *, int);
extern int far pascal SetFont(int);
extern void far pascal OffsetRect(struct dialog_rect far *,
                                  const struct dialog_rect far *, int, int);
extern void far pascal draw_text_in_rect(const struct dialog_rect far *,
                                        struct dialog_range, const char far *);
extern void far pascal MoveTo(int, int);
extern void far pascal draw_text(const char far *);
extern int far pascal find_dialog_string(int);
extern char far * far pascal lock_handle(int);
extern char far * far pascal lock_string_handle(int);
extern int far pascal allocate_handle(unsigned long);
extern void far pascal read_palette(int, int, void far *);
extern void far pascal apply_palette(int, int, const void far *, int);
extern void far pascal save_dialog_port(struct dialog_port *, int,
                                        void far *, void far *, int);
typedef struct palette_handle *PaletteHandle;
extern int far pascal fade_palette(PaletteHandle, int, int);
extern void far pascal set_dialog_port(void far *);

void far pascal DialogDrawStr(const char far *, int, int, const struct dialog_point *,
                              const struct dialog_rect far *, const struct dialog_range *);

void far pascal draw_dialog_text_block(struct dialog_text far *block)
{
    struct dialog_range range;
    int style;

    style = current_port->mode == 12 ? 0 : 1;
    range.last = -1;
    range.first = 0;
    DialogDrawStr(block->text, 10, style, 0, &block->bounds, &range);
    release_handle(hGetFont(10));
}

void far pascal DialogDrawNumber(int number, const struct dialog_point *point,
                                 const struct dialog_rect far *rect,
                                 const struct dialog_range *range)
{
    char text[10];

    _itoa(number, text, 10);
    DialogDrawStr(text, 10, 0, point, rect, range);
    release_handle(hGetFont(10));
}

void far pascal DialogDrawStr(const char far *text, int font, int style,
                              const struct dialog_point *point,
                              const struct dialog_rect far *rect,
                              const struct dialog_range *bounds)
{
    struct dialog_rect shadow_rect;
    struct dialog_range range;
    int ink, shadow, first_ink;

    if (style == 0) {
        first_ink = 0;
        ink = 15;
        shadow = 14;
    } else {
        first_ink = 2;
        ink = 4;
        shadow = 13;
    }
    if (bounds != 0)
        range = *bounds;
    else {
        range.last = -1;
        range.first = 0;
    }

    SetFont(font);
    current_port->foreground = first_ink;
    if (rect) {
        OffsetRect(&shadow_rect, rect, 1, 0);
        draw_text_in_rect(&shadow_rect, range, text);
    } else {
        MoveTo(point->y + 1, point->x);
        draw_text(text);
    }

    current_port->foreground = shadow;
    if (rect)
        draw_text_in_rect(rect, range, text);
    else {
        MoveTo(point->y, point->x);
        draw_text(text);
    }

    SetFont(font + 1);
    current_port->foreground = ink;
    if (rect)
        draw_text_in_rect(rect, range, text);
    else {
        MoveTo(point->y, point->x);
        draw_text(text);
    }
}

void far pascal DialogDrawStrId(int string_id, int font, int style,
                                const struct dialog_point *point,
                                const struct dialog_rect far *rect,
                                const struct dialog_range *range)
{
    int handle;

    handle = find_dialog_string(string_id);
    if (handle) {
        DialogDrawStr(lock_string_handle(handle), font, style, point, rect, range);
        release_handle(handle);
    }
}

extern struct dialog_port dialog_port;
extern char saved_port[];
extern int dialog_port_handle;

int far pascal DialogFadeIn(void)
{
    int handle, result;

    read_palette(0, 32, lock_handle(handle = allocate_handle(96UL)));
    apply_palette(0, 32, 0, 1);
    save_dialog_port(&dialog_port, dialog_port_handle, saved_port, saved_port, 0);
    result = fade_palette((PaletteHandle)handle, 3, 0);
    if (result)
        apply_palette(0, 32, lock_handle(handle), 1);
    release_handle(handle);
    return result;
}

int far pascal DialogFadeOut(void)
{
    int result;

    result = fade_palette(0, 3, 0);
    set_dialog_port(saved_port);
    return result;
}
extern struct dialog_port game_port;
extern int gameplay_dialog_active;
extern int game_is_suspended;
extern signed char room_effect;
extern int active_menu, menu_disabled, dialog_blocked;
extern signed char pending_key;
extern int control_mode;
extern int (far pascal *dialog_wait)(void);

extern void far pascal free_port(int);
extern int far pascal create_port(struct dialog_port far *);
extern void far pascal restore_game_picture(void);
extern void far pascal restore_game_palette(void);
extern void far pascal suspend_input(void);
extern void far pascal draw_room(int);
extern void far pascal set_room_effect(signed char);
extern int far pascal game_wait(void);
extern int far pascal read_key(void);
extern void far pascal wait_for_key(void);
extern void far pascal acknowledge_key(int, int, int);
typedef struct resource_handle *DialogAsset;
extern int far pascal create_frame_asset(DialogAsset, int, int);
extern void far pascal use_frame_palette(DialogAsset, int, int, int, int);
extern void far pascal fill_frame_buffer(void far *, int, int, int);
extern void far pascal set_frame_dimensions(void far *, int, int);
extern void far pascal inset_rect(struct dialog_rect far *, void far *, int, int);
extern void far pascal frame_rect(struct dialog_rect far *, int);
extern void far pascal dispose_dialog_asset(DialogAsset);
extern void far pascal stop_dialog_effect(int);
extern void far pascal prepare_game_for_dialog(void);
extern void far pascal prepare_dialog_picture(void);
extern DialogAsset far pascal open_dialog_asset(int, int, int);

void far pascal DialogResetToGamePlay(int restart_input)
{
    if (dialog_port_handle)
        free_port(dialog_port_handle);
    dialog_port_handle = create_port(&game_port);
    current_port = &dialog_port;
    SetFont(0);
    current_port->mode = 0;
    current_port->foreground = 15;
    set_dialog_port(saved_port);
    restore_game_picture();
    release_handle(hGetFont(11));
    release_handle(hGetFont(12));
    release_handle(hGetFont(13));
    if (restart_input && !game_is_suspended) {
        suspend_input();
        draw_room(0);
        gameplay_dialog_active = 1;
    }
    restore_game_palette();
    set_room_effect(room_effect);
    dialog_wait = game_wait;
}

int far pascal DialogWaitFunc(void)
{
    int key;

    key = read_key();
    if (key) {
        if (key == 0x1000 || key == 17)
            goto acknowledge;
        if (key == 27 || key == 32)
            goto done;
        key = 0;
        goto empty;
acknowledge:
        acknowledge_key(0, 0, 0);
empty:
        return key;
    }
    if (control_mode == 2) {
        wait_for_key();
        key = pending_key;
    }
done:
    return key;
}

DialogAsset far pascal load_frame_asset(void);

struct frame_bitmap {
    int width, stride, height;
    char pixels[1];
};

void far pascal DrawDialogFrame(DialogAsset asset)
{
    struct dialog_rect border;
    int temporary;
    int handle;
    struct frame_bitmap far *bitmap;

    temporary = asset == 0;
    if (temporary)
        asset = load_frame_asset();
    use_frame_palette(asset, 1, 0, 0, 0);
    use_frame_palette(asset, 2, 0, 192, 0);
    bitmap = (struct frame_bitmap far *)lock_handle(handle = create_frame_asset(asset, 3, 1));
    fill_frame_buffer(bitmap, 0, 8, 0);
    set_frame_dimensions(bitmap->pixels, bitmap->width, bitmap->height);
    fill_frame_buffer(bitmap, 312, 8, 0);
    release_handle(handle);
    inset_rect(&border, saved_port, 9, 9);
    frame_rect(&border, 12);
    if (temporary)
        dispose_dialog_asset(asset);
}

void far pascal InitDialog(void)
{
    stop_dialog_effect(0);
    prepare_game_for_dialog();
    dialog_wait = DialogWaitFunc;
    current_port = &dialog_port;
    prepare_dialog_picture();
    SetFont(11);
    current_port->mode = 14;
    free_port(dialog_port_handle);
    dialog_port_handle = create_port((struct dialog_port far *)saved_port);
    current_port = (struct dialog_port *)dialog_port_handle;
    SetFont(11);
    set_dialog_port(saved_port);
}

int far pascal IsValidTimeForDialog(void)
{
    int result = 1;

    if ((!active_menu && !menu_disabled) || dialog_blocked)
        result = 0;
    return result;
}

DialogAsset far pascal load_frame_asset(void)
{
    return open_dialog_asset(8000, 1, 1);
}
struct save_slot_table {
    int count;
    char initials[10][25];
};

extern struct dialog_rect save_heading, save_description;
extern signed char saved_level, level_to_start, main_theme, sound_effect;
extern signed char kid_alive, kid_lives;
extern int requested_level, resume_selected, dialog_timer, dialog_timer_copy;
extern void far pascal dispose_buffer(void far *);
extern void far pascal prepare_main_menu(void);
extern int far pascal lock_resource(int);
extern const char save_success_text[], save_failure_text[];
extern void far pascal play_menu_message(const char *);
extern void far pascal request_level_change(int);
extern unsigned far cdecl _fstrlen(const char far *);
extern char far * far cdecl _fstrcpy(char far *, const char far *);
extern void far pascal fill_rect(const struct dialog_rect far *);
extern void far pascal draw_bevel_rect(struct dialog_rect far *, const struct dialog_rect far *, int, int);
extern void far pascal menu_sound(int);

int far pascal load_game_dialog(void);
int far pascal save_game_dialog(void);
int far pascal main_menu_choice(void);
void far pascal draw_save_slots(struct save_slot_table far *, int);
int far pascal select_save_slot(struct save_slot_table far *, int);
struct save_slot_table far * far pascal read_save_slot_table(void);
int far pascal ResumeGame(struct save_slot_table far *, int);
int far pascal SaveGame(struct save_slot_table far *, int);
void far pascal post_save_message(int);
void far pascal show_dialog_notice(int);
void far pascal draw_main_menu(void);
void far pascal SaveOptionsToDisk(void);

int far pascal load_game_dialog(void)
{
    struct save_slot_table far *slots;
    int selection = 0;
    int loaded;

    InitDialog();
    slots = read_save_slot_table();
    if (slots) {
        DrawDialogFrame(0);
        draw_save_slots(slots, 0);
        current_port = &dialog_port;
        DialogFadeIn();
        free_port(dialog_port_handle);
        dialog_port_handle = selection;
        selection = select_save_slot(slots, 0);
        if (selection != -1) {
            loaded = ResumeGame(slots, selection);
            if (loaded) {
                level_to_start = saved_level;
                requested_level = saved_level;
                resume_selected = 1;
            }
            selection = loaded;
        } else {
            selection = -1;
        }
        dispose_buffer(slots);
        DialogFadeOut();
    }
    DialogResetToGamePlay(selection == -1 ? 1 : 0);
    if (selection > 0)
        show_dialog_notice(4);
    if (selection != -1 && dialog_port_handle) {
        free_port(dialog_port_handle);
        dialog_port_handle = 0;
    }
    return selection;
}

int far pascal save_game_dialog(void)
{
    struct save_slot_table far *slots;
    int selection = 0;

    InitDialog();
    DrawDialogFrame(0);
    slots = read_save_slot_table();
    if (slots) {
        draw_save_slots(slots, 1);
        current_port = &dialog_port;
        DialogFadeIn();
        free_port(dialog_port_handle);
        dialog_port_handle = selection;
        selection = select_save_slot(slots, 1);
        if (selection != -1)
            selection = SaveGame(slots, selection);
        else
            selection = -1;
        dispose_buffer(slots);
        DialogFadeOut();
    }
    DialogResetToGamePlay(1);
    post_save_message(selection);
    return selection;
}

struct resource_shape {
    int width, height, attributes;
};

void far pascal main_menu_dialog(void)
{
    int selection;

    set_dialog_port(saved_port);
    prepare_main_menu();
    InitDialog();
    DrawDialogFrame(0);
    ((struct resource_shape far *)lock_handle(lock_resource(1000)))->attributes = 0;
    open_dialog_asset(1000, 2, 1);
    draw_main_menu();
    current_port = &dialog_port;
    DialogFadeIn();
    free_port(dialog_port_handle);
    dialog_port_handle = 0;
    selection = main_menu_choice();
    DialogFadeOut();
    release_handle(lock_resource(1000));
    DialogResetToGamePlay(0);
    if (selection == 3)
        save_game_dialog();
    else if (selection == 4)
        load_game_dialog();
    SaveOptionsToDisk();
    set_room_effect(room_effect);
    gameplay_dialog_active = 1;
}

struct dialog_rect far * far pascal compute_save_slot_rect(struct dialog_rect far *rect, int index)
{
    if (index >= 5) {
        rect->top = 163;
        index -= 5;
    } else {
        rect->top = 19;
    }
    rect->bottom = rect->top + 137;
    rect->left = index * 22 + 37;
    rect->right = rect->left + 20;
    return rect;
}

void far pascal draw_save_slot_frame(struct dialog_rect *rect, int selected);

void far pascal draw_save_slot(struct save_slot_table far *slots, int index, int selected)
{
    struct { const char far *text; } initials;
    struct dialog_rect rect;
    int style;

    compute_save_slot_rect(&rect, index);
    draw_save_slot_frame(&rect, selected);
    style = selected != 0;
    initials.text = slots->initials[index];
    if (_fstrlen(initials.text)) {
        draw_bevel_rect(&rect, &rect, 2, 2);
        DialogDrawStr(initials.text, 10, style, 0, &rect, 0);
    }
}

void far pascal draw_save_slot_frame(struct dialog_rect *rect, int selected)
{
    int fill, edge;

    if (selected) {
        fill = 3;
        edge = 14;
    } else {
        fill = 6;
        edge = 11;
    }
    frame_rect(rect, edge);
    current_port->foreground = fill;
    fill_rect(rect);
}

void far pascal draw_save_slots(struct save_slot_table far *slots, int saving)
{
    int title, action, i;
    int light_font, dark_font;
    struct dialog_range range;

    light_font = hGetFont(12);
    dark_font = hGetFont(13);
    range.first = range.last = 0;
    if (!saving) {
        title = 8004;
        action = 8013;
    } else {
        title = 8000;
        action = 8003;
    }
    DialogDrawStrId(title, 12, 0, 0, &save_heading, &range);
    release_handle(light_font);
    release_handle(dark_font);
    light_font = hGetFont(10);
    hGetFont(11);
    OffsetRect(&save_description, &save_description, 0, -18);
    DialogDrawStrId(8001, 10, 0, 0, &save_description, &range);
    OffsetRect(&save_description, &save_description, 0, 18);
    range.last = -1;
    DialogDrawStrId(action, 10, 0, 0, &save_description, &range);
    range.last = 1;
    DialogDrawStrId(8002, 10, 0, 0, &save_description, &range);
    i = 0;
    do {
        draw_save_slot(slots, i, 0);
    } while (++i < 10);
    release_handle(light_font);
}

void far pascal post_save_message(int selection)
{
    if (selection != -1) {
        if (selection)
            play_menu_message(save_success_text);
        else {
            play_menu_message(save_failure_text);
            request_level_change(-1);
        }
        dialog_timer = dialog_timer_copy = 24;
    }
}

struct slot_editor {
    char state[0x2e];
    void (far pascal *draw)(struct dialog_text far *);
};

extern void far pascal close_slot_editor(struct slot_editor far *);
extern struct slot_editor far * far pascal create_slot_editor(struct dialog_rect far *,
                                          struct dialog_rect far *, char far *, int);
extern void far pascal start_slot_editor(struct slot_editor far *);
extern void far pascal refresh_slot_editor(struct slot_editor far *);
extern void far pascal edit_slot_key(struct slot_editor far *, int);

int far pascal select_save_slot(struct save_slot_table far *slots, int saving)
{
    char initials[26];
    struct slot_editor far *editor;
    struct dialog_rect rect;
    int result, done, previous, slot, key;

    result = previous = -1;
    done = slot = 0;
    editor = 0;

    do {
        if (previous != slot) {
            if (previous != -1) {
                draw_save_slot(slots, previous, 0);
                if (editor)
                    close_slot_editor(editor);
            }
            previous = slot;
            draw_save_slot(slots, slot, 1);
            if (saving == 1) {
                compute_save_slot_rect(&rect, slot);
                draw_bevel_rect(&rect, &rect, 2, 2);
                _fstrcpy(initials, slots->initials[slot]);
                editor = create_slot_editor(&rect, &rect, initials, 24);
                editor->draw = draw_dialog_text_block;
                start_slot_editor(editor);
            }
        }
        if (editor)
            refresh_slot_editor(editor);
        key = read_key();
        switch (key) {
        case 9:
            previous = slot;
            goto advance_slot;
        case 13:
            if ((saving == 0 && !_fstrlen(slots->initials[slot])) ||
                (saving == 1 && !_fstrlen(initials))) {
                menu_sound(-2);
                break;
            }
            if (editor)
                _fstrcpy(slots->initials[slot], initials);
            result = slot;
            /* Enter and Escape both leave the slot loop. */
        case 27:
            done = 1;
            break;
        case 17:
        case 0x1000:
            acknowledge_key(0, 0, 0);
            break;
        default:
            if (editor) {
                if (key != '>' && key != '<')
                    edit_slot_key(editor, key);
            } else {
                switch (key) {
                case 0x4700: slot = 0; break;
                case 0x4800: slot = slot ? slot - 1 : 9; break;
                case 0x4b00:
                case 0x4d00:
                    slot += slot >= 5 ? -5 : 5;
                    break;
                case 0x4f00: slot = 9; break;
                case 0x5000:
                advance_slot:
                    slot = slot == 9 ? 0 : slot + 1;
                    break;
                }
            }
            break;
        }
    } while (!done);
    if (editor)
        close_slot_editor(editor);
    return result;
}

extern const char save_path[];
extern unsigned far cdecl _dos_open(const char *, unsigned, int *);
extern long far cdecl filelength(int);
extern void far * far pascal allocate_buffer(long);
extern unsigned far cdecl _dos_read(int, void far *, unsigned, unsigned *);
extern unsigned far cdecl _dos_close(int);
extern void far * far cdecl _fmemset(void far *, int, unsigned);

struct save_slot_table far * far pascal read_save_slot_table(void)
{
    unsigned read_count;
    struct save_slot_table far *slots;
    int count;
    int file;

    if (!_dos_open(save_path, 2, &file)) {
        count = filelength(file) >= sizeof(*slots) ?
                sizeof(*slots) : (int)filelength(file);
        slots = allocate_buffer(count);
        if (_dos_read(file, slots, count, &read_count) ||
            read_count != count || slots->count > 10) {
            dispose_buffer(slots);
            slots = 0;
        }
        _dos_close(file);
    } else {
        _fmemset(slots = allocate_buffer(sizeof(*slots)), 0, sizeof(*slots));
    }
    return slots;
}

int far pascal IsValidTimeForSaveGame(void)
{
    int result = 1;

    if (saved_level > 14 || kid_alive >= 0 || kid_lives <= 0)
        result = 0;
    return result;
}
#pragma pack(1)
struct rebirth_record {
    int point_id;
    int reserved2;
    int menu, disabled;
    signed char level;
    signed char health;
    signed char direction;
    char saved_flags;
    signed char facing;
    char reserved13[2];
    int level_flags;
    char room_state;
    char reserved18[5];
    char rooms[960];
    char room_links[3840];
    char room_objects[3712];
    char level_state[1088];
};

struct rebirth_point {
    signed char room;
    signed char cell;
};

extern struct rebirth_record far *rebirth_data;
extern const struct rebirth_point rebirth_points[2];

struct box { int left, top, right, bottom; };
struct character {
    char frame, direction;
    int x, y;
    char action;
    unsigned sequence;
    signed char column, row, floor, strength, fall, room, type;
    signed char skill, alive, max_hp, hp_delta, pushed;
    int hurt, previous_x, alternate_x;
    struct box bounds;
    char alert;
    int collision;
    char unused26[6];
    struct box previous_bounds;
    int previous_sequence_x;
    char unused36[3];
    signed char opponent;
    char unused3a[6];
};

extern struct character Char;
extern signed char room_to_draw, starting_health, start_facing;
extern signed char kid_max_health, cutscene_flags, current_room_state, live_direction;
extern int saved_game_active, level_flags;
extern char rooms[960], room_links[3840], room_objects[3712], level_state[1088];
extern unsigned far cdecl _dos_creat(const char *, unsigned, int *);
extern unsigned far cdecl _dos_write(int, const void far *, unsigned, unsigned *);
extern long far cdecl lseek(int, long, int);
extern void far * far cdecl _fmemcpy(void far *, const void far *, unsigned);

void far pascal RemoveRebirthPt(void);

int far pascal ResumeGame(struct save_slot_table far *slots, int slot)
{
    long offset;
    int result = 0;
    struct rebirth_record far *saved;
    int file;
    unsigned bytes_read;

    if (slot < 10 && !_dos_open(save_path, 2, &file)) {
        saved = allocate_buffer(sizeof(*saved));
        offset = sizeof(*slots) + slot * (long)sizeof(*saved);
        lseek(file, offset, 0);
        if (!_dos_read(file, saved, sizeof(*saved), &bytes_read) &&
            bytes_read == sizeof(*saved)) {
            if (saved->point_id) {
                if (!rebirth_data)
                    rebirth_data = allocate_buffer(sizeof(*rebirth_data));
                _fmemcpy(rebirth_data, saved, sizeof(*saved));
            } else {
                RemoveRebirthPt();
            }
            active_menu = saved->menu;
            menu_disabled = saved->disabled;
            saved_level = saved->level;
            kid_max_health = kid_lives = starting_health = saved->health;
            cutscene_flags = saved->saved_flags;
            start_facing = saved->facing;
            level_flags = saved->level_flags;
            current_room_state = saved->room_state;
            saved_game_active = 1;
            lseek(file, offset, 0);
            _dos_write(file, saved, sizeof(*saved), &bytes_read);
            result = 1;
        }
        dispose_buffer(saved);
        _dos_close(file);
    }
    return result;
}

int far pascal SaveGame(struct save_slot_table far *slots, int slot)
{
    int result = 0;
    unsigned size;
    int file;
    unsigned transferred;
    struct rebirth_record far *saved;

    if (slot < 10 &&
        (!_dos_open(save_path, 1, &file) || !_dos_creat(save_path, 0, &file))) {
        saved = allocate_buffer(sizeof(*saved));
        if (rebirth_data)
            _fmemcpy(saved, rebirth_data, sizeof(*saved));
        else {
            _fmemset(saved, 0, sizeof(*saved));
            saved->level = saved_level;
            saved->health = starting_health;
            saved->direction = live_direction;
        }
        saved->menu = active_menu;
        saved->disabled = menu_disabled;
        saved->saved_flags = cutscene_flags;
        saved->facing = start_facing;
        saved->level_flags = level_flags;
        saved->room_state = current_room_state;
        size = sizeof(*slots);
        if (!_dos_write(file, slots, size, &transferred) && transferred == size) {
            size = sizeof(*saved);
            lseek(file, slot * (long)size, 1);
            if (!_dos_write(file, saved, size, &transferred) && transferred == size)
                result = 1;
        }
        dispose_buffer(saved);
        _dos_close(file);
    }
    return result;
}

void far pascal prepare_saved_room_objects(void);

void far pascal SaveRebirthPtStatus(int point_id)
{
    if (!rebirth_data)
        rebirth_data = allocate_buffer(sizeof(*rebirth_data));
    rebirth_data->point_id = point_id;
    _fmemcpy(rebirth_data->rooms, rooms, sizeof(rooms));
    _fmemcpy(rebirth_data->room_links, room_links, sizeof(room_links));
    _fmemcpy(rebirth_data->room_objects, room_objects, sizeof(room_objects));
    _fmemcpy(rebirth_data->level_state, level_state, sizeof(level_state));
    rebirth_data->level = saved_level;
    rebirth_data->health = kid_max_health;
    rebirth_data->direction = live_direction;
    rebirth_data->level_flags = level_flags;
    rebirth_data->facing = start_facing;
    prepare_saved_room_objects();
}

void far pascal RemoveRebirthPt(void)
{
    if (rebirth_data) {
        dispose_buffer(rebirth_data);
        rebirth_data = 0;
    }
}

struct room_object {
    unsigned state;
    unsigned linked_tile;
};

struct room_control {
    char reserved[2];
    unsigned char state;
};

extern unsigned char rooms_in_level;
extern struct room_control *far pascal find_room_control(unsigned char, int);

void far pascal prepare_saved_room_objects(void)
{
    unsigned char room;
    signed char tile;
    unsigned base;
    struct room_object far *object;
    struct room_control *control;
    unsigned state, low;

    for (room = 0; room < rooms_in_level; ++room) {
        base = room * 30;
        for (tile = 0; tile < 30; ++tile) {
            object = (struct room_object far *)rebirth_data->room_links + base + tile;
            switch ((unsigned char)rebirth_data->rooms[base + tile]) {
            case 4:
                state = object->state;
                low = state & 0xff;
                control = find_room_control(room + 1, tile);
                if (control && control->state == 3)
                    low = 200;
                else if (control)
                    low = 0;
                object->state = (state & 0xff00) | low;
                break;
            case 17:
                state = object->state;
                if (state & 0xff)
                    object->state = (state & 0xff00) | 42;
                break;
            }
        }
    }
}

void far pascal SetRebirthPtLevelStatus(void)
{
    if (rebirth_data) {
        _fmemcpy(rooms, rebirth_data->rooms, sizeof(rooms));
        _fmemcpy(room_links, rebirth_data->room_links, sizeof(room_links));
        _fmemcpy(room_objects, rebirth_data->room_objects, sizeof(room_objects));
        _fmemcpy(level_state, rebirth_data->level_state, sizeof(level_state));
        level_flags = rebirth_data->level_flags;
    }
}

int far pascal SetRebirthPtKidStatus(void)
{
    const struct rebirth_point *point;
    int result;

    if (rebirth_data) {
        /* Saved point IDs are one-based. */
        point = &rebirth_points[rebirth_data->point_id - 1];
        room_to_draw = Char.room = point->room;
        Char.column = point->cell % 10;
        Char.row = point->cell / 10;
        starting_health = rebirth_data->health;
        Char.direction = rebirth_data->direction;
        start_facing = rebirth_data->facing;
        result = 2;
    } else {
        result = -1;
    }
    return result;
}
#pragma pack()

/* The intervening copy-protection and hall-of-fame routines are still open. */

int * far pascal ComputeSymbolPt(int *p, int index)
{
    p[1] = (index % 5) * 42 + 62;
    p[0] = (index / 5) * 42 + 69;
    return p;
}
int far * far pascal ComputeHallPlayerRect(int far *p, int index)
{
    p[1] = 25;
    p[3] = 155;
    p[0] = index * 24 + 59;
    p[2] = p[0] + 20;
    return p;
}
int * far pascal MAKE_INDEX_RECT(int *p, int index)
{
    if (index >= 3) {
        p[1] = 198;
        index -= 3;
    } else {
        p[1] = 38;
    }
    p[3] = (p[1] + 140 > 310) ? 310 : p[1] + 140;
    p[0] = index * 22 + 40;
    p[2] = p[0] + 20;
    return p;
}

void far pascal draw_menu_label(int, int);
void far pascal draw_menu_icon(int, int);
int far pascal change_menu_option(int);

int far pascal main_menu_choice(void)
{
    int previous = -1;
    int option = 0;
    int done = 0;

    do {
        if (previous != option) {
            if (previous != -1 && previous != option)
                draw_menu_icon(previous, 0);
            previous = option;
            draw_menu_icon(option, 1);
        }
        switch (read_key()) {
        case 9:
            previous = option;
            /* Tab and the Down key both advance. */
        case 0x5000:
            if (option == 5)
                option = 0;
            else
                ++option;
            break;
        case 0x4700:
            option = 0;
            break;
        case 13:
            if (option == 3 || option == 4) {
                if (option == 3 && !IsValidTimeForSaveGame())
                    menu_sound(-2);
                else
                    done = 1;
            } else if (change_menu_option(option)) {
                draw_menu_label(option, 1);
            } else {
                menu_sound(-2);
            }
            break;
        case 17:
        case 0x1000:
            acknowledge_key(0, 0, 0);
            break;
        case 27:
            option = 0;
            done = 1;
            break;
        case 0x4800:
            if (option) {
                --option;
                break;
            }
        case 0x4f00:
            option = 5;
            break;
        case 0x4b00:
        case 0x4d00:
            if (option >= 3)
                option -= 3;
            else
                option += 3;
            break;
        }
    } while (!done);
    return option;
}

void far pascal draw_main_menu(void)
{
    int i;
    struct dialog_range range;

    current_port->mode = 12;
    range.last = range.first = 0;
    DialogDrawStrId(8016, 12, 0, 0, &save_heading, &range);
    OffsetRect(&save_description, &save_description, 0, -18);
    DialogDrawStrId(8001, 10, 0, 0, &save_description, &range);
    range.last = -1;
    OffsetRect(&save_description, &save_description, 0, 18);
    DialogDrawStrId(8014, 10, 0, 0, &save_description, &range);
    range.last = 1;
    DialogDrawStrId(8015, 10, 0, 0, &save_description, &range);
    for (i = 0; i < 6; ++i)
        draw_menu_label(i, 0);
    draw_menu_icon(0, 1);
}

extern unsigned char sound_volume, menu_music;
extern const char volume_on[], volume_off[], music_on[], music_off[];
extern const char keyboard_option[], controller_option[];
extern const char save_option[10], resume_option[12], continue_option[10];

void far pascal draw_menu_label(int index, int selected)
{
    char label_buffer[40];
    struct dialog_rect rect;
    struct dialog_range range;
    const char *label;

    if (MAKE_INDEX_RECT((int *)&rect, index)) {
        label = label_buffer;
        range.last = -1;
        range.first = 0;
        switch (index) {
        case 0: label = sound_volume ? volume_on : volume_off; break;
        case 1: label = menu_music ? music_on : music_off; break;
        case 2: label = control_mode == 0 ? keyboard_option : controller_option; break;
        case 3: _fmemcpy(label_buffer, save_option, sizeof(save_option)); label = label_buffer; break;
        case 4: _fmemcpy(label_buffer, resume_option, sizeof(resume_option)); label = label_buffer; break;
        case 5: _fmemcpy(label_buffer, continue_option, sizeof(continue_option)); label = label_buffer; break;
        }
        if (selected)
            frame_rect(&rect, 12);
        DialogDrawStr(label, 10, 0, 0, &rect, &range);
    }
}

extern int far pascal acquire_frame_shape(int, int);
extern void far pascal remap_frame_shape(void far *, int, int);
extern void far pascal draw_frame_shape(void far *, int, int, int);

void far pascal draw_menu_icon(int index, int selected)
{
    struct dialog_rect rect;
    struct dialog_point origin;
    struct frame_bitmap far *bitmap;
    int handle;

    if (MAKE_INDEX_RECT((int *)&rect, index)) {
        origin.x = rect.left + 6;
        origin.y = rect.top - 22;
        handle = acquire_frame_shape(lock_resource(1000), 5);
        if (handle) {
            bitmap = (struct frame_bitmap far *)lock_handle(handle);
            if (selected) {
                remap_frame_shape(bitmap->pixels, bitmap->width, bitmap->height);
                draw_frame_shape(bitmap, origin.y, origin.x, 10);
            } else {
                rect.left = origin.x;
                rect.top = origin.y;
                rect.right = rect.left + bitmap->width;
                rect.bottom = rect.top + bitmap->stride;
                frame_rect(&rect, 12);
            }
        }
    }
}

extern const char options_path[];
extern void far pascal set_sound_volume(unsigned char);

struct menu_options {
    int sound, music;
};

void far pascal SaveOptionsToDisk(void)
{
    struct menu_options options;
    unsigned written;
    int file;

    options.sound = sound_volume == 15 ? 1 : 0;
    options.music = menu_music;
    if (!_dos_open(options_path, 1, &file) || !_dos_creat(options_path, 0, &file)) {
        _dos_write(file, &options, sizeof(options), &written);
        _dos_close(file);
    }
}

void far pascal SetOptionsFromDisk(void)
{
    struct menu_options options;
    unsigned transferred;
    int file;
    unsigned size;

    if (!_dos_open(options_path, 0, &file)) {
        size = sizeof(options);
        if (!_dos_read(file, &options, size, &transferred) && transferred == size) {
            if (!options.sound)
                set_sound_volume(0);
            if (!options.music)
                menu_music = 0;
        }
        _dos_close(file);
    }
}
