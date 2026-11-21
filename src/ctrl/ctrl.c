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

struct CharacterRecord { char bytes[64]; };
/* The active and opponent character records are adjacent. */
struct ActorPair {
    struct CharacterRecord current;
    struct CharacterRecord rival;
};
extern struct ActorPair actor_pair;
extern struct CharacterRecord Kid;
extern unsigned char hidden_facing;
#define Char actor_pair.current
#define opponent actor_pair.rival
#define char_direction (*(unsigned char *)&Char.bytes[1])
#define char_x (*(int *)&Char.bytes[2])
#define character_row (Char.bytes[10])
#define character_room (Char.bytes[14])

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
extern signed char state_code;
extern volatile unsigned char character_kind;
extern signed char horizontal_step;
extern signed char vertical_step;
extern int character_y;
extern int animation_contact;
extern int animation_advance;
extern int restricted_step;
extern unsigned char STATE_MODE;
extern unsigned char STATE_LEVEL;
extern unsigned char base_cell;
extern unsigned char animation_variant;
extern int animation_items;
extern int animation_portion;
extern int sound_steps;
extern unsigned char animation_tick;
extern int kid_step;


typedef int far * near * SequenceHandle;

int far _fastcall GetSeqOp(SequenceHandle sequence);
unsigned int far _fastcall find_char_sequence(unsigned int id);
void far pascal anim_char_tap(int sound);
int far pascal reset_char_y(void);
void far pascal SetKidDeathMethod(unsigned char method);
void far pascal PotionEffect(void);
void far pascal remove_char(void);

extern unsigned int far pascal resource_status(SequenceHandle handle);
extern void far pascal retain_resource(SequenceHandle handle);
extern void far pascal restore_resource(SequenceHandle handle, unsigned int status);
extern unsigned int far pascal find_resource(unsigned long type, unsigned int id);
extern void far pascal play_sequence_music(int cue);
extern void far pascal stop_sequence_music(void);
extern int far pascal test_animation_condition(int condition);
extern void far pascal animation_event(int event);
extern void far pascal TriggerJinnee(void);
extern void far pascal play_special_step(int sound);
extern void far pascal play_step(int sound);
extern int far pascal should_shift_step(unsigned char room);

/* The animation dispatcher and tap are placement drafts. */
void far pascal AnimChar(void)
{
    int previous_position = animation_position;
    int reading = 1;
    SequenceHandle sequence = (SequenceHandle)find_char_sequence(animation_sequence);
    int opcode;
    int first, second;
    unsigned int status;

    if (!sequence)
        return;

    status = resource_status(sequence);
    retain_resource(sequence);

    do {
        opcode = GetSeqOp(sequence);
        switch (opcode) {
        case -24:
            if (GetSeqOp(sequence))
                play_sequence_music(GetSeqOp(sequence));
            else
                stop_sequence_music();
            break;
        case -23:
            animation_position = previous_position - 1 >= 0 ? previous_position - 1 : 0;
            break;
        case -22:
            opcode = GetSeqOp(sequence);
            first = GetSeqOp(sequence);
            second = GetSeqOp(sequence);
            if (test_animation_condition(opcode))
                animation_event(first);
            else
                animation_event(second);
            break;
        case -21:
            animation_contact = GetSeqOp(sequence);
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
            anim_char_tap(GetSeqOp(sequence));
            break;
        case -14:
            opcode = GetSeqOp(sequence);
            if (opcode == 1 || (opcode == 2 && animation_portion == 6)) {
                PotionEffect();
                if (!animation_position) {
                    restore_resource(sequence, status);
                    sequence = (SequenceHandle)find_char_sequence(animation_sequence);
                    goto retain_sequence;
                }
            } else if (opcode == 3) {
                TriggerJinnee();
            }
            break;
        case -13:
            animation_advance = -1;
            break;
        case -12:
            animation_advance = 1;
            break;
        case -11:
            SetKidDeathMethod(GetSeqOp(sequence));
            break;
        case -10:
            if (restricted_step)
                goto change_sequence;
            GetSeqOp(sequence);
            break;
        case -9:
            opcode = GetSeqOp(sequence);
            first = GetSeqOp(sequence);
            if (opcode && horizontal_step < 16 &&
                (!vertical_step || vertical_step < 32)) {
                horizontal_step = horizontal_step + opcode > 16
                    ? 16 : horizontal_step + opcode;
            }
            if (first && vertical_step < 32) {
                vertical_step = vertical_step + first > 32
                    ? 32 : vertical_step + first;
            }
            break;
        case -8:
            opcode = GetSeqOp(sequence);
            if (opcode != 10000)
                horizontal_step = opcode;
            opcode = GetSeqOp(sequence);
            if (opcode != 10000)
                vertical_step = opcode;
            break;
        case -7: state_code = GetSeqOp(sequence); break;
        case -6: character_y += GetSeqOp(sequence); break;
        case -5: char_x = AddCharX(GetSeqOp(sequence)); break;
        case -4: ++character_row; break;
        case -3: --character_row; break;
        case -2:
            if (character_kind == 10)
                char_x = AddCharX(25);
            char_direction = ~char_direction;
            break;
        case -1:
        change_sequence:
            opcode = GetSeqOp(sequence);
            if (STATE_LEVEL == 5 && character_row == 1 &&
                (character_room == 10 || character_room == 7 || character_room == 12) &&
                opcode == 57 && animation_repeat != 60 && base_cell == 9)
                opcode = 86;
            animation_sequence = opcode;
            animation_position = 0;
            restore_resource(sequence, status);
            sequence = (SequenceHandle)find_char_sequence(opcode);
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

int far _fastcall GetSeqOp(SequenceHandle sequence)
{
    int opcode;
    int far *entries;

    if (sequence) {
        entries = lock_frame_resource((unsigned int)sequence);
        opcode = entries[animation_position]; ++animation_position;
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
                char_x = AddCharX(Char.bytes[0] - 1);
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

extern struct CharacterRecord actor_slots[];

void copy_kid_to_char(void);
void far pascal save_actor_slot(void);
void far pascal load_actor_slot(signed char slot);

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

void far pascal load_shadow_with_opponent(signed char slot)
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

extern signed char life_delta;

int far pascal decrement_life_points(volatile signed char amount)
{
    int lethal = 0;

    if (character_kind == 4 && STATE_LEVEL == 5 && character_row == 1 &&
          (character_room == 10 || character_room == 7 || character_room == 12))
        goto done;
    if (Char.bytes[18] > amount)
        goto pending;
    if (-Char.bytes[18] < life_delta)
        life_delta = -Char.bytes[18];
    lethal = 1;
    goto done;
pending:
    if (-life_delta < amount) {
        life_delta = -amount;
        goto done;
    }
    lethal = (unsigned)(Char.bytes[18] + life_delta) < 1;
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
struct CopyRecord {
    char bytes[5];
};

extern struct CopyRecord SelDir;
extern struct CopyRecord Dir;
extern unsigned int crouch_fall;
extern unsigned int grab_delay;
extern unsigned int ledge_hold;
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
        ((char_direction == 0 && Dir.bytes[0]) ||
         (char_direction == 0xff && Dir.bytes[1]))) {
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

extern int far pascal PlayerCtrl(void);
extern void LoadSelect(void);
extern void SaveSelect(void);
extern void far pascal ClrJoystick(void);
extern void far pascal UserCtrl(void);
extern void far pascal FaceJoystick(void);
extern void far pascal InvertJoystick(void);
extern void far pascal reset_actor_state(int actor);
extern void far pascal update_player_room(void);
extern void far pascal play_death_method(unsigned char method);
extern void far pascal resolve_player_death(void);
extern void far pascal refresh_death_timer(void);
extern int far pascal is_dead_pos(unsigned int frame);
extern void far pascal read_user_input(void);
extern int far pascal check_recording_end(void);
extern void far pascal play_shadow_death(void);
extern void far pascal update_shadow_control(void);
extern void far pascal handle_replay_input(void);
extern void far pascal apply_input(void);
extern unsigned int prefer_kid_state;
extern unsigned int death_sequence_primary, death_sequence_alternate;
extern unsigned int pending_death_sound, death_timer, death_timer_limit;
extern unsigned int delayed_death, replay_input, invert_input;
extern signed char input_room;
extern signed char state_byte_5e30, state_byte_5e31, state_byte_5e32;
extern signed char shadow_advance;

int far pascal ctrl_player(void)
{
    int result;
    prepare_player_control();
    result = PlayerCtrl();
    if (result != -2 || fall_latch < 0 || !is_dead_pos(frame_number))
        goto done;

    if (prefer_kid_state) {
        reset_actor_state(0);
        Kid.bytes[20] = Kid.bytes[19];
        copy_kid_to_char();
        animation_event(2);
        char_x += 16;
        AnimChar();
        reread_cells();
        update_player_room();
    }

    if (character_kind != 0 && character_kind != 1)
        goto done;
    if (sequence_available(death_sequence_primary + 10000) &&
        death_sequence_primary != 4 && death_sequence_primary != 7 &&
        death_sequence_primary != 54)
        goto done;

    pending_death_sound = 0;
    if (fall_latch >= 0 && fall_latch < 6)
        goto advance;
    if (fall_latch == 6) {
        play_death_method(STATE_ARGUMENT);
        goto advance;
    }
    if (fall_latch != 7 || sequence_available(death_sequence_primary + 10000))
        goto done;
    if (sequence_available(death_sequence_alternate + 10000))
        goto done;
    if (!delayed_death) {
        resolve_player_death();
        result = -1;
    } else if (death_timer_limit != 600) {
        death_timer = death_timer_limit = 600;
        refresh_death_timer();
    }
advance:
    ++fall_latch;
done:
    return result;
}

int far pascal PlayerCtrl(void)
{
    int result;
    if (fall_latch < 0 && !Char.bytes[18])
        fall_latch = 0;
    if (grab_delay)
        --grab_delay;
    LoadSelect();
    read_user_input();
    if (input_room != animation_items || !character_room || animation_repeat == 59)
        state_byte_5e31 = state_byte_5e30 = 0;
    ClrJoystick();
    UserCtrl();
    result = check_recording_end();
    SaveSelect();
    return result;
}

void far pascal unused_shadow_hook(void)
{
}

void far pascal shad_ctrl(void)
{
    if (fall_latch >= 0) {
        if (fall_latch >= 6) {
            play_shadow_death();
            if (hidden_facing == (unsigned char)Char.bytes[0])
                hidden_facing = -1;
        } else if (shadow_advance) {
            ++fall_latch;
        }
    } else if (!Char.bytes[18]) {
        fall_latch = 0;
    }
    update_shadow_control();
    if (replay_input)
        handle_replay_input();
    if (character_kind != 1 && character_kind != 12)
        apply_input();
}

void far pascal UserCtrl(void)
{
    unsigned char direction = char_direction;
    if (!direction)
        FaceJoystick();
    if (invert_input)
        InvertJoystick();
    if (replay_input)
        handle_replay_input();
    apply_input();
    if (!direction)
        FaceJoystick();
    if (invert_input)
        InvertJoystick();
}

extern signed char state_byte_5e30;

void far pascal FaceJoystick(void)
{
    char temporary;
    state_byte_5e30 = -state_byte_5e30;
    temporary = Dir.bytes[0];
    Dir.bytes[0] = Dir.bytes[1];
    Dir.bytes[1] = temporary;
}

extern signed char state_byte_5e31;

void far pascal InvertJoystick(void)
{
    char temporary;
    state_byte_5e31 = -state_byte_5e31;
    temporary = Dir.bytes[2];
    Dir.bytes[2] = Dir.bytes[3];
    Dir.bytes[3] = temporary;
}

/* Neutral DS-relative byte aliases; original names and qualifiers are unknown. */

unsigned char far pascal ClrAll(void)
{
    char zero = 0;
    Dir.bytes[3] = zero;
    Dir.bytes[2] = zero;
    Dir.bytes[0] = zero;
    Dir.bytes[1] = zero;
    return 1;
}

/* SaveSelect: remember the current selection (FM Towns CTRL.C public; anchored by PlayerCtrl
   call sites in both builds). Dir/SelDir are the FM Towns globals at DOS DS:628c/DS:626a. */

void SaveSelect(void)
{
    SelDir = Dir;
}

/* LoadSelect: restore the remembered selection before input polling (FM Towns CTRL.C). */
extern struct CopyRecord Dir;
extern struct CopyRecord SelDir;

void LoadSelect(void)
{
    Dir = SelDir;
}

void *memset(void *, int, unsigned);
#pragma intrinsic(memset)
void InitInput(void) { memset(SelDir.bytes, 0, sizeof(SelDir.bytes)); }

void far pascal ClrJoystick(void)
{
    if ((signed char)Dir.bytes[0] >= 0) {
        if (state_byte_5e30 == -1) {
            if (!Dir.bytes[0])
                Dir.bytes[0] = -1;
        } else
            Dir.bytes[0] = 0;
    }
    if ((signed char)Dir.bytes[1] >= 0) {
        if (state_byte_5e30 == 1) {
            if (!Dir.bytes[1])
                Dir.bytes[1] = -1;
        } else
            Dir.bytes[1] = 0;
    }
    if ((signed char)Dir.bytes[2] >= 0) {
        if (state_byte_5e31 == -1) {
            if (!Dir.bytes[2])
                Dir.bytes[2] = -1;
        } else
            Dir.bytes[2] = 0;
    }
    if ((signed char)Dir.bytes[3] >= 0) {
        if (state_byte_5e31 == 1) {
            if (!Dir.bytes[3])
                Dir.bytes[3] = -1;
        } else
            Dir.bytes[3] = 0;
    }
    if (state_byte_5e32 == -1) {
        if (grab_enabled != 1)
            grab_enabled = -1;
    } else if (state_byte_5e32 == -2) {
        if (grab_enabled != 2)
            grab_enabled = -2;
    } else
        grab_enabled = 0;
}

extern int far pascal ledge_material(char kind);

int far pascal check_ledge(char above, char ahead, unsigned int above_modifier,
                           unsigned int ahead_modifier)
{
    int result;
    if (!tile_has_floor(above) || (STATE_MODE == 2 && (above_modifier & 0x80)) ||
        ((ahead == 11 || ahead == 15) && (ahead_modifier & 15)) ||
        !ledge_material(ahead) ||
        (ahead == 4 && !(ahead_modifier & 255) && char_direction == 255) ||
        (STATE_MODE == 5 && character_room == 15 && character_row != 0))
        result = 0;
    else
        result = 1;
    return result;
}

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


/* Semantic name only; this external names the independently witnessed DS byte. */
/* FaceDx uses the active actor's direction. */

int far pascal FaceDx(int value)
{
    if (!char_direction)
        value = -value;
    return value;
}

extern int special_object_flag;
extern unsigned char object_removed[];
extern struct object_modifier { unsigned int flags, reserved; } object_modifiers[];
extern void far pascal redraw_removed_object(void);
void far pascal RemoveObj(int portion)
{
    unsigned int modifier;
    int index;
    animation_portion = portion;
    grab_enabled = 1;
    if (STATE_MODE == 3)
        modifier = cell_modifier & 0x3f00;
    else if (STATE_MODE == 4)
        modifier = cell_modifier & 0x0f00;
    else
        modifier = 0;
    if (special_object_flag)
        modifier |= 0xc000;
    index = (unsigned char)30 * cell_room + cell_index;
    object_removed[index] = 1;
    object_modifiers[index].flags = modifier;
    redraw_removed_object();
}

extern Rect default_clip_rect;
extern signed char above_col, front_col, above_row, contact_row;
extern unsigned int clip_tables[];
extern int far pascal clip_floor(int near *bounds, unsigned int table, int column, int row);
extern void far pascal clip_cave(void);
extern void far pascal clip_ruins(void);
extern void far pascal clip_temple(void);
extern void far pascal clip_rooftops(void);
extern int far _fastcall clip_desert_floor(unsigned int modifier);
extern int clip_top_cache;
extern int sprite_y;
void far pascal MakeFCharClipRect(void);
int far pascal is_char_non_viewable(void);

void far pascal ClipChar(void)
{
    int clearance = 1;
    unsigned int frame = frame_number;
    int action = state_code;
    signed char row = character_row;
    int flying;
    char tile;
    int bounds[4];
    volatile int top;

    init_fchar_rect();
    MakeFCharClipRect();
    if (is_char_non_viewable()) {
        fchar_rect = default_clip_rect;
        return;
    }
    if (character_kind == 0 && frame_number >= 217 && frame_number <= 226) {
        tile = ReadCell(character_room, base_cell, character_row);
        if (clip_floor(bounds, clip_tables[STATE_MODE], base_cell + (tile == 16), character_row)) {
            fchar_rect.top = bounds[0];
            fchar_rect.right = bounds[3];
        }
        return;
    }
    if (state_code == 9) {
        if (STATE_MODE == 5)
            clip_cave();
        return;
    }

    flying = character_kind == 7 || character_kind == 8;
    tile = ReadCell(character_room, above_col, above_row);
    if (tile_has_floor(tile))
        goto solid;
    if (STATE_MODE == 3 && tile == 20 && clip_desert_floor(cell_modifier))
        goto solid;
    if (cell_room == 1 && STATE_LEVEL == 14)
        goto solid;
    if ((cell_room == 7 || cell_room == 8) && STATE_MODE == 6 && animation_repeat != 14)
        goto solid;
    if (action == 0 && (frame == 79 || frame == 81))
        goto adjust;
    tile = ReadCell(character_room, front_col, above_row);
    if (tile_has_floor(tile) || (tile == 20 && STATE_MODE == 3 && clip_desert_floor(cell_modifier))) {
    solid:
        clearance = 0;
    }

adjust:
    if (clearance && STATE_MODE != 5 && STATE_MODE != 1 &&
        !(character_room == 16 && STATE_LEVEL == 9) &&
        !(character_room == 27 && STATE_LEVEL == 6) &&
        !(character_room == 3 && STATE_MODE == 6) &&
        !(character_room == 6 && STATE_MODE == 6)) {
        tile = row + 1;
        if (tile) {
            frame = (signed char)63 * tile - 60;
            if ((int)frame < sprite_y) {
                if ((int)frame - 15 >= clip_top_cache)
                    clearance = 0;
            } else {
                clearance = 0;
            }
        }
        if (clearance && !flying) {
            top = (signed char)63 * tile - 60;
            if (top > fchar_rect.top)
                fchar_rect.top = clip_top_cache = top;
        }
    }
    switch (STATE_MODE) {
    case 1: clip_ruins(); break;
    case 4: clip_temple(); break;
    case 5: clip_cave(); break;
    case 6: clip_rooftops(); break;
    }
}

extern int sprite_id;
extern unsigned char sprite_table_index;
extern unsigned int sprite_tables[];
extern void far *far pascal sprite_dimensions(unsigned int table, int id, int far *size);

void far pascal MakeFCharClipRect(void)
{
    int size[2];
    if (sprite_id != -1 && sprite_dimensions(sprite_tables[sprite_table_index], sprite_id, size)) {
        fchar_rect.top = sprite_y - size[0];
        fchar_rect.bottom = sprite_y + 1;
        if (!signflag) {
            fchar_rect.right = (*(int *)&accumulator);
            fchar_rect.left = (*(int *)&accumulator) - size[1];
        } else {
            fchar_rect.left = (*(int *)&accumulator);
            fchar_rect.right = (*(int *)&accumulator) + size[1];
        }
        return;
    }
    fchar_rect = default_clip_rect;
}

void far pascal uneven_floor(void)
{
    if (character_kind != 11 && character_kind != 7 && character_kind != 8 && get_under_foot() == 6)
        character_y = (signed char)63 * character_row + 57;
}

struct FCharRecord { char bytes[20]; };
extern struct FCharRecord FChar, saved_fchar;

void far pascal IndexChar(void)
{
    unsigned int frame = frame_number;
    int action = state_code;
    if (action == 1) {
        cell_row = contact_row;
        cell_column = above_col;
    } else {
        cell_row = character_row;
        cell_column = base_cell;
    }
    if ((frame >= 135 && frame < 149) || action == 2 || action == 3 || action == 4 || action == 6)
        cell_column += behind_offsets[(signed char)char_direction];
    FChar.bytes[0] = IndexCell1(cell_column, cell_row);
}


extern unsigned char potion_flash;
extern void far pascal apply_life_potion(void);
extern void far pascal apply_motion_potion(void);
extern void far pascal apply_environment_potion(void);
extern void far pascal on_remove_shadow(void);
void far pascal VanishChar(void);

void far pascal PotionEffect(void)
{
    if (character_kind || !animation_portion)
        return;
    if (animation_portion == -1) {
        potion_flash = 0;
    } else {
        switch (--animation_portion) {
        case 0:
            if (Char.bytes[19] != Char.bytes[18])
                life_delta = 1;
            play_special_step(101);
            break;
        case 1:
            apply_life_potion();
            play_special_step(101);
            break;
        case 2:
            apply_motion_potion();
            break;
        case 3:
            apply_environment_potion();
            break;
        case 4:
            if (invert_input) {
                apply_environment_potion();
                break;
            }
            if (decrement_life_points(1))
                animation_event(71);
            play_step(14);
            break;
        case 5:
            play_step(14);
            animation_event(111);
            break;
        }
    }
    animation_portion = 0;
}

int far pascal is_static_frame(void)
{
    int result;
    if (is_dead_pos(frame_number) || frame_number == 0 || frame_number == 15 ||
        frame_number == 109 || frame_number == 166 || frame_number == 171 || frame_number == 229)
        result = 1;
    else
        result = 0;
    return result;
}

int far pascal is_char_non_viewable(void)
{
    int result;
    if (animation_contact == 4 || animation_contact == 5 || animation_contact == 6 ||
        animation_contact == 7 || animation_contact == 10 || animation_contact == 12 ||
        (STATE_MODE == 5 && character_room != 19 && character_room != 16 &&
         character_room != 15 && is_dead_pos(frame_number)))
        result = 1;
    else
        result = 0;
    return result;
}


extern unsigned char actor_variant;

int far pascal is_dead_pos(unsigned int frame)
{
    int dead;
    if (frame == 185) {
        dead = 1;
        goto done;
    }
    switch ((signed char)character_kind) {
    case 0:
        dead = frame == 242 || frame == 243 || frame == 185 || frame == 271 || frame == 266;
        if (!dead && STATE_LEVEL == 5 && character_room == 3)
            dead = fall_latch >= 0 && animation_contact == 10;
        break;
    case 2:
        if (!actor_variant) {
            dead = frame == 228;
            break;
        }
        dead = frame == 245 || frame == 248 || frame == 251 || frame == 252 ||
               frame == 253 || frame == 254 || frame == 255;
        break;
    case 4:
        dead = frame == 206 || frame == 207;
        break;
    case 10:
        dead = frame == 213;
        break;
    case 11:
        dead = frame == 273;
        break;
    default:
        dead = 0;
        break;
    }
done:
    return dead;
}


void far pascal remove_char(void)
{
    if (character_room == 6 && STATE_MODE == 6 &&
        animation_repeat == 240 && Kid.bytes[17] < 0)
        on_remove_shadow();
    VanishChar();
    character_room = 0;
}

extern void far pascal change_actor_life(char slot, int life, int maximum);
void far pascal VanishChar(void)
{
    signed char zero = 0;
    change_actor_life(Char.bytes[0], 0, Char.bytes[19]);
    char_direction = 86;
    state_code = zero;
    fall_latch = zero;
    Char.bytes[18] = zero;
    if (Char.bytes[0] != zero && hidden_facing == (unsigned char)Char.bytes[0])
        hidden_facing = turn_hidden_actor(1);
}

void SaveFChar(void) { saved_fchar = FChar; }
void RestoreFChar(void) { FChar = saved_fchar; }

extern int far pascal guard_cell_passable(char cell, unsigned int modifier);
extern void far pascal guard_adjustment(void);
void far pascal guard_ctrl(void)
{
    if (frame_number == 166 && (signed char)Dir.bytes[3] < 0) {
        if ((signed char)Dir.bytes[0] < 0) {
            if (!guard_cell_passable(get_behind(), cell_modifier))
                char_x = AddCharX(5);
            guard_adjustment();
        } else {
            Dir.bytes[3] = 1;
            animation_event(80);
        }
    }
}

#define opponent_direction ((unsigned char)opponent.bytes[1])
#define opponent_x (*(int *)&opponent.bytes[2])
#define opponent_row (opponent.bytes[10])
#define opponent_room (opponent.bytes[14])
#define opponent_life (opponent.bytes[18])
int far pascal get_opp_dist(void)
{
    int distance;
    if (opponent_life && opponent_room == character_room && opponent_row == character_row &&
        (unsigned char)opponent.bytes[1] != 86) {
        distance = opponent_x - char_x;
        if (char_direction)
            distance = -distance;
        if (distance >= 0 && char_direction != (unsigned char)opponent.bytes[1])
            distance += 13;
    } else {
        distance = 999;
    }
    return distance;
}



static struct slot_error_messages {
    char save[27];
    char load[27];
} slot_errors = {
    "Error in SaveShad(). Quit?",
    "Error in LoadShad(). Quit?"
};
extern int far pascal report_slot_error(char far *message);
extern void far pascal stop_on_slot_error(int code, int first, int second);

void far pascal save_actor_slot(void)
{
    if (Char.bytes[0] >= 0 && Char.bytes[0] < 5) {
        actor_slots[Char.bytes[0]] = Char;
        return;
    }
    if (report_slot_error((char far *)slot_errors.save))
        stop_on_slot_error(1, 0, 0);
}

void far pascal load_actor_slot(signed char slot)
{
    if (slot >= 0 && slot < 5)
        Char = actor_slots[slot];
    else if (report_slot_error((char far *)slot_errors.load))
        stop_on_slot_error(1, 0, 0);
}

void copy_kid_to_char(void)
{
    Char = Kid;
}

/* The frame's final routines follow the Kid-to-Char copy. */

