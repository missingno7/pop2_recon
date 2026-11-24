struct dialog_rect {
    int left, top, right, bottom;
};

struct dialog_point {
    int x, y;
};

struct dialog_range {
    int first, last;
};

struct ScreenPort {
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

static struct dialog_rect save_heading = {14, 16, 37, 304};
static struct dialog_rect save_description = {167, 16, 183, 304};

static const struct dialog_point symbol_heading = {28, 47};
static const struct dialog_point symbol_caption = {44, 72};
static struct dialog_rect notice_port[1] = {{32, 73, 50, 266}};
static struct { int done; } notice_state = {0};
static const struct dialog_point hall_name_caption = {53, 25};
static const struct dialog_point hall_score_caption = {53, 260};
static struct dialog_rect hall_box = {38, 20, 57, 300};
static const char save_path[12] = "PRINCE.SAV";
static const char hall_read_path[12] = "PRINCE.HOF";
static const char options_path[12] = "PRINCE.OPT";
static const int credits_first[] = {1000, 1007, 1009, 1011, 1012};
static const int credits_last[] = {1006, 1008, 1010, 1011, 1012};
static const struct dialog_rect credits_blink_box = {70, 17, 85, 307};
struct notice_and_menu_text {
    char notice_failure[25];
    char hall_write_path[11];
    char save_option[10];
    char resume_option[12];
    char continue_option[10];
};
static const struct notice_and_menu_text dialog_texts = {
    "Copy protection failure.", "PRINCE.HOF", "Save Game", "Resume Game", "Quit Game"
};
#define notice_failure dialog_texts.notice_failure
#define hall_write_path dialog_texts.hall_write_path
#define save_option dialog_texts.save_option
#define resume_option dialog_texts.resume_option
#define continue_option dialog_texts.continue_option

extern struct ScreenPort *Graf;

extern int far pascal hGetFont(int);
extern void far pascal DISPOSE_HANDLE(int);
extern char * far cdecl _itoa(int, char *, int);
extern int far pascal SetFont(int);
extern void far pascal OffsetRect(struct dialog_rect far *,
                                  const struct dialog_rect far *, int, int);
extern void far pascal DRAW_NUL_TEXT_IN_RECT(const struct dialog_rect far *,
                                        struct dialog_range, const char far *);
extern void far pascal MoveTo(int, int);
extern void far pascal DRAW_NUL_TERMINATED_TEXT(const char far *);
extern int far pascal hGetTxt4(int);
extern char far * far pascal RESOLVE_HANDLE_TO_FAR_PAYLOAD(int);
extern char far * far pascal FETCH_LOCKED_SHAPE_LIST(int);
extern int far pascal ALLOCATE_PAYLOAD_HANDLE(unsigned long);
extern void far pascal READ_DAC_BLOCK(int, int, void far *);
extern void far pascal WRITE_DAC_BLOCK(int, int, const void far *, int);
extern void far pascal MEASURE_TEXT(struct ScreenPort *, int,
                                        void far *, void far *, int);
typedef struct palette_handle *PaletteHandle;
extern int far pascal fade_to(PaletteHandle, int, int);
extern void far pascal STROKE_PORT_RECT(void far *);

void far pascal DialogDrawStr(const char far *, int, int, const struct dialog_point *,
                              const struct dialog_rect far *, const struct dialog_range *);

void far pascal draw_dialog_text_block(struct dialog_text far *block)
{
    struct dialog_range range;
    int style;

    style = Graf->mode == 12 ? 0 : 1;
    range.last = -1;
    range.first = 0;
    DialogDrawStr(block->text, 10, style, 0, &block->bounds, &range);
    DISPOSE_HANDLE(hGetFont(10));
}

void far pascal DialogDrawNumber(int number, const struct dialog_point *point,
                                 const struct dialog_rect far *rect,
                                 const struct dialog_range *range)
{
    char text[10];

    _itoa(number, text, 10);
    DialogDrawStr(text, 10, 0, point, rect, range);
    DISPOSE_HANDLE(hGetFont(10));
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
    Graf->foreground = first_ink;
    if (rect) {
        OffsetRect(&shadow_rect, rect, 1, 0);
        DRAW_NUL_TEXT_IN_RECT(&shadow_rect, range, text);
    } else {
        MoveTo(point->y + 1, point->x);
        DRAW_NUL_TERMINATED_TEXT(text);
    }

    Graf->foreground = shadow;
    if (rect)
        DRAW_NUL_TEXT_IN_RECT(rect, range, text);
    else {
        MoveTo(point->y, point->x);
        DRAW_NUL_TERMINATED_TEXT(text);
    }

    SetFont(font + 1);
    Graf->foreground = ink;
    if (rect)
        DRAW_NUL_TEXT_IN_RECT(rect, range, text);
    else {
        MoveTo(point->y, point->x);
        DRAW_NUL_TERMINATED_TEXT(text);
    }
}

void far pascal DialogDrawStrId(int string_id, int font, int style,
                                const struct dialog_point *point,
                                const struct dialog_rect far *rect,
                                const struct dialog_range *range)
{
    int handle;

    handle = hGetTxt4(string_id);
    if (handle) {
        DialogDrawStr(FETCH_LOCKED_SHAPE_LIST(handle), font, style, point, rect, range);
        DISPOSE_HANDLE(handle);
    }
}

extern struct ScreenPort ScreenPort;
extern struct dialog_rect screen_bounds;
extern int dialog_port_handle;

int far pascal DialogFadeIn(void)
{
    int handle, result;

    READ_DAC_BLOCK(0, 32, RESOLVE_HANDLE_TO_FAR_PAYLOAD(handle = ALLOCATE_PAYLOAD_HANDLE(96UL)));
    WRITE_DAC_BLOCK(0, 32, 0, 1);
    MEASURE_TEXT(&ScreenPort, dialog_port_handle, &screen_bounds, &screen_bounds, 0);
    result = fade_to((PaletteHandle)handle, 3, 0);
    if (result)
        WRITE_DAC_BLOCK(0, 32, RESOLVE_HANDLE_TO_FAR_PAYLOAD(handle), 1);
    DISPOSE_HANDLE(handle);
    return result;
}

int far pascal DialogFadeOut(void)
{
    int result;

    result = fade_to(0, 3, 0);
    STROKE_PORT_RECT(&screen_bounds);
    return result;
}
extern struct dialog_rect GamePlayRect;
extern int gameplay_dialog_active;
extern int demo_pending;
#pragma pack(1)
typedef struct {
    unsigned int modifier, unknown_02;
} LEVELCELL;

typedef struct {
    unsigned char left, right, above, below;
} ROOMLINKS;

typedef struct {
    signed char opponent_count;
    unsigned char opponent_data[5][23];
} ROOMOBJECTS;

typedef struct {
    signed char room, cell;
} REBIRTHPOINT;

typedef struct {
    signed char count;
    unsigned char spawn_flags;
    unsigned char header[2];
    unsigned char points[3][10];
} ROOMGENERATORS;

typedef struct {
    unsigned char cell_kinds[32][30];                  /* 0000 */
    LEVELCELL cell_info[32][30];                       /* 03c0 */
    unsigned char unknown_12c0[0x500];                 /* 12c0 */
    ROOMLINKS room_links[32];                          /* 17c0 */
    unsigned char room_count;                         /* 1840 */
    unsigned char unknown_1841[4];
    unsigned char environment_kind;                  /* 1845 */
    unsigned char unknown_1846;
    signed char level_number;                         /* 1847 */
    int background_overlays[8];                       /* 1848 */
    unsigned char unknown_1858[4];
    int actor_phase;                                  /* 185c */
    unsigned char unknown_185e[2];
    signed char entry_room, entry_cell, entry_facing;    /* 1860-1862 */
    unsigned char unknown_1863[3];
    unsigned char preferred_opponent;                /* 1866 */
    ROOMOBJECTS room_objects[32];                      /* 1867 */
    REBIRTHPOINT rebirth_points[2];                    /* 26e7 */
    unsigned char unknown_26eb[0x0e];
    ROOMGENERATORS generators[32];                     /* 26f9 */
    unsigned char unknown_2b39[0x3c0];                 /* 2b39-2ef8 */
} LEVELREC;
#pragma pack()
extern LEVELREC Level;
extern int active_menu, menu_disabled, dialog_blocked;
extern signed char pending_key;
extern int control_mode;
extern int (far pascal *dialog_wait)(void);

extern void far pascal DeallocBackPort(int);
extern int far pascal AllocBackPort(struct ScreenPort far *);
extern void far pascal set_default_palette(void);
extern void far pascal restore_game_play_palette(void);
extern void far pascal clear_row_handles(void);
extern int far pascal PrepCut(int);
extern void far pascal set_kid_palette(signed char);
extern int far pascal Input(void);
extern int far pascal read_queued_key(void);
extern void far pascal wait_for_key(void);
extern void far pascal abort_with_message(int, const char far *);
typedef struct resource_handle *DialogAsset;
extern int far pascal create_frame_asset(DialogAsset, int, int);
extern void far pascal PaintRLEShapeItem(DialogAsset, int, int, int, int);
extern void far pascal fill_frame_buffer(void far *, int, int, int);
extern void far pascal set_frame_dimensions(void far *, int, int);
extern void far pascal InsetRect(struct dialog_rect far *, const void far *, int, int);
extern void far pascal SET_RECORD_RANGE(struct dialog_rect far *, int);
extern void far pascal release_shape_list(DialogAsset);
extern void far pascal dispatch_sound(int);
extern void far pascal save_game_play_palette(void);
extern void far pascal clear_life_bar(void);
extern DialogAsset far pascal hGetNMapShpList(int, int, int);

void far pascal DialogResetToGamePlay(int restart_input)
{
    if (dialog_port_handle)
        DeallocBackPort(dialog_port_handle);
    dialog_port_handle = AllocBackPort((struct ScreenPort far *)&GamePlayRect);
    Graf = &ScreenPort;
    SetFont(0);
    Graf->mode = 0;
    Graf->foreground = 15;
    STROKE_PORT_RECT(&screen_bounds);
    set_default_palette();
    DISPOSE_HANDLE(hGetFont(11));
    DISPOSE_HANDLE(hGetFont(12));
    DISPOSE_HANDLE(hGetFont(13));
    if (restart_input && !demo_pending) {
        clear_row_handles();
        PrepCut(0);
        gameplay_dialog_active = 1;
    }
    restore_game_play_palette();
    set_kid_palette((signed char)Level.environment_kind);
    dialog_wait = Input;
}

int far pascal DialogWaitFunc(void)
{
    int key;

    key = read_queued_key();
    if (key) {
        if (key == 0x1000 || key == 17)
            goto acknowledge;
        if (key == 27 || key == 32)
            goto done;
        key = 0;
        goto empty;
acknowledge:
        abort_with_message(0, 0);
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
    PaintRLEShapeItem(asset, 1, 0, 0, 0);
    PaintRLEShapeItem(asset, 2, 0, 192, 0);
    bitmap = (struct frame_bitmap far *)RESOLVE_HANDLE_TO_FAR_PAYLOAD(handle = create_frame_asset(asset, 3, 1));
    fill_frame_buffer(bitmap, 0, 8, 0);
    set_frame_dimensions(bitmap->pixels, bitmap->width, bitmap->height);
    fill_frame_buffer(bitmap, 312, 8, 0);
    DISPOSE_HANDLE(handle);
    InsetRect(&border, &screen_bounds, 9, 9);
    SET_RECORD_RANGE(&border, 12);
    if (temporary)
        release_shape_list(asset);
}

void far pascal InitDialog(void)
{
    dispatch_sound(0);
    save_game_play_palette();
    dialog_wait = DialogWaitFunc;
    Graf = &ScreenPort;
    clear_life_bar();
    SetFont(11);
    Graf->mode = 14;
    DeallocBackPort(dialog_port_handle);
    dialog_port_handle = AllocBackPort((struct ScreenPort far *)&screen_bounds);
    Graf = (struct ScreenPort *)dialog_port_handle;
    SetFont(11);
    STROKE_PORT_RECT(&screen_bounds);
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
    return hGetNMapShpList(8000, 1, 1);
}
struct save_slot_table {
    int count;
    char initials[10][25];
};

extern signed char LevelIdx, level_to_start, main_theme, sound_effect;
#pragma pack(1)
struct char_bounds { int top, left, bottom, right; };
typedef struct {
    signed char slot;                  /* 00 */
    signed char direction;             /* 01 */
    int x, y;                          /* 02, 04 */
    signed char character_kind;        /* 06 */
    unsigned int frame;                /* 07 */
    signed char column, row;           /* 09, 0a */
    signed char action;                /* 0b */
    signed char horizontal_step;       /* 0c */
    signed char vertical_step;         /* 0d */
    signed char room;                  /* 0e */
    signed char field_0f;              /* 0f */
    signed char stance;                /* 10 */
    signed char alive;                 /* 11 */
    signed char life_points;           /* 12 */
    signed char life_initial;          /* 13 */
    signed char life_delta;            /* 14 */
    int sequence_position;             /* 15 */
    int sequence_id;                   /* 17 */
    unsigned int sequence;             /* 19 */
    struct char_bounds bounds;                    /* 1b: top, left, bottom, right */
    signed char alert_delay;           /* 23 */
    int sequence_group;                /* 24 */
    int previous_x, previous_y;        /* 26, 28 */
    unsigned int previous_frame;       /* 2a */
    struct char_bounds previous_bounds;           /* 2c: top, left, bottom, right */
    int field_34;                      /* 34 */
    int previous_mode;                 /* 36 */
    unsigned char behavior;            /* 38 */
    signed char opponent_index;        /* 39 */
    unsigned char field_3a;            /* 3a */
    unsigned char unknown_3b[5];        /* 3b..3f */
} CHARREC;
#pragma pack()

extern CHARREC Kid;
extern int requested_level, resume_selected, dialog_timer, dialog_timer_copy;
extern void far pascal CLEAR_PAYLOAD_HEADER(void far *);
extern void far pascal UnloadCustomRoomData(void);
extern int far pascal find_shape_list(int);
extern char saved_message[];
extern char nosave_message[];
extern void far pascal draw_message(const char *);
extern void far pascal play_song(int);
extern unsigned far cdecl _fstrlen(const char far *);
extern char far * far cdecl _fstrcpy(char far *, const char far *);
extern void far pascal FRAME_RECT(const struct dialog_rect far *);
extern void far pascal play_sound(int);

int far pascal load_game_dialog(void);
int far pascal save_game_dialog(void);
int far pascal main_menu_choice(void);
void far pascal draw_save_slots(struct save_slot_table far *, int);
int far pascal select_save_slot(struct save_slot_table far *, int);
struct save_slot_table far * far pascal read_save_slot_table(void);
int far pascal ResumeGame(int slot);
int far pascal SaveGame(struct save_slot_table far *, int);
void far pascal post_save_message(int);
void far pascal show_dialog_notice(void);
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
        Graf = &ScreenPort;
        DialogFadeIn();
        DeallocBackPort(dialog_port_handle);
        *(volatile int *)&dialog_port_handle = selection;
        selection = select_save_slot(slots, 0);
        if (selection != -1) {
            loaded = ResumeGame(selection);
            if (loaded) {
                level_to_start = LevelIdx;
                requested_level = LevelIdx;
                resume_selected = 1;
            }
            selection = loaded;
        } else {
            selection = -1;
        }
        CLEAR_PAYLOAD_HEADER(slots);
        DialogFadeOut();
    }
    DialogResetToGamePlay(selection == -1 ? 1 : 0);
    if (selection > 0)
        show_dialog_notice();
    if (selection != -1 && dialog_port_handle) {
        DeallocBackPort(dialog_port_handle);
        dialog_port_handle = 0;
    }
    return selection;
}

int far pascal save_game_dialog(void)
{
    struct save_slot_table far *slots;
    int selection = 0;
    int chosen;

    InitDialog();
    DrawDialogFrame(0);
    slots = read_save_slot_table();
    if (slots) {
        draw_save_slots(slots, 1);
        Graf = &ScreenPort;
        DialogFadeIn();
        DeallocBackPort(dialog_port_handle);
        dialog_port_handle = selection;
        chosen = select_save_slot(slots, 1);
        if (chosen != -1)
            selection = SaveGame(slots, chosen);
        else
            selection = -1;
        CLEAR_PAYLOAD_HEADER(slots);
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

    STROKE_PORT_RECT(&screen_bounds);
    UnloadCustomRoomData();
    InitDialog();
    DrawDialogFrame(0);
    ((struct resource_shape far *)RESOLVE_HANDLE_TO_FAR_PAYLOAD(find_shape_list(1000)))->attributes = 0;
    hGetNMapShpList(1000, 2, 1);
    draw_main_menu();
    Graf = &ScreenPort;
    DialogFadeIn();
    DeallocBackPort(dialog_port_handle);
    dialog_port_handle = 0;
    selection = main_menu_choice();
    DialogFadeOut();
    DISPOSE_HANDLE(find_shape_list(1000));
    DialogResetToGamePlay(0);
    if (selection == 3)
        save_game_dialog();
    else if (selection == 4)
        load_game_dialog();
    SaveOptionsToDisk();
    set_kid_palette((signed char)Level.environment_kind);
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
    struct dialog_rect rect;
    int style;

    compute_save_slot_rect(&rect, index);
    draw_save_slot_frame(&rect, selected);
    style = selected != 0;
    if (_fstrlen(slots->initials[index])) {
        InsetRect(&rect, &rect, 2, 2);
        DialogDrawStr(slots->initials[index], 10, style, 0, &rect, 0);
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
    SET_RECORD_RANGE(rect, edge);
    Graf->foreground = fill;
    FRAME_RECT(rect);
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
    DISPOSE_HANDLE(light_font);
    DISPOSE_HANDLE(dark_font);
    light_font = hGetFont(10);
    hGetFont(11);
    OffsetRect(&save_description, &save_description, 0, -18);
    DialogDrawStrId(8001, 10, 0, 0, &save_description, &range);
    OffsetRect(&save_description, &save_description, 0, 18); range.last = -1; DialogDrawStrId(action, 10, 0, 0, &save_description, &range);
    range.last = 1; DialogDrawStrId(8002, 10, 0, 0, &save_description, &range);
    i = 0;
    do {
        draw_save_slot(slots, i, 0);
    } while (++i < 10);
    DISPOSE_HANDLE(light_font);
}

void far pascal post_save_message(int selection)
{
    if (selection != -1) {
        if (selection)
            draw_message(saved_message);
        else {
            draw_message(nosave_message);
            play_song(-1);
        }
        dialog_timer = dialog_timer_copy = 24;
    }
}

struct slot_editor {
    char state[0x2e];
    void (far pascal *draw)(struct dialog_text far *);
};

extern void far pascal CLEAR_FLAGS_OF_FAR_BLOCK(struct slot_editor far *);
extern struct slot_editor far * far pascal create_edit_field(struct dialog_rect far *,
                                          struct dialog_rect far *, char far *, int);
extern void far pascal show_edit_caret(struct slot_editor far *);
extern void far pascal blink_edit_caret(struct slot_editor far *);
extern void far pascal handle_edit_key(struct slot_editor far *, int);

int far pascal select_save_slot(struct save_slot_table far *slots, int saving)
{
    int result, done, slot, previous, key;
    struct slot_editor far *editor;
    struct dialog_rect rect;
    char initials[26];

    result = previous = -1;
    done = slot = 0;
    editor = 0;

    do {
        if (previous != slot) {
            if (previous != -1) {
                draw_save_slot(slots, previous, 0);
                if (editor)
                    CLEAR_FLAGS_OF_FAR_BLOCK(editor);
            }
            previous = slot;
            draw_save_slot(slots, slot, 1);
            if (saving == 1) {
                compute_save_slot_rect(&rect, slot);
                InsetRect(&rect, &rect, 2, 2);
                _fstrcpy(initials, slots->initials[slot]);
                editor = create_edit_field(&rect, &rect, initials, 24);
                editor->draw = draw_dialog_text_block;
                show_edit_caret(editor);
            }
        }
        if (editor)
            blink_edit_caret(editor);
        key = read_queued_key();
        switch (key) {
        case 9:
            previous = slot;
            if (slot != 9)
                ++slot;
            else
                slot = 0;
            break;
        case 13:
            if ((saving == 0 && !_fstrlen(slots->initials[slot])) ||
                (saving == 1 && !_fstrlen(initials))) {
                play_sound(-2);
                break;
            }
            if (editor)
                _fstrcpy(slots->initials[slot], initials);
            result = slot;
            /* fall through: Enter and Escape both leave the loop */
        case 27:
            done = 1;
            break;
        case 17:
        case 0x1000:
            abort_with_message(0, 0);
            break;
        default:
            if (editor) {
                if (key != '>' && key != '<')
                    handle_edit_key(editor, key);
            } else {
                switch (key) {
                case 0x4700:
                    slot = 0;
                    break;
                case 0x4800:
                    if (slot == 0)
                        slot = 9;
                    else
                        --slot;
                    break;
                case 0x4b00:
                case 0x4d00:
                    if (slot >= 5)
                        slot -= 5;
                    else
                        slot += 5;
                    break;
                case 0x4f00:
                    slot = 9;
                    break;
                case 0x5000:
                    if (slot != 9)
                        ++slot;
                    else
                        slot = 0;
                    break;
                }
            }
            break;

        }
    } while (!done);
    if (editor)
        CLEAR_FLAGS_OF_FAR_BLOCK(editor);
    return result;
}

extern unsigned far cdecl _dos_open(const char *, unsigned, int *);
extern long far cdecl filelength(int);
extern void far * far pascal ALLOCATE_PINNED_PAYLOAD(long);
extern unsigned far cdecl _dos_read(int, void far *, unsigned, unsigned *);
extern unsigned far cdecl _dos_close(int);
extern void far * far cdecl _fmemset(void far *, int, unsigned);

struct save_slot_table far * far pascal read_save_slot_table(void)
{
    int file;
    unsigned read_count;
    struct save_slot_table far *slots;
    int count;

    if (!_dos_open(save_path, 2, &file)) {
        count = filelength(file) >= sizeof(*slots) ?
                sizeof(*slots) : (int)filelength(file);
        slots = ALLOCATE_PINNED_PAYLOAD(count);
        if (_dos_read(file, slots, count, &read_count) ||
            read_count != count || slots->count > 10) {
            CLEAR_PAYLOAD_HEADER(slots);
            slots = 0;
        }
        _dos_close(file);
    } else {
        _fmemset(slots = ALLOCATE_PINNED_PAYLOAD(sizeof(*slots)), 0, sizeof(*slots));
    }
    return slots;
}

int far pascal IsValidTimeForSaveGame(void)
{
    int result = 1;

    if (LevelIdx > 14 || Kid.alive >= 0 || Kid.life_points <= 0)
        result = 0;
    return result;
}
#pragma pack(1)
struct room_object {
    unsigned state;
    unsigned linked_tile;
};

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
    int Invert;
    char room_state;
    char reserved18[5];
    unsigned char rooms[32][30];
    struct room_object room_links[32][30];
    char room_objects[3712];
    char level_state[1088];
};

extern struct rebirth_record far *hRebirthSaved;
extern CHARREC Char;
extern signed char CutRoom, starting_health, start_facing;
extern signed char cutscene_flags, timer_disabled;
extern int saved_game_active, Invert;
extern unsigned far cdecl _dos_creat(const char *, unsigned, int *);
extern unsigned far cdecl _dos_write(int, const void far *, unsigned, unsigned *);
extern long far cdecl lseek(int, long, int);
extern void far * far cdecl _fmemcpy(void far *, const void far *, unsigned);

void far pascal RemoveRebirthPt(void);

int far pascal ResumeGame(int slot)
{
    long offset;
    int result = 0;
    struct rebirth_record far *saved;
    int file;
    unsigned bytes_read;

    if (slot < 10 && !_dos_open(save_path, 2, &file)) {
        saved = ALLOCATE_PINNED_PAYLOAD(sizeof(*saved));
        offset = sizeof(struct save_slot_table) + slot * (long)sizeof(*saved);
        lseek(file, offset, 0);
        if (!_dos_read(file, saved, sizeof(*saved), &bytes_read) &&
            bytes_read == sizeof(*saved)) {
            if (saved->point_id) {
                if (!hRebirthSaved)
                    hRebirthSaved = ALLOCATE_PINNED_PAYLOAD(sizeof(*hRebirthSaved));
                _fmemcpy(hRebirthSaved, saved, sizeof(*saved));
            } else {
                RemoveRebirthPt();
            }
            active_menu = saved->menu;
            menu_disabled = saved->disabled;
            LevelIdx = saved->level;
            Kid.life_initial = Kid.life_points = starting_health = saved->health;
            cutscene_flags = saved->saved_flags;
            start_facing = saved->facing;
            Invert = saved->Invert;
            timer_disabled = saved->room_state;
            saved_game_active = 1;
            lseek(file, offset, 0);
            _dos_write(file, saved, sizeof(*saved), &bytes_read);
            result = 1;
        }
        CLEAR_PAYLOAD_HEADER(saved);
        _dos_close(file);
    }
    return result;
}

int far pascal SaveGame(struct save_slot_table far *slots, int slot)
{
    int result = 0;
    int size;
    int file;
    unsigned transferred;
    struct rebirth_record far *saved;

    if (slot < 10 &&
        (!_dos_open(save_path, 1, &file) || !_dos_creat(save_path, 0, &file))) {
        saved = ALLOCATE_PINNED_PAYLOAD(sizeof(*saved));
        if (hRebirthSaved)
            _fmemcpy(saved, hRebirthSaved, sizeof(*saved));
        else {
            _fmemset(saved, 0, sizeof(*saved));
            saved->level = LevelIdx;
            saved->health = starting_health;
            saved->direction = Level.entry_facing;
        }
        saved->menu = active_menu;
        saved->disabled = menu_disabled;
        saved->saved_flags = cutscene_flags;
        saved->facing = start_facing;
        saved->Invert = Invert;
        saved->room_state = timer_disabled;
        size = sizeof(*slots);
        if (!_dos_write(file, slots, size, &transferred) && transferred == size) {
            size = sizeof(*saved);
            lseek(file, slot * (long)size, 1);
            if (!_dos_write(file, saved, size, &transferred) && transferred == size)
                result = 1;
        }
        CLEAR_PAYLOAD_HEADER(saved);
        _dos_close(file);
    }
    return result;
}

void far pascal prepare_saved_room_objects(void);

void far pascal SaveRebirthPtStatus(int point_id)
{
    if (!hRebirthSaved)
        hRebirthSaved = ALLOCATE_PINNED_PAYLOAD(sizeof(*hRebirthSaved));
    hRebirthSaved->point_id = point_id;
    _fmemcpy(hRebirthSaved->rooms, Level.cell_kinds, sizeof(Level.cell_kinds));
    _fmemcpy(hRebirthSaved->room_links, Level.cell_info, sizeof(Level.cell_info));
    _fmemcpy(hRebirthSaved->room_objects, Level.room_objects, sizeof(Level.room_objects));
    _fmemcpy(hRebirthSaved->level_state, Level.generators, sizeof(Level.generators));
    hRebirthSaved->level = Level.level_number;
    hRebirthSaved->health = Kid.life_initial;
    hRebirthSaved->direction = Kid.direction;
    hRebirthSaved->Invert = Invert;
    hRebirthSaved->facing = start_facing;
    prepare_saved_room_objects();
}

void far pascal RemoveRebirthPt(void)
{
    if (hRebirthSaved) {
        CLEAR_PAYLOAD_HEADER(hRebirthSaved);
        hRebirthSaved = 0;
    }
}

struct room_control {
    char reserved[2];
    unsigned char state;
};

extern struct room_control *far pascal get_trob_room_cell(unsigned char, int);

void far pascal prepare_saved_room_objects(void)
{
    unsigned char room = 0;
    signed char tile;
    unsigned saved;
    struct room_control *control;
    unsigned value;

    if (Level.room_count != 0) {
        do {
            tile = 0;
            do {
                switch (hRebirthSaved->rooms[room][tile]) {
                case 4:
                    saved = hRebirthSaved->room_links[room][tile].state;
                    value = saved & 0xff;
                    control = get_trob_room_cell(room + 1, tile);
                    if (control && control->state == 3)
                        value = 200;
                    else if (control)
                        value = 0;
                    saved &= 0xff00;
                    value |= saved;
                    break;
                case 17:
                    value = hRebirthSaved->room_links[room][tile].state;
                    if (!(hRebirthSaved->room_links[room][tile].state & 0xff))
                        continue;
                    value &= 0xff00;
                    value |= 42;
                    break;
                default:
                    continue;
                }
                hRebirthSaved->room_links[room][tile].state = value;
            } while (++tile < 30);
        } while (++room < Level.room_count);
    }
}

void far pascal SetRebirthPtLevelStatus(void)
{
    if (hRebirthSaved) {
        _fmemcpy(Level.cell_kinds, hRebirthSaved->rooms, sizeof(Level.cell_kinds));
        _fmemcpy(Level.cell_info, hRebirthSaved->room_links, sizeof(Level.cell_info));
        _fmemcpy(Level.room_objects, hRebirthSaved->room_objects, sizeof(Level.room_objects));
        _fmemcpy(Level.generators, hRebirthSaved->level_state, sizeof(Level.generators));
        Invert = hRebirthSaved->Invert;
    }
}

int far pascal SetRebirthPtKidStatus(void)
{
    const REBIRTHPOINT *point;
    int result;

    if (hRebirthSaved) {
        /* Saved point IDs are one-based. */
        point = &Level.rebirth_points[hRebirthSaved->point_id - 1];
        CutRoom = Char.room = point->room;
        Char.column = point->cell % 10;
        Char.row = point->cell / 10;
        starting_health = hRebirthSaved->health;
        Char.direction = hRebirthSaved->direction;
        start_facing = hRebirthSaved->facing;
        result = 2;
    } else {
        result = -1;
    }
    return result;
}
#pragma pack()

/* The sibling DOS image preserves the guard before the challenge setup. */
struct notice_record {
    unsigned symbol, detail;
};

extern int replay_input;
extern int far pascal HGETRSRC(unsigned long resource, int index);
extern unsigned far pascal Rnd(unsigned);

void far pascal draw_notice_symbols(unsigned, DialogAsset, int);
int far pascal read_notice_selection(void);

void far pascal show_dialog_notice(void)
{
    int passed = 1;
    unsigned attempt;
    unsigned previous;
    unsigned index;
    unsigned count;
    int table_handle;
    DialogAsset asset;
    unsigned far *table;
    struct notice_record prompt;

    if (!notice_state.done && !replay_input) {
        notice_state.done = passed;
        InitDialog();
        asset = load_frame_asset();
        DrawDialogFrame(asset);
        attempt = 0;
        previous = -1;
        table_handle = HGETRSRC(0UL, 8000);
        table = (unsigned far *)FETCH_LOCKED_SHAPE_LIST(table_handle);
        count = table[0];
        do {
            index = Rnd(count - 1);
            if (index == previous)
                index = (Rnd(count - 2) + index + 1) % count;
            previous = index;
            index = index * 2 + 1;
            _fmemcpy(&prompt, &table[index], sizeof(prompt));
            Graf = (struct ScreenPort *)dialog_port_handle;
            draw_notice_symbols(prompt.symbol, asset, attempt);
            Graf = &ScreenPort;
            if (!attempt)
                DialogFadeIn();
            else
                MEASURE_TEXT(&ScreenPort, dialog_port_handle, notice_port, notice_port, 0);
            index = read_notice_selection();
            passed = index == prompt.detail || index == (unsigned)-2;
        } while (!passed && ++attempt != 3);
        DISPOSE_HANDLE(table_handle);
        release_shape_list(asset);
        if (dialog_port_handle) {
            DeallocBackPort(dialog_port_handle);
            dialog_port_handle = 0;
        }
        DialogFadeOut();
        DialogResetToGamePlay(0);
    }
    if (!passed)
        abort_with_message(1, notice_failure);
}

int * far pascal ComputeSymbolPt(int *p, int index)
{
    p[1] = (index % 5) * 42 + 62;
    p[0] = (index / 5) * 42 + 69;
    return p;
}
extern char * far cdecl _nmalloc(unsigned);
extern void far cdecl _nfree(void *);
extern int far cdecl sprintf(char *, const char *, ...);

void far pascal draw_notice_symbols(unsigned symbol, DialogAsset asset, int attempt)
{
    char text[100];
    int text_handle;
    const char far *format;
    char *near_format;
    int point[2];
    struct dialog_range range;
    int index;

    if (attempt)
        SET_RECORD_RANGE((struct dialog_rect far *)notice_port, 12);
    text_handle = hGetTxt4(8006);
    format = FETCH_LOCKED_SHAPE_LIST(text_handle);
    near_format = _nmalloc(_fstrlen(format) + 1);
    _fstrcpy(near_format, format);
    sprintf(text, near_format, symbol);
    DISPOSE_HANDLE(text_handle);
    _nfree(near_format);
    DialogDrawStr(text, 10, 0, &symbol_caption, 0, 0);

    if (!attempt) {
        DialogDrawStrId(8005, 10, 0, &symbol_heading, 0, 0);
        /* The original writes only the last bound for these two lines. */
        range.last = -1;
        DialogDrawStrId(8007, 10, 0, 0, &save_description, &range);
        range.last = 1;
        DialogDrawStrId(8008, 10, 0, 0, &save_description, &range);
        index = 0;
        do {
            ComputeSymbolPt(point, index);
            PaintRLEShapeItem(asset, index + 4, point[1], point[0], 10);
        } while (++index < 10);
    }
}

void far pascal draw_notice_cursor(int index, int selected)
{
    struct { int * ink; struct dialog_rect rect; } state;
    int previous_ink;

    state.ink = &Graf->foreground;
    previous_ink = *state.ink;

    if (selected)
        *state.ink = 14;
    else
        *(unsigned *)state.ink = 12;
    ComputeSymbolPt((int *)&state.rect, index);
    state.rect.left -= 3;
    state.rect.right = state.rect.left + 35;
    state.rect.top -= 3;
    state.rect.bottom = state.rect.top + 35;
    FRAME_RECT(&state.rect);
    Graf->foreground = previous_ink;
}


int far pascal read_notice_selection(void)
{
    int index = 0;
    int previous = -1;
    int done = 0;
    unsigned key;

    do {
        if (previous != index) {
            if (previous != -1)
                draw_notice_cursor(previous, 0);
            previous = index;
            draw_notice_cursor(index, 1);
        }
        key = read_queued_key();
        switch (key) {
        case 9:
            previous = index;
            if (index != 9)
                ++index;
            else
                index = 0;
            continue;
        case 13:
            done = 1;
            continue;
        case 17:
        case 0x1000:
            abort_with_message(0, 0);
            break;
        case 0x4700:
            index = 0;
            continue;
        case 0x4800:
        case 0x5000:
            if (index >= 5)
                index -= 5;
            else
                index += 5;
            continue;
        case 0x4b00:
            if (index % 5 == 0)
                index += 4;
            else
                --index;
            continue;
        case 0x4d00:
            if (index % 5 == 4)
                index -= 4;
            else
                ++index;
            continue;
        case 0x4f00:
            index = 9;
            continue;
        }
    } while (!done);
    draw_notice_cursor(index, 0);
    return index;
}

#pragma pack(1)
struct hall_record {
    char name[27];
    int score;
};

struct hall_table {
    int count;
    struct hall_record records[5];
};
#pragma pack()

struct hall_table far * far pascal read_hall_file(int, int *);
void far pascal draw_hall_records(struct hall_table far *);
void far pascal edit_hall_name(struct hall_table far *, int);
int far pascal write_hall_file(struct hall_table far *);
int far * far pascal ComputeHallPlayerRect(int far *, int);
extern int far pascal wait_ticks(int, int);

void far pascal enter_hall_record(int score)
{
    int index;
    struct hall_table far *entries;
    struct dialog_rect rect;
    struct hall_table far *table;

    entries = read_hall_file(score, &index);
    if (entries) {
        InitDialog();
        draw_hall_records(entries);
        Graf = &ScreenPort;
        DialogFadeIn();
        edit_hall_name(entries, index);
        Graf = (struct ScreenPort *)dialog_port_handle;
        ComputeHallPlayerRect((int far *)&rect, index);
        InsetRect(&rect, &rect, -2, 0);
        SET_RECORD_RANGE(&rect, 12);
        InsetRect(&rect, &rect, 2, 0);
        table = entries;
        DialogDrawStr(table->records[index].name, 10, 0, 0, &rect, 0);
        InsetRect(&rect, &rect, -2, 0);
        MEASURE_TEXT(Graf = &ScreenPort, dialog_port_handle, &rect, &rect, 0);
        write_hall_file(table);
        wait_ticks(0, 900);
        DialogFadeOut();
        DialogResetToGamePlay(0);
        if (dialog_port_handle) {
            DeallocBackPort(dialog_port_handle);
            dialog_port_handle = 0;
        }
        if (entries)
            CLEAR_PAYLOAD_HEADER(entries);
        dispatch_sound(10253);
    }
}

int far * far pascal ComputeHallPlayerRect(int far *p, int index)
{
    p[1] = 25;
    p[3] = 155;
    p[0] = index * 24 + 59;
    p[2] = p[0] + 20;
    return p;
}
extern int far pascal wait_key(void);
extern int far pascal GET_PAYLOAD_SIZE(void far *);
extern void far * far cdecl _fmemmove(void far *, const void far *, unsigned);

struct hall_editor {
    char private_state[0x26];
    char far *name;
    char reserved[4];
    void (far pascal *draw)(struct dialog_text far *);
};

void far pascal draw_hall_records(struct hall_table far *table)
{
    struct dialog_rect rect;
    struct dialog_range range;
    int row;

    DrawDialogFrame(0);
    range.first = range.last = 0;
    DialogDrawStrId(8009, 12, 0, 0, &save_heading, &range);
    draw_save_slot_frame(&hall_box, 0);
    range.last = -1;
    DialogDrawStrId(8010, 10, 0, &hall_name_caption, 0, &range);
    DialogDrawStrId(8012, 10, 0, &hall_score_caption, 0, &range);

    for (row = 0; row < table->count; ++row) {
        ComputeHallPlayerRect((int far *)&rect, row);
        range.last = -1; DialogDrawStr(table->records[row].name, 10, 0, 0, &rect, &range);
        rect.top = 185; rect.bottom = 220;
        range.last = 0;
        OffsetRect(&rect, &rect, 75, 0);
        DialogDrawNumber(table->records[row].score, 0, &rect, &range);
    }
}

void far pascal edit_hall_name(struct hall_table far *table, int row)
{
    struct dialog_rect rect;
    struct hall_editor far *editor;
    int key, done = 0;

    ComputeHallPlayerRect((int far *)&rect, row);
    InsetRect(&rect, &rect, -2, 0);
    draw_save_slot_frame(&rect, 1);
    InsetRect(&rect, &rect, 2, 2);
    editor = (struct hall_editor far *)create_edit_field(
        &rect, &rect, table->records[row].name, 24);
    editor->draw = draw_dialog_text_block;
    show_edit_caret((struct slot_editor far *)editor);
    do {
        blink_edit_caret((struct slot_editor far *)editor);
        key = read_queued_key();
        if (key == 13) {
            if (!_fstrlen(editor->name))
                play_sound(-2);
            else
                done = 1;
        } else if (key && key != '>' && key != '<') {
            handle_edit_key((struct slot_editor far *)editor, key);
        }
    } while (!done);
    CLEAR_FLAGS_OF_FAR_BLOCK((struct slot_editor far *)editor);
}

struct hall_table far * far pascal insert_hall_record(
    struct hall_table far *table, int score, int *index)
{
    struct hall_table far *old;
    int size, row;

    if (table) {
        old = table;
        size = GET_PAYLOAD_SIZE(old) + sizeof(struct hall_record);
        table = ALLOCATE_PINNED_PAYLOAD(size);
        _fmemcpy(table, old, size - sizeof(struct hall_record));
        CLEAR_PAYLOAD_HEADER(old);
    } else {
        table = ALLOCATE_PINNED_PAYLOAD(sizeof(table->count) + sizeof(struct hall_record));
        table->count = 0;
    }

    for (row = 0; row < table->count; ++row)
        if (table->records[row].score <= score)
            break;
    if (row < table->count && row != 4) {
        _fmemmove(&table->records[row + 1], &table->records[row],
                  (table->count - row) * sizeof(struct hall_record));
    }
    if (table->count < 5)
        ++table->count;
    else if (row == 5)
        row = 4;
    table->records[row].name[0] = 0;
    table->records[row].score = score;
    *index = row;
    return table;
}

struct hall_table far * far pascal read_hall_file(int score, int *index)
{
    struct hall_table far *table = 0;
    int file, size;
    unsigned transferred;

    if (!_dos_open(hall_read_path, 2, &file)) {
        size = (int)filelength(file);
        table = ALLOCATE_PINNED_PAYLOAD(size);
        if (_dos_read(file, table, size, &transferred) ||
            transferred != size || table->count > 5) {
            CLEAR_PAYLOAD_HEADER(table);
            table = 0;
        }
        _dos_close(file);
    }
    if (score > 0)
        table = insert_hall_record(table, score, index);
    else {
        if (table)
            return table;
        table = ALLOCATE_PINNED_PAYLOAD(sizeof(table->count) + sizeof(struct hall_record));
        table->count = 0;
    }
    return table;
}

int far pascal show_hall_records(int hold)
{
    struct hall_table far *table;
    int index;
    unsigned key = 0;

    table = read_hall_file(0, &index);
    if (table && (table->count || hold)) {
        InitDialog();
        play_song(253);
        draw_hall_records(table);
        Graf = &ScreenPort;
        key = DialogFadeIn();
        if (!key)
            key = hold ? wait_key() : wait_ticks(0, 900);
        if (key && !hold)
            SET_RECORD_RANGE(&screen_bounds, 0);
        else
            key = DialogFadeOut();
        DialogResetToGamePlay(hold);
        if (dialog_port_handle && !hold) {
            DeallocBackPort(dialog_port_handle);
            dialog_port_handle = 0;
        }
        dispatch_sound(10253);
    }
    if (table)
        CLEAR_PAYLOAD_HEADER(table);
    return key;
}

int far pascal write_hall_file(struct hall_table far *table)
{
    int written = 0;
    int file;
    unsigned transferred;
    int size;

    if (!_dos_creat(hall_write_path, 0, &file)) {
        size = table->count * sizeof(struct hall_record) + sizeof(table->count);
        if (!_dos_write(file, table, size, &transferred) && size == transferred)
            written = 1;
        _dos_close(file);
    }
    return written;
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
        switch ((unsigned)read_queued_key()) {
        case 9:
            previous = option;
            if (option == 5)
                option = 0;
            else
                ++option;
            break;
        case 0x4700:
            option = 0;
            break;
        case 13:
            if (option != 3 && option != 4) {
                if (change_menu_option(option))
                    draw_menu_label(option, 1);
                else
                    play_sound(-2);
            } else if (option == 3 && !IsValidTimeForSaveGame())
                play_sound(-2);
            else
                done = 1;
            break;
        case 17:
        case 0x1000:
            abort_with_message(0, 0);
            break;
        case 27:
            option = 0;
            done = 1;
            break;
        case 0x4800:
            if (option == 0)
                option = 5;
            else
                --option;
            break;
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
        case 0x5000:
            if (option == 5)
                option = 0;
            else
                ++option;
            break;
        }
    } while (!done);
    return option;
}

void far pascal draw_main_menu(void)
{
    int i;
    struct dialog_range range;

    Graf->mode = 12;
    range.first = range.last = 0;
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
extern char sound_on_message[], sound_off_message[], ambient_on_message[], ambient_off_message[], keyboard_mode_message[], joystick_mode_message[];

struct menu_text_10 { char value[10]; };
struct menu_text_12 { char value[12]; };

void far pascal draw_menu_label(int index, int selected)
{
    char label_buffer[40];
    struct dialog_rect rect;
    struct dialog_range range;
    const char *label;

    if (MAKE_INDEX_RECT((int *)&rect, index)) {
        range.last = -1;
        range.first = 0;
        switch (index) {
        case 0:
            label = sound_volume ? sound_on_message
                                 : sound_off_message;
            break;
        case 1:
            label = menu_music ? ambient_on_message
                               : ambient_off_message;
            break;
        case 2:
            /* The keyboard label immediately follows the joystick strings in DGROUP. */
            label = control_mode == 0 ? keyboard_mode_message
                                      : joystick_mode_message;
            break;
        case 3:
            *(struct menu_text_10 *)label_buffer = *(const struct menu_text_10 *)save_option;
            label = label_buffer;
            break;
        case 4:
            *(struct menu_text_12 *)label_buffer = *(const struct menu_text_12 *)resume_option;
            label = label_buffer;
            break;
        case 5:
            *(struct menu_text_10 *)label_buffer = *(const struct menu_text_10 *)continue_option;
            label = label_buffer;
            break;
        }
        if (selected)
            SET_RECORD_RANGE(&rect, 12);
        DialogDrawStr(label, 10, 0, 0, &rect, &range);
    }
}

extern int far pascal acquire_frame_shape(int, int);
extern void far pascal MIRRORMCGABLOCK(void far *, int, int);
extern void far pascal PAINT_PLAIN(void far *, int, int, int);

void far pascal draw_menu_icon(int index, int selected)
{
    struct dialog_rect rect;
    struct dialog_point origin;
    struct frame_bitmap far *bitmap;
    int handle;

    if (MAKE_INDEX_RECT((int *)&rect, index)) {
        origin.x = rect.left + 6;
        origin.y = rect.top - 22;
        handle = acquire_frame_shape(find_shape_list(1000), 5);
        if (handle) {
            bitmap = (struct frame_bitmap far *)RESOLVE_HANDLE_TO_FAR_PAYLOAD(handle);
            if (selected) {
                MIRRORMCGABLOCK(bitmap->pixels, bitmap->width, bitmap->height);
                PAINT_PLAIN(bitmap, origin.y, origin.x, 10);
            } else {
                *(struct dialog_point *)&rect = origin;
                rect.right = bitmap->width + rect.left;
                rect.bottom = bitmap->stride + rect.top;
                SET_RECORD_RANGE(&rect, 12);
            }
        }
    }
}

extern void far pascal SET_SOUND_LEVEL(unsigned char);

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
    int size;

    if (!_dos_open(options_path, 0, &file)) {
        size = sizeof(options);
        if (!_dos_read(file, &options, size, &transferred) && transferred == size) {
            if (!options.sound)
                SET_SOUND_LEVEL(0);
            if (!options.music)
                menu_music = 0;
        }
        _dos_close(file);
    }
}

extern void far pascal toggle_ambient(void);
extern void far pascal joystick_status(void);

int far pascal change_menu_option(int option)
{
    int changed = 1;

    switch (option) {
    case 0:
        SET_SOUND_LEVEL((unsigned char)(!sound_volume * 15));
        break;
    case 1:
        changed = menu_music;
        toggle_ambient();
        changed = menu_music != changed;
        break;
    case 2:
        if (control_mode == 2)
            control_mode = 0;
        else {
            joystick_status();
            if (control_mode != 2)
                changed = 0;
        }
        break;
    case 5:
        abort_with_message(0, 0);
        break;
    }
    return changed;
}

extern int far pascal animate_port(int, int, const struct dialog_rect far *, long);
extern int credits_timer;
extern int far pascal find_option(const char far *);
extern int far pascal allocate_clipped_port(const struct dialog_rect far *);
extern void far pascal draw_record_bounds(int);
extern void far pascal free_record_pair(int);
extern void far pascal Move(int, int);
extern char far * far pascal lookup_prefixed_string(char far *, unsigned char);

void far pascal draw_credits_page(int, int);
void far pascal draw_credit_category(int);
static const char credits_blink_key[] = "BLINK";

int far pascal show_credits(void)
{
    struct dialog_rect clip;
    int key, page;

    InitDialog();
    Graf = (struct ScreenPort *)dialog_port_handle;
    DrawDialogFrame(0);
    play_song(268);
    InsetRect(&clip, &screen_bounds, 9, 9);
    page = key = 0;
    do {
        if (key)
            break;
        Graf = (struct ScreenPort *)dialog_port_handle;
        SET_RECORD_RANGE(&clip, 12);
        draw_credits_page(credits_first[page], credits_last[page]);
        Graf = &ScreenPort;
        if (page)
            key = animate_port(1, dialog_port_handle, &screen_bounds, 90);
        else
            key = DialogFadeIn();
        if (!key) {
            if (!page && find_option(credits_blink_key)) {
                int port;
                credits_timer = 480;
                port = allocate_clipped_port(&credits_blink_box);
                while (!key && credits_timer) {
                    draw_record_bounds(port);
                    key = wait_ticks(1, 15);
                    if (!key) {
                        SET_RECORD_RANGE(&credits_blink_box, 12);
                        key = wait_ticks(1, 15);
                    }
                }
                free_record_pair(port);
            } else {
                key = wait_ticks(0, 480);
            }
        }
    } while (++page < sizeof(credits_first) / sizeof(*credits_first));
    Graf = &ScreenPort;
    DialogFadeOut();
    DISPOSE_HANDLE(hGetFont(10));
    DialogResetToGamePlay(0);
    if (dialog_port_handle) {
        DeallocBackPort(dialog_port_handle);
        dialog_port_handle = 0;
    }
    dispatch_sound(10268);
    return key;
}

void far pascal draw_credits_page(int first, int last)
{
    int id;

    MoveTo(save_heading.top, save_heading.right);
    id = first;
    if (id <= last) {
        do {
            draw_credit_category(id);
            Move(0, 4);
        } while (++id <= last);
    }
}

void far pascal draw_credit_category(int id)
{
    struct dialog_rect rect;
    struct dialog_range range;
    char far *strings, far *payload;
    char far *title;
    unsigned char line;
    int handle = hGetTxt4(id);

    if (!handle)
        return;
    payload = FETCH_LOCKED_SHAPE_LIST(handle);
    rect.right = *(int *)((char *)Graf + 0x22);
    rect.top = save_description.top;
    rect.left = rect.right - 19;
    rect.bottom = save_description.bottom;
    strings = payload;
    title = lookup_prefixed_string(strings, 1);
    if ((unsigned char)strings[0] == 1) {
        range.last = 0;
        if (id == credits_first[4] || id == credits_first[3] || (id >= 1020 && id <= 1026))
            InsetRect(&rect, &screen_bounds, 9, 9);
    } else {
        range.last = -1;
    }
    range.first = 0;
    DialogDrawStr(title, 10, 0, 0, &rect, &range);
    if ((unsigned char)strings[0] > 1) {
        range.last = 1;
        line = 2;
        if ((unsigned char)strings[0] >= 2) {
            do {
                DialogDrawStr(lookup_prefixed_string(strings, line), 10, 0, 0, &rect, &range);
                OffsetRect(&rect, &rect, 0, 21);
                Move(0, 21);
            } while (++line <= (unsigned char)strings[0]);
        }
    } else {
        Move(0, 21);
    }
    DISPOSE_HANDLE(handle);
}
