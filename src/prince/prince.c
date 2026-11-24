#pragma pack(1)
typedef struct {
    unsigned char number;                             /* 00 */
    unsigned char left, right, above, below;           /* 01-04 */
    unsigned char above_left, above_right;             /* 05-06 */
    unsigned char below_left, below_right;             /* 07-08 */
    unsigned char custom;                             /* 09 */
} ROOMREC;

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

/* Resident PRINCE.C, DOS root [0000,1340). */

extern const char *resource_files[3];
extern LEVELREC Level;
extern char selected_level;
extern char LevelIdx;
extern int command_line_enabled;
extern int skip_opening;
extern int restart_from_demo;

extern int demo_level;
extern int demo_changed;
extern int demo_pending;
extern int minutes_remaining;
static char title[] = "PRINCE OF PERSIA 2 v1.1";
/* FM Towns names these adjacent lookup tables OppCharId and CharOppId. */
unsigned char OppCharId[11] = { 2, 10, 4, 6, 1, 7, 8, 2, 11, 12, 0 };
unsigned char opponent_outfit_by_character[15] = {
    10, 4, 0, 0, 2, 0, 3, 5, 6, 0, 1, 8, 9, 0, 0
};

typedef struct {
    int unused;
    int control_mode;
    int memory;
    unsigned int digital_type, midi_type;
} GameOptions;

extern GameOptions *configuration;
extern unsigned char active_sound_channels;
extern void far *digital_driver, far *midi_driver;
extern int far cdecl sprintf(char *buffer, const char *format, ...);
extern void far pascal reset_overlay_state(const char *message);
extern void far pascal platform_setup(void);
extern void far pascal SetOptionsFromDisk(void);
extern void far pascal open_verified_resource(const char *filename);
extern int far pascal Rnd(int seed);
extern char near *far pascal find_option(const char far *option);
extern int far cdecl atoi(const char near *text);
extern void far pascal check_heaps(const char near *message);
extern void far pascal check_null_area(const char near *message);
extern void far pascal show_dialog(const char near *title, const char far *message, int flags);
extern char far pascal start_game(char level);
extern int far pascal play_nis(int sequence);
extern void far pascal enter_hall_record(int level);
extern void far pascal CheckPlayNIS(int status, int flags);
extern void far pascal RECLAIM_HEAP_BLOCKS(long color);
extern void far pascal COMPACT_HEAP_BLOCKS(long color);
extern int far pascal show_credits(void);
extern int far pascal show_hall_records(int mode);
extern void far pascal TurnRecordedGamePlayerOff(void);
extern void far pascal TurnRecordedGamePlayerOn(void);
extern void far pascal reset_level_overlays(void);
extern int far pascal load_game_dialog(void);
extern void far pascal dispatch_sound(int mode);
extern int far pascal init_port(void near *port, void far *buffer);
extern void far pascal clear_peels(void);

extern unsigned char ScreenPort[];
extern unsigned char game_buffer[];
extern int Graf;
extern int LastResume;

void far pascal ResetGame(void);
void far pascal read_start_level(void);
char far pascal PlayOpeningSequence(void);
char far pascal poll_and_clear_pending_state(void);
void far pascal ResetData(void);

/* 0000 */ void far cdecl main(void)
{
    const char **file;
    char message[100];

    platform_setup();
    if (find_option("sbdiag")) {
        sprintf(message, "DType:%d MType:%d\rDDrv:%d MDrv: %d\rDInit:%d MInit:%d",
                configuration->digital_type != 0, configuration->midi_type != 0,
                digital_driver ? 1 : 0, midi_driver ? 1 : 0,
                (active_sound_channels & 1) != 0, (active_sound_channels & 2) != 0);
        reset_overlay_state(message);
    }
    SetOptionsFromDisk();
    read_start_level();
    file = resource_files + 1;
    do {
        open_verified_resource(*file);
        ++file;
    } while (file < resource_files + 3);
    Rnd(1);
    Level.environment_kind = 0;
    ResetGame();
}

/* 0036 */ void far pascal ResetGame(void)
{
    char level = selected_level;

    for (;;) {
        if (command_line_enabled) {
            if (find_option("WATCHMEM")) {
                const char near * const restart_message = "At beginning of ResetGame() loop.";
                check_heaps(restart_message);
                check_null_area(restart_message);
                show_dialog(0, restart_message, 0);
            }
            if (find_option("NIS") || find_option("TREE"))
                break;
        }
        ResetData();
        if (selected_level == 0) {
            level = PlayOpeningSequence();
        } else {
            level = start_game(level);
            if (restart_from_demo)
                Level.level_number = level = 0;
            if (minutes_remaining == 0) {
                selected_level = level = 0;
            } else if (level > 14) {
                play_nis(11);
                enter_hall_record(minutes_remaining);
                selected_level = level = 0;
            } else if (selected_level != 0 && level == 0) {
                selected_level = level = Level.level_number;
            }
        }
    }
    for (;;)
        CheckPlayNIS(0, 0);
}

/* 0116 */ void far pascal read_start_level(void)
{
    char near *option;

    selected_level = 0;
    if (command_line_enabled) {
        option = find_option("LEVEL");
        if (option == 0)
            return;
        selected_level = (char)atoi(&option[5]);
        if (selected_level < 1 || selected_level > 14)
            selected_level = 1;
    }
}

static int skip_intro = 1;

/* 014e */ char far pascal PlayOpeningSequence(void)
{
    int result;
    char level;

    RECLAIM_HEAP_BLOCKS(0x00ffffffL); COMPACT_HEAP_BLOCKS(0x00ffffffL);
    result = 1;
    if (!skip_intro) {
        if (skip_opening)
            result = 2;
        else if (show_credits() || show_hall_records(0))
            result = 2;
    } else {
        skip_intro = 0;
    }
    if (result != 2)
        result = play_nis(7);
next_opening_clip:
    if (result != 2) result = play_nis(4);
    if (result != 2) result = play_nis(8);
    if (result == 2) {
        selected_level = 1;
        skip_intro = 1;
        TurnRecordedGamePlayerOff();
    } else {
        TurnRecordedGamePlayerOn();
    }
    level = selected_level;
    reset_level_overlays();
    if (demo_pending)
        level = poll_and_clear_pending_state();
    return level;
}

/* 0210 */ char far pascal poll_and_clear_pending_state(void)
{
    char level = selected_level;

    if (load_game_dialog() > 0) {
        level = selected_level = LevelIdx;
        demo_changed = 0;
    }
    demo_pending = 0;
    return level;
}


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
extern CHARREC Char;
extern CHARREC Kid;

extern ROOMREC Room;
extern signed char joystick[3];
#define joystick_y (joystick[0])
#define joystick_x (joystick[1])
#define joystick_button (joystick[2])
extern signed char timer_disabled;
extern int Pause;
extern int minutes_remaining, countdown_pending, restart_blocked;
extern int requested_level, control_mode, message_ticks, message_duration;
extern unsigned int ticks_remaining;
extern char sound_on_message[], sound_off_message[], joystick_unavailable_message[], joystick_mode_message[], joystick_missing_message[], keyboard_mode_message[];

extern unsigned int far pascal poll_user(void);
extern int far pascal IsValidTimeForDialog(void);
extern int far pascal IsValidTimeForSaveGame(void);
extern void far pascal save_game_dialog(void);
extern void far pascal main_menu_dialog(void);
extern void far pascal copy_kid_to_char(void);
extern int far pascal toggle_sound(void);
extern int far pascal show_hall_records(int mode);
extern char *far pascal joystick_status(void);
extern int far pascal load_game_dialog(void);
extern const char *far pascal toggle_ambient(void);
extern int far pascal is_leave_level_music_on(void);
extern void far pascal draw_message(const char *message);
extern void far pascal dispatch_sound(int mode);
extern int far pascal in_special_room(void);
extern void far pascal clear_keyboard(void);
extern void far pascal handle_debug_input(unsigned int key);

unsigned int far pascal prince_poll(void)
{
    const char *message = 0; int restart = 0; unsigned int key = poll_user();

    if (joystick_button || key) {
        if ((minutes_remaining && Kid.alive > 6) || restart_from_demo) { restart = 1;
            if (restart_from_demo) skip_opening = 1;
        }
    }

    if (key) {
        clear_keyboard(); switch (key) {
        case 0x1b: Pause = 1; break;
        case 0x20: if (timer_disabled >= 0) countdown_pending = 1; break;
        case 0x1300:
            selected_level = 0; timer_disabled = -1;
        case 0x1e00:
            if (!is_leave_level_music_on()) restart = 1; break;
        case 0x1800: if (IsValidTimeForDialog() && !restart_from_demo) {
                main_menu_dialog();
                copy_kid_to_char();
            } break;
        case 0x1f00: message = toggle_sound() ? sound_on_message : sound_off_message; break;
        case 0x2200:
            if (!IsValidTimeForDialog()) break;
            if (!IsValidTimeForSaveGame()) break;
            save_game_dialog(); copy_kid_to_char(); break;
        case 0x2300:
        /* This case starts a separate CodeView record. */
        shortcut_dialog:
            if (IsValidTimeForDialog()) show_hall_records(1); break;
        case 0x2400: message = joystick_status(); break;
        case 0x2500:
            /* The keyboard label follows the audio and joystick strings. */
            message = keyboard_mode_message; control_mode = 0; break;
        case 0x2600:
            if (!restart_from_demo) {
                if (IsValidTimeForDialog()) load_game_dialog();
                copy_kid_to_char();
            } else {
                demo_pending = 1;
            }
            break;
        case 0x2f00: message = title; break;
        case 0x3100:
            if (LevelIdx <= 3 || command_line_enabled) { if (LevelIdx == 14 && command_line_enabled) {
                    requested_level = 1;
                } else {
                    requested_level = LevelIdx + 1;
                    if (!command_line_enabled && minutes_remaining > 15) {
                        minutes_remaining = 15; ticks_remaining = 719; }
                }
                dispatch_sound(0);
            } break;
        case 0x3200: message = toggle_ambient(); break;
        }

        if (command_line_enabled) handle_debug_input(key);
        if (message) { draw_message(message); message_ticks = message_duration = 24;
        }
    }
    if (restart && restart_blocked == 0 &&
        (Room.number != 4 || Level.level_number != 13 || !in_special_room())) {
        demo_changed = 1; dispatch_sound(0);
    }
    return key;
}



extern CHARREC RoomOpp[5];

extern int minutes_remaining, message_ticks, message_duration, countdown_pending;
extern int debug_alt_view, debug_reload_room;
extern int debug_show_objects, debug_show_messages, debug_show_buildings;
extern int debug_show_collisions, debug_show_door_links, debug_show_meters;
extern int debug_kid_x, debug_kid_y, player_input_disabled;


extern int far cdecl sprintf(char *buffer, const char *format, ...);
extern void far pascal draw_message(const char *message);
extern void far pascal LoadCustomRoomData(unsigned char room);
extern void far pascal STROKE_PORT_RECT(const char far *asset);
extern void far pascal ToggleInvertState(void);
extern void far pascal AddSong(int value);
extern void far pascal AddSound(int value);
extern unsigned char CutRoom;
extern void far pascal ChangeMeters(void);
extern void far pascal CheckMeters(void);
extern void far pascal copy_kid_to_char(void);
extern char far pascal BoostMeter(void);
extern void far pascal GoWeightless(void);
extern void far pascal load_actor_slot(signed char opponent);
extern void far pascal save_actor_slot(void);
extern void far pascal JumpSeq(int sequence);
extern void far pascal bones_fall(void);
extern void far pascal set_skeleton_timer(signed char opponent, int value);
extern void far pascal CheckShadowmanCreate(void);
extern void far pascal set_palette(int duration, int range, unsigned char color, unsigned char mode);
extern void far pascal DrawOppMeter(signed char id, int value, int flags);
extern void far pascal clear_message(int restore);
extern int far pascal toggle_recording_player(void);
extern int far pascal AddCharX(int amount);

extern char GamePlayRect[];
extern void *cdecl memcpy(void *destination, const void *source, unsigned int size);
#pragma intrinsic(memcpy)

static struct {
    char room_number_format[8];
    char debug_cheat_message[30];
    char recording_on_message[10];
    char recording_off_message[11];
    char say_cheese_option[10];
    char kid_x_saved_message[12];
    char kid_x_restored_message[15];
    char kid_y_saved_message[12];
    char kid_y_restored_message[15];
} debug_messages = {
    "Room %d",
    "Called via program cheat key.",
    "PLAYER ON",
    "PLAYER OFF",
    "SAYCHEESE",
    "Kid.X saved",
    "Kid.X restored",
    "Kid.Y saved",
    "Kid.Y restored",
};

void far pascal handle_debug_input(unsigned int key)
{
    char text[80];
    char *message = 0;
    signed char index, count;

    switch (key) {
    case '+':
        ++minutes_remaining; message_ticks = message_duration = 0;
        countdown_pending = 1; break;
    case '-':
        if (minutes_remaining > 1) --minutes_remaining;
        message_ticks = message_duration = 0;
        countdown_pending = 1; break;
    case 'B':
        debug_alt_view = !debug_alt_view;
        if (debug_alt_view) STROKE_PORT_RECT(GamePlayRect);
        else {
            debug_reload_room = 1; LoadCustomRoomData(Room.number);
        }
        index = 0; count = Level.room_objects[Room.number - 1].opponent_count;
        if (index < count) do {
            RoomOpp[index].previous_x = 0;
        } while (++index < count);
        break;
    case 'I': ToggleInvertState(); break;
    case 'K':
        Char.life_delta = -1; AddSound(31);
        Kid = Char; ChangeMeters();
        CheckMeters(); copy_kid_to_char();
        if (Char.life_points == 0) JumpSeq(71);
        Char.life_delta = 0; break;
    case 'R':
        sprintf(text, debug_messages.room_number_format, (unsigned)Room.number); draw_message(text);
        message_ticks = message_duration = 24; break;
    case 'S':
        if (Level.environment_kind == 2 || Level.environment_kind == 6) {
            player_input_disabled = 7; Char.sequence = 5;
            CheckShadowmanCreate();
            if (Level.environment_kind == 6) set_palette(2000, 48, 16, 1);
            else
                set_palette(2000, 48, 16, 0);
        }
        break;
    case 'T':
        AddSong(101); BoostMeter(); Kid = Char; break;
    case 'W': GoWeightless(); break;
    case 'g':
        Kid = Char;
        if (Char.opponent_index != -1) { load_actor_slot(Char.opponent_index); ++Char.life_initial;
            Char.life_delta = 1; save_actor_slot();
            ChangeMeters(); CheckMeters();
        }
        copy_kid_to_char(); break;
    case 'k':
        Char.opponent_index = -1; Kid = Char;
        index = 0; count = Level.room_objects[Room.number - 1].opponent_count;
        if (index < count) do {
            load_actor_slot(index); Char.alive = 0;
            Char.life_delta = -Char.life_points;
            if (Level.preferred_opponent == 2) { bones_fall();
            } else if (Char.character_kind == 10) { set_skeleton_timer(index, 480); JumpSeq(107);
                Char.field_0f = 1;
            } else if (Level.preferred_opponent == 5 || Level.preferred_opponent == 6) {
                Char.direction = ~Kid.direction; Char.life_points = 0;
                JumpSeq(154);
            } else {
                JumpSeq(85);
            }
            save_actor_slot();
        } while (++index < count);
        copy_kid_to_char(); break;
    case 'r':
        if (Kid.alive > 0) {
            restart_blocked = 20;
            Kid.alive = -1;
            clear_message(1);
        }
        break;
    case 'v':
        text[0] = 0;
        break;
    case 0x2000:
        show_dialog(0, debug_messages.debug_cheat_message, 0);
        break;
    case 0x3b00:
        debug_show_objects = !debug_show_objects;
        break;
    case 0x3c00:
        debug_show_messages = !debug_show_messages;
        break;
    case 0x3d00:
        message = (char *)(toggle_recording_player() ? debug_messages.recording_on_message : debug_messages.recording_off_message);
        break;
    case 0x3f00:
        debug_show_buildings = !debug_show_buildings;
        break;
    case 0x4000:
        debug_show_collisions = !debug_show_collisions;
        break;
    case 0x4100:
        debug_show_door_links = !debug_show_door_links;
        break;
    case 0x4200:
        debug_show_meters = !debug_show_meters;
        break;
    }

    if (find_option(debug_messages.say_cheese_option)) {
        switch (key) {
        case 0x4800: CutRoom = Room.above; break;
        case 0x4b00: CutRoom = Room.left; break;
        case 0x4d00: CutRoom = Room.right; break;
        case 0x5000: CutRoom = Room.below; break;
        }
        if (CutRoom)
            DrawOppMeter(-1, 0, 8);
    }

    if (control_mode) {
        switch (key) {
        case '1':
            copy_kid_to_char();
            debug_kid_x = Char.x;
            Kid = Char;
            memcpy(text, debug_messages.kid_x_saved_message, sizeof(debug_messages.kid_x_saved_message));
            message = text;
            break;
        case '2':
            copy_kid_to_char();
            Char.x = debug_kid_x;
            Kid = Char;
            memcpy(text, debug_messages.kid_x_restored_message, sizeof(debug_messages.kid_x_restored_message));
            message = text;
            break;
        case '3':
            copy_kid_to_char();
            debug_kid_y = Char.y;
            Kid = Char;
            memcpy(text, debug_messages.kid_y_saved_message, sizeof(debug_messages.kid_y_saved_message));
            message = text;
            break;
        case '4':
            copy_kid_to_char();
            Char.y = debug_kid_y;
            Kid = Char;
            memcpy(text, debug_messages.kid_y_restored_message, sizeof(debug_messages.kid_y_restored_message));
            message = text;
            break;
        case 0x4700:
            copy_kid_to_char();
            if (Char.direction == -1)
                Char.x = AddCharX(32);
            else
                Char.x = AddCharX(-32);
            Kid = Char;
            break;
        case 0x4900:
            copy_kid_to_char();
            if (Char.direction == 0)
                Char.x = AddCharX(32);
            else
                Char.x = AddCharX(-32);
            Kid = Char;
            break;
        case 0x4b00:
            copy_kid_to_char();
            if (Char.direction == -1)
                Char.x = AddCharX(1);
            else
                Char.x = AddCharX(-1);
            Kid = Char;
            break;
        case 0x4d00:
            copy_kid_to_char();
            if (Char.direction == 0)
                Char.x = AddCharX(1);
            else
                Char.x = AddCharX(-1);
            Kid = Char;
            break;
        case 0x4f00:
            copy_kid_to_char();
            if (Char.direction == -1)
                Char.x = AddCharX(5);
            else
                Char.x = AddCharX(-5);
            Kid = Char;
            break;
        case 0x5100:
            copy_kid_to_char();
            if (Char.direction == 0)
                Char.x = AddCharX(5);
            else
                Char.x = AddCharX(-5);
            Kid = Char;
            break;
        }
    }
    if (message) {
        draw_message(message);
        message_ticks = message_duration = 24;
    }
}

#undef debug_char
#undef debug_kid

extern char one_second_left[], seconds_left[], minutes_left[], time_expired[];

extern signed char sound_volume;
extern void far pascal SET_SOUND_LEVEL(signed char level);
extern void far pascal SaveOptionsToDisk(void);

int far pascal toggle_sound(void)
{
    int enabled;

    if (sound_volume == 0) {
        SET_SOUND_LEVEL(15);
        enabled = 1;
    } else {
        SET_SOUND_LEVEL(0);
        enabled = 0;
    }
    SaveOptionsToDisk();
    return enabled;
}

extern int control_mode;
#pragma pack(1)
typedef struct {
    int x, y;
    int center_x, center_y;
    signed char button;
} GameportState;
#pragma pack()
extern GameportState gameport_state;
extern int far pascal check_game_port(void);
extern void *cdecl memset(void *destination, int value, unsigned int size);
#pragma intrinsic(memset)

char *far pascal joystick_status(void)
{
    char *message;
    if ((active_sound_channels & 1) && configuration->midi_type == 32) {
        message = joystick_unavailable_message;
    } else {
        configuration->control_mode = 2;
        if (check_game_port()) {
            message = joystick_mode_message;
            configuration->control_mode = control_mode = 2;
        } else {
            message = joystick_missing_message;
            configuration->control_mode = control_mode = 0;
            memset(&gameport_state, 0, sizeof(gameport_state));
        }
    }
    return message;
}

extern void far pascal SetSideRoomInfo(void);
extern void far pascal select_room_arrays(unsigned char room);
extern unsigned char far pascal FirstInRow(int tile);
extern unsigned char far pascal select_trob_cell(int tile);
extern void far pascal CheckSpikesJammed(unsigned char room, int tile);
extern void far pascal trigger_flask(unsigned char room, signed char tile);
extern void far pascal trigger_torch(unsigned char room, signed char tile);
extern void far pascal trigger_lava(unsigned char room, signed char tile);
extern void far pascal add_trob(unsigned char room, signed char tile,
                                      signed char active, unsigned char type);
extern int far pascal trigger_stone_at(unsigned char room, signed char tile, signed char delay);
extern void far pascal trigger_bridge_floor(unsigned char room, signed char tile);
extern void far pascal trigger_crystal(unsigned char room, signed char tile);
extern void far pascal trigger_gargoyle(unsigned char room, signed char tile);
#pragma pack(1)
typedef struct {
    unsigned char bytes[4];
} TileInfo;

typedef struct {
    unsigned char tile;
    TileInfo info;
} Tile;
#pragma pack()
extern TileInfo BlueSpec;
extern Tile left_tiles[];

void far pascal add_room_trobs(void)
{
    int index;
    unsigned char room;
    signed char tile;
    unsigned char kind;

    SetSideRoomInfo();
    room = Room.number;
    index = 0;
    tile = 0;
    do {
        if (index < 30) {
            kind = select_trob_cell(index);
        } else {
            if (index == 30) {
                if (Room.left) {
                    room = Room.left;
                    select_room_arrays(room);
                } else {
                    index += 3;
                }
            }
            if (index == 33) {
                if (!Room.below)
                    break;
                room = Room.below;
                select_room_arrays(room);
            }
            if (index < 33) {
                tile = FirstInRow(index - 30) + 9;
                kind = left_tiles[index - 30].tile;
            } else {
                tile = index - 33;
                kind = select_trob_cell(tile);
            }
        }

        switch (kind) {
        default:
            if (room == 6 && Level.environment_kind == 6 && (BlueSpec.bytes[1] & 16)) {
                trigger_crystal(room, tile);
            } else if ((room == 7 || room == 8) &&
                       Level.environment_kind == 6 && (BlueSpec.bytes[1] & 16)) {
                trigger_gargoyle(room, tile);
            }
            break;
        case 2:
            CheckSpikesJammed(room, tile);
            break;
        case 10:
            if (index < 33)
                trigger_flask(room, tile);
            break;
        case 19:
        case 32:
            trigger_torch(room, tile);
            break;
        case 23:
            if (index < 33)
                trigger_lava(room, tile);
            break;
        case 28:
        case 29:
        case 31:
        case 37:
        case 38:
        case 39:
        case 40:
        case 43:
            add_trob(room, tile, 1, kind);
            break;
        case 30:
            trigger_stone_at(room, tile, 3);
            break;
        case 44:
            trigger_bridge_floor(room, tile);
            break;
        }
        ++tile;
    } while (++index < 43);
    select_room_arrays(Room.number);
}

extern signed char timer_disabled;
extern int minutes_remaining, countdown_pending;
extern unsigned int ticks_remaining;
extern int message_ticks, message_duration, deferred_room, LastResume;
extern void far pascal draw_message(const char *message);
extern int far cdecl sprintf(char *buffer, const char *format, ...);
extern char *cdecl strcpy(char *destination, const char *source);
#pragma intrinsic(strcpy)

void far pascal KeepTime(void)
{
    char text[40];
    unsigned int seconds;

    if (timer_disabled >= 0 && Kid.alive < 0 && minutes_remaining != 0) {
        if (--ticks_remaining == 0) {
            ticks_remaining = 719;
            if (--minutes_remaining != 0 &&
                (minutes_remaining < 5 || minutes_remaining % 5 == 0))
                countdown_pending = 1;
        } else if (minutes_remaining == 1 && ticks_remaining % 12 == 0) {
            countdown_pending = 1;
            message_ticks = message_duration = 0;
        }
    }

    if (message_ticks)
        return;
    if (deferred_room) {
        LastResume = deferred_room;
        deferred_room = 0;
        return;
    }
    if (countdown_pending) {
        if (minutes_remaining > 0) {
            if (minutes_remaining == 1) {
                seconds = (ticks_remaining + 1) / 12;
                if (seconds == 1)
                    strcpy(text, one_second_left);
                else
                    sprintf(text, seconds_left, seconds);
            } else {
                sprintf(text, minutes_left, minutes_remaining);
            }
            draw_message(text);
        } else {
            draw_message(time_expired);
        }
        message_ticks = message_duration = 24;
        countdown_pending = 0;
    }
}

extern int cut_required;
extern unsigned char bank_opponents[2];

extern void far pascal get_rooms(void);
extern int far pascal IsLevelStartWithNISScreen(void);
extern void far pascal save_game_play_palette(void);
extern void far pascal LoadCustomRoomData(unsigned char room);
extern void far pascal preload_room_data(unsigned char room);
extern void far pascal initialize_room_actors(void);
extern signed char far pascal pick_hidden_actor(signed char mode);
extern void far pascal copy_kid_to_char(void);
extern void *cdecl memset(void *destination, int value, unsigned int size);
#pragma intrinsic(memset)


int far pascal PrepCut(int keep_screen)
{
    signed char result = -2;
    if (CutRoom && Room.number != CutRoom) {
        Room.number = CutRoom;
        get_rooms();
        if (!keep_screen || !IsLevelStartWithNISScreen())
            save_game_play_palette();
        LoadCustomRoomData(Room.number);
        cut_required = 1;
        copy_kid_to_char();
        add_room_trobs();
        memset(bank_opponents, 0, sizeof(bank_opponents));
        preload_room_data(Room.number);
        initialize_room_actors();
        if (!Level.room_objects[Room.number - 1].opponent_count)
            DrawOppMeter(-1, 0, 8);
        else
            Kid.opponent_index = pick_hidden_actor(1);
    }
    return result;
}

extern void far pascal copy_kid_to_char(void);
extern void far pascal DrawKidMeter(int previous, int current);
extern unsigned long FrameNum;
extern unsigned int ticks_remaining;


/* Increment the signed state byte up to 12, then refresh the step delta. */
char far pascal UPDATE_STEP_DELTA(void);

char far pascal BoostMeter(void)
{
    int next = (signed char)Char.life_initial + 1;
    Char.life_initial = (next > 12) ? 12 : next;
    return UPDATE_STEP_DELTA();
}

char far pascal UPDATE_STEP_DELTA(void)
{
    Char.life_delta = Char.life_initial - Char.life_points;
    return Char.life_delta;
}

void far pascal CheckMeters(void)
{
    copy_kid_to_char();
    if (Char.life_delta)
        DrawKidMeter((signed char)Char.life_points, (signed char)Char.life_initial);
    if (Char.life_points == 1) {
        if (FrameNum & 1)
            DrawKidMeter(1, 0);
        else
            DrawKidMeter(0, 1);
    }
    Kid = Char;
    if (Kid.opponent_index != -1) {
        load_actor_slot(Kid.opponent_index);
        if (Char.life_delta && !((Char.character_kind == 7 || Char.character_kind == 8) && Char.sequence_group == 2))
            DrawOppMeter(Char.slot, (signed char)Char.life_points, (signed char)Char.life_initial);
        if (Char.life_points == 1 && !((Char.character_kind == 7 || Char.character_kind == 8) && Char.sequence_group == 2)) {
            if (ticks_remaining & 1)
                DrawOppMeter(Char.slot, 1, (signed char)Char.life_initial);
            else
                DrawOppMeter(Char.slot, 0, (signed char)Char.life_initial);
        }
    }
}

extern void far pascal CheckShadowPaletteChange(void);


void far pascal ChangeMeters(void)
{
    int hp;
    signed char opponent, count;
    CHARREC *meter;

    if (Kid.life_delta) {
        if (Kid.character_kind == 1 && Level.environment_kind == 6)
            CheckShadowPaletteChange();
        hp = Kid.life_points + Kid.life_delta;
        hp = hp < 0 ? 0 : hp;
        hp = hp > Kid.life_initial ? Kid.life_initial : hp;
        Kid.life_points = hp;
    }
    opponent = 0;
    count = Level.room_objects[Room.number - 1].opponent_count;
    if (count > 0) {
        do {
            meter = &RoomOpp[opponent];
            if (meter->life_delta) {
                hp = meter->life_points + meter->life_delta;
                hp = hp < 0 ? 0 : hp;
                hp = hp > meter->life_initial ? meter->life_initial : hp;
                meter->life_points = hp;
            }
        } while (++opponent < count);
    }
}


extern signed char joystick[3];
extern int Pause;
extern char paused_message[];

extern void far pascal read_controls(void);
extern unsigned int far pascal prince_poll(void);
extern void far pascal draw_message(const char *message);
extern void far pascal dispatch_sound(int mode);
extern void far pascal wait_key(void);
extern void far pascal clear_message(int restore);
extern void *cdecl memset(void *buffer, int value, unsigned int size);
#pragma intrinsic(memset)

int far pascal Input(void)
{
    int result;

    CutRoom = 0;
    memset(joystick, 0, sizeof(joystick));
    read_controls();
    result = prince_poll();
    if (Pause) {
        Pause = 0;
        draw_message(paused_message);
        dispatch_sound(0);
        wait_key();
        clear_message(1);
    }
    if (result == 0)
        result = joystick[2];
    return result;
}


extern int control_mode;
extern unsigned char key_down[];
extern void far pascal POLL_GAME_PORT(void);

#define keyboard_flags (*(unsigned char far *)0x00400017UL)

void far pascal read_gameport_controls(void);
void far pascal read_keyboard_controls(void);

/* 105a */ void far pascal read_controls(void)
{
    if (control_mode == 2)
        read_gameport_controls();
    else
        read_keyboard_controls();
}

void far pascal read_gameport_controls(void)
{
    int displacement;
    int threshold;

    POLL_GAME_PORT();
    threshold = *(volatile int *)&gameport_state.center_y / 2;
    displacement = gameport_state.y - gameport_state.center_y;
    if (displacement > threshold)
        joystick_y = 1;
    else if (displacement < -threshold)
        joystick_y = -1;

    threshold = *(volatile int *)&gameport_state.center_x / 2;
    displacement = gameport_state.x - gameport_state.center_x;
    if (displacement > threshold)
        joystick_x = 1;
    else if (-threshold > displacement)
        joystick_x = -1;

    joystick_button = -gameport_state.button;
    if (joystick_button < -2)
        joystick_button = -1;
}

/* 10d8 */ void far pascal read_keyboard_controls(void)
{
    if (keyboard_flags & 8)
        return;

    if (!(key_down[0x48] || key_down[0x17] || key_down[0x12] ||
          key_down[0x47] || key_down[0x16] || key_down[0x11] ||
          key_down[0x49] || key_down[0x18] || key_down[0x13])) {
        if (key_down[0x50] || key_down[0x25] || key_down[0x20] ||
            key_down[0x4c] || key_down[0x33] || key_down[0x2e] ||
            key_down[0x4f] || key_down[0x32] || key_down[0x2d] ||
            key_down[0x51] || key_down[0x34] || key_down[0x2f])
            joystick_x = 1;
    } else {
        joystick_x = -1;
    }

    if (!(key_down[0x4b] || key_down[0x24] || key_down[0x1f] ||
          key_down[0x47] || key_down[0x16] || key_down[0x11] ||
          key_down[0x4f] || key_down[0x32] || key_down[0x2d])) {
        if (key_down[0x4d] || key_down[0x26] || key_down[0x21] ||
            key_down[0x49] || key_down[0x18] || key_down[0x13] ||
            key_down[0x51] || key_down[0x34] || key_down[0x2f])
            joystick_y = 1;
    } else {
        joystick_y = -1;
    }

    keyboard_flags &= 0x0f;
    if (!(keyboard_flags & 3) && !key_down[0x53]) {
        if ((keyboard_flags & 4) || key_down[0x52])
            joystick_button = -2;
    } else {
        joystick_button = -1;
    }
}

extern int far pascal POLL_DOS_KEY(void);
extern void far pascal post_pointer_event(int type, int data, int key);

void far pascal clear_keyboard(void)
{
    int key;

    if (control_mode == 0) {
        do {
            key = POLL_DOS_KEY();
            if (key)
                post_pointer_event(8, 0, key);
        } while (key);
    }
}

/* PRINCE.C, resident DOS entries 12de through 133f. */
extern void far pascal dispatch_sound(int mode);
extern int far pascal init_port(void near *port, void far *buffer);
extern void far pascal clear_peels(void);
extern unsigned char ScreenPort[];
extern unsigned char game_buffer[];
extern int Graf;
extern char LevelIdx;
extern int LastResume;

extern unsigned int Invert;
extern unsigned int MODE_CHANGED;

typedef struct {
    int top, left, bottom, right;
} Rectangle;


extern unsigned char ScreenPort[];
extern unsigned int source_port;
extern void far pascal MEASURE_TEXT(void near *port, unsigned int source,
                                     const Rectangle far *source_rect,
                                     const Rectangle far *dest_rect, int flags);

void far pascal PrinceCopy(const Rectangle far *rect)
{
    Rectangle flipped;
    Rectangle far *copy_rect;

    if (Invert != 0) {
        copy_rect = &flipped;
        *copy_rect = *rect;
        copy_rect->top = 192 - rect->bottom;
        flipped.bottom = 192 - rect->top;
    } else {
        copy_rect = (Rectangle far *)rect;
    }

    MEASURE_TEXT(ScreenPort, source_port, copy_rect, copy_rect, 0);
}

void far pascal ToggleInvertState(void)
{
    if (Invert != 0) {
        Invert = 0;
        AddSong(154);
    } else {
        Invert = 1080;
        AddSong(153);
    }
    MODE_CHANGED = 1;
}

/* Restrict the step value, then request mode 105 unless already pending. */
extern unsigned int RESTRICTED_STEP;
extern unsigned int death_sequence_primary;
extern int fall_transition;

void far pascal GoWeightless(void)
{
    RESTRICTED_STEP = 228;
    AddSong(105);
    dispatch_sound(death_sequence_primary + 10000);
    fall_transition = -1;
}

/* 1316 */ void far pascal ResetData(void)
{
    dispatch_sound(0);
    Graf = init_port(ScreenPort, game_buffer);
    clear_peels();
    LevelIdx = -1;
    LastResume = 0;
}

/* Window callback that ignores its arguments. */
void far pascal ignore_window_action(void *window, int action)
{
}
