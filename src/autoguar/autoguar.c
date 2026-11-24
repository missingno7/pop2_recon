/* AUTOGUAR.C. Opening routines in original order. */

extern signed char direction_factor[];
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
    signed char behavior;            /* 38 */
    signed char opponent_index;        /* 39 */
    signed char flame_offset;            /* 3a */
    unsigned char unknown_3b[5];        /* 3b..3f */
} CHARREC;
#pragma pack()


extern void far pascal load_changed_opponent_set(signed char kind);
extern CHARREC Opp, Kid, Char;


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
    unsigned char level_number;                         /* 1847 */
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
extern void far pascal JumpSeq(int sequence);
extern void far pascal AnimChar(void);
int far pascal get_opponent_death_sequence(unsigned char kind);

void far pascal check_multiple_dead_bodies(void)
{
    int sequence;
    if (Char.frame == 185) {
        sequence = get_opponent_death_sequence(Level.preferred_opponent);
        if (sequence != 0) {
            JumpSeq(sequence);
            AnimChar();
        }
        if (Char.alive < 20)
            ++Char.alive;
    }
}

int far pascal get_opponent_death_sequence(unsigned char kind)
{
    int count;
    int sequence_result;
    int sequence;

    sequence = 0;
    sequence_result = sequence;
    if (kind == 7) {
        sequence = 191;
        count = 4;
    } else if (kind == 0 && Level.environment_kind != 6) {
        sequence = 195;
        count = 1;
    }

    if (sequence && Char.slot % (count + 1))
        sequence_result = sequence + Char.slot % (count + 1) - 1;

    return sequence_result;
}



void far pascal start_opponent_running(void)
{
    JumpSeq(Char.character_kind == 4 ? 102 : 84);
    Char.stance = 0;
    Char.alert_delay = 0;
}

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
extern CHARREC RoomOpp[5];
extern int guard_wait;

extern signed char far pascal get_behind(void);
extern signed char far pascal get_under_foot(void);
extern signed char far pascal get_cells_ahead(signed char distance);
extern int far pascal IS_SIX_KIND(signed char tile);
extern int far pascal IsImpassable(signed char tile);
extern int far pascal GetBarrCode(signed char tile);
extern signed char far pascal ReadCell(unsigned char room, signed char column, signed char row);
extern int far pascal generator_has_tumble(unsigned char room);
extern int far pascal Rnd(int limit);
extern void far pascal load_actor_slot(signed char id);
extern int far pascal is_dead_pos(int frame);
extern void far pascal copy_kid_to_char(void);
signed char far pascal count_live_opponents(void);

int far pascal check_opponent_tumble(void)
{
    signed char tile, live, count, index;
    int result;
    CHARREC saved;

    if (Level.environment_kind == 5)
        result = Char.room == 19 || Char.room == 16 ||
            (IS_SIX_KIND(get_behind()) && Char.direction == -1);
    else
        result = 0;

    if (!result && generator_has_tumble(Char.room)) {
        live = count_live_opponents();
        result = (signed char)Rnd(3) <= live;
        if (!result && live != 0) {
            saved = Char;
            count = Level.room_objects[Room.number - 1].opponent_count;
            index = 0;
            if (index < count) {
                do {
                    if (result)
                        break;
                    if (index != saved.slot) {
                        load_actor_slot(index);
                        if (Char.row == saved.row &&
                            Char.column == saved.column &&
                            is_dead_pos(Char.frame))
                            result = 1;
                    }
                } while (++index < count);
            }
            Char = saved;
        }
        if (result) {
            if (Opp.direction == Char.direction)
                result = 0;
            else {
                tile = get_under_foot();
                if (tile == 3 || tile == 8 || tile == 4)
                    result = 0;
                else {
                    tile = get_behind();
                    if (tile == 3 || tile == 8)
                        result = 0;
                }
            }
        }
    }
    return result;
}

void far pascal check_guard_step(void)
{
    signed char tile;
    int stop;

    if (Opp.action == 2 || Opp.action == 6)
        return;

    stop = 0;
    tile = get_cells_ahead(1);
    if (GetBarrCode(tile)) {
        stop = 1;
    } else if (!IS_SIX_KIND(tile)) {
        auto_forward();
    } else {
        ++Cell.row;
        tile = ReadCell(Cell.room, Cell.column, Cell.row);
        if ((IsImpassable(tile) && Char.character_kind != 10) ||
            GetBarrCode(tile) || IS_SIX_KIND(tile) ||
            Char.row - Opp.row + 1 != 0) {
            stop = 1;
        } else {
            if (Level.background_overlays[0] != stop &&
                ReadCell(Cell.room, Cell.column - 1, Cell.row) == 2)
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
    limit = Level.room_objects[Room.number - 1].opponent_count;
    index = 0;
    if (index < limit) {
        do {
            if (RoomOpp[index].alive >= 0)
                ++live;
        } while (++index < limit);
    }
    return live;
}


extern int effect_y, guard_counter;
extern void far pascal generic_fight(int distance);
extern void far _fastcall get_char_strike_ranges(int near *, int near *);
extern int far pascal bridge_skeleton_can_advance(void);
extern int far pascal get_opp_dist(void);

void far pascal check_guard_range(int distance)
{
    int horizontal, vertical;

    if (Opp.stance == 1 && Opp.direction != Char.direction) {
        generic_fight(distance); return;
    }

    if (effect_y == 0) {
        get_char_strike_ranges(&vertical, &horizontal);
        if (horizontal > distance) {
            set_overlay_state_bytes(254);
            return;
        }
        if (!Char.row || (unsigned char)Level.level_number != 5 ||
            (Char.room != 10 && Char.room != 7 && Char.room != 12) ||
            bridge_skeleton_can_advance())
            auto_forward();
    }
    return;
}

void far pascal alert_opponent(void)
{
    int distance;

    if (Opp.alive < 0) {
        distance = get_opp_dist();
        if (Opp.row == Char.row && distance < 0 && distance >= -18) {
            if ((signed char)Char.alert_delay >= 2 && Char.frame == 166) goto engage;
        } else if (guard_counter) {
            if (distance < 0 && distance < -4) {
                auto_down();
                return;
            }
        } else if (distance >= 0 && Char.alert_delay != 0) {
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

extern signed char far pascal pick_hidden_actor(signed char facing);
extern Opponent near *far pascal GetOppInfoIdx(int room, int id);
extern int far pascal can_continue_run_jump(signed char tile, int modifier);
extern int far pascal get_edge_distance(void);
extern int far pascal AddCharX(int amount);
extern int far pascal IsExitChasingSkel(unsigned char room);
void far pascal try_opponent_run_jump(void);

extern int abs(int value);
#pragma intrinsic(abs)

void far pascal chase_opponent(void)
{
    signed char tile;
    Opponent near *opponent;
    int nearest, kid_distance;
    int sequence;

    if (!(((unsigned int)Char.frame >= 194 && (unsigned int)Char.frame <= 199 &&
           (Char.frame & 1)) || Char.frame == 212)) {
        if ((signed char)Char.behavior >= 1 && (Char.frame == 192 || Char.frame == 196)) {
            try_opponent_run_jump(); return;
        }
        return;
    }

    tile = pick_hidden_actor(~Char.direction);
    if (tile != Char.slot)
        nearest = abs(Char.x - RoomOpp[tile].x);
    else
        nearest = 121;

    tile = get_cells_ahead(1);
    kid_distance = abs(Char.x - Kid.x);
    if ((Kid.frame >= 1 && Kid.frame <= 14) ||
        (Kid.frame >= 49 && Kid.frame <= 56) ||
        (Kid.frame >= 34 && Kid.frame <= 44)) {
        if (Kid.direction != Char.direction)
            kid_distance -= 32;
        else
            kid_distance += 32;
    } else if (Kid.stance != 1) {
        kid_distance += 32;
    }

    opponent = GetOppInfoIdx((unsigned char)Char.room, Char.slot);
    if ((Kid.row == Char.row || opponent->row == Kid.row) &&
        can_continue_run_jump(tile, (int)Cell.modifier)) {
        if ((Kid.row == Char.row &&
             !(Opp.direction == Char.direction &&
               ((Opp.frame >= 1 && Opp.frame <= 14) ||
                (Opp.frame >= 49 && Opp.frame <= 56) ||
                (Opp.frame >= 34 && Opp.frame <= 44)) &&
               Opp.action != 7) &&
             (nearest <= 120 || kid_distance <= 120)) ||
            (Char.direction == -1 && Kid.column > Char.column) ||
            (Char.direction == 0 && Kid.column < Char.column)) {
            Char.alert_delay = 2;
            sequence = 101;
        } else if (Kid.alive < 0 && Kid.sequence_group != 13) {
            if (IsExitChasingSkel(Char.room) && kid_distance <= 80) {
                sequence = 87;
                Char.stance = 1;
            } else
                sequence = -1;
        } else {
            Char.alert_delay = 0;
            sequence = 101;
        }
    } else {
        kid_distance = get_edge_distance();
        if ((Kid.row == Char.row || opponent->row == Kid.row) &&
            !((signed char)Char.behavior >= 1 && kid_distance < 27) &&
            (signed char)Char.behavior >= 1 && tile != 4)
            sequence = -1;
        else {
            Char.alert_delay = 0;
            sequence = 101;
        }
    }

    if (sequence != -1) {
        JumpSeq(sequence);
        tile = get_behind();
        if (Char.frame == 212 && !can_continue_run_jump(tile, (int)Cell.modifier))
            Char.x = AddCharX(8);
    }
}

extern int far pascal run_jump(int id);

void far pascal try_opponent_run_jump(void)
{
    signed char tile1, tile2, start, steps_ahead;
    Opponent near *opponent;
    int attempt, selected;

    if (tile1 = get_cells_ahead(1), IS_SIX_KIND(tile2 = get_cells_ahead(2))) {
        if (IS_SIX_KIND(tile1))
            start = 1;
        else {
            start = 2;
            tile1 = tile2;
        }
        steps_ahead = 0;
        if (IS_SIX_KIND(tile1)) {
            while (steps_ahead <= 4) {
                ++steps_ahead;
                tile1 = ReadCell(Char.room, Char.column +
                                  direction_factor[Char.direction] * (steps_ahead + start),
                                  Char.row);
                if (!IS_SIX_KIND(tile1))
                    break;
            }
        }

        if (steps_ahead < 4 || (steps_ahead == 4 && !Rnd((signed char)Char.behavior - 1)) ||
            (Char.room == 13 && (unsigned char)Level.level_number == 13 && Kid.sequence_group != 13))
            attempt = 1;
        else {
            opponent = GetOppInfoIdx((unsigned char)Char.room, Char.slot);
            attempt = opponent && opponent->row != Char.row;
        }

        selected = attempt ? run_jump(100) : 0;
        if (!selected) {
            JumpSeq(101);
            Char.alert_delay = 2;
        }
    }
}


extern void far pascal guard_no_jump(void);
extern void far pascal wait_en_garde(void);
extern int far pascal bridge_skeleton_must_advance(void);
extern int effect_y;

void far pascal guard_en_garde(void)
{
    signed char tile;
    int upper, lower;
    unsigned int frame = Char.frame;
    unsigned int pose;
    int distance;

    if (Char.alert_delay == 1 && Char.alert_delay == 2) {
        wait_en_garde();
    } else if (frame != 166 && frame >= 150) {
        if (Char.alert_delay == 0) {
            if (guard_wait)
                check_guard_step();
            else if (Char.character_kind != 4) {
                auto_run_away();
                return;
            }
        } else {
            pose = Opp.frame;
            distance = get_opp_dist();
            if (distance >= 27 && pose >= 102 && pose < 118 && Opp.action == 5)
                return;

            get_char_strike_ranges(&upper, &lower);
            if (lower + 12 > distance) {
                if (upper > distance) {
                    if (Char.row && (unsigned char)Level.level_number == 5 &&
                        (Char.room == 10 || Char.room == 7 || Char.room == 12) &&
                        bridge_skeleton_must_advance()) {
                        auto_forward();
                        return;
                    }
                    tile = get_behind();
                    if (can_continue_run_jump(tile, (int)Cell.modifier) && distance > 0) {
                        auto_back();
                        return;
                    }
                    if (!Char.row || (unsigned char)Level.level_number != 5 ||
                        (Char.room != 10 && Char.room != 7 && Char.room != 12) ||
                        bridge_skeleton_can_advance()) {
                        auto_forward();
                        return;
                    }
                } else {
                    check_guard_range(distance);
                    return;
                }
            } else if (!effect_y) {
                if (Opp.direction != Char.direction) {
                    if (pose >= 7 && pose < 15) {
                        if (distance < 71)
                            set_overlay_state_bytes(254);
                        return;
                    }
                    if (pose >= 34 && pose < 44) {
                        if (distance < 94)
                            set_overlay_state_bytes(254);
                        return;
                    }
                }
                guard_no_jump();
            }
        }
    }
}

extern int far pascal opponent_spacing(void);

extern void far pascal maybe_advance(void);
#define RUNNING_FRAME(frame) \
    (((frame) >= 1 && (frame) <= 14) || \
     ((frame) >= 49 && (frame) <= 56) || \
     ((frame) >= 34 && (frame) <= 44))

extern int far pascal IsExitOpen(void);
static const unsigned int strike_chance[12] =
    {75, 100, 75, 75, 75, 50, 100, 220, 0, 60, 40, 60};
static const unsigned int quick_strike_chance[12] =
    {0, 0, 0, 5, 5, 175, 20, 10, 0, 255, 255, 150};
void far pascal wait_en_garde(void)
{
    signed char slot, tile;
    int spacing, distance, can_stand;

    slot = pick_hidden_actor(~Char.direction);
    if (slot == -1)
        slot = pick_hidden_actor(1);
    if (slot == -1)
        return;
    do { if (Char.slot != slot) {
        can_stand = 0;
        tile = get_cells_ahead(1);
        if (can_continue_run_jump(tile, (int)Cell.modifier) &&
            can_continue_run_jump(tile = get_cells_ahead(2), (int)Cell.modifier) &&
            can_continue_run_jump(tile = get_cells_ahead(3), (int)Cell.modifier))
            can_stand = 1;

        distance = abs(Char.x - RoomOpp[slot].x);
        spacing = opponent_spacing();
        if (spacing + 24 >= distance &&
            (RoomOpp[slot].direction != Char.direction || RoomOpp[slot].sequence != 84)) {
            if (can_stand && Opp.direction == Char.direction &&
                RUNNING_FRAME(Opp.frame) && Opp.action != 7 && Char.behavior &&
                ((Char.x < Opp.x && Opp.direction == 0) ||
                 (Char.x > Opp.x && Opp.direction == -1))) {
                start_opponent_running();
                return;
            }

            if (distance < spacing)
                break;
            return;
        } else {
            if (!can_stand || Char.sequence == 84)
                return;
            if (((RoomOpp[slot].sequence == 84 && RoomOpp[slot].direction == Char.direction) ||
                 (Opp.direction == Char.direction && RUNNING_FRAME(Opp.frame) && Opp.action != 7)) &&
                Char.behavior)
                start_opponent_running();
            else
                auto_forward();
            return;
        }
    } else {
        distance = abs(Kid.x - Char.x);
        if (distance >= 64)
            return;
        if (distance < 32)
            tile = 1;
        else
            tile = 2;
        tile = get_cells_ahead(tile);
        if (!IsImpassable(tile) && !IS_SIX_KIND(tile) &&
            (tile != 4 || (unsigned char)(int)Cell.modifier >= 112))
            return;
    } } while (0);
    if (can_continue_run_jump(get_behind(), (int)Cell.modifier))
        auto_back();
    return;
}

int far pascal opponent_spacing(void)
{
    int spacing = 96;
    signed char index, count;

    count = Level.room_objects[Room.number - 1].opponent_count;
    index = Char.slot + 1;
    if (count > index) {
        do {
            if (abs(Char.x - RoomOpp[index].x) < 24)
                spacing += 24;
        } while (++index < count);
    }
    return spacing;
}

void far pascal guard_no_jump(void)
{
    signed char front, further;

    front = get_cells_ahead(1);
    further = get_cells_ahead(2);
    if (IS_SIX_KIND(front) && IS_SIX_KIND(further)) {
        if (!(Char.row && (unsigned char)Level.level_number == 5 &&
              (Char.room == 10 || Char.room == 7 || Char.room == 12) &&
              bridge_skeleton_must_advance()))
            auto_back();
        return;
    }

    if (Opp.direction == Char.direction && RUNNING_FRAME(Opp.frame) &&
        Opp.action != 7 && Char.behavior &&
        (!Char.row || (unsigned char)Level.level_number != 5 ||
         (Char.room != 10 && Char.room != 7 && Char.room != 12))) {
        start_opponent_running();
        return;
    }
    maybe_advance();
}

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

    get_char_strike_ranges(&upper, &lower);
    if (upper <= distance && distance < lower &&
        !(Char.row && (unsigned char)Level.level_number == 5 &&
          (Char.room == 10 || Char.room == 7 || Char.room == 12) &&
          bridge_skeleton_must_advance())) {
        maybe_block();
        if (effect_y == 0)
            maybe_strike();
    } else {
        maybe_advance();
    }
}

void far pascal maybe_advance(void)
{
    unsigned char skill;

    if (Char.row && (unsigned char)Level.level_number == 5 &&
        (Char.room == 10 || Char.room == 7 || Char.room == 12) &&
        bridge_skeleton_must_advance())
        goto advance;
    if ((skill = Level.room_objects[(unsigned char)Char.room - 1].opponent_data[Char.slot][4]) && advance_timer)
        return;
    if (Char.row && (unsigned char)Level.level_number == 5 &&
        (Char.room == 10 || Char.room == 7 || Char.room == 12) &&
        !bridge_skeleton_can_advance())
        return;

    if (advance_chance[skill] > Rnd(255)) {
advance:
        auto_forward();
    }
}

void far pascal maybe_block(void)
{
    unsigned char skill;

    if (Char.character_kind == 10 ||
        (Opp.frame != 152 && Opp.frame != 153 && Opp.frame != 162))
        return;
    skill = Level.room_objects[(unsigned char)Char.room - 1].opponent_data[Char.slot][4];

    if (effect_x) {
        if (quick_block_chance[skill] > Rnd(255))
            goto act;
    } else if (block_chance[skill] > Rnd(255)) {
act:
        auto_up();
    }
}

void far pascal maybe_strike(void)
{
    unsigned char skill;

    if (Opp.frame == 169 || Opp.frame == 151)
        return;
    skill = Level.room_objects[(unsigned char)Char.room - 1].opponent_data[Char.slot][4];
    if (Char.character_kind != 7 && Char.character_kind != 8 &&
        (Char.frame == 161 || Char.frame == 150)) {
        if (quick_strike_chance[skill] > Rnd(255))
            goto act;
    } else if (strike_chance[skill] > Rnd(255)) {
act:
        set_overlay_state_bytes(254);
    }
}

extern int far pascal IsWall(signed char tile);
extern int far pascal gate_blocks_passage(void);
extern void far pascal save_actor_slot(void);
extern void far pascal set_opponent_life(Opponent near *opponent);
extern void far pascal ClearCharVel(void);


int far pascal skeleton_finished_shaking(signed char id);
void far pascal bones_fall(void);
void far pascal face_kid_for_skeleton(void);

int far pascal opponent_close(signed char slot, signed char column, signed char row, signed char radius)
{
    signed char tile, limit, step;
    int clear = 0;
    Opponent near *opponent = GetOppInfoIdx(Room.number, slot);

    if (!opponent || Char.alive < 0 || Char.sequence_group == 5 || Kid.character_kind)
        goto done;
    if (opponent->timer && !IsExitChasingSkel(Char.room))
        goto done;

    if (Char.column > column) {
        limit = Char.column + 1;
        step = 1;
    } else {
        limit = Char.column - 1;
        step = -1;
    }

    if (Char.row == row && column - radius <= Char.column && column + radius >= Char.column) {
        do {
            tile = ReadCell(Char.room, column, row);
            if ((!IsImpassable(tile) || Char.character_kind == 10) &&
                !IsWall(tile) && (tile != 4 || !gate_blocks_passage()))
                clear = 1;
            else
                clear = 0;
            column += step;
        } while (clear && limit != column);
    }
done:
    return clear;
}

extern int far pascal query_sound(int sequence);
extern void far pascal AddSong(int request);
extern void far pascal remove_char(void);
extern void far pascal mark_foreground(void);
extern unsigned int sprite_table_handle;
extern int far pascal bridge_skeleton_progress(void);
extern void far pascal run_guard_script(void);

void far pascal raise_nearby_opponents(void)
{
    signed char index, count, radius;
    int started;
    Opponent near *opponent;

    if (IsExitChasingSkel((unsigned char)Kid.room))
        radius = 8;
    else if (Level.preferred_opponent == 2)
        radius = 4;
    else
        radius = 5;

    count = Level.room_objects[Room.number - 1].opponent_count; index = 0;
    if (count > 0) {
        do {
            load_actor_slot(index);
            if (Kid.alive < 0 && Kid.action != 4 && Kid.action != 3 &&
                Kid.action != 5 && opponent_close(index, Kid.column, Kid.row, radius)) {
                opponent = GetOppInfoIdx((unsigned char)Char.room, Char.slot);
                if (Level.preferred_opponent == 2) {
                    if (opponent->shake_delay != -1) {
                        started = opponent->timer; if (skeleton_finished_shaking(Char.slot)) {
                            JumpSeq(88);
                            started = 1;
                            Char.character_kind = 4;
                        } else {
                            if (!started && Char.sequence != 112)
                                JumpSeq(112);
                            started = 0;
                        }
                    }
                } else {
                    JumpSeq(106);
                    started = 1;
                    Char.character_kind = 10;
                }
                AnimChar();
                if (started) {
                    set_opponent_life(opponent);
                    Char.alive = (unsigned char)-1;
                    Char.stance = 1;
                    Char.alert_delay = 3;
                    if (Char.character_kind == 10) {
                        signed char facing;
                        if (Char.x >= Kid.x)
                            facing = -1;
                        else
                            facing = 0;
                        Char.direction = facing;
                    }
                    guard_counter = 0;
                    *(volatile int near *)&effect_y = 0;
                    ClearCharVel();
                    if (Kid.opponent_index == -1)
                        Kid.opponent_index = Char.slot;
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

    if ((signed char)Char.alive < 0) {
        active = 1;
        if (Char.row && (unsigned char)Level.level_number == 5 &&
            (Char.room == 10 || Char.room == 7 || Char.room == 12))
            active = bridge_skeleton_progress();
        else if (Kid.alive >= 5 && Char.character_kind == 4 && Char.sequence != 120) {
            bones_fall();
            active = 0;
        }

        if (active) {
            if (Char.sequence == 88 && Char.frame == 177) {
                face_kid_for_skeleton();
                return;
            }
            if (active) {
                run_guard_script();
                return;
            }
        }
    } else {
        opponent = GetOppInfoIdx((unsigned char)Char.room, Char.slot);
        if (opponent && opponent->timer > 0)
            --opponent->timer;
    }
}



extern Opponent near *far pascal GetOppInfoIdx(int room, int id);
extern void far pascal load_actor_slot(signed char id);
extern void far pascal JumpSeq(int sequence);
extern void far pascal save_actor_slot(void);

extern int far pascal IS_SIX_KIND(signed char tile);
extern int far pascal get_edge_distance(void);
extern int far pascal AddCharX(int amount);
extern void far pascal AddSound(int id);
extern void far pascal set_guard_life(signed char id, int flags, int health);
extern int far pascal decrement_life_points(signed char amount);
extern void far pascal begin_actor_death(void);
void far pascal set_skeleton_timer(signed char id, int duration);
void far pascal set_skeleton_shakes(signed char id);

void far pascal bones_fall(void)
{
    int distance;

    JumpSeq(120);
    AddSound(76);
    Char.field_0f = 1;
    Char.stance = 0;
    if ((signed char)Char.alive < 0)
        Char.alive = 0;

    if (Char.direction == 0 && IS_SIX_KIND(get_behind())) {
        distance = 32 - get_edge_distance();
        if (distance <= 35) {
            distance -= 35;
            Char.x = AddCharX(-distance);
        }
    }
    save_actor_slot();
    set_guard_life(Char.slot, 0, Char.life_initial);
    set_skeleton_timer(Char.slot, 120);
    set_skeleton_shakes(Char.slot);
    load_actor_slot(Char.slot);
}


int far pascal IsExitChasingSkel(unsigned char value)
{
    int result = 0;

    if (Level.level_number == 4 && value >= 22 && value <= 28 && value != 23) {
        result = 1;
    }
    return result;
}

void far pascal set_skeleton_shakes(signed char id)
{
    Opponent near *opponent = GetOppInfoIdx(Room.number, id);
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
    if (Level.preferred_opponent == 2 || RoomOpp[0].character_kind == 10) {
        opponent = GetOppInfoIdx(Room.number, id);
        if (opponent)
            opponent->timer = duration;
    }
}


void far pascal shake_bones(unsigned char room, signed char row)
{
    signed char index, count;
    CHARREC saved;
    Opponent near *opponent;

    saved = Char;
    index = 0;
    count = Level.room_objects[room - 1].opponent_count;
    if (count > 0) {
        do {
            load_actor_slot(index);
            if (Char.row == row && Char.frame == 206 && Char.sequence_group != 5) {
                opponent = GetOppInfoIdx((unsigned char)Char.room, Char.slot);
                if (opponent->shake_delay == 0) {
                    JumpSeq(112);
                    save_actor_slot();
                }
            }
        } while (++index < count);
    }
    Char = saved;
}

extern int far pascal IS_SIX_KIND(signed char tile);
extern int far pascal get_edge_distance(void);
extern int far pascal AddCharX(int amount);
extern void far pascal AddSound(int id);

Opponent near *far pascal initialize_skeleton(Opponent near *opponent)
{
    int distance;

    if (opponent->shake_delay == 1) {
        opponent->sequence = 63;
        Char.stance = 1;
    } else {
        if (Char.slot == 0 && opponent->shake_delay == 0)
            opponent->sequence = 112;
        else
            opponent->sequence = 119;
        opponent->timer = opponent->shake_delay;
        set_skeleton_shakes(Char.slot);
        Char.stance = 0;

        if ((Char.direction == -1 && IS_SIX_KIND(get_cells_ahead(1))) ||
            (Char.direction == 0 && IS_SIX_KIND(get_behind()))) {
            distance = get_edge_distance();
            if (Char.direction == 0)
                distance = 32 - distance;
            if (distance <= 35) {
                distance = 35 - distance;
                if (Char.direction == -1)
                    distance = -distance;
                Char.x = AddCharX(distance);
            }
        }
    }
    opponent->position = 0;
    Char.field_34 = 8;
    return opponent;
}

void far pascal impale_skeleton(void)
{
    if (decrement_life_points(1)) {
        begin_actor_death();
        return;
    }
    JumpSeq(126);
    AddSound(72);
}


extern signed char far pascal ReadCell(unsigned char room, signed char column, signed char row);
#define random_below Rnd

int far pascal skeleton_finished_shaking(signed char id)
{
    int result;
    int reset_timer;
    Opponent near *opponent = GetOppInfoIdx(Room.number, id);

    if (opponent) {
        result = 0;
        reset_timer = result;
        if (opponent->shake_delay != -1) {
            if (IsExitChasingSkel(Char.room)) {
                ReadCell(22, 4, 2);
                if (IsExitOpen() && Kid.room == (signed char)Char.room &&
                    Kid.x < Char.x) {
                    if (!query_sound(10106))
                        AddSong(106);
                    result = 1;
                } else {
                    reset_timer = opponent->timer == 0 && Char.sequence == 112;
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

#pragma pack(1)
typedef struct {
    int image, equipment_image;                       /* 00,02 */
    signed char x, y;                                 /* 04-05 */
    unsigned char flags;                              /* 06 */
} FRAMEREC;
#pragma pack()
extern FRAMEREC Frame;
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
extern int far pascal AddCharX(int delta);
extern int far pascal AddFCharX(int delta);
extern SpriteFrame far *far pascal RESOLVE_HANDLE_TO_FAR_PAYLOAD(unsigned int handle);


void far pascal face_kid_for_skeleton(void)
{
    if (Kid.x > Char.x) {
        if (Char.direction == 0)
            return;
        Char.direction = 0;
    } else {
        if (Char.direction == -1)
            return;
        Char.direction = -1;
    }
    Char.x = AddCharX(10);
}

void far pascal setup_flame_sword(void)
{
    SpriteFrame far *table = RESOLVE_HANDLE_TO_FAR_PAYLOAD(sprite_table_handle);
    AddFCharX(table[(unsigned int)Frame.equipment_image].x);
    FChar.y += table[(unsigned int)Frame.equipment_image].y;

    if (FChar.image == 4 || FChar.image == 19 || FChar.image == 20) {
        if (Char.flame_offset != 0)
            FChar.image = 18 - Char.flame_offset;
        if (Char.flame_offset <= -4) {
            Char.flame_offset = 0;
            return;
        }
        --Char.flame_offset;
    }
}


extern void far pascal AddSound(int sound);
extern void far pascal reset_char_y(void);
extern int far pascal decrement_life_points(signed char points);
extern void far pascal JumpSeq(int sequence);
extern void far pascal set_skeleton_timer(signed char id, int duration);
extern void far pascal AddSong(int effect);
int far pascal reduce_flame_sword_life(signed char points);
void far pascal drop_flame_sword(void);

int far pascal block_flame_sword(void)
{
    int sequence = reduce_flame_sword_life(1) ? -1 : 103;
    AddSound(12);
    reset_char_y();
    return sequence;
}

int far pascal reduce_flame_sword_life(signed char points)
{
    int result;
    if (decrement_life_points(points)) {
        drop_flame_sword();
        result = 1;
    } else {
        result = 0;
    }
    return result;
}

void far pascal drop_flame_sword(void)
{
    JumpSeq(107);
    set_skeleton_timer(Char.slot, 480);
    AddSong(192);
    Char.alive = 0;
    Char.field_0f = 1;
}





#define room_column direction_factor
extern int guard_counter, effect_x, effect_y;

extern signed char far pascal get_col(int x);
extern signed char far pascal FirstInRow(signed char row);
extern void far pascal reset_char_y(void);
extern void far pascal JumpSeq(int sequence);
extern void far pascal AddSong(int effect);
extern void far pascal set_opponent_life(Opponent near *opponent);
extern void far pascal ClearCharVel(void);
extern void far pascal save_actor_slot(void);




extern SpriteFrame far *far pascal RESOLVE_HANDLE_TO_FAR_PAYLOAD(unsigned int handle);
extern int far pascal AddFCharX(int x);
extern void far pascal MakeFCharClipRect(void);
extern void far _fastcall add_char_object(unsigned char style);
typedef void (far pascal *SmokeDrawer)(void);
extern void far pascal mark_bufs_for_rect(SmokeDrawer draw, void near *clip, signed char id);
#define smoke_table_handle sprite_table_handle

extern void far pascal OFFSETRECT(void far *destination, void far *source, int dx, int dy);

void far pascal float_flame_sword(void)
{
    int row_y;
    int amount, drift, dy;

    if (FChar.image == 4 || FChar.image == 19 || FChar.image == 20) {
        amount = 4;
        if ((row_y = Char.row * 63) + 54 >= Char.y) {
            drift = amount = 1;
        } else {
            drift = -2;
            if (Char.row * 63 + 58 <= Char.y)
                amount = 1;
        }

        dy = random_below(amount) + drift;
        Char.y += dy;
        FChar.y += dy;
        OFFSETRECT(&FChar.clip_top, &FChar.clip_top, 0, dy);
    }
}

void far pascal check_jinnee_column(void)
{
    if (Char.column < -1 || Char.column > 10)
        remove_char();
}

void far pascal setup_jinnee_smoke(void)
{
    SpriteFrame far *table;
    unsigned int frame;

    if ((unsigned int)Frame.equipment_image >= 171 && (unsigned int)Frame.equipment_image <= 199) {
        frame = (unsigned int)Frame.equipment_image;
        table = RESOLVE_HANDLE_TO_FAR_PAYLOAD(smoke_table_handle);
        if (table[frame].frame != -1) {
            FChar.image = table[frame].frame;
            AddFCharX(table[frame].x);
            FChar.y += table[frame].y;
            FChar.image_set = 3;
            MakeFCharClipRect();
            add_char_object(9);
            mark_bufs_for_rect(mark_foreground, &FChar.clip_top, Char.slot);
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
    x = 85 * room_column[Kid.direction] + Kid.x;
    column = get_col(x);
    row = Kid.row;
    opponent = GetOppInfoIdx((unsigned char)Kid.room,
                            Char.slot = Level.room_objects[(unsigned char)Kid.room - 1].opponent_count++);
    Char.character_kind = 12;
    opponent->kind = 9;
    Char.room = Kid.room;
    Char.row = row;
    opponent->cell = FirstInRow(row) + column;
    reset_char_y();
    Char.x = opponent->x = x;
    Char.column = column;
    Char.direction = opponent->facing = -1;
    column = 0;
    opponent->motion = column;
    Char.behavior = column;
    opponent->secondary_motion = column;
    Char.opponent_index = column;
    Char.alert_delay = column;
    Char.previous_frame = 0;
    Char.sequence_group = Char.previous_y = Char.previous_x = 0;
    Char.field_34 = 4;
    Char.stance = Char.field_0f = 1;
    opponent->sequence = 180;
    opponent->position = 0;
    JumpSeq(180);
    AddSong(196);
    opponent->health = 6;
    set_opponent_life(opponent);
    opponent->state = 10;
    Char.alive = -1;
    guard_counter = 0;
    effect_x = 0;
    effect_y = 0;
    ClearCharVel();
    Char.action = 1;
    load_changed_opponent_set(opponent->kind);
    save_actor_slot();
    copy_kid_to_char();
}



