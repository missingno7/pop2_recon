/* AUTOHEAD.C. Routines in original DOS order. */

#pragma pack(1)
typedef struct {
    int x, y;
    unsigned char kind;
    int frame;
    int step;
    unsigned char reserved[6];
    unsigned char opponent_kind, padding;
    signed char opponent_mode;
} HeadSprite;

typedef struct {
    signed char width, height, top, left;
} SpriteBounds;

typedef struct {
    signed char x, y;
} NeckPoint;
#pragma pack()

extern signed char actor_facing, body_facing;
extern unsigned char actor_kind;
extern int actor_x, sprite_x, sprite_y, kid_sheet, kid_frame;
extern void near *kid_shapes;

extern HeadSprite near *far pascal find_sprite(unsigned char kind, unsigned char index);
extern NeckPoint far *far pascal lock_shape_data(void near *handle);
extern void far pascal sprite_dimensions(int sheet, int frame, SpriteBounds far *bounds);
extern void far pascal unlock_shape_data(void near *handle);

typedef struct {
    signed char mode, column, row, direction;
    unsigned char flags, sound_played;
} HeadPlan;

typedef struct {
    unsigned char prefix[15], kind, reserved;
    signed char mode, column, row;
    int timer;
    unsigned char sound_played;
} OpponentRecord;

extern struct { signed char column; } actor_column;
#define actor_column actor_column.column
extern signed char target_column, target_row, target_life, target_facing;
extern signed char actor_row, actor_life;
extern signed char actor_velocity_x, actor_velocity_y, actor_alert, head_active;
extern unsigned char actor_room, level_head_kind, target_visible, tile_flags, target_room;
extern signed char actor_slot;
extern signed char camera_actor;
extern unsigned char tile_modifier;
extern unsigned int actor_frame, target_frame, target_progress;
extern int actor_y, target_x, actor_sequence, actor_timer;
extern signed char tile_test_x;
extern int current_sprite_frame, snake_distance;
extern unsigned char collision_room, collision_column, collision_row;
extern unsigned char room_opponents[][116];
static const signed char first_chomp[12] = {
    1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 99
};
static const signed char last_chomp[12] = {
    1, 1, 2, 2, 2, 3, 3, 3, 3, 4, 4, 99
};
static const int flee_length[12] = {
    70, 65, 60, 55, 50, 45, 40, 35, 30, 25, 20, 15
};
extern int cell_x[];
extern signed char move_with_facing[], move_against_facing[];

extern signed char far pascal read_tile(unsigned char room, signed char column, signed char row);
extern int far pascal tile_blocks_head(signed char tile);
extern int far pascal tile_allows_vertical(signed char tile);
extern int far pascal GetBarrCode(unsigned char tile);
extern int far pascal head_has_target(void);
extern int far pascal gate_blocks_head(void);
extern int far pascal bounded_random(int maximum);
extern void far pascal reset_char_y(void);
extern void far pascal clear_char_velocity(void);
extern void far pascal play_sound(int sound);
extern void far pascal play_loop_sound(int sound);
extern int far pascal sound_is_playing(int sound);
extern int far pascal lose_life(signed char amount);
extern int far pascal offset_char_x(int amount);
extern void far pascal select_animation_sequence(int sequence);
extern int far pascal target_wall_gap(unsigned char room, unsigned char column, unsigned char row);
extern signed char far pascal actor_x_to_column(void);
extern signed char far pascal random_open_column(int room);
extern int far pascal get_move_room(void);
extern int far pascal target_distance(void);
extern void far pascal clear_actor_motion(void);
extern void far pascal set_actor_speed(signed char id, int x, int y);
extern void far pascal refresh_actor_movement(signed char id);
extern OpponentRecord near *far pascal get_opponent_info(int room, int id);
extern void far pascal load_actor_slot(signed char id);
extern void far pascal store_actor_slot(void);
extern void far pascal advance_actor_frame(void);
extern void far pascal update_actor_sprite(void);
extern signed char far pascal get_cells_ahead(signed char direction);
extern int far pascal tile_blocks_snake(signed char tile);
extern signed char far pascal last_clear_column(unsigned char room, signed char column,
                                                signed char row, signed char direction);
extern signed char far pascal tile_under_actor(void);
extern int far pascal distance_into_cell(void);

int far pascal head_clear_in_dir(int direction);
int far pascal head_go_to_dest(HeadPlan near *plan);
void far pascal head_face_kid(void);
void far pascal head_get_angry(HeadPlan near *plan);
void far pascal head_get_peaceful(HeadPlan near *plan);
void far pascal head_focus_on_neck(HeadPlan near *plan);
int far pascal head_set_to_flee_mode(void);
int far pascal is_able_to_flee_in_room(unsigned char room);
int far pascal just_start_vertical_seq(void);
int far pascal XWithinMiddleOfCell(int first, register int second);
int far pascal YWithinMiddleOfCell(int value, register int half_width);
int far pascal snake_slithering_toward_opp(void);
void far pascal snake_attack(OpponentRecord near *record);
void far pascal snake_come_out_of_hole(OpponentRecord near *record);
void far pascal snake_go_into_hole(OpponentRecord near *record);

int abs(int value);
#pragma intrinsic(abs)

void far pascal unused_head_entry(void)
{
}

void far pascal align_head_on_kid_neck(void)
{
    signed char neck_x;
    volatile SpriteBounds bounds;
    register HeadSprite near *body;
    register NeckPoint far *neck;

    body = find_sprite(2, 0);
    if (body == 0)
        return;

    neck = lock_shape_data(kid_shapes);
    if (body_facing != actor_facing)
        neck_x = neck[body->frame].y;
    else {
        sprite_dimensions(kid_sheet, kid_frame, (SpriteBounds far *)&bounds);
        neck_x = -((actor_kind == 7 ? 10 : 16) + neck[body->frame].y + bounds.top);
    }
    if (actor_kind == 8)
        neck_x += 6;
    if (actor_facing == 0)
        neck_x = -neck_x;
    sprite_x = body->x + neck_x;
    sprite_y = body->y + ((signed char far *)neck)[body->frame * 2];
    unlock_shape_data(kid_shapes);
    actor_x = sprite_x + 130;
}

int far pascal head_visibility(void)
{
    signed char column, row, error, columns, end_column, diagonal, rows;
    signed char step_row, step_column, end_row;
    int clear = 1;
    int result;

    if (target_column == actor_column && target_row == actor_row) {
        result = 4;
        goto done;
    }

    row = actor_row * 2;
    column = actor_column * 2;
    if ((actor_x + tile_test_x - 122) % 32 > 16)
        ++column;
    end_row = target_row * 2 + 1;
    end_column = target_column * 2;
    if ((target_x - 130) % 32 > 16)
        ++end_column;

    {
        int delta = end_row - row;
        rows = delta < 0 ? -delta : delta;
    }
    if (column < end_column)
        step_column = 1;
    else
        step_column = -1;
    if (row < end_row)
        step_row = 1;
    else
        step_row = -1;
    {
        int delta = end_column - column;
        columns = delta < 0 ? -delta : delta;
    }

    if (rows < columns) {
        error = rows * 2 - columns;
        end_column = rows * 2;
        diagonal = (rows - columns) * 2;
        do {
            column += step_column;
            if ((step_column > 0 && column % 2 == 0) ||
                (step_column < 0 && column % 2 != 0))
                clear = !tile_blocks_head(read_tile(actor_room, column / 2, row / 2));
            if (clear) {
                if (error < 0)
                    error += end_column;
                else {
                    if ((step_row > 0 && row % 2 != 0) ||
                        (step_row < 0 && row % 2 == 0)) {
                        if (step_row < 0)
                            clear = tile_allows_vertical(read_tile(actor_room, column / 2, (row - 1) / 2));
                        else
                            clear = tile_allows_vertical(read_tile(actor_room, column / 2, row / 2));
                    }
                    row += step_row;
                    error += diagonal;
                }
            }
        } while (--columns && clear);
    } else {
        error = columns * 2 - rows;
        end_column = columns * 2;
        diagonal = (columns - rows) * 2;
        do {
            if ((step_row > 0 && row % 2 != 0) ||
                (step_row < 0 && row % 2 == 0)) {
                if (step_row < 0)
                    clear = tile_allows_vertical(read_tile(actor_room, column / 2, (row - 1) / 2));
                else
                    clear = tile_allows_vertical(read_tile(actor_room, column / 2, row / 2));
            }
            if (clear) {
                row += step_row;
                if (error < 0)
                    error += end_column;
                else {
                    column += step_column;
                    if ((step_column > 0 && column % 2 == 0) ||
                        (step_column < 0 && column % 2 != 0))
                        clear = !tile_blocks_head(read_tile(actor_room, column / 2, row / 2));
                    error += diagonal;
                }
            }
        } while (--rows && clear);
    }
    if (clear) {
        if (!head_has_target())
            result = 4;
        else if (target_row != actor_row)
            result = step_row > 0 ? 2 : 3;
        else
            result = step_column > 0 ? 1 : 0;
    } else
        result = -1;
done:
    return result;
}

int far pascal chomp_again(HeadPlan near *plan)
{
    unsigned char room_skill = room_opponents[actor_room][actor_slot * 23 + 5];
    int result;
    signed char column;

    if (plan->column < first_chomp[room_skill])
        result = 1;
    else if (last_chomp[room_skill] <= (column = plan->column))
        result = 0;
    else
        result = bounded_random(1);
    return result;
}

HeadSprite near *far pascal head_add_opponent(HeadSprite near *sprite)
{
    if (sprite->opponent_kind == 0) {
        sprite->opponent_kind = 5;
        actor_kind = level_head_kind;
        reset_char_y();
    }
    if (actor_kind == 11) {
        if (actor_life < 0) {
            if (sprite->opponent_mode <= 0) {
                sprite->frame = 170;
                sprite->step = 0;
                sprite->opponent_mode = -1;
            } else {
                sprite->frame = 175;
                sprite->step = 0;
                actor_timer = 6;
            }
            head_active = 1;
            play_sound(71);
        } else
            head_active = 0;
    } else {
        actor_x = cell_x[actor_column] + 30;
        actor_facing = -1;
        sprite->frame = 130;
        sprite->step = 0;
        sprite->opponent_mode = head_active = 0;
    }
    return sprite;
}

void far pascal head_attack_prog(HeadPlan near *plan)
{
    unsigned char dx, dy;
    int direction, vertical_sequence;
    if (actor_sequence == 140 || actor_sequence == 152 ||
        actor_sequence == 156 || actor_sequence == 162)
        return;
    if (actor_sequence >= 147 && actor_sequence <= 151) {
        head_focus_on_neck(plan);
        return;
    }
    if (actor_frame != 150 && actor_frame != 165 && actor_frame != 166 &&
        actor_frame != 158 && actor_frame != 173)
        return;
    direction = head_visibility();
    if (direction != -1) {
        plan->direction = 1;
        {
            int delta = actor_column - target_column;
            dx = delta < 0 ? -delta : delta;
        }
        {
            int delta = actor_row - target_row;
            dy = delta < 0 ? -delta : delta;
        }
        if (dy == 0 && dx < 2) {
            vertical_sequence = actor_x < target_x ? 1 : 0;
            if (dx == 0 || head_clear_in_dir(vertical_sequence)) {
                if (actor_sequence != 147)
                    select_animation_sequence(147);
            } else
                goto stop_horizontal;
        } else if (dx == 0 && dy == 1 && head_clear_in_dir(direction)) {
            vertical_sequence = target_row < actor_row ? 143 : 144;
            if (vertical_sequence != actor_sequence)
                select_animation_sequence(vertical_sequence);
        } else if (direction == 4) {
            if (dx || dy) {
stop_horizontal:
                if (actor_sequence != 166 && YWithinMiddleOfCell(actor_y, 15))
                    select_animation_sequence(166);
            }
        } else {
            plan->row = target_row;
            plan->column = target_column;
            if ((direction == 3 || direction == 2) && !head_clear_in_dir(direction))
                direction = target_x > actor_x;
            if (head_clear_in_dir(direction)) {
                if (direction == 3)
                    direction = 143;
                else if (direction == 2)
                    direction = 144;
                else {
                    direction = 142;
                    head_face_kid();
                    reset_char_y();
                }
            } else {
                direction = 166;
                clear_char_velocity();
            }
            if (actor_sequence != direction)
                select_animation_sequence(direction);
        }
    } else {
        if (plan->direction != 0) {
            plan->direction = 0;
            plan->column = target_column;
            plan->row = target_row;
        }
        head_go_to_dest(plan);
    }
}

int far pascal head_blocked(void)
{
    register int sequence;
    if (lose_life(1))
        sequence = 154;
    else {
        play_sound(87);
        head_set_to_flee_mode();
        sequence = 153;
    }
    return sequence;
}

int far pascal head_clear_in_dir(int direction)
{
    signed char tile;
    int clear = 0;
    switch (direction) {
    case 0:
        if (actor_column < -1 || just_start_vertical_seq())
            break;
        tile = read_tile(actor_room, actor_column - 1, actor_row);
        if (tile_blocks_head(tile))
            break;
        if (tile == 4 && gate_blocks_head())
            break;
        clear = 1;
        break;
    case 1:
        if (actor_column > 10 || just_start_vertical_seq())
            break;
        tile = read_tile(actor_room, actor_column + 1, actor_row);
        if (tile_blocks_head(tile))
            break;
        if (tile == 4 && gate_blocks_head())
            break;
        if (read_tile(actor_room, actor_column, actor_row) != 4 || !gate_blocks_head())
            clear = 1;
        break;
    case 2:
        if (actor_row != 2 && XWithinMiddleOfCell(actor_x, 8))
            clear = tile_allows_vertical(read_tile(actor_room, actor_column, actor_row));
        break;
    case 3:
        if (actor_row != 0 && XWithinMiddleOfCell(actor_x, 8))
            clear = tile_allows_vertical(read_tile(actor_room, actor_column, actor_row - 1));
        break;
    }
    return clear;
}

void far pascal head_done_attacking_kid(HeadPlan near *plan)
{
    head_get_peaceful(plan);
    actor_timer = 0;
}

int far pascal head_enemy_collide(int minimum_gap)
{
    int sequence = -1;
    int gap;

    if (actor_timer != 1) {
        gap = target_wall_gap(collision_room, collision_column, collision_row);
        if (gap < 5 && minimum_gap <= gap && gap < 5 && -snake_distance <= gap) {
            actor_x = offset_char_x(-(gap + 14));
            if (actor_life < 0 && actor_sequence != 153)
                sequence = 157;
            else if (actor_sequence != 164) {
                if (actor_frame <= 191) {
                    sequence = 164;
                    actor_x = cell_x[actor_x_to_column()] + 14;
                    if (actor_facing == -1)
                        actor_x += 32;
                } else
                    select_animation_sequence(145);
                lose_life(100);
            }
        }
    }
    return sequence;
}

void far pascal head_face_kid(void)
{
    signed char column = actor_column;
    if (column < target_column) {
        if (actor_facing != 0) {
            actor_x = offset_char_x(-14);
            actor_facing = 0;
        }
    } else if (actor_facing != -1) {
        actor_x = offset_char_x(-14);
        actor_facing = -1;
    }
}

void far pascal head_flee_prog(HeadPlan near *plan)
{
    if (actor_sequence != 153) {
        if (actor_sequence == 159) {
            if (plan->column == actor_column && XWithinMiddleOfCell(actor_x, 2)) {
                if (actor_row == target_row && abs(actor_column - target_column) < 3)
                    head_get_angry(plan);
                else {
                    select_animation_sequence(160);
                    *(int near *)&plan->direction = flee_length[room_opponents[actor_room][actor_slot * 23 + 5]];
                }
            }
        } else if (actor_row == target_row && abs(actor_column - target_column) < 3 &&
                   target_visible && (target_frame != 70 || (target_progress != 0 && target_progress < 223)))
            head_get_angry(plan);
        else if (--*(int near *)&plan->direction == 0) {
            if (target_visible && (target_frame != 70 || (target_progress != 0 && target_progress < 223)) &&
                head_visibility() != -1) {
                head_get_angry(plan);
                select_animation_sequence(162);
            } else {
                head_get_peaceful(plan);
                select_animation_sequence(163);
            }
        }
        if (!plan->sound_played && !sound_is_playing(10087)) {
            plan->sound_played = 1;
            play_loop_sound(146);
        }
    } else if (actor_timer == 2) {
        actor_alert = 0;
        select_animation_sequence(159);
    }
}

void far pascal head_focus_on_neck(HeadPlan near *plan)
{
    int low, high, distance;
    if (actor_sequence == 147 || actor_sequence == 144 || actor_sequence == 143) {
        if (target_facing == 1) {
            low = -25;
            high = 0;
        } else {
            low = -3;
            high = 30;
        }
        distance = target_distance();
        if (actor_row == target_row && low <= distance && high >= distance) {
            clear_char_velocity();
            select_animation_sequence(bounded_random(1) + 150);
            plan->mode = 3;
            plan->column = 1;
        } else {
            if ((target_facing == 1 ? -35 : -13) > distance && actor_sequence == 147) {
                clear_char_velocity();
                select_animation_sequence(156);
            }
            if (distance - 64 <= high && actor_row == target_row && actor_sequence != 144)
                play_sound(90);
        }
    }
}

void far pascal head_get_angry(HeadPlan near *plan)
{
    plan->mode = 1;
    plan->row = target_row;
    plan->column = target_column;
    plan->direction = 1;
    select_animation_sequence(140);
    clear_char_velocity();
    head_face_kid();
    actor_alert = 3;
    head_active = 1;
    if (camera_actor == -1) {
        clear_actor_motion();
        set_actor_speed(actor_slot, actor_velocity_x, actor_velocity_y);
        refresh_actor_movement(actor_slot);
        camera_actor = actor_slot;
    }
    play_sound(87);
}

void far pascal head_get_peaceful(HeadPlan near *plan)
{
    signed char zero = 0;
    plan->mode = zero;
    head_active = zero;
    clear_char_velocity();
    select_animation_sequence(139);
    actor_column = random_open_column(get_move_room());
    actor_x = cell_x[actor_column] + 30;
    reset_char_y();
    actor_alert = 0;
}

int far pascal head_go_to_dest(HeadPlan near *plan)
{
    register int clear = 0;
    register int sequence;
    int direction;
    if (actor_column != plan->column || plan->row != actor_row) {
        if (plan->row != actor_row) {
            if (plan->row > actor_row) {
                direction = 2;
                sequence = 144;
            } else {
                direction = 3;
                sequence = 143;
            }
            clear = head_clear_in_dir(direction);
            if (clear && actor_sequence != sequence)
                select_animation_sequence(sequence);
        }
        if (!clear && actor_column != plan->column) {
            if (plan->column > actor_column)
                sequence = 1;
            else
                sequence = 0;
            clear = head_clear_in_dir(sequence);
            if (clear && actor_sequence != 142)
                select_animation_sequence(142);
        }
    }
    if (!clear)
        head_get_peaceful(*(HeadPlan near *volatile *)&plan);
    return clear;
}

void far pascal head_locked_prog(HeadPlan near *plan)
{
    int lost = 0;
    register OpponentRecord near *record;
    if ((actor_sequence == 150 && actor_frame == 167 && actor_timer != 1) ||
        (actor_sequence == 151 && actor_frame == 166)) {
        if (chomp_again(plan)) {
            if (target_life < 0) {
                ++plan->column;
                select_animation_sequence(150 + (actor_sequence == 150));
            } else {
                record = get_opponent_info(actor_room, actor_slot);
                if (record && record->prefix[4] >= 11)
                    select_animation_sequence(165);
            }
        } else {
            select_animation_sequence(152);
            actor_x = offset_char_x(-8);
            lost = 1;
        }
    }
    if (lost || actor_sequence == 158 || actor_sequence == 157) {
        plan->mode = 1;
        plan->column = target_column;
        plan->row = target_row;
    }
}

void far pascal unused_head_alert(void)
{
}

void far pascal head_peaceful_prog(HeadPlan near *plan)
{
    int direction, sequence;
    if (actor_frame != 150)
        return;
    direction = bounded_random(3);
    if (direction == 3) {
        if (actor_sequence >= 131 && actor_sequence <= 133)
            direction = 0;
        else if (actor_sequence >= 134 && actor_sequence <= 136)
            direction = 1;
        else
            direction = -1;
    } else if (direction == 2)
        direction += bounded_random(1);
    if (direction != -1 && head_clear_in_dir(direction)) {
        if (direction == 3)
            sequence = 137;
        else if (direction == 2)
            sequence = 138;
        else if (direction == 0)
            sequence = bounded_random(2) + 131;
        else
            sequence = bounded_random(2) + 134;
        select_animation_sequence(sequence);
    }
}

void far pascal head_prog(void)
{
    OpponentRecord near *record;
    HeadPlan near *plan;
    if (actor_life >= 0)
        return;
    record = get_opponent_info(actor_room, actor_slot);
    if (!record)
        return;
    plan = (HeadPlan near *)&record->mode;
    load_actor_slot(actor_slot);
    advance_actor_frame();
    if (current_sprite_frame != -1) {
        kid_frame = current_sprite_frame;
        update_actor_sprite();
    }
    switch (plan->mode) {
    case 0:
        if (target_visible && (target_frame != 70 || (target_progress != 0 && target_progress < 223)) &&
            (actor_frame == 150 || actor_frame == 165 || actor_frame == 166 ||
             actor_frame == 158 || actor_frame == 173) && head_visibility() != -1)
            head_get_angry(plan);
        else
            head_peaceful_prog(plan);
        break;
    case 1:
        if (target_visible && (target_frame != 70 || (target_progress != 0 && target_progress < 223)))
            head_attack_prog(plan);
        else
            head_done_attacking_kid(plan);
        break;
    case 2:
        head_flee_prog(plan);
        break;
    case 3:
        if (target_visible && (target_frame != 70 || (target_progress != 0 && target_progress < 223)))
            head_locked_prog(plan);
        else
            head_done_attacking_kid(plan);
        break;
    }
    store_actor_slot();
}

int far pascal head_set_to_flee_mode(void)
{
    unsigned char skill;
    signed char column;
    int clear, edge, initial_column;
    OpponentRecord near *record;

    skill = room_opponents[actor_room][actor_slot * 23 + 5];
    if (!is_able_to_flee_in_room(actor_room) || bounded_random(skill / 3 + 1) != 0 ||
        !((actor_facing == 0 && actor_column >= 3) ||
          (actor_facing == -1 && actor_column + 3 <= 10)))
        goto cannot_flee;
    clear = 1;
    initial_column = *(volatile signed char *)&column = actor_column;
    edge = actor_facing == 0 ? -1 : 10;
    if (initial_column != edge)
        do {
            if (!clear)
                break;
            skill = read_tile(actor_room, column, actor_row);
            if (GetBarrCode(skill) && (skill != 4 || gate_blocks_head()))
                clear = 0;
            column += move_against_facing[actor_facing];
        } while (column != edge);
    if (!clear) {
        column += move_with_facing[actor_facing] * 2;
        if (abs(column - actor_column) >= 3)
            clear = 1;
    } else
        column += move_with_facing[actor_facing];
    if (clear) {
        head_active = 0;
        actor_alert = 1;
        record = get_opponent_info(actor_room, actor_slot);
        record->mode = 2;
        record->column = column;
        record->row = actor_row;
        record->timer = -1;
        record->sound_played = 0;
        clear_actor_motion();
        set_actor_speed(actor_slot, 0, actor_velocity_y);
        refresh_actor_movement(actor_slot);
        if (actor_slot == camera_actor)
            camera_actor = -1;
    }
    goto done;
cannot_flee:
    clear = 0;
done:
    return clear;
}

int far pascal is_able_to_flee_in_room(unsigned char room)
{
    signed char count, slot;
    int room_number = room;
    int available;

    count = room_opponents[room_number][0];
    if (count > 1) {
        available = 1;
        slot = 0;
        while (count > slot &&
               (available = get_opponent_info(*(volatile int *)&room_number, slot)->mode != 2,
                ++slot, available))
            ;
    } else
        available = 0;
    return available;
}

int far pascal is_head_locked_on(unsigned char room, int slot)
{
    OpponentRecord near *record;
    int result;
    record = get_opponent_info(room, slot);
    result = record != 0 && record->mode == 3;
    return result;
}

int far pascal just_start_vertical_seq(void)
{
    int baseline = actor_row * 63 + 37;
    int result;
    if (actor_sequence != 144) {
        if (actor_sequence == 143)
            result = actor_y < baseline;
        else
            result = 0;
        return result;
    } else if (actor_y > baseline)
        result = 1;
    else
        result = 0;
    return result;
}

int far pascal XWithinMiddleOfCell(int first, register int second)
{
    int result;
    first = (first - 144) % 32;
    result = first >= 16 - second && 16 + second >= first;
    return result;
}

int far pascal YWithinMiddleOfCell(int value, register int half_width)
{
    int result;
    value = (value - 3) % 63;
    result = value >= 31 - half_width && 31 + half_width >= value;
    return result;
}

void far pascal snake_prog(void)
{
    OpponentRecord near *record;
    if (actor_life < 0) {
        record = get_opponent_info(actor_room, actor_slot);
        if (record) {
            load_actor_slot(actor_slot);
            if (record->mode < 0) {
                if (snake_slithering_toward_opp())
                    snake_attack(record);
                else
                    snake_go_into_hole(record);
            } else
                snake_come_out_of_hole(record);
            store_actor_slot();
        }
    }
}

void far pascal snake_attack(OpponentRecord near *record)
{
    int hit = 0;
    int distance = target_distance();
    signed char tile;
    if (distance >= 13 && distance <= 45) {
        if (actor_frame == 278 || actor_frame == 279 ||
            (actor_sequence == 176 &&
             ((target_progress >= 1 && target_progress <= 14) || (target_progress >= 49 && target_progress <= 56))))
            hit = 1;
        else if (actor_sequence != 176 && actor_sequence != 173) {
            select_animation_sequence(176);
            play_sound(71);
        }
    } else if (actor_sequence == 176 &&
               ((target_progress >= 1 && target_progress <= 14) || (target_progress >= 49 && target_progress <= 56)) &&
               distance > 13 && distance < 50)
        hit = 1;
    else if (actor_sequence == 176 && target_progress >= 135 && target_progress <= 149 &&
             distance > 13 && distance < 45)
        hit = 1;
    else if ((actor_frame == 278 || actor_frame == 279) &&
             (distance < 13 || distance > 49)) {
        tile = get_cells_ahead(1);
        if (tile_blocks_snake(tile) || (tile == 7 && (tile_flags & 128)))
            select_animation_sequence(177);
    }
    if (hit && actor_sequence != 173) {
        actor_x = offset_char_x(distance - 32);
        select_animation_sequence(173);
    }
}

void far pascal snake_come_out_of_hole(OpponentRecord near *record)
{
    signed char tile;
    signed char left, right;
    if (--record->mode > 0)
        return;
    record->mode = -1;
    left = last_clear_column(actor_room, actor_column, actor_row, -1);
    right = last_clear_column(actor_room, actor_column, actor_row, 1);
    tile = tile_under_actor();
    if (target_room == actor_room && actor_row == target_row &&
        actor_column > left && actor_column < right) {
        if (actor_column <= target_column &&
            (!tile_blocks_head(tile) || tile_blocks_snake(tile) || (tile == 7 && (tile_flags & 128))))
            actor_facing = 0;
        else
            actor_facing = -1;
    } else
        actor_facing = ~actor_facing;
    if (!tile_blocks_head(tile) || tile_blocks_snake(tile) || (tile == 7 && (tile_flags & 128)))
        goto emerge_in_place;
    select_animation_sequence(170);
    actor_x = offset_char_x(38);
    goto reset_timer;
emerge_in_place:
    select_animation_sequence(172);
reset_timer:
    actor_timer = 0;
}

void far pascal snake_go_into_hole(OpponentRecord near *record)
{
    signed char tile;
    int distance;
    if (target_life >= 0) {
        if (actor_sequence != 176 && actor_frame != 278 && actor_frame != 279)
            select_animation_sequence(176);
        return;
    }
    if (actor_sequence != 171 && actor_sequence != 172 && actor_timer != 9) {
        tile_under_actor();
        if ((tile_modifier & 15) == 12) {
            distance = (actor_x - 144) % 32;
            if (distance >= 14 && distance <= 18) {
                distance = 16 - distance;
                if (actor_facing == -1)
                    distance = -distance;
                else
                    distance += 8;
                actor_x = offset_char_x(distance);
                select_animation_sequence(171);
            }
        } else if (actor_sequence != 176 && actor_sequence != 168 &&
                   actor_frame != 278 && actor_frame != 279) {
            tile = get_cells_ahead(1);
            if (tile_blocks_snake(tile) || (tile == 7 && (tile_flags & 128)))
                return;
            if (!tile_blocks_head(tile))
                select_animation_sequence(176);
            else {
                if (tile != 7) {
                    if (actor_facing == 0) {
                        actor_timer = 9;
                        return;
                    }
                    if (actor_frame != 254)
                        return;
                }
                select_animation_sequence(168);
            }
        } else {
            tile = get_cells_ahead(1);
            if (tile_blocks_snake(tile) || (tile == 7 && (tile_flags & 128)))
                select_animation_sequence(177);
        }
    } else {
        if (actor_timer == 9) {
            tile = tile_under_actor();
            distance = distance_into_cell();
            if (tile_blocks_head(tile) && distance < 8) {
                actor_frame = 264;
                select_animation_sequence(168);
            }
        }
        if (actor_frame == 264) {
            actor_timer = 6;
            record->mode = actor_alert == 2 ? 6 : bounded_random(60) + 36;
        }
    }
}

int far pascal snake_slithering_toward_opp(void)
{
    int result = 0;
    if (actor_sequence != 171 && actor_alert >= 2) {
        if (actor_facing == 0) {
            signed char column = actor_column;
            if (target_column >= column)
                goto toward;
        }
        if (actor_facing == -1) {
            signed char column = actor_column;
            if (target_column <= column)
toward:
                result = 1;
        }
    }
    return result;
}

void far pascal snake_update_opp_info(OpponentRecord near *record)
{
    signed char tile;
    if (actor_sequence == 171 || actor_timer == 9) {
        actor_timer = 6;
        record->mode = bounded_random(60) + 36;
    } else {
        tile = get_cells_ahead(1);
        if (!tile_blocks_snake(tile) && (tile != 7 || !(tile_flags & 128)) &&
            (actor_sequence == 176 || actor_sequence == 168 || actor_sequence == 170)) {
            record->prefix[3] = ~actor_facing;
            *(int near *)&record->prefix[1] = offset_char_x(-32);
            return;
        }
    }
}
