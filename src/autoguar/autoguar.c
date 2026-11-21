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
    unsigned char records[5][23];
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
        generic_fight(distance); return;
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
            if ((signed char)alert_state >= 2 && death_frame == 166) goto engage;
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
    Opponent near *volatile opponent;
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
    nearest = tile == guard_id ? 121 : abs(ACTOR_WORD(Char, 2) - ACTOR_WORD(actor_slots[tile], 2));

    tile = get_tile_ahead(1);
    kid_distance = abs(ACTOR_WORD(Char, 2) - ACTOR_WORD(Kid, 2));
    if ((kid_frame >= 1 && kid_frame <= 14) ||
        (kid_frame >= 49 && kid_frame <= 56) ||
        (kid_frame >= 34 && kid_frame <= 44)) {
        if (kid_facing != actor_facing) kid_distance -= 32;
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
        *(volatile signed char near *)&tile = get_tile_below();
        if (death_frame == 212 && !guard_can_cross(tile, cell_modifier)) ACTOR_WORD(Char, 2) = offset_char_x(10);
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

    if (tile1 = get_tile_ahead(1), tile_is_solid(tile2 = get_tile_ahead(2))) {
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


extern void far pascal guard_no_jump(void);
extern void far pascal wait_en_garde(void);
extern int far pascal room_allows_guard_advance(void);
extern int fight_timeout;

void far pascal guard_en_garde(void)
{
    signed char tile;
    int upper, lower;
    unsigned int frame = death_frame;
    unsigned int pose;
    int distance;

    if (alert_state == 1 || alert_state == 2 || frame == 166 || frame < 150)
        goto wait;

    if (alert_state == 0) {
        if (guard_wait) {
            check_guard_step();
            goto done;
        }
        if (actor_kind != 4)
            auto_run_away();
        goto done;
    }

    pose = target_frame;
    distance = opponent_distance();
    if (distance >= 27 && pose >= 102 && pose < 118 && target_action == 5)
        goto done;

    measure_guard_range(&upper, &lower);
    if (lower + 12 > distance) {
        if (upper > distance) {
            if (level_number == 5 && actor_row == 1 &&
                (actor_room == 10 || actor_room == 7 || actor_room == 12) &&
                room_allows_guard_advance()) {
advance:
                auto_forward();
                return;
            }

            tile = get_tile_below();
            if (guard_can_cross(tile, cell_modifier) && distance > 0) {
                auto_back();
                return;
            }
            if (level_number != 5 || actor_row != 1 ||
                (actor_room != 10 && actor_room != 7 && actor_room != 12))
                goto advance;
            if (!check_skeleton_room())
                goto done;
            goto advance;
        }
        check_guard_range(distance);
        return;
    }

    if (fight_timeout != 0)
        goto done;
    if (target_facing == actor_facing)
        goto no_jump;
    if (pose >= 7 && pose < 15) {
        if (distance >= 71)
            goto done;
    } else {
        if (pose < 34 || pose >= 44)
            goto no_jump;
        if (distance >= 94)
            goto done;
    }
    set_overlay_state_bytes(254);
    return;

no_jump:
    guard_no_jump();
    return;

wait:
    if (alert_state == 1 || alert_state == 2)
        wait_en_garde();
done:
    return;
}

extern int far pascal opponent_spacing(void);
extern int target_x;
extern void far pascal maybe_advance(void);
#define RUNNING_FRAME(frame) \
    (((frame) >= 1 && (frame) <= 14) || \
     ((frame) >= 49 && (frame) <= 56) || \
     ((frame) >= 34 && (frame) <= 44))

void far pascal wait_en_garde(void)
{
    signed char tile, slot;
    int spacing, distance, can_stand;

    slot = actor_in_direction(~actor_facing);
    if (slot == -1)
        slot = actor_in_direction(1);
    if (slot == -1)
        goto done;
    if (slot == guard_id)
        goto player;

    can_stand = 0;
    if (guard_can_cross(tile = get_tile_ahead(1), cell_modifier) &&
        guard_can_cross(tile = get_tile_ahead(2), cell_modifier) &&
        guard_can_cross(tile = get_tile_ahead(3), cell_modifier))
        can_stand = 1;

    distance = abs(ACTOR_WORD(Char, 2) - ACTOR_WORD(actor_slots[slot], 2));
    spacing = opponent_spacing();
    if (spacing + 24 < distance ||
        (actor_slots[slot].bytes[1] == actor_facing && ACTOR_WORD(actor_slots[slot], 25) == 84))
        goto close_path;

    if (can_stand && target_facing == actor_facing &&
        RUNNING_FRAME(target_frame) && target_action != 7 && steps &&
        ((ACTOR_WORD(Char, 2) < target_x && target_facing == 0) ||
         (ACTOR_WORD(Char, 2) > target_x && target_facing == -1))) {
start_run:
        start_opponent_running();
        return;
    }

    if (distance < spacing)
        goto back_up;
    goto done;

close_path:
    if (!can_stand || ACTOR_WORD(Char, 25) == 84)
        goto done;
    if (((ACTOR_WORD(actor_slots[slot], 25) == 84 && actor_slots[slot].bytes[1] == actor_facing) ||
         (target_facing == actor_facing && RUNNING_FRAME(target_frame) && target_action != 7)) &&
        steps)
        goto start_run;
    auto_forward();
    return;

player:
    distance = abs(ACTOR_WORD(Kid, 2) - ACTOR_WORD(Char, 2));
    if (distance >= 64)
        goto done;
    tile = get_tile_ahead(distance < 32 ? 1 : 2);
    if (!tile_is_ledge(tile) && !tile_is_solid(tile) &&
        (tile != 4 || (unsigned char)cell_modifier >= 112))
        goto done;
back_up:
    if (guard_can_cross(get_tile_below(), cell_modifier))
        auto_back();
done:
    return;
}

int far pascal opponent_spacing(void)
{
    int spacing = 96;
    signed char index, count;

    count = guard_rooms[room_to_check].count;
    index = guard_id + 1;
    if (count > index) {
        do {
            if (abs(ACTOR_WORD(Char, 2) - ACTOR_WORD(actor_slots[index], 2)) < 24)
                spacing += 24;
        } while (++index < count);
    }
    return spacing;
}

void far pascal guard_no_jump(void)
{
    signed char front, further;

    front = get_tile_ahead(1);
    further = get_tile_ahead(2);
    if (tile_is_solid(front) && tile_is_solid(further)) {
        if (!(level_number == 5 && actor_row == 1 &&
              (actor_room == 10 || actor_room == 7 || actor_room == 12) &&
              room_allows_guard_advance()))
            auto_back();
        return;
    }

    if (target_facing == actor_facing && RUNNING_FRAME(target_frame) &&
        target_action != 7 && steps &&
        (level_number != 5 || actor_row != 1 ||
         (actor_room != 10 && actor_room != 7 && actor_room != 12))) {
        start_opponent_running();
        return;
    }
    maybe_advance();
}

extern unsigned int advance_chance[], block_chance[], quick_block_chance[];
extern unsigned int strike_chance[], quick_strike_chance[];
extern int effect_x, effect_y, advance_timer;
void far pascal maybe_block(void);
void far pascal maybe_strike(void);

void far pascal generic_fight(int distance)
{
    int upper, lower;

    measure_guard_range(&upper, &lower);
    if (upper <= distance && distance < lower &&
        !(level_number == 5 && actor_row == 1 &&
          (actor_room == 10 || actor_room == 7 || actor_room == 12) &&
          room_allows_guard_advance())) {
        maybe_block();
        if (effect_y == 0)
            maybe_strike();
    } else {
        maybe_advance();
    }
}

void far pascal maybe_advance(void)
{
    unsigned char skill = guard_rooms[actor_room].records[guard_id][4];

    if (level_number == 5 && actor_row == 1 &&
        (actor_room == 10 || actor_room == 7 || actor_room == 12) &&
        room_allows_guard_advance())
        goto advance;
    if (skill && advance_timer)
        return;
    if (level_number == 5 && actor_row == 1 &&
        (actor_room == 10 || actor_room == 7 || actor_room == 12) &&
        !check_skeleton_room())
        return;

    if (advance_chance[skill] > random_below_word(255)) {
advance:
        auto_forward();
    }
}

void far pascal maybe_block(void)
{
    unsigned char skill;
    int block;

    if (actor_kind == 10 ||
        (target_frame != 152 && target_frame != 153 && target_frame != 162))
        return;
    skill = guard_rooms[actor_room].records[guard_id][4];

    if (effect_x)
        block = quick_block_chance[skill] > random_below_word(255);
    else
        block = block_chance[skill] > random_below_word(255);
    if (block)
        auto_up();
}

void far pascal maybe_strike(void)
{
    unsigned char skill;
    int strike;

    if (target_frame == 169 || target_frame == 151)
        return;
    skill = guard_rooms[actor_room].records[guard_id][4];
    if (actor_kind != 7 && actor_kind != 8 &&
        (death_frame == 161 || death_frame == 150))
        strike = quick_strike_chance[skill] > random_below_word(255);
    else
        strike = strike_chance[skill] > random_below_word(255);
    if (strike)
        set_overlay_state_bytes(254);
}
