extern char one_second_left[], seconds_left[], minutes_left[], time_expired[];
extern char joystick_unavailable_message[], joystick_mode_message[], joystick_missing_message[];

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
        message = joystick_unavailable_message;
    } else if (detect_gameport()) {
        message = joystick_mode_message;
        game_options->control_mode = control_mode = 2;
    } else {
        message = joystick_missing_message;
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
                    strcpy(text, one_second_left);
                else
                    sprintf(text, seconds_left, seconds);
            } else {
                sprintf(text, minutes_left, minutes_remaining);
            }
            DrawMessage(text);
        } else {
            DrawMessage(time_expired);
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

typedef struct {
    signed char opponent_count;
    unsigned char remainder[0x73];
} RoomSettingsRow;

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
        hp = kid_hp + kid_hp_change;
        if (hp < 0)
            hp = 0;
        if (hp > kid_maximum_hp)
            hp = kid_maximum_hp;
        kid_hp = hp;
    }
    opponent = 0;
    count = room_settings[current_room].opponent_count;
    if (opponent < count) do {
        meter = &opponent_meters[opponent];
        if (meter->delta) {
            hp = meter->current + meter->delta;
            if (hp < 0)
                hp = 0;
            if (hp > meter->maximum)
                hp = meter->maximum;
            meter->current = hp;
        }
    } while (++opponent < count);
}
