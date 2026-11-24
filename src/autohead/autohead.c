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
    int height, width;
} SpriteBounds;

typedef struct {
    signed char x, y;
} NeckPoint;
#pragma pack()

#pragma pack(1)
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


#pragma pack(1)
typedef struct {
    signed char index, flipped;                       /* 00-01 */
    int x, y, image;                                   /* 02,04,06 */
    unsigned char image_set;                          /* 08 */
    int clip_top, clip_left, clip_bottom, clip_right;    /* 09,0b,0d,0f */
    int depth;                                        /* 11 */
    unsigned char object_type;                        /* 13 */
} FCHARREC;
#pragma pack()
extern FCHARREC FChar;
extern int sprite_x;
extern CHARREC Kid;
extern CHARREC Opp;
extern CHARREC Char;
extern int sprite_y;
extern void near *opponent_shapes;
extern unsigned int opponent_effect_frames;

extern HeadSprite near *far pascal get_gate_image(char kind, unsigned char index);
extern NeckPoint far *far pascal fetch_locked_shape_list(unsigned int handle);
extern int far *far pascal get_set_image_size(void near *sheet, int frame, SpriteBounds far *bounds);
extern void far pascal clear_handle_owned(unsigned int handle);

typedef struct HeadPlan HeadPlan;

#pragma pack(1)
typedef struct {
    unsigned char unknown_00;
    int spawn_x;
    unsigned char spawn_direction, skill;
    unsigned char unknown_05[10], kind, reserved;
    signed char mode, column, row;
    int timer;
    unsigned char sound_played;
} OpponentRecord;
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
extern unsigned char OppCharId[];


#pragma pack(1)
typedef struct {
    int image, equipment_image;                       /* 00,02 */
    signed char x, y;                                 /* 04-05 */
    unsigned char flags;                              /* 06 */
} FRAMEREC;
#pragma pack()
extern FRAMEREC Frame;
extern int snake_distance;
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
extern LEVELREC Level;
static const signed char first_chomp[12] = {
    1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 99
};
static const signed char last_chomp[12] = {
    1, 1, 2, 2, 2, 3, 3, 3, 3, 4, 4, 99
};
static const int flee_length[12] = {
    70, 65, 60, 55, 50, 45, 40, 35, 30, 25, 20, 15
};
typedef int COL_EDGE_X[20];                           /* DS:0d7c */
extern COL_EDGE_X ColEdgeX;
extern signed char move_with_facing[], move_against_facing[];

extern signed char far pascal ReadCell(unsigned char room, signed char column, signed char row);
extern int far pascal IsWall(signed char tile);
extern int far pascal IS_SIX_KIND(signed char tile);
extern int far pascal GetBarrCode(unsigned char tile);
extern int far pascal is_selected_actor(void);
extern int far pascal gate_blocks_passage(void);
extern int far pascal Rnd(int maximum);
extern void far pascal reset_char_y(void);
extern void far pascal ClearCharVel(void);
extern void far pascal AddSound(int sound);
extern void far pascal AddSong(int sound);
extern int far pascal query_sound(int sound);
extern int far pascal decrement_life_points(signed char amount);
extern int far pascal AddCharX(int amount);
extern void far pascal JumpSeq(int sequence);
extern int far pascal barrier_distance_from_face(unsigned char room, unsigned char column, unsigned char row);
extern signed char far pascal GetCellCol(void);
extern signed char far pascal get_char_col(int room);
extern int far pascal get_base_x(void);
extern int far pascal get_opp_dist(void);
extern void far pascal save_actor_slot(void);
extern void far pascal DrawOppMeter(signed char id, int x, int y);
extern void far pascal load_actor_slot(signed char id);
extern OpponentRecord near *far pascal GetOppInfoIdx(int room, int id);
extern void far pascal load_shadow_with_opponent(signed char id);
extern void far pascal save_shadow_with_opponent(void);
extern void far pascal get_frame_info(void);
extern void far pascal get_char_edges(void);
extern signed char far pascal get_cells_ahead(signed char direction);
extern int far pascal is_open_tile(signed char tile);
extern signed char far pascal find_wall_column(unsigned char room, signed char column,
                                                signed char row, signed char direction);
extern signed char far pascal get_under_foot(void);
extern int far pascal get_edge_distance(void);

int far pascal head_clear_in_dir(int direction);
int far pascal head_go_to_dest(HeadPlan near *plan);
void far pascal head_face_kid(void);
void far pascal head_get_angry(HeadPlan near *plan);
void far pascal head_get_peaceful(HeadPlan near *plan);
void far pascal head_focus_on_neck(HeadPlan near *plan);
int far pascal head_set_to_flee_mode(void);
int far pascal is_able_to_flee_in_room(unsigned char room);
int far pascal just_start_vertical_seq(void);
int far pascal XWithinMiddleOfCell(int first, int second);
int far pascal YWithinMiddleOfCell(int value, int half_width);
int far pascal snake_slithering_toward_opp(void);
void far pascal snake_attack(void);
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
    SpriteBounds bounds;
    HeadSprite near *body;
    NeckPoint far *neck;

    body = get_gate_image(2, 0);
    if (body == 0)
        return;

    neck = fetch_locked_shape_list(opponent_effect_frames);
    if (Kid.direction != Char.direction)
        neck_x = neck[body->frame].y;
    else {
        get_set_image_size(opponent_shapes, FChar.image, &bounds);
        neck_x = -((Char.character_kind == 7 ? 10 : 16) + neck[body->frame].y + (signed char)bounds.width);
    }
    if (Char.character_kind == 8)
        neck_x += 6;
    if (Char.direction == 0)
        neck_x = -neck_x;
    sprite_x = body->x + neck_x;
    sprite_y = body->y + ((signed char far *)neck)[body->frame * 2];
    clear_handle_owned(opponent_effect_frames);
    Char.x = sprite_x + 130;
}

int far pascal head_visibility(void)
{
    signed char row, column, end_row, end_column, rows, columns, error;
    signed char step_column, step_row, straight_error, diagonal;
    int clear = 1;
    int result;

    if (Opp.column == Char.column && Opp.row == Char.row) {
        result = 4;
    } else {
        row = Char.row * 2;
        column = Char.column * 2;
        if ((Char.x + Frame.x - 122) % 32 > 16)
            ++column;
        end_row = Opp.row * 2 + 1;
        end_column = Opp.column * 2;
        if ((Opp.x - 130) % 32 > 16)
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
            straight_error = rows * 2;
            diagonal = (rows - columns) * 2;
            do {
                column += step_column;
                if ((step_column > 0 && column % 2 == 0) ||
                    (step_column < 0 && column % 2 != 0))
                    clear = !IsWall(ReadCell(Char.room, column / 2, row / 2));
                if (clear) {
                    if (error < 0)
                        error += straight_error;
                    else {
                        if ((step_row > 0 && row % 2 != 0) ||
                            (step_row < 0 && row % 2 == 0)) {
                            if (step_row < 0)
                                clear = IS_SIX_KIND(ReadCell(Char.room, column / 2, (row - 1) / 2));
                            else
                                clear = IS_SIX_KIND(ReadCell(Char.room, column / 2, row / 2));
                        }
                        row += step_row;
                        error += diagonal;
                    }
                }
            } while (--columns && clear);
        } else {
            error = columns * 2 - rows;
            straight_error = columns * 2;
            diagonal = (columns - rows) * 2;
            do {
                if ((step_row > 0 && row % 2 != 0) ||
                    (step_row < 0 && row % 2 == 0)) {
                    if (step_row < 0)
                        clear = IS_SIX_KIND(ReadCell(Char.room, column / 2, (row - 1) / 2));
                    else
                        clear = IS_SIX_KIND(ReadCell(Char.room, column / 2, row / 2));
                }
                if (clear) {
                    row += step_row;
                    if (error < 0)
                        error += straight_error;
                    else {
                        column += step_column;
                        if ((step_column > 0 && column % 2 == 0) ||
                            (step_column < 0 && column % 2 != 0))
                            clear = !IsWall(ReadCell(Char.room, column / 2, row / 2));
                        error += diagonal;
                    }
                }
            } while (--rows && clear);
        }
        if (clear) {
            if (!is_selected_actor())
                result = 4;
            else if (Opp.row != Char.row)
                result = step_row > 0 ? 2 : 3;
            else
                result = step_column > 0 ? 1 : 0;
        } else
            result = -1;
    }
    return result;
}

#pragma pack(1)
struct HeadPlan {
    signed char mode, column, row;
    union {
        struct {
            signed char direction;
            unsigned char flags;
        } attack;
        int flee_timer;
    } state;
    unsigned char sound_played;
};
#pragma pack()

int far pascal chomp_again(HeadPlan near *plan)
{
    unsigned char room_skill = Level.room_objects[(unsigned char)Char.room - 1].opponent_data[Char.slot][4];
    int result;
    signed char column;

    if (plan->column < first_chomp[room_skill])
        result = 1;
    else if (last_chomp[room_skill] <= (column = plan->column))
        result = 0;
    else
        result = Rnd(1);
    return result;
}

HeadSprite near *far pascal head_add_opponent(HeadSprite near *sprite)
{
    if (sprite->opponent_kind == 0) {
        sprite->opponent_kind = 5;
        Char.character_kind = OppCharId[5];
        reset_char_y();
    }
    if (Char.character_kind == 11) {
        if (Char.alive < 0) {
            if (sprite->opponent_mode <= 0) {
                sprite->frame = 170;
                sprite->step = 0;
                sprite->opponent_mode = -1;
            } else {
                sprite->frame = 175;
                sprite->step = 0;
                Char.sequence_group = 6;
            }
            Char.stance = 1;
            AddSound(71);
        } else
            Char.stance = 0;
    } else {
        Char.x = ColEdgeX[Char.column + 5] + 30;
        Char.direction = -1;
        sprite->frame = 130;
        sprite->step = 0;
        sprite->opponent_mode = Char.stance = 0;
    }
    return sprite;
}

void far pascal head_attack_prog(HeadPlan near *plan)
{
    unsigned char dx, dy;
    int direction, vertical_sequence;

    if (Char.sequence == 140 || Char.sequence == 152 ||
        Char.sequence == 156 || Char.sequence == 162)
        return;
    if ((int)Char.sequence >= 147 && (int)Char.sequence <= 151) {
        head_focus_on_neck(plan);
        return;
    }
    if (Char.frame != 150 && Char.frame != 165 && Char.frame != 166 &&
        Char.frame != 158 && Char.frame != 173)
        return;
    direction = head_visibility();
    if (direction != -1) {
        plan->state.attack.direction = 1;
        {
            int delta = Char.column - Opp.column;
            dx = delta < 0 ? -delta : delta;
        }
        {
            int delta = Char.row - Opp.row;
            dy = delta < 0 ? -delta : delta;
        }
        if (dy == 0 && dx < 2) {
            vertical_sequence = Char.x < Opp.x ? 1 : 0;
            if (dx != 0 && !head_clear_in_dir(vertical_sequence)) {
                if (Char.sequence != 166 && YWithinMiddleOfCell(Char.y, 15))
                    JumpSeq(166);
                return;
            }
            if (Char.sequence != 147)
                JumpSeq(147);
        } else if (dx == 0 && dy == 1 && head_clear_in_dir(direction)) {
            if (Opp.row < Char.row)
                vertical_sequence = 143;
            else
                vertical_sequence = 144;
            if (vertical_sequence != Char.sequence)
                JumpSeq(vertical_sequence);
        } else if (direction == 4) {
            if (dx || dy) {
                if (Char.sequence != 166 && YWithinMiddleOfCell(Char.y, 15))
                    JumpSeq(166);
            }
        } else {
            plan->row = Opp.row;
            plan->column = Opp.column;
            if ((direction == 3 || direction == 2) && !head_clear_in_dir(direction))
                direction = Char.x < Opp.x ? 1 : 0;
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
                ClearCharVel();
            }
            if (Char.sequence != direction)
                JumpSeq(direction);
        }
    } else {
        if (plan->state.attack.direction != 0) {
            plan->state.attack.direction = 0;
            plan->column = Opp.column;
            plan->row = Opp.row;
        }
        head_go_to_dest(plan);
    }
}

int far pascal head_blocked(void)
{
    int sequence;
    if (decrement_life_points(1))
        sequence = 154;
    else {
        AddSound(87);
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
        if (Char.column < -1 || just_start_vertical_seq())
            break;
        tile = ReadCell(Char.room, Char.column - 1, Char.row);
        if (IsWall(tile))
            break;
        if (tile == 4 && gate_blocks_passage())
            break;
        clear = 1;
        break;
    case 1:
        if (Char.column > 10 || just_start_vertical_seq())
            break;
        tile = ReadCell(Char.room, Char.column + 1, Char.row);
        if (IsWall(tile))
            break;
        if (tile == 4 && gate_blocks_passage())
            break;
        if (ReadCell(Char.room, Char.column, Char.row) != 4 || !gate_blocks_passage())
            clear = 1;
        break;
    case 2:
        if (Char.row != 2 && XWithinMiddleOfCell(Char.x, 8))
            clear = IS_SIX_KIND(ReadCell(Char.room, Char.column, Char.row));
        break;
    case 3:
        if (Char.row != 0 && XWithinMiddleOfCell(Char.x, 8))
            clear = IS_SIX_KIND(ReadCell(Char.room, Char.column, Char.row - 1));
        break;
    }
    return clear;
}

void far pascal head_done_attacking_kid(HeadPlan near *plan)
{
    head_get_peaceful(plan);
    Char.sequence_group = 0;
}

int far pascal head_enemy_collide(int minimum_gap)
{
    int sequence = -1;
    int gap;

    if (Char.sequence_group != 1) {
        gap = barrier_distance_from_face((unsigned char)Cell.kind, Cell.room, (unsigned char)Cell.column);
        if (gap < 5 && minimum_gap <= gap && gap < 5 && -snake_distance <= gap) {
            Char.x = AddCharX(-(gap + 14));
            if (Char.alive < 0 && Char.sequence != 153)
                sequence = 157;
            else if (Char.sequence != 164) {
                if (Char.frame <= 191) {
                    sequence = 164;
                    Char.x = ColEdgeX[GetCellCol() + 5] + 14;
                    if (Char.direction == -1)
                        Char.x += 32;
                } else
                    JumpSeq(145);
                decrement_life_points(100);
            }
        }
    }
    return sequence;
}

void far pascal head_face_kid(void)
{
    signed char column = Char.column;
    if (column < Opp.column) {
        if (Char.direction != 0) {
            Char.x = AddCharX(-14);
            Char.direction = 0;
        }
    } else if (Char.direction != -1) {
        Char.x = AddCharX(-14);
        Char.direction = -1;
    }
}

void far pascal head_flee_prog(HeadPlan near *plan)
{
    if (Char.sequence != 153) {
        if (Char.sequence == 159) {
            if (plan->column == Char.column && XWithinMiddleOfCell(Char.x, 2)) {
                if (Char.row == Opp.row && abs(Char.column - Opp.column) < 3)
                    head_get_angry(plan);
                else {
                    JumpSeq(160);
                    plan->state.flee_timer = flee_length[Level.room_objects[(unsigned char)Char.room - 1].opponent_data[Char.slot][4]];
                }
            }
        } else if (Char.row == Opp.row && abs(Char.column - Opp.column) < 3 &&
                   Opp.life_points && (Opp.sequence != 70 || (Opp.frame != 0 && Opp.frame < 223)))
            head_get_angry(plan);
        else if (--plan->state.flee_timer == 0) {
            if (Opp.life_points && (Opp.sequence != 70 || (Opp.frame != 0 && Opp.frame < 223)) &&
                head_visibility() != -1) {
                head_get_angry(plan);
                JumpSeq(162);
            } else {
                head_get_peaceful(plan);
                JumpSeq(163);
            }
        }
        if (!plan->sound_played && !query_sound(10087)) {
            plan->sound_played = 1;
            AddSong(146);
        }
    } else if (Char.sequence_group == 2) {
        Char.alert_delay = 0;
        JumpSeq(159);
    }
}

void far pascal head_focus_on_neck(HeadPlan near *plan)
{
    int low, high, distance;
    if (Char.sequence == 147 || Char.sequence == 144 || Char.sequence == 143) {
        if (Opp.stance == 1) {
            low = -25;
            high = 0;
        } else {
            low = -3;
            high = 30;
        }
        distance = get_opp_dist();
        if (Char.row == Opp.row && low <= distance && high >= distance) {
            ClearCharVel();
            JumpSeq(Rnd(1) + 150);
            plan->mode = 3;
            plan->column = 1;
        } else {
            int close_limit = Opp.stance == 1 ? -35 : -13;
            if ((Char.row != Opp.row || close_limit > distance) && Char.sequence == 147) {
                ClearCharVel();
                JumpSeq(156);
            }
            if (distance - 64 <= high && Char.row == Opp.row && Char.sequence != 144)
                AddSound(90);
        }
    }
}

void far pascal head_get_angry(HeadPlan near *plan)
{
    plan->mode = 1;
    plan->row = Opp.row;
    plan->column = Opp.column;
    plan->state.attack.direction = 1;
    JumpSeq(140);
    ClearCharVel();
    head_face_kid();
    Char.alert_delay = 3;
    Char.stance = 1;
    if (Opp.opponent_index == -1) {
        save_actor_slot();
        DrawOppMeter(Char.slot, Char.life_points, Char.life_initial);
        load_actor_slot(Char.slot);
        Opp.opponent_index = Char.slot;
    }
    AddSound(87);
}

void far pascal head_get_peaceful(HeadPlan near *plan)
{
    signed char zero = 0;
    plan->mode = zero;
    Char.stance = zero;
    ClearCharVel();
    JumpSeq(139);
    Char.column = get_char_col(get_base_x());
    Char.x = ColEdgeX[Char.column + 5] + 30;
    reset_char_y();
    Char.alert_delay = 0;
}

int far pascal head_go_to_dest(HeadPlan near *plan)
{
    int clear = 0;
    int sequence;
    int direction;
    if (Char.column != plan->column || plan->row != Char.row) {
        if (plan->row != Char.row) {
            if (plan->row > Char.row) {
                direction = 2;
                sequence = 144;
            } else {
                direction = 3;
                sequence = 143;
            }
            clear = head_clear_in_dir(direction);
            if (clear && Char.sequence != sequence)
                JumpSeq(sequence);
        }
        if (!clear && Char.column != plan->column) {
            if (plan->column > Char.column)
                sequence = 1;
            else
                sequence = 0;
            clear = head_clear_in_dir(sequence);
            if (clear && Char.sequence != 142)
                JumpSeq(142);
        }
    }
    if (!clear)
        head_get_peaceful(plan);
    return clear;
}

void far pascal head_locked_prog(HeadPlan near *plan)
{
    int lost = 0;
    OpponentRecord near *record;
    if ((Char.sequence == 150 && Char.frame == 167 && Char.sequence_group != 1) ||
        (Char.sequence == 151 && Char.frame == 166)) {
        if (chomp_again(plan)) {
            if (Opp.alive < 0) {
                ++plan->column;
                JumpSeq(150 + (Char.sequence == 150));
            } else {
                record = GetOppInfoIdx((unsigned char)Char.room, Char.slot);
                if (record && record->skill >= 11)
                    JumpSeq(165);
            }
        } else {
            JumpSeq(152);
            Char.x = AddCharX(-8);
            lost = 1;
        }
    }
    if (lost || Char.sequence == 158 || Char.sequence == 157) {
        plan->mode = 1;
        plan->column = Opp.column;
        plan->row = Opp.row;
    }
}

void far pascal unused_head_alert(void)
{
}

void far pascal head_peaceful_prog(void)
{
    int direction, sequence;
    if (Char.frame != 150)
        return;
    direction = Rnd(3);
    if (direction == 3) {
        if ((int)Char.sequence >= 131 && (int)Char.sequence <= 133)
            direction = 0;
        else if ((int)Char.sequence >= 134 && (int)Char.sequence <= 136)
            direction = 1;
        else
            direction = -1;
    } else if (direction == 2)
        direction += Rnd(1);
    if (direction != -1 && head_clear_in_dir(direction)) {
        if (direction == 3)
            sequence = 137;
        else if (direction == 2)
            sequence = 138;
        else if (direction == 0)
            sequence = Rnd(2) + 131;
        else
            sequence = Rnd(2) + 134;
        JumpSeq(sequence);
    }
}

void far pascal head_prog(void)
{
    OpponentRecord near *record;
    HeadPlan near *plan;
    if (Char.alive >= 0)
        return;
    record = GetOppInfoIdx((unsigned char)Char.room, Char.slot);
    if (!record)
        return;
    plan = (HeadPlan near *)&record->mode;
    load_shadow_with_opponent(Char.slot);
    get_frame_info();
    if (Frame.image != -1) {
        FChar.image = Frame.image;
        get_char_edges();
    }
    switch (plan->mode) {
    case 0:
        if (Opp.life_points && (Opp.sequence != 70 || (Opp.frame != 0 && Opp.frame < 223)) &&
            (Char.frame == 150 || Char.frame == 165 || Char.frame == 166 ||
             Char.frame == 158 || Char.frame == 173) && head_visibility() != -1)
            head_get_angry(plan);
        else
            head_peaceful_prog();
        break;
    case 1:
        if (Opp.life_points && (Opp.sequence != 70 || (Opp.frame != 0 && Opp.frame < 223)))
            head_attack_prog(plan);
        else
            head_done_attacking_kid(plan);
        break;
    case 2:
        head_flee_prog(plan);
        break;
    case 3:
        if (Opp.life_points && (Opp.sequence != 70 || (Opp.frame != 0 && Opp.frame < 223)))
            head_locked_prog(plan);
        else
            head_done_attacking_kid(plan);
        break;
    }
    save_shadow_with_opponent();
}

int far pascal head_set_to_flee_mode(void)
{
    unsigned char skill;
    signed char column;
    int clear, edge, initial_column;
    OpponentRecord near *record;

    skill = Level.room_objects[(unsigned char)Char.room - 1].opponent_data[Char.slot][4];
    if (!is_able_to_flee_in_room(Char.room) || Rnd(skill / 3 + 1) != 0 ||
        !((Char.direction == 0 && Char.column >= 3) ||
          (Char.direction == -1 && Char.column + 3 <= 10)))
        goto cannot_flee;
    clear = 1;
    initial_column = *(volatile signed char *)&column = Char.column;
    edge = Char.direction == 0 ? -1 : 10;
    if (initial_column != edge)
        do {
            if (!clear)
                break;
            skill = ReadCell(Char.room, column, Char.row);
            if (GetBarrCode(skill) && (skill != 4 || gate_blocks_passage()))
                clear = 0;
            column += move_against_facing[Char.direction];
        } while (column != edge);
    if (!clear) {
        column += move_with_facing[Char.direction] * 2;
        if (abs(column - Char.column) >= 3)
            clear = 1;
    } else
        column += move_with_facing[Char.direction];
    if (clear) {
        Char.stance = 0;
        Char.alert_delay = 1;
        record = GetOppInfoIdx((unsigned char)Char.room, Char.slot);
        record->mode = 2;
        record->column = column;
        record->row = Char.row;
        record->timer = -1;
        record->sound_played = 0;
        save_actor_slot();
        DrawOppMeter(Char.slot, 0, Char.life_initial);
        load_actor_slot(Char.slot);
        if (Char.slot == Opp.opponent_index)
            Opp.opponent_index = -1;
    }
    goto done;
cannot_flee:
    clear = 0;
done:
    return clear;
}

int far pascal is_able_to_flee_in_room(unsigned char room)
{
    signed char slot, count;
    int room_number = room;
    int available;

    count = (unsigned char)Level.room_objects[room_number - 1].opponent_count;
    if (count > 1) {
        available = 1;
        slot = 0;
        do {
            if (!(slot < count))
                break;
            {
                int can_flee = GetOppInfoIdx(room, slot)->mode != 2;
                available = can_flee;
                ++slot;
                if (!can_flee)
                    break;
            }
        } while (1);
    } else
        available = 0;
    return available;
}

int far pascal is_head_locked_on(unsigned char room, int slot)
{
    OpponentRecord near *record;
    int result;
    record = GetOppInfoIdx(room, slot);
    result = record != 0 && record->mode == 3;
    return result;
}

int far pascal just_start_vertical_seq(void)
{
    int baseline = Char.row * 63 + 37;
    int result;
    if (Char.sequence != 144) {
        if (Char.sequence == 143)
            result = Char.y < baseline;
        else
            result = 0;
        return result;
    } else if (Char.y > baseline)
        result = 1;
    else
        result = 0;
    return result;
}

int far pascal XWithinMiddleOfCell(int first, int second)
{
    int result;
    first = (first - 144) % 32;
    result = first >= 16 - second && 16 + second >= first;
    return result;
}

int far pascal YWithinMiddleOfCell(int value, int half_width)
{
    int result;
    value = (value - 3) % 63;
    result = value >= 31 - half_width && 31 + half_width >= value;
    return result;
}

void far pascal snake_prog(void)
{
    OpponentRecord near *record;
    if (Char.alive < 0) {
        record = GetOppInfoIdx((unsigned char)Char.room, Char.slot);
        if (record) {
            load_shadow_with_opponent(Char.slot);
            if (record->mode < 0) {
                if (snake_slithering_toward_opp())
                    snake_attack();
                else
                    snake_go_into_hole(record);
            } else
                snake_come_out_of_hole(record);
            save_shadow_with_opponent();
        }
    }
}

void far pascal snake_attack(void)
{
    int hit = 0;
    int distance = get_opp_dist();
    signed char tile;
    if (distance >= 13 && distance <= 45) {
        if (Char.frame == 278 || Char.frame == 279 ||
            (Char.sequence == 176 &&
             ((Opp.frame >= 1 && Opp.frame <= 14) || (Opp.frame >= 49 && Opp.frame <= 56))))
            hit = 1;
        else if (Char.sequence != 176 && Char.sequence != 173) {
            JumpSeq(176);
            AddSound(71);
        }
    } else if (Char.sequence == 176 &&
               ((Opp.frame >= 1 && Opp.frame <= 14) || (Opp.frame >= 49 && Opp.frame <= 56)) &&
               distance > 13 && distance < 50)
        hit = 1;
    else if (Char.sequence == 176 && Opp.frame >= 135 && Opp.frame <= 149 &&
             distance > 13 && distance < 45)
        hit = 1;
    else if ((Char.frame == 278 || Char.frame == 279) &&
             (distance < 13 || distance > 49)) {
        tile = get_cells_ahead(1);
        if (is_open_tile(tile) || (tile == 7 && (Cell.modifier & 128)))
            JumpSeq(177);
    }
    if (hit && Char.sequence != 173) {
        Char.x = AddCharX(distance - 32);
        JumpSeq(173);
    }
}

void far pascal snake_come_out_of_hole(OpponentRecord near *record)
{
    signed char tile;
    signed char left, right;
    if (--record->mode > 0)
        return;
    record->mode = -1;
    left = find_wall_column(Char.room, Char.column, Char.row, -1);
    right = find_wall_column(Char.room, Char.column, Char.row, 1);
    tile = get_under_foot();
    if ((unsigned char)Opp.room == (unsigned char)Char.room && Char.row == Opp.row &&
        Char.column > left && Char.column < right) {
        if (Char.column <= Opp.column &&
            (!IsWall(tile) || is_open_tile(tile) || (tile == 7 && (Cell.modifier & 128))))
            Char.direction = 0;
        else
            Char.direction = -1;
    } else
        Char.direction = ~Char.direction;
    if (IsWall(tile) && !is_open_tile(tile) && (tile != 7 || !(Cell.modifier & 128))) {
        JumpSeq(170);
        Char.x = AddCharX(38);
    } else {
        JumpSeq(172);
    }
    Char.sequence_group = 0;
}

void far pascal snake_go_into_hole(OpponentRecord near *record)
{
    signed char tile;
    int distance;
    if (Opp.alive >= 0) {
        if (Char.sequence != 176 && Char.frame != 278 && Char.frame != 279)
            JumpSeq(176);
        return;
    }
    if (Char.sequence != 171 && Char.sequence != 172 && Char.sequence_group != 9) {
        get_under_foot();
        if ((((unsigned char *)&Cell.modifier)[1] & 15) == 12) {
            distance = (Char.x - 144) % 32;
            if (distance >= 14 && distance <= 18) {
                distance = 16 - distance;
                if (Char.direction == -1)
                    distance = -distance;
                else
                    distance += 8;
                Char.x = AddCharX(distance);
                JumpSeq(171);
            }
        } else if (Char.sequence != 176 && Char.sequence != 168 &&
                   Char.frame != 278 && Char.frame != 279) {
            tile = get_cells_ahead(1);
            if (is_open_tile(tile) || (tile == 7 && (Cell.modifier & 128)))
                return;
            if (!IsWall(tile))
                JumpSeq(176);
            else {
                if (tile != 7) {
                    if (Char.direction == 0) {
                        Char.sequence_group = 9;
                        return;
                    }
                    if (Char.frame != 254)
                        return;
                }
                JumpSeq(168);
            }
        } else {
            tile = get_cells_ahead(1);
            if (is_open_tile(tile) || (tile == 7 && (Cell.modifier & 128)))
                JumpSeq(177);
        }
    } else {
        if (Char.sequence_group == 9) {
            tile = get_under_foot();
            distance = get_edge_distance();
            if (IsWall(tile) && distance < 8) {
                Char.frame = 264;
                JumpSeq(168);
            }
        }
        if (Char.frame == 264) {
            Char.sequence_group = 6;
            record->mode = Char.alert_delay == 2 ? 6 : Rnd(60) + 36;
        }
    }
}

int far pascal snake_slithering_toward_opp(void)
{
    int result = 0;
    if (Char.sequence != 171 && Char.alert_delay >= 2) {
        if (Char.direction == 0) {
            signed char column = Char.column;
            if (Opp.column >= column)
                goto toward;
        }
        if (Char.direction == -1) {
            signed char column = Char.column;
            if (Opp.column <= column)
toward:
                result = 1;
        }
    }
    return result;
}

void far pascal snake_update_opp_info(OpponentRecord near *record)
{
    signed char tile;
    if (Char.sequence == 171 || Char.sequence_group == 9) {
        Char.sequence_group = 6;
        record->mode = Rnd(60) + 36;
    } else {
        tile = get_cells_ahead(1);
        if (!is_open_tile(tile) && (tile != 7 || !(Cell.modifier & 128)) &&
            (Char.sequence == 176 || Char.sequence == 168 || Char.sequence == 170)) {
            record->spawn_direction = ~Char.direction;
            record->spawn_x = AddCharX(-32);
            return;
        }
    }
}
