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
extern unsigned char death_count, guard_kind, game_variant;
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
    } else if (kind == 0 && game_variant != 6) {
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
#define guard_id (Char.bytes[0])
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
    signed char index, limit, live;
    int someone_blocks;
    GuardActor saved;
    int result;

    if (game_variant == 5)
        result = actor_room == 19 || actor_room == 16 ||
            (tile_is_solid(get_tile_below()) && actor_facing == -1);
    else
        result = 0;

    if (!result && room_can_tumble(actor_room)) {
        live = count_live_opponents();
        someone_blocks = random_below(3) <= live;
        if (!someone_blocks && live != 0) {
            saved = Char;
            limit = guard_rooms[room_to_check].count;
            index = 0;
            if (index < limit) {
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

    live = 0;
    limit = guard_rooms[room_to_check].count;
    index = 0;
    if (index < limit) {
        do {
            if (actor_slots[index].bytes[17] >= 0)
                ++live;
        } while (++index < limit);
    }
    return live;
}

extern unsigned char target_visible, level_number;
extern signed char target_life;
extern int effect_y, guard_counter;
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

    if (effect_y == 0) {
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

#pragma pack(1)
typedef struct {
    signed char cell;
    int x;
    signed char facing;
    unsigned char state;
    int sequence;
    int position;
    unsigned char reserved[4];
    signed char motion, secondary_motion;
    unsigned char kind, health;
    int shake_delay;
    int timer;
    signed char row;
    unsigned char shakes;
} Opponent;
#pragma pack()


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
            try_opponent_run_jump(); return;
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
extern int effect_y;

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
                auto_forward();
                return;
            }
            tile = get_tile_below();
            if (guard_can_cross(tile, cell_modifier) && distance > 0) {
                auto_back();
                return;
            }
            if (level_number != 5 || actor_row != 1 ||
                (actor_room != 10 && actor_room != 7 && actor_room != 12)) {
                auto_forward();
                return;
            }
            if (!check_skeleton_room())
                goto done;
            auto_forward();
            return;
        }
        check_guard_range(distance);
        return;
    }

    if (effect_y != 0)
        goto done;
    if (target_facing == actor_facing) {
        guard_no_jump();
        return;
    }
    if (pose >= 7 && pose < 15) {
        if (distance < 71) {
            set_overlay_state_bytes(254);
        }
        return;
    }
    if (pose < 34 || pose >= 44) {
        guard_no_jump();
        return;
    }
    if (distance < 94) {
        set_overlay_state_bytes(254);
        return;
    }
    goto done;

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

#define SLOT_ACTOR(offset) (*(GuardActor near *)((char near *)actor_slots + (offset)))

void far pascal wait_en_garde(void)
{
    signed char tile;
    volatile signed char slot;
    unsigned int other_offset;
    int spacing, distance, can_stand;

    slot = actor_in_direction(~actor_facing);
    if (slot == -1)
        slot = actor_in_direction(1);
    if (slot == -1)
        goto done;
    if (slot != guard_id) {
        can_stand = 0;
        if (guard_can_cross(tile = get_tile_ahead(1), cell_modifier) &&
            guard_can_cross(tile = get_tile_ahead(2), cell_modifier) &&
            guard_can_cross(tile = get_tile_ahead(3), cell_modifier))
            can_stand = 1;

        other_offset = slot * sizeof(GuardActor);
        distance = abs(ACTOR_WORD(Char, 2) - ACTOR_WORD(SLOT_ACTOR(other_offset), 2));
        spacing = opponent_spacing();
        if (spacing + 24 < distance ||
            (SLOT_ACTOR(other_offset).bytes[1] == actor_facing && ACTOR_WORD(SLOT_ACTOR(other_offset), 25) == 84))
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
        if (((ACTOR_WORD(SLOT_ACTOR(other_offset), 25) == 84 && SLOT_ACTOR(other_offset).bytes[1] == actor_facing) ||
             (target_facing == actor_facing && RUNNING_FRAME(target_frame) && target_action != 7)) &&
            steps)
            goto start_run;
        auto_forward();
        return;
    }
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

static const unsigned int strike_chance[12] =
    {75, 100, 75, 75, 75, 50, 100, 220, 0, 60, 40, 60};
static const unsigned int quick_strike_chance[12] =
    {0, 0, 0, 5, 5, 175, 20, 10, 0, 255, 255, 150};
static const unsigned int block_chance[12] =
    {0, 150, 150, 200, 200, 255, 200, 250, 0, 255, 255, 255};
static const unsigned int quick_block_chance[12] =
    {0, 75, 75, 100, 100, 145, 100, 250, 0, 145, 255, 175};
static const unsigned int advance_chance[12] =
    {255, 200, 200, 200, 255, 255, 200, 0, 0, 255, 100, 100};
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

    if (actor_kind == 10 ||
        (target_frame != 152 && target_frame != 153 && target_frame != 162))
        return;
    skill = guard_rooms[actor_room].records[guard_id][4];

    if (effect_x) {
        if (quick_block_chance[skill] > random_below_word(255))
            goto act;
    } else if (block_chance[skill] > random_below_word(255)) {
act:
        auto_up();
    }
}

void far pascal maybe_strike(void)
{
    unsigned char skill;

    if (target_frame == 169 || target_frame == 151)
        return;
    skill = guard_rooms[actor_room].records[guard_id][4];
    if (actor_kind != 7 && actor_kind != 8 &&
        (death_frame == 161 || death_frame == 150)) {
        if (quick_strike_chance[skill] > random_below_word(255))
            goto act;
    } else if (strike_chance[skill] > random_below_word(255)) {
act:
        set_overlay_state_bytes(254);
    }
}

extern int far pascal tile_is_unsafe(signed char tile);
extern int far pascal barrier_blocks_guard(void);
extern int far pascal skeleton_room_gate(void);
extern void far pascal advance_skeleton(void);
extern void far pascal save_actor_slot(void);
extern void far pascal set_opponent_life(Opponent near *opponent);
extern void far pascal ClearCharVel(void);
extern signed char pending_slot;

int far pascal skeleton_finished_shaking(signed char id);
void far pascal bones_fall(void);
void far pascal face_kid_for_skeleton(void);

int far pascal opponent_close(signed char slot, signed char column, signed char row, signed char radius)
{
    signed char tile, limit, step;
    int clear = 0;
    Opponent near *opponent = GetOppInfoIdx(room_to_check, slot);

    if (!opponent || (signed char)death_count < 0 || ACTOR_WORD(Char, 36) == 5)
        goto done;
    if (opponent->timer && !IsExitChasingSkel(actor_room))
        goto done;

    if (*(volatile signed char near *)&actor_column > column) {
        limit = actor_column + 1;
        step = 1;
    } else {
        limit = actor_column - 1;
        step = -1;
    }

    if (actor_row == row && column - radius <= actor_column && column + radius >= actor_column) {
        do {
            tile = read_tile(actor_room, column, row);
            if ((!tile_is_ledge(tile) || actor_kind == 10) &&
                !tile_is_unsafe(tile) && (tile != 4 || !barrier_blocks_guard()))
                clear = 1;
            else
                clear = 0;
            column += step;
        } while (clear && limit != column);
    }
done:
    return clear;
}

void far pascal raise_nearby_opponents(void)
{
    signed char index, count, radius;
    int started;
    Opponent near *opponent;

    if (IsExitChasingSkel((unsigned char)Kid.bytes[14]))
        radius = 8;
    else if (guard_kind == 2)
        radius = 4;
    else
        radius = 5;

    count = guard_rooms[room_to_check].count;
    index = 0;
    if (count > 0) {
        do {
            load_actor_slot(index);
            if (kid_life < 0 && Kid.bytes[11] != 4 && Kid.bytes[11] != 3 &&
                Kid.bytes[11] != 5 && opponent_close(index, kid_column, kid_row, radius)) {
                opponent = GetOppInfoIdx(actor_room, guard_id);
                if (guard_kind == 2) {
                    if (opponent->shake_delay != -1) {
                        started = opponent->timer;
                        if (skeleton_finished_shaking(guard_id)) {
                            select_animation_sequence(88);
                            started = 1;
                            actor_kind = 4;
                        } else {
                            if (!started && ACTOR_WORD(Char, 25) != 112)
                                select_animation_sequence(112);
                            started = 0;
                        }
                    }
                } else {
                    select_animation_sequence(106);
                    started = 1;
                    actor_kind = 10;
                }
                AnimChar();
                if (started) {
                    set_opponent_life(opponent);
                    death_count = (unsigned char)-1;
                    running_state = 1;
                    alert_state = 3;
                    if (actor_kind == 10) {
                        signed char facing;
                        if (ACTOR_WORD(Char, 2) >= ACTOR_WORD(Kid, 2))
                            facing = -1;
                        else
                            facing = 0;
                        actor_facing = facing;
                    }
                    *(volatile int near *)&guard_counter = 0;
                    *(volatile int near *)&effect_y = 0;
                    ClearCharVel();
                    if (pending_slot == -1)
                        pending_slot = guard_id;
                }
            }
            save_actor_slot();
        } while (++index < count);
    }
}

void far pascal update_skeleton(void)
{
    int active;
    Opponent near *opponent;

    if ((signed char)death_count < 0) {
        active = 1;
        if (level_number == 5 && actor_row == 1 &&
            (actor_room == 10 || actor_room == 7 || actor_room == 12))
            active = skeleton_room_gate();
        else if (kid_life >= 5 && actor_kind == 4 && ACTOR_WORD(Char, 25) != 120) {
            bones_fall();
            active = 0;
        }

        if (active) {
            if (ACTOR_WORD(Char, 25) == 88 && death_frame == 177) {
                face_kid_for_skeleton();
                return;
            }
            if (active) {
                advance_skeleton();
                return;
            }
        }
    } else {
        opponent = GetOppInfoIdx(actor_room, guard_id);
        if (opponent && opponent->timer > 0)
            --opponent->timer;
    }
}



typedef GuardActor Actor;
#define room_info guard_rooms
extern unsigned char guard_room, guard_kind, skeleton_kind;
extern Opponent near *far pascal GetOppInfoIdx(int room, int id);
extern void far pascal load_actor_slot(signed char id);
extern void far pascal select_animation_sequence(int sequence);
extern void far pascal save_actor_slot(void);

extern signed char far pascal tile_below(void);
extern int far pascal tile_is_solid(signed char tile);
extern int far pascal edge_distance(void);
extern int far pascal offset_char_x(int amount);
extern void far pascal play_sound(int id);
extern void far pascal set_guard_life(signed char id, int flags, int health);
void far pascal set_skeleton_timer(signed char id, int duration);
void far pascal set_skeleton_shakes(signed char id);

void far pascal bones_fall(void)
{
    int distance;

    select_animation_sequence(120);
    play_sound(76);
    Char.bytes[15] = 1;
    Char.bytes[16] = 0;
    if (Char.bytes[17] < 0)
        Char.bytes[17] = 0;

    if (Char.bytes[1] == 0 && tile_is_solid(tile_below())) {
        distance = 32 - edge_distance();
        if (distance <= 35) {
            distance -= 35;
            *(int near *)&Char.bytes[2] = offset_char_x(-distance);
        }
    }
    save_actor_slot();
    set_guard_life(Char.bytes[0], 0, Char.bytes[19]);
    set_skeleton_timer(Char.bytes[0], 120);
    set_skeleton_shakes(Char.bytes[0]);
    load_actor_slot(Char.bytes[0]);
}

#define STATE_LEVEL (*(volatile unsigned char near *)&level_number)

int far pascal IsExitChasingSkel(unsigned char value)
{
    int result = 0;

    if (STATE_LEVEL == 4 && value >= 22 && value <= 28 && value != 23) {
        result = 1;
    }
    return result;
}

void far pascal set_skeleton_shakes(signed char id)
{
    Opponent near *opponent = GetOppInfoIdx(guard_room, id);
    if (opponent) {
        if (opponent->state)
            opponent->shakes = (opponent->state + 1) % 2;
        else
            opponent->shakes = 3;
    }
}

void far pascal set_skeleton_timer(signed char id, int duration)
{
    Opponent near *opponent;
    if (guard_kind == 2 || skeleton_kind == 10) {
        opponent = GetOppInfoIdx(guard_room, id);
        if (opponent)
            opponent->timer = duration;
    }
}

#define CW(n) (*(int near *)&Char.bytes[n])

void far pascal shake_bones(unsigned char room, signed char row)
{
    signed char index, count;
    Actor saved;
    Opponent near *opponent;

    saved = Char;
    index = 0;
    count = room_info[room].count;
    if (count > 0) {
        do {
            load_actor_slot(index);
            if (Char.bytes[10] == row && CW(7) == 206 && CW(36) != 5) {
                opponent = GetOppInfoIdx((unsigned char)Char.bytes[14], Char.bytes[0]);
                if (opponent->shake_delay == 0) {
                    select_animation_sequence(112);
                    save_actor_slot();
                }
            }
        } while (++index < count);
    }
    Char = saved;
}

extern signed char far pascal tile_in_front(signed char distance);
extern signed char far pascal tile_below(void);
extern int far pascal tile_is_solid(signed char tile);
extern int far pascal edge_distance(void);
extern int far pascal offset_char_x(int amount);
extern int far pascal decrement_life(signed char amount);
extern void far pascal kill_current_skeleton(void);
extern void far pascal play_sound(int id);

Opponent near *far pascal initialize_skeleton(Opponent near *opponent)
{
    int distance;

    if (opponent->shake_delay == 1) {
        opponent->sequence = 63;
        Char.bytes[16] = 1;
    } else {
        if (Char.bytes[0] == 0 && opponent->shake_delay == 0)
            opponent->sequence = 112;
        else
            opponent->sequence = 119;
        opponent->timer = opponent->shake_delay;
        set_skeleton_shakes(Char.bytes[0]);
        Char.bytes[16] = 0;

        if ((Char.bytes[1] == -1 && tile_is_solid(tile_in_front(1))) ||
            (Char.bytes[1] == 0 && tile_is_solid(tile_below()))) {
            distance = edge_distance();
            if (Char.bytes[1] == 0)
                distance = 32 - distance;
            if (distance <= 35) {
                distance = 35 - distance;
                if (Char.bytes[1] == -1)
                    distance = -distance;
                CW(2) = offset_char_x(distance);
            }
        }
    }
    opponent->position = 0;
    CW(52) = 8;
    return opponent;
}

void far pascal impale_skeleton(void)
{
    if (decrement_life(1)) {
        kill_current_skeleton();
        return;
    }
    select_animation_sequence(126);
    play_sound(72);
}


extern signed char far pascal read_tile(unsigned char room, signed char column, signed char row);
extern int far pascal IsExitOpen(void);
extern int far pascal sequence_available(int sequence);
extern void far pascal request_if_unset(int request);
#define random_below random_below_word

int far pascal skeleton_finished_shaking(signed char id)
{
    int result;
    int reset_timer;
    Opponent near *opponent = GetOppInfoIdx(guard_room, id);

    if (opponent) {
        result = 0;
        reset_timer = result;
        if (opponent->shake_delay != -1) {
            if (IsExitChasingSkel(Char.bytes[14])) {
                read_tile(22, 4, 2);
                if (IsExitOpen() && Kid.bytes[14] == Char.bytes[14] &&
                    *(int near *)&Kid.bytes[2] < CW(2)) {
                    if (!sequence_available(10106))
                        request_if_unset(106);
                    result = 1;
                } else {
                    reset_timer = opponent->timer == 0 && CW(25) == 112;
                }
            } else if (opponent->shakes != 0) {
                --opponent->shakes;
                reset_timer = 1;
            } else {
                result = 1;
            }
        }
        if (reset_timer)
            set_skeleton_timer(id, random_below(30) + 15);
    }
    return result;
}


typedef struct {
    int frame;
    signed char x;
    signed char y;
} SpriteFrame;

extern Actor Char, Kid;
extern unsigned int sprite_frame_index, sprite_table_handle;
extern int sprite_y, sprite_frame;
#define flame_offset (Char.bytes[58])
extern int far pascal offset_char_x(int delta);
extern int far pascal offset_sprite_x(int delta);
extern SpriteFrame far *far pascal resolve_sprite_table(unsigned int handle);

#define ACTOR_X(actor) (*(int near *)&(actor).bytes[2])

void far pascal face_kid_for_skeleton(void)
{
    if (ACTOR_X(Kid) > ACTOR_X(Char)) {
        if (Char.bytes[1] == 0)
            return;
        Char.bytes[1] = 0;
    } else {
        if (Char.bytes[1] == -1)
            return;
        Char.bytes[1] = -1;
    }
    ACTOR_X(Char) = offset_char_x(10);
}

void far pascal setup_flame_sword(void)
{
    SpriteFrame far *table = resolve_sprite_table(sprite_table_handle);
    offset_sprite_x(table[sprite_frame_index].x);
    sprite_y += table[sprite_frame_index].y;

    if (sprite_frame == 4 || sprite_frame == 19 || sprite_frame == 20) {
        if (flame_offset != 0)
            sprite_frame = 18 - flame_offset;
        if (flame_offset <= -4) {
            flame_offset = 0;
            return;
        }
        --flame_offset;
    }
}


extern void far pascal play_sound(int sound);
extern void far pascal reset_char_y(void);
extern int far pascal decrement_life(signed char points);
extern void far pascal select_animation_sequence(int sequence);
extern void far pascal set_skeleton_timer(signed char id, int duration);
extern void far pascal request_if_unset(int effect);
int far pascal reduce_flame_sword_life(signed char points);
void far pascal drop_flame_sword(void);

int far pascal block_flame_sword(void)
{
    int sequence = reduce_flame_sword_life(1) ? -1 : 103;
    play_sound(12);
    reset_char_y();
    return sequence;
}

int far pascal reduce_flame_sword_life(signed char points)
{
    int result;
    if (decrement_life(points)) {
        drop_flame_sword();
        result = 1;
    } else {
        result = 0;
    }
    return result;
}

void far pascal drop_flame_sword(void)
{
    select_animation_sequence(107);
    set_skeleton_timer(Char.bytes[0], 240);
    request_if_unset(192);
    Char.bytes[17] = 0;
    Char.bytes[15] = 1;
}





extern signed char room_column[];
extern int guard_counter, effect_x, effect_y;

extern signed char far pascal get_col(int x);
extern signed char far pascal FirstInRow(signed char row);
extern void far pascal reset_char_y(void);
extern void far pascal select_animation_sequence(int sequence);
extern void far pascal request_if_unset(int effect);
extern void far pascal set_opponent_life(Opponent near *opponent);
extern void far pascal ClearCharVel(void);
extern void far pascal save_actor_slot(void);
extern void far pascal copy_kid_to_char(void);

#define CHAR_WORD(n) (*(int near *)&Char.bytes[n])
#define KID_WORD(n) (*(int near *)&Kid.bytes[n])



extern void far pascal remove_char(void);
extern SpriteFrame far *far pascal resolve_sprite_table(unsigned int handle);
extern int far pascal offset_sprite_x(int x);
extern void far pascal make_sprite_clip(void);
extern void far _fastcall set_sprite_style(unsigned char style);
typedef void (far pascal *SmokeDrawer)(void);
extern void far pascal draw_smoke(void);
extern void far pascal submit_guard_sprite(SmokeDrawer draw, void near *clip, signed char id);
#define smoke_frame sprite_frame_index
#define smoke_table_handle sprite_table_handle
extern int sprite_frame, sprite_y;
extern unsigned char sprite_style, sprite_clip[];

extern void far pascal offset_sprite_clip(void far *destination, void far *source, int dx, int dy);

void far pascal float_flame_sword(void)
{
    volatile int row_y;
    int amount, drift, dy;

    if (sprite_frame == 4 || sprite_frame == 19 || sprite_frame == 20) {
        amount = 4;
        row_y = Char.bytes[10] * 63;
        if (row_y + 54 < CW(4)) {
            drift = -2;
            if (row_y + 58 > CW(4))
                goto apply_drift;
        } else {
            drift = 1;
        }
        amount = 1;
apply_drift:

        dy = random_below(amount) + drift;
        CW(4) += dy;
        sprite_y += dy;
        offset_sprite_clip(sprite_clip, sprite_clip, 0, dy);
    }
}

void far pascal check_jinnee_column(void)
{
    if (Char.bytes[9] < -1 || Char.bytes[9] > 10)
        remove_char();
}

void far pascal setup_jinnee_smoke(void)
{
    SpriteFrame far *table;
    SpriteFrame far *smoke;
    unsigned int frame;

    if (smoke_frame >= 171 && smoke_frame <= 199) {
        frame = smoke_frame;
        table = resolve_sprite_table(smoke_table_handle);
        smoke = &table[frame];
        if (smoke->frame != -1) {
            sprite_frame = smoke->frame;
            offset_sprite_x(smoke->x);
            sprite_y += smoke->y;
            sprite_style = 3;
            make_sprite_clip();
            set_sprite_style(9);
            submit_guard_sprite(draw_smoke, sprite_clip, Char.bytes[0]);
        }
    }
}

void far pascal TriggerJinnee(void)
{
    Opponent near *opponent;
    signed char column;
    signed char row;
    int x;

    Kid = Char;
    x = 85 * room_column[Kid.bytes[1]] + KID_WORD(2);
    column = get_col(x);
    row = Kid.bytes[10];
    opponent = GetOppInfoIdx((unsigned char)Kid.bytes[14],
                            Char.bytes[0] = room_info[(unsigned char)Kid.bytes[14]].count++);
    Char.bytes[6] = 12;
    opponent->kind = 9;
    Char.bytes[14] = Kid.bytes[14];
    Char.bytes[10] = row;
    opponent->cell = FirstInRow(row) + column;
    reset_char_y();
    CHAR_WORD(2) = opponent->x = x;
    Char.bytes[9] = column;
    Char.bytes[1] = opponent->facing = -1;
    column = 0;
    opponent->motion = column;
    Char.bytes[56] = column;
    opponent->secondary_motion = column;
    Char.bytes[57] = column;
    Char.bytes[35] = column;
    (*(volatile int near *)&Char.bytes[42]) = 0;
    CHAR_WORD(36) = CHAR_WORD(40) = CHAR_WORD(38) = 0;
    CHAR_WORD(52) = 4;
    Char.bytes[15] = 1;
    Char.bytes[16] = 1;
    opponent->sequence = 180;
    opponent->position = 0;
    select_animation_sequence(180);
    request_if_unset(196);
    opponent->health = 6;
    set_opponent_life(opponent);
    opponent->state = 10;
    Char.bytes[17] = -1;
    guard_counter = 0;
    effect_x = 0;
    effect_y = 0;
    ClearCharVel();
    Char.bytes[11] = 1;
    save_actor_slot();
    copy_kid_to_char();
}



