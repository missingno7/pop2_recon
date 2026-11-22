/* PRINCE.C whole-file working draft, resident DOS root [0000,1340).
 * The separately verified contributions are listed in submission.json.
 */

/* Resident PRINCE.C startup. Externs denote symbols in other contributions. */

extern const char *resource_files[2];
extern unsigned char restart_requested;
extern char selected_level;
extern char current_level;
extern char restart_level;
extern int command_line_enabled;
extern int skip_intro;
extern int skip_opening;
extern int restart_from_demo;

extern int demo_level;
extern int demo_changed;
extern int demo_pending;
extern int minutes_remaining;
extern char watch_memory_option[];
extern char restart_message[];
/* These three adjacent option strings are used by the startup routines. */
extern struct {
    char nis[4];
    char tree[5];
    char level[7];
} startup_options;

extern void far pascal platform_setup(void);
extern void far pascal save_options(void);
extern void far pascal OpenRsrcVerify(const char *filename);
extern int far pascal Rnd(int seed);
extern char near *far pascal find_option(const char far *option);
extern int far cdecl atoi(const char near *text);
extern void far pascal log_message(const char near *message);
extern void far pascal write_message(const char near *message);
extern void far pascal show_dialog(const char near *title, const char far *message, int flags);
extern char far pascal Start(char level);
extern int far pascal PlayNIS(int sequence);
extern void far pascal finish_restart(int level);
extern void far pascal quit_game(int status, int flags);
extern void far pascal clear_picture(long color);
extern void far pascal clear_display(long color);
extern int far pascal opening_disabled(void);
extern int far pascal opening_input(int mode);
extern void far pascal TurnRecordedGamePlayerOff(void);
extern void far pascal TurnRecordedGamePlayerOn(void);
extern void far pascal finish_opening(void);
extern int far pascal poll_demo_state(void);
extern void far pascal update_display(int mode);
extern int far pascal InitPort(void near *port, void far *buffer);
extern void far pascal ZeroPeels(void);

extern unsigned char game_port[];
extern unsigned char game_buffer[];
extern int game_port_id;
extern int room_number;

void far pascal ResetGame(void);
void far pascal read_start_level(void);
char far pascal PlayOpeningSequence(void);
char far pascal poll_and_clear_pending_state(void);
void far pascal ResetData(void);

/* 0000 */ void far cdecl main(void)
{
    const char **file;

    platform_setup();
    save_options();
    read_start_level();
    file = resource_files;
    do {
        OpenRsrcVerify(*file);
        ++file;
    } while (file < resource_files + 2);
    Rnd(1);
    restart_requested = 0;
    ResetGame();
}

/* 0036 */ void far pascal ResetGame(void)
{
    char level = selected_level;

    for (;;) {
        if (command_line_enabled) {
            if (find_option(watch_memory_option)) {
                log_message(restart_message);
                write_message(restart_message);
                show_dialog(0, restart_message, 0);
            }
            if (find_option(startup_options.nis) || find_option(startup_options.tree))
                break;
        }
        ResetData();
        if (selected_level == 0) {
            level = PlayOpeningSequence();
        } else {
            level = Start(level);
            if (restart_from_demo)
                restart_level = level = 0;
            if (minutes_remaining == 0) {
                selected_level = level = 0;
            } else if (level > 14) {
                PlayNIS(11);
                finish_restart(minutes_remaining);
                selected_level = level = 0;
            } else if (selected_level != 0 && level == 0) {
                selected_level = level = restart_level;
            }
        }
    }
    for (;;)
        quit_game(0, 0);
}

#pragma optimize("e", off)
/* 0116 */ void far pascal read_start_level(void)
{
    const char *option;

    selected_level = 0;
    if (command_line_enabled) {
        option = find_option(startup_options.level);
        if (option == 0)
            return;
        selected_level = (char)atoi(option + 5);
        if (selected_level < 1 || selected_level > 14)
            selected_level = 1;
    }
}

#pragma optimize("e", on)
/* 014e */ char far pascal PlayOpeningSequence(void)
{
    int result;
    char level;

    clear_picture(0x00ffffffL); clear_display(0x00ffffffL);
    result = 1;
    if (!skip_intro) {
        if (skip_opening)
            result = 2;
        else if (opening_disabled() || opening_input(0))
            result = 2;
    } else {
        skip_intro = 0;
    }
    if (result != 2)
        result = PlayNIS(7);
next_opening_clip:
    if (result != 2) result = PlayNIS(4);
    if (result != 2)
        result = PlayNIS(8);
    if (result == 2) {
        selected_level = 1;
        skip_intro = 1;
        TurnRecordedGamePlayerOff();
    } else {
        TurnRecordedGamePlayerOn();
    }
    level = selected_level;
    finish_opening();
    if (demo_pending)
        level = poll_and_clear_pending_state();
    return level;
}

/* 0210 */ char far pascal poll_and_clear_pending_state(void)
{
    char level = selected_level;

    if (poll_demo_state() > 0) {
        level = selected_level = current_level;
        demo_changed = 0;
    }
    demo_pending = 0;
    return level;
}


typedef struct {
    char room;
    char unknown1[5];
    char kind;
    char unknown7[29];
    int sequence;
    char rest[26];
} CharacterState;

extern CharacterState Char;
extern CharacterState Kid;

extern unsigned char current_room;
extern signed char joystick[3];
#define joystick_y (joystick[0])
#define joystick_x (joystick[1])
#define joystick_button (joystick[2])
extern signed char kid_life, timer_disabled;
extern int paused;
extern int minutes_remaining, countdown_pending, restart_blocked;
extern int requested_level, control_mode, message_ticks, message_duration;
extern unsigned int ticks_remaining;
extern const char sound_on[], sound_off[], keyboard_mode[], title[];

extern unsigned int far pascal read_key_code(void);
extern int far pascal dialog_allowed(void);
extern int far pascal show_game_settings(void);
extern void far pascal apply_settings(void);
extern void far pascal show_game_help(void);
extern void far pascal CtrlPlayer(void);
extern int far pascal toggle_sound(void);
extern int far pascal opening_input(int mode);
extern char *far pascal joystick_status(void);
extern int far pascal poll_demo_state(void);
extern const char *far pascal toggle_music(void);
extern int far pascal recording_enabled(void);
extern void far pascal DrawMessage(const char *message);
extern void far pascal update_display(int mode);
extern int far pascal in_special_room(void);
extern void far pascal clear_keyboard(void);
extern void far pascal handle_debug_input(unsigned int key);

unsigned int far pascal prince_poll(void)
{
    const char *message = 0;
    int restart = 0;
    unsigned int key = read_key_code();

    if (joystick_button || key) {
        if ((minutes_remaining && kid_life > 6) || restart_from_demo) {
            restart = 1;
            if (restart_from_demo)
                skip_opening = 1;
        }
    }

    if (key) {
        clear_keyboard();
        switch (key) {
        case 0x1b:
            paused = 1;
            break;
        case 0x20:
            if (timer_disabled >= 0)
                countdown_pending = 1;
            break;
        case 0x1300:
            selected_level = 0;
            timer_disabled = -1;
            goto request_restart;
        case 0x1800:
            if (dialog_allowed()) {
                show_game_help();
                CtrlPlayer();
            }
            break;
        case 0x1e00:
        request_restart:
            if (!recording_enabled())
                restart = 1;
            break;
        case 0x1f00:
            message = toggle_sound() ? sound_on : sound_off;
            break;
        case 0x2200:
            if (dialog_allowed() && show_game_settings()) {
                apply_settings();
                CtrlPlayer();
            }
            break;
        case 0x2300:
            if (dialog_allowed())
                opening_input(1);
            break;
        case 0x2400:
            message = joystick_status();
            break;
        case 0x2500:
            message = keyboard_mode;
            control_mode = 0;
            break;
        case 0x2600:
            if (dialog_allowed())
                poll_demo_state();
            CtrlPlayer();
            break;
        case 0x2f00:
            message = title;
            break;
        case 0x3100:
            if (current_level <= 3 || command_line_enabled) {
                if (current_level == 14 && command_line_enabled) {
                    requested_level = 1;
                } else {
                    requested_level = current_level + 1;
                    if (!command_line_enabled && minutes_remaining > 15) {
                        minutes_remaining = 15;
                        ticks_remaining = 719;
                    }
                }
                update_display(0);
            }
            break;
        case 0x3200:
            message = toggle_music();
            break;
        }

        if (command_line_enabled)
            handle_debug_input(key);
        if (message) {
            DrawMessage(message);
            message_ticks = message_duration = 24;
        }
    }
    if (restart && restart_blocked == 0 &&
        (current_room != 4 || restart_level != 13 || !in_special_room())) {
        demo_changed = 1;
        update_display(0);
    }
    return key;
}


typedef struct {
    signed char room, direction;
    int x, y;
    signed char kind, unk7[8], changed, unk10, unk11;
    signed char hp, max_hp, hp_delta, trap_progress;
    signed char rest[3], frame, unk1a[10];
    int sequence, next_sequence;
    signed char unk28[17], linked, unk3a[6];
} DebugActor;

typedef struct {
    signed char opponent_count;
    unsigned char remainder[0x73];
} RoomSettingsRow;

extern DebugActor opponents[];

#define debug_char (*(DebugActor near *)&Char)
#define debug_kid  (*(DebugActor near *)&Kid)
extern RoomSettingsRow room_settings[];
extern unsigned char current_room, left_room, above_room, right_room, below_room;
extern unsigned char next_room, current_level_number, stage_type;
extern int minutes_remaining, message_ticks, message_duration, countdown_pending;
extern int debug_alt_view, debug_reload_room;
extern int debug_show_objects, debug_show_messages, debug_show_buildings;
extern int debug_show_collisions, debug_show_door_links, debug_show_meters;
extern int debug_kid_x, debug_kid_y, player_input_disabled;
extern signed char kid_life;

extern int far cdecl sprintf(char *buffer, const char *format, ...);
extern void far pascal DrawMessage(const char *message);
extern void far pascal LoadCustomRoomData(unsigned char room);
extern void far pascal update_alt_view(const char far *asset);
extern void far pascal debug_room_message(const char *title, const char far *text, int flags);
extern void far pascal ToggleInvertState(void);
extern void far pascal request_sound(int value);
extern void far pascal ChangeMeters(void);
extern void far pascal CheckMeters(void);
extern void far pascal CtrlPlayer(void);
extern char far pascal BoostMeter(void);
extern void far pascal GoWeightless(void);
extern void far pascal select_opponent(signed char opponent);
extern void far pascal write_back_opponent(void);
extern void far pascal start_actor_animation(int sequence);
extern void far pascal start_skeleton_animation(void);
extern void far pascal start_mouse_animation(signed char opponent, int value);
extern void far pascal cheat_switch_room(void);
extern void far pascal schedule_room_effect(int duration, int range, unsigned char color, unsigned char mode);
extern void far pascal highlight_room(signed char id, int value, int flags);
extern void far pascal ClrMessage(int restore);
extern int far pascal toggle_recording(void);
extern int far pascal move_kid_x(int amount);

extern char alternate_view_asset[];
extern void *cdecl memcpy(void *destination, const void *source, unsigned int size);
#pragma intrinsic(memcpy)

extern const char room_number_format[8];
extern const char debug_cheat_message[30];
extern const char recording_on_message[10];
extern const char recording_off_message[11];
extern const char say_cheese_option[10];
extern const char kid_x_saved_message[12];
extern const char kid_x_restored_message[15];
extern const char kid_y_saved_message[12];
extern const char kid_y_restored_message[15];
void far pascal handle_debug_input(unsigned int key)
{
    char text[80];
    char *message = 0;
    signed char index, count;

    switch (key) {
    case '+':
        ++minutes_remaining;
        message_ticks = message_duration = 0;
        countdown_pending = 1;
        break;
    case '-':
        if (minutes_remaining > 1)
            --minutes_remaining;
        message_ticks = message_duration = 0;
        countdown_pending = 1;
        break;
    case 'B':
        debug_alt_view = !debug_alt_view;
        if (debug_alt_view)
            update_alt_view(alternate_view_asset);
        else {
            debug_reload_room = 1;
            LoadCustomRoomData(current_room);
        }
        index = 0;
        count = room_settings[current_room].opponent_count;
        if (index < count) do {
            opponents[index].next_sequence = 0;
        } while (++index < count);
        break;
    case 'I':
        ToggleInvertState();
        break;
    case 'K':
        debug_char.hp_delta = -1;
        request_sound(31);
        Kid = Char;
        ChangeMeters();
        CheckMeters();
        CtrlPlayer();
        if (debug_char.hp == 0)
            start_actor_animation(71);
        debug_char.hp_delta = 0;
        break;
    case 'R':
        sprintf(text, room_number_format, (unsigned)current_room);
        DrawMessage(text);
        message_ticks = message_duration = 24;
        break;
    case 'S':
        if (current_level_number == 2 || current_level_number == 6) {
            player_input_disabled = 7;
            *(int near *)&debug_char.frame = 5;
            cheat_switch_room();
            if (current_level_number == 6)
                schedule_room_effect(2000, 48, 16, 1);
            else
                schedule_room_effect(2000, 48, 16, 0);
        }
        break;
    case 'T':
        request_sound(101);
        BoostMeter();
        Kid = Char;
        break;
    case 'W':
        GoWeightless();
        break;
    case 'g':
        Kid = Char;
        if (debug_char.linked != -1) {
            select_opponent(debug_char.linked);
            ++debug_char.max_hp;
            debug_char.hp_delta = 1;
            write_back_opponent();
            ChangeMeters();
            CheckMeters();
        }
        CtrlPlayer();
        break;
    case 'k':
        debug_char.linked = -1;
        Kid = Char;
        index = 0;
        count = room_settings[current_room].opponent_count;
        if (index < count) do {
            select_opponent(index);
            debug_char.unk11 = 0;
            debug_char.hp_delta = -debug_char.hp;
            if (stage_type == 2) {
                start_skeleton_animation();
            } else if (debug_char.kind == 10) {
                start_mouse_animation(index, 240);
                start_actor_animation(107);
                debug_char.changed = 1;
            } else if (stage_type == 5 || stage_type == 6) {
                debug_char.direction = ~debug_kid.direction;
                debug_char.hp = 0;
                start_actor_animation(154);
            } else {
                start_actor_animation(85);
            }
            write_back_opponent();
        } while (++index < count);
        CtrlPlayer();
        break;
    case 'r':
        if (kid_life > 0) {
            countdown_pending = 20;
            kid_life = -1;
            ClrMessage(1);
        }
        break;
    case 'v':
        text[0] = 0;
        break;
    case 0x2000:
        debug_room_message(0, debug_cheat_message, 0);
        break;
    case 0x3b00:
        debug_show_objects = !debug_show_objects;
        break;
    case 0x3c00:
        debug_show_messages = !debug_show_messages;
        break;
    case 0x3d00:
        message = (char *)(toggle_recording() ? recording_on_message : recording_off_message);
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

    if (find_option(say_cheese_option)) {
        switch (key) {
        case 0x4800: next_room = above_room; break;
        case 0x4b00: next_room = left_room; break;
        case 0x4d00: next_room = right_room; break;
        case 0x5000: next_room = below_room; break;
        }
        if (next_room)
            highlight_room(-1, 0, 10);
    }

    if (control_mode) {
        switch (key) {
        case '1':
            CtrlPlayer();
            debug_kid_x = debug_char.x;
            Kid = Char;
            memcpy(text, kid_x_saved_message, sizeof(kid_x_saved_message));
            message = text;
            break;
        case '2':
            CtrlPlayer();
            debug_char.x = debug_kid_x;
            Kid = Char;
            memcpy(text, kid_x_restored_message, sizeof(kid_x_restored_message));
            message = text;
            break;
        case '3':
            CtrlPlayer();
            debug_kid_y = debug_char.y;
            Kid = Char;
            memcpy(text, kid_y_saved_message, sizeof(kid_y_saved_message));
            message = text;
            break;
        case '4':
            CtrlPlayer();
            debug_char.y = debug_kid_y;
            Kid = Char;
            memcpy(text, kid_y_restored_message, sizeof(kid_y_restored_message));
            message = text;
            break;
        case 0x4700:
            CtrlPlayer();
            if (debug_char.direction == -1)
                debug_char.x = move_kid_x(32);
            else
                debug_char.x = move_kid_x(-32);
            Kid = Char;
            break;
        case 0x4900:
            CtrlPlayer();
            if (debug_char.direction == 0)
                debug_char.x = move_kid_x(32);
            else
                debug_char.x = move_kid_x(-32);
            Kid = Char;
            break;
        case 0x4b00:
            CtrlPlayer();
            if (debug_char.direction == -1)
                debug_char.x = move_kid_x(1);
            else
                debug_char.x = move_kid_x(-1);
            Kid = Char;
            break;
        case 0x4d00:
            CtrlPlayer();
            if (debug_char.direction == 0)
                debug_char.x = move_kid_x(1);
            else
                debug_char.x = move_kid_x(-1);
            Kid = Char;
            break;
        case 0x4f00:
            CtrlPlayer();
            if (debug_char.direction == -1)
                debug_char.x = move_kid_x(5);
            else
                debug_char.x = move_kid_x(-5);
            Kid = Char;
            break;
        case 0x5100:
            CtrlPlayer();
            if (debug_char.direction == 0)
                debug_char.x = move_kid_x(5);
            else
                debug_char.x = move_kid_x(-5);
            Kid = Char;
            break;
        }
    }
    if (message) {
        DrawMessage(message);
        message_ticks = message_duration = 24;
    }
}

#undef debug_char
#undef debug_kid

struct CountdownMessages {
    char one_second[14];
    char seconds[16];
    char minutes[16];
    char expired[18];
};

struct JoystickMessages {
    char unavailable[22];
    char mode[14];
    char missing[20];
};

/* The original initialized message pool is contiguous in preloaded DGROUP.
 * Fixed member sizes retain the even-aligned strings and their terminators. */
extern struct {
    struct CountdownMessages countdown;
    char press_button[26];
    char press_key[22];
    char sound_on[10];
    char sound_off[10];
    char ambient_on[18];
    char ambient_off[18];
    char music_unavailable[18];
    struct JoystickMessages joystick;
} prince_messages;

extern signed char sound_volume;
extern void far pascal set_sound_volume(signed char level);
extern void far pascal set_options_from_disk(void);

int far pascal toggle_sound(void)
{
    int enabled;

    if (sound_volume == 0) {
        set_sound_volume(15);
        enabled = 1;
    } else {
        set_sound_volume(0);
        enabled = 0;
    }
    set_options_from_disk();
    return enabled;
}

typedef struct {
    int unused;
    int control_mode;
    int unrelated[2];
    int buttons;
} GameOptions;

extern unsigned char available_devices;
extern GameOptions *game_options;
extern int control_mode;
extern char gameport_state[9];
extern int far pascal detect_gameport(void);
extern void *cdecl memset(void *destination, int value, unsigned int size);
#pragma intrinsic(memset)

char *far pascal joystick_status(void)
{
    char *message;
    if ((available_devices & 1) && game_options->buttons == 32) {
        message = prince_messages.joystick.unavailable;
    } else if (detect_gameport()) {
        message = prince_messages.joystick.mode;
        game_options->control_mode = control_mode = 2;
    } else {
        message = prince_messages.joystick.missing;
        game_options->control_mode = control_mode = 0;
        memset(gameport_state, 0, sizeof(gameport_state));
    }
    return message;
}

extern void far pascal clear_trobs(void);
extern void far pascal load_room_tiles(unsigned char room);
extern unsigned char far pascal tile_type(int tile);
extern unsigned char far pascal tile_type_extended(int tile);
extern void far pascal add_lava_trob(unsigned char room, int tile);
extern void far pascal add_torch_trob(unsigned char room, signed char tile);
extern void far pascal add_chomper_trob(unsigned char room, signed char tile);
extern void far pascal add_loose_trob(unsigned char room, signed char tile);
extern void far pascal add_crumble_trob(unsigned char room, signed char tile,
                                      signed char active, unsigned char type);
extern void far pascal add_special_gate(unsigned char room, signed char tile, signed char delay);
extern void far pascal add_special_room_trob(unsigned char room, signed char tile);
extern void far pascal add_sixth_room_trob(unsigned char room, signed char tile);
extern void far pascal add_later_room_trob(unsigned char room, signed char tile);
extern unsigned char current_room, left_room, right_room;
extern unsigned char current_level_number, level_flags;

typedef struct {
    unsigned char kind;
    unsigned char properties[4];
} RoomExtension;

extern RoomExtension room_extensions[];

void far pascal add_room_trobs(void)
{
    int index;
    unsigned char room;
    signed char tile;
    unsigned char kind;

    clear_trobs();
    room = current_room;
    index = 0;
    tile = 0;
    do {
        if (index < 30) {
            kind = tile_type_extended(index);
        } else {
            if (index == 30) {
                if (!left_room)
                    break;
                room = left_room;
                load_room_tiles(room);
            }
            if (index == 33) {
                if (right_room) {
                    room = right_room;
                    load_room_tiles(room);
                } else {
                    index += 3;
                }
            }
            if (index < 33) {
                tile = tile_type(index - 30) + 9;
                kind = room_extensions[index].kind;
            } else {
                tile = index - 33;
                kind = tile_type_extended(tile);
            }
        }

        switch (kind) {
        default:
            if (room == 6 && current_level_number == 6 && (level_flags & 16)) {
                add_sixth_room_trob(room, tile);
            } else if ((room == 7 || room == 8) &&
                       current_level_number == 6 && (level_flags & 16)) {
                add_later_room_trob(room, tile);
            }
            break;
        case 2:
            add_lava_trob(room, tile);
            break;
        case 10:
            if (index < 33)
                add_torch_trob(room, tile);
            break;
        case 19:
        case 32:
            add_chomper_trob(room, tile);
            break;
        case 23:
            if (index < 33)
                add_loose_trob(room, tile);
            break;
        case 28:
        case 29:
        case 31:
        case 37:
        case 38:
        case 39:
        case 40:
        case 43:
            add_crumble_trob(room, tile, 1, kind);
            break;
        case 30:
            add_special_gate(room, tile, 3);
            break;
        case 44:
            add_special_room_trob(room, tile);
            break;
        }
        ++tile;
    } while (++index < 43);
    load_room_tiles(current_room);
}

extern signed char timer_disabled, kid_life;
extern int minutes_remaining, countdown_pending;
extern unsigned int ticks_remaining;
extern int message_ticks, message_duration, deferred_room, room_number;
extern void far pascal DrawMessage(const char *message);
extern int far cdecl sprintf(char *buffer, const char *format, ...);
extern char *cdecl strcpy(char *destination, const char *source);
#pragma intrinsic(strcpy)

void far pascal KeepTime(void)
{
    char text[40];
    unsigned int seconds;

    if (timer_disabled >= 0 && kid_life < 0 && minutes_remaining != 0) {
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
        room_number = deferred_room;
        deferred_room = 0;
        return;
    }
    if (countdown_pending) {
        if (minutes_remaining > 0) {
            if (minutes_remaining == 1) {
                seconds = (ticks_remaining + 1) / 12;
                if (seconds == 1)
                    strcpy(text, prince_messages.countdown.one_second);
                else
                    sprintf(text, prince_messages.countdown.seconds, seconds);
            } else {
                sprintf(text, prince_messages.countdown.minutes, minutes_remaining);
            }
            DrawMessage(text);
        } else {
            DrawMessage(prince_messages.countdown.expired);
        }
        message_ticks = message_duration = 24;
        countdown_pending = 0;
    }
}

extern unsigned char next_room;
extern int room_transition_active;
extern unsigned char room_controls[2];
extern signed char opponent_index;
extern void far pascal set_room_redraw(void);
extern int far pascal IsLevelStartWithNISScreen(void);
extern void far pascal refresh_room_screen(void);
extern void far pascal LoadCustomRoomData(unsigned char room);
extern void far pascal change_room_sound(signed char character, int id, int flag);
extern void far pascal load_room_effects(unsigned char room);
extern void far pascal reset_room_characters(void);
extern signed char far pascal set_room_opponents(signed char mode);
extern void far pascal CtrlPlayer(void);
extern void *cdecl memset(void *destination, int value, unsigned int size);
#pragma intrinsic(memset)

extern RoomSettingsRow room_settings[];

int far pascal PrepCut(int keep_screen)
{
    unsigned char room = current_room;
    if (next_room && next_room != room) {
        current_room = next_room;
        set_room_redraw();
        if (!keep_screen || !IsLevelStartWithNISScreen())
            refresh_room_screen();
        LoadCustomRoomData(current_room);
        room_transition_active = 1;
        CtrlPlayer();
        add_room_trobs();
        memset(room_controls, 0, sizeof(room_controls));
        load_room_effects(current_room);
        reset_room_characters();
        if (!room_settings[current_room].opponent_count)
            change_room_sound(-1, 0, 8);
        else
            opponent_index = set_room_opponents(1);
    }
    return -2;
}

extern void far pascal CtrlPlayer(void);
extern void far pascal select_character(char index);
extern void far pascal DrawKidMeter(int previous, int current);
extern void far pascal DrawOppMeter(char room, int previous, int current);
extern int video_ticks;
extern unsigned int ticks_remaining;

extern signed char opponent_index;

/* Increment the signed state byte up to 12, then refresh the step delta. */
extern unsigned char STATE_BYTE;
extern unsigned char STATE_BASE;
extern unsigned char STATE_DELTA;
char far pascal UPDATE_STEP_DELTA(void);

char far pascal BoostMeter(void)
{
    int next = (signed char)STATE_BYTE + 1;
    STATE_BYTE = (next > 12) ? 12 : next;
    return UPDATE_STEP_DELTA();
}

char far pascal UPDATE_STEP_DELTA(void)
{
    STATE_DELTA = STATE_BYTE - STATE_BASE;
    return STATE_DELTA;
}

void far pascal CheckMeters(void)
{
    CtrlPlayer();
    if (STATE_DELTA)
        DrawKidMeter((signed char)STATE_BASE, (signed char)STATE_BYTE);
    if (STATE_BASE == 1) {
        if (video_ticks % 2)
            DrawKidMeter(1, 0);
        else
            DrawKidMeter(0, 1);
    }
    Kid = Char;
    if (opponent_index != -1) {
        select_character(opponent_index);
        if (STATE_DELTA && !((Char.kind == 7 || Char.kind == 8) && Char.sequence == 2))
            DrawOppMeter(Char.room, (signed char)STATE_BASE, (signed char)STATE_BYTE);
        if (STATE_BASE == 1 && !((Char.kind == 7 || Char.kind == 8) && Char.sequence == 2)) {
            if (ticks_remaining & 1)
                DrawOppMeter(Char.room, 1, (signed char)STATE_BYTE);
            else
                DrawOppMeter(Char.room, 0, (signed char)STATE_BYTE);
        }
    }
}

typedef struct {
    char unused[0x12];
    signed char current, maximum, delta;
    char rest[0x2b];
} OpponentMeters;

extern signed char kid_hp, kid_maximum_hp, kid_hp_change;
extern char kid_meter_mode;
extern unsigned char current_room;
extern OpponentMeters opponent_meters[];
extern void far pascal refresh_meter_animation(void);


void far pascal ChangeMeters(void)
{
    int hp;
    signed char opponent, count;
    OpponentMeters *meter;

    if (kid_hp_change) {
        if (kid_meter_mode == 1)
            refresh_meter_animation();
        hp = kid_hp_change + kid_hp;
        hp = hp > 0 ? hp : 0;
        kid_hp = hp > kid_maximum_hp ? kid_maximum_hp : (signed char)hp;
    }
    opponent = 0;
    count = room_settings[current_room].opponent_count;
    for (; opponent < count; ++opponent) {
        meter = &opponent_meters[opponent];
        if (meter->delta) {
            hp = meter->current + meter->delta;
            hp = hp > 0 ? hp : 0;
            if (hp > meter->maximum)
                hp = meter->maximum;
            meter->current = hp;
        }
    }
}


extern unsigned char input_changed;
extern signed char joystick[3];
extern int paused;
extern char prince_pause_message[];

extern void far pascal read_controls(void);
extern unsigned int far pascal prince_poll(void);
extern void far pascal DrawMessage(const char *message);
extern void far pascal update_display(int mode);
extern void far pascal WaitKey(void);
extern void far pascal ClrMessage(int restore);
extern void *cdecl memset(void *buffer, int value, unsigned int size);
#pragma intrinsic(memset)

int far pascal Input(void)
{
    int result;

    input_changed = 0;
    memset(joystick, 0, sizeof(joystick));
    read_controls();
    result = prince_poll();
    if (paused) {
        paused = 0;
        DrawMessage(prince_pause_message);
        update_display(0);
        WaitKey();
        ClrMessage(1);
    }
    if (result == 0)
        result = joystick[2];
    return result;
}


extern int control_mode;
extern int joystick_x_raw, joystick_y_raw;
extern int joystick_x_center, joystick_y_center;
extern signed char joystick_button_raw;
extern unsigned char key_down[];
extern void far pascal sample_gameport(void);
extern void far pascal flush_bios_keyboard(void);

#define keyboard_flags (*(volatile unsigned char far *)0x00400017UL)

void far pascal read_gameport_controls(void);
void far pascal read_keyboard_controls(void);

/* 105a */ void far pascal read_controls(void)
{
    if (control_mode == 2)
        read_gameport_controls();
    else
        read_keyboard_controls();
}

/* 106c */ void far pascal read_gameport_controls(void)
{
    register int displacement;
    int threshold;

    sample_gameport();
    threshold = *(volatile int *)&joystick_y_center / 2;
    displacement = joystick_y_raw - joystick_y_center;
    if (displacement > threshold)
        joystick_y = 1;
    else if (-threshold > displacement)
        joystick_y = -1;

    threshold = *(volatile int *)&joystick_x_center / 2;
    displacement = joystick_x_raw - joystick_x_center;
    if (displacement > threshold)
        joystick_x = 1;
    else if (-threshold > displacement)
        joystick_x = -1;

    joystick_button = -joystick_button_raw;
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

/* 125a */ void far pascal clear_keyboard(void)
{
    if (control_mode == 0)
        flush_bios_keyboard();
}

/* PRINCE.C, resident DOS entries 12de through 133f. */
extern void far pascal update_display(int mode);
extern int far pascal InitPort(void near *port, void far *buffer);
extern void far pascal ZeroPeels(void);
extern unsigned char game_port[];
extern unsigned char game_buffer[];
extern int game_port_id;
extern char current_level;
extern int room_number;

extern volatile unsigned int MODE_TIMER;
extern volatile unsigned int MODE_CHANGED;
extern void far pascal REQUEST_IF_UNSET(unsigned int value);

typedef struct {
    int top, left, bottom, right;
} Rectangle;


extern unsigned char game_port[];
extern unsigned int source_port;
extern void far pascal copy_port_rect(void near *port, unsigned int source,
                                     const Rectangle far *source_rect,
                                     const Rectangle far *dest_rect, int flags);

void far pascal PrinceCopy(const Rectangle far *rect)
{
    Rectangle flipped;
    Rectangle far *copy_rect;

    if (MODE_TIMER != 0) {
        copy_rect = &flipped;
        *copy_rect = *rect;
        copy_rect->top = 192 - rect->bottom;
        flipped.bottom = 192 - rect->top;
    } else {
        copy_rect = (Rectangle far *)rect;
    }

    copy_port_rect(game_port, source_port, copy_rect, copy_rect, 0);
}

void far pascal ToggleInvertState(void)
{
    if (MODE_TIMER != 0) {
        MODE_TIMER = 0;
        REQUEST_IF_UNSET(154);
    } else {
        MODE_TIMER = 1080;
        REQUEST_IF_UNSET(153);
    }
    MODE_CHANGED = 1;
}

/* Restrict the step value, then request mode 105 unless already pending. */
extern unsigned int RESTRICTED_STEP;
extern void far pascal REQUEST_IF_UNSET(unsigned int value);

void far pascal GoWeightless(void)
{
    RESTRICTED_STEP = 228;
    REQUEST_IF_UNSET(105);
}

/* 1316 */ void far pascal ResetData(void)
{
    update_display(0);
    game_port_id = InitPort(game_port, game_buffer);
    ZeroPeels();
    current_level = -1;
    room_number = 0;
}
