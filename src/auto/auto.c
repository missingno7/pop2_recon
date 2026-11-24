#pragma pack(1)
typedef struct {
    unsigned char number;                             /* 00 */
    unsigned char left, right, above, below;           /* 01-04 */
    unsigned char above_left, above_right;             /* 05-06 */
    unsigned char below_left, below_right;             /* 07-08 */
    unsigned char custom;                             /* 09 */
} ROOMREC;

extern ROOMREC Room;

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

#define ROOM_LEFT 0
#define ROOM_RIGHT 1
#define ROOM_ABOVE 2
#define ROOM_BELOW 3
typedef struct {
    unsigned char neighbors[4];
} ROOMLINKS;

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
    struct vertical_positions {
        int saved_y;
        int other_y;
        /* Generators write the low byte of this stored word. */
        union {
            int y;
            signed char alternate_column;
        } entry;
    } position;
} opponent_record;

typedef struct {
    signed char opponent_count;
    opponent_record opponents[5];
} ROOMOBJECTS;

typedef struct {
    signed char room, cell;
} REBIRTHPOINT;

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

typedef struct {
    signed char count;
    unsigned char spawn_flags;
    unsigned char header[2];
    generator_point points[3];
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
    unsigned char room;                /* 0e */
    signed char field_0f;              /* 0f */
    unsigned char stance;              /* 10 */
    signed char alive;                 /* 11 */
    signed char life_points;           /* 12 */
    signed char life_initial;          /* 13 */
    signed char life_delta;            /* 14 */
    int sequence_position;             /* 15 */
    int sequence_id;                   /* 17 */
    int sequence;                      /* 19 */
    struct char_bounds bounds;                    /* 1b: top, left, bottom, right */
    unsigned char alert_delay;           /* 23 */
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
typedef struct {
    signed char character, frame;
    int x;
    unsigned char low;
} WATERSTATE;
extern WATERSTATE InWater;
extern CELLREC Cell;
extern int Pause;
extern CHARREC Opp, Kid, Char;

void far pascal SetOppMaxLifePts(opponent_record *record);

void far pascal reset_overlay_state(void)
{
    Pause = 0;
}

void far pascal set_opponent_life(opponent_record *record)
{
    SetOppMaxLifePts(record);
    if ((Char.character_kind == 4 && Char.sequence == 119) ||
        (Char.character_kind == 10 && Char.sequence == 105)) {
        Char.life_points = 0;
        Char.life_delta = 0;
    } else {
        Char.life_points = Char.life_initial;
        Char.life_delta = Char.life_points;
    }
}

void far pascal SetOppMaxLifePts(opponent_record *record)
{
    if (record->max_life == 0)
        Char.life_initial = 3;
    else
        Char.life_initial = record->max_life;
}

extern unsigned char loaded_opponent;
extern LEVELREC Level;

extern unsigned char OppCharId[];

#define actor_index Char.slot

extern unsigned char OppBank[2];

#define actor_room Char.room

#define actor_bank_mask Char.field_34

extern int Refract, JustBlocked, AlertGuard;

extern int CutDir;

opponent_record *far pascal GetOppInfoIdx(int room, int index);
void far pascal check_side_rooms(void);
void far pascal add_opponent_to_room(int room, opponent_record *record);
void far pascal RemoveOppFromRoom(unsigned char room, int index);
void far _fastcall reserve_opponent_bank(unsigned char kind);
void far pascal set_palette(int resource, int start, unsigned char size,
                                   unsigned char kind);
void far pascal load_opponent_set(signed char appearance);
void far pascal load_changed_opponent_set(signed char appearance);
int far pascal Rnd(int limit);
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

            Char.character_kind = OppCharId[record->appearance];
            load_changed_opponent_set(record->appearance);

            actor_index = index;
            actor_room = Room.number;
            Char.row = record->tile / 10;
            if (record->tile < 0)
                --Char.row;

            if (Char.character_kind == 2 && (record->action == 100 || record->action == 100)) {
                Char.y = record->position.saved_y;
            } else if (Char.character_kind == 6 && record->position.entry.y != 0) {
                Char.y = record->position.entry.y;
                if (record->action == 12) {
                    Char.horizontal_step = 2;
                    Char.vertical_step = 18;
                }
            } else {
                reset_char_y();
            }
            Char.x = record->x;
            Char.column = get_char_col(Char.x);
            Char.direction = record->direction;
            Char.behavior = record->behavior;
            Char.opponent_index = record->behavior_type;
            Char.previous_frame = 0;
            Char.sequence_group = Char.previous_y = Char.previous_x = 0;
            Char.field_3a = Char.alert_delay = 0;

            if (Char.character_kind == 0)
                actor_bank_mask = 2;
            else if (Char.character_kind == 6) {
                actor_bank_mask = 4;
                set_palette(25001u, 32, 16, 7);
                if (Room.number == 6 && Level.environment_kind == 6)
                    Char.direction = Rnd(1) - 1;
            } else if (Level.preferred_opponent != 0 && Level.preferred_opponent != 5 && Level.preferred_opponent != 6)
                actor_bank_mask = 4;
            else
                reserve_opponent_bank(record->palette_kind);

            if (record->saved_sequence == 0) {
                if (Char.character_kind == 4)
                    record = initialize_skeleton(record);
                else if (Char.character_kind == 7 || Char.character_kind == 8 || Char.character_kind == 11)
                    record = head_add_opponent(record);
                else if (Char.character_kind == 10) {
                    if (is_open_tile(get_under_foot())) {
                        record->action = 105;
                        Char.stance = 0;
                    } else {
                        record->action = 106;
                        Char.stance = 1;
                        Char.alert_delay = 3;
                        if (Char.x >= Kid.x)
                            Char.direction = -1;
                        else
                            Char.direction = 0;
                    }
                } else if (Char.character_kind == 6)
                    record->action = 2;
                else {
                    if (record->action != 22)
                        record->action = 77;
                    Char.stance = 0;
                }
                JumpSeq(record->action);
                set_opponent_life(record);
            } else {
                Char.field_0f = 1;
                Char.sequence_id = record->action;
                Char.sequence_position = record->saved_sequence;
                Char.sequence = record->action;
                Char.stance = record->combat_mode;
                Char.life_points = Char.life_delta = record->life;
                if (Char.character_kind != 0)
                    SetOppMaxLifePts(record);
            }

            AnimChar();
            if (is_dead_pos(Char.frame) && Char.character_kind != 0) {
                if (Char.character_kind == 2)
                    check_multiple_dead_bodies();
                Char.alive = 7;
                Char.life_points = Char.field_0f = 0;
                Char.life_delta = -Char.life_initial;
                if (Char.character_kind == 4 || Char.character_kind == 10)
                    set_skeleton_timer(actor_index, 0);
            } else {
                Char.alive = -1;
                Refract = JustBlocked = AlertGuard = 0;
                if (Char.character_kind == 11 && Char.frame == 264)
                    Char.sequence_group = 6;
            }
            if (Char.character_kind == 6 && Char.sequence == 12)
                Char.action = 4;
            else {
                ClearCharVel();
                Char.action = Char.character_kind == 7 || Char.character_kind == 8 ? 0 : 1;
            }
            save_actor_slot();
        } while (count > ++index);
    }

    if (count == 0) {
        if (Level.level_number == 12 && (Room.number == 11 || Room.number == 19) &&
            loaded_opponent != 1)
            load_opponent_set(1);
        else if (Level.generators[Room.number - 1].count != 0 &&
                 loaded_opponent != Level.preferred_opponent &&
                 Level.preferred_opponent != 5 && Level.preferred_opponent != 6)
            load_opponent_set(Level.preferred_opponent);
    }
}

void far pascal check_side_rooms(void)
{
    int index, side, shift;
    unsigned char room;
    signed char count;
    opponent_record *record;
    opponent_record copy;

    for (side = 0; side < 2; ++side) {
        if (side == 0) {
            room = Room.left;
            shift = -320;
        } else {
            room = Room.right;
            shift = 320;
        }
        if (room == 0)
            continue;
        count = Level.room_objects[room - 1].opponent_count;
        for (index = 0; index < count; ++index) {
            record = GetOppInfoIdx(room, index);
            if ((side == 0 && record->x > 367) || (side == 1 && record->x < 208) ||
                ((room == 7 || room == 8) && Level.environment_kind == 6 && record->appearance == 3)) {
                copy = *record;
                copy.x += shift;
                copy.index = Level.room_objects[Room.number - 1].opponent_count;
                add_opponent_to_room(Room.number, &copy);
                RemoveOppFromRoom(room, index);
                --index;
                --count;
            }
        }
    }
}

extern signed char opponent_outfit_by_character[];

extern int actor_y_offset[];
extern int ColEdgeX[20];
extern signed char Minus1[2];

extern CHARREC RoomOpp[5];

int abs(int value);
#pragma intrinsic(abs)

unsigned char far pascal register_actor_room(void);
void far pascal jam_plate(void);
signed char far pascal index_cell(signed char, signed char);
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
    opponent_record copy;

    if (Char.direction == 86 || Char.sequence_group == 5 || Char.sequence_group == 4 || Char.sequence == 196) {
        RemoveOppFromRoom(actor_room, actor_index);
        removed = 1;
    } else {
        record = GetOppInfoIdx(actor_room, actor_index);
        if (record != 0) {
            record->tile = index_cell(Char.column, Char.row);
            if (record->tile != 30) {
                record->x = Char.x;
                record->direction = Char.direction;
                if (Char.alive < 0)
                    record->saved_sequence = removed;
                else {
                    record->saved_sequence = Char.sequence_position;
                    record->action = Char.sequence_id;
                }
                record->combat_mode = Char.stance;
                record->life = Char.life_points;
                record->appearance = opponent_outfit_by_character[Char.character_kind];
                set_special_opponent(record);
                room = get_under_foot();
                if (room == 5 || room == 6)
                    jam_plate();
            } else {
                room = register_actor_room();
                if (room != 0 && Char.character_kind != 12) {
                    prior = GetOppInfoIdx(actor_room, actor_index);
                    copy.tile = FirstInRow(Char.row);
                    copy.x = Char.x;
                    copy.direction = Char.direction;
                    copy.flags = prior->flags;
                    copy.action = Char.sequence_id;
                    copy.saved_sequence = Char.sequence_position;
                    copy.palette_kind = prior->palette_kind;
                    copy.combat_mode = Char.stance;
                    copy.life = Char.life_points;
                    copy.behavior = Char.behavior;
                    copy.behavior_type = Char.opponent_index;
                    copy.appearance = prior->appearance;
                    copy.max_life = prior->max_life;
                    copy.position = prior->position;
                    add_opponent_to_room(room, &copy);
                } else if (Char.action == 9 && Level.environment_kind == 5) {
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
        Char.life_points = 0;
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
    cut_actor(CutDir);
    copy.tile = FirstInRow(Char.row);
    copy.x = Char.x;
    copy.direction = Char.direction;
    copy.action = Char.sequence_id;
    copy.saved_sequence = Char.sequence_position;
    copy.combat_mode = Char.stance;
    copy.life = Char.life_points;
    copy.palette_kind = Char.field_34 == 4 ? OppBank[0] : OppBank[1];
    set_special_opponent(&copy);
    add_opponent_to_room(actor_room, &copy);
}

void far pascal set_special_opponent(opponent_record *record)
{
    if (Char.character_kind == 2) {
        if ((Char.sequence == 186) | (Char.sequence == 100))
            record->position.saved_y = Char.y;
        if (Level.preferred_opponent == 7) {
            record->appearance = 7;
            return;
        }
    } else if (Char.character_kind == 11) {
        snake_update_opp_info(record);
        return;
    } else if (Char.character_kind == 6) {
        update_jaffar_slot(record);
    }
}

void far pascal RemoveOppFromRoom(unsigned char room, int index)
{
    signed char count = Level.room_objects[room - 1].opponent_count;
    opponent_record *record, *following;

    if (count > index) {
        if (count > index + 1) {
            do {
                record = GetOppInfoIdx(room, index);
                following = GetOppInfoIdx(room, index + 1);
                *record = *following;
                record->index = index;
                if (Room.number == room) {
                    RoomOpp[index] = RoomOpp[index + 1];
                    RoomOpp[index].slot = index;
                }
            } while (++index + 1 < count);
        }
        --Level.room_objects[room - 1].opponent_count;
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
        result = &Level.room_objects[room - 1].opponents[index];
    else
        result = 0;
    return result;
}

signed char far pascal pick_hidden_actor(signed char direction)
{
    signed char best = -1, index, count;
    int best_dx, dx, best_dy, dy;
    CHARREC *actor;
    opponent_record *record;

    if (Kid.room != 0) {
        index = 0;
        count = Level.room_objects[Kid.room - 1].opponent_count;
        for (; index < count; ++index) {
            record = GetOppInfoIdx(Room.number, index);
            actor = &RoomOpp[index];
            dy = abs(actor->y - Kid.y);
            if (actor->alive < 0 && actor->character_kind != 11 &&
                !((actor->character_kind == 7 || actor->character_kind == 8) && (unsigned char)record->position.saved_y == 2)) {
                if (direction == 1)
                    dx = abs(actor->x - Kid.x);
                else if (direction == -1 && actor->x <= Kid.x)
                    dx = Kid.x - actor->x < 0 ? 0 : Kid.x - actor->x;
                else if (direction == 0 && actor->x > Kid.x)
                    dx = actor->x - Kid.x < 0 ? 0 : actor->x - Kid.x;
                else
                    dy = -1;
            } else
                dy = -1;

            if (dy >= 0 && dx >= 0) {
                if (best != -1) {
                    int difference;
                    best_dx = abs(RoomOpp[best].x - Kid.x);
                    best_dy = abs(RoomOpp[best].y - Kid.y);
                    difference = abs(dy - best_dy);
                    if (!(difference > 42 && dy < best_dy) &&
                        (difference >= 42 || dx >= best_dx))
                        continue;
                }
                best = index;
            }
        }
    }
    return best;
}

int far pascal is_selected_actor(void)
{
    if (pick_hidden_actor(Kid.x >= Char.x ? -1 : 0) == actor_index)
        return 1;
    return 0;
}

void far pascal check_generator_points(unsigned char room)
{
    generator_point *point;

    if (Kid.alive < 0 && Kid.character_kind != 1) {
        point = find_active_generator(Kid.room, Kid.row);
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
    signed char index = Level.room_objects[room - 1].opponent_count;
    signed char row;
    opponent_record *record;

    if (index < 5) {
        ++Level.room_objects[room - 1].opponent_count;
        record = GetOppInfoIdx(room, index);
        record->tile = FirstInRow(point->column) + point->row;
        if (point->row <= 5) {
            record->x = ColEdgeX[point->row + 3];
            record->direction = 0;
        } else {
            record->x = ColEdgeX[point->row + 7];
            record->direction = -1;
        }
        record->flags = Level.generators[room - 1].spawn_flags;
        record->palette_kind = OppBank[0];
        record->index = index;
        record->combat_mode = 0;
        record->behavior = point->behavior;
        record->position.entry.alternate_column = point->alternate_column;
        record->max_life = record->life = point->flags & 15;
        actor_index = index;
        Char.direction = record->direction;
        Char.x = record->x;
        Char.character_kind = OppCharId[(signed char)Level.preferred_opponent];
        Char.column = point->row;
        Char.row = point->column;
        Char.behavior = point->behavior;
        actor_room = room;
        Char.life_delta = Char.life_points = Char.life_initial = record->life;
        Char.alive = -1;
        Char.field_34 = 4;
        reset_char_y();
        ClearCharVel();
        Char.sequence_group = 0;
        Char.field_0f = 1;
        row = Kid.column;
        if (Kid.direction != Char.direction &&
            ((Kid.frame >= 1 && Kid.frame <= 14) ||
             (Kid.frame >= 49 && Kid.frame <= 56) ||
             (Kid.frame >= 34 && Kid.frame <= 44)))
            row -= Minus1[Kid.direction + 1];
        if (point->row + 3 >= row && point->row - 3 <= row &&
            (point->flags & 15) >= 6) {
            Char.stance = 1;
            JumpSeq(90);
            Char.alert_delay = 3;
        } else {
            Char.stance = 0;
            JumpSeq(84);
            Char.alert_delay = 0;
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
        abs(point->row - Kid.column) > 2 && point->enabled != 0)
        result = 1;
    else
        result = 0;
    return result;
}

extern signed char LevelIdx;
extern int NextLevel;
extern unsigned long FrameNum;

signed char far pascal find_wall_column(unsigned char room, signed char row,
                                    signed char column, signed char direction);

int far pascal should_generate(generator_point *point, unsigned char room)
{
    signed char count = Level.room_objects[room - 1].opponent_count;
    signed char index, room_row;
    unsigned char between;
    int allowed;

    if (count < 5 && LevelIdx == NextLevel && FrameNum % 3 != 0) {
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
                    ((point->row < Kid.column && RoomOpp[index].x < Kid.x) ||
                     (point->row > Kid.column && RoomOpp[index].x > Kid.x)) &&
                    ((point->row < room_row && Kid.column < room_row) ||
                     (point->row > room_row && Kid.column > room_row))) {
                    ++between;
                    if (point->limit <= between ||
                        (point->row < Kid.column && RoomOpp[index].column <= point->row) ||
                        (point->row > Kid.column && RoomOpp[index].column >= point->row))
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
        point = &Level.generators[room - 1].points[index];
    else
        point = 0;
    return point;
}

int far pascal generator_has_tumble(unsigned char room)
{
    int found = 0;
    signed char count = Level.generators[Char.room - 1].count;
    signed char index = 0;
    generator_point *point;

    for (; index < count && !found; ++index) {
        point = get_generator(room, index);
        if (point && (point->flags & 128)) found = 1;
        else found = 0;
    }
    return found;
}

void far pascal set_palette(int resource, int start, unsigned char size,
                                   unsigned char kind);

void far _fastcall reserve_opponent_bank(unsigned char kind)
{
    int slot;

    if (Level.preferred_opponent != 0 && Level.preferred_opponent != 5 && Level.preferred_opponent != 6) {
        Char.field_34 = Level.preferred_opponent == 2 ? 8 : 4;
        return;
    }
    Char.field_34 = 0;
    if (kind == 0)
        kind = 1;

    for (slot = 0; slot < 2 && Char.field_34 == 0; ++slot) {
        if (OppBank[slot] == kind)
            Char.field_34 = 4 << slot;
    }
    if (Char.field_34 == 0) {
        for (slot = 0; slot < 2 && Char.field_34 == 0; ++slot) {
            if (OppBank[slot] == 0) {
                OppBank[slot] = kind;
                Char.field_34 = 4 << slot;
            }
        }
    }
    if (Char.field_34 == 0)
        Char.field_34 = 4;
}

void far pascal load_opponent_banks(void)
{
    int slot;
    int start;

    if (Level.preferred_opponent != 0 && Level.preferred_opponent != 5 && Level.preferred_opponent != 6) {
        if (Level.preferred_opponent == 2) {
            set_palette(750, 48, 16, 0); return;
        }
    } else {
        for (slot = 0, start = 32; start < 64; ++slot, start += 16) {
            if (OppBank[slot] != 0) set_palette(750, start, 16, OppBank[slot] - 1);
        }
    }
}

extern unsigned char CutRoom;
extern int CutTimer;

void far pascal load_actor_slot(signed char index);
int far pascal query_sound(int object_id);
void far pascal add_opponent_death_song(void);
void far pascal DrawOppMeter(signed char index, int change, int life);
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
void far pascal SetKidDeathMethod(unsigned char state);
void far pascal ChangeMeters(void);
signed char far pascal GetRow(int y);

void far pascal cut_room_opponents(void)
{
    signed char index = 0;
    signed char count = Level.room_objects[Room.number - 1].opponent_count;

    for (; index < count; ++index) {
        load_actor_slot(index);
        if (Char.direction == 86 || Char.y < 384)
            continue;
        if (Level.environment_kind == 5 && InWater.character == actor_index)
            continue;

        if (Char.action != 4 && Char.action != 3 &&
            (Char.frame < 202u || Char.frame > 212u)) {
            if (!update_opponent_record())
                save_actor_slot();
        } else if (!Room.below) {
            update_opponent_record();
            if (Char.character_kind == 2 && !query_sound(10025))
                AddSound(25);
        }

        add_opponent_death_song();
        DrawOppMeter(actor_index, 0, Char.life_points);
        if (Kid.opponent_index == actor_index)
            Kid.opponent_index = -1;
    }
}

void far pascal check_cut(void)
{
    if (CutTimer) {
        --CutTimer;
        return;
    }

    copy_kid_to_char();
    setup_char();
    get_char_edges();
    CutDir = choose_cut_side();

    if (CutDir != -1 && actor_room != 0) {
        Kid = Char;
        CutRoom = actor_room;
        transfer_room_actors();
        return;
    }

    if (actor_room == 0 && Char.alive < 0) {
        if (Level.environment_kind == 5 && CutDir == 0) {
            Char.x = AddCharX(320);
            trigger_character_water();
        } else {
            AddKidScream(Room.number);
            if (Level.environment_kind != 5) {
                decrement_life_points(100);
                Char.frame = 185;
                SetKidDeathMethod(3);
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
        if (Char.direction == 86)
            continue;

        if ((((Char.frame >= 202 && Char.frame <= 212 && Char.behavior == 1) ||
              (Char.frame >= 186 && Char.frame <= 201 && Char.behavior != 0)) &&
             (Char.character_kind == 2 || Char.character_kind == 4)) ||
            actor_follows_to_adjacent_room() ||
            (Char.row && Level.level_number == 5 &&
             (actor_room == 10 || actor_room == 7 || actor_room == 12)) ||
            Char.character_kind == 0) {
            stay = 0;
        } else if (Char.character_kind == 6 &&
                   (actor_room == 7 || actor_room == 8) &&
                   Level.environment_kind == 6 &&
                   (CutRoom == 7 || CutRoom == 8) &&
                   Level.environment_kind == 6) {
            stay = 0;
        } else if (Char.alive >= 0 || !can_actor_follow()) {
            stay = 1;
        } else {
            if (CutDir == 0) {
                stay = Char.x >= 208;
            } else if (CutDir == 1) {
                stay = Char.x < 371;
            } else if (CutDir == 2) {
                stay = Char.row >= 0;
            } else {
                stay = Char.row < 3;
            }
        }

        if (stay) {
            removed = update_opponent_record();
        } else {
            transfer_opponent_record();
            removed = 1;
        }
        if (removed) {
            --index;
            --count;
        }
    }
}

int far pascal can_actor_follow(void)
{
    int allowed;

    if (Level.room_objects[Kid.room - 1].opponent_count < 5)
        allowed = 1;
    else
        allowed = 0;
    if (allowed) {
        if (Char.stance == 1 || Char.character_kind == 7 || Char.character_kind == 8)
            allowed = 1;
        else
            allowed = 0;
        if (allowed) {
            allowed = (Char.character_kind != 10);
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

    if (Kid.room == destination)
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
            room = Level.room_links[room - 1].neighbors[ROOM_ABOVE];
            if (room) {
                Char.y += 192;
                Char.row += 3;
            } else {
                room = previous;
                previous = 0;
            }
        } else if (Char.row >= 3) {
            room = Level.room_links[room - 1].neighbors[ROOM_BELOW];
            if (room) {
                Char.y -= 192;
                Char.row -= 3;
            } else {
                room = previous;
                previous = 0;
            }
        }

        if (Char.column < 0) {
            room = Level.room_links[room - 1].neighbors[ROOM_LEFT];
            if (room) {
                Char.x += 320;
                Char.column += 10;
            }
        } else if (Char.column >= 10) {
            room = Level.room_links[room - 1].neighbors[ROOM_RIGHT];
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
    actor_room = Level.room_links[actor_room - 1].neighbors[direction];
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

extern int ImHeight, ImWidth;
extern int LeftEdge, RightEdge;
extern int GuardTimer;

extern signed char script_motion;

void far pascal ReadCell(unsigned char room, signed char column, signed char row);
int far pascal final_noop(void);
void far pascal kill_kid_on_wharf(void);
void far pascal cut_kid_inside_level6(void);
void far pascal update_skeleton(void);
void far pascal run_jaffar(void);
void far pascal Head_Prog(void);
void far pascal Snake_Prog(void);
void far pascal altar_shadow_progress(void);
void far pascal chase_opponent(void);
void far pascal alert_opponent(void);
void far pascal guard_en_garde(void);
void far pascal reset_actor_inputs(void);
void far pascal run_shadow_script(void);
void far pascal run_guard_script(void);

int far pascal choose_cut_side(void)
{
    int y = Char.y;
    int mode = Char.action;
    unsigned int action = Char.frame;
    int side = -1;

    if ((Room.number == 7 || Room.number == 8) && Level.environment_kind == 6 &&
        (RoomOpp[0].sequence == 243 || RoomOpp[1].sequence == 243)) {
        side = final_noop();
    } else if (LevelIdx == NextLevel && Char.sequence != 70 &&
               (Char.character_kind != 1 || Char.frame != 185)) {
        if (mode != 5 && mode != 4 && mode != 3 && y < 10 && y >= -16) {
            if (Level.level_number != 5 || actor_room != 3)
                side = 2;
        } else if (y >= 231 &&
                   (Level.room_links[actor_room - 1].neighbors[ROOM_BELOW] || y >= ImHeight + 231) &&
                   (Level.environment_kind != 5 || actor_index != InWater.character)) {
            side = 3;
        } else if ((action >= 135 && action < 150) ||
                   (action >= 110 && action <= 119) ||
                   (action >= 150 && action < 163) ||
                   (action >= 166 && action < 169) || mode == 7) {
            side = -1;
        } else if (Char.direction == -1) {
            if (LeftEdge <= 125 &&
                (Level.room_links[actor_room - 1].neighbors[ROOM_LEFT] || LeftEdge <= 125 - ImWidth)) {
                if (Level.level_number == 9 && actor_room == 8 && Char.row == 0 && Char.frame == 40)
                    ++NextLevel;
                else
                    side = 0;
            } else if (LeftEdge >= 450 &&
                       (Level.room_links[actor_room - 1].neighbors[ROOM_RIGHT] || LeftEdge >= ImWidth + 449) &&
                       (Level.environment_kind != 1 || actor_room != 3)) {
                side = 1;
            }
        } else {
            ReadCell(actor_room, 9, Char.row);
            if (RightEdge >= 454 &&
                (Level.room_links[actor_room - 1].neighbors[ROOM_RIGHT] || RightEdge >= ImWidth + 454) &&
                (Level.environment_kind != 1 || actor_room != 3)) {
                side = 1;
            } else if (RightEdge <= 129 &&
                       (Level.room_links[actor_room - 1].neighbors[ROOM_LEFT] || RightEdge <= 130 - ImWidth)) {
                side = 0;
            }
        }
    }
    switch (side) {
      case 0:
        if (actor_room == 1 && Level.level_number == 2)
            side = -1;
        break;
      case 1:
        if (Level.environment_kind == 5 && actor_room == 15) {
            side = -1;
            if (Char.sequence != 71)
                kill_kid_on_wharf();
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
             (Char.sequence == 68 || Char.sequence == 25 || Char.sequence == 10))) {
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
        cut_kid_inside_level6();
    return side;
}

void far pascal update_automatic_actor(void)
{
    signed char kind;

    reset_actor_inputs();
    kind = Char.character_kind;
    if (JustBlocked)
        --JustBlocked;
    if (GuardTimer)
        --GuardTimer;
    if (Refract)
        --Refract;

    switch (kind) {
      default:
        if (Char.character_kind != 0) run_guard_script();
        return;
      case 1:
        run_shadow_script(); return;
      case 4:
      case 10:
        update_skeleton();
        return;
      case 6:
        run_jaffar();
        return;
      case 7:
      case 8:
        Head_Prog();
        return;
      case 11:
        Snake_Prog();
        return;
    }
}

extern unsigned char Dir[5];
extern unsigned char Joy[3];
void *cdecl memset(void *buffer, int byte, unsigned size);
#pragma intrinsic(memset)

void far pascal reset_actor_inputs(void)
{
    memset(Dir, 0, sizeof(Dir));
    memset(Joy, 0, sizeof(Joy));
}

void far pascal run_shadow_script(void)
{
    if (actor_room == 4 && Level.level_number == 13)
        altar_shadow_progress();
}

void far pascal run_guard_script(void)
{
    if (Char.alive < 0 && Char.life_points != 0 &&
        (Char.life_points + Char.life_delta) != 0) {
        if (Char.stance == 0) {
            if (Char.frame >= 186u && Char.frame <= 212u)
                chase_opponent();
            else
                alert_opponent();

            if (AlertGuard != 0 &&
                Level.room_objects[actor_room - 1].opponent_count - actor_index == 1) {
                AlertGuard = 0;
                return;
            }
        } else if (Kid.sequence_group != 13) {
            guard_en_garde();
            return;
        } else if (Level.actor_phase != 0 && Char.sequence != 110) {
            JumpSeq(110);
        }
    }
}

typedef struct {
    unsigned char head[6];
    unsigned char flags;
} hit_action_record;
extern hit_action_record far frame_table[];

extern signed char hit_actor_motion;

extern unsigned char actor_stage;
extern unsigned char actor_counterface;
static int opponent_recovery_time[] = {
    20, 20, 20, 20, 10, 10, 10, 10, 0, 10, 0
};

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
void far pascal set_kid_palette(unsigned char style);
signed char far pascal get_behind(void);
signed char far pascal get_cells_ahead(unsigned char direction);
int far pascal IsWall(signed char kind);
void far pascal head_set_to_flee_mode(void);
void far pascal get_frame_info(void);
int far pascal get_edge_distance(void);
int far pascal IS_SIX_KIND(signed char kind);
int far pascal IsMobFloor(signed char kind);
void far pascal reread_cells(void);
void far pascal PutDeadBodyOnSolidGround(void);
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
        load_actor_slot(index);
        if (Char.action == 99) {
            load_shadow_with_opponent(index);
            if (Opp.action == 99)
                Opp.action = 1;
            stab_actor();
            Refract = opponent_recovery_time[
                Level.room_objects[actor_room - 1].opponents[actor_index].flags];
            save_shadow_with_opponent();
        }
    }

    if (Kid.action == 99) {
        for (index = 0; index < count; ++index) {
            load_actor_slot(index);
            if (Char.character_kind == 7 || Char.character_kind == 8) {
                if (Char.frame == 167 || Char.frame == 181)
                    break;
            } else if (IsTheStrikePos(Char.character_kind, Char.frame))
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

    if (Char.alive >= 0)
        return;
    if (Char.character_kind == 0 && Char.sequence == 5 && (Level.environment_kind == 2 || Level.environment_kind == 6))
        set_kid_palette(Level.environment_kind);

    do {
        if (Char.stance == 255 || Char.sequence == 10 || Char.sequence == 16 ||
            Char.sequence == 28 || Char.sequence == 14) {
            decrement_life_points(100);
            begin_actor_death();
        } else {
            if (Char.character_kind == 7 || Char.character_kind == 8) {
                next_action = 153;
                damage = actor_stage == 2 ? 1 : 2;
                if (damage < Char.life_points)
                    head_set_to_flee_mode();
            } else if (Char.character_kind == 11 || Opp.character_kind == 11) {
                damage = 100;
                next_action = default_action;
            } else {
                damage = (Char.character_kind == 6 || Opp.character_kind == 6) ? 100 : 1;
                next_action = Opp.direction != Char.direction ? 74 : 94;
                if (Opp.character_kind == 7 || Opp.character_kind == 8) {
                    if (Char.stance != 1 &&
                        !(frame_table[Char.frame].flags & 64))
                        next_action = 45;
                    else if (Char.frame >= 246u && Char.frame <= 261u) {
                        if (IsWall(get_behind()))
                            Char.x = AddCharX(16);
                        else if (IsWall(get_cells_ahead(1)))
                            Char.x = AddCharX(-16);
                    }
                }
                if (Char.character_kind == 0 && next_action != 45)
                    Char.stance = 1;
            }

            if (!decrement_life_points(damage) && Char.character_kind != 11) {
                JumpSeq(next_action);
                reset_char_y();
                Char.vertical_step = 0;
                break;
            }
        }

        get_frame_info();
        next_action = get_edge_distance();
        if ((IS_SIX_KIND(get_under_foot()) ||
             (IS_SIX_KIND(get_behind()) && next_action >= 4)) &&
            Char.character_kind != 7 && Char.character_kind != 8 && Char.character_kind != 11 && Char.character_kind != 6) {
            next_action -= 8;
            if (Cell.column != Char.column)
                next_action -= 32;
            Char.x = AddCharX(next_action);
            reread_cells();
            ++Char.row;
            JumpSeq(81);
        } else {
            begin_actor_death();
            reset_char_y();
            Char.vertical_step = 0;
        }
    } while (0);

    if (Opp.character_kind != 7 && Opp.character_kind != 8 && Opp.character_kind != 11) {
        if (Char.character_kind == 0)
            sound = Opp.character_kind == 10 ? 194 : 14;
        else if (Char.character_kind == 4)
            sound = 72;
        else if (Char.character_kind == 7 || Char.character_kind == 8)
            sound = 86;
        else if (Char.character_kind == 11)
            sound = 70;
        else
            sound = 21;
        AddSound(sound);
    }
    AnimChar();
    if (Char.sequence == 85) {
        reread_cells();
        damage = get_under_foot();
        if (IS_SIX_KIND(damage) || IsMobFloor(damage)) {
            Char.x = AddCharX(-24);
            reread_cells();
        }
        PutDeadBodyOnSolidGround();
    }
}

void far pascal begin_actor_death(void)
{
    int next_action;
    signed char index;

    Char.alert_delay = 0;
    Char.field_0f = 1;
    next_action = -1;
    if (Char.character_kind == 4) {
        bones_fall();
    } else if (Char.character_kind == 7 || Char.character_kind == 8) {
        Char.direction = ~Kid.direction;
        Char.life_points = 0;
        next_action = 154;
    } else if (Char.character_kind == 11) {
        next_action = 174;
    } else if (Char.character_kind == 6) {
        next_action = 196;
    } else {
        index = get_behind();
        if (Char.character_kind != 0 && Char.direction == -1 &&
            (index == 4 || (signed char)get_under_foot() == 4)) {
            index = GetCellCol();
            Char.x = ColEdgeX[index - (Cell.kind != 4) + 5] + 14;
            Char.x = AddCharX(20);
        }

        if (Char.character_kind == 0 && Char.frame >= 246u && Char.frame <= 261u) {
            next_action = 98;
            if (Opp.character_kind == 11)
                AddSound(69);
            else if (Opp.character_kind == 7 || Opp.character_kind == 8)
                AddSound(89);
            else
                AddSound(14);
        } else if (Opp.character_kind == 11) {
            next_action = 97;
            AddSound(69);
        } else {
            if (Opp.character_kind == 6)
                dismiss_other_jaffars();
            next_action = 85;
            if (Char.character_kind != 0 && check_opponent_tumble()) {
                next_action = 185;
                Char.x = AddCharX(-8);
            }
        }
        if (Char.character_kind == 0) {
            switch (Opp.character_kind) {
              case 2:
                index = 10 + ((Level.preferred_opponent == 0) ? 4 : 0);
                break;
              case 6:
                index = 17;
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
            SetKidDeathMethod(index);
        }
    }
    if (Opp.character_kind == 0 || Opp.character_kind == 1) {
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

    if (Char.stance != 1 || Char.row != Opp.row ||
        (abs(Char.y - Opp.y) >= 7 &&
         Char.character_kind != 7 && Char.character_kind != 8 && Opp.character_kind != 7 && Opp.character_kind != 8) ||
        !is_striking_position(Char.character_kind, Char.frame) || Opp.alive >= 0 ||
        Opp.character_kind == 10 || Opp.character_kind == 1 || Opp.action == 8 ||
        Opp.sequence == 110)
        return;
    if (Char.character_kind == 1 && Opp.character_kind == 0)
        return;

    distance = get_opp_dist();
    if (Char.character_kind == 7 || Char.character_kind == 8) {
        minimum = 0;
        maximum = 28;
    } else {
        minimum = 0;
        maximum = 44;
    }

    if (Char.direction != Opp.direction && distance >= 0 && distance <= maximum &&
        Opp.character_kind != 7 && Opp.character_kind != 8 && Opp.character_kind != 11 &&
        Char.character_kind != 11 && (Opp.frame == 161 || Opp.frame == 150)) {
        Opp.frame = 161;
        if (Char.character_kind == 0)
            Char.opponent_index = Opp.slot;
        else {
            Opp.opponent_index = actor_index;
            JustBlocked = 4;
        }

        if (Char.character_kind == 7 || Char.character_kind == 8)
            distance = head_blocked();
        else if (Char.character_kind == 10)
            distance = block_flame_sword();
        else
            distance = 69;
        if (distance != -1)
            JumpSeq(distance);
        AnimChar();
    } else if (IsTheStrikePos(Char.character_kind, Char.frame)) {
        get_char_strike_ranges(&minimum, &maximum);
        if (distance >= minimum && distance <= maximum) {
            Opp.action = 99;
            if (Char.character_kind == 0)
                Char.opponent_index = Opp.slot;
        }
    } else if ((Char.character_kind == 7 || Char.character_kind == 8) && Char.sequence_group == 1 && JustBlocked == 0)
        Opp.action = 99;

    if (IsTheStrikePos(Char.character_kind, Char.frame) &&
        (Opp.frame != 161 || Opp.character_kind == 7 || Opp.character_kind == 8) &&
        Opp.action != 99) {
        if (Char.character_kind == 10)
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

    if (Char.character_kind != 0 && Char.character_kind != 1) {
        if (Char.character_kind == 10) {
            *minimum = -10;
            *maximum = 38;
        } else if (Char.character_kind == 11) {
            if (63 * Opp.row + 51 <= Opp.y) {
                if (Opp.direction == Char.direction) {
                    *minimum = -2;
                    *maximum = 27;
                } else {
                    *minimum = -27;
                    *maximum = -4;
                    return;
                }
            }
        } else if (Char.character_kind == 6) {
            *minimum = 13;
            *maximum = 53;
            return;
        } else if (Opp.direction != Char.direction) {
            *minimum = 19;
            *maximum = 46;
            if (Char.character_kind == 4) {
                *maximum = 48;
                return;
            }
        } else if (Opp.stance == 1) {
            *minimum = 34;
            *maximum = 57;
            return;
        } else {
            *minimum = 22;
            *maximum = 42;
            return;
        }
    } else {
        if (Opp.character_kind == 11) {
            if (Opp.sequence_group != 6 && Opp.sequence != 171 && Char.sequence == 96) {
                if (Opp.direction == Char.direction) {
                    *minimum = 31;
                    *maximum = 75;
                } else {
                    *minimum = -7;
                    *maximum = 45;
                }
            }
        } else if (Opp.character_kind == 7 || Opp.character_kind == 8) {
            *minimum = 19;
            *maximum = 46;
        } else if (Opp.character_kind == 6) {
            if (Opp.direction != Char.direction) {
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
            if (Opp.character_kind == 4)
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
        result = Char.sequence == 173 && (action == 274 || action == 280);
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
        if (Char.sequence == 173 && value == 280)
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

    if (!IsWall(tile) && (tile != 4 || (unsigned char)modifier >= 112))
        allowed = 1;
    else
        allowed = 0;
    if (allowed && Char.character_kind != 10) {
        allowed = !IsImpassable(tile);
        if (allowed && Char.character_kind != 7 && Char.character_kind != 8)
            allowed = !IS_SIX_KIND(tile);
    }
    return allowed;
}

int unused_auto_flag = 0;                            /* never referenced */
