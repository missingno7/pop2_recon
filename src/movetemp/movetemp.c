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

struct char_bounds { int top, left, bottom, right; };
typedef struct {
    signed char slot;                  /* 00 */
    signed char direction;             /* 01 */
    int x, y;                          /* 02, 04 */
    signed char character_kind;        /* 06 */
    unsigned int frame;                /* 07 */
    signed char column, row;           /* 09, 0a */
    signed char action;                /* 0b */
    signed char horizontal_step;       /* 0c */
    signed char vertical_step;         /* 0d */
    signed char room;                  /* 0e */
    signed char field_0f;              /* 0f */
    signed char stance;                /* 10 */
    signed char alive;                 /* 11 */
    signed char life_points;           /* 12 */
    signed char life_initial;          /* 13 */
    signed char life_delta;            /* 14 */
    int sequence_position;             /* 15 */
    int sequence_id;                   /* 17 */
    unsigned int sequence;             /* 19 */
    struct char_bounds bounds;                    /* 1b: top, left, bottom, right */
    signed char alert_delay;           /* 23 */
    int sequence_group;                /* 24 */
    int previous_x, previous_y;        /* 26, 28 */
    unsigned int previous_frame;       /* 2a */
    struct char_bounds previous_bounds;           /* 2c: top, left, bottom, right */
    int field_34;                      /* 34 */
    int previous_mode;                 /* 36 */
    unsigned char behavior;            /* 38 */
    signed char opponent_index;        /* 39 */
    unsigned char field_3a;            /* 3a */
    unsigned char unknown_3b[5];        /* 3b..3f */
} CHARREC;
#pragma pack()

extern int Rnd(int);
extern signed char FirstInRow(signed char row);
extern signed char get_char_col(int x);
extern int IsWall(unsigned char kind);
extern void draw_moving_tile(int);
extern signed char GetRow(int y);
extern unsigned char IndexCell1(signed char col, signed char row);
extern int is_visible_room_cell(unsigned char room, unsigned char cell);
extern void mark_mob_redraw(unsigned char);
extern void copy_kid_to_char(void);
extern void far _fastcall draw_movement_shape(unsigned char kind, int x, int y, int frame, unsigned char pass);
extern int probe_floors_toward_rock(signed char, int);
extern void hit_character_with_rock(void);
extern struct slide_rock *far _fastcall slide_rock_on_row(unsigned char, signed char);

extern unsigned int movement_end;
extern int moving_x, moving_y, moving_mode;
extern unsigned char moving_kind, moving_room;
extern signed char moving_row;
extern unsigned char FChar;
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
extern CHARREC Char, Kid;
extern unsigned char slide_det_flags[];
extern unsigned char room_foregrounds[];
extern int tile_right_edges[], outside_barrier;
extern int screen_left, screen_right, screen_tile_x, screen_tile_y, screen_rock_x;
extern unsigned char viewed_room;

int choose_trapdoor_delay(int active)
{
    int delay;
    if (active)
        delay = 0;
    else
        delay = Rnd(2) + 1;
    return delay;
}

void move_slide_bolt(void)
{
    signed char row = moving_row;
    int end = movement_end;
    if (end > moving_x) {
        do {
            FChar = FirstInRow(row) + moving_x / 32;
            if (moving_kind == 8)
                ++FChar;
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
        row = GetRow(moving_y);
        FChar = IndexCell1(moving_x / 32, row);
        if (is_visible_room_cell(moving_room, FChar)) {
            sequence = sequence_b;
            if (!probe_floors_toward_rock(-1, 52)) {
                y = moving_y;
                moving_y -= 59;
                old_speed = rock_speed_a;
                rock_speed_a = (int)(movement_end - moving_x) <= old_speed ? (int)(movement_end - moving_x) : old_speed;
                rock_speed_b = (int)(movement_end - moving_x) <= rock_speed_b ? (int)(movement_end - moving_x) : rock_speed_b;
                moving_x += 5;
                sequence_b = sequence_a = 9;
                mark_mob_redraw(2);
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
                FChar = FirstInRow(row) + moving_x / 32;
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
    copy_kid_to_char();
    kid_x = Char.direction == 0 ? Char.x : Char.x + 13;
    if (moving_room == (unsigned char)Char.room && moving_row == Char.row) {
        rock_x = moving_x + 140;
        if (Char.sequence_group == 4 || (rock_x < kid_x && rock_x + 16 > kid_x)) {
            hit_character_with_rock();
            Kid = Char;
        }
    }
}

void draw_object_slide_rock(unsigned char kind)
{
    struct moving_rock *rock;
    int x, frame;
    rock = (struct moving_rock *)slide_rock_on_row(viewed_room, screen_tile_y / 63);
    screen_rock_x = x = 320;
    if (rock)
        screen_rock_x = x = rock->screen_x;
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
                if (Char.direction == 0)
                    right += screen_right - screen_left;
                barrier = right >= barrier ? right : barrier;
            }
        } else
            barrier += 35;
    } else if (direction == -1)
        barrier = outside_barrier;
    else
        barrier = 0;
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

    rock = (struct moving_rock *)slide_rock_on_row((unsigned char)Char.room, row);
    if (rock) {
        left = rock->x + 137;
        if (left < screen_right) {
            column = get_char_col(left);
            if (column >= 0 && column < 10) {
                flags[column] |= 0x0f;
                rooms[column] = rock->room;
            }
            if (rock->mode != 100 && row == Char.row) {
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
            if (rock->mode != 100 && row == Char.row) {
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

extern struct slide_rock *get_mob_type_item(unsigned char kind, int index);

struct slide_rock *far _fastcall slide_rock_on_row(unsigned char room, signed char row)
{
    struct slide_rock *rock;
    int finished = 0;
    int index = 1;

    do {
        rock = get_mob_type_item(6, index++);
        if (!rock || (rock->room == room && rock->row == row))
            finished = 1;
    } while (!finished);
    return rock;
}

extern unsigned char ReadCell(unsigned char, signed char, signed char);
extern int IS_SIX_KIND(unsigned char);
extern void get_tile_knockback(unsigned char);
extern void AddSound(int);
extern void dispatch_sound(int);
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
    if (Char.action != 5) {
        sequence = -1;
        if (Char.x - 144 > moving_x) {
            Char.x = moving_x + 140;
            if (Char.direction == -1)
                Char.x -= 13;
        }
        x = Char.x - 8;
        if (Char.direction == 0)
            x -= 13;
        column = get_char_col(x);
        tile = ReadCell((unsigned char)Char.room, column, Char.row);
        if (!IsWall(tile) && tile != 4) {
            if (Char.alive < 0) {
                if (Char.stance != 1)
                    sequence = Char.direction == 0 ? 47 : 48;
                else {
                    sequence = 93;
                    Char.stance = 0;
                }
            } else
                Char.x -= 8;
        } else {
            Char.x = tile_right_edges[column] + 8;
            if (Char.direction == 0)
                Char.x += 17;
            if (tile != 2) {
                if (rock_motion_state == 100) {
                    if (Char.alive < 0) {
                        get_tile_knockback(Char.life_points);
                        Char.alive = 0;
                    }
                    Char.frame = 185;
                } else {
                    x = moving_x - 10;
                    column = x / 32;
                    tile = ReadCell((unsigned char)Char.room, column, Char.row);
                    if (!IsWall(tile) && tile != 4) {
                        if (Char.alive < 0) {
                            int other_column;
                            x -= 5;
                            other_column = x / 32;
                            if (other_column != column) {
                                tile = ReadCell((unsigned char)Char.room, other_column, Char.row);
                                if (IsWall(tile) || tile == 4)
                                    AddSound(1);
                            }
                        }
                    } else {
                        if (Char.alive < 0) {
                            dispatch_sound(10001);
                            AddSound(55);
                        }
                        mark_kid_contact(13);
                    }
                    Char.sequence_group = 4;
                    sequence = 71;
                }
            }
        }
        if (sequence != -1) {
            reset_kid_collision();
            Char.vertical_step = 0;
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
            tile = ReadCell(moving_room, first++, row);
            if (IS_SIX_KIND(tile) || tile == 25)
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
struct current_tile {
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
extern struct current_tile current_tile;
extern struct tile_animation tile_animations[][30];
extern unsigned short *BlueS;
extern unsigned char *room_trapdoor_flags;
extern int query_sound(int);
extern int current_sound;
extern void rock_hit_spikes(void);
extern void character_release_grip(void);
extern int probe_chain_floor(int);
extern int room_edge_x[], row_y[];
extern int background_shapes;
extern int trapdoor_top, trapdoor_left;
extern void add_mob(void);
extern unsigned char current_modifier;
extern unsigned char level_flags;
extern unsigned char room_tile_marker;
extern void AddSong(int);
extern void altar_watch_kid(void);
extern unsigned long get_set_image_size(int sheet, int shape, struct trapdoor_bounds far *bounds);
extern void compute_mob_rect(struct rock_rect *rect);
extern void trapdoor_update_rect(struct rock_rect *rect);
extern void mark_bufs_for_rect(void (far pascal *update)(struct rock_rect *),
                                    struct rock_rect *rect, signed char flags);
extern void trigger_trapdoor(unsigned char room, signed char cell);

void move_rock_record(void)
{
    int next_x, x, prior_column;
    signed char prev_column, new_column;
    unsigned char tile;
    signed char column;

    if (rock_motion_state == 0 || rock_motion_state == 100)
        return;
    x = moving_x;
    if (rock_motion_state == -1)
        --x;
    else if (can_slide_rock_stop() && rock_motion_state >= 8) {
        rock_motion_state = 0;
        dispatch_sound(10054);
        if (current_sound == 54)
            current_sound = -1;
    } else {
        moving_x -= 8;
        if (can_slide_rock_stop())
            AddSound(53);
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
    tile = ReadCell(moving_room, column, moving_row);
    if (moving_mode != 1)
        goto advance;
    moving_mode = 0;
    rock_motion_state = 100;
    ++moving_y;
    moving_x = next_x + 2;
    if (tile != 2)
        goto finish;
    copy_kid_to_char();
    rock_hit_spikes();
    goto copy_kid;
no_room:
    moving_mode = -1;
    goto stop_timer;
advance:
    new_column = (next_x - 4) / 32;
    prev_column = (moving_x - 4) / 32;
    if (new_column != prev_column || column != prev_column) {
        column = ReadCell(moving_room, new_column, moving_row);
        if ((IsWall(column) || column == 4 || tile == 2) && moving_mode == 0) {
            moving_mode = 1;
            --moving_y;
            if (!query_sound(10055) && tile != 2)
                AddSound(8);
        }
    }
    copy_kid_to_char();
    if (Char.frame >= 87 && Char.frame <= 99) {
        column = (next_x - 8) / 32;
        if (moving_row - Char.row + 1 == 0 && column == Char.column)
            character_release_grip();
    }
copy_kid:
    Kid = Char;
finish:
    if (rock_motion_state != 100) {
        moving_x = next_x;
        if (rock_motion_state != 0)
            AddSound(54);
        return;
    }
stop_timer:
    dispatch_sound(10054);
done:;
}

extern struct { unsigned char left, right, up, down; } current_view;
extern unsigned char view_left, view_right, view_up, view_down;

int slide_rock_near_spikes(void)
{
    struct current_tile saved = current_tile;
    int result;
    ReadCell(current_tile.room, current_tile.column + 1, current_tile.row);
    result = probe_chain_floor(16);
    if (!result) {
        ReadCell(current_tile.room, current_tile.column + 2, current_tile.row);
        result = probe_chain_floor(16);
    }
    current_tile = saved;
    return result;
}

int probe_chain_floor(int margin)
{
    int result = 0;
    int rock, left, right;
    rock = slide_rock_barrier(0, current_tile.room, current_tile.row);
    if (rock) {
        left = current_tile.column * 32;
        right = left + 32;
        rock -= 140;
        if (left - margin + 32 > rock || right - margin + 32 < rock) {
            rock = slide_rock_barrier(-1, current_tile.room, current_tile.row);
            if (left - margin <= (rock -= 140) && right - margin >= rock)
                result = 1;
        } else
            result = 1;
    }
    return result;
}

int far _fastcall get_slide_rock_x(unsigned char room, signed char row)
{
    int x = slide_rock_barrier(0, room, row);
    if (x && Char.x < x)
        x = slide_rock_barrier(-1, room, row);
    return x;
}

void trigger_slide_rock(unsigned char room, signed char cell, unsigned char action)
{
    struct { signed char row; signed char column; } location;
    struct moving_rock *rock;
    location.row = cell / (signed char)10;
    rock = (struct moving_rock *)slide_rock_on_row(room, location.row);
    if (rock) {
        if (action == 1) {
            if (rock->mode >= 0 && rock->mode != 100)
                rock->mode = -1;
        } else if (action == 2 && rock->mode == -1)
            rock->mode = 1;
    } else if (action == 1) {
        int x;
        location.column = cell % (signed char)10;
        ReadCell(room, location.column, location.row);
        rock_motion_state = -1;
        moving_mode = 0;
        x = room_edge_x[current_tile.column];
        if (x > 320 && (room == current_view.left)) {
            room = viewed_room;
            x -= 320;
        }
        movement_end = moving_x = x;
        moving_room = room;
        moving_y = row_y[current_tile.row] - 5;
        moving_row = location.row;
        moving_kind = 6;
        add_mob();
    }
}

extern struct { int heights[13], widths[13]; } mob_dimensions;

void move_trapdoor(void)
{
    int x = moving_x, y = moving_y;
    unsigned char room = moving_room;
    signed char column;
    signed char absolute_column;
    int step, saved_x, saved_y;
    struct trapdoor_bounds bounds;
    struct {
        signed char saved_column;
        union {
            struct rock_rect rect;
            int alignment;
        } area;
    } draw;

    if (room != viewed_room) {
        if (room == current_view.down)
            y += 191;
        else if (moving_room == current_view.up)
            y -= 191;
        else if (moving_room == current_view.left)
            x -= 320;
        else if (moving_room == current_view.right)
            x += 320;
        else
            goto draw;
        room = viewed_room;
    }
draw:
    draw.saved_column = column = x / 32;
    FChar = IndexCell1(draw.saved_column, y / 63);
    if (FChar != 30) {
        if (column)
            --FChar;
        if ((signed char)FChar < 20)
            FChar += 10;
    }
    absolute_column = FirstInRow(moving_row) + moving_x / 32;
    step = movement_end & 31;
    if (step > 16) {
        moving_mode = -1;
        tile_animations[moving_room][absolute_column].state &= 0xff00;
        room_foregrounds[(unsigned char)30 * *(volatile unsigned char *)&moving_room + absolute_column] = 26;
        step = 17;
        dispatch_sound(10095);
        if (current_sound == 95)
            current_sound = -1;
        AddSound(3);
    }
    if (is_visible_room_cell(room, FirstInRow(moving_row) + draw.saved_column)) {
        saved_y = moving_y;
        moving_y += temple_draw_offsets.trapdoor_y[step];
        y += temple_draw_offsets.trapdoor_y[step];
        saved_x = moving_x;
        x += temple_draw_offsets.trapdoor_x[step];
        moving_x = x;
        if (get_set_image_size(background_shapes, temple_draw_offsets.trapdoor_shape[step] - 1, &bounds)) {
            int left_edge;
            mob_dimensions.heights[10] = bounds.top + 1;
            left_edge = column * 32 - moving_x + 57;
            ++bounds.left;
            if (left_edge > bounds.left)
                bounds.left = left_edge;
            mob_dimensions.widths[10] = bounds.left;
        }
        mark_mob_redraw(0);
        if (moving_mode != -1) {
            compute_mob_rect(&draw.area.rect);
            draw.area.rect.left += 16;
            mark_bufs_for_rect(trapdoor_update_rect, &draw.area.rect, -1);
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
        if (!query_sound(10094) && !query_sound(10095))
            AddSound(95);
    }
}

void check_trapdoor(void)
{
    if ((level_flags & 0x40) &&
        (signed char)63 * Char.row - Char.y == -56 &&
        Char.character_kind != 1 &&
        ReadCell((unsigned char)Char.room, Char.column, Char.row) == 26 &&
        !(current_tile.state & 31))
        trigger_trapdoor(current_tile.room, current_tile.cell);
}

void shake_trapdoor(void)
{
    if (!(current_tile.state & 0x40)) {
        if (current_tile.cell % (signed char)10 != Char.column) {
            struct tile_animation *animation = (struct tile_animation *)BlueS;
            current_tile.state |= 0x40;
            animation[current_tile.cell].state = current_tile.state;
        }
        trigger_trapdoor(current_tile.room, current_tile.cell);
    }
}

void trigger_trapdoor(unsigned char room, signed char cell)
{
    tile_animations[room][cell].state = 0x81;
    room_trapdoor_flags[cell] = 0;
    moving_mode = rock_motion_state = 0;
    moving_x = room_edge_x[cell % (signed char)10];
    movement_end = (current_tile.state & 0x40) ? 75 : 1;
    moving_room = room;
    moving_y = row_y[current_tile.row];
    moving_row = cell / (signed char)10;
    moving_kind = 10;
    add_mob();
    AddSound(94);
    mark_mob_redraw(0);
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
    if ((signed char)Kid.column >= 9)
        AddSong(92);
}

void check_temple_event(void)
{
    if (viewed_room == 4 && room_tile_marker == 13)
        altar_watch_kid();
}

void temple_frame(void)
{
}
