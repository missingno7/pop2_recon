#pragma pack(1)
struct moving_rock {
    int x;
    int y;
    unsigned char room;
    char field_5[2];
    int mode;
    char field_9;
    signed char row;
    int screen_x;
};

struct rock_rect {
    int top, left, bottom, right;
};

struct character_state {
    char kind;
    char direction;
    int x;
    int y;
    char knocked_back;
    unsigned int sequence;
    signed char column;
    signed char row;
    unsigned char action_state;
    char field_12;
    char contact_state;
    unsigned char room;
    char field_15;
    char grounded;
    signed char fall_speed;
    unsigned char fall_damage;
    char padding_19[17];
    int action;
    char padding_38[26];
};
#pragma pack()

extern int bounded_random(int);
extern signed char FirstInRow(signed char);
extern signed char get_char_col(int);
extern int IsWall(unsigned char);
extern void draw_moving_tile(int);
extern signed char find_tile_row(int);
extern unsigned char tile_column(int, signed char);
extern int check_rock_column(unsigned char, unsigned char);
extern void play_movement_sound(unsigned char);
extern void load_kid_state(void);
extern void far _fastcall draw_movement_shape(unsigned char, int, int, int, unsigned char);
extern int probe_floors_toward_rock(signed char, int);
extern void hit_character_with_rock(void);
extern struct slide_rock *far _fastcall slide_rock_on_row(unsigned char, signed char);

extern int moving_x, moving_y, moving_mode;
extern unsigned int movement_end;
extern unsigned char moving_kind, moving_room;
extern signed char moving_row;
extern unsigned char current_tile_column;
extern int sequence_a, sequence_b, rock_speed_a, rock_speed_b;
static struct {
    int bolt_y_2, bolt_x_2;
    int bolt_y_3, bolt_x_3;
    int bolt_y_1;
    int trapdoor_x[17];
    int trapdoor_y[17];
    int trapdoor_shape[18];
} temple_draw_offsets = {
    -42, 39, -11, 40, -31,
    { 51, 5, 5, 16, 16, 16, 16, 9, 9, -5, 0, 5, 16, 9, -5, 0, 5 },
    { 0, 7, 7, 12, 12, 12, 12, 7, 7, -2, 0, 7, 12, 7, -1, 0, 7 },
    { 0, 91, 91, 92, 92, 92, 92, 93, 93, 94, 21, 91, 92, 93, 94, 21, 91, 21 }
};
extern struct character_state active_character, kid_state;
extern unsigned char slide_det_flags[];
extern unsigned char room_foregrounds[];
extern int tile_right_edges[], outside_barrier;
extern int screen_left, screen_right, screen_tile_x, screen_tile_y, screen_rock_x;
extern unsigned char view_room;

int choose_trapdoor_delay(int active)
{
    int delay;
    if (active)
        delay = 0;
    else
        delay = bounded_random(2) + 1;
    return delay;
}

void move_slide_bolt(void)
{
    signed char row = moving_row;
    int end = movement_end;
    if (end > moving_x) {
        do {
            current_tile_column = FirstInRow(row) + moving_x / 32;
            if (moving_kind == 8)
                ++current_tile_column;
            draw_moving_tile(moving_y);
            moving_x += 24;
        } while (moving_x < end);
    }
}

void move_slide_rock(void)
{
    int x, y, sequence, old_speed;
    signed char row;
    if (moving_mode != -1) {
        row = find_tile_row(moving_y);
        current_tile_column = tile_column(moving_x / 32, row);
        if (check_rock_column(moving_room, current_tile_column)) {
            sequence = sequence_b;
            if (!probe_floors_toward_rock(-1, 52)) {
                y = moving_y;
                moving_y -= 59;
                old_speed = rock_speed_a;
                rock_speed_a = (int)(movement_end - moving_x) <= old_speed ? (int)(movement_end - moving_x) : old_speed;
                rock_speed_b = (int)(movement_end - moving_x) <= rock_speed_b ? (int)(movement_end - moving_x) : rock_speed_b;
                moving_x += 5;
                sequence_b = sequence_a = 9;
                play_movement_sound(2);
                moving_y = y;
                moving_x -= 5;
                rock_speed_b = rock_speed_a = old_speed;
                sequence_b = sequence_a = sequence;
            } else
                sequence_b = sequence_a = 58;
            moving_kind = 7;
            draw_moving_tile(moving_y);
            x = moving_x;
            moving_x += 32;
            y = moving_y;
            if (movement_end > (unsigned int)moving_x) {
                current_tile_column = FirstInRow(row) + moving_x / 32;
                moving_kind = 6;
                --moving_y;
                draw_moving_tile(moving_y);
            }
            moving_x = temple_draw_offsets.trapdoor_x[0] + x;
            if ((unsigned int)moving_x < movement_end) {
                moving_y = temple_draw_offsets.bolt_y_1 + y;
                moving_kind = 8;
                move_slide_bolt();
            }
            moving_x = temple_draw_offsets.bolt_x_2 + x;
            if ((unsigned int)moving_x < movement_end) {
                moving_y = temple_draw_offsets.bolt_y_2 + y;
                moving_kind = 9;
                move_slide_bolt();
                moving_x = temple_draw_offsets.bolt_x_3 + x;
                moving_y = temple_draw_offsets.bolt_y_3 + y;
                move_slide_bolt();
            }
            moving_x = x;
            moving_y = y;
            moving_kind = 6;
            sequence_b = sequence_a = sequence;
        }
    }
}

int can_slide_rock_stop(void)
{
    int result = 0;
    if (probe_floors_toward_rock(-1, 32))
        result = probe_floors_toward_rock(0, 32);
    return result;
}

struct rock_rect *compute_slide_rock_rect(struct rock_rect *rect, unsigned char room, signed char row)
{
    struct moving_rock *rock = (struct moving_rock *)slide_rock_on_row(room, row);
    if (rock) {
        rect->top = rock->y - 69;
        rect->left = rock->x;
        rect->bottom = rock->y + 1;
        rect->right = rect->left + 52;
    } else
        rect = 0;
    return rect;
}

void check_slide_rock_crush(void)
{
    int kid_x, rock_x;
    load_kid_state();
    kid_x = active_character.direction == 0 ? active_character.x : active_character.x + 13;
    if (moving_room == active_character.room && moving_row == active_character.row) {
        rock_x = moving_x + 140;
        if (active_character.action == 4 || (rock_x < kid_x && rock_x + 16 > kid_x)) {
            hit_character_with_rock();
            kid_state = active_character;
        }
    }
}

void draw_object_slide_rock(unsigned char kind)
{
    struct moving_rock *rock;
    int x, frame;
    rock = (struct moving_rock *)slide_rock_on_row(view_room, screen_tile_y / 63);
    screen_rock_x = x = rock ? rock->screen_x : 320;
    x -= screen_tile_x;
    switch (kind) {
    case 134: frame = 79; break;
    case 135: frame = 80; break;
    case 136: frame = x % 5 + 86; break;
    case 137: frame = x % 5 + 81; break;
    default: frame = -1; break;
    }
    if (frame != -1)
        draw_movement_shape(4, frame, screen_tile_x, screen_tile_y, 10);
}

int slide_rock_barrier(signed char direction, unsigned char room, signed char row)
{
    struct moving_rock *rock = (struct moving_rock *)slide_rock_on_row(room, row);
    int barrier;
    signed char column;
    unsigned char tile;

    if (rock) {
        barrier = rock->x + 140;
        if (direction == -1) {
            int right;
            barrier -= 3;
            column = get_char_col(barrier - 32);
            tile = room_foregrounds[(unsigned char)30 * rock->room +
                                    (signed char)(FirstInRow(rock->row) + column)];
            if (IsWall(tile) || tile == 4) {
                right = tile_right_edges[column] + 24;
                if (active_character.direction == 0)
                    right += screen_right - screen_left;
                barrier = right >= barrier ? right : barrier;
            }
        } else
            barrier += 35;
    } else
        barrier = direction == -1 ? outside_barrier : 0;
    return barrier;
}

void slide_rock_collision_row(signed char row, unsigned char *rooms, unsigned char *flags)
{
    struct moving_rock *rock;
    int left;
    struct {
        unsigned char *neighbor_flags;
        int right;
    } edge;
    signed char column;

    rock = (struct moving_rock *)slide_rock_on_row(active_character.room, row);
    if (rock) {
        left = rock->x + 137;
        if (left < screen_right) {
            column = get_char_col(left);
            if (column >= 0 && column < 10) {
                flags[column] |= 0x0f;
                rooms[column] = rock->room;
            }
            if (rock->mode != 100 && row == active_character.row) {
                signed char neighbor = get_char_col(left + 1);
                if (neighbor != column) {
                    edge.neighbor_flags = &slide_det_flags[neighbor];
                    slide_det_flags[column] |= *edge.neighbor_flags & 0x0f;
                    if (!(flags[neighbor] & 0x0f))
                        *edge.neighbor_flags &= 0xf0;
                }
            }
        }
        edge.right = left + 38;
        if (edge.right > screen_left) {
            column = get_char_col(edge.right);
            if (column >= 0 && column < 10) {
                flags[column] |= 0xf0;
                rooms[column] = rock->room;
            }
            if (rock->mode != 100 && row == active_character.row) {
                signed char neighbor = get_char_col(edge.right + 1);
                if (neighbor != column) {
                    edge.neighbor_flags = &slide_det_flags[neighbor];
                    slide_det_flags[column] |= *edge.neighbor_flags & 0xf0;
                    if (!(flags[neighbor] & 0xf0))
                        *edge.neighbor_flags &= 0x0f;
                }
            }
        }
    }
}

struct slide_rock {
    char unused[4];
    unsigned char room;
    char middle[5];
    signed char row;
};

extern struct slide_rock *find_moving_record(unsigned char kind, int index);

struct slide_rock *far _fastcall slide_rock_on_row(unsigned char room, signed char row)
{
    struct slide_rock *rock;
    int finished = 0;
    int index = 1;

    do {
        rock = find_moving_record(6, index++);
        if (!rock || (rock->room == room && rock->row == row))
            finished = 1;
    } while (!finished);
    return rock;
}

extern unsigned char floor_at(unsigned char, signed char, signed char);
extern int floor_supports_rock(unsigned char);
extern void get_tile_knockback(unsigned char);
extern void start_sound(int);
extern void alter_timer(int);
extern void mark_kid_contact(unsigned char);
extern void reset_kid_collision(void);
extern void set_character_sequence(int);
extern void update_kid_contact(void);
extern int rock_motion_state;


void hit_character_with_rock(void)
{
    int sequence;
    unsigned char tile;
    signed char column;
    int x;
    if (active_character.action_state != 5) {
        sequence = -1;
        if (active_character.x - 144 > moving_x) {
            active_character.x = moving_x + 140;
            if (active_character.direction == -1)
                active_character.x -= 13;
        }
        x = active_character.x - 8;
        if (active_character.direction == 0)
            x -= 13;
        column = get_char_col(x);
        tile = floor_at(active_character.room, column, active_character.row);
        if (!IsWall(tile) && tile != 4) {
            if (active_character.fall_speed < 0) {
                if (active_character.grounded != 1)
                    sequence = active_character.direction == 0 ? 47 : 48;
                else {
                    sequence = 93;
                    active_character.grounded = 0;
                }
            } else
                active_character.x -= 8;
        } else {
            active_character.x = tile_right_edges[column] + 8;
            if (active_character.direction == 0)
                active_character.x += 17;
            if (tile != 2) {
                if (rock_motion_state == 100) {
                    if (active_character.fall_speed < 0) {
                        get_tile_knockback(active_character.fall_damage);
                        active_character.fall_speed = 0;
                    }
                    active_character.sequence = 185;
                } else {
                    x = moving_x - 10;
                    column = x / 32;
                    tile = floor_at(active_character.room, column, active_character.row);
                    if (!IsWall(tile) && tile != 4) {
                        if (active_character.fall_speed < 0) {
                            int other_column;
                            x -= 5;
                            other_column = x / 32;
                            if (other_column != column) {
                                tile = floor_at(active_character.room, other_column, active_character.row);
                                if (IsWall(tile) || tile == 4)
                                    start_sound(1);
                            }
                        }
                    } else {
                        if (active_character.fall_speed < 0) {
                            alter_timer(10001);
                            start_sound(55);
                        }
                        mark_kid_contact(13);
                    }
                    active_character.action = 4;
                    sequence = 71;
                }
            }
        }
        if (sequence != -1) {
            reset_kid_collision();
            active_character.contact_state = 0;
            set_character_sequence(sequence);
            update_kid_contact();
        }
    }
}

int probe_floors_toward_rock(signed char row_delta, int step)
{
    int result = 1;
    signed char first = (moving_x + 6) / 32;
    signed char last, row;
    unsigned char tile;
    step = (unsigned int)step < movement_end - moving_x ? step : movement_end - moving_x;
    last = (step + moving_x) / 32;
    row = moving_row + row_delta;
    if (last >= first) {
        do {
            if (!result)
                break;
            tile = floor_at(moving_room, first++, row);
            if (floor_supports_rock(tile) || tile == 25)
                result = 0;
            else
                result = 1;
        } while (last >= first);
    }
    return result;
}

#pragma pack(1)
struct room_links {
    unsigned char left, right, up, down;
};
struct tile_state {
    char kind;
    unsigned short state;
    signed char cell;
    unsigned char room;
    signed char column, row;
    char rest[4];
};
struct tile_animation {
    unsigned short state;
    char rest[2];
};
struct trapdoor_bounds {
    int top, left;
};
#pragma pack()

extern struct room_links room_links[];
extern struct tile_state tile_state;
extern struct tile_animation tile_animations[][30];
extern unsigned short *room_animation_states;
extern unsigned char *room_trapdoor_flags;
extern int indexed_timer_running(int);
extern int current_sound;
extern void rock_hit_spikes(void);
extern void character_release_grip(void);
extern int probe_chain_floor(int);
extern int room_edge_x[], row_y[];
extern unsigned char view_left, view_right, view_up, view_down;
extern int trapdoor_sheet;
extern int trapdoor_top, trapdoor_left;
extern void add_moving_record(void);
extern unsigned char current_modifier;
extern unsigned char level_flags;
extern unsigned char inscription_marker;
extern unsigned char room_tile_marker;
extern void play_room_sound(int);
extern void prepare_temple_event(void);
extern unsigned long shape_bounds(int sheet, int shape, struct trapdoor_bounds far *bounds);
extern void build_trapdoor_rect(struct rock_rect *rect);
extern void trapdoor_update_rect(struct rock_rect *rect);
extern void dispatch_trapdoor_update(void (far pascal *update)(struct rock_rect *),
                                    struct rock_rect *rect, signed char flags);
extern void trigger_trapdoor(unsigned char room, signed char cell);

void move_rock_record(void)
{
    signed char column, new_column, prev_column;
    unsigned char tile;
    int x, next_x, prior_column;

    if (rock_motion_state == 0 || rock_motion_state == 100)
        return;
    x = moving_x;
    if (rock_motion_state == -1)
        --x;
    else if (can_slide_rock_stop() && rock_motion_state >= 8) {
        rock_motion_state = 0;
        alter_timer(10054);
        if (current_sound == 54)
            current_sound = -1;
    } else {
        moving_x -= 8;
        if (can_slide_rock_stop())
            start_sound(53);
        moving_x = x--;
        if (rock_motion_state <= 8)
            ++rock_motion_state;
    }
    if (x < 0) {
        moving_room = room_links[moving_room].left;
        x += 320;
    }
    next_x = x;
    if (!moving_room)
        goto no_room;
    if (rock_motion_state == 0 || rock_motion_state == 100)
        goto done;
    prior_column = next_x - 20;
    column = prior_column / 32;
    if (prior_column <= 0)
        --column;
    tile = floor_at(moving_room, column, moving_row);
    if (moving_mode != 1)
        goto advance;
    moving_mode = 0;
    rock_motion_state = 100;
    ++moving_y;
    moving_x = next_x + 2;
    if (tile != 2)
        goto finish;
    load_kid_state();
    rock_hit_spikes();
    goto copy_kid;
no_room:
    moving_mode = -1;
    goto stop_timer;
advance:
    new_column = (next_x - 4) / 32;
    prev_column = (moving_x - 4) / 32;
    if (new_column != prev_column || column != prev_column) {
        column = floor_at(moving_room, new_column, moving_row);
        if ((IsWall(column) || column == 4 || tile == 2) && moving_mode == 0) {
            moving_mode = 1;
            --moving_y;
            if (!indexed_timer_running(10055) && tile != 2)
                start_sound(8);
        }
    }
    load_kid_state();
    if (active_character.sequence >= 87 && active_character.sequence <= 99) {
        column = (next_x - 8) / 32;
        if (moving_row - active_character.row + 1 == 0 && column == active_character.column)
            character_release_grip();
    }
copy_kid:
    kid_state = active_character;
finish:
    if (rock_motion_state != 100) {
        moving_x = next_x;
        if (rock_motion_state != 0)
            start_sound(54);
        return;
    }
stop_timer:
    alter_timer(10054);
done:;
}

int slide_rock_near_spikes(void)
{
    struct tile_state saved = tile_state;
    int result;
    floor_at(tile_state.room, tile_state.column + 1, tile_state.row);
    result = probe_chain_floor(16);
    if (!result) {
        floor_at(tile_state.room, tile_state.column + 2, tile_state.row);
        result = probe_chain_floor(16);
    }
    tile_state = saved;
    return result;
}

int probe_chain_floor(int margin)
{
    int result = 0;
    int rock, left, right;
    rock = slide_rock_barrier(0, tile_state.room, tile_state.row);
    if (rock) {
        left = tile_state.column * 32;
        right = left + 32;
        rock -= 140;
        if (left - margin + 32 > rock || right - margin + 32 < rock) {
            rock = slide_rock_barrier(-1, tile_state.room, tile_state.row) - 140;
            if (left - margin <= rock && right - margin >= rock)
                result = 1;
        } else
            result = 1;
    }
    return result;
}

int far _fastcall get_slide_rock_x(unsigned char room, signed char row)
{
    int x = slide_rock_barrier(0, room, row);
    if (x && active_character.x < x)
        x = slide_rock_barrier(-1, room, row);
    return x;
}

void trigger_slide_rock(unsigned char room, signed char cell, unsigned char action)
{
    signed char row = cell / (signed char)10;
    struct moving_rock *rock = (struct moving_rock *)slide_rock_on_row(room, row);
    if (rock) {
        if (action == 1) {
            if (rock->mode >= 0 && rock->mode != 100)
                rock->mode = -1;
        } else if (action == 2 && rock->mode == -1)
            rock->mode = 1;
    } else if (action == 1) {
        int x;
        floor_at(room, cell % (signed char)10, row);
        rock_motion_state = -1;
        moving_mode = 0;
        x = room_edge_x[tile_state.column];
        if (x > 320 && room == view_left) {
            room = view_room;
            x -= 320;
        }
        movement_end = moving_x = x;
        moving_room = room;
        moving_y = row_y[tile_state.row] - 5;
        moving_row = row;
        moving_kind = 6;
        add_moving_record();
    }
}

void move_trapdoor(void)
{
    int x = moving_x, y = moving_y;
    unsigned char room = moving_room;
    signed char column;
    signed char absolute_column;
    int step, saved_x, saved_y;
    struct trapdoor_bounds bounds;
    struct rock_rect rect;

    if (room != view_room) {
        if (room == view_down)
            y += 191;
        else if (moving_room == view_up)
            y -= 191;
        else if (moving_room == view_left)
            x -= 320;
        else if (moving_room == view_right)
            x += 320;
        else
            goto draw;
        room = view_room;
    }
draw:
    column = x / 32;
    current_tile_column = tile_column(column, y / 63);
    if (current_tile_column != 30) {
        if (column)
            --current_tile_column;
        if ((signed char)current_tile_column < 20)
            current_tile_column += 10;
    }
    absolute_column = FirstInRow(moving_row) + moving_x / 32;
    step = movement_end & 31;
    if (step > 16) {
        moving_mode = -1;
        tile_animations[moving_room][absolute_column].state &= 0xff00;
        room_foregrounds[(unsigned char)30 * *(volatile unsigned char *)&moving_room + absolute_column] = 26;
        step = 17;
        alter_timer(10095);
        if (current_sound == 95)
            current_sound = -1;
        start_sound(3);
    }
    if (check_rock_column(room, FirstInRow(moving_row) + column)) {
        saved_y = moving_y;
        moving_y += temple_draw_offsets.trapdoor_y[step];
        y += temple_draw_offsets.trapdoor_y[step];
        saved_x = moving_x;
        x += temple_draw_offsets.trapdoor_x[step];
        moving_x = x;
        if (shape_bounds(trapdoor_sheet, temple_draw_offsets.trapdoor_shape[step] - 1, &bounds)) {
            int left_edge;
            trapdoor_top = bounds.top + 1;
            left_edge = column * 32 - moving_x + 57;
            ++bounds.left;
            if (left_edge > bounds.left)
                bounds.left = left_edge;
            trapdoor_left = bounds.left;
        }
        play_movement_sound(0);
        if (moving_mode != -1) {
            build_trapdoor_rect(&rect);
            rect.left += 16;
            dispatch_trapdoor_update(trapdoor_update_rect, &rect, -1);
            draw_moving_tile(y);
        }
        moving_x = saved_x;
        moving_y = saved_y;
    }
}

void move_trapdoor_record(void)
{
    if (moving_mode != -1) {
        if ((signed char)(movement_end & 31) <= 16)
            ++movement_end;
        if (!indexed_timer_running(10094) && !indexed_timer_running(10095))
            start_sound(95);
    }
}

void check_trapdoor(void)
{
    if ((level_flags & 0x40) &&
        (signed char)63 * active_character.row - active_character.y == -56 &&
        active_character.knocked_back != 1 &&
        floor_at(active_character.room, active_character.column, active_character.row) == 26 &&
        !(tile_state.state & 31))
        trigger_trapdoor(tile_state.room, tile_state.cell);
}

void shake_trapdoor(void)
{
    if (!(tile_state.state & 0x40)) {
        if (tile_state.cell % (signed char)10 != active_character.column) {
            struct tile_animation *animation = (struct tile_animation *)room_animation_states;
            tile_state.state |= 0x40;
            animation[tile_state.cell].state = tile_state.state;
        }
        trigger_trapdoor(tile_state.room, tile_state.cell);
    }
}

void trigger_trapdoor(unsigned char room, signed char cell)
{
    tile_animations[room][cell].state = 0x81;
    room_trapdoor_flags[cell] = 0;
    moving_mode = rock_motion_state = 0;
    moving_x = room_edge_x[cell % (signed char)10];
    movement_end = (tile_state.state & 0x40) ? 75 : 1;
    moving_room = room;
    moving_y = row_y[tile_state.row];
    moving_row = cell / (signed char)10;
    moving_kind = 10;
    add_moving_record();
    start_sound(94);
    play_movement_sound(0);
}

void draw_object_trapdoor(unsigned char modifier)
{
    int shape = temple_draw_offsets.trapdoor_shape[current_modifier & 31];
    if (shape == 21) {
        draw_movement_shape(4, shape, screen_tile_x + 1, screen_tile_y - 2, 10);
        draw_movement_shape(4, 22, screen_tile_x, screen_tile_y - 1, 0);
    } else
        draw_movement_shape(4, shape, screen_tile_x, screen_tile_y - 1, 10);
}

void load_inscription_room(void)
{
    if ((signed char)inscription_marker >= 9)
        play_room_sound(92);
}

void check_temple_event(void)
{
    if (view_room == 4 && room_tile_marker == 13)
        prepare_temple_event();
}

void temple_frame(void)
{
}
