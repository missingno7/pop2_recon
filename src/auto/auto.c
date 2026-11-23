#pragma pack(1)
typedef struct {
    unsigned char number;                             /* 00 */
    unsigned char left, right, above, below;           /* 01-04 */
    unsigned char above_left, above_right;             /* 05-06 */
    unsigned char below_left, below_right;             /* 07-08 */
    unsigned char custom;                             /* 09 */
} ROOMREC;

typedef struct {
    signed char kind;                                 /* 00 */
    unsigned int modifier;                            /* 01 */
    signed char index;                                /* 03 */
    unsigned char room;                               /* 04 */
    signed char column, row;                          /* 05-06 */
    signed char first_col, last_col;                   /* 07-08 */
    unsigned char top_row, bottom_row;                 /* 09-0a */
} CELLREC;

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

/* AUTO.C ordered reconstruction through 2344:0fae. */

extern int Pause;
extern signed char cut_blocked_index;
extern CHARREC Char;


void far pascal SetOppMaxLifePts(register unsigned char *p);

void far pascal reset_overlay_state(void)
{
    Pause = 0;
}

void far pascal set_opponent_life(unsigned char *record)
{
    SetOppMaxLifePts(record);
    if (((*(volatile unsigned char near *)&Char.character_kind) == 4 && (*(volatile int near *)&Char.sequence) == 119) ||
        ((*(volatile unsigned char near *)&Char.character_kind) == 10 && (*(volatile int near *)&Char.sequence) == 105)) {
        (*(unsigned char near *)&Char.life_points) = 0;
        (*(unsigned char near *)&Char.life_delta) = 0;
    } else {
        (*(unsigned char near *)&Char.life_points) = (*(unsigned char near *)&Char.life_initial);
        (*(unsigned char near *)&Char.life_delta) = (*(unsigned char near *)&Char.life_points);
    }
}

void far pascal SetOppMaxLifePts(register unsigned char *p)
{
    if (p[16] == 0)
        (*(unsigned char near *)&Char.life_initial) = 3;
    else
        (*(unsigned char near *)&Char.life_initial) = p[16];
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


extern LEVELREC Level;

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
extern ROOMREC Room;
extern unsigned char loaded_appearance;
extern unsigned char appearance_kind[];

#define actor_index Char.slot

extern volatile unsigned char bank_opponents[2];
extern CHARREC Opp;
extern CHARREC Kid;


#define actor_room (*(unsigned char near *)&Char.room)

#define actor_bank_mask (*(volatile int near *)&Char.field_34)


extern int opponent_guard_timeout, opponent_lock, running_guard_state;


extern int previous_actor;


opponent_record *far pascal GetOppInfoIdx(int room, int index);
void far pascal check_side_rooms(void);
void far pascal add_opponent_to_room(int room, opponent_record *record);
void far pascal RemoveOppFromRoom(unsigned char room, int index);
void far _fastcall reserve_opponent_bank(unsigned char kind);
void far pascal load_opponent_bank(int resource, int start, unsigned char size,
                                   unsigned char kind);
void far pascal load_opponent_set(signed char appearance);
void far pascal reset_char_y(void);
signed char far pascal get_char_col(int x);
opponent_record *far pascal initialize_skeleton(opponent_record *record);
opponent_record *far pascal head_add_opponent(opponent_record *record);
int far pascal get_under_foot(void);
int far pascal is_open_tile(int stage);
void far pascal JumpSeq(int action);
void far pascal AnimChar(void);
int far pascal is_dead_pos(int action);
void far pascal check_multiple_dead_bodies(void);
void far pascal set_skeleton_timer(signed char index, int behavior);
void far pascal ClearCharVel(void);
void far pascal save_actor_slot(void);

void far pascal initialize_room_actors(void)
{
    opponent_record *record;
    signed char count;
    int index;

    check_side_rooms();
    index = 0;
    count = Level.room_objects[Room.number - 1].opponent_count;
    if (count > 0) {
        do {
            record = GetOppInfoIdx(Room.number, index);
            if (Level.preferred_opponent != 5 && Level.preferred_opponent != 6 &&
                record->appearance != 1 && record->appearance != 9 &&
                record->appearance != 10 && record->appearance != 3)
                record->appearance = Level.preferred_opponent;

            (*(volatile unsigned char near *)&Char.character_kind) = appearance_kind[record->appearance];
            if ((unsigned char)record->appearance != loaded_appearance &&
                Level.preferred_opponent != 5 && Level.preferred_opponent != 6 &&
                (*(volatile unsigned char near *)&Char.character_kind) != 11 && (*(volatile unsigned char near *)&Char.character_kind) != 0 && (*(volatile unsigned char near *)&Char.character_kind) != 6)
                load_opponent_set(record->appearance);

            actor_index = index;
            actor_room = Room.number;
            Char.row = record->tile / 10;
            if (record->tile < 0)
                --Char.row;

            if ((*(volatile unsigned char near *)&Char.character_kind) == 2 && (record->action == 100 || record->action == 100)) {
                Char.y = record->saved_y;
            } else if ((*(volatile unsigned char near *)&Char.character_kind) == 6 && record->entry_y != 0) {
                Char.y = record->entry_y;
                if (record->action == 12) {
                    Char.horizontal_step = 2;
                    Char.vertical_step = 18;
                }
            } else {
                reset_char_y();
            }
            Char.x = record->x;
            Char.column = get_char_col(Char.x);
            (*(volatile signed char near *)&Char.direction) = record->direction;
            Char.behavior = record->behavior;
            (*(unsigned char near *)&Char.opponent_index) = record->behavior_type;
            Char.previous_frame = 0;
            Char.sequence_group = Char.previous_y = Char.previous_x = 0;
            Char.field_3a = (*(unsigned char near *)&Char.alert_delay) = 0;

            if ((*(volatile unsigned char near *)&Char.character_kind) == 0)
                actor_bank_mask = 2;
            else if ((*(volatile unsigned char near *)&Char.character_kind) == 6) {
                actor_bank_mask = 4;
                load_opponent_bank(25001u, 32, 16, 7);
            } else if (Level.preferred_opponent != 0 && Level.preferred_opponent != 5 && Level.preferred_opponent != 6)
                actor_bank_mask = 4;
            else
                reserve_opponent_bank(record->palette_kind);

            if (record->saved_sequence == 0) {
                if ((*(volatile unsigned char near *)&Char.character_kind) == 4)
                    record = initialize_skeleton(record);
                else if ((*(volatile unsigned char near *)&Char.character_kind) == 7 || (*(volatile unsigned char near *)&Char.character_kind) == 8 || (*(volatile unsigned char near *)&Char.character_kind) == 11)
                    record = head_add_opponent(record);
                else if ((*(volatile unsigned char near *)&Char.character_kind) == 10) {
                    if (is_open_tile(get_under_foot())) {
                        record->action = 105;
                        (*(unsigned char near *)&Char.stance) = 0;
                    } else {
                        record->action = 106;
                        (*(unsigned char near *)&Char.stance) = 1;
                        (*(unsigned char near *)&Char.alert_delay) = 3;
                        if (Char.x >= Kid.x)
                            (*(volatile signed char near *)&Char.direction) = -1;
                        else
                            (*(volatile signed char near *)&Char.direction) = 0;
                    }
                } else if ((*(volatile unsigned char near *)&Char.character_kind) == 6)
                    record->action = 2;
                else {
                    if (record->action != 22)
                        record->action = 77;
                    (*(unsigned char near *)&Char.stance) = 0;
                }
                JumpSeq(record->action);
                set_opponent_life((unsigned char *)record);
            } else {
                (*(unsigned char near *)&Char.field_0f) = 1;
                Char.sequence_id = record->action;
                Char.sequence_position = record->saved_sequence;
                (*(volatile int near *)&Char.sequence) = record->action;
                (*(unsigned char near *)&Char.stance) = record->combat_mode;
                (*(unsigned char near *)&Char.life_points) = (*(unsigned char near *)&Char.life_delta) = record->life;
                if ((*(volatile unsigned char near *)&Char.character_kind) != 0)
                    SetOppMaxLifePts((unsigned char *)record);
            }

            AnimChar();
            if (is_dead_pos((*(volatile int near *)&Char.frame)) && (*(volatile unsigned char near *)&Char.character_kind) != 0) {
                if ((*(volatile unsigned char near *)&Char.character_kind) == 2)
                    check_multiple_dead_bodies();
                (*(unsigned char near *)&Char.alive) = 7;
                (*(unsigned char near *)&Char.life_points) = (*(unsigned char near *)&Char.field_0f) = 0;
                (*(unsigned char near *)&Char.life_delta) = -(*(unsigned char near *)&Char.life_initial);
                if ((*(volatile unsigned char near *)&Char.character_kind) == 4 || (*(volatile unsigned char near *)&Char.character_kind) == 10)
                    set_skeleton_timer(actor_index, 0);
            } else {
                (*(unsigned char near *)&Char.alive) = -1;
                opponent_guard_timeout = opponent_lock = running_guard_state = 0;
                if ((*(volatile unsigned char near *)&Char.character_kind) == 11 && (*(volatile int near *)&Char.frame) == 264)
                    Char.sequence_group = 6;
            }
            if ((*(volatile unsigned char near *)&Char.character_kind) == 6 && (*(volatile int near *)&Char.sequence) == 12)
                (*(unsigned char near *)&Char.action) = 4;
            else {
                ClearCharVel();
                (*(unsigned char near *)&Char.action) = (*(volatile unsigned char near *)&Char.character_kind) == 7 || (*(volatile unsigned char near *)&Char.character_kind) == 8 ? 0 : 1;
            }
            save_actor_slot();
        } while (count > ++index);
    }

    if (count == 0) {
        if (Level.level_number == 12 && (Room.number == 11 || Room.number == 19) &&
            loaded_appearance != 1)
            load_opponent_set(1);
        else if (Level.generators[Room.number - 1].count != 0 &&
                 loaded_appearance != Level.preferred_opponent &&
                 Level.preferred_opponent != 5 && Level.preferred_opponent != 6)
            load_opponent_set(Level.preferred_opponent);
    }
}

void far pascal check_side_rooms(void)
{
    struct {
        int shift, side;
        unsigned char room;
        signed char count;
        int index;
    } scan;
    int displacement;
    opponent_record *record;
    opponent_record copy;

    for (scan.side = 0; scan.side < 2; ++scan.side) {
        if (scan.side == 0) {
            scan.room = Room.left;
            displacement = -320;
        } else {
            scan.room = Room.right;
            displacement = 320;
        }
        if (scan.room == 0)
            continue;
        scan.shift = displacement;
        for (scan.index = 0, scan.count = Level.room_objects[scan.room - 1].opponent_count; scan.index < scan.count; ++scan.index) {
            record = GetOppInfoIdx(scan.room, scan.index);
            if ((scan.side == 0 && record->x > 367) || (scan.side == 1 && record->x < 208) ||
                ((scan.room == 7 || scan.room == 8) && Level.environment_kind == 6 && record->appearance == 3)) {
                copy = *record;
                copy.x += scan.shift;
                copy.index = Level.room_objects[Room.number - 1].opponent_count;
                add_opponent_to_room(Room.number, &copy);
                RemoveOppFromRoom(scan.room, scan.index);
                --scan.index;
                --scan.count;
            }
        }
    }
}

extern signed char appearance_for_kind[];


extern int actor_y_offset[];
extern int generator_x_left[], generator_x_right[];
extern signed char direction_row_adjustment[];


extern CHARREC RoomOpp[5];

int abs(int value);
#pragma intrinsic(abs)

unsigned char far pascal register_actor_room(void);
void far pascal jam_plate(void);
signed char far pascal index_cell(signed char row, signed char column);
signed char far pascal FirstInRow(signed char column);
void far pascal dispatch_sound(int object_id);
void far pascal AddSound(int sound);
int far pascal cut_actor(int direction);
void far pascal snake_update_opp_info(opponent_record *record);
void far pascal update_jaffar_slot(opponent_record *record);
void far pascal set_special_opponent(opponent_record *record);
signed char far pascal pick_hidden_actor(signed char direction);
generator_point *far pascal find_active_generator(unsigned char room, signed char column);
int far pascal should_generate(generator_point *point, unsigned char room);
void far pascal GenerateOpponent(generator_point *point, unsigned char room);


int far pascal update_opponent_record(void)
{
    int removed = 0;
    opponent_record *record, *prior;
    unsigned char room;
    struct vertical_positions { int saved, other, entry; };
    opponent_record copy;

    if ((*(volatile signed char near *)&Char.direction) == 86 || Char.sequence_group == 5 || Char.sequence_group == 4 || (*(volatile int near *)&Char.sequence) == 196) {
        RemoveOppFromRoom(actor_room, actor_index);
        removed = 1;
    } else {
        record = GetOppInfoIdx(actor_room, actor_index);
        if (record != 0) {
            record->tile = index_cell(Char.column, Char.row);
            if (record->tile != 30) {
                record->x = Char.x;
                record->direction = (*(volatile signed char near *)&Char.direction);
                if ((signed char)(*(unsigned char near *)&Char.alive) < 0)
                    record->saved_sequence = removed;
                else {
                    record->saved_sequence = Char.sequence_position;
                    record->action = Char.sequence_id;
                }
                record->combat_mode = (*(unsigned char near *)&Char.stance);
                record->life = (*(unsigned char near *)&Char.life_points);
                record->appearance = appearance_for_kind[(signed char)(*(volatile unsigned char near *)&Char.character_kind)];
                set_special_opponent(record);
                room = get_under_foot();
                if (room == 5 || room == 6)
                    jam_plate();
            } else {
                room = register_actor_room();
                if (room != 0 && (*(volatile unsigned char near *)&Char.character_kind) != 12) {
                    prior = GetOppInfoIdx(actor_room, actor_index);
                    copy.tile = FirstInRow(Char.row);
                    copy.x = Char.x;
                    copy.direction = (*(volatile signed char near *)&Char.direction);
                    copy.flags = prior->flags;
                    copy.action = Char.sequence_id;
                    copy.saved_sequence = Char.sequence_position;
                    copy.palette_kind = prior->palette_kind;
                    copy.combat_mode = (*(unsigned char near *)&Char.stance);
                    copy.life = (*(unsigned char near *)&Char.life_points);
                    copy.behavior = Char.behavior;
                    copy.behavior_type = (*(unsigned char near *)&Char.opponent_index);
                    copy.appearance = prior->appearance;
                    copy.max_life = prior->max_life;
                    *(struct vertical_positions *)&copy.saved_y =
                        *(struct vertical_positions *)&prior->saved_y;
                    add_opponent_to_room(room, &copy);
                } else if ((*(unsigned char near *)&Char.action) == 9 && Level.environment_kind == 5) {
                    dispatch_sound(10025);
                    if (Room.number != 19 && Room.number != 16)
                        AddSound(0);
                }
                RemoveOppFromRoom(actor_room, actor_index);
                removed = 1;
                if (Level.room_objects[Room.number - 1].opponent_count == 0)
                    RoomOpp[actor_index].direction = 86;
            }
        }
        (*(unsigned char near *)&Char.life_points) = 0;
    }
    return removed;
}

void far pascal transfer_opponent_record(void)
{
    opponent_record copy;

    opponent_record *record;

    record = GetOppInfoIdx(actor_room, actor_index);
    copy = *record;
    RemoveOppFromRoom(actor_room, actor_index);
    cut_actor(previous_actor);
    copy.tile = FirstInRow(Char.row);
    copy.x = Char.x;
    copy.direction = (*(volatile signed char near *)&Char.direction);
    copy.action = Char.sequence_id;
    copy.saved_sequence = Char.sequence_position;
    copy.combat_mode = (*(unsigned char near *)&Char.stance);
    copy.life = (*(unsigned char near *)&Char.life_points);
    copy.palette_kind = (*(volatile int near *)&Char.field_34) == 4 ? bank_opponents[0] : bank_opponents[1];
    set_special_opponent(&copy);
    add_opponent_to_room(actor_room, &copy);
}

void far pascal set_special_opponent(opponent_record *record)
{
    if ((*(volatile unsigned char near *)&Char.character_kind) == 2) {
        if (((*(volatile int near *)&Char.sequence) == 186) | ((*(volatile int near *)&Char.sequence) == 100))
            record->saved_y = Char.y;
        if (Level.preferred_opponent == 7) {
            record->appearance = 7;
            return;
        }
    } else if ((*(volatile unsigned char near *)&Char.character_kind) == 11) {
        snake_update_opp_info(record);
        return;
    } else if ((*(volatile unsigned char near *)&Char.character_kind) == 6) {
        update_jaffar_slot(record);
    }
}

void far pascal RemoveOppFromRoom(unsigned char room, int index)
{
    int room_index = room;
    ROOMOBJECTS *list = &Level.room_objects[room_index - 1];
    signed char count = list->opponent_count;
    opponent_record *record;
    CHARREC *live, *next;

    if (count > index) {
        if (count > index + 1) {
            live = &RoomOpp[index];
            do {
                record = GetOppInfoIdx(room_index, index);
                *record = *GetOppInfoIdx(room_index, index + 1);
                record->index = index;
                if (Room.number == room) {
                    next = &live[1];
                    *live = *next;
                    live->slot = index;
                }
                ++live;
            } while (++index + 1 < count);
        }
        --list->opponent_count;
    }
}

void far pascal add_opponent_to_room(int room, opponent_record *record)
{
    ROOMOBJECTS *list = &Level.room_objects[room - 1];
    signed char index = list->opponent_count;

    if (index < 5) {
        record->index = index;
        ++Level.room_objects[room - 1].opponent_count;
        *GetOppInfoIdx(room, index) = *record;
    }
}

opponent_record *far pascal GetOppInfoIdx(int room, int index)
{
    opponent_record *result;

    if (Level.room_objects[room - 1].opponent_count > index)
        result = (opponent_record near *)Level.room_objects[room - 1].opponent_data[index];
    else
        result = 0;
    return result;
}

#pragma optimize("s", on)
signed char far pascal pick_hidden_actor(signed char direction)
{
    signed char best = -1, index, count;
    int dx, dy, best_dx, best_dy, dy_difference, kx;
    CHARREC *actor;
    opponent_record *record;

    if ((*(volatile unsigned char near *)&Kid.room) != 0) {
        index = 0;
        count = Level.room_objects[(*(volatile unsigned char near *)&Kid.room) - 1].opponent_count;
        for (; index < count; ++index) {
            record = GetOppInfoIdx(Room.number, index);
            actor = &RoomOpp[index];
            dy = abs(actor->y - Kid.y);
            if (actor->alive < 0 && actor->character_kind != 11 &&
                !((actor->character_kind == 7 || actor->character_kind == 8) && (unsigned char)record->saved_y == 2)) {
                if (direction == 1) {
                    dx = abs(Kid.x - actor->x);
                } else if (direction == -1 && actor->x <= Kid.x) {
                    dx = Kid.x - actor->x;
                    goto clamp_distance;
                } else if (direction == 0 && actor->x > Kid.x) {
                    dx = actor->x - Kid.x;
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
                    best_dx = abs(RoomOpp[best].x - Kid.x);
                    best_dy = abs(RoomOpp[best].y - Kid.y);
                    dy_difference = abs(dy - best_dy);
                    if ((dy_difference > 43 && dy < best_dy) ||
                        (dy_difference < 43 && best_dx > dx))
                        best = index;
                }
            }
        }
    }
    return best;
}

#pragma optimize("", on)
int far pascal is_selected_actor(void)
{
    if (pick_hidden_actor(Kid.x >= Char.x ? -1 : 0) == actor_index)
        return 1;
    return 0;
}

void far pascal check_generator_points(unsigned char room)
{
    generator_point *point;

    if (Kid.alive < 0 && (*(unsigned char near *)&Kid.character_kind) != 1) {
        point = find_active_generator((*(volatile unsigned char near *)&Kid.room), Kid.row);
        if (point && should_generate(point, room)) {
            if (point->delay == 0) {
                GenerateOpponent(point, room);
                point->delay = point->reset_delay;
                --point->enabled;
            } else
                --point->delay;
        }
    }
}

void far pascal GenerateOpponent(generator_point *point, unsigned char room)
{
    int room_index = room;
    ROOMOBJECTS *list = &Level.room_objects[room_index - 1];
    signed char index = list->opponent_count;
    signed char row;
    opponent_record *record;

    if (index < 5) {
        list->opponent_count = index + 1;
        record = GetOppInfoIdx(room_index, index);
        record->tile = FirstInRow(point->column) + point->row;
        if (point->row <= 5) {
            record->x = generator_x_left[point->row];
            record->direction = 0;
        } else {
            record->x = generator_x_right[point->row];
            record->direction = -1;
        }
        record->flags = Level.generators[room_index - 1].spawn_flags;
        record->palette_kind = bank_opponents[0];
        record->index = index;
        record->combat_mode = 0;
        record->behavior = point->behavior;
        ((unsigned char *)&record->entry_y)[0] = point->alternate_column;
        record->max_life = record->life = point->flags & 15;
        actor_index = index;
        (*(volatile signed char near *)&Char.direction) = record->direction;
        Char.x = record->x;
        (*(volatile unsigned char near *)&Char.character_kind) = appearance_kind[(signed char)Level.preferred_opponent];
        Char.column = point->row;
        Char.row = point->column;
        Char.behavior = point->behavior;
        actor_room = room;
        (*(unsigned char near *)&Char.life_delta) = (*(unsigned char near *)&Char.life_points) = (*(unsigned char near *)&Char.life_initial) = record->life;
        (*(unsigned char near *)&Char.alive) = -1;
        (*(volatile int near *)&Char.field_34) = 4;
        reset_char_y();
        ClearCharVel();
        Char.sequence_group = 0;
        (*(unsigned char near *)&Char.field_0f) = 1;
        row = (*(volatile signed char near *)&Kid.column);
        if (Kid.direction != (*(volatile signed char near *)&Char.direction) &&
            ((Kid.frame >= 1 && Kid.frame <= 14) ||
             (Kid.frame >= 49 && Kid.frame <= 56) ||
             (Kid.frame >= 34 && Kid.frame <= 44)))
            row -= direction_row_adjustment[Kid.direction];
        if (point->row + 3 >= row && point->row - 3 <= row &&
            (point->flags & 15) >= 6) {
            (*(unsigned char near *)&Char.stance) = 1;
            JumpSeq(90);
            (*(unsigned char near *)&Char.alert_delay) = 3;
        } else {
            (*(unsigned char near *)&Char.stance) = 0;
            JumpSeq(84);
            (*(unsigned char near *)&Char.alert_delay) = 0;
        }
        AnimChar();
        if (Kid.opponent_index == -1)
            Kid.opponent_index = actor_index;
        save_actor_slot();
    }
}

generator_point *far pascal get_generator(unsigned char room, signed char index);
int far pascal is_active_generator(generator_point *point, signed char column);

generator_point *far pascal find_active_generator(unsigned char room, signed char column)
{
    signed char index = 0;
    generator_point *point = 0;
    signed char count = Level.generators[room - 1].count;

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
        abs(point->row - (*(volatile signed char near *)&Kid.column)) > 2 && point->enabled != 0)
        result = 1;
    else
        result = 0;
    return result;
}

extern volatile signed char LevelIdx;
extern int generator_level;
extern int FrameNum;


signed char far pascal find_wall_column(unsigned char room, signed char row,
                                    signed char column, signed char direction);

int far pascal should_generate(generator_point *point, unsigned char room)
{
    signed char count = Level.room_objects[room - 1].opponent_count;
    signed char index, room_row;
    unsigned char between;
    int allowed;

    if (count < 5 && LevelIdx == generator_level && FrameNum % 3 != 0) {
        allowed = 1;
        index = 0;
        between = 0;
        room_row = find_wall_column(room, point->row, point->column,
                                 point->row >= 5 ? -1 : 1);
        if (count > 0) {
            do {
                if (!allowed)
                    break;
                if (RoomOpp[index].alive < 0 &&
                    ((point->row < (*(volatile signed char near *)&Kid.column) && RoomOpp[index].x < Kid.x) ||
                     (point->row > (*(volatile signed char near *)&Kid.column) && RoomOpp[index].x > Kid.x)) &&
                    ((point->row < room_row && (*(volatile signed char near *)&Kid.column) < room_row) ||
                     (point->row > room_row && (*(volatile signed char near *)&Kid.column) > room_row))) {
                    ++between;
                    if (point->limit <= between ||
                        (point->row < (*(volatile signed char near *)&Kid.column) && RoomOpp[index].column <= point->row) ||
                        (point->row > (*(volatile signed char near *)&Kid.column) && RoomOpp[index].column >= point->row))
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

    if (room && Level.generators[room - 1].count > index)
        point = (generator_point near *)Level.generators[room - 1].points[index];
    else
        point = 0;
    return point;
}


int far pascal generator_has_tumble(unsigned char room)
{
    int found = 0;
    signed char count = Level.generators[(*(unsigned char near *)&Char.room) - 1].count;
    signed char index = 0;
    generator_point *point;

    for (; index < count && !found; ++index) {
        point = get_generator(room, index);
        if (point && (point->flags & 128)) found = 1;
        else found = 0;
    }
    return found;
}


extern volatile unsigned char bank_opponents[2];
void far pascal load_opponent_bank(int resource, int start, unsigned char size,
                                   unsigned char kind);

void far _fastcall reserve_opponent_bank(unsigned char kind)
{
    int slot;

    if (Level.preferred_opponent != 0 && Level.preferred_opponent != 5 && Level.preferred_opponent != 6) {
        (*(volatile int near *)&Char.field_34) = Level.preferred_opponent == 2 ? 8 : 4;
        return;
    }
    (*(volatile int near *)&Char.field_34) = 0;
    if (kind == 0)
        kind = 1;

    for (slot = 0; slot < 2 && (*(volatile int near *)&Char.field_34) == 0; ++slot) {
        if (bank_opponents[slot] == kind)
            (*(volatile int near *)&Char.field_34) = 4 << slot;
    }
    if ((*(volatile int near *)&Char.field_34) == 0) {
        for (slot = 0; slot < 2 && (*(volatile int near *)&Char.field_34) == 0; ++slot) {
            if (bank_opponents[slot] == 0) {
                bank_opponents[slot] = kind;
                (*(volatile int near *)&Char.field_34) = 4 << slot;
            }
        }
    }
    if ((*(volatile int near *)&Char.field_34) == 0)
        (*(volatile int near *)&Char.field_34) = 4;
}

void far pascal load_opponent_banks(void)
{
    int slot;
    int start;

    if (Level.preferred_opponent != 0 && Level.preferred_opponent != 5 && Level.preferred_opponent != 6) {
        if (Level.preferred_opponent == 2) {
            load_opponent_bank(750, 48, 16, 0); return;
        }
    } else {
        for (slot = 0, start = 32; start < 64; ++slot, start += 16) {
            if (bank_opponents[slot] != 0) load_opponent_bank(750, start, 16, bank_opponents[slot] - 1);
        }
    }
}


extern unsigned char CutRoom;
extern int opponent_review_delay;



void far pascal load_actor_slot(signed char index);
int far pascal query_sound(int object_id);
void far pascal add_opponent_death_song(void);
void far pascal refresh_actor_life(signed char index, int change, int life);
void far pascal copy_kid_to_char(void);
void far pascal setup_char(void);
void far pascal get_char_edges(void);
int far pascal choose_cut_side(void);
void far pascal transfer_room_actors(void);
int far pascal can_actor_follow(void);
int far pascal actor_follows_to_adjacent_room(void);
int far pascal AddCharX(int x);
void far pascal trigger_character_water(void);
void far _fastcall AddKidScream(unsigned char room);
int far pascal decrement_life_points(unsigned char action);
void far pascal set_actor_fall(unsigned char state);
void far pascal ChangeMeters(void);
signed char far pascal GetRow(int y);

void far pascal cut_room_opponents(void)
{
    signed char index = 0;
    signed char count = Level.room_objects[Room.number - 1].opponent_count;

    for (; index < count; ++index) {
        load_actor_slot(index);
        if ((*(volatile signed char near *)&Char.direction) == 86 || Char.y < 384)
            continue;
        if (Level.environment_kind == 5 && cut_blocked_index == actor_index)
            continue;

        if ((*(unsigned char near *)&Char.action) != 4 && (*(unsigned char near *)&Char.action) != 3 &&
            ((unsigned)(*(volatile int near *)&Char.frame) < 202u || (unsigned)(*(volatile int near *)&Char.frame) > 212u)) {
            if (!update_opponent_record())
                save_actor_slot();
        } else if (!Room.below) {
            update_opponent_record();
            if ((*(volatile unsigned char near *)&Char.character_kind) == 2 && !query_sound(10025))
                AddSound(25);
        }

        add_opponent_death_song();
        refresh_actor_life(actor_index, 0, (signed char)(*(unsigned char near *)&Char.life_points));
        if (Kid.opponent_index == actor_index)
            Kid.opponent_index = -1;
    }
}

void far pascal check_cut(void)
{
    if (opponent_review_delay) {
        --opponent_review_delay;
        return;
    }

    copy_kid_to_char();
    setup_char();
    get_char_edges();
    previous_actor = choose_cut_side();

    if (previous_actor != -1 && actor_room != 0) {
        Kid = Char;
        CutRoom = actor_room;
        transfer_room_actors();
        return;
    }

    if (actor_room == 0 && (signed char)(*(unsigned char near *)&Char.alive) < 0) {
        if (Level.environment_kind == 5 && previous_actor == 0) {
            Char.x = AddCharX(320);
            trigger_character_water();
        } else {
            AddKidScream(Room.number);
            if (Level.environment_kind != 5) {
                decrement_life_points(100);
                (*(volatile int near *)&Char.frame) = 185;
                set_actor_fall(3);
                Kid = Char;
                ChangeMeters();
                copy_kid_to_char();
            }
        }
    }
    Kid = Char;
}

void far pascal transfer_room_actors(void)
{
    signed char index = 0;
    signed char count = Level.room_objects[Room.number - 1].opponent_count;
    int stay, removed;

    for (; index < count; ++index) {
        load_actor_slot(index);
        if ((*(volatile signed char near *)&Char.direction) == 86)
            continue;

        if ((((unsigned)(*(volatile int near *)&Char.frame) - 202u <= 10u &&
              Char.behavior == 1) ||
             ((unsigned)(*(volatile int near *)&Char.frame) >= 186u && (unsigned)(*(volatile int near *)&Char.frame) <= 201u &&
              Char.behavior != 0)) && ((*(volatile unsigned char near *)&Char.character_kind) == 2 || (*(volatile unsigned char near *)&Char.character_kind) == 4))
            goto follow;
        if (actor_follows_to_adjacent_room())
            goto follow;
        if (Level.level_number == 5 && Char.row == 1 &&
            (actor_room == 10 || actor_room == 7 || actor_room == 12))
            goto follow;
        if ((*(volatile unsigned char near *)&Char.character_kind) == 0)
            goto follow;
        if ((*(volatile unsigned char near *)&Char.character_kind) == 6 && (actor_room == 7 || actor_room == 8) &&
            Level.environment_kind == 6 && ((unsigned)CutRoom - 7u <= 1u) &&
            Level.environment_kind == 6) {
follow:
            stay = 0;
process:
            if (stay) {
                removed = update_opponent_record();
                goto next_actor;
            }
            goto transfer;
        }
        if ((signed char)(*(unsigned char near *)&Char.alive) < 0 && can_actor_follow()) {
            if (previous_actor == 0) {
                if (Char.x < 208) goto follow;
            } else if (previous_actor == 1) {
                if (Char.x >= 371) goto follow;
            } else if (previous_actor == 2) {
                if (Char.row < 0) goto follow;
            } else if (Char.row >= 3)
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

    if (Level.room_objects[(*(volatile unsigned char near *)&Kid.room) - 1].opponent_count < 5)
        allowed = 1;
    else
        allowed = 0;
    if (allowed) {
        if ((*(unsigned char near *)&Char.stance) == 1 || (*(volatile unsigned char near *)&Char.character_kind) == 7 || (*(volatile unsigned char near *)&Char.character_kind) == 8)
            allowed = 1;
        else
            allowed = 0;
        if (allowed) {
            allowed = ((*(volatile unsigned char near *)&Char.character_kind) != 10);
            if (allowed) return allowed;
        }
    }
    return allowed;
}

int far pascal actor_follows_to_adjacent_room(void)
{
    CHARREC saved;
    unsigned char destination;

    saved = Char;
    destination = register_actor_room();
    Char = saved;

    if ((*(volatile unsigned char near *)&Kid.room) == destination)
        return 1;
    return 0;
}

unsigned char far pascal register_actor_room(void)
{
    unsigned char room = actor_room;
    unsigned char previous;

    if (Char.column < 0 || Char.column >= 10 || Char.row < 0 || Char.row >= 3) {
        do {
        previous = room;
        if (Char.row < 0) {
            room = Level.room_links[room - 1].above;
            if (room) {
                Char.y += 192;
                Char.row += 3;
            } else {
                room = previous;
                previous = 0;
            }
        } else if (Char.row >= 3) {
            room = Level.room_links[room - 1].below;
            if (room) {
                Char.y -= 192;
                Char.row -= 3;
            } else {
                room = previous;
                previous = 0;
            }
        }

        if (Char.column < 0) {
            room = Level.room_links[room - 1].left;
            if (room) {
                Char.x += 320;
                Char.column += 10;
            }
        } else if (Char.column >= 10) {
            room = Level.room_links[room - 1].right;
            if (room) {
                Char.x -= 320;
                Char.column -= 10;
            }
        } else if (!previous)
            room = 0;

        } while (room && (Char.column < 0 || Char.column >= 10 || Char.row < 0 || Char.row >= 3));
    }
    return room;
}

int far pascal cut_actor(int direction)
{
    actor_room = ((unsigned char *)&Level.room_links[actor_room - 1])[direction];
    if (direction == 0) {
        Char.x += 320;
        Char.column = get_char_col(Char.x);
        direction = 1;
    } else if (direction == 1) {
        Char.x -= 320;
        Char.column = get_char_col(Char.x);
        direction = 0;
    } else if (direction == 2) {
        Char.y += 189;
        Char.row = GetRow(Char.y);
        direction = 3;
    } else {
        Char.y -= 189;
        Char.row = GetRow(Char.y);
        direction = 2;
    }
    return direction;
}


extern int lower_exit_margin, horizontal_exit_margin;
extern int left_edge_x, right_edge_x;
extern int opponent_cooldown;

extern signed char script_motion;

void far pascal probe_room_edge(unsigned char room, signed char column, signed char row);
void far pascal leave_special_room(void);
void far pascal on_left_exit(void);
void far pascal run_actor_kind_four(void);
void far pascal run_actor_kind_six(void);
void far pascal run_actor_kind_seven(void);
void far pascal run_actor_kind_eleven(void);
void far pascal altar_shadow_progress(void);
void far pascal chase_opponent(void);
void far pascal alert_opponent(void);
void far pascal guard_en_garde(void);
void far pascal reset_actor_inputs(void);
void far pascal run_shadow_script(void);
void far pascal run_guard_script(void);

int far pascal choose_cut_side(void)
{
    volatile int y = Char.y;
    volatile int mode = Char.action;
    unsigned int action = (*(volatile int near *)&Char.frame);
    int none = -1;
    int side;

    if (LevelIdx != generator_level || (*(volatile int near *)&Char.sequence) == 70 ||
        ((*(volatile unsigned char near *)&Char.character_kind) == 1 && action == 185))
        goto leave_unchanged;
    if (mode == 5 || mode == 4 || mode == 3)
        goto check_lower_exit;
    if (y < 10 && y >= -16) {
        if (Level.level_number == 5) {
            side = none;
            if (actor_room == 3)
                goto check_side;
        }
        side = 2;
        goto check_side;
    }
check_lower_exit:
    if (y >= 231 &&
        (Level.room_links[actor_room - 1].below || y >= lower_exit_margin + 231) &&
        (Level.environment_kind != 5 || actor_index != cut_blocked_index)) {
        side = 3;
        goto check_side;
    }
    if ((action >= 135 && action < 150) ||
        (action >= 110 && action <= 119) ||
        (action >= 150 && action < 163) ||
        (action >= 166 && action < 169) || mode == 7)
        goto keep_actor;

    if ((*(volatile signed char near *)&Char.direction) == -1) {
        if (left_edge_x > 125 ||
            (!Level.room_links[actor_room - 1].left && left_edge_x > 125 - horizontal_exit_margin))
            goto check_right_of_actor;
        if (Level.level_number != 9 || actor_room != 8 || Char.row != 0 || action != 40)
            goto leave_left;
        ++generator_level;
leave_unchanged:
        side = none;
        goto check_side;
check_right_of_actor:
        side = none;
        if (left_edge_x < 450)
            goto check_side;
        if (!Level.room_links[actor_room - 1].right && left_edge_x < horizontal_exit_margin + 449)
            goto check_side;
        if (Level.environment_kind == 1 && actor_room == 3)
            goto check_side;
leave_right:
        side = 1;
        goto check_side;
    }

    side = none;
    probe_room_edge(actor_room, 9, Char.row);
    if (right_edge_x >= 454 &&
        (Level.room_links[actor_room - 1].right || right_edge_x >= horizontal_exit_margin + 454) &&
        (Level.environment_kind != 1 || actor_room != 3))
        goto leave_right;
    if (right_edge_x > 129)
        goto check_side;
    if (Level.room_links[actor_room - 1].left == 0) {
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
        if (actor_room == 1 && Level.level_number == 2)
            side = -1;
        break;
      case 1:
        if (Level.environment_kind == 5 && actor_room == 15) {
            side = -1;
            if ((*(volatile int near *)&Char.sequence) != 71)
                leave_special_room();
        } else if (actor_room == 3 && Level.environment_kind == 6)
            side = -1;
        break;
      case 3:
        if ((actor_room == 11 && Level.environment_kind == 5) ||
            (actor_room == 3 && Level.environment_kind == 6) ||
            (actor_room == 5 && Level.environment_kind == 6) ||
            (Level.level_number == 5 && actor_room == 10) ||
            (actor_room == 4 && Level.level_number == 13) ||
            (Level.level_number == 13 && actor_room == 13) ||
            (((actor_room == 16 && Level.level_number == 9) ||
              (actor_room == 27 && Level.level_number == 6)) &&
             ((*(volatile int near *)&Char.sequence) == 68 || (*(volatile int near *)&Char.sequence) == 25 || (*(volatile int near *)&Char.sequence) == 10))) {
            side = -1;
            if ((Level.level_number == 5 && actor_room == 10) ||
                (actor_room == 4 && Level.level_number == 13) ||
                (Level.level_number == 13 && actor_room == 13)) {
                decrement_life_points(100);
                actor_room = 0;
            }
        }
        break;
    }

    if (side != -1)
        cut_actor(side);
    if (side == 0 && Room.number == 27 && Level.level_number == 6)
        on_left_exit();
    return side;
}

void far pascal update_automatic_actor(void)
{
    signed char kind;

    reset_actor_inputs();
    kind = (*(volatile unsigned char near *)&Char.character_kind);
    if (opponent_lock)
        --opponent_lock;
    if (opponent_cooldown)
        --opponent_cooldown;
    if (opponent_guard_timeout)
        --opponent_guard_timeout;

    switch (kind) {
      default:
        kind = (*(volatile unsigned char near *)&Char.character_kind);
        if (kind == 0) run_actor_kind_eleven();
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
    if (actor_room == 4 && Level.level_number == 13)
        altar_shadow_progress();
}

void far pascal run_guard_script(void)
{
    if ((signed char)(*(unsigned char near *)&Char.alive) < 0 && (*(unsigned char near *)&Char.life_points) != 0 &&
        ((signed char)(*(unsigned char near *)&Char.life_points) + (signed char)(*(unsigned char near *)&Char.life_delta)) != 0) {
        if ((*(unsigned char near *)&Char.stance) == 0) {
            if ((unsigned)(*(volatile int near *)&Char.frame) >= 186u && (unsigned)(*(volatile int near *)&Char.frame) <= 212u)
                chase_opponent();
            else
                alert_opponent();

            if (running_guard_state != 0 &&
                Level.room_objects[actor_room - 1].opponent_count - actor_index == 1) {
                running_guard_state = 0;
                return;
            }
        } else if (Kid.sequence_group != 13) {
            guard_en_garde();
            return;
        } else if (Level.actor_phase != 0 && (*(volatile int near *)&Char.sequence) != 110) {
            JumpSeq(110);
        }
    }
}


typedef struct {
    unsigned char head[6];
    unsigned char flags;
} hit_action_record;
extern hit_action_record far hit_actions[];


extern signed char hit_actor_motion;


extern unsigned char actor_stage;
extern signed char actor_bump_column;
extern unsigned char actor_counterface;
static int opponent_recovery_time[] = {
    20, 20, 20, 20, 10, 10, 10, 10, 0, 10, 0
};
extern CELLREC Cell;

extern int ColEdgeX[20];
extern unsigned int hit_actor_phase;

void far pascal begin_actor_death(void);
void far pascal stab_actor(void);
void far pascal test_actor_strike(void);
int far pascal is_striking_position(signed char kind, unsigned int action);
int far pascal IsTheStrikePos(char kind, int value);
void far _fastcall get_char_strike_ranges(int *minimum, int *maximum);

void far pascal load_shadow_with_opponent(signed char index);
void far pascal save_shadow_with_opponent(void);
void far pascal load_kid_with_opponent(signed char index);
void far pascal save_kid_with_opponent(void);
void far pascal play_fall_script(unsigned char style);
signed char far pascal get_behind(void);
signed char far pascal adjacent_stage_kind(unsigned char direction);
int far pascal stage_blocks_impact(signed char kind);
void far pascal flash_death_effect(void);
void far pascal actor_end_fall(void);
int far pascal calculate_horizontal_clearance(void);
int far pascal IS_SIX_KIND(signed char kind);
int far pascal stage_interrupts_fall(signed char kind);
void far pascal update_actor_tile_column(void);
void far pascal finish_fall_animation(void);
void far pascal bones_fall(void);
unsigned char far pascal GetCellCol(void);
void far pascal dismiss_other_jaffars(void);
int far pascal check_opponent_tumble(void);
int far pascal get_opp_dist(void);
int far pascal block_flame_sword(void);
int far pascal head_blocked(void);
int far pascal IsImpassable(signed char tile);


void far pascal check_stab(void)
{
    signed char index = 0;
    signed char count = Level.room_objects[Room.number - 1].opponent_count;

    for (; index < count; ++index) {
        load_actor_slot(index); if ((*(unsigned char near *)&Char.action) == 99) { load_shadow_with_opponent(index);
            if ((*(unsigned char near *)&Opp.action) == 99)
                (*(unsigned char near *)&Opp.action) = 1;
            stab_actor();
            opponent_guard_timeout = opponent_recovery_time[
                ((opponent_record near *)Level.room_objects[actor_room - 1].opponent_data)[actor_index].flags];
            save_shadow_with_opponent();
        }
    }

    if ((*(unsigned char near *)&Kid.action) == 99) {
        for (index = 0; index < count; ++index) {
            load_actor_slot(index);
            if ((*(volatile unsigned char near *)&Char.character_kind) == 7 || (*(volatile unsigned char near *)&Char.character_kind) == 8) {
                if ((*(volatile int near *)&Char.frame) == 167 || (*(volatile int near *)&Char.frame) == 181)
                    break;
            } else if (IsTheStrikePos((*(volatile unsigned char near *)&Char.character_kind), (*(volatile int near *)&Char.frame)))
                break;
        }
        if (index < count) {
            load_kid_with_opponent(index);
            stab_actor();
            save_kid_with_opponent();
        }
    }
}

void far pascal stab_actor(void)
{
    signed char damage;
    int next_action;
    int sound, default_action;

    if ((signed char)(*(unsigned char near *)&Char.alive) >= 0)
        return;
    if ((*(volatile unsigned char near *)&Char.character_kind) == 0 && (*(volatile int near *)&Char.sequence) == 5 && (Level.environment_kind == 2 || Level.environment_kind == 6))
        play_fall_script(Level.environment_kind);

    if ((*(unsigned char near *)&Char.stance) == 255 || (*(volatile int near *)&Char.sequence) == 10 || (*(volatile int near *)&Char.sequence) == 16 ||
        (*(volatile int near *)&Char.sequence) == 28 || (*(volatile int near *)&Char.sequence) == 14) {
        decrement_life_points(100);
        begin_actor_death();
    } else {
        if ((*(volatile unsigned char near *)&Char.character_kind) == 7 || (*(volatile unsigned char near *)&Char.character_kind) == 8) {
            next_action = 153;
            damage = actor_stage == 2 ? 1 : 2;
            if (damage < (signed char)(*(unsigned char near *)&Char.life_points))
                flash_death_effect();
        } else if ((*(volatile unsigned char near *)&Char.character_kind) == 11 || (*(unsigned char near *)&Opp.character_kind) == 11) {
            damage = 100;
            next_action = default_action;
        } else {
            damage = ((*(volatile unsigned char near *)&Char.character_kind) == 6 || (*(unsigned char near *)&Opp.character_kind) == 6) ? 100 : 1;
            next_action = Opp.direction != (*(volatile signed char near *)&Char.direction) ? 74 : 94;
            if ((*(unsigned char near *)&Opp.character_kind) == 7 || (*(unsigned char near *)&Opp.character_kind) == 8) {
                if ((*(unsigned char near *)&Char.stance) != 1 &&
                    !(hit_actions[Kid.frame].flags & 64))
                    next_action = 45;
                else if ((unsigned)(*(volatile int near *)&Char.frame) >= 246u && (unsigned)(*(volatile int near *)&Char.frame) <= 261u) {
                    if (stage_blocks_impact(get_behind()))
                        Char.x = AddCharX(16);
                    else if (stage_blocks_impact(adjacent_stage_kind(1)))
                        Char.x = AddCharX(-16);
                }
            }
            if ((*(volatile unsigned char near *)&Char.character_kind) == 0 && next_action != 45)
                (*(unsigned char near *)&Char.stance) = 1;
        }

        if (!decrement_life_points(damage) && (*(volatile unsigned char near *)&Char.character_kind) != 11) {
            JumpSeq(next_action);
            goto stop_falling;
        }
    }

    actor_end_fall();
    next_action = calculate_horizontal_clearance();
    if ((IS_SIX_KIND(get_under_foot()) ||
         (IS_SIX_KIND(get_behind()) && next_action >= 4)) &&
        (*(volatile unsigned char near *)&Char.character_kind) != 7 && (*(volatile unsigned char near *)&Char.character_kind) != 8 && (*(volatile unsigned char near *)&Char.character_kind) != 11 && (*(volatile unsigned char near *)&Char.character_kind) != 6) {
        next_action -= 8;
        if (actor_bump_column != Char.column)
            next_action -= 32;
        Char.x = AddCharX(next_action);
        update_actor_tile_column();
        ++Char.row;
        JumpSeq(81);
    } else {
        begin_actor_death();
stop_falling:
        reset_char_y();
        Char.vertical_step = 0;
    }

    if ((*(unsigned char near *)&Opp.character_kind) != 7 && (*(unsigned char near *)&Opp.character_kind) != 8 && (*(unsigned char near *)&Opp.character_kind) != 11) {
        if ((*(volatile unsigned char near *)&Char.character_kind) == 0)
            sound = (*(unsigned char near *)&Opp.character_kind) == 10 ? 194 : 14;
        else if ((*(volatile unsigned char near *)&Char.character_kind) == 4)
            sound = 72;
        else if ((*(volatile unsigned char near *)&Char.character_kind) == 7 || (*(volatile unsigned char near *)&Char.character_kind) == 8)
            sound = 86;
        else if ((*(volatile unsigned char near *)&Char.character_kind) == 11)
            sound = 70;
        else
            sound = 21;
        AddSound(sound);
    }
    AnimChar();
    if ((*(volatile int near *)&Char.sequence) == 85) {
        update_actor_tile_column();
        damage = get_under_foot();
        if (IS_SIX_KIND(damage) || stage_interrupts_fall(damage)) {
            Char.x = AddCharX(-24);
            update_actor_tile_column();
        }
        finish_fall_animation();
    }
}

void far pascal begin_actor_death(void)
{
    int next_action;
    signed char index;

    (*(unsigned char near *)&Char.alert_delay) = 0;
    (*(unsigned char near *)&Char.field_0f) = 1;
    next_action = -1;
    if ((*(volatile unsigned char near *)&Char.character_kind) == 4) {
        bones_fall();
    } else if ((*(volatile unsigned char near *)&Char.character_kind) == 7 || (*(volatile unsigned char near *)&Char.character_kind) == 8) {
        (*(volatile signed char near *)&Char.direction) = ~Kid.direction;
        (*(unsigned char near *)&Char.life_points) = 0;
        next_action = 154;
    } else if ((*(volatile unsigned char near *)&Char.character_kind) == 11) {
        next_action = 174;
    } else if ((*(volatile unsigned char near *)&Char.character_kind) == 6) {
        next_action = 196;
    } else {
        index = get_behind();
        if ((*(volatile unsigned char near *)&Char.character_kind) != 0 && (*(volatile signed char near *)&Char.direction) == -1 &&
            (index == 4 || (signed char)get_under_foot() == 4)) {
            index = GetCellCol();
            Char.x = ColEdgeX[index - (Cell.kind != 4) + 5] + 14;
            Char.x = AddCharX(20);
        }

        if ((*(volatile unsigned char near *)&Char.character_kind) == 0 && (unsigned)(*(volatile int near *)&Char.frame) >= 246u && (unsigned)(*(volatile int near *)&Char.frame) <= 261u) {
            next_action = 98;
            if ((*(unsigned char near *)&Opp.character_kind) == 11)
                AddSound(69);
            else if ((*(unsigned char near *)&Opp.character_kind) == 7 || (*(unsigned char near *)&Opp.character_kind) == 8)
                AddSound(89);
            else
                AddSound(14);
        } else if ((*(unsigned char near *)&Opp.character_kind) == 11) {
            next_action = 97;
            AddSound(69);
        } else {
            if ((*(unsigned char near *)&Opp.character_kind) == 6)
                dismiss_other_jaffars();
            next_action = 85;
            if ((*(volatile unsigned char near *)&Char.character_kind) != 0 && check_opponent_tumble()) {
                next_action = 185;
                Char.x = AddCharX(-8);
            }
        }
        if ((*(volatile unsigned char near *)&Char.character_kind) == 0) {
            switch ((signed char)(*(unsigned char near *)&Opp.character_kind)) {
              case 2:
                index = 10 + ((*(unsigned char *)&Level.preferred_opponent == 0) ? 4 : 0);
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
    if ((*(unsigned char near *)&Opp.character_kind) == 0 || (*(unsigned char near *)&Opp.character_kind) == 1) {
        save_shadow_with_opponent();
        Opp.opponent_index = pick_hidden_actor(Opp.direction);
    }
    if (next_action != -1)
        JumpSeq(next_action);
}

void far pascal dispatch_guard_reaction(void)
{
    signed char index, count;

    if (Kid.frame != 0 && (Kid.frame < 219 || Kid.frame >= 229)) {
        index = 0;
        count = Level.room_objects[Room.number - 1].opponent_count;
        for (; index < count; ++index) {
            load_shadow_with_opponent(index);
            test_actor_strike();
            save_shadow_with_opponent();
            load_kid_with_opponent(index);
            test_actor_strike();
            save_kid_with_opponent();
        }
    }
}

void far pascal test_actor_strike(void)
{
    int distance, minimum, maximum;

    if ((*(unsigned char near *)&Char.stance) != 1 || Char.row != Opp.row ||
        (abs(Char.y - Opp.y) >= 7 &&
         (*(volatile unsigned char near *)&Char.character_kind) != 7 && (*(volatile unsigned char near *)&Char.character_kind) != 8 && (*(unsigned char near *)&Opp.character_kind) != 7 && (*(unsigned char near *)&Opp.character_kind) != 8) ||
        !is_striking_position((*(volatile unsigned char near *)&Char.character_kind), (*(volatile int near *)&Char.frame)) || Opp.alive >= 0 ||
        (*(unsigned char near *)&Opp.character_kind) == 10 || (*(unsigned char near *)&Opp.character_kind) == 1 || (*(unsigned char near *)&Opp.action) == 8 ||
        (*(int near *)&Opp.sequence) == 110)
        return;

    distance = get_opp_dist();
    if ((*(volatile unsigned char near *)&Char.character_kind) == 7 || (*(volatile unsigned char near *)&Char.character_kind) == 8) {
        minimum = 0;
        maximum = 28;
    } else {
        minimum = 0;
        maximum = 44;
    }

    if ((*(volatile signed char near *)&Char.direction) != Opp.direction && distance >= 0 && distance <= maximum &&
        (*(unsigned char near *)&Opp.character_kind) != 7 && (*(unsigned char near *)&Opp.character_kind) != 8 && (*(unsigned char near *)&Opp.character_kind) != 11 &&
        (*(volatile unsigned char near *)&Char.character_kind) != 11 && (Opp.frame == 161 || Opp.frame == 150)) {
        Opp.frame = 161;
        if ((*(volatile unsigned char near *)&Char.character_kind) == 0)
            (*(unsigned char near *)&Char.opponent_index) = (*(unsigned char near *)&Opp.slot);
        else {
            Opp.opponent_index = actor_index;
            opponent_lock = 4;
        }

        if ((*(volatile unsigned char near *)&Char.character_kind) == 7 || (*(volatile unsigned char near *)&Char.character_kind) == 8)
            distance = head_blocked();
        else if ((*(volatile unsigned char near *)&Char.character_kind) == 10)
            distance = block_flame_sword();
        else
            distance = 69;
        if (distance != -1)
            JumpSeq(distance);
        AnimChar();
    } else if (IsTheStrikePos((*(volatile unsigned char near *)&Char.character_kind), (*(volatile int near *)&Char.frame))) {
        get_char_strike_ranges(&minimum, &maximum);
        if (distance >= minimum && distance <= maximum) {
            (*(unsigned char near *)&Opp.action) = 99;
            if ((*(volatile unsigned char near *)&Char.character_kind) == 0)
                (*(unsigned char near *)&Char.opponent_index) = (*(unsigned char near *)&Opp.slot);
        }
    } else if (((*(volatile unsigned char near *)&Char.character_kind) == 7 || (*(volatile unsigned char near *)&Char.character_kind) == 8) && Char.sequence_group == 1 && opponent_lock == 0)
        (*(unsigned char near *)&Opp.action) = 99;

    if (IsTheStrikePos((*(volatile unsigned char near *)&Char.character_kind), (*(volatile int near *)&Char.frame)) &&
        (Opp.frame != 161 || (*(unsigned char near *)&Opp.character_kind) == 7 || (*(unsigned char near *)&Opp.character_kind) == 8) &&
        (*(unsigned char near *)&Opp.action) != 99) {
        if ((*(volatile unsigned char near *)&Char.character_kind) == 10)
            distance = 193;
        else
            distance = 13;
        AddSound(distance);
    }
}

void far _fastcall get_char_strike_ranges(int *minimum, int *maximum)
{
    *minimum = 0;
    *maximum = -1;

    if ((*(volatile unsigned char near *)&Char.character_kind) != 0 && (*(volatile unsigned char near *)&Char.character_kind) != 1) {
        if ((*(volatile unsigned char near *)&Char.character_kind) == 10) {
            *minimum = -10;
            *maximum = 38;
        } else if ((*(volatile unsigned char near *)&Char.character_kind) == 11) {
            if (63 * Opp.row + 51 <= Opp.y) {
                if (Opp.direction == (*(volatile signed char near *)&Char.direction)) {
                    *minimum = -2;
                    *maximum = 27;
                } else {
                    *minimum = -27;
                    *maximum = -4;
                    return;
                }
            }
        } else if ((*(volatile unsigned char near *)&Char.character_kind) == 6) {
            *minimum = 13;
            *maximum = 53;
            return;
        } else if (Opp.direction != (*(volatile signed char near *)&Char.direction)) {
            *minimum = 19;
            *maximum = 46;
            if ((*(volatile unsigned char near *)&Char.character_kind) == 4) {
                *maximum = 48;
                return;
            }
        } else if ((*(unsigned char near *)&Opp.stance) == 1) {
            *minimum = 34;
            *maximum = 57;
            return;
        } else {
            *minimum = 22;
            *maximum = 42;
            return;
        }
    } else {
        if ((*(unsigned char near *)&Opp.character_kind) == 11) {
            if (Opp.sequence_group != 6 && (*(int near *)&Opp.sequence) != 171 && (*(volatile int near *)&Char.sequence) == 96) {
                if (Opp.direction == (*(volatile signed char near *)&Char.direction)) {
                    *minimum = 31;
                    *maximum = 75;
                } else {
                    *minimum = -7;
                    *maximum = 45;
                }
            }
        } else if ((*(unsigned char near *)&Opp.character_kind) == 7 || (*(unsigned char near *)&Opp.character_kind) == 8) {
            *minimum = 19;
            *maximum = 46;
        } else if ((*(unsigned char near *)&Opp.character_kind) == 6) {
            if (Opp.direction != (*(volatile signed char near *)&Char.direction)) {
                *minimum = 13;
                *maximum = 57;
            } else {
                *minimum = 13;
                *maximum = 53;
            }
        } else {
            *minimum = 19;
            *maximum = 46;
            if (!Opp.alert_delay) {
                *minimum += 14;
                *maximum += 14;
            }
            if ((*(unsigned char near *)&Opp.character_kind) == 4)
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
        result = (*(volatile int near *)&Char.sequence) == 173 && (action == 274 || action == 280);
    } else {
        result = action == 153 || action == 154;
        if (!result && kind == 0)
            result = action == 244 || action == 245;
    }
    return result;
}


int far pascal IsTheStrikePos(char kind, int value)
{
    int result;

    if (kind == 10) {
        if (value == 224)
            result = 1;
        else
            result = 0;
    } else if (kind == 11) {
        if ((*(volatile int near *)&Char.sequence) == 173 && value == 280)
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
    if (allowed && (*(volatile unsigned char near *)&Char.character_kind) != 10) {
        allowed = !IsImpassable(tile);
        if (allowed && (*(volatile unsigned char near *)&Char.character_kind) != 7 && (*(volatile unsigned char near *)&Char.character_kind) != 8)
            allowed = !IS_SIX_KIND(tile);
    }
    return allowed;
}
