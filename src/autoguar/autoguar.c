/* AUTOGUAR.C. Opening routines in original order. */

extern signed char input_x, input_y;
extern signed char local_forward, local_back, local_up, local_down;

void far pascal unused_guard_entry(void)
{
}

void far pascal auto_forward(void)
{
    local_forward = input_x = -1;
}

void far pascal auto_back(void)
{
    local_back = -1;
    input_x = 1;
}

void far pascal auto_up(void)
{
    local_up = input_y = -1;
}

void far pascal auto_down(void)
{
    local_down = -1;
    input_y = 1;
}

void far pascal auto_run_away(void)
{
    local_down = -1;
    auto_back();
}

void far pascal auto_en_garde(void)
{
    local_down = -1;
    auto_forward();
}

extern unsigned char STATE_A;
extern unsigned char STATE_B;

void far pascal set_overlay_state_bytes(unsigned char value)
{
    STATE_A = value;
    STATE_B = value;
}

extern int death_frame;
extern unsigned char death_count, guard_kind, STATE_LEVEL;
extern signed char guard_id;
extern void far pascal select_animation_sequence(int sequence);
extern void far pascal AnimChar(void);
int far pascal get_opponent_death_sequence(unsigned char kind);

void far pascal check_multiple_dead_bodies(void)
{
    int sequence;
    if (death_frame == 185) {
        sequence = get_opponent_death_sequence(guard_kind);
        if (sequence != 0) {
            select_animation_sequence(sequence);
            AnimChar();
        }
        ++death_count;
    }
}

int far pascal get_opponent_death_sequence(unsigned char kind)
{
    int count;
    int remainder;
    volatile int sequence_result;
    int sequence;

    sequence = 0;
    sequence_result = sequence;
    if (kind == 7) {
        sequence = 191;
        count = 4;
    } else if (kind == 0 && STATE_LEVEL != 6) {
        sequence = 195;
        count = 1;
    }

    if (sequence && (remainder = guard_id % (count + 1)) != 0)
        sequence_result = sequence + remainder - 1;

    return sequence_result;
}

extern unsigned char actor_kind, running_state, alert_state;

void far pascal start_opponent_running(void)
{
    select_animation_sequence(actor_kind == 4 ? 102 : 84);
    running_state = 0;
    alert_state = 0;
}

typedef struct {
    signed char bytes[64];
} GuardActor;

typedef struct {
    signed char count;
    unsigned char tail[115];
} GuardRoom;

extern GuardActor Char, actor_slots[];
extern GuardRoom guard_rooms[];
extern unsigned char actor_room, room_to_check;
extern signed char actor_facing, target_facing, actor_row, target_row, actor_column;
extern unsigned char target_action;
extern unsigned char traced_room;
extern signed char traced_column, traced_row;
extern int allow_ledge_test, guard_wait;

extern signed char far pascal get_tile_below(void);
extern signed char far pascal get_tile_front(void);
extern signed char far pascal get_tile_ahead(signed char distance);
extern int far pascal tile_is_solid(signed char tile);
extern int far pascal tile_is_ledge(signed char tile);
extern int far pascal tile_is_gate(signed char tile);
extern signed char far pascal read_tile(unsigned char room, signed char column, signed char row);
extern int far pascal room_can_tumble(unsigned char room);
extern signed char far pascal random_below(int limit);
extern void far pascal load_actor_slot(signed char id);
extern int far pascal is_dead_pos(int frame);
signed char far pascal count_live_opponents(void);

int far pascal check_opponent_tumble(void)
{
    signed char index, limit;
    int someone_blocks;
    GuardActor saved;
    int result;

    if (STATE_LEVEL == 5 &&
        (actor_room == 19 || actor_room == 16 ||
         (tile_is_solid(get_tile_below()) && actor_facing == -1)))
        result = 1;
    else
        result = 0;

    if (!result && room_can_tumble(actor_room)) {
        index = count_live_opponents();
        someone_blocks = random_below(3) <= index;
        if (!someone_blocks && index != 0) {
            saved = Char;
            index = 0;
            limit = guard_rooms[room_to_check].count;
            if (limit > 0) {
                do {
                    if (someone_blocks)
                        break;
                    if (saved.bytes[0] != index) {
                        load_actor_slot(index);
                        if (actor_row == saved.bytes[10] &&
                            actor_column == saved.bytes[9] &&
                            is_dead_pos(death_frame))
                            someone_blocks = 1;
                    }
                } while (++index < limit);
            }
            Char = saved;
        }
        if (someone_blocks) {
            if (target_facing == actor_facing)
                goto cancel;
            index = get_tile_front();
            if (index == 3 || index == 8 || index == 4)
                goto cancel;
            index = get_tile_below();
            if (index == 3)
                goto cancel;
            result = someone_blocks;
            if (index != 8)
                goto done;
cancel:
            result = 0;
        } else {
            result = someone_blocks;
        }
    }
done:
    return result;
}

void far pascal check_guard_step(void)
{
    signed char tile;
    int stop;

    if (target_action == 2 || target_action == 6)
        return;

    stop = 0;
    tile = get_tile_ahead(1);
    if (tile_is_gate(tile)) {
        stop = 1;
    } else if (!tile_is_solid(tile)) {
        auto_forward();
    } else {
        ++traced_row;
        tile = read_tile(traced_room, traced_column, traced_row);
        if ((tile_is_ledge(tile) && actor_kind != 10) ||
            tile_is_gate(tile) || tile_is_solid(tile) ||
            actor_row - target_row + 1 != 0) {
            stop = 1;
        } else {
            if (allow_ledge_test != stop &&
                read_tile(traced_room, traced_column - 1, traced_row) == 2)
                stop = 1;
            if (!stop)
                auto_forward();
        }
    }

    if (stop) {
        guard_wait = 0;
        auto_back();
    }
}

signed char far pascal count_live_opponents(void)
{
    signed char index, limit, live;

    limit = guard_rooms[room_to_check].count;
    index = live = 0;
    if (limit > 0) {
        do {
            if (actor_slots[index].bytes[17] >= 0)
                ++live;
        } while (++index < limit);
    }
    return live;
}

extern unsigned char target_visible, level_number;
extern signed char target_life;
extern int fight_timeout, guard_counter;
extern void far pascal generic_fight(int distance);
extern void far _fastcall measure_guard_range(int near *horizontal, int near *vertical);
extern int far pascal check_skeleton_room(void);
extern int far pascal opponent_distance(void);

void far pascal check_guard_range(int distance)
{
    int horizontal, vertical;

    if (target_visible == 1 && target_facing != actor_facing) {
        generic_fight(distance);
        return;
    }

    if (fight_timeout == 0) {
        measure_guard_range(&vertical, &horizontal);
        if (horizontal > distance) {
            set_overlay_state_bytes(254);
            return;
        }
        if (level_number != 5 || actor_row != 1 ||
            (actor_room != 10 && actor_room != 7 && actor_room != 12) ||
            check_skeleton_room())
            auto_forward();
    }
    return;
}

void far pascal alert_opponent(void)
{
    int distance;

    if (target_life < 0) {
        distance = opponent_distance();
        if (target_row == actor_row && distance < 0 && distance >= -18) {
            if ((signed char)alert_state >= 2 && death_frame == 166)
                goto engage;
        } else if (guard_counter) {
            if (distance < 0 && distance < -4) {
                auto_down();
                return;
            }
        } else if (distance >= 0 && alert_state != 0) {
engage:
            auto_en_garde();
        }
    }
}

typedef struct {
    unsigned char bytes[21];
    signed char row;
} Opponent;


extern GuardActor Kid;
extern unsigned int target_frame, kid_frame;
extern signed char steps, kid_column, kid_row, kid_facing, kid_life;
extern unsigned char kid_action;
extern int kid_room_test, cell_modifier;
extern signed char far pascal actor_in_direction(signed char facing);
extern Opponent near *far pascal GetOppInfoIdx(int room, int id);
extern int far pascal guard_can_cross(signed char tile, int modifier);
extern int far pascal edge_distance(void);
extern int far pascal offset_char_x(int amount);
extern int far pascal IsExitChasingSkel(unsigned char room);
void far pascal try_opponent_run_jump(void);

extern int abs(int value);
#pragma intrinsic(abs)
#define ACTOR_WORD(actor, k) (*(int near *)&(actor).bytes[k])

void far pascal chase_opponent(void)
{
    signed char tile;
    Opponent near *opponent;
    int nearest, kid_distance;
    int sequence;

    if (!(((unsigned int)death_frame >= 194 && (unsigned int)death_frame <= 199 &&
           (death_frame & 1)) || death_frame == 212)) {
        if (steps >= 1 && (death_frame == 192 || death_frame == 196)) {
            try_opponent_run_jump();
            return;
        }
        return;
    }

    tile = actor_in_direction(~actor_facing);
    if (tile != guard_id)
        nearest = abs(ACTOR_WORD(Char, 2) - ACTOR_WORD(actor_slots[tile], 2));
    else
        nearest = 121;

    tile = get_tile_ahead(1);
    kid_distance = abs(ACTOR_WORD(Char, 2) - ACTOR_WORD(Kid, 2));
    if ((kid_frame >= 1 && kid_frame <= 14) ||
        (kid_frame >= 49 && kid_frame <= 56) ||
        (kid_frame >= 34 && kid_frame <= 44)) {
        if (kid_facing != actor_facing)
            kid_distance -= 32;
        else
            kid_distance += 32;
    } else if (kid_action != 1) {
        kid_distance += 32;
    }

    opponent = GetOppInfoIdx(actor_room, guard_id);
    if ((kid_row == actor_row || opponent->row == kid_row) &&
        guard_can_cross(tile, cell_modifier)) {
        if ((kid_row == actor_row &&
             !(target_facing == actor_facing &&
               ((target_frame >= 1 && target_frame <= 14) ||
                (target_frame >= 49 && target_frame <= 56) ||
                (target_frame >= 34 && target_frame <= 44)) &&
               target_action != 7) &&
             (nearest <= 120 || kid_distance <= 120)) ||
            (actor_facing == -1 && kid_column > actor_column) ||
            (actor_facing == 0 && kid_column < actor_column)) {
            alert_state = 2;
            goto run;
        }
        if (kid_life < 0 && kid_room_test != 13) {
            if (IsExitChasingSkel(actor_room) && kid_distance <= 80) {
                sequence = 87;
                running_state = 1;
                goto select;
            }
            goto keep_pose;
        }
    } else {
        kid_distance = edge_distance();
        if ((kid_row == actor_row || opponent->row == kid_row) &&
            !(kid_distance < 27 && steps >= 1) && steps >= 1 && tile != 4)
            goto keep_pose;
    }
    alert_state = 0;
run:
    sequence = 101;
    goto select;

keep_pose:
    sequence = -1;
select:
    if (sequence != -1) {
        select_animation_sequence(sequence);
        tile = get_tile_below();
        if (death_frame == 212 && !guard_can_cross(tile, cell_modifier))
            ACTOR_WORD(Char, 2) = offset_char_x(10);
    }
}

extern int far pascal random_below_word(int limit);
extern int far pascal perform_run_jump(int id);
extern signed char direction_factor[];
extern unsigned char level_number;

void far pascal try_opponent_run_jump(void)
{
    signed char tile1, tile2, start, steps_ahead;
    Opponent near *opponent;
    int attempt, selected;

    tile1 = get_tile_ahead(1);
    tile2 = get_tile_ahead(2);
    if (tile_is_solid(tile2)) {
        if (tile_is_solid(tile1))
            start = 1;
        else {
            start = 2;
            tile1 = tile2;
        }
        steps_ahead = 0;
        if (tile_is_solid(tile1)) {
            while (steps_ahead < 5) {
                ++steps_ahead;
                tile1 = read_tile(actor_room, actor_column +
                                  direction_factor[actor_facing] * (steps_ahead + start),
                                  actor_row);
                if (!tile_is_solid(tile1))
                    break;
            }
        }

        if (steps_ahead < 4 || (steps_ahead == 4 && !random_below_word(steps - 1)) ||
            (actor_room == 13 && level_number == 13 && kid_room_test != 13))
            attempt = 1;
        else {
            opponent = GetOppInfoIdx(actor_room, guard_id);
            attempt = opponent && opponent->row != actor_row;
        }

        selected = attempt ? perform_run_jump(100) : 0;
        if (!selected) {
            select_animation_sequence(101);
            alert_state = 2;
        }
    }
}

