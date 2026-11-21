/* CTRL.C reconstruction. See notes.md for claims and unresolved bodies.
 * Functions follow original order. */

extern unsigned char cell_room;
extern signed char cell_column;
extern signed char cell_row;
extern signed char cell_index;
extern unsigned int cell_modifier;
/* The kind byte immediately precedes the modifier word in the DOS cell result. */
#define cell_kind (((char near *)&cell_modifier)[-1])
extern unsigned char *room_kinds;
/* Each modifier entry occupies two words. ReadCell uses its first word. */
extern unsigned int (*room_modifiers)[2];

struct room_links {
    unsigned char left;
    unsigned char right;
    unsigned char above;
    unsigned char below;
};
extern struct room_links room_neighbors[];

extern void far pascal select_cell_tables(unsigned char room);
char far pascal FirstInRow(signed char value);
char far pascal GetUndefineCellId(signed char x, signed char y);

unsigned int far pascal Handler(void)
{
    struct {
        unsigned char room;
        signed char fix_x;
    } local;

    local.fix_x = 0;
    local.room = cell_room;

    if (cell_column < 0) {
        local.fix_x = 10;
        if (!local.room)
            goto check_row;
        local.room = room_neighbors[local.room].left;
        if (!local.room)
            goto check_row;
        cell_column += 10;
    } else if (cell_column >= 10) {
        local.fix_x = -10;
        if (!cell_room)
            goto check_row;
        local.room = room_neighbors[cell_room].right;
        if (!local.room)
            goto check_row;
        cell_column -= 10;
    } else {
        goto check_row;
    }

    local.fix_x = 0;
    cell_room = local.room;
    Handler();

check_row:
    if (cell_room) {
        if (cell_row < 0) {
            cell_row += 3;
            local.room = room_neighbors[cell_room].above;
        } else if (cell_row >= 3) {
            cell_row -= 3;
            local.room = room_neighbors[cell_room].below;
        } else {
            goto finish;
        }
        cell_room = local.room;
        Handler();
    }

finish:
    if (!local.room && cell_room)
        cell_room = local.room;
    if (cell_column < 0 || cell_column >= 10)
        cell_column += local.fix_x;
    return cell_room;
}

char far pascal ReadCell(unsigned char room, signed char column, signed char row)
{
    cell_room = room;
    cell_column = column;
    cell_row = row;
    cell_room = Handler();
    cell_index = FirstInRow(cell_row) + cell_column;
    if (cell_room) {
        select_cell_tables(cell_room);
        cell_kind = room_kinds[cell_index];
        cell_modifier = room_modifiers[cell_index][0];
    } else {
        cell_modifier = 0;
        cell_kind = GetUndefineCellId(column, row);
    }
    return cell_kind;
}

extern unsigned char STATE_MODE;
extern unsigned char STATE_LEVEL;
extern unsigned char STATE_KIND;
extern unsigned char STATE_FLAG;

char far pascal GetUndefineCellId(signed char x, signed char y)
{
    char result;
    switch (STATE_MODE) {
    case 2:
        if ((STATE_KIND == 4 && STATE_LEVEL == 13) ||
            (STATE_LEVEL == 13 && STATE_KIND == 13))
            result = 0;
        else
            result = 20;
        break;
    case 4:
        if (!STATE_FLAG && y == -1) {
            result = 0;
            break;
        }
        if (STATE_KIND == 27 && STATE_LEVEL == 6) {
            result = 0;
            break;
        }
        result = 20;
        break;
    case 5:
        result = 0;
        break;
    case 6:
        if (((STATE_KIND == 7 || STATE_KIND == 8) && STATE_MODE == 6) ||
            (STATE_KIND == 3 && STATE_MODE == 6 && (x < 0 || x == 9) && y == 2))
            result = 20;
        else
            result = 0;
        break;
    default:
        result = 20;
        break;
    }
    return result;
}

signed char far pascal index_cell(signed char column, signed char row)
{
    /* Preserve the distinct byte stores for the exceptional and normal rows. */
    union {
        volatile signed char value;
        unsigned char alternate;
    } index;

    if (row < 3 && row >= -1 && column < 10 && column >= 0) {
        if (row == -1)
            index.value = -1 - column;
        else
            index.alternate = FirstInRow(row) + column;
    } else {
        index.value = 30;
    }
    return index.value;
}

signed char far pascal IndexCell1(signed char column, signed char row)
{
    signed char index = index_cell(column, row);
    /* The fallback has its own byte store in the original. */
    union {
        signed char value;
        volatile unsigned char fallback;
    } result;

    if (index >= 0)
        result.value = index;
    else
        result.fallback = 30;
    return result.value;
}


struct frame_info {
    unsigned char fields[7];
};

extern struct frame_info current_frame;
extern struct frame_info far default_frames[];
extern volatile unsigned char character_kind;
extern unsigned int frame_number;
extern unsigned int frame_resource;
extern void far *far pascal lock_frame_resource(unsigned int handle);

extern unsigned char base_cell;
int far pascal get_base_x(void);
char far pascal get_char_col(int x);
char far pascal get_col(int x);
int far pascal AddCharX(int value);
void far pascal get_frame_info(void);
void far pascal reread_base_cell(void);

void far pascal reread_cells(void)
{
    get_frame_info();
    reread_base_cell();
}

void far pascal reread_base_cell(void)
{
    base_cell = get_char_col(get_base_x());
}

void far pascal get_frame_info(void)
{
    unsigned int adjust = 0;
    struct frame_info far *frames = lock_frame_resource(frame_resource);

    switch ((signed char)character_kind) {
    case 0:
    case 1:
    case 6:
        current_frame = default_frames[frame_number];
        break;
    case 2:
    case 4:
    case 10:
    case 12:
        if (frame_number >= 102 && frame_number < 107)
            adjust = 70;
        current_frame = frames[frame_number + adjust - 149];
        break;
    case 8:
        if (frame_number < 183)
            adjust = 43;
        /* Fall through. */
    case 7:
    case 11:
        current_frame = frames[frame_number + adjust - 149];
        break;
    }
}

extern unsigned char char_direction;
extern int char_x;

int far pascal AddCharX(int value)
{
    if (char_direction)
        value = -value;
    return value + char_x;
}

/* Semantic names are hypotheses only; direct addresses are established separately. */
extern volatile unsigned char signflag;
extern volatile int accumulator;

int far pascal AddFCharX(int value)
{
    if (signflag)
        value = -value;
    accumulator = accumulator + value;
    return accumulator;
}

extern int animation_position;
extern unsigned int animation_sequence;
extern int animation_repeat;
extern unsigned int character_action;
extern volatile unsigned char character_kind;
extern signed char character_row;
extern signed char character_velocity_x;
extern signed char character_velocity_y;
extern signed char character_fall;
extern signed char character_room;
extern signed char character_width;
extern int character_y;
extern int animation_contact;
extern int animation_advance;
extern int animation_jump_flag;
extern unsigned char STATE_MODE;
extern unsigned char STATE_LEVEL;
extern unsigned char base_cell;
extern unsigned char animation_variant;
extern int animation_items;
extern int animation_portion;
extern int sound_steps;
extern unsigned char animation_tick;
extern int kid_step;

int far _fastcall get_seq_op(unsigned int sequence);
unsigned int far _fastcall find_char_sequence(unsigned int id);
void far pascal anim_char_tap(int sound);
int far pascal reset_char_y(void);
void far pascal SetKidDeathMethod(unsigned char method);
void far pascal PotionEffect(void);
void far pascal remove_char(void);

extern unsigned int far pascal resource_status(unsigned int handle);
extern void far pascal retain_resource(unsigned int handle);
extern void far pascal restore_resource(unsigned int handle, unsigned int status);
extern unsigned int far pascal find_resource(unsigned long type, unsigned int id);
extern void far pascal play_sequence_music(int cue);
extern void far pascal stop_sequence_music(void);
extern int far pascal test_animation_condition(int condition);
extern void far pascal animation_event(int event);
extern void far pascal show_animation_message(void);
extern void far pascal play_special_step(int sound);
extern void far pascal play_step(int sound);
extern int far pascal should_shift_step(unsigned char room);

/* The animation dispatcher and tap are placement drafts. */
void far pascal AnimChar(void)
{
    int previous_position = animation_position;
    int reading = 1;
    unsigned int sequence = find_char_sequence(animation_sequence);
    int opcode;
    int first, second;
    unsigned int status;

    if (!sequence)
        return;

    status = resource_status(sequence);
    retain_resource(sequence);

    do {
        opcode = get_seq_op(sequence);
        switch (opcode) {
        case -24:
            if (get_seq_op(sequence))
                play_sequence_music(get_seq_op(sequence));
            else
                stop_sequence_music();
            break;
        case -23:
            animation_position = previous_position - 1 >= 0 ? previous_position - 1 : 0;
            break;
        case -22:
            opcode = get_seq_op(sequence);
            first = get_seq_op(sequence);
            second = get_seq_op(sequence);
            animation_event(test_animation_condition(opcode) ? first : second);
            break;
        case -21:
            animation_contact = get_seq_op(sequence);
            break;
        case -19:
            reset_char_y();
            break;
        case -18:
            animation_repeat = 0;
            break;
        case -17:
            remove_char();
            reading = 0;
            break;
        case -16:
            ++animation_items;
            if (STATE_LEVEL == 6)
                animation_variant = (signed char)base_cell >= 6 ? 2 : 1;
            break;
        case -15:
            anim_char_tap(get_seq_op(sequence));
            break;
        case -14:
            opcode = get_seq_op(sequence);
            if (opcode == 1 || (opcode == 2 && animation_portion == 6)) {
                PotionEffect();
                if (!animation_position) {
                    restore_resource(sequence, status);
                    sequence = find_char_sequence(animation_sequence);
                    goto retain_sequence;
                }
            } else if (opcode == 3) {
                show_animation_message();
            }
            break;
        case -13:
            animation_advance = -1;
            break;
        case -12:
            animation_advance = 1;
            break;
        case -11:
            SetKidDeathMethod(get_seq_op(sequence));
            break;
        case -10:
            if (animation_jump_flag)
                goto change_sequence;
            get_seq_op(sequence);
            break;
        case -9:
            opcode = get_seq_op(sequence);
            first = get_seq_op(sequence);
            if (opcode && character_velocity_x < 16 &&
                (!character_velocity_y || character_velocity_y < 32)) {
                character_velocity_x = character_velocity_x + opcode > 16
                    ? 16 : character_velocity_x + opcode;
            }
            if (first && character_velocity_y < 32) {
                character_velocity_y = character_velocity_y + first > 32
                    ? 32 : character_velocity_y + first;
            }
            break;
        case -8:
            opcode = get_seq_op(sequence);
            if (opcode != 10000)
                character_velocity_x = opcode;
            opcode = get_seq_op(sequence);
            if (opcode != 10000)
                character_velocity_y = opcode;
            break;
        case -7:
            character_fall = get_seq_op(sequence);
            break;
        case -6:
            character_y += get_seq_op(sequence);
            break;
        case -5:
            char_x = AddCharX(get_seq_op(sequence));
            break;
        case -4:
            ++character_row;
            break;
        case -3:
            --character_row;
            break;
        case -2:
            if (character_kind == 10)
                char_x = AddCharX(25);
            char_direction = ~char_direction;
            break;
        case -1:
        change_sequence:
            opcode = get_seq_op(sequence);
            if (STATE_LEVEL == 5 && character_row == 1 &&
                (character_room == 10 || character_room == 7 || character_room == 12) &&
                opcode == 57 && animation_repeat != 60 && base_cell == 9)
                opcode = 86;
            animation_sequence = opcode;
            animation_position = 0;
            restore_resource(sequence, status);
            sequence = find_char_sequence(opcode);
            if (!sequence)
                break;
        retain_sequence:
            status = resource_status(sequence);
            retain_resource(sequence);
            break;
        default:
            frame_number = opcode;
            reading = 0;
            break;
        }
    } while (reading);

    restore_resource(sequence, status);
}

int far _fastcall get_seq_op(unsigned int sequence)
{
    int opcode;
    int far *entries;

    if (sequence) {
        entries = lock_frame_resource(sequence);
        opcode = entries[animation_position];
        ++animation_position;
    }
    return opcode;
}

unsigned int far _fastcall find_char_sequence(unsigned int id)
{
    return find_resource(0x53455153UL, id);
}

void far pascal anim_char_tap(int sound)
{
    switch (sound) {
    case 1:
        ++sound_steps;
        if (STATE_MODE != 1 && !(animation_tick & 3)) {
            play_step(sound_steps % 2 + 23);
            if (character_kind == 4 && should_shift_step(character_room))
                char_x = AddCharX(character_width - 1);
        }
        /* Fall through. */
    case 0:
        if (!character_kind)
            kid_step = 1;
        break;
    default:
        if (sound <= 273) {
            if (sound == 271)
                play_special_step(sound);
            else
                play_step(sound);
        }
        break;
    }
}


#define STATE_KIND character_kind
extern volatile unsigned char STATE_KIND;
extern volatile unsigned char STATE_ARGUMENT;

void far pascal SetKidDeathMethod(unsigned char value)
{
    if (STATE_KIND == 0 || STATE_KIND == 1)
        STATE_ARGUMENT = value;
}
#undef STATE_KIND

int far pascal reset_char_y(void)
{
    character_y = (signed char)63 * character_row + 56;
    if (character_kind == 7 || character_kind == 8)
        character_y -= 19;
    return character_y;
}

char far FirstInRow(signed char value)
{
    char result;

    if (value >= 0)
        result = value * 10;
    else
        result = value * 10 + 9;

    return result;
}

struct CharacterRecord {
    char bytes[64];
};

extern unsigned char hidden_facing;
extern struct CharacterRecord Char, Kid;
extern struct CharacterRecord opponent, actor_slots[];

void copy_kid_to_char(void);
void far pascal save_actor_slot(void);
void far pascal load_actor_slot(int slot);

void far pascal load_kid_with_opponent(signed char slot)
{
    if (slot >= 0 && slot < 5) {
        copy_kid_to_char();
        opponent = actor_slots[slot];
    }
}

void far pascal save_kid_with_opponent(void)
{
    if (opponent.bytes[0] >= 0 && opponent.bytes[0] < 5) {
        Kid = Char;
        actor_slots[opponent.bytes[0]] = opponent;
    }
}

void far pascal load_shadow_with_opponent(int slot)
{
    load_actor_slot(slot);
    opponent = Kid;
}

void far pascal save_shadow_with_opponent(void)
{
    if (Char.bytes[0] >= 0 && Char.bytes[0] < 5)
        save_actor_slot();
    Kid = opponent;
}

/* init_fchar_rect (descriptive name; FM Towns inlines it in SetupCarpet, the Mac body is unlabelled):
   copy the play-area rectangle into the 8-byte rectangle stored at byte 9 of the FChar record.
   fchar_rect (DS:625b = FChar + 9) is a descriptive alias: the layout of FChar is not proven here.
   GamePlayRect (DS:09fe) is the FM Towns global name anchored by matching copies. */
typedef struct {
    int top, left, bottom, right;
} Rect;

extern Rect fchar_rect;
extern Rect GamePlayRect;

void init_fchar_rect(void)
{
    fchar_rect = GamePlayRect;
}

/* Semantic name hypothesis only; original symbol remains unrecovered. */
void far pascal ConvertX(register int value, int near *value_out) {
    if (value < 0)
        --value;
    *value_out = value;
}

/* Names and qualifiers are reconstruction hypotheses. */
extern signed char state_code;
extern signed char vertical_step;
extern int restricted_step;

void far pascal Gravity(void)
{
    if (state_code == 4 || state_code == 9) {
        if (restricted_step) {
            ++vertical_step;
            if (vertical_step > 4)
                vertical_step = 4;
        } else {
            vertical_step += 3;
            if (vertical_step > 33)
                vertical_step = 33;
        }
    }
}

extern signed char horizontal_step;

void far pascal add_char_velocity(void)
{
    if (character_y < 2000) {
        character_y += vertical_step;
        if (state_code == 4 || character_kind == 7 || character_kind == 8) {
            char_x = AddCharX(horizontal_step);
            reread_cells();
        }
    }
}

/* Names, signedness and qualifiers are reconstruction hypotheses. */
extern signed char horizontal_step;
extern signed char vertical_step;

void far pascal ClearCharVel(void)
{
    register signed char zero = 0;
    horizontal_step = zero;
    vertical_step = zero;
}

extern signed char life_points;
extern signed char life_delta;

int far pascal decrement_life_points(volatile signed char amount)
{
    int lethal = 0;

    if (character_kind == 4 && STATE_LEVEL == 5 && character_row == 1 &&
          (character_room == 10 || character_room == 7 || character_room == 12))
        goto done;
    if (life_points > amount)
        goto pending;
    if (-life_points < life_delta)
        life_delta = -life_points;
    lethal = 1;
    goto done;
pending:
    if (-life_delta < amount) {
        life_delta = -amount;
        goto done;
    }
    lethal = (unsigned)(life_points + life_delta) < 1;
done:
    return lethal;
}

char far pascal get_under_foot(void)
{
    return ReadCell(character_room, base_cell, character_row);
}

extern unsigned char frame_flags;
extern int restricted_step;

extern signed char front_offsets[];
extern signed char behind_offsets[];
extern signed char floor_contact;
extern signed char fall_latch;
extern unsigned char grab_enabled;
extern unsigned int crouch_fall;
extern unsigned int grab_delay;
extern unsigned int ledge_hold;
extern unsigned char state_byte_628c, state_byte_628d;
extern char fall_check_before[], fall_check_after[];
extern int far pascal check_fall_state(char far *name);
extern int far pascal tile_blocks_movement(char kind);
extern int far pascal tile_has_floor(char kind);
extern int far pascal tile_is_ledge(char kind);
extern int far pascal cave_floor_blocks_fall(void);
extern void far pascal prepare_palace_fall(void);
extern int far pascal palace_grab_test(void);
extern int far pascal chain_floor_blocks_fall(int index);
extern unsigned char far pascal turn_hidden_actor(unsigned char direction);
extern int far pascal sequence_available(int mask);
extern void far pascal on_undefined_room_death(void);

void far pascal start_fall(void);
char far pascal inside_cell(void);
int far pascal get_edge_distance(void);
int far pascal get_edge_distance_at(int x);
char far pascal get_cells_ahead(signed char distance);
char far pascal get_behind(void);
char far pascal get_above(void);
char far pascal get_above_front(void);
char far pascal get_above_behind(void);
int far pascal check_ledge(char above, char ahead, unsigned int above_modifier,
                         unsigned int ahead_modifier);
int far pascal GrabTest(void);

void far pascal on_ground(void)
{
    char kind;

    if ((frame_flags & 0x40) && character_kind != 10) {
        kind = get_under_foot();
        if (tile_blocks_movement(kind)) {
            if (kind != 7 || ((frame_number < 246 || frame_number > 261) &&
                             character_kind != 11))
                kind = inside_cell();
        }
        if (tile_has_floor(kind) ||
            (kind == 30 && ((unsigned char)cell_modifier & 15) < 2)) {
            if (STATE_MODE == 5 && cave_floor_blocks_fall())
                return;
            if (STATE_MODE == 2 && chain_floor_blocks_fall(16))
                return;
            if (character_kind == 11)
                char_x = AddCharX(get_edge_distance() - 42);
            else
                start_fall();
        }
    }
}

void far pascal start_fall(void)
{
    unsigned int old_frame = frame_number;
    int adjust_clearance = 0;
    int sequence = -1;
    int distance;
    int min_y;

    ++character_row;
    if (floor_contact == 1)
        floor_contact = 0;
    if (character_kind == 0 && STATE_LEVEL == 5 && character_room == 10 &&
        ((char_direction == 0 && state_byte_628c) ||
         (char_direction == 0xff && state_byte_628d))) {
        sequence = 198;
        adjust_clearance = 1;
    }

    if (sequence == -1) {
        switch (old_frame) {
        case 9:
            sequence = 7;
            break;
        case 13:
            sequence = 19;
            break;
        case 26:
            sequence = 18;
            adjust_clearance = 1;
            break;
        case 44:
            sequence = 21;
            adjust_clearance = 1;
            break;
        case 81:
            sequence = 7;
            character_y += 12;
            break;
        default:
            adjust_clearance = 1;
            if (old_frame >= 81 && old_frame < 86) {
                sequence = 19;
            } else if (old_frame - 150U < 30U) {
                if (!character_kind) {
                    crouch_fall = adjust_clearance;
                    if (old_frame == 153 || animation_repeat == 56)
                        sequence = 95;
                    else
                        sequence = 81;
                } else {
                    crouch_fall = 0;
                    if (animation_repeat == 86 || animation_repeat == 67 ||
                        animation_repeat == 108)
                        sequence = 83;
                    else
                        sequence = 82;
                }
            } else if (character_kind == 2 && animation_repeat == 100) {
                sequence = 186;
            } else {
                sequence = 7;
            }
            break;
        }
    }

    if (!check_fall_state((char far *)fall_check_before) && adjust_clearance) {
        if (tile_blocks_movement(get_cells_ahead(1)) ||
            tile_blocks_movement(get_behind()) ||
            !tile_has_floor(get_above()) || !tile_has_floor(get_above_front()) ||
            !tile_has_floor(get_above_behind())) {
            distance = get_edge_distance();
            if (distance <= 7)
                char_x = AddCharX(distance - 11);
            else if (distance >= 25)
                char_x = AddCharX(distance - 21);
            if (!tile_has_floor(get_above()) || !tile_has_floor(get_above_front())) {
                min_y = (signed char)63 * character_row + 1;
                if (min_y > character_y)
                    character_y = min_y;
            }
        }
    }

    if (STATE_MODE == 1) {
        sequence = 27;
        prepare_palace_fall();
        SetKidDeathMethod(8);
    } else if (character_kind == 0 && restricted_step && animation_repeat != 228) {
        sequence = 228;
    }
    animation_event(sequence);
    AnimChar();
    reread_cells();

    if (check_fall_state((char far *)fall_check_after) && adjust_clearance) {
        if (tile_blocks_movement(get_cells_ahead(1)) ||
            tile_blocks_movement(get_behind()) ||
            !tile_has_floor(get_above()) || !tile_has_floor(get_above_front()) ||
            !tile_has_floor(get_above_behind())) {
            distance = get_edge_distance();
            if (distance <= 7)
                char_x = AddCharX(distance - 11);
            else if (distance >= 25)
                char_x = AddCharX(distance - 21);
            if (!tile_has_floor(get_above()) || !tile_has_floor(get_above_front())) {
                min_y = (signed char)63 * character_row + 1;
                if (min_y > character_y)
                    character_y = min_y;
            }
        }
    }

    if (tile_blocks_movement(get_under_foot())) {
        inside_cell();
    } else if (tile_blocks_movement(get_cells_ahead(1))) {
        if (old_frame == 44 && get_edge_distance() < 14) {
            animation_event(114);
            AnimChar();
        } else {
            char_x = AddCharX(-2);
        }
        reread_cells();
    }
}

void far pascal FallingInUndefinedRoom(void)
{
    Gravity();
    add_char_velocity();
    if (STATE_MODE == 5) {
        if (character_y > 384) {
            if (STATE_KIND != 19 && STATE_KIND != 16) {
                play_step(0);
                decrement_life_points(100);
                frame_number = 185;
                SetKidDeathMethod(3);
            }
            character_y = 384;
            vertical_step = 0;
            state_code = 1;
            if (character_kind && hidden_facing == (unsigned char)Char.bytes[0])
                hidden_facing = turn_hidden_actor(char_direction);
        } else if (character_kind == 2 && frame_number != 185 &&
                   !sequence_available(0x2729)) {
            play_step(25);
        }
    } else if (character_y > 384) {
        decrement_life_points(100);
        frame_number = 185;
        SetKidDeathMethod(3);
        character_y = 384;
        vertical_step = 0;
        if (character_kind == 0)
            state_code = 1;
        else if (character_kind == 1)
            on_undefined_room_death();
    }
}

int far pascal is_undefined_room(void)
{
    if (character_room == 0 ||
        (((character_room == 3 && STATE_MODE == 6) ||
          (character_room == 5 && STATE_MODE == 6)) &&
         character_row >= 11))
        return 1;
    return 0;
}

void far pascal FallOn(void)
{
    int position;
    if (grab_enabled && vertical_step < 32 && fall_latch < 0 &&
        (signed char)63 * character_row + 56 <= character_y + 25) {
        position = char_x;
        char_x = AddCharX(-16);
        reread_cells();
        if (!GrabTest()) {
            char_x = position;
            return;
        }
        position = get_edge_distance();
        position += 4;
        if (char_direction == 0xff)
            position -= 7;
        else
            position -= 9;
        char_x = AddCharX(position);
        reset_char_y();
        vertical_step = 0;
        animation_event(15);
        AnimChar();
        play_step(11);
        grab_delay = 12;
        ledge_hold = 0;
    }
}

int far pascal GrabTest(void)
{
    char above = get_above();
    unsigned int modifier = cell_modifier;
    char ahead = get_above_front();
    int result = check_ledge(above, ahead, modifier, cell_modifier);

    if (STATE_MODE == 5 && !result)
        return palace_grab_test();
    return result;
}

char far pascal inside_cell(void)
{
    int distance;
    char ahead;

    if (animation_contact != 4 && character_kind != 11 &&
        !(character_room == 3 && STATE_MODE == 6 && (signed char)base_cell < 0)) {
        distance = get_edge_distance();
        ahead = get_cells_ahead(1);
        if (distance < 11 && !tile_blocks_movement(ahead))
            distance += 11;
        else
            distance = 15 - distance;
        char_x = AddCharX(distance);
        reread_cells();
    }
    return get_under_foot();
}

char far pascal get_cells_ahead(signed char distance)
{
    return ReadCell(character_room,
                    base_cell + front_offsets[(signed char)char_direction] * distance,
                    character_row);
}

char far pascal get_behind(void)
{
    return ReadCell(character_room, base_cell + behind_offsets[(signed char)char_direction],
                    character_row);
}

extern unsigned char frame_x;
extern unsigned char frame_flags;
extern signed char column_remainder;

int far pascal get_base_x(void)
{
    return AddCharX((signed char)(frame_x - (frame_flags & 31)));
}

int far pascal get_edge_distance(void)
{
    return get_edge_distance_at(get_base_x());
}

int far pascal get_edge_distance_at(int x)
{
    int distance;
    get_char_col(x);
    if (!char_direction)
        distance = 31 - column_remainder;
    else
        distance = column_remainder;
    return distance;
}

char far pascal get_char_col(int x)
{
    return get_col(x - 14);
}

char far pascal get_col(int x)
{
    x -= 4;
    column_remainder = x % 32;
    return x / 32 - 4;
}

char far pascal GetRow(int value)
{
    int shifted = value - 3;
    char result = shifted / 63;

    if (shifted <= 0)
        --result;

    return result;
}

extern unsigned char state_left_room;
extern unsigned char state_right_room;
extern unsigned char state_other_room;

char far pascal GetCellCol(void)
{
    signed char col = cell_column;

    if (cell_room != STATE_KIND && cell_room != state_other_room && cell_room != STATE_FLAG) {
        if (cell_room == state_left_room)
            col -= 10;
        else if (cell_room == state_right_room)
            col += 10;
    }

    return col;
}

void far pascal prepare_player_control(void)
{
}

/* Later CTRL.C bodies are incomplete. These are same-file callee drafts. */
char far pascal get_above(void)
{
    return ReadCell(character_room, base_cell, character_row - 1);
}

char far pascal get_above_behind(void)
{
    return ReadCell(character_room, base_cell + behind_offsets[(signed char)char_direction],
                    character_row - 1);
}

char far pascal get_above_front(void)
{
    return ReadCell(character_room, base_cell + front_offsets[(signed char)char_direction],
                    character_row - 1);
}

extern char bad_save_slot[];
extern char bad_load_slot[];
extern int far pascal report_slot_error(char far *message);
extern void far pascal stop_on_slot_error(int code, int first, int second);

void far pascal save_actor_slot(void)
{
    if (Char.bytes[0] >= 0 && Char.bytes[0] < 5) {
        actor_slots[Char.bytes[0]] = Char;
        return;
    }
    if (report_slot_error((char far *)bad_save_slot))
        stop_on_slot_error(1, 0, 0);
}

void copy_kid_to_char(void)
{
    Char = Kid;
}

/* The frame's final routines follow the Kid-to-Char copy. */
