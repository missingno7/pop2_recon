/* AUTO.C ordered reconstruction through 2344:0fae. */

extern int opponent_update_state;
extern unsigned char STATE_BYTE;
extern volatile unsigned char actor_kind;
extern volatile int state_word;
extern unsigned char opponent_life;
extern unsigned char opponent_life_display;

void far pascal SetOppMaxLifePts(register unsigned char *p);

void far pascal reset_overlay_state(void)
{
    opponent_update_state = 0;
}

void far pascal set_opponent_life(unsigned char *record)
{
    SetOppMaxLifePts(record);
    if ((actor_kind == 4 && state_word == 119) ||
        (actor_kind == 10 && state_word == 105)) {
        opponent_life = 0;
        opponent_life_display = 0;
    } else {
        opponent_life = STATE_BYTE;
        opponent_life_display = opponent_life;
    }
}

void far pascal SetOppMaxLifePts(register unsigned char *p)
{
    if (p[16] == 0)
        STATE_BYTE = 3;
    else
        STATE_BYTE = p[16];
}


#pragma pack(1)
typedef struct {
    signed char tile;
    int x;
    signed char direction;
    unsigned char flags;
    int action;
    int saved_sequence;
    unsigned char palette_kind;
    unsigned char index;
    unsigned char combat_mode;
    signed char life;
    unsigned char behavior;
    unsigned char behavior_type;
    signed char appearance;
    unsigned char max_life;
    int saved_y;
    int other_y;
    int entry_y;
} opponent_record;

typedef struct {
    signed char count;
    opponent_record opponents[5];
} room_opponents;

extern room_opponents opponent_rooms[];

typedef struct {
    unsigned char kind;
    unsigned char limit;
    signed char column;
    signed char row;
    unsigned char delay;
    unsigned char reset_delay;
    unsigned char behavior;
    signed char alternate_column;
    unsigned char enabled;
    unsigned char flags;
} generator_point;
typedef struct {
    signed char count;
    unsigned char spawn_flags;
    unsigned char header[2];
    generator_point points[3];
} room_generators;
extern room_generators generators[];
extern volatile unsigned char room_number, left_room, right_room;
extern volatile unsigned char preferred_opponent, loaded_appearance, room_style, encounter_type;
extern unsigned char appearance_kind[];
extern signed char actor_index;
extern volatile signed char actor_direction;
extern signed char actor_column, actor_row, actor_horizontal_speed, actor_vertical_speed;
extern unsigned char current_room;
#define actor_room current_room
extern volatile int bank_mask;
#define actor_bank_mask bank_mask
extern volatile unsigned char bank_opponents[2];
extern volatile signed char kid_row;
extern unsigned char actor_behavior, actor_behavior_type, actor_combat_mode;
extern unsigned char actor_loaded, actor_control, actor_move_mode, actor_sequence_mode;
extern volatile int actor_action;
extern int actor_x, actor_y, actor_sequence, actor_saved_action;
extern int actor_timer, actor_delay, actor_wait, actor_flags;
extern int encounter_state, encounter_timer, encounter_flags;
extern volatile int kid_x;

opponent_record *far pascal GetOppInfoIdx(int room, int index);
void far pascal check_side_rooms(void);
void far pascal add_opponent_to_room(int room, opponent_record *record);
void far pascal RemoveOppFromRoom(unsigned char room, int index);
void far _fastcall reserve_opponent_bank(unsigned char kind);
void far pascal load_opponent_bank(int resource, int start, unsigned char size,
                                   unsigned char kind);
void far pascal load_opponent_appearance(signed char appearance);
void far pascal set_default_actor_y(void);
signed char far pascal row_for_actor_x(int x);
opponent_record *far pascal init_special_opponent(opponent_record *record);
opponent_record *far pascal init_guard_opponent(opponent_record *record);
int far pascal current_stage(void);
int far pascal stage_has_shadow(int stage);
void far pascal set_actor_action(int action);
void far pascal update_actor_position(void);
int far pascal actor_action_has_flag(int action);
void far pascal init_opponent_script(void);
void far pascal set_opponent_behavior(signed char index, int behavior);
void far pascal update_actor_behavior(void);
void far pascal commit_actor_state(void);

void far pascal initialize_room_actors(void)
{
    opponent_record *record;
    signed char count;
    int index;

    check_side_rooms();
    index = 0;
    count = opponent_rooms[room_number].count;
    if (count > 0) {
        do {
            record = GetOppInfoIdx(room_number, index);
            if (preferred_opponent != 5 && preferred_opponent != 6 &&
                record->appearance != 1 && record->appearance != 9 &&
                record->appearance != 10 && record->appearance != 3)
                record->appearance = preferred_opponent;

            actor_kind = appearance_kind[record->appearance];
            if ((unsigned char)record->appearance != loaded_appearance &&
                preferred_opponent != 5 && preferred_opponent != 6 &&
                actor_kind != 11 && actor_kind != 0 && actor_kind != 6)
                load_opponent_appearance(record->appearance);

            actor_index = index;
            actor_room = room_number;
            actor_column = record->tile / 10;
            if (record->tile < 0)
                --actor_column;

            if (actor_kind == 2 && record->action == 100) {
                actor_y = record->saved_y;
            } else if (actor_kind == 6 && record->entry_y != 0) {
                actor_y = record->entry_y;
                if (record->action == 12) {
                    actor_horizontal_speed = 2;
                    actor_vertical_speed = 18;
                }
            } else {
                set_default_actor_y();
            }
            actor_x = record->x;
            actor_row = row_for_actor_x(actor_x);
            actor_direction = record->direction;
            actor_behavior = record->behavior;
            actor_behavior_type = record->behavior_type;
            actor_timer = 0;
            actor_flags = actor_wait = actor_delay = 0;
            actor_control = actor_sequence_mode = 0;

            if (actor_kind == 0)
                actor_bank_mask = 2;
            else if (actor_kind == 6) {
                actor_bank_mask = 4;
                load_opponent_bank(25001u, 32, 16, 7);
            } else if (preferred_opponent != 0 && (unsigned)preferred_opponent - 5u > 1u)
                actor_bank_mask = 4;
            else
                reserve_opponent_bank(record->palette_kind);

            if (record->saved_sequence == 0) {
                if (actor_kind == 4)
                    record = init_special_opponent(record);
                else if (actor_kind == 7 || actor_kind == 8 || actor_kind == 11)
                    record = init_guard_opponent(record);
                else if (actor_kind == 10) {
                    if (stage_has_shadow(current_stage())) {
                        record->action = 105;
                        actor_combat_mode = 0;
                    } else {
                        record->action = 106;
                        actor_combat_mode = 1;
                        actor_control = 3;
                        actor_direction = kid_x <= actor_x ? -1 : 0;
                    }
                } else if (actor_kind == 6)
                    record->action = 2;
                else {
                    if (record->action != 22)
                        record->action = 77;
                    actor_combat_mode = 0;
                }
                set_actor_action(record->action);
                set_opponent_life((unsigned char *)record);
            } else {
                actor_loaded = 1;
                actor_saved_action = record->action;
                actor_sequence = record->saved_sequence;
                state_word = record->action;
                actor_combat_mode = record->combat_mode;
                opponent_life_display = opponent_life = record->life;
                if (actor_kind != 0)
                    SetOppMaxLifePts((unsigned char *)record);
            }

            update_actor_position();
            if (actor_action_has_flag(actor_action) && actor_kind != 0) {
                if (actor_kind == 2)
                    init_opponent_script();
                actor_move_mode = 7;
                actor_loaded = opponent_life = 0;
                opponent_life_display = -STATE_BYTE;
                if (actor_kind == 4 || actor_kind == 10)
                    set_opponent_behavior(actor_index, 0);
            } else {
                actor_move_mode = -1;
                encounter_state = encounter_timer = encounter_flags = 0;
                if (actor_kind == 11 && actor_action == 264)
                    actor_flags = 6;
            }
            if (actor_kind == 6 && state_word == 12)
                actor_sequence_mode = 4;
            else {
                update_actor_behavior();
                actor_sequence_mode = actor_kind == 7 || actor_kind == 8 ? 0 : 1;
            }
            commit_actor_state();
        } while (count > ++index);
    }

    if (count == 0) {
        if (encounter_type == 12 && (room_number == 11 || room_number == 19) &&
            loaded_appearance != 1)
            load_opponent_appearance(1);
        else if (generators[room_number].count != 0 &&
                 loaded_appearance != preferred_opponent &&
                 ((unsigned)preferred_opponent - 5u > 1u))
            load_opponent_appearance(preferred_opponent);
    }
}

void far pascal check_side_rooms(void)
{
    int side, index;
    unsigned char room;
    signed char count;
    int delta, shift;
    opponent_record *record;
    opponent_record copy;

    for (side = 0; side < 2; ++side) {
        if (side == 0) {
            room = left_room;
            delta = -320;
        } else {
            room = right_room;
            delta = 320;
        }
        if (room == 0)
            continue;
        shift = delta;
        for (index = 0, count = opponent_rooms[room].count; index < count; ++index) {
            record = GetOppInfoIdx(room, index);
            if ((side == 0 && record->x > 367) || (side == 1 && record->x < 208) ||
                ((room == 7 || room == 8) && room_style == 6 && record->appearance == 3)) {
                copy = *record;
                copy.x += shift;
                copy.index = opponent_rooms[room_number].count;
                add_opponent_to_room(room_number, &copy);
                RemoveOppFromRoom(room, index);
                --index;
                --count;
            }
        }
    }
}

extern signed char appearance_for_kind[];
extern volatile unsigned char adjacent_actor_room;
extern signed char kid_column, kid_direction;
extern int kid_y;
extern signed char kid_active;
extern unsigned char kid_motion;
extern unsigned int kid_action;
extern int previous_actor;
extern signed char tracked_actor_index;
extern int actor_y_offset[];
extern int generator_x_left[], generator_x_right[];
extern signed char direction_row_adjustment[];

typedef struct {
    signed char index;
    signed char direction;
    int x;
    int y;
    unsigned char kind;
    unsigned char column;
    unsigned char field_8;
    signed char row;
    unsigned char tile;
    unsigned char suffix[6];
    signed char active;
    unsigned char reserved[46];
} live_opponent;
extern live_opponent live_opponents[];

int abs(int value);
#pragma intrinsic(abs)

unsigned char far pascal register_actor_room(void);
void far pascal note_opponent_type(void);
signed char far pascal encode_tile(signed char row, signed char column);
signed char far pascal column_to_tile(signed char column);
void far pascal check_room_object(int object_id);
void far pascal set_encounter_sound(int sound);
void far pascal cut_actor(int index);
void far pascal update_special_info(opponent_record *record);
void far pascal update_guard_info(opponent_record *record);
void far pascal set_special_opponent(opponent_record *record);
signed char far pascal pick_hidden_actor(signed char direction);
generator_point *far pascal find_active_generator(unsigned char room, signed char column);
int far pascal should_generate(generator_point *point, unsigned char room);
void far pascal generate_opponent(generator_point *point, unsigned char room);

int far pascal update_opponent_record(void)
{
    int removed = 0;
    opponent_record *record, *prior;
    unsigned char room;
    opponent_record copy;

    if (actor_direction == 86 || actor_flags == 5 || actor_flags == 4 || state_word == 196) {
        RemoveOppFromRoom(actor_room, actor_index);
        removed = 1;
    } else {
        record = GetOppInfoIdx(actor_room, actor_index);
        if (record != 0) {
            record->tile = encode_tile(actor_row, actor_column);
            if (record->tile != 30) {
                record->x = actor_x;
                record->direction = actor_direction;
                if ((signed char)actor_move_mode < 0)
                    record->saved_sequence = removed;
                else {
                    record->saved_sequence = actor_sequence;
                    record->action = actor_saved_action;
                }
                record->combat_mode = actor_combat_mode;
                record->life = opponent_life;
                record->appearance = appearance_for_kind[(signed char)actor_kind];
                set_special_opponent(record);
                room = current_stage();
                if (room == 5 || room == 6)
                    note_opponent_type();
            } else {
                room = register_actor_room();
                if (room != 0 && actor_kind != 12) {
                    prior = GetOppInfoIdx(actor_room, actor_index);
                    copy.tile = column_to_tile(actor_column);
                    copy.x = actor_x;
                    copy.direction = actor_direction;
                    copy.flags = prior->flags;
                    copy.action = actor_saved_action;
                    copy.saved_sequence = actor_sequence;
                    copy.palette_kind = prior->palette_kind;
                    copy.combat_mode = actor_combat_mode;
                    copy.life = opponent_life;
                    copy.behavior = actor_behavior;
                    copy.behavior_type = actor_behavior_type;
                    copy.appearance = prior->appearance;
                    copy.max_life = prior->max_life;
                    copy.saved_y = prior->saved_y;
                    copy.other_y = prior->other_y;
                    copy.entry_y = prior->entry_y;
                    add_opponent_to_room(room, &copy);
                } else if (actor_sequence_mode == 9 && room_style == 5) {
                    check_room_object(10025);
                    if (room_number != 19 && room_number != 16)
                        set_encounter_sound(0);
                }
                RemoveOppFromRoom(actor_room, actor_index);
                removed = 1;
                if (opponent_rooms[room_number].count == 0)
                    live_opponents[actor_index].direction = 86;
            }
        }
        opponent_life = 0;
    }
    return removed;
}

void far pascal transfer_opponent_record(void)
{
    opponent_record copy;

    copy = *GetOppInfoIdx(actor_room, actor_index);
    RemoveOppFromRoom(actor_room, actor_index);
    cut_actor(previous_actor);
    copy.tile = column_to_tile(actor_column);
    copy.x = actor_x;
    copy.direction = actor_direction;
    copy.action = actor_saved_action;
    copy.saved_sequence = actor_sequence;
    copy.combat_mode = actor_combat_mode;
    copy.life = opponent_life;
    copy.palette_kind = bank_mask == 4 ? bank_opponents[0] : bank_opponents[1];
    set_special_opponent(&copy);
    add_opponent_to_room(actor_room, &copy);
}

void far pascal set_special_opponent(opponent_record *record)
{
    if (actor_kind == 2) {
        if ((state_word == 186) | (state_word == 100))
            record->saved_y = actor_y;
        if (preferred_opponent == 7) {
            record->appearance = 7;
            return;
        }
    } else if (actor_kind == 11) {
        update_special_info(record);
        return;
    } else if (actor_kind == 6) {
        update_guard_info(record);
    }
}

void far pascal RemoveOppFromRoom(unsigned char room, int index)
{
    int room_index = room;
    room_opponents *list = &opponent_rooms[room_index];
    signed char count = list->count;
    opponent_record *record;
    live_opponent *live;

    if (count > index) {
        if (count > index + 1) {
            live = &live_opponents[index];
            do {
                record = GetOppInfoIdx(room_index, index);
                *record = *GetOppInfoIdx(room_index, index + 1);
                record->index = index;
                if (room_number == room) {
                    *live = live[1];
                    live->index = index;
                }
                ++live;
            } while (++index + 1 < count);
        }
        --list->count;
    }
}

void far pascal add_opponent_to_room(int room, opponent_record *record)
{
    room_opponents *list = &opponent_rooms[room];
    signed char index = list->count;

    if (index < 5) {
        record->index = index;
        ++list->count;
        *GetOppInfoIdx(room, index) = *record;
    }
}

opponent_record *far pascal GetOppInfoIdx(int room, int index)
{
    opponent_record *result;

    if (opponent_rooms[room].count > index)
        result = &opponent_rooms[room].opponents[index];
    else
        result = 0;
    return result;
}

signed char far pascal pick_hidden_actor(signed char direction)
{
    signed char best = -1, index, count;
    int dx, dy, best_dx, best_dy, dy_difference, kx;
    live_opponent *actor;
    opponent_record *record;

    if (adjacent_actor_room != 0) {
        index = 0;
        count = opponent_rooms[adjacent_actor_room].count;
        for (; index < count; ++index) {
            record = GetOppInfoIdx(room_number, index);
            actor = &live_opponents[index];
            dy = abs(actor->y - kid_y);
            if (actor->active < 0 && actor->kind != 11 &&
                !((actor->kind == 7 || actor->kind == 8) && (unsigned char)record->saved_y == 2)) {
                if (direction == 1) {
                    dx = abs(kid_x - actor->x);
                } else if (direction == -1 && actor->x <= (kx = kid_x)) {
                    dx = kx - actor->x;
                    goto clamp_distance;
                } else if (direction == 0 && actor->x > kid_x) {
                    dx = actor->x - kid_x;
clamp_distance:
                    if (dx < 0)
                        dx = 0;
                }
                else
                    dy = -1;
            } else
                dy = -1;

            if (dy >= 0 && dx >= 0) {
                if (best == -1) {
                    best = index;
                } else {
                    best_dx = abs(live_opponents[best].x - kid_x);
                    best_dy = abs(live_opponents[best].y - kid_y);
                    dy_difference = abs(dy - best_dy);
                    if (dy_difference > 43) {
                        if (dy < best_dy)
                            best = index;
                    } else if (dy_difference < 43 && dx < best_dx)
                        best = index;
                }
            }
        }
    }
    return best;
}

int far pascal is_selected_actor(void)
{
    if (pick_hidden_actor(kid_x >= actor_x ? -1 : 0) == actor_index)
        return 1;
    return 0;
}

void far pascal check_generator_points(unsigned char room)
{
    generator_point *point;

    if (kid_active < 0 && kid_motion != 1) {
        point = find_active_generator(adjacent_actor_room, kid_column);
        if (point && should_generate(point, room)) {
            if (point->delay == 0) {
                generate_opponent(point, room);
                point->delay = point->reset_delay;
                --point->enabled;
            } else
                --point->delay;
        }
    }
}

void far pascal generate_opponent(generator_point *point, unsigned char room)
{
    int room_index = room;
    room_opponents *list = &opponent_rooms[room_index];
    signed char index = list->count;
    signed char row;
    opponent_record *record;

    if (index < 5) {
        ++list->count;
        record = GetOppInfoIdx(room_index, index);
        record->tile = column_to_tile(point->column) + point->row;
        if (point->row <= 5) {
            record->x = generator_x_left[point->row];
            record->direction = 0;
        } else {
            record->x = generator_x_right[point->row];
            record->direction = -1;
        }
        record->flags = generators[room_index].spawn_flags;
        record->palette_kind = bank_opponents[0];
        record->index = index;
        record->combat_mode = 0;
        record->behavior = point->behavior;
        ((unsigned char *)&record->entry_y)[0] = point->alternate_column;
        record->max_life = record->life = point->flags & 15;
        actor_index = index;
        actor_direction = record->direction;
        actor_x = record->x;
        actor_kind = appearance_kind[(signed char)preferred_opponent];
        actor_row = point->row;
        actor_column = point->column;
        actor_behavior = point->behavior;
        actor_room = room;
        opponent_life_display = opponent_life = STATE_BYTE = record->life;
        actor_move_mode = -1;
        bank_mask = 4;
        set_default_actor_y();
        update_actor_behavior();
        actor_flags = 0;
        actor_loaded = 1;
        row = kid_row;
        if (kid_direction != actor_direction &&
            ((kid_action >= 1 && kid_action <= 14) ||
             (kid_action >= 49 && kid_action <= 56) ||
             (kid_action >= 34 && kid_action <= 44)))
            row -= direction_row_adjustment[kid_direction];
        if (point->row + 3 >= row && point->row - 3 <= row &&
            (point->flags & 15) >= 6) {
            actor_combat_mode = 1;
            set_actor_action(90);
            actor_control = 3;
        } else {
            actor_combat_mode = 0;
            set_actor_action(84);
            actor_control = 0;
        }
        update_actor_position();
        if (tracked_actor_index == -1)
            tracked_actor_index = actor_index;
        commit_actor_state();
    }
}

generator_point *far pascal get_generator(unsigned char room, signed char index);
int far pascal is_active_generator(generator_point *point, signed char column);

generator_point *far pascal find_active_generator(unsigned char room, signed char column)
{
    signed char index = 0;
    generator_point *point = 0;
    signed char count = generators[room].count;

    for (; index < count; ++index) {
        point = get_generator(room, index);
        if (is_active_generator(point, column))
            break;
        point = 0;
    }
    return point;
}

int far pascal is_active_generator(generator_point *point, signed char column)
{
    int result;

    if ((point->column == column || point->alternate_column == column) &&
        abs(point->row - kid_row) > 2 && point->enabled != 0)
        result = 1;
    else
        result = 0;
    return result;
}

extern volatile signed char current_level;
extern int generator_level;
extern int game_tick;
extern volatile int kid_x;

signed char far pascal find_room_row(unsigned char room, signed char row,
                                    signed char column, signed char direction);

int far pascal should_generate(generator_point *point, unsigned char room)
{
    signed char count = opponent_rooms[room].count;
    signed char index, room_row;
    unsigned char between;
    int allowed;

    if (count < 5 && current_level == generator_level && game_tick % 3 != 0) {
        allowed = 1;
        index = 0;
        between = 0;
        room_row = find_room_row(room, point->row, point->column,
                                 point->row >= 5 ? -1 : 1);
        if (count > 0) {
            do {
                if (!allowed)
                    break;
                if (live_opponents[index].active < 0 &&
                    ((point->row < kid_row && live_opponents[index].x < kid_x) ||
                     (point->row > kid_row && live_opponents[index].x > kid_x)) &&
                    ((point->row < room_row && kid_row < room_row) ||
                     (point->row > room_row && kid_row > room_row))) {
                    ++between;
                    if (point->limit <= between ||
                        (point->row < kid_row && live_opponents[index].row <= point->row) ||
                        (point->row > kid_row && live_opponents[index].row >= point->row))
                        allowed = 0;
                }
                ++index;
            } while (index < count);
        }
    } else
        allowed = 0;
    return allowed;
}

generator_point *far pascal get_generator(unsigned char room, signed char index)
{
    generator_point *point;

    if (room && generators[room].count > index)
        point = &generators[room].points[index];
    else
        point = 0;
    return point;
}

extern unsigned char current_room;
int far pascal generator_has_tumble(unsigned char room)
{
    int found = 0;
    signed char count = generators[current_room].count;
    signed char index = 0;
    generator_point *point;

    for (; index < count && !found; ++index) {
        point = get_generator(room, index);
        if (point && (point->flags & 128)) found = 1;
        else found = 0;
    }
    return found;
}

extern volatile unsigned char preferred_opponent;
extern volatile int bank_mask;
extern volatile unsigned char bank_opponents[2];
void far pascal load_opponent_bank(int resource, int start, unsigned char size,
                                   unsigned char kind);

void far _fastcall reserve_opponent_bank(unsigned char kind)
{
    int slot;

    if (preferred_opponent != 0 && preferred_opponent != 5 && preferred_opponent != 6) {
        bank_mask = preferred_opponent == 2 ? 8 : 4;
        return;
    }
    bank_mask = 0;
    if (kind == 0)
        kind = 1;

    for (slot = 0; slot < 2 && bank_mask == 0; ++slot) {
        if (bank_opponents[slot] == kind)
            bank_mask = 4 << slot;
    }
    if (bank_mask == 0) {
        for (slot = 0; slot < 2 && bank_mask == 0; ++slot) {
            if (bank_opponents[slot] == 0) {
                bank_opponents[slot] = kind;
                bank_mask = 4 << slot;
            }
        }
    }
    if (bank_mask == 0)
        bank_mask = 4;
}

void far pascal load_opponent_banks(void)
{
    int slot;
    int start;

    if (preferred_opponent != 0 && preferred_opponent != 5 && preferred_opponent != 6) {
        if (preferred_opponent == 2) {
            load_opponent_bank(750, 48, 16, 0);
            return;
        }
    } else {
        for (slot = 0, start = 32; start < 64; ++slot, start += 16) {
            if (bank_opponents[slot] != 0)
                load_opponent_bank(750, start, 16, bank_opponents[slot] - 1);
        }
    }
}
