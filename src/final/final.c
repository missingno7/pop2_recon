#pragma pack(1)
typedef struct {
    unsigned char number;                             /* 00 */
    unsigned char left, right, above, below;           /* 01-04 */
    unsigned char above_left, above_right;             /* 05-06 */
    unsigned char below_left, below_right;             /* 07-08 */
    unsigned char custom;                             /* 09 */
} ROOMREC;

typedef struct {
    signed char index, flipped;                       /* 00-01 */
    int x, y, image;                                   /* 02,04,06 */
    unsigned char image_set;                          /* 08 */
    int clip_top, clip_left, clip_bottom, clip_right;    /* 09,0b,0d,0f */
    int depth;                                        /* 11 */
    unsigned char object_type;                        /* 13 */
} FCHARREC;

typedef struct {
    int x, y;                                         /* 00,02 */
    unsigned char room;                               /* 04 */
    int velocity, auxiliary;                          /* 05,07 */
    signed char kind, row;                             /* 09-0a */
    unsigned int flags;                               /* 0b */
} MOBREC;

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

/* FINAL.C, MSC 6.00A medium model. Claims and drafts are listed in notes.md. */

#pragma pack(1)

typedef signed char sbyte;
typedef unsigned char byte;

typedef struct {
    byte kind;
    byte attributes[3];
    unsigned int animation;
} FinalObject;

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

extern ROOMREC Room;
extern LEVELREC Level;
extern int Pause;
extern CHARREC Char;
extern CHARREC RoomOpp[5];
extern CHARREC Opp;
extern CHARREC Kid;
extern int trob_count;

extern sbyte far pascal ReadCell(sbyte kind, sbyte room, sbyte column);
extern unsigned int far pascal Rnd(unsigned int max);
extern sbyte far pascal FirstInRow(sbyte row);
extern void far pascal select_room_arrays(sbyte kind);
extern void far pascal trigger_flask(sbyte kind, sbyte cell);
extern void far pascal mark_redraw(int cell);
extern void far pascal JumpSeq(int sequence);
extern void far pascal AddSound(int sound);
extern int far pascal is_dead_pos(int sequence);

void far pascal draw_crystal(FinalObject near *object);
void far pascal draw_gargoyle(FinalObject near *object);
void far pascal FinlGenerateOpp(sbyte variant, sbyte room);

void far pascal draw_final_object(register FinalObject near *object)
{
    if (Room.number == 6 && Level.environment_kind == 6 &&
        (object->animation & 0x1000))
        draw_crystal(object);

    if ((Room.number == 7 || Room.number == 8) &&
        Level.environment_kind == 6 && (object->animation & 0x1000))
        draw_gargoyle(object);
}

void far pascal load_final_code(void)
{
    Pause = 0;
}

void far pascal check_dead_opponent(void)
{
    if (Char.alive == 6) {
        sbyte cell;
        unsigned int frame;

        cell = ReadCell(Char.room, Char.column, Char.row);
        frame = Rnd(2);
        if (frame != 0 && cell == 1 && trob_count < 4) {
            cell = FirstInRow(Char.row) + Char.column;
            if (cell < 22 || cell > 24) {
                Level.cell_kinds[2][cell] = 10;
                Level.cell_info[2][cell].modifier = (frame | 0x200) << 5;
                select_room_arrays(3);
                trigger_flask(3, cell);
                mark_redraw(cell);
            }
        }
        ++Char.alive;
    } else if (Char.alive >= 96 && Char.sequence != 239) {
        JumpSeq(239);
        AddSound(197);
        ++Char.alive;
    }
    if ((*(volatile byte *)&Char.field_0f) == 0 && Char.alive > 6)
        ++Char.alive;
}

void far pascal check_new_opponent(sbyte count)
{
    sbyte variant;
    sbyte room = -1;

    if (count <= 1) {
        if (count == 1) {
            if (RoomOpp[0].sequence != 238) {
                room = (RoomOpp[0].row == 1) ? 2 : 1;
                if (room == Kid.row)
                    room = -1;
            }
        } else {
            room = (Kid.row == 2) ? 1 : 2;
        }
    }
    if (room >= 0 && (count == 0 || Rnd(20) == 0)) {
        variant = (room == 1) ? 4 : 2;
        if (Rnd(1))
            variant += 4;
        FinlGenerateOpp(variant, room);
    }
}

typedef struct {
    byte cell;
    int x;
    sbyte direction;
    byte style;
    byte unused[4];
    byte bank;
    byte index;
    byte status;
    byte strength;
    byte delay;
    byte unused2[2];
    byte initial_strength;
    sbyte stance[4];
    sbyte row;
} OpponentSlot;

extern int ColEdgeX[20];
extern byte opponent_bank;
extern byte OppCharId[];

extern OpponentSlot near *far pascal GetOppInfoIdx(int bank, int serial);
extern void far _fastcall reserve_opponent_bank(byte kind);
extern void far pascal load_opponent_banks(void);
extern void far pascal reset_char_y(void);
extern void far pascal ClearCharVel(void);
extern void far pascal AnimChar(void);
extern void far pascal save_actor_slot(void);

void far pascal FinlGenerateOpp(sbyte variant, sbyte room)
{
    sbyte index;
    OpponentSlot near *opponent;

    opponent = GetOppInfoIdx(3, index = Level.room_objects[2].opponent_count++);
    opponent->cell = FirstInRow(room) + variant;
    opponent->x = ColEdgeX[variant + 5];
    if (variant < 5)
        opponent->direction = 0;
    else
        *(byte near *)&opponent->direction = 0xff;
    opponent->style = 7;
    if (opponent_bank == 0) {
        reserve_opponent_bank(1);
        load_opponent_banks();
    }
    opponent->bank = opponent_bank;
    opponent->index = index;
    opponent->delay = opponent->status = 0;
    opponent->row = room;
    opponent->initial_strength = opponent->strength = Rnd(2) + 3;
    Char.slot = index;
    Char.direction = opponent->direction;
    Char.x = opponent->x;
    Char.character_kind = OppCharId[((sbyte)Level.preferred_opponent)];
    Char.column = variant;
    Char.row = room;
    (*(volatile unsigned char *)&Char.behavior) = 0;
    Char.room = 3;
    Char.life_delta = Char.life_points = Char.life_initial = opponent->strength;
    Char.alive = -1;
    Char.field_34 = 4;
    reset_char_y();
    ClearCharVel();
    (*(volatile int *)&Char.sequence_group) = 0;
    (*(volatile byte *)&Char.field_0f) = 1;
    (*(volatile byte *)&Char.stance) = 0;
    JumpSeq(238);
    Char.alert_delay = 0;
    AnimChar();
    AddSound(197);
    if ((byte)Kid.opponent_index == (byte)-1)
        Kid.opponent_index = Char.slot;
    save_actor_slot();
}

void far pascal final_noop(void)
{
}

extern FCHARREC FChar;

void far pascal clip_final_character(void)
{
    if (Char.room == 3 && Level.environment_kind == 6 &&
        Char.row == 1 && is_dead_pos((int)Char.frame))
        FChar.clip_bottom = 119;
}

extern sbyte CutRoom;
extern byte scroll_pending;
extern int sound_period;
extern byte LevelIdx;
extern void far pascal ClrAll(void);
extern int far pascal query_sound(int tune);
extern void far pascal AddSong(int tune);
extern void far pascal play_nis(int mode);
extern void far pascal reset_level_overlays(void);
extern void far pascal load_level(sbyte room);
extern void far pascal fade_scene(int ticks, int speed, byte color, byte direction);
extern void far pascal set_kid_palette(sbyte room);

void far pascal FinlWatchKid(void);
void far pascal FinlWatchOpps(void);

void far pascal cut_kid_to_chess_room(void)
{
    Char.field_34 = 2;
    Char.direction = 0;
    JumpSeq(55);
    (*(volatile byte *)&Char.stance) = 1;
    AddSound(19);
    CutRoom = Char.room = 4;
    Char.column = 5;
    Char.x = ColEdgeX[10] + 14;
    Char.row = 1;
    reset_char_y();
    Char.life_delta = Char.life_points;
    AnimChar();
    Kid = Char;
    ClrAll();
    scroll_pending = 0;
}

void far pascal run_final_frame(void)
{
    FinlWatchKid();
    FinlWatchOpps();

    if (!query_sound(sound_period + 10000)) {
        int tune = -1;

        switch (Room.number) {
        case 3:
            tune = 269;
            break;
        case 6:
            if (Level.room_objects[6 - 1].opponent_count)
                tune = 270;
            break;
        case 7:
        case 8:
            if (Level.room_objects[Room.number - 1].opponent_count)
                tune = 263;
            break;
        }
        if (tune != -1)
            AddSong(tune);
    }
}

void far pascal enter_princess_room(void)
{
    play_nis(5);
    reset_level_overlays();
    load_level(LevelIdx);
    fade_scene(1000, 240, 16, 0);
    set_kid_palette(6);
    cut_kid_to_chess_room();
}

typedef struct {
    byte data[2];
    int shape;
    byte tail[16];
} ForegroundImage;

typedef struct {
    sbyte shape_delta;
    byte attributes[24];
} ShapeEntry;

typedef struct {
    byte room, tile;
    int first_shape;
    byte data[24];
    ShapeEntry entries[1];
} RoomShapeTable;

extern RoomShapeTable near *shapes_by_room;
extern ForegroundImage far foreground_image[];
extern _segment foreground_segment;
extern int foreground_count;
extern void far cdecl move_bytes(void far *dst, const void far *src, unsigned int size);

void far pascal sort_foreground(void)
{
    ForegroundImage _based(foreground_segment) *destination;
    int out, index;

    out = index = foreground_count - 1;
    if (index >= 0) {
        int _based(foreground_segment) *shape_ptr;
        ForegroundImage saved;

        destination = (ForegroundImage _based(foreground_segment) *)&foreground_image[out];
        shape_ptr = (int _based(foreground_segment) *)&foreground_image[index].shape;
        do {
            int shape;

            shape = shapes_by_room->entries[*shape_ptr - shapes_by_room->first_shape].shape_delta
                  + shapes_by_room->first_shape;
            if ((shape >= 0x1021 && shape <= 0x1024) ||
                (shape >= 0x1058 && shape <= 0x105c)) {
                if (index != out) {
                    ForegroundImage far *source;
                    char _based(void) *at;
                    _segment storage_segment = (_segment)foreground_image;

                    at = (char _based(void) *)shape_ptr;
                    source = storage_segment :> (ForegroundImage _based(void) *)(at - 2);
                    saved = *source;
                    move_bytes(source, storage_segment :> (void _based(void) *)(at + 18),
                               (out - index) * sizeof(ForegroundImage));
                    *destination = saved;
                }
                --destination;
                --out;
            }
            shape_ptr -= sizeof(ForegroundImage) / sizeof(*shape_ptr);
        } while (--index >= 0);
    }
}

extern int player_right;
extern byte screen_tile;
extern void far pascal copy_kid_to_char(void);
extern void far pascal load_actor_slot(sbyte slot);
extern sbyte far pascal GetKidIdxAsOpp(void);
extern sbyte far pascal register_actor_room(void);
extern void far pascal decrement_life_points(sbyte frames);
extern void far pascal ChangeMeters(void);
extern void far pascal CheckMeters(void);

typedef struct {
    byte data[6];
    byte flags;
} AnimationFrame;

typedef AnimationFrame AnimationFrameTable[];
extern AnimationFrameTable far hit_actions;

void far pascal FinlWatchKid(void)
{
    copy_kid_to_char();
    if (Char.room == 2) {
        if (player_right <= 305 && Char.action != 4)
            enter_princess_room();
    } else if (Char.character_kind == 1 && Char.sequence != 71) {
        sbyte room;

        load_actor_slot(GetKidIdxAsOpp());
        room = register_actor_room();
        if ((room == 7 || room == 8) && Level.environment_kind == 6 &&
            Kid.room == 5 && Level.environment_kind == 6 && Kid.column <= 4 &&
            (hit_actions[(int)Kid.frame].flags & 0x40)) {
            copy_kid_to_char();
            decrement_life_points(100);
            JumpSeq(71);
            Kid = Char;
            ChangeMeters(); CheckMeters();
        }
        copy_kid_to_char();
    }
    Kid = Char;
}

void far pascal FinlWatchOpps(void)
{
    sbyte i;
    sbyte count = Level.room_objects[Room.number - 1].opponent_count;

    if (count) {
        i = 0;
        if (count > 0)
            do {
                load_actor_slot(i);
                save_actor_slot();
            } while (count > ++i);
    }
    if (Room.number == 3 && Level.environment_kind == 6)
        check_new_opponent(count);
}

void far pascal run_jaffar_in_crystal(void);
void far pascal run_jaffar_in_escher(void);

void far pascal run_jaffar(void)
{
    if (Char.room == 6 && Level.environment_kind == 6) {
        run_jaffar_in_crystal();
        return;
    }
    run_jaffar_in_escher();
}

void far pascal dismiss_other_jaffars(void)
{
    CHARREC saved = Char;
    sbyte i = 0;
    sbyte count = Level.room_objects[(byte)Opp.room - 1].opponent_count;

    if (count > 0)
        do {
            load_actor_slot(i);
            if (Char.character_kind == 6 && Opp.slot != Char.slot && Char.frame == 15) {
                JumpSeq(240);
                save_actor_slot();
            }
        } while (count > ++i);
    Char = saved;
}

void far pascal cast_jaffar_spell(void)
{
    int sequence = -1;

    if (Char.frame == 15) {
        if ((Opp.x < Char.x && Char.direction != -1) ||
            (Opp.x >= Char.x && Char.direction != 0))
            sequence = 5;
        else
            sequence = 242;
    } else if (((Char.frame >= 1 && Char.frame <= 14) ||
                (Char.frame >= 49 && Char.frame <= 56)) &&
               Char.sequence != 13 && Char.sequence != 5) {
        sequence = 13;
    }
    if (sequence != -1)
        JumpSeq(sequence);
}

extern sbyte shadow_row;

void far pascal JaffarSetToNextPos(char near *p);
void far pascal JaffarSetToPrevPos(char near *record);
void far pascal send_jaffar_to_position(sbyte near *stance);
int far pascal shadow_in_range(sbyte position, sbyte direction);
void far pascal finish_casting_at_kid(void);
int far pascal jaffar_has_clear_shot(void);

void far pascal jaffar_attack_in_escher(void)
{
    int move = 1;
    sbyte near *stance;
    OpponentSlot near *raw;

    raw = GetOppInfoIdx((byte)Char.room, Char.slot);
    stance = raw->stance;
    if (raw->stance[0] != 2)
        stance[0] = 2;
    if (Char.sequence == 242) {
        if (Char.frame == 281)
            AddSound(62);
        else if (Char.frame == 282 && jaffar_has_clear_shot())
            finish_casting_at_kid();
        move = 0;
    } else if (stance[1] == stance[2]) {
        if (Opp.row == Char.row && jaffar_has_clear_shot())
            cast_jaffar_spell();
        if (shadow_in_range(stance[1], -1))
            JaffarSetToNextPos((char near *)stance);
        else
            JaffarSetToPrevPos((char near *)stance);
    }
    if (move && stance[1] != stance[2] && Opp.life_points > 0) {
        if (Char.row == 1 && jaffar_has_clear_shot()) {
            cast_jaffar_spell();
            return;
        }
        send_jaffar_to_position(stance);
    }
}

void far pascal jaffar_flee_in_escher(void)
{
    sbyte near *stance;
    OpponentSlot near *raw;

    raw = GetOppInfoIdx((byte)Char.room, Char.slot);
    stance = raw->stance;
    if (raw->stance[0] != 1)
        stance[0] = 1;
    if (stance[1] == stance[2]) {
        if (shadow_in_range(stance[1], -1))
            JaffarSetToPrevPos((char near *)stance);
        else if (shadow_in_range(stance[1], 1))
            JaffarSetToNextPos((char near *)stance);
    }
    if (stance[1] != stance[2])
        send_jaffar_to_position(stance);
}

int far pascal jaffar_has_clear_shot(void)
{
    int clear = 0;

    if (Opp.life_points > 0 && (Opp.room == 7 || Opp.room == 8) &&
        Level.environment_kind == 6 && Char.row == Opp.row &&
        (hit_actions[(int)Char.frame].flags & 0x40) && Char.frame != 109) {
        sbyte position = Char.column;

        if (Char.room == 8)
            position += 10;
        if (Char.row == 0) {
            clear = position <= 6 || position >= 13;
        } else if (Char.row == 1) {
            sbyte shadow = Opp.column;

            if (Opp.room == 8)
                shadow += 10;
            if (position <= 12) {
                if (shadow <= 12)
                    clear = 1;
            } else if (shadow >= 14) {
                clear = 1;
            }
        } else if (Char.row == 2) {
            clear = 1;
        }
    }
    return clear;
}

void far pascal JaffarSetToNextPos(char near *p)
{
    if (p[1] == 3)
        p[1] = 0;
    else
        ++p[1];
}

void far pascal JaffarSetToPrevPos(char near *record)
{
    if (record[1] != 0) {
        --record[1];
        return;
    }

    record[1] = 3;
}

typedef struct {
    sbyte room_x;
    byte flags;
    sbyte direction;
    int min_x, max_x;
    sbyte shadow_column[2];
    sbyte shadow_min[2];
    sbyte shadow_max[2];
} PositionRule;

static PositionRule position_rules[4] = {
    {17, 0, -1, 388, 400, {1, 0}, {15, 1}, {18, 18}},
    {14, 2, -1, 280, 304, {2, 1}, {0, 14}, {18, 18}},
    { 5, 2,  0, 312, 336, {1, 2}, {2, 0}, {9, 18}},
    { 6, 0,  0, 360, 368, {0, 1}, {1, 6}, {18, 10}}
};
extern const sbyte direction_step[];
static volatile int jaffar_motion_disabled = 0;
static byte blink_frame_order[6] = {0, 1, 2, 2, 1, 0};
extern void far pascal step_forward(int delta);
extern int far pascal jaffar_go_to_position_0(sbyte near *stance);
extern int far pascal jaffar_go_to_position_1(sbyte near *stance);
extern int far pascal jaffar_go_to_position_2(sbyte near *stance);
extern int far pascal jaffar_go_to_position_3(sbyte near *stance);

void far pascal send_jaffar_to_position(sbyte near *stance)
{
    if (!jaffar_motion_disabled) {
        int move;
        switch (stance[1]) {
        case 0:
            move = jaffar_go_to_position_0(stance);
            break;
        case 1:
            move = jaffar_go_to_position_1(stance);
            break;
        case 2:
            move = jaffar_go_to_position_2(stance);
            break;
        case 3:
            move = jaffar_go_to_position_3(stance);
            break;
        }
        if (move != -1)
            JumpSeq(move);
    }
}

int far pascal settle_jaffar_at_position(sbyte near *stance)
{
    int sequence = -1;

    if (Char.frame == 15) {
        if (stance[0] == 2 && jaffar_has_clear_shot()) {
            cast_jaffar_spell();
        } else {
            const PositionRule near *rule = &position_rules[stance[1]];
            int x = Char.x;

            if (rule->direction != Char.direction)
                x += direction_step[rule->direction] * 6;
            if (rule->room_x < 10) {
                if (Char.room == 8)
                    x += 320;
            } else if (Char.room == 7) {
                x -= 320;
            }
            if (x < rule->min_x) {
                if (Char.direction == 0)
                    step_forward(rule->min_x - x + 2);
                else
                    sequence = 5;
            } else if (x > rule->max_x) {
                if (Char.direction == -1)
                    step_forward(x - rule->min_x + 2);
                else
                    sequence = 5;
            } else if (rule->direction != Char.direction) {
                sequence = 5;
            } else {
                stance[2] = stance[1];
            }
        }
    }
    return sequence;
}

extern void far pascal standing_jump(void);
extern void far pascal jump_up(void);
extern void far pascal start_running(void);
extern void far pascal climb_up(void);
extern void far pascal actor_down(void);
extern void far pascal control_drop(void);
extern void far pascal run_jump(int mode);

int far pascal jaffar_go_to_position_0(sbyte near *stance)
{
    sbyte x = Char.column + ((Char.room == 8) ? 1 : 0) * 10;
    int sequence = -1;
    const PositionRule near *rule = &position_rules[stance[1]];

    if (stance[2] == 3) {
        if (Char.frame == 15) {
            if (x == 6)
                standing_jump();
            else if (x > 7 && x < rule->room_x - 1)
                start_running();
            else
                goto settle;
        } else if (Char.frame == 7 || Char.frame == 11) {
            if ((x >= 5 && x <= 6) || (x >= 10 && x <= 11))
                run_jump(4);
            else if (x >= rule->room_x - 1)
                sequence = 13;
        }
    } else if (Char.row == 2) {
        if (Char.frame == 15) {
            if (Char.direction != 0)
                goto turn;
            jump_up();
        } else if (Char.frame == 91)
            climb_up();
    } else if (Char.row == 1) {
        if (Char.frame == 15) {
            if (Char.direction == 0) {
                if (x < 17)
                    start_running();
                else
                    goto turn;
            } else if (x >= 17)
                jump_up();
            else {
turn:
                sequence = 5;
            }
        } else if (Char.frame == 7 && x >= 17)
            sequence = 13;
        else if (Char.frame == 91)
            climb_up();
        else if (Char.frame == 109)
            sequence = 49;
    } else {
settle:
        sequence = settle_jaffar_at_position(stance);
    }
    return sequence;
}

int far pascal jaffar_go_to_position_1(sbyte near *stance)
{
    int sequence = -1; const PositionRule near *rule = &position_rules[stance[1]];

    if (stance[2] < stance[1]) {
        if (Char.row == 0) {
            if (Char.direction == 0 && Char.sequence != 5)
                sequence = 5;
            else if (Char.frame == 15)
                actor_down();
        } else if (Char.row == 1) {
            if (Char.frame >= 87 && Char.frame <= 91) {
                control_drop();
            } else if (Char.frame == 15) {
                if (Char.direction != -1)
                    sequence = 5;
                else
                    start_running();
            } else if (Char.frame == 109) {
                sequence = 49;
            }
        } else if (Char.row == 2) {
            if (Char.frame != 109)
                sequence = settle_jaffar_at_position(stance);
            else
                sequence = 49;
        }
    } else {
        sbyte x = Char.column + ((Char.room == 8) ? 1 : 0) * 10;

        if (x == position_rules[2].room_x && Char.frame == 15)
            start_running();
        else if (x >= 6 && x <= 7) {
            run_jump(4);
        } else if (x > 10) {
            if (Char.frame == 7 && x >= rule->room_x - 1)
                sequence = 13;
            else
                sequence = settle_jaffar_at_position(stance);
        }
    }
    return sequence;
}

int far pascal jaffar_go_to_position_2(sbyte near *stance)
{
    int sequence = -1;

    if (stance[2] < stance[1]) {
        sbyte x = Char.column + ((Char.room == 8) ? 1 : 0) * 10;

        if (x == position_rules[1].room_x && Char.frame == 15) {
            start_running();
        } else if (x >= 12 && x <= 13 && Char.frame == 11) {
            run_jump(4);
        } else if (x <= 6 && Char.frame == 7) {
            sequence = 13;
        } else {
            goto settle;
        }
    } else if (Char.row == 0) {
        if (Char.frame == 15) {
            if (Char.direction == 0)
                sequence = 5;
            else
                actor_down();
        }
    } else if (Char.row == 1) {
        if (Char.frame >= 87 && Char.frame <= 91) {
            control_drop();
        } else if (Char.frame == 15) {
            if (Char.direction == -1)
                start_running();
            else
                sequence = 5;
        } else if (Char.frame == 109) {
            sequence = 49;
        }
    } else if (Char.row == 2) {
        if (Char.frame == 109)
            sequence = 49;
        else {
settle:
            sequence = settle_jaffar_at_position(stance);
        }
    }
    return sequence;
}

int far pascal jaffar_go_to_position_3(sbyte near *stance)
{
    int sequence = -1;
    const PositionRule near *rule = &position_rules[stance[1]];
    sbyte x = Char.column + ((Char.room == 8) ? 1 : 0) * 10;

    if (stance[2] == 0) {
        if (Char.frame == 15) {
            if (x > 10) start_running();
            else if (x <= rule->room_x)
                sequence = settle_jaffar_at_position(stance);
        } else if (Char.frame == 7 || Char.frame == 11) {
            if (x <= rule->room_x) sequence = 13;
            else if ((x >= 14 && x <= 15) || (x >= 9 && x <= 10))
                run_jump(4);
        }
    } else if (Char.row == 2) {
        if (Char.frame == 15) {
            jump_up();
        } else if (Char.frame == 91) {
            climb_up();
        }
    } else if (Char.row == 1) {
        if (Char.frame == 15) {
            if (Char.direction != 0) jump_up();
            else if (x == 6) step_forward(32);
            else
                sequence = 5;
        } else {
            if (Char.frame == 91) climb_up();
            else if (Char.frame == 109) sequence = 49;
        }
    } else {
        sequence = settle_jaffar_at_position(stance);
    }
    return sequence;
}

extern void far pascal load_shadow_with_opponent(sbyte slot);
extern void far pascal run_player_script(sbyte script);

void far pascal finish_casting_at_kid(void)
{
    sbyte slot;

    JumpSeq(244);
    slot = Char.slot;
    save_actor_slot();
    copy_kid_to_char();
    JumpSeq(71);
    AddSound(14);
    decrement_life_points(100);
    run_player_script(17);
    Kid = Char;
    load_shadow_with_opponent(slot);
}

extern int battle_progress;
extern int escape_counter;

extern int far pascal get_opp_dist(void);
extern int far pascal AddCharX(int x);
extern sbyte far pascal index_cell(sbyte column, sbyte row);
extern int abs(int x);
#pragma intrinsic(abs)

void far pascal run_jaffar_in_crystal(void)
{
    if (Char.alive < 0) {
        sbyte count = Level.room_objects[(byte)Char.room - 1].opponent_count;

        if (Opp.character_kind == 1)
            --count;
        if (count == 1 && Char.sequence != 240 && Kid.alive < 0) {
            JumpSeq(240);
            return;
        }
        if (Opp.direction == Char.direction) {
            if ((*(volatile byte *)&Char.stance) == 0 && Kid.alive < 0) {
                int range = get_opp_dist() - 15;

                if (range > 18 && range < 48) {
                    Char.x = AddCharX(15);
                    (*(volatile byte *)&Char.stance) = 1;
                    JumpSeq(75);
                    return;
                }
            } else if (Char.frame == 171) {
                (*(volatile byte *)&Char.stance) = 0;
                JumpSeq(92);
                return;
            } else if (Char.frame == 52 && Kid.alive > 0) {
                JumpSeq(197);
                AddSound(262);
            }
        }
    }
}

void far pascal run_jaffar_in_escher(void)
{
    int flee;
    int attack;
    sbyte slot;
    OpponentSlot near *opponent;
    sbyte near *stance;

    opponent = GetOppInfoIdx((byte)Char.room, Char.slot);
    stance = opponent->stance;
    if (opponent->stance[0] == 1 && stance[2] != stance[1])
        flee = 1;
    else
        flee = 0;

    attack = 0;
    if (Opp.character_kind == 0) {
        if (Opp.stance == 1 &&
            !(Char.frame >= 207 && Char.frame <= 210) &&
            Char.frame != 158) {
            if (abs(get_opp_dist()) < 62) {
                slot = Char.slot;
                save_actor_slot();
                copy_kid_to_char();
                JumpSeq(241);
                AddSong(265);
                (*(volatile byte *)&Char.stance) = (byte)-1;
                Kid = Char;
                load_actor_slot(slot);
            }
        } else if (Opp.row == Char.row && battle_progress >= 4) {
            if (Opp.life_points <= 2 && Opp.sequence != 242)
                flee = 0;
            else
                flee = 1;
        } else if (Opp.life_points <= 2 && Opp.stance == -1 && Opp.sequence != 242) {
            attack = 1;
        } else {
            attack = 0;
        }
    } else {
        if (Opp.life_points <= 2 && Opp.sequence != 242)
            flee = 0;
        else
            flee = 1;
        if (Opp.life_points <= 2 && Opp.sequence != 242)
            attack = 1;
        else
            attack = 0;
    }

    if (Char.sequence == 243 && Char.frame == 351)
        ++escape_counter;
    if (flee) {
        jaffar_flee_in_escher();
        return;
    }
    if (attack)
        jaffar_attack_in_escher();
}

void far pascal update_jaffar_slot(OpponentSlot near *opponent)
{
    sbyte near *stance = opponent->stance;

    if (!((CutRoom == 7 || CutRoom == 8) && Level.environment_kind == 6) &&
        stance[2] != stance[1]) {
        const PositionRule near *rule;

        stance[2] = stance[1];
        rule = &position_rules[stance[1]];
        Char.row = rule->flags;
        Char.column = rule->room_x;
        Char.x = rule->min_x + 6;
        if (Char.room == 8) {
            Char.x -= 320;
            Char.column -= 10;
        }
        Char.direction = rule->direction;
        reset_char_y();
        JumpSeq(2);
        AnimChar();
        opponent->cell = index_cell(Char.column, Char.row);
        opponent->x = Char.x;
        opponent->direction = Char.direction;
        *(int near *)((byte near *)opponent + 7) = Char.sequence_position;
        *(int near *)((byte near *)opponent + 5) = Char.sequence_id;
    }
    *(int near *)(stance + 4) = Char.y;
}

int far pascal shadow_in_range(sbyte position, sbyte direction)
{
    const PositionRule near *rule = &position_rules[position];
    int side;
    int inside;
    sbyte x;
    const sbyte near *side_fields;

    direction = (direction < 0) ? 0 : 1;
    side = direction;
    side_fields = (const sbyte near *)rule + side;
    if (side_fields[7] == Opp.row &&
        (x = Opp.column + ((Opp.room == 8) ? 1 : 0) * 10) >= side_fields[9] &&
        x <= side_fields[11])
        inside = 1;
    else
        inside = 0;
    return inside;
}

extern int playing_scene;
extern unsigned int current_sound_bank;
extern unsigned int Graf;
extern byte GamePlayRect;
extern void far pascal update_opponent_record(void);
extern void far pascal SET_RECORD_RANGE(void far *work, int resource);
extern void far pascal STROKE_PORT_RECT(void far *work);

void far pascal transfer_jaffar_to_escher2(void)
{
    Char.column = 17;
    Char.x = 712;
    Char.row = -3;
    Char.y = -133;
    Char.direction = position_rules[0].direction;
    JumpSeq(2);
    AnimChar();
    update_opponent_record();
}

void far pascal load_tower_top(void)
{
    if (!playing_scene)
        fade_scene(3500, 64, 192, 0);
    if (current_sound_bank)
        Graf = current_sound_bank;
    SET_RECORD_RANGE(&GamePlayRect, 139);
}

void far pascal load_chess(void)
{
    fade_scene(3500, 64, 192, 1);
    fade_scene(1000, 240, 16, 0);
    if (Room.number == 4 && Level.environment_kind == 6) {
        unsigned int previous = Graf;

        Graf = current_sound_bank;
        STROKE_PORT_RECT(&GamePlayRect);
        Graf = previous;
        AddSong(268);
    } else if (Room.number == 3 && Level.environment_kind == 6) {
        fade_scene(3000, 224, 16, 0);
        AddSong(269);
    }
}

typedef struct {
    int shape;
    sbyte x_offset, y_offset;
} SmokeFrame;

typedef struct {
    int image, equipment_image;
    sbyte x, y;
    byte flags;
} FRAMEREC;

extern FRAMEREC Frame;
extern unsigned int sprite_table_handle;
extern SmokeFrame far *far pascal RESOLVE_HANDLE_TO_FAR_PAYLOAD(unsigned int handle);
extern int far pascal offset_sprite_x(int x);
extern void far pascal MakeFCharClipRect(void);
extern void far _fastcall add_char_object(byte style);
typedef void (far pascal *AnimationCallback)(sbyte cell, void near *clip);
extern void far pascal mark_foreground(sbyte cell, void near *clip);
extern void far pascal mark_bufs_for_rect(AnimationCallback callback, void near *state, sbyte slot);

void far pascal setup_guard_smoke(void)
{
    SmokeFrame far *table;
    unsigned int index;

    if (Char.character_kind == 2 && Char.frame >= 229 && Char.frame <= 254) {
        index = (unsigned int)Frame.equipment_image;
        table = RESOLVE_HANDLE_TO_FAR_PAYLOAD(sprite_table_handle);
        if (table[index].shape != -1) {
            FChar.image = table[index].shape;
            offset_sprite_x(table[index].x_offset);
            FChar.y += table[index].y_offset;
            FChar.image_set = 3;
            MakeFCharClipRect();
            add_char_object(9);
            mark_bufs_for_rect(mark_foreground, &FChar.clip_top, Char.slot);
        }
    }
}

typedef struct {
    int xmin, ymin, xmax, ymax;
} Rect;

typedef struct {
    byte type;
    unsigned long shape_list;
    byte layer;
    byte attributes[5];
    Rect bounds;
    byte tail[6];
} FinalImage;

typedef struct {
    byte attributes[28];
    FinalImage images[1];
} RoomImageTable;

extern RoomImageTable near *room_image_table;
/* Each frame selects a room image slot. Each shape pair packs two word offsets. */
static byte crystal_frame_slot[] = {
    0, 1, 2, 3, 2, 1, 0, 0
};
static unsigned long crystal_shapes[] = {
    0x0136000aUL, 0x002f0048UL, 0x008a0054UL,
    0x00ce0041UL, 0x00eb0054UL, 0x000e0082UL,
    0x00340088UL, 0x008000a4UL, 0x010b009bUL
};
static byte crystal_layer[] = {
    1, 0, 0, 0, 0, 1, 1, 1, 1, 0
};
static byte gargoyle_frame_slot[] = {
    0, 1, 2, 4, 3, 4, 2, 1, 0, 0
};
static unsigned long gargoyle_shapes[] = {
    0x00240043UL, 0x003f0043UL, 0x005e0043UL,
    0x007b0043UL, 0x009a0043UL, 0x00710083UL,
    0xffff0044UL, 0x0019007eUL, 0x00d60083UL
};
static byte gargoyle_layer[] = {
    0, 0, 0, 0, 0, 1, 0, 1, 1, 0
};
extern void far pascal OFFSETRECT(Rect far *output, Rect far *input, int dx, int dy);
extern void far _fastcall draw_custom_room_image(byte index, byte layer);

extern unsigned int BlueSpec;
extern int blink_always;
extern byte final_object_visibility;
extern int fade_active;
static byte crystal_serial;
static byte gargoyle_serial;
extern unsigned int (*near BlueS)[2];

extern void far pascal add_trob(sbyte row, sbyte col, byte value, byte effect);
extern void far pascal mark_move(sbyte cell, void near *clip);

#define ROOM_RECT(offset) ((Rect near *)((byte near *)room_image_table + (offset)))

void far pascal animate_jaffar_blink(void)
{
    unsigned int frame;

    ++BlueSpec;
    frame = BlueSpec & 15;
    if (frame >= 5 && room_image_table->attributes[1] == 26) {
        frame -= 5;
        if (frame < 6) {
            mark_bufs_for_rect(mark_move, ROOM_RECT(89), -1);
            return;
        }
        final_object_visibility = (byte)-1;
        BlueSpec = 0;
    }
}

void far pascal draw_jaffar_blink(FinalObject near *object)
{
    unsigned int frame = object->animation & 15;

    if ((blink_always || frame >= 5) && object->kind == 11) {
        if (!blink_always)
            frame -= 5;
        if (frame < 6)
            draw_custom_room_image(blink_frame_order[frame] + 2, 11);
    }
}

void far pascal animate_spider(void)
{
    if (room_image_table->attributes[1] == 25) {
        unsigned int state = BlueSpec;
        int ticks = (state & 0x7f8) >> 3;

        if (state & 0x800) {
            state = Rnd(5);
            if (--ticks > 0 && !blink_always) {
                state |= 0x800;
            } else {
                ticks = Rnd(60) + 30;
            }
            mark_bufs_for_rect(mark_move, ROOM_RECT(189), -1);
        } else if (--ticks <= 0) {
            if (!fade_active) {
                ticks = Rnd(2) + 1;
                state |= 0x800;
            } else {
                ticks = Rnd(60) + 30;
            }
        }
        state &= 0xf807;
        BlueSpec = (ticks << 3) | state;
        return;
    }
    final_object_visibility = (byte)-1;
}

extern int sprite_vertical_origin;

void far pascal draw_spider(FinalObject near *object)
{
    if (object->kind == 11) {
        int previous = sprite_vertical_origin;

        sprite_vertical_origin -= 6;
        draw_custom_room_image((object->animation & 7) + 6, 11);
        sprite_vertical_origin = previous;
    }
}

void far pascal setup_crystal_room(void)
{
    fade_scene(3500, 64, 192, 2);
    fade_scene(1000, 240, 16, 0);
    fade_scene(25001, 32, 16, 7);
    crystal_serial = 0;
    if (Room.number == 7) {
        fade_scene(3000, 224, 16, gargoyle_serial = 0);
        AddSong(263);
    } else if (Room.number == 8) {
        gargoyle_serial = 6;
        AddSong(263);
    } else {
        AddSong(270);
    }
}

void far pascal animate_crystal(void)
{
    if (room_image_table->attributes[1] == 28) {
        unsigned int state = BlueSpec; if (state & 0x800) {
            unsigned int frame = ((state & 0x7f0) >> 4) + 1;

            if (frame < 7) {
                Rect bounds;
                const int near *shape;

                state &= 0xf80f; state |= frame << 4;
                shape = (const int near *)&crystal_shapes[state & 15]; OFFSETRECT(&bounds, &room_image_table->images[4].bounds, shape[1], shape[0]);
                mark_bufs_for_rect(mark_move, &bounds, -1);
            } else {
                state &= 0xf80f; state |= (Rnd(40) + 40) << 4; state &= ~0x800;
            }
        } else {
            int ticks = (state & 0x7f0) >> 4; state &= 0xf80f; if (--ticks <= 0) {
                if (fade_active && trob_count)
                    state |= (Rnd(40) + 40) << 4;
                else
                    state |= 0x800;
            } else {
                state |= ticks << 4;
            }
        }
        BlueSpec = state | 0x1000;
        return;
    }
    final_object_visibility = (byte)-1;
}

void far pascal draw_crystal(FinalObject near *object)
{
    int frame;
    byte index, shape;
    FinalImage near *image;
    Rect bounds;

    if (object->kind != 1 || !(object->animation & 0x800))
        return;
    frame = (short)(object->animation & 0x7f0) >> 4;
    if (--frame < 0)
        return;
    index = crystal_frame_slot[frame] + 4;
    image = &room_image_table->images[index];
    bounds = image->bounds;
    shape = object->animation & 15;
    image->shape_list = crystal_shapes[shape];
    OFFSETRECT(&image->bounds, &image->bounds, ((volatile int near *)&image->shape_list)[1], ((volatile int near *)&image->shape_list)[0]);
    image->layer = crystal_layer[shape];
    draw_custom_room_image(index, crystal_layer[shape]);
    image->layer = 11;
    image->bounds = bounds;
    ((unsigned int near *)&image->shape_list)[0] = 0;
    ((unsigned int near *)&image->shape_list)[1] = 0;
}

void far pascal trigger_crystal(sbyte row, sbyte col)
{
    unsigned int serial = crystal_serial++;
    unsigned int ticks = Rnd(40);

    BlueS[col][0] = ((ticks | 0x100) << 4) | serial;
    add_trob(row, col, 1, 41);
}

void far pascal animate_gargoyle(void)
{
    if (room_image_table->attributes[1] == 29 || room_image_table->attributes[1] == 30) {
        unsigned int state = BlueSpec;
        byte serial = (byte)(state & 15);

        if ((serial < 6 && Room.number == 7) || (serial >= 6 && Room.number == 8)) {
            if (state & 0x800) {
                unsigned int frame = ((state & 0x7f0) >> 4) + 1;

                if (frame < 9) {
                    Rect bounds;
                    const int near *shape;

                    state &= 0xf80f;
                    state |= frame << 4;
                    shape = (const int near *)&gargoyle_shapes[serial];
                    OFFSETRECT(&bounds, &room_image_table->images[2].bounds, shape[1], shape[0]);
                    mark_bufs_for_rect(mark_foreground, &bounds, -1);
                } else {
                    state &= 0xf80f;
                    state |= (Rnd(40) + 30) << 4;
                    state &= ~0x800;
                }
            } else {
                int ticks = (state & 0x7f0) >> 4;

                state &= 0xf80f;
                if (--ticks <= 0) {
                    if (fade_active && trob_count)
                        state |= (Rnd(40) + 30) << 4;
                    else
                        state |= 0x800;
                } else {
                    state |= ticks << 4;
                }
            }
            BlueSpec = state | 0x1000;
            return;
        }
    }
    final_object_visibility = (byte)-1;
}

void far pascal draw_gargoyle(FinalObject near *object)
{
    int frame;
    byte index, shape;
    FinalImage near *image;
    Rect bounds;

    if (object->kind != 1 || !(object->animation & 0x800))
        return;
    frame = (short)(object->animation & 0x7f0) >> 4;
    if (--frame < 0)
        return;
    index = gargoyle_frame_slot[frame] + 2;
    image = &room_image_table->images[index];
    bounds = image->bounds;
    shape = object->animation & 15;
    image->shape_list = gargoyle_shapes[shape];
    OFFSETRECT(&image->bounds, &image->bounds, ((volatile int near *)&image->shape_list)[1], ((volatile int near *)&image->shape_list)[0]);
    image->layer = gargoyle_layer[shape];
    draw_custom_room_image(index, gargoyle_layer[shape]);
    image->layer = 11;
    image->bounds = bounds;
    ((unsigned int near *)&image->shape_list)[0] = 0;
    ((unsigned int near *)&image->shape_list)[1] = 0;
}

void far pascal trigger_gargoyle(sbyte row, sbyte col)
{
    unsigned int serial = gargoyle_serial++;
    unsigned int ticks = Rnd(40);

    BlueS[col][0] = ((ticks | 0x100) << 4) | serial;
    add_trob(row, col, 1, 42);
}

extern MOBREC Mob;
extern struct {
    int heights[13], widths[13];
} mob_dimensions;
#define fireball_half_width mob_dimensions.widths[12]
#define fireball_height mob_dimensions.heights[12]
extern int player_left;
extern unsigned int kid_shapes;
extern int object_counter;

extern byte far pascal GetRow(int y);
extern byte far pascal IndexCell1(sbyte col, byte row);
extern void far *far pascal get_set_image_size(unsigned int sheet, unsigned int image, int far *size);
extern void far pascal add_mobile_sprite(int y);
extern void far pascal mark_mob_redraw(byte changed);
extern int far pascal IsWall(sbyte tile);
extern void far pascal dispatch_sound(unsigned int image);
extern void far pascal add_mob(void);
extern void far _fastcall add_middle_image(byte layer, int frame, int x, int y, byte flags);

typedef struct {
    int x;
    byte unknown0[2];
    byte room;
    byte unknown1[2];
    int velocity;
    byte unknown2;
    sbyte row;
    unsigned int flags;
} FireballSlot;

typedef struct {
    byte coordinates[10];
    unsigned int flags;
    byte mode;
    byte reserved[7];
} FireballQueueItem;

extern FireballSlot near *far pascal get_mob_type_item(byte kind, int index);
extern _segment fireball_queue_segment;
extern FireballQueueItem _based(fireball_queue_segment) fireball_queue[];
int far _fastcall get_fireball_image(unsigned int state);
void far pascal trigger_fireball(void);

void far pascal update_fireball_view(void)
{
    int visible = 1;
    volatile int y = Mob.y;

    if ((*(volatile byte near *)&Mob.room) == Room.number) {
        if (Mob.x > 449 || Mob.x < 0)
            visible = 0;
    } else if (Room.left == (*(volatile byte near *)&Mob.room)) {
        if (Mob.x + fireball_half_width > 320) {
            Mob.x -= 320;
            Mob.room = Room.number;
        } else {
            visible = 0;
        }
    } else if (Room.right == (*(volatile byte near *)&Mob.room)) {
        if (Mob.x - fireball_half_width < 320) {
            Mob.x += 320;
            Mob.room = Room.number;
        } else {
            visible = 0;
        }
    } else {
        visible = 0;
    }

    if (visible) {
        volatile sbyte col;
        unsigned int image;
        int size[2];

        FChar.index = IndexCell1((col = Mob.x / 32), GetRow(y));
        image = get_fireball_image(Mob.flags);
        if (get_set_image_size(kid_shapes, image, size)) {
            int x = Mob.x;
            int saved_y;
            Mob.x -= size[1] / 2;
            saved_y = y;
            Mob.y = saved_y + size[0] / 2;
            fireball_height = *(volatile int near *)&size[0];
            fireball_half_width = size[1];
            add_mobile_sprite(Mob.y);
            mark_mob_redraw(1);
            Mob.y = saved_y;
            Mob.x = x;
        }
    }
}

void far pascal check_fireball_hit_character(void)
{
    if (Char.character_kind == 6 && Char.alive < 0 && Char.sequence != 243) {
        int index = 1;
        FireballSlot near *ball;

        do {
            int offset;

            ball = get_mob_type_item(12, index);
            if (ball && ball->row == Char.row && !(ball->flags & 8)) {
                if (ball->room == (byte)Char.room)
                    offset = 0;
                else if (Char.room == 8 && Level.environment_kind == 6)
                    offset = -320;
                else
                    offset = 320;
                offset += ball->x + ball->velocity + 113;
                if (offset + 34 > player_left && offset < player_right) {
                    ball->flags = 8;
                    ball->x = ball->velocity > 0 ? player_left - 125 : player_right - 135;
                    AddSound(62);
                    decrement_life_points(100);
                    JumpSeq(243);
                    AnimChar();
                    AddSong(267);
                }
            }
            ++index;
        } while (ball);
    }
}

void far pascal check_fireball_hit_wall(void)
{
    int x = Mob.x + fireball_half_width - 14;
    sbyte col;

    if ((*(volatile int near *)&Mob.auxiliary) < 0)
        x -= 2 * fireball_half_width;
    col = x / 32;
    if (x < 0)
        --col;
    if (IsWall(ReadCell(Mob.room, col, Mob.row))) {
        Mob.x = col * 32 + 14;
        if ((*(volatile int near *)&Mob.auxiliary) < 0)
            Mob.x += 32;
        Mob.flags = 8;
        AddSound(259);
        dispatch_sound(0x281a);
    }
}

int far pascal do_throw_fireball(void)
{
    register int sequence;
    if (Char.life_points > 2) {
        sequence = 242;

        decrement_life_points(2);
        AddSong(266);
    } else {
        sequence = -1;
    }
    return sequence;
}

void far pascal draw_fireball_object(FinalObject near *object)
{
    int image = get_fireball_image(FChar.image);

    add_middle_image(2, image + 1, FChar.x, FChar.y, 10);
    fireball_queue[object_counter].flags = 8;
    fireball_queue[object_counter].mode = 0;
}

int far _fastcall get_fireball_image(unsigned int state)
{
    unsigned int frame = state & 7;
    int number;

    if (state & 8) {
        number = frame;
        number += 308;
    } else {
        number = frame;
        number += 304;
    }
    return number;
}

void far pascal move_fireball(void)
{
    if (Mob.velocity >= 0) {
        unsigned int frame = Mob.flags & 7;

        if (Mob.flags & 8) {
            if (frame >= 5)
                Mob.velocity = -1;
            else
                ++frame;
        } else {
            if ((*(volatile int near *)&Mob.auxiliary) > 0) {
                if ((*(volatile int near *)&Mob.auxiliary) < 16)
                    (*(volatile int near *)&Mob.auxiliary) += 2;
            } else if ((*(volatile int near *)&Mob.auxiliary) > -16) {
                (*(volatile int near *)&Mob.auxiliary) -= 2;
            }
            Mob.x += (*(volatile int near *)&Mob.auxiliary);
            if (Mob.room == 0) {
                Mob.velocity = -1;
                dispatch_sound(0x2812);
            } else {
                check_fireball_hit_wall();
                if (!(Mob.flags & 8)) {
                    if (frame >= 3)
                        frame = 0;
                    else
                        ++frame;
                    AddSound(258);
                }
            }
        }
        if (Mob.velocity == -1) {
            if (Kid.life_points <= 2) {
                fade_scene(2000, 48, 16, 0);
                return;
            }
        } else {
            Mob.flags &= ~7;
            Mob.flags |= frame;
        }
    }
}

void far pascal throwing_fireball(void)
{
    if (Char.frame == 281)
        trigger_fireball();
    AddSound(258);
}

void far pascal trigger_fireball(void)
{
    int y;

    Mob.x = AddCharX(27) - 130;
    Mob.y = y = Char.y - 29;
    Mob.room = (byte)Char.room;
    Mob.velocity = 0;
    (*(volatile int near *)&Mob.auxiliary) = direction_step[Char.direction] * 8;
    Mob.kind = 12;
    Mob.row = GetRow(y);
    Mob.flags = 2;
    check_fireball_hit_wall();
    add_mob();
    AddSound(258);
}

extern void far pascal draw_flask(FinalObject near *);

typedef void (far pascal *LevelTileProc)(FinalObject near *);
LevelTileProc final_level_spec[] = {
    draw_final_object, draw_final_object, 0, 0, 0,
    0, 0, 0, 0, 0,
    draw_flask, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    draw_final_object, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, draw_jaffar_blink, 0, 0, 0,
    0, 0, 0, 0, 0,
    draw_spider, draw_crystal, draw_gargoyle
};
