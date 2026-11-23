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
extern void draw_moving_tile(int y);
extern signed char GetRow(int y);
extern unsigned char IndexCell1(signed char col, signed char row);
extern int is_visible_room_cell(unsigned char room, unsigned char cell);
extern void mark_mob_redraw(unsigned char cell);
extern void copy_kid_to_char(void);
extern void far _fastcall add_middle_image(unsigned char kind, int x, int y, int frame, unsigned char pass);
extern int probe_floors_toward_rock(signed char row, int row_delta);
extern void hit_character_with_rock(void);
extern struct slide_rock *far _fastcall slide_rock_on_row(unsigned char, signed char);

#pragma pack(1)
typedef struct {
    int x, y;                                         /* 00,02 */
    unsigned char room;                               /* 04 */
    int velocity, auxiliary;                          /* 05,07 */
    signed char kind, row;                             /* 09-0a */
    unsigned int flags;                               /* 0b */
} MOBREC;

typedef struct {
    signed char index, flipped;                       /* 00-01 */
    int x, y, image;                                   /* 02,04,06 */
    unsigned char image_set;                          /* 08 */
    int clip_top, clip_left, clip_bottom, clip_right;    /* 09,0b,0d,0f */
    int depth;                                        /* 11 */
    unsigned char object_type;                        /* 13 */
} FCHARREC;
#pragma pack()

extern MOBREC Mob;
extern FCHARREC FChar;
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
#pragma pack(1)
typedef struct {
    unsigned int modifier, unknown_02;
} LEVELCELL;

typedef struct {
    unsigned char left, right, above, below;
} ROOMLINKS;

typedef struct {
    signed char opponent_count;
    unsigned char opponent_data[5][23];
} ROOMOBJECTS;

typedef struct {
    signed char room, cell;
} REBIRTHPOINT;

typedef struct {
    signed char count;
    unsigned char spawn_flags;
    unsigned char header[2];
    unsigned char points[3][10];
} ROOMGENERATORS;

typedef struct {
    unsigned char cell_kinds[32][30];                  /* 0000 */
    LEVELCELL cell_info[32][30];                       /* 03c0 */
    unsigned char unknown_12c0[0x500];                 /* 12c0 */
    ROOMLINKS room_links[32];                          /* 17c0 */
    unsigned char room_count;                         /* 1840 */
    unsigned char unknown_1841[4];
    unsigned char environment_kind;                  /* 1845 */
    unsigned char unknown_1846;
    signed char level_number;                         /* 1847 */
    int background_overlays[8];                       /* 1848 */
    unsigned char unknown_1858[4];
    int actor_phase;                                  /* 185c */
    unsigned char unknown_185e[2];
    signed char entry_room, entry_cell, entry_facing;    /* 1860-1862 */
    unsigned char unknown_1863[3];
    unsigned char preferred_opponent;                /* 1866 */
    ROOMOBJECTS room_objects[32];                      /* 1867 */
    REBIRTHPOINT rebirth_points[2];                    /* 26e7 */
    unsigned char unknown_26eb[0x0e];
    ROOMGENERATORS generators[32];                     /* 26f9 */
    unsigned char unknown_2b39[0x3c0];                 /* 2b39-2ef8 */
} LEVELREC;
#pragma pack()

#pragma pack(1)
typedef struct {
    unsigned char number;                             /* 00 */
    unsigned char left, right, above, below;           /* 01-04 */
    unsigned char above_left, above_right;             /* 05-06 */
    unsigned char below_left, below_right;             /* 07-08 */
    unsigned char custom;                             /* 09 */
} ROOMREC;
#pragma pack()

extern ROOMREC Room;
extern LEVELREC Level;
extern int ColEdgeX[20];
extern int screen_left, screen_right;


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
    signed char row = Mob.row;
    int end = Mob.flags;
    if (end > Mob.x) {
        do {
            FChar.index = FirstInRow(row) + Mob.x / 32;
            if (Mob.kind == 8)
                ++FChar.index;
            draw_moving_tile(Mob.y);
            Mob.x += 24;
        } while (Mob.x < end);
    }
}

void move_slide_rock(void)
{
    int x, y, sequence, old_speed;
    signed char row;
    if (Mob.velocity != -1) {
        row = GetRow(Mob.y);
        FChar.index = IndexCell1(Mob.x / 32, row);
        if (is_visible_room_cell(Mob.room, (unsigned char)FChar.index)) {
            sequence = sequence_b;
            if (!probe_floors_toward_rock(-1, 52)) {
                y = Mob.y;
                Mob.y -= 59;
                old_speed = rock_speed_a;
                rock_speed_a = (int)(Mob.flags - Mob.x) <= old_speed ? (int)(Mob.flags - Mob.x) : old_speed;
                rock_speed_b = (int)(Mob.flags - Mob.x) <= rock_speed_b ? (int)(Mob.flags - Mob.x) : rock_speed_b;
                Mob.x += 5;
                sequence_b = sequence_a = 9;
                mark_mob_redraw(2);
                Mob.y = y;
                Mob.x -= 5;
                rock_speed_b = rock_speed_a = old_speed;
                sequence_b = sequence_a = sequence;
            } else
                sequence_b = sequence_a = 58;
            Mob.kind = 7;
            draw_moving_tile(Mob.y);
            x = Mob.x;
            Mob.x += 32;
            y = Mob.y;
            if (Mob.flags > (unsigned int)Mob.x) {
                FChar.index = FirstInRow(row) + Mob.x / 32;
                Mob.kind = 6;
                --Mob.y;
                draw_moving_tile(Mob.y);
            }
            Mob.x = temple_draw_offsets.trapdoor_x[0] + x;
            if ((unsigned int)Mob.x < Mob.flags) {
                Mob.y = temple_draw_offsets.bolt_y_1 + y;
                Mob.kind = 8;
                move_slide_bolt();
            }
            Mob.x = temple_draw_offsets.bolt_x_2 + x;
            if ((unsigned int)Mob.x < Mob.flags) {
                Mob.y = temple_draw_offsets.bolt_y_2 + y;
                Mob.kind = 9;
                move_slide_bolt();
                Mob.x = temple_draw_offsets.bolt_x_3 + x;
                Mob.y = temple_draw_offsets.bolt_y_3 + y;
                move_slide_bolt();
            }
            Mob.x = x;
            Mob.y = y;
            Mob.kind = 6;
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
    if (Mob.room == (unsigned char)Char.room && Mob.row == Char.row) {
        rock_x = Mob.x + 140;
        if (Char.sequence_group == 4 || (rock_x < kid_x && rock_x + 16 > kid_x)) {
            hit_character_with_rock();
            Kid = Char;
        }
    }
}

#pragma optimize("g", on)
void draw_object_slide_rock(unsigned char kind)
{
    struct moving_rock *rock;
    int x, frame, right;
    rock = (struct moving_rock *)slide_rock_on_row(Room.number, FChar.y / 63);
    right = rock ? rock->screen_x : 320;
    FChar.clip_right = right;
    x = right - FChar.x;
    switch (kind) {
    case 134: frame = 79; break;
    case 135: frame = 80; break;
    case 136: frame = x % 5 + 86; break;
    case 137: frame = x % 5 + 81; break;
    default: frame = -1; break;
    }
    if (frame != -1)
        add_middle_image(4, frame, FChar.x, FChar.y, 10);
}

#pragma optimize("g", off)

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
            tile = (&Level.cell_kinds[0][0])[(unsigned char)30 * rock->room - 30 +
                                    (signed char)(FirstInRow(rock->row) + column)];
            if (IsWall(tile) || tile == 4) {
                right = ColEdgeX[column + 6] + 24;
                if (Char.direction == 0)
                    right += screen_right - screen_left;
                barrier = right >= barrier ? right : barrier;
            }
        } else
            barrier += 35;
    } else if (direction == -1)
        barrier = ColEdgeX[19];
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
extern void get_tile_knockback(unsigned char kind);
extern void AddSound(int);
extern void dispatch_sound(int);
extern void mark_kid_contact(unsigned char);
extern void reset_kid_collision(void);
extern void set_character_sequence(int);
extern void update_kid_contact(void);

void hit_character_with_rock(void)
{
    int sequence;
    unsigned char tile;
    signed char column;
    int x;
    if (Char.action != 5) {
        sequence = -1;
        if (Char.x - 144 > Mob.x) {
            Char.x = Mob.x + 140;
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
            Char.x = ColEdgeX[column + 6] + 8;
            if (Char.direction == 0)
                Char.x += 17;
            if (tile != 2) {
                if (Mob.auxiliary == 100) {
                    if (Char.alive < 0) {
                        get_tile_knockback(Char.life_points);
                        Char.alive = 0;
                    }
                    Char.frame = 185;
                } else {
                    x = Mob.x - 10;
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
    signed char first = (Mob.x + 6) / 32;
    signed char last, row;
    unsigned char tile;
    step = (unsigned int)step < Mob.flags - Mob.x ? step : Mob.flags - Mob.x;
    last = (step + Mob.x) / 32;
    row = Mob.row + row_delta;
    if (last >= first) {
        do {
            if (!result)
                break;
            tile = ReadCell(Mob.room, first++, row);
            if (IS_SIX_KIND(tile) || tile == 25)
                result = 0;
            else
                result = 1;
        } while (last >= first);
    }
    return result;
}

#pragma pack(1)
struct tile_animation {
    unsigned short state;
    char rest[2];
};
struct trapdoor_bounds {
    int top, left;
};
#pragma pack()

#pragma pack(1)
typedef struct {
    signed char kind;                                 /* 00 */
    unsigned int modifier;                            /* 01 */
    signed char index;                                /* 03 */
    unsigned char room;                               /* 04 */
    signed char column, row;                          /* 05-06 */
    signed char first_col, last_col;                   /* 07-08 */
    unsigned char top_row, bottom_row;                 /* 09-0a */
} CELLREC;
#pragma pack()

extern CELLREC Cell;
extern unsigned short *BlueS;
extern unsigned char *room_trapdoor_flags;
extern int query_sound(int);
extern int current_sound;
extern void rock_hit_spikes(void);
extern void character_release_grip(void);
extern int probe_chain_floor(int);
extern int background_shapes;
extern int trapdoor_top, trapdoor_left;
extern void add_mob(void);
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

    if (Mob.auxiliary == 0 || Mob.auxiliary == 100)
        return;
    x = Mob.x;
    if (Mob.auxiliary == -1)
        --x;
    else if (can_slide_rock_stop() && Mob.auxiliary >= 8) {
        Mob.auxiliary = 0;
        dispatch_sound(10054);
        if (current_sound == 54)
            current_sound = -1;
    } else {
        Mob.x -= 8;
        if (can_slide_rock_stop())
            AddSound(53);
        Mob.x = x--;
        if (Mob.auxiliary <= 8)
            ++Mob.auxiliary;
    }
    if (x < 0) {
        Mob.room = Level.room_links[Mob.room - 1].left;
        x += 320;
    }
    next_x = x;
    if (!Mob.room)
        goto no_room;
    if (Mob.auxiliary == 0 || Mob.auxiliary == 100)
        goto done;
    prior_column = next_x - 20;
    column = prior_column / 32;
    if (prior_column <= 0)
        --column;
    tile = ReadCell(Mob.room, column, Mob.row);
    if (Mob.velocity != 1)
        goto advance;
    Mob.velocity = 0;
    Mob.auxiliary = 100;
    ++Mob.y;
    Mob.x = next_x + 2;
    if (tile != 2)
        goto finish;
    copy_kid_to_char();
    rock_hit_spikes();
    goto copy_kid;
no_room:
    Mob.velocity = -1;
    goto stop_timer;
advance:
    new_column = (next_x - 4) / 32;
    prev_column = (Mob.x - 4) / 32;
    if (new_column != prev_column || column != prev_column) {
        column = ReadCell(Mob.room, new_column, Mob.row);
        if ((IsWall(column) || column == 4 || tile == 2) && Mob.velocity == 0) {
            Mob.velocity = 1;
            --Mob.y;
            if (!query_sound(10055) && tile != 2)
                AddSound(8);
        }
    }
    copy_kid_to_char();
    if (Char.frame >= 87 && Char.frame <= 99) {
        column = (next_x - 8) / 32;
        if (Mob.row - Char.row + 1 == 0 && column == Char.column)
            character_release_grip();
    }
copy_kid:
    Kid = Char;
finish:
    if (Mob.auxiliary != 100) {
        Mob.x = next_x;
        if (Mob.auxiliary != 0)
            AddSound(54);
        return;
    }
stop_timer:
    dispatch_sound(10054);
done:;
}

extern unsigned char view_left, view_right, view_up, view_down;

int slide_rock_near_spikes(void)
{
    CELLREC saved = Cell;
    int result;
    ReadCell(Cell.room, Cell.column + 1, Cell.row);
    result = probe_chain_floor(16);
    if (!result) {
        ReadCell(Cell.room, Cell.column + 2, Cell.row);
        result = probe_chain_floor(16);
    }
    Cell = saved;
    return result;
}

int probe_chain_floor(int margin)
{
    int result = 0;
    int rock, left, right;
    rock = slide_rock_barrier(0, Cell.room, Cell.row);
    if (rock) {
        left = Cell.column * 32;
        right = left + 32;
        rock -= 140;
        if (left - margin + 32 > rock || right - margin + 32 < rock) {
            rock = slide_rock_barrier(-1, Cell.room, Cell.row);
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

extern int Col2PixelX[13];
extern int Row2PixelY[5];

#pragma pack(1)
typedef struct {
    int image, equipment_image;                       /* 00,02 */
    signed char x, y;                                 /* 04-05 */
    unsigned char flags;                              /* 06 */
} FRAMEREC;
#pragma pack()

extern FRAMEREC Frame;

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
        Mob.auxiliary = -1;
        Mob.velocity = 0;
        x = Col2PixelX[Cell.column + 1];
        if (x > 320 && (room == Room.left)) {
            room = Room.number;
            x -= 320;
        }
        Mob.flags = Mob.x = x;
        Mob.room = room;
        Mob.y = Row2PixelY[Cell.row + 1] - 5;
        Mob.row = location.row;
        Mob.kind = 6;
        add_mob();
    }
}

extern struct { int heights[13], widths[13]; } mob_dimensions;

void move_trapdoor(void)
{
    int x = Mob.x, y = Mob.y;
    unsigned char room = Mob.room;
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

    if (room != Room.number) {
        if (room == Room.below)
            y += 191;
        else if (Mob.room == Room.above)
            y -= 191;
        else if (Mob.room == Room.left)
            x -= 320;
        else if (Mob.room == Room.right)
            x += 320;
        else
            goto draw;
        room = Room.number;
    }
draw:
    draw.saved_column = column = x / 32;
    FChar.index = IndexCell1(draw.saved_column, y / 63);
    if (FChar.index != 30) {
        if (column)
            --FChar.index;
        if (FChar.index < 20)
            FChar.index += 10;
    }
    absolute_column = FirstInRow(Mob.row) + Mob.x / 32;
    step = Mob.flags & 31;
    if (step > 16) {
        Mob.velocity = -1;
        Level.cell_info[Mob.room - 1][absolute_column].modifier &= 0xff00;
        (&Level.cell_kinds[0][0])[(unsigned char)30 * *(volatile unsigned char *)&Mob.room - 30 + absolute_column] = 26;
        step = 17;
        dispatch_sound(10095);
        if (current_sound == 95)
            current_sound = -1;
        AddSound(3);
    }
    if (is_visible_room_cell(room, FirstInRow(Mob.row) + draw.saved_column)) {
        saved_y = Mob.y;
        Mob.y += temple_draw_offsets.trapdoor_y[step];
        y += temple_draw_offsets.trapdoor_y[step];
        saved_x = Mob.x;
        x += temple_draw_offsets.trapdoor_x[step];
        Mob.x = x;
        if (get_set_image_size(background_shapes, temple_draw_offsets.trapdoor_shape[step] - 1, &bounds)) {
            int left_edge;
            mob_dimensions.heights[10] = bounds.top + 1;
            left_edge = column * 32 - Mob.x + 57;
            ++bounds.left;
            if (left_edge > bounds.left)
                bounds.left = left_edge;
            mob_dimensions.widths[10] = bounds.left;
        }
        mark_mob_redraw(0);
        if (Mob.velocity != -1) {
            compute_mob_rect(&draw.area.rect);
            draw.area.rect.left += 16;
            mark_bufs_for_rect(trapdoor_update_rect, &draw.area.rect, -1);
            draw_moving_tile(y);
        }
        Mob.x = saved_x;
        Mob.y = saved_y;
    }
}

void move_trapdoor_record(void)
{
    if (Mob.velocity != -1) {
        if ((signed char)(Mob.flags & 31) <= 16)
            ++Mob.flags;
        if (!query_sound(10094) && !query_sound(10095))
            AddSound(95);
    }
}

void check_trapdoor(void)
{
    if ((Frame.flags & 0x40) &&
        (signed char)63 * Char.row - Char.y == -56 &&
        Char.character_kind != 1 &&
        ReadCell((unsigned char)Char.room, Char.column, Char.row) == 26 &&
        !(Cell.modifier & 31))
        trigger_trapdoor(Cell.room, Cell.index);
}

void shake_trapdoor(void)
{
    if (!(Cell.modifier & 0x40)) {
        if (Cell.index % (signed char)10 != Char.column) {
            struct tile_animation *animation = (struct tile_animation *)BlueS;
            Cell.modifier |= 0x40;
            animation[Cell.index].state = Cell.modifier;
        }
        trigger_trapdoor(Cell.room, Cell.index);
    }
}

void trigger_trapdoor(unsigned char room, signed char cell)
{
    Level.cell_info[room - 1][cell].modifier = 0x81;
    room_trapdoor_flags[cell] = 0;
    Mob.velocity = Mob.auxiliary = 0;
    Mob.x = Col2PixelX[cell % (signed char)10 + 1];
    Mob.flags = (Cell.modifier & 0x40) ? 75 : 1;
    Mob.room = room;
    Mob.y = Row2PixelY[Cell.row + 1];
    Mob.row = cell / (signed char)10;
    Mob.kind = 10;
    add_mob();
    AddSound(94);
    mark_mob_redraw(0);
}

void draw_object_trapdoor(unsigned char modifier)
{
    int shape = temple_draw_offsets.trapdoor_shape[(*(unsigned char *)&FChar.image) & 31];
    if (shape == 21) {
        add_middle_image(4, shape, FChar.x + 1, FChar.y - 2, 10);
        add_middle_image(4, 22, FChar.x, FChar.y - 1, 0);
    } else
        add_middle_image(4, shape, FChar.x, FChar.y - 1, 10);
}

void load_inscription_room(void)
{
    if ((signed char)Kid.column >= 9)
        AddSong(92);
}

void check_temple_event(void)
{
    if (Room.number == 4 && ((unsigned char)Level.level_number) == 13)
        altar_watch_kid();
}

void temple_frame(void)
{
}
