#pragma pack(1)
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
extern volatile unsigned char preferred_opponent, loaded_appearance, encounter_type;
extern unsigned char room_style;
extern unsigned char appearance_kind[];
extern signed char cut_blocked_index;
extern volatile unsigned char adjacent_actor_room;
extern live_opponent actor_state;
#define actor_index actor_state.index
extern signed char tracked_actor_index;
extern volatile signed char actor_direction;
extern signed char hit_actor_column, hit_actor_direction;
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
extern int kid_x;

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

extern signed char kid_column, kid_direction;
extern int kid_y;
extern signed char kid_active;
extern unsigned char kid_motion;
extern unsigned int kid_action;
extern int previous_actor;

extern int actor_y_offset[];
extern int generator_x_left[], generator_x_right[];
extern signed char direction_row_adjustment[];


extern live_opponent live_opponents[];

int abs(int value);
#pragma intrinsic(abs)

unsigned char far pascal register_actor_room(void);
void far pascal note_opponent_type(void);
signed char far pascal encode_tile(signed char row, signed char column);
signed char far pascal column_to_tile(signed char column);
void far pascal check_room_object(int object_id);
void far pascal set_encounter_sound(int sound);
int far pascal cut_actor(int direction);
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
        list->count = index + 1;
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
extern int kid_x;

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


extern unsigned char opponent_transfer_disabled;
extern unsigned char previous_actor_room;
extern int opponent_review_delay;
extern live_opponent saved_kid_state;

typedef struct {
    unsigned char left, right, above, below;
} room_neighbors;
extern room_neighbors room_links[];

void far pascal load_live_actor(signed char index);
int far pascal test_room_object(int object_id);
void far pascal stop_actor_audio(void);
void far pascal refresh_actor_life(signed char index, int change, int life);
void far pascal sync_live_actors(void);
void far pascal update_room_resources(void);
void far pascal check_offscreen_actors(void);
int far pascal choose_cut_side(void);
void far pascal transfer_room_actors(void);
int far pascal can_actor_follow(void);
int far pascal actor_follows_to_adjacent_room(void);
int far pascal shift_actor_x(int x);
void far pascal setup_room_entry(void);
void far _fastcall queue_room_audio(unsigned char room);
int far pascal set_actor_special_state(unsigned char action);
void far pascal set_actor_fall(unsigned char state);
void far pascal stage_new_room_actor(void);
signed char far pascal column_for_actor_y(int y);

void far pascal cut_room_opponents(void)
{
    signed char index = 0;
    signed char count = opponent_rooms[room_number].count;

    for (; index < count; ++index) {
        load_live_actor(index);
        if (actor_direction == 86 || actor_y < 384)
            continue;
        if (room_style == 5 && cut_blocked_index == actor_index)
            continue;

        if (actor_sequence_mode != 4 && actor_sequence_mode != 3 &&
            ((unsigned)actor_action < 202u || (unsigned)actor_action > 212u)) {
            if (!update_opponent_record())
                commit_actor_state();
        } else if (!opponent_transfer_disabled) {
            update_opponent_record();
            if (actor_kind == 2 && !test_room_object(10025))
                set_encounter_sound(25);
        }

        stop_actor_audio();
        refresh_actor_life(actor_index, 0, (signed char)opponent_life);
        if (tracked_actor_index == actor_index)
            tracked_actor_index = -1;
    }
}

void far pascal check_cut(void)
{
    if (opponent_review_delay) {
        --opponent_review_delay;
        return;
    }

    sync_live_actors();
    update_room_resources();
    check_offscreen_actors();
    previous_actor = choose_cut_side();

    if (previous_actor != -1 && actor_room != 0) {
        saved_kid_state = actor_state;
        previous_actor_room = actor_room;
        transfer_room_actors();
        return;
    }

    if (actor_room == 0 && (signed char)actor_move_mode < 0) {
        if (room_style == 5 && previous_actor == 0) {
            actor_x = shift_actor_x(320);
            setup_room_entry();
        } else {
            queue_room_audio(room_number);
            if (room_style != 5) {
                set_actor_special_state(100);
                actor_action = 185;
                set_actor_fall(3);
                saved_kid_state = actor_state;
                stage_new_room_actor();
                sync_live_actors();
            }
        }
    }
    saved_kid_state = actor_state;
}

void far pascal transfer_room_actors(void)
{
    signed char index = 0;
    signed char count = opponent_rooms[room_number].count;
    int stay, removed;

    for (; index < count; ++index) {
        load_live_actor(index);
        if (actor_direction == 86)
            continue;

        if ((((unsigned)actor_action - 202u <= 10u &&
              actor_behavior == 1) ||
             ((unsigned)actor_action >= 186u && (unsigned)actor_action <= 201u &&
              actor_behavior == 0)) && (actor_kind == 2 || actor_kind == 4))
            goto follow;
        if (actor_follows_to_adjacent_room())
            goto follow;
        if (encounter_type == 5 && actor_column == 1 &&
            (actor_room == 10 || actor_room == 7 || actor_room == 12))
            goto follow;
        if (actor_kind == 0)
            goto follow;
        if (actor_kind == 6 && (actor_room == 7 || actor_room == 8) &&
            room_style == 6 && ((unsigned)previous_actor_room - 7u <= 1u) &&
            room_style == 6) {
follow:
            stay = 0;
process:
            if (stay) {
                removed = update_opponent_record();
                goto next_actor;
            }
            goto transfer;
        }
        if ((signed char)actor_move_mode < 0 && can_actor_follow()) {
            if (previous_actor == 0) {
                if (actor_x < 208) goto follow;
            } else if (previous_actor == 1) {
                if (actor_x >= 371) goto follow;
            } else if (previous_actor == 2) {
                if (actor_column < 0) goto follow;
            } else if (actor_column >= 3)
                goto follow;
        }
        stay = 1;
        goto process;
transfer:
        transfer_opponent_record();
        removed = 1;
next_actor:
        if (removed) {
            --index;
            --count;
        }
    }
}

int far pascal can_actor_follow(void)
{
    int allowed;

    if (opponent_rooms[adjacent_actor_room].count < 5)
        allowed = 1;
    else
        allowed = 0;
    if (allowed) {
        if (actor_combat_mode == 1 || actor_kind == 7 || actor_kind == 8)
            allowed = 1;
        else
            allowed = 0;
        if (allowed) {
            allowed = (actor_kind != 10);
            if (allowed) return allowed;
        }
    }
    return allowed;
}

int far pascal actor_follows_to_adjacent_room(void)
{
    live_opponent saved;
    unsigned char destination;

    saved = actor_state;
    destination = register_actor_room();
    actor_state = saved;

    if (adjacent_actor_room == destination)
        return 1;
    return 0;
}

unsigned char far pascal register_actor_room(void)
{
    unsigned char room = actor_room;
    unsigned char previous;

    if (actor_row < 0 || actor_row >= 10 || actor_column < 0 || actor_column >= 3) {
        do {
        previous = room;
        if (actor_column < 0) {
            room = room_links[room].above;
            if (room) {
                actor_y += 192;
                actor_column += 3;
            } else {
                room = previous;
                previous = 0;
            }
        } else if (actor_column >= 3) {
            room = room_links[room].below;
            if (room) {
                actor_y -= 192;
                actor_column -= 3;
            } else {
                room = previous;
                previous = 0;
            }
        }

        if (actor_row < 0) {
            room = room_links[room].left;
            if (room) {
                actor_x += 320;
                actor_row += 10;
            }
        } else if (actor_row >= 10) {
            room = room_links[room].right;
            if (room) {
                actor_x -= 320;
                actor_row -= 10;
            }
        } else if (!previous)
            room = 0;

        } while (room && (actor_row < 0 || actor_row >= 10 || actor_column < 0 || actor_column >= 3));
    }
    return room;
}

int far pascal cut_actor(int direction)
{
    actor_room = ((unsigned char *)&room_links[actor_room])[direction];
    if (direction == 0) {
        actor_x += 320;
        actor_row = row_for_actor_x(actor_x);
        direction = 1;
    } else if (direction == 1) {
        actor_x -= 320;
        actor_row = row_for_actor_x(actor_x);
        direction = 0;
    } else if (direction == 2) {
        actor_y += 189;
        actor_column = column_for_actor_y(actor_y);
        direction = 3;
    } else {
        actor_y -= 189;
        actor_column = column_for_actor_y(actor_y);
        direction = 2;
    }
    return direction;
}


extern int lower_exit_margin, horizontal_exit_margin;
extern int left_edge_x, right_edge_x;
extern int actor_phase;
extern int opponent_lock, opponent_cooldown, opponent_guard_timeout;
extern int running_guard_state;
extern int scripted_guard_mode;
extern signed char script_motion;

void far pascal probe_room_edge(unsigned char room, signed char column, signed char row);
void far pascal leave_special_room(void);
void far pascal on_left_exit(void);
void far pascal run_actor_kind_four(void);
void far pascal run_actor_kind_six(void);
void far pascal run_actor_kind_seven(void);
void far pascal run_actor_kind_eleven(void);
void far pascal run_shadow_room_script(void);
void far pascal run_walking_guard(void);
void far pascal run_idle_guard(void);
void far pascal run_guard_combat(void);
void far pascal reset_actor_inputs(void);
void far pascal run_shadow_script(void);
void far pascal run_guard_script(void);

int far pascal choose_cut_side(void)
{
    int y = actor_y;
    volatile int mode = (signed char)actor_sequence_mode;
    unsigned int action = actor_action;
    int none = -1;
    int side;

    if (current_level != generator_level || state_word == 70 ||
        (actor_kind == 1 && action == 185))
        goto leave_unchanged;
    if (mode != 5 && mode != 4 && mode != 3 && y < 10 && y >= -16) {
        if (encounter_type == 5) {
            side = none;
            if (actor_room == 3)
                goto check_side;
        }
        side = 2;
        goto check_side;
    }
    if (y >= 231 &&
        (room_links[actor_room].below || y >= lower_exit_margin + 231) &&
        (room_style != 5 || actor_index != cut_blocked_index)) {
        side = 3;
        goto check_side;
    }
    if ((action >= 135 && action < 150) ||
        (action >= 110 && action <= 119) ||
        (action >= 150 && action < 163) ||
        (action >= 166 && action < 169) || mode == 7)
        goto keep_actor;

    if (actor_direction == -1) {
        if (left_edge_x > 125 ||
            (!room_links[actor_room].left && left_edge_x > 125 - horizontal_exit_margin))
            goto check_right_of_actor;
        if (encounter_type != 9 || actor_room != 8 || actor_column != 0 || action != 40)
            goto leave_left;
        ++generator_level;
leave_unchanged:
        side = none;
        goto check_side;
check_right_of_actor:
        side = none;
        if (left_edge_x < 450)
            goto check_side;
        if (!room_links[actor_room].right && left_edge_x < horizontal_exit_margin + 449)
            goto check_side;
        if (room_style == 1 && actor_room == 3)
            goto check_side;
leave_right:
        side = 1;
        goto check_side;
    }

    side = none;
    probe_room_edge(actor_room, 9, actor_column);
    if (right_edge_x >= 454 &&
        (room_links[actor_room].right || right_edge_x >= horizontal_exit_margin + 454) &&
        (room_style != 1 || actor_room != 3))
        goto leave_right;
    if (right_edge_x > 129)
        goto check_side;
    if (room_links[actor_room].left == 0) {
        if (right_edge_x > 130 - horizontal_exit_margin)
            goto check_side;
    }
leave_left:
    side = 0;
    goto check_side;
keep_actor:
    side = -1;
check_side:
    switch (side) {
      case 0:
        if (actor_room == 1 && encounter_type == 2)
            side = -1;
        break;
      case 1:
        if (room_style == 5 && actor_room == 15) {
            side = -1;
            if (state_word != 71)
                leave_special_room();
        } else if (actor_room == 3 && room_style == 6)
            side = -1;
        break;
      case 3:
        if ((actor_room == 11 && room_style == 5) ||
            (actor_room == 3 && room_style == 6) ||
            (actor_room == 5 && room_style == 6) ||
            (encounter_type == 5 && actor_room == 10) ||
            (actor_room == 4 && encounter_type == 13) ||
            (encounter_type == 13 && actor_room == 13) ||
            (((actor_room == 16 && encounter_type == 9) ||
              (actor_room == 27 && encounter_type == 6)) &&
             (state_word == 68 || state_word == 25 || state_word == 10))) {
            side = -1;
            if ((encounter_type == 5 && actor_room == 10) ||
                (actor_room == 4 && encounter_type == 13) ||
                (encounter_type == 13 && actor_room == 13)) {
                set_actor_special_state(100);
                actor_room = 0;
            }
        }
        break;
    }

    if (side != -1)
        cut_actor(side);
    if (side == 0 && room_number == 27 && encounter_type == 6)
        on_left_exit();
    return side;
}

void far pascal update_automatic_actor(void)
{
    signed char kind;

    reset_actor_inputs();
    kind = actor_kind;
    if (opponent_lock)
        --opponent_lock;
    if (opponent_cooldown)
        --opponent_cooldown;
    if (opponent_guard_timeout)
        --opponent_guard_timeout;

    switch (kind) {
      default:
        if (actor_kind == 0) run_actor_kind_eleven();
        else run_guard_script();
        return;
      case 1:
        run_shadow_script();
        return;
      case 4:
      case 10:
        run_actor_kind_four();
        return;
      case 6:
        run_actor_kind_six();
        return;
      case 7:
      case 8:
        run_actor_kind_seven();
        return;
      case 11:
        run_actor_kind_eleven();
        return;
    }
}

extern unsigned char automatic_input[5], automatic_adjustment[3];
void *cdecl memset(void *buffer, int byte, unsigned size);
#pragma intrinsic(memset)

void far pascal reset_actor_inputs(void)
{
    memset(automatic_input, 0, sizeof(automatic_input));
    memset(automatic_adjustment, 0, sizeof(automatic_adjustment));
}

void far pascal run_shadow_script(void)
{
    if (actor_room == 4 && encounter_type == 13)
        run_shadow_room_script();
}

void far pascal run_guard_script(void)
{
    if ((signed char)actor_move_mode < 0 && opponent_life != 0 &&
        ((signed char)opponent_life + (signed char)opponent_life_display) != 0) {
        if (actor_combat_mode == 0) {
            if ((unsigned)actor_action >= 186u && (unsigned)actor_action <= 212u)
                run_walking_guard();
            else
                run_idle_guard();

            if (running_guard_state != 0 &&
                opponent_rooms[actor_room].count - actor_index == 1) {
                running_guard_state = 0;
                return;
            }
        } else if (scripted_guard_mode != 13) {
            run_guard_combat();
            return;
        } else if (actor_phase != 0 && state_word != 110) {
            set_actor_action(110);
        }
    }
}

extern unsigned int hit_actor_action, hit_actor_reaction;
typedef struct {
    unsigned char head[6];
    unsigned char flags;
} hit_action_record;
extern unsigned int hit_action_segment;
#define hit_action_table ((hit_action_record far *)((unsigned long)hit_action_segment << 16))
extern unsigned char hit_actor_flags, hit_actor_draw_mode;
extern unsigned char hit_actor_kind, hit_actor_number;

extern signed char hit_actor_motion, hit_actor_active;
extern int hit_actor_y, hit_actor_state, hit_actor_spacing;
extern signed char hit_actor_target;
extern unsigned char actor_stage;
extern signed char actor_bump_column;
extern unsigned char actor_counterface;
extern int opponent_recovery_time[];
extern unsigned char cliff_style_for_impact;
extern unsigned char hit_actor_pair_state;
extern int landing_x[];
extern unsigned int hit_actor_phase;

void far pascal begin_actor_death(void);
void far pascal stab_actor(void);
void far pascal test_actor_strike(void);
int far pascal is_striking_position(signed char kind, unsigned int action);
int far pascal IsTheStrikePos(char kind, int value);
void far _fastcall get_char_strike_ranges(int *minimum, int *maximum);

void far pascal select_active_actor_pair(signed char index);
void far pascal restore_active_actor_pair(void);
void far pascal load_reverse_actor_pair(signed char index);
void far pascal restore_reverse_actor_pair(void);
void far pascal play_fall_script(unsigned char style);
signed char far pascal current_stage_byte(void);
signed char far pascal adjacent_stage_kind(unsigned char direction);
int far pascal stage_blocks_impact(signed char kind);
void far pascal flash_death_effect(void);
void far pascal actor_end_fall(void);
int far pascal calculate_horizontal_clearance(void);
int far pascal stage_blocks_fall(signed char kind);
int far pascal stage_interrupts_fall(signed char kind);
void far pascal update_actor_tile_column(void);
void far pascal finish_fall_animation(void);
void far pascal begin_special_actor_death(void);
unsigned char far pascal impact_column(void);
void far pascal play_guard_death_script(void);
int far pascal try_spawn_guard(void);
int far pascal actor_strike_distance(void);
int far pascal select_shadow_reaction(void);
int far pascal select_guard_reaction(void);
int far pascal tile_prevents_advance(signed char tile);

void far pascal check_stab(void)
{
    signed char index = 0;
    signed char count = opponent_rooms[room_number].count;

    for (; index < count; ++index) {
        load_live_actor(index);
        if (actor_sequence_mode == 99) {
            select_active_actor_pair(index);
            if (hit_actor_flags == 99)
                hit_actor_flags = 1;
            stab_actor();
            opponent_guard_timeout = opponent_recovery_time[
                opponent_rooms[actor_room].opponents[actor_index].flags];
            restore_active_actor_pair();
        }
    }

    if (kid_motion == 99) {
        for (index = 0; index < count; ++index) {
            load_live_actor(index);
            if (actor_kind == 7 || actor_kind == 8) {
                if (actor_action == 167 || actor_action == 181)
                    break;
            } else if (IsTheStrikePos(actor_kind, actor_action))
                break;
        }
        if (index < count) {
            load_reverse_actor_pair(index);
            stab_actor();
            restore_reverse_actor_pair();
        }
    }
}

void far pascal stab_actor(void)
{
    signed char damage;
    int next_action = -1;
    int sound;

    if ((signed char)actor_move_mode >= 0)
        return;
    if (actor_kind == 0 && state_word == 5 && (room_style == 2 || room_style == 6))
        play_fall_script(room_style);

    if (actor_combat_mode == 255 || state_word == 10 || state_word == 16 ||
        state_word == 28 || state_word == 14) {
        set_actor_special_state(100);
        begin_actor_death();
    } else {
        if (actor_kind == 7 || actor_kind == 8) {
            next_action = 153;
            damage = actor_stage == 2 ? 1 : 2;
            if (damage < (signed char)opponent_life)
                flash_death_effect();
        } else if (actor_kind == 11 || hit_actor_kind == 11) {
            damage = 100;
        } else {
            damage = (actor_kind == 6 || hit_actor_kind == 6) ? 100 : 1;
            next_action = hit_actor_direction != actor_direction ? 74 : 94;
            if (hit_actor_kind == 7 || hit_actor_kind == 8) {
                if (actor_combat_mode != 1 &&
                    !(hit_action_table[hit_actor_action].flags & 64))
                    next_action = 45;
                else if ((unsigned)actor_action >= 246u && (unsigned)actor_action <= 261u) {
                    if (stage_blocks_impact(current_stage_byte()))
                        actor_x = shift_actor_x(16);
                    else if (stage_blocks_impact(adjacent_stage_kind(1)))
                        actor_x = shift_actor_x(-16);
                }
            }
            if (actor_kind == 0 && next_action != 45)
                actor_combat_mode = 1;
        }

        if (!set_actor_special_state(damage) && actor_kind != 11) {
            set_actor_action(next_action);
            goto stop_falling;
        }
    }

    actor_end_fall();
    next_action = calculate_horizontal_clearance();
    if ((stage_blocks_fall(current_stage()) ||
         (stage_blocks_fall(current_stage_byte()) && next_action >= 4)) &&
        actor_kind != 7 && actor_kind != 8 && actor_kind != 11 && actor_kind != 6) {
        next_action -= 8;
        if (actor_bump_column != actor_row)
            next_action -= 32;
        actor_x = shift_actor_x(next_action);
        update_actor_tile_column();
        ++actor_column;
        set_actor_action(81);
    } else {
        begin_actor_death();
stop_falling:
        set_default_actor_y();
        actor_vertical_speed = 0;
    }

    if (hit_actor_kind != 7 && hit_actor_kind != 8 && hit_actor_kind != 11) {
        if (actor_kind == 0)
            sound = hit_actor_kind == 10 ? 194 : 14;
        else if (actor_kind == 4)
            sound = 72;
        else if (actor_kind == 7 || actor_kind == 8)
            sound = 86;
        else if (actor_kind == 11)
            sound = 70;
        else
            sound = 21;
        set_encounter_sound(sound);
    }
    update_actor_position();
    if (state_word == 85) {
        update_actor_tile_column();
        damage = current_stage();
        if (stage_blocks_fall(damage) || stage_interrupts_fall(damage)) {
            actor_x = shift_actor_x(-24);
            update_actor_tile_column();
        }
        finish_fall_animation();
    }
}

void far pascal begin_actor_death(void)
{
    int next_action = -1;
    signed char index;
    int sound;

    actor_control = 0;
    actor_loaded = 1;
    if (actor_kind == 4) {
        begin_special_actor_death();
    } else if (actor_kind == 7 || actor_kind == 8) {
        actor_direction = ~kid_direction;
        opponent_life = 0;
        next_action = 154;
    } else if (actor_kind == 11) {
        next_action = 174;
    } else if (actor_kind == 6) {
        next_action = 196;
    } else {
        index = current_stage_byte();
        if (actor_kind != 0 && actor_direction == -1 &&
            (index == 4 || current_stage() == 4)) {
            index = impact_column();
            actor_x = landing_x[index - (cliff_style_for_impact != 4)] + 14;
            actor_x = shift_actor_x(20);
        }

        if (actor_kind == 0 && (unsigned)actor_action >= 246u && (unsigned)actor_action <= 261u) {
            next_action = 98;
            if (hit_actor_kind == 11)
                sound = 69;
            else if (hit_actor_kind == 7 || hit_actor_kind == 8)
                sound = 89;
            else
                sound = 14;
            goto record_sound;
        } else if (hit_actor_kind == 11) {
            next_action = 97;
            sound = 69;
record_sound:
            set_encounter_sound(sound);
        } else {
            if (hit_actor_kind == 6)
                play_guard_death_script();
            next_action = 85;
            if (actor_kind != 0 && try_spawn_guard()) {
                next_action = 185;
                actor_x = shift_actor_x(-8);
            }
        }
        if (actor_kind == 0) {
            switch (hit_actor_kind) {
              case 2:
                index = preferred_opponent < 1 ? 14 : 10;
                break;
              case 7:
              case 8:
                index = 6;
                break;
              case 10:
                index = 12;
                break;
              case 11:
                index = 7;
                break;
              default:
                index = 2;
                break;
            }
            set_actor_fall(index);
        }
    }
    if (hit_actor_kind == 0 || hit_actor_kind == 1) {
        restore_active_actor_pair();
        hit_actor_target = pick_hidden_actor(hit_actor_direction);
    }
    if (next_action != -1)
        set_actor_action(next_action);
}

void far pascal dispatch_guard_reaction(void)
{
    signed char index, count;

    if (kid_action != 0 && (kid_action < 219 || kid_action >= 229)) {
        index = 0;
        count = opponent_rooms[room_number].count;
        for (; index < count; ++index) {
            select_active_actor_pair(index);
            test_actor_strike();
            restore_active_actor_pair();
            load_reverse_actor_pair(index);
            test_actor_strike();
            restore_reverse_actor_pair();
        }
    }
}

void far pascal test_actor_strike(void)
{
    int distance, minimum, maximum;

    if (actor_combat_mode != 1 || actor_column != hit_actor_column ||
        (abs(actor_y - hit_actor_y) >= 7 &&
         actor_kind != 7 && actor_kind != 8 && hit_actor_kind != 7 && hit_actor_kind != 8) ||
        !is_striking_position(actor_kind, actor_action) || hit_actor_active >= 0 ||
        hit_actor_kind == 10 || hit_actor_kind == 1 || hit_actor_flags == 8 ||
        hit_actor_state == 110)
        return;

    distance = actor_strike_distance();
    if (actor_kind == 7 || actor_kind == 8) {
        minimum = 0;
        maximum = 28;
    } else {
        minimum = 0;
        maximum = 44;
    }

    if (actor_direction != hit_actor_direction && distance >= 0 && distance <= maximum &&
        hit_actor_kind != 7 && hit_actor_kind != 8 && hit_actor_kind != 11 &&
        actor_kind != 11 && (hit_actor_reaction == 161 || hit_actor_reaction == 150)) {
        hit_actor_reaction = 161;
        if (actor_kind == 0)
            actor_behavior_type = hit_actor_number;
        else {
            hit_actor_target = actor_index;
            opponent_lock = 4;
        }

        if (actor_kind == 7 || actor_kind == 8)
            distance = select_guard_reaction();
        else if (actor_kind == 10)
            distance = select_shadow_reaction();
        else
            distance = 69;
        if (distance != -1)
            set_actor_action(distance);
        update_actor_position();
    } else if (IsTheStrikePos(actor_kind, actor_action)) {
        get_char_strike_ranges(&minimum, &maximum);
        if (distance >= minimum && distance <= maximum) {
            hit_actor_flags = 99;
            if (actor_kind == 0)
                actor_behavior_type = hit_actor_number;
        }
    } else if ((actor_kind == 7 || actor_kind == 8) && actor_flags == 1 && opponent_lock == 0)
        hit_actor_flags = 99;

    if (IsTheStrikePos(actor_kind, actor_action) &&
        (hit_actor_reaction != 161 || hit_actor_kind == 7 || hit_actor_kind == 8) &&
        hit_actor_flags != 99) {
        if (actor_kind == 10)
            distance = 193;
        else
            distance = 13;
        set_encounter_sound(distance);
    }
}

void far _fastcall get_char_strike_ranges(int *minimum, int *maximum)
{
    *minimum = 0;
    *maximum = -1;

    if (actor_kind != 0 && actor_kind != 1) {
        if (actor_kind == 10) {
            *minimum = -10;
            *maximum = 38;
        } else if (actor_kind == 11) {
            if (63 * hit_actor_column + 51 <= hit_actor_y) {
                if (hit_actor_direction == actor_direction) {
                    *minimum = -2;
                    *maximum = 27;
                } else {
                    *minimum = -27;
                    *maximum = -4;
                    return;
                }
            }
        } else if (actor_kind == 6) {
            *minimum = 13;
            *maximum = 53;
            return;
        } else if (hit_actor_direction != actor_direction) {
            *minimum = 19;
            *maximum = 46;
            if (actor_kind == 4) {
                *maximum = 48;
                return;
            }
        } else if (hit_actor_draw_mode == 1) {
            *minimum = 34;
            *maximum = 57;
            return;
        } else {
            *minimum = 22;
            *maximum = 42;
            return;
        }
    } else {
        if (hit_actor_kind == 11) {
            if (hit_actor_spacing != 6 && hit_actor_state != 171 && state_word == 96) {
                if (hit_actor_direction == actor_direction) {
                    *minimum = 31;
                    *maximum = 75;
                } else {
                    *minimum = -7;
                    *maximum = 45;
                }
            }
        } else if (hit_actor_kind == 7 || hit_actor_kind == 8) {
            *minimum = 19;
            *maximum = 46;
        } else if (hit_actor_kind == 6) {
            if (hit_actor_direction != actor_direction) {
                *minimum = 13;
                *maximum = 57;
            } else {
                *minimum = 13;
                *maximum = 53;
            }
        } else {
            *minimum = 19;
            *maximum = 46;
            if (!hit_actor_pair_state) {
                *minimum += 14;
                *maximum += 14;
            }
            if (hit_actor_kind == 4)
                *maximum += 2;
        }
        if (actor_stage == 2)
            *maximum -= 5;
    }
}

int far pascal is_striking_position(signed char kind, unsigned int action)
{
    int result;

    if (kind == 7 || kind == 8) {
        result = (action >= 165 && action <= 167) || action == 181;
    } else if (kind == 10) {
        result = action == 223 || action == 224;
    } else if (kind == 11) {
        result = state_word == 173 && (action == 274 || action == 280);
    } else {
        result = action == 153 || action == 154;
        if (!result && kind == 0)
            result = action == 244 || action == 245;
    }
    return result;
}

extern volatile int state_word;

int far pascal IsTheStrikePos(char kind, int value)
{
    int result;

    if (kind == 10) {
        if (value == 224)
            result = 1;
        else
            result = 0;
    } else if (kind == 11) {
        if (state_word == 173 && value == 280)
            result = 1;
        else
            result = 0;
    } else {
        result = value == 154;
        if (!result && kind == 0)
            result = value == 245;
    }
    return result;
}

int far pascal can_continue_run_jump(signed char tile, unsigned int modifier)
{
    int allowed;

    if (tile == 4 && (unsigned char)modifier < 112)
        allowed = 0;
    else
        allowed = 1;
    if (allowed && actor_kind != 10) {
        allowed = !tile_prevents_advance(tile);
        if (allowed && actor_kind != 7 && actor_kind != 8)
            allowed = !stage_blocks_fall(tile);
    }
    return allowed;
}
