typedef unsigned char byte;
typedef signed char sbyte;

typedef struct {
    sbyte cell;
    byte room;
    sbyte state;
    byte kind;
} Trob;

extern int trob_count;
extern Trob current_trob;
extern Trob trobs[20];

static sbyte exit_close_speed[] = {5, 17, 99, 0};
static sbyte gate_close_speed[] = {0, 0, 0, 20, 40, 60, 80, 100, 120, 0};
static sbyte gate_open_speed[] = {-1, -4, 4, 4};

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
    signed char cell_kinds[32][30];                  /* 0000 */
    LEVELCELL cell_info[32][30];                       /* 03c0 */
    struct {
        byte room;
        sbyte cell;
        byte timer, prev, next;
    } tile_links[256];                                 /* 12c0 */
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
typedef struct {
    unsigned state;
    unsigned extra;
} RoomObject;

extern RoomObject *BlueS;
extern RoomObject BlueSpec;
extern byte *room_kinds;
extern byte drawn_tile_kind;

#define TrobStateOffset ((unsigned)&((Trob *)0)->state)

void AnimTrob(int index);

void AnimTrobs(void)
{
    int i;
    /* This work word holds the removal flag until compaction starts. */
    union {
        int removed;
        Trob *next;
    } pass;
    struct {
        int kept;
    } compact;

    if (trob_count) {
        pass.removed = 0;
        for (i = 0; i < trob_count; ++i) {
            current_trob = trobs[i];
            AnimTrob(i);
            if ((trobs[i].state = current_trob.state) < 0)
                pass.removed = 1;
        }
        if (pass.removed) {
            compact.kept = i = 0;
            if (i < trob_count) {
                sbyte *state = &trobs[0].state;
                pass.next = trobs;
                do {
                    if (*state >= 0) {
                        Trob *dst = pass.next;
                        ++*(Trob * volatile *)&pass.next;
                        ++compact.kept;
                        *dst = *(Trob *)(state - TrobStateOffset);
                    }
                    state += sizeof(Trob);
                } while (++i < *(volatile int *)&trob_count);
            }
            trob_count = compact.kept;
        }
    }
}

extern void select_room_arrays(byte room);
void select_trob_cell(int cell);
void animate_flask(void);
void animate_loose_floor(void);
void animate_exit(void);
void animate_torch(int index);
void animate_gate(void);
void animate_plate(void);
extern void animate_desert_exit(void);
extern void AnimScythe(void);
extern void animate_lava(void);
extern void animate_cave_bars(void);
extern void animate_desert_rocks(void);
extern void animate_desert_planks(void);
extern void animate_desert_stone(void);
extern void animate_jaffar_blink(void);
extern void animate_dart(void);
extern void AnimRoofPillar(void);
extern void animate_roof_ship(void);
extern void animate_roof_glass(void);
extern void animate_spider(void);
extern void animate_crystal(void);
extern void animate_gargoyle(void);
extern void animate_sacred_flame(void);
extern void animate_bridge_floor(void);

/* Draft. External provider names and same-file prototypes remain provisional. */
void AnimTrob(int index)
{
    select_room_arrays(current_trob.room);
    select_trob_cell(current_trob.cell);
    switch (current_trob.kind) {
    default:
        current_trob.state = -1;
        break;
    case 4:
        if (Level.environment_kind == 1)
            animate_desert_exit();
        else
            animate_gate();
        break;
    case 5:
    case 6:
    case 34:
        animate_plate();
        break;
    case 10:
        animate_flask();
        break;
    case 11:
        animate_loose_floor();
        break;
    case 13:
        AnimScythe();
        break;
    case 17:
        animate_exit();
        break;
    case 19:
    case 32:
        animate_torch(index);
        break;
    case 23:
        animate_lava();
        break;
    case 27:
        animate_cave_bars();
        break;
    case 28:
        animate_desert_rocks();
        break;
    case 29:
        animate_desert_planks();
        break;
    case 30:
        animate_desert_stone();
        break;
    case 31:
        animate_jaffar_blink();
        break;
    case 36:
        animate_dart();
        break;
    case 37:
        AnimRoofPillar();
        break;
    case 38:
        animate_roof_ship();
        break;
    case 39:
        animate_roof_glass();
        break;
    case 40:
        animate_spider();
        break;
    case 41:
        animate_crystal();
        break;
    case 42:
        animate_gargoyle();
        break;
    case 43:
        animate_sacred_flame();
        break;
    case 44:
        animate_bridge_floor();
        break;
    }
    BlueS[current_trob.cell].state = BlueSpec.state;
}

typedef struct {
    int edges[4];
} MoveRect;

typedef void (far *CellMark)(sbyte cell, MoveRect *rect);

static MoveRect temple_gate_rect = {{-11, 17, 63, 57}};
static MoveRect gate_rect = {{-11, 32, 63, 57}};
static MoveRect exit_rect_templates[] = {
    {{1, 6, 51, 46}}, {{-5, 6, 51, 49}}, {{-5, 6, 51, 50}}
};
static MoveRect flask_rect = {{30, 18, 52, 31}};
static MoveRect loose_rect_templates[] = {
    {{52, 0, 72, 50}}, {{48, 0, 69, 57}}
};
static MoveRect plate_rect = {{49, 0, 67, 57}};
static MoveRect torch_rect = {{3, 32, 31, 51}};
static MoveRect cave_torch_rect = {{-4, 25, 18, 57}};
static MoveRect temple_plate_rect = {{51, 11, 63, 43}};
static MoveRect clip_char_rect_templates[] = {
    {{9, -32, 192, 46}}, {{6, -32, 192, 48}}, {{6, -32, 192, 50}}
};

static MoveRect *loose_rects[5] = {
    0, 0, &loose_rect_templates[1], &loose_rect_templates[0], &loose_rect_templates[1]
};
static MoveRect *exit_rects[5] = {
    0, 0, &exit_rect_templates[2], &exit_rect_templates[0], &exit_rect_templates[1]
};
/* ClipChar in CTRL.C selects its template by level type. */
MoveRect *clip_char_rects[5] = {
    0, 0, &clip_char_rect_templates[2], &clip_char_rect_templates[0], &clip_char_rect_templates[1]
};

#pragma pack(1)
typedef struct {
    unsigned char number;                             /* 00 */
    unsigned char left, right, above, below;           /* 01-04 */
    unsigned char above_left, above_right;             /* 05-06 */
    unsigned char below_left, below_right;             /* 07-08 */
    unsigned char custom;                             /* 09 */
} ROOMREC;
#pragma pack()
extern int IS_SIX_KIND(byte);
extern sbyte CellToCol(byte room, int cell);
extern ROOMREC Room;
extern sbyte CellToRow(byte room, int cell);
extern MoveRect *ComputeOffsetCellRect(MoveRect *out, MoveRect *base, int col, int row);
MoveRect *compute_trob_move_rect(MoveRect *out, MoveRect *base);
sbyte check_trob(void);
sbyte check_trob_left(void);
sbyte check_trob_right(void);
sbyte check_trob_above(void);
sbyte check_trob_above_right(void);
sbyte check_trob_below(void);
void mark_move(sbyte cell, MoveRect *rect);
void mark_bufs_for_rect(CellMark mark, MoveRect *rect, sbyte selector);
int is_visible_room_cell(byte room, sbyte cell);

void redraw_torch(void)
{
    struct { MoveRect rect; union { int cave; struct { byte pad; sbyte cell; } bytes; } tail; } work;
    work.tail.cave = (Level.environment_kind == 2);

    work.rect = *(work.tail.cave ? &cave_torch_rect : &torch_rect);
    if (compute_trob_move_rect(&work.rect, &work.rect)) {
        if (work.tail.cave) {
            mark_move(current_trob.cell, &work.rect);
            work.tail.bytes.cell = check_trob_above(); mark_move(work.tail.bytes.cell, &work.rect);
            mark_move(check_trob_above_right(), &work.rect);
        }
        mark_move(check_trob_right(), &work.rect);
    }
}

void redraw_exit(void)
{
    struct { MoveRect rect; byte pad; sbyte cell; } work;

    if (compute_trob_move_rect(&work.rect, exit_rects[Level.environment_kind])) {
        mark_move(current_trob.cell, &work.rect);
        work.cell = check_trob_right(); mark_move(work.cell, &work.rect);
        work.cell = check_trob_above(); mark_move(work.cell, &work.rect);
        mark_move(check_trob_above_right(), &work.rect);
    }
}

void redraw_flask(void)
{
    MoveRect rect;

    if (compute_trob_move_rect(&rect, &flask_rect))
        mark_move(check_trob(), &rect);
}

void redraw_plate(void)
{
    MoveRect rect;
    MoveRect *source;

    if (Level.environment_kind == 3)
        source = &temple_plate_rect;
    else
        source = &plate_rect;
    if (compute_trob_move_rect(&rect, source))
        mark_bufs_for_rect(mark_move, &rect, -1);
}

void redraw_loose(void)
{
    struct { MoveRect rect; byte pad; sbyte cell; } work;

    if (compute_trob_move_rect(&work.rect, loose_rects[Level.environment_kind])) {
        mark_move(work.cell = check_trob(), &work.rect);
        mark_move(check_trob_right(), &work.rect);
        if (Level.environment_kind == 3)
            mark_move(check_trob_below(), &work.rect);
    }
}

void redraw_gate(void)
{
    struct { MoveRect rect; byte pad; sbyte cell; } work;

    if (Level.environment_kind == 3)
        work.rect = temple_gate_rect;
    else
        work.rect = gate_rect;
    work.cell = check_trob_above_right();
    if (work.cell != 30 && work.cell >= 0 && !IS_SIX_KIND((&Level.cell_kinds[0][0])[30 * Room.number - 30 + work.cell]))
        work.rect.edges[0] = 0;
    if (compute_trob_move_rect(&work.rect, &work.rect)) {
        mark_move(current_trob.cell, &work.rect);
        work.cell = check_trob_right(); mark_move(work.cell, &work.rect);
        mark_move(check_trob_above_right(), &work.rect);
    }
}

void redraw_trob(MoveRect *base)
{
    struct { MoveRect rect; byte pad; sbyte cell; } work;

    if (base && compute_trob_move_rect(&work.rect, base)) {
        mark_move(work.cell = check_trob(), &work.rect);
        mark_move(check_trob_right(), &work.rect);
    }
}

MoveRect *compute_trob_move_rect(MoveRect *out, MoveRect *base)
{
    MoveRect *result = out;
    sbyte column = CellToCol(current_trob.room, current_trob.cell);
    out = result;
    return ComputeOffsetCellRect(out, base, column,
                                 CellToRow(current_trob.room, current_trob.cell));
}

sbyte check_trob(void)
{
    sbyte cell = current_trob.cell;

    if (Room.above == current_trob.room) {
        if (cell >= 20 && cell < 30)
            cell = 19 - cell;
        else
            cell = 30;
    } else if (Room.number != current_trob.room)
        cell = 30;
    return cell;
}

int check_trob_visibility(void)
{
    int visible = is_visible_room_cell(current_trob.room, current_trob.cell);
    if (!visible)
        current_trob.state = -1;
    return visible;
}

sbyte check_trob_left(void)
{
    sbyte cell = current_trob.cell;

    if (Room.number == current_trob.room) {
        if (cell % 10 == 0)
            cell = 30;
        else
            --cell;
    } else if (Room.right == current_trob.room && cell % 10 == 0)
        cell += 9;
    else
        cell = 30;
    return cell;
}

sbyte check_trob_right(void)
{
    sbyte cell = current_trob.cell;

    if (Room.number == current_trob.room) {
        if (cell % 10 == 9)
            cell = 30;
        else
            ++cell;
    } else if (Room.left == current_trob.room) {
        if (cell % 10 == 9)
            cell -= 9;
        else
            cell = 30;
    } else if (Room.above == current_trob.room) {
        if (cell >= 20 && cell < 29)
            cell = 18 - cell;
        else
            cell = 30;
    } else if (Room.above_left == current_trob.room)
        cell = cell == 29 ? -1 : 30;
    else
        cell = 30;
    return cell;
}

sbyte check_trob_above(void)
{
    sbyte cell = current_trob.cell;

    if (Room.number == current_trob.room) {
        if (cell < 10)
            cell = -1 - cell;
        else
            cell -= 10;
    } else if (Room.below == current_trob.room && cell < 10)
        cell += 20;
    else
        cell = 30;
    return cell;
}

sbyte check_trob_above_right(void)
{
    sbyte cell = current_trob.cell;

    if (Room.number == current_trob.room) {
        if (cell % 10 == 9)
            cell = 30;
        else if (cell < 10)
            cell = -2 - cell;
        else
            cell -= 9;
    } else if (Room.left == current_trob.room) {
        if (cell == 9)
            cell = -1;
        else if (cell % 10 == 9)
            cell -= 19;
        else
            cell = 30;
    } else if (Room.below == current_trob.room) {
        if (cell < 9)
            cell += 21;
        else
            cell = 30;
    } else if (Room.below_left == current_trob.room)
        cell = cell == 9 ? 20 : 30;
    else
        cell = 30;
    return cell;
}

sbyte check_trob_below(void)
{
    sbyte cell = current_trob.cell;

    if (Room.number == current_trob.room) {
        if (cell < 20)
            cell += 10;
        else
            cell = 30;
    } else if (Room.above == current_trob.room && cell >= 20)
        cell %= 10;
    else
        cell = 30;
    return cell;
}

#pragma pack(1)
typedef MoveRect CharBounds;
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
    CharBounds bounds;                    /* 1b: top, left, bottom, right */
    signed char alert_delay;           /* 23 */
    int sequence_group;                /* 24 */
    int previous_x, previous_y;        /* 26, 28 */
    unsigned int previous_frame;       /* 2a */
    CharBounds previous_bounds;           /* 2c: top, left, bottom, right */
    int field_34;                      /* 34 */
    int previous_mode;                 /* 36 */
    unsigned char behavior;            /* 38 */
    signed char opponent_index;        /* 39 */
    unsigned char field_3a;            /* 3a */
    unsigned char unknown_3b[5];        /* 3b..3f */
} CHARREC;
#pragma pack()
extern int floor_override, drew_pressed_tile;
#pragma pack(1)
typedef struct {
    int image, equipment_image;                       /* 00,02 */
    signed char x, y;                                 /* 04-05 */
    unsigned char flags;                              /* 06 */
} FRAMEREC;
#pragma pack()
extern void on_ground(void);
extern void FallOn(void);
extern void Falling(void);
extern sbyte get_above(void);
extern sbyte get_under_foot(void);
extern void fall_spikes(void);
extern byte CutRoom;
extern CHARREC Char;
void break_loose_floor(sbyte);
void jam_plate(void);
void push_plate(sbyte sound, int link);

void check_floor(void)
{
    unsigned frame;

    if (Char.character_kind == 7 || Char.character_kind == 8 || Char.action == 9)
        goto falling;

    frame = Char.frame;
    switch (Char.action) {
    default:
        on_ground();
        return;
    case 2:
    case 6:
        break;
    case 3:
        if (frame >= 0x66 && frame < 0x6a) {
            FallOn();
            return;
        }
        break;
    case 4:
        goto update_fall;
    case 5:
        if (frame == 0x6d)
            on_ground();
        break;
    }
    return;

falling:
    if (!Char.vertical_step)
        return;
update_fall:
    Falling();
}

extern FRAMEREC Frame;

void check_press(void)
{
    int action;
    unsigned frame;
    sbyte kind;

    if (Char.character_kind == 11 || Char.character_kind == 10 || (Char.character_kind == 0 && floor_override)
            || (Char.character_kind == 1 && (int)Char.sequence >= 29 && (int)Char.sequence <= 42))
        return;

    action = Char.action;
    frame = Char.frame;
    if ((frame >= 0x57 && frame <= 0x63) || (frame >= 0x87 && frame < 0x8d))
        kind = get_above();
    else if (action < 2 || action == 7 || action == 5 || action == 8) {
        kind = get_above();
        if (frame == 0x4f && kind == 11)
            break_loose_floor(0);
        else if (frame == 0x4f && kind == 15)
            fall_spikes();
        else
            if (!(Frame.flags & 0x40))
                kind = 0;
            else
                kind = get_under_foot();
    } else
        kind = 0;

    if (kind == 5 || kind == 6 || kind == 34) {
        if (Char.alive >= 0 && Char.character_kind != 4) {
            jam_plate();
            return;
        }
        push_plate(0, -1);
    } else if (kind == 11 || kind == 15) {
        drew_pressed_tile = 1;
        if (kind == 11) {
            break_loose_floor(0);
            return;
        }
        if (kind == 15)
            fall_spikes();
    }
}

extern int FrameNum;

extern int skip_alternate_torches;
extern CHARREC Kid;
int get_torch_frame(int frame);

void animate_flask(void)
{
    if (drawn_tile_kind == 10) {
        if (current_trob.state < 0 || !check_trob_visibility())
            return;
        {
            unsigned state = BlueSpec.state & ~31U;
            int frame = BlueSpec.state & 31;
            if (frame >= 27)
                frame = 0;
            else
                ++frame;
            BlueSpec.state = state | frame;
            redraw_flask();
        }
        return;
    }
    current_trob.state = -1;
}

void animate_torch(int index)
{
    if (check_trob_visibility()) {
        int frame = get_torch_frame((byte)BlueSpec.state);
        BlueSpec.state = (BlueSpec.state & 0xff00U) + frame;
        if (!skip_alternate_torches || ((FrameNum + index) & 1))
            redraw_torch();
    }
}

extern struct {
    sbyte facing;
    byte death_countdown;
} player_start;
extern byte active_sound_channels;

extern void AddSound(int value), dispatch_sound(int);
void stop_gate(int height);
void check_gate_fingers(void);
int did_player_escape_guard(void);
void add_gate_sound(int sound);
extern int move_cave_gate(int height);

void animate_gate(void)
{
    unsigned modifier = BlueSpec.state;
    int height = modifier & 255;
    sbyte phase = current_trob.state;
    int sound;

    if (phase >= 0) {
        if (phase >= 4) {
            if (phase < 8)
                current_trob.state = ++phase;
            height -= gate_close_speed[phase];
            if (height <= 0) {
                sbyte col, row;
                if (Level.environment_kind == 3) {
                    row = current_trob.cell / 10;
                    col = current_trob.cell % 10;
                    if (current_trob.room != (byte)Kid.room) {
                        if (current_trob.room == Room.left)
                            col -= 10;
                        else if (current_trob.room == Room.right)
                            col += 10;
                    }
                }
                if (Level.environment_kind == 3 && Kid.sequence == 118 && row == Kid.row
                        && (col == Kid.column || Kid.column - col == 1)) {
                    height = 36;
                    sound = 9;
                } else {
                    height = 0;
                    sound = Level.environment_kind != 3 ? 15 : 9;
                    check_gate_fingers();
                    if (did_player_escape_guard())
                        player_start.death_countdown = 4;
                }
                add_gate_sound(sound);
                current_trob.state = -1;
            }
        } else {
            if (height == 255)
                stop_gate(255);
            else {
                height += gate_open_speed[phase];
                if (phase == 0 || phase == 1) {
                    if (height <= 0) {
                        height = 0;
                        stop_gate(height);
                        check_gate_fingers();
                    } else if (height < 200) {
                        add_gate_sound(Level.environment_kind != 3 ? 4 : 7);
                        current_trob.state = 1;
                        if (Level.environment_kind == 3)
                            height = move_cave_gate(height);
                    } else if (Level.level_number == 5 && current_trob.room == 7)
                        height -= gate_open_speed[phase];
                } else if (height < 200) {
                    if (Level.environment_kind != 3) {
                        if (height % 8 == 0)
                            add_gate_sound(4);
                    } else
                        add_gate_sound(7);
                } else if (phase < 3) {
                    redraw_gate();
                    height = 250;
                    current_trob.state = 0;
                    add_gate_sound(Level.environment_kind != 3 ? 5 : 8);
                } else {
                    height = 255;
                    stop_gate(height);
                }
            }
        }
        BlueSpec.state = (BlueSpec.state & 0xff00U) + height;
    }
    if (height <= 200)
        redraw_gate();
}

extern void copy_kid_to_char(void);
extern void JumpSeq(int sequence);
extern sbyte FirstInRow(sbyte row);

void stop_gate(int height)
{
    int actor_cell;

    current_trob.state = -1;
    add_gate_sound(Level.environment_kind != 3 ? 6 : 8);
    copy_kid_to_char();
    actor_cell = FirstInRow(Char.row - 1) + Char.column;
    if (current_trob.cell == actor_cell && Char.direction == -1
            && Char.action == 2 && height == 0) {
        JumpSeq(47);
        Kid = Char;
    }
}

void check_gate_fingers(void)
{
    if (current_trob.room == (byte)Kid.room && (Kid.action == 2 || Kid.action == 6)) {
        sbyte actor_cell = FirstInRow(Kid.row) + Kid.column;
        if (current_trob.cell - actor_cell == -10 || current_trob.cell - actor_cell == -11)
            AddSound(31);
    }
}

extern CHARREC RoomOpp[5];
#pragma intrinsic(abs)

int did_player_escape_guard(void)
{
    int escaped = 0;
    sbyte count = Level.room_objects[current_trob.room - 1].opponent_count;
    sbyte row = current_trob.cell / 10;
    sbyte col = current_trob.cell % 10;
    sbyte i;
    if (count && (byte)Kid.room == current_trob.room && Kid.row == row
            && abs(col - Kid.column) < 5) {
        for (i = 0; i < count; ++i) {
            if (RoomOpp[i].row == row && RoomOpp[i].alive < 0 && RoomOpp[i].room == Kid.room) {
                if ((Kid.column > col && RoomOpp[i].column > col)
                        || (Kid.column <= col && RoomOpp[i].column <= col)) {
                    escaped = 0;
                    break;
                }
                escaped = 1;
            }
        }
    }
    return escaped;
}

void animate_exit(void)
{
    int height = BlueSpec.state & 255;

    if (current_trob.state >= 0) {
        if (current_trob.state >= 4) {
            ++current_trob.state;
            height -= exit_close_speed[current_trob.state - 4];
            if (height < 0) {
                height = 0;
                current_trob.state = -1;
                AddSound(9);
            } else if (current_trob.state == 4 && (active_sound_channels & 1))
                AddSound(7);
        } else {
            ++height;
            if (height >= 42) {
                current_trob.state = -1;
                if (Room.number != 4 || Level.level_number != 13) {
                    dispatch_sound(0);
                    AddSound(8);
                }
            } else if (Room.number != 4 || Level.level_number != 13)
                AddSound(7);
        }
    }
    BlueSpec.state = (BlueSpec.state & 0xff00U) + height;
    redraw_exit();
}

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
extern int get_cave_torch_frame(int frame), choose_trapdoor_delay(int frame);
extern int EMPTYRECT(MoveRect far *rect);
extern sbyte index_cell(sbyte column, sbyte row);
extern CELLREC Cell;

int IsExitOpen(void)
{
    if ((byte)Cell.modifier >= 42)
        return 1;
    return 0;
}


int get_torch_frame(int frame)
{
    int result;
    if (Level.environment_kind == 3)
        result = get_cave_torch_frame(frame);
    else
        result = choose_trapdoor_delay(frame);
    return result;
}

typedef struct {
    int dirty;
    MoveRect rect;
} MarkedRect;

extern MarkedRect top_marks[10], move_marks[30], floor_marks[30], foreground_marks[30];
extern int half_marks[30];

extern void ComputeClipRect(MoveRect far *out, int column, int row);
extern int SECTRECT(MoveRect far *out, MoveRect far *a, MoveRect far *b);
extern void UNIONRECT(MoveRect far *out, MoveRect far *a, MoveRect far *b);

void mark_rect_buf(MarkedRect *mark, int cell, MoveRect *clip);
void mark_guard_updates(MoveRect *clip, sbyte except);

void mark_top(int cell, MoveRect *clip)
{
    MoveRect rect;

    if (cell <= -1 && cell >= -10) {
        ComputeClipRect(&rect, -((cell + 1) % 10), -1);
        if (!clip || SECTRECT(&rect, &rect, clip)) {
            ++cell;
            cell = -cell;
            if (top_marks[cell].dirty)
                UNIONRECT(&top_marks[cell].rect, &top_marks[cell].rect, &rect);
            else {
                top_marks[cell].dirty = 1;
                top_marks[cell].rect = rect;
            }
        }
    }
}

void mark_move(sbyte cell, MoveRect *clip)
{
    if (cell < 30) {
        if (cell >= 0)
            mark_rect_buf(&move_marks[cell], cell, clip);
        else
            mark_top(cell, clip);
        mark_guard_updates(clip, -1);
    }
}

void mark_floor(sbyte cell, MoveRect *clip)
{
    if (cell < 30) {
        if (cell >= 0) {
            mark_rect_buf(&floor_marks[cell], cell, clip);
            return;
        }
        mark_top(cell, clip);
    }
}

void mark_half(sbyte cell)
{
    if (cell < 30) {
        if (cell >= 0) {
            half_marks[cell] = 1;
            return;
        }
        mark_top(cell, 0);
    }
}

void mark_redraw(int cell)
{
    extern unsigned int RedrawBuf[];

    if (cell < 30) {
        if (cell >= 0)
            *(int *)((byte *)RedrawBuf + (cell << 1)) = 1;
        else
            mark_top(cell, 0);
    }
}

void mark_cell_redraw(void)
{
    sbyte cell = Cell.index;

    if (Room.number == Cell.room)
        ;
    else if (Room.left == Cell.room)
        cell -= 9;
    else if (Room.right == Cell.room)
        cell += 9;
    else if (Room.above == Cell.room)
        cell -= 20;
    else if (Room.below == Cell.room)
        cell += 20;
    mark_redraw(cell);
}

void mark_foreground(sbyte cell, MoveRect *clip)
{
    if (cell < 30) {
        if (cell >= 0) {
            mark_rect_buf(&foreground_marks[cell], cell, clip);
            return;
        }
        mark_top(cell, clip);
    }
}

void mark_bufs_for_rect(CellMark mark, MoveRect *rect, sbyte except)
{
    struct {
        int top, left, bottom, right;
    } bounds;
    sbyte row, column;

    if (!EMPTYRECT(rect)) {
        if (rect->edges[0] >= 3)
            bounds.top = (rect->edges[0] - 3) / 63;
        else
            bounds.top = -1;
        if (rect->edges[2] >= 3)
            bounds.bottom = (rect->edges[2] - 3) / 63;
        else
            bounds.bottom = -1;
        bounds.left = rect->edges[1] / 32;
        bounds.right = rect->edges[3] / 32;
        for (row = bounds.top; row <= bounds.bottom; ++row) {
            for (column = bounds.left; column <= bounds.right; ++column)
                mark(index_cell(column, row), rect);
        }
        mark_guard_updates(rect, except);
    }
}

void mark_guard_updates(MoveRect *clip, sbyte except)
{
    MoveRect hit;
    sbyte count = Level.room_objects[Room.number - 1].opponent_count;
    int i;

    for (i = 0; i < count; ++i) {
        if (RoomOpp[i].slot != except)
            RoomOpp[i].previous_mode |= SECTRECT(&hit, &RoomOpp[i].bounds, clip);
    }
}

void mark_rect_buf(MarkedRect *mark, int cell, MoveRect *clip)
{
    MoveRect rect;

    ComputeClipRect(&rect, cell % 10, cell / 10);
    if (SECTRECT(&rect, &rect, clip)) {
        if (mark->dirty)
            UNIONRECT(&mark->rect, &mark->rect, &rect);
        else {
            mark->dirty = 1;
            mark->rect = rect;
        }
    }
}

int get_link_next(int link);
int get_link_cell(int link);
byte get_link_room(int link);
Trob *get_trob_room_cell(byte room, int cell);
void add_trob(byte room, sbyte cell, sbyte state, byte kind);
sbyte trigger_object(byte kind, byte room, sbyte cell, int action);
extern unsigned Rnd(unsigned limit);

void trigger_link(int link, int action)
{
    byte room, kind;
    sbyte cell, state;

    if ((int)get_link_room(link) != -2 && link != 0xfd) {
        do {
            room = get_link_room(link);
            select_room_arrays(room);
            cell = (sbyte)get_link_cell(link);
            kind = room_kinds[cell];
            state = trigger_object(kind, room, cell, action);
            if (state >= 0)
                add_trob(room, cell, state, kind);
            link = get_link_next(link);
        } while (link != 0xfd);
    }
}

void trigger_flask(byte room, sbyte cell)
{
    unsigned state = (BlueS[cell].state & ~31U) | (Rnd(26) + 1);
    BlueS[cell].state = state;
    add_trob(room, cell, 1, 10);
}

extern sbyte trigger_reverse_gate(byte room, sbyte cell);
extern int is_dead_pos(unsigned int frame);

sbyte trigger_gate(byte room, sbyte cell, int action)
{
    sbyte state = -1;
    unsigned value = BlueS[cell].state;
    unsigned height = value & 255;
    unsigned modifier = value;
    unsigned flags = modifier & 0xff00U;

    switch (action) {
    case 5:
        if (height == 255)
            break;
        if (height >= 200) {
            BlueS[cell].state = flags + 250;
            if (Char.alive >= 0 || (CutRoom && CutRoom != (byte)Char.room)) {
                BlueS[cell].state = flags + 255;
                state = 3;
            }
        } else {
            BlueS[cell].state = flags + ((height + 3) & 0xfc);
            if (Char.alive >= 0 || (CutRoom && CutRoom != (byte)Char.room))
                state = 3;
            else
                state = 2;
        }
        break;
    case 6: {
        Trob *trob = get_trob_room_cell(room, cell);
        if (is_dead_pos(Char.frame) && Char.alive >= 2)
            break;
        if ((!trob && height) || (trob && trob->state != 4))
            state = 4;
        break;
    }
    case 14:
        if (height < 200)
            state = 3;
        else
            BlueS[cell].state = flags + 255;
        break;
    case 34:
        state = trigger_reverse_gate(room, cell);
        break;
    }
    return state;
}

extern int query_sound(int);
extern void AddSong(int sound);
extern void trigger_slide_rock(byte room, sbyte cell, sbyte state);
extern sbyte trigger_cave_bars(byte, sbyte);
extern void trigger_dart(byte, sbyte);

sbyte trigger_object(byte kind, byte room, sbyte cell, int action)
{
    sbyte state;

    switch (kind) {
    case 4:
        state = trigger_gate(room, cell, action);
        break;
    case 17:
        if ((sbyte)room != Level.entry_room && Level.level_number == 8 && player_start.facing != 1) {
            state = -1;
        } else {
            int height = BlueS[cell].state & 255;
            state = height ? -1 : 1;
            if (state == 1 && !query_sound(10027) && Level.level_number != 13)
                AddSong(27);
        }
        break;
    case 25:
        if (action == 5)
            state = 2;
        else
            state = 1;
        trigger_slide_rock(room, cell, state);
        state = -1;
        break;
    case 27:
        state = trigger_cave_bars(room, cell);
        break;
    case 36:
        if (!(Cell.modifier & 0x0800U))
            trigger_dart(room, cell);
        state = -1;
        break;
    default:
        state = -1;
        break;
    }
    return state;
}

void trigger_torch(byte room, sbyte cell)
{
    unsigned state = (BlueS[cell].state & 0xff00U) + Rnd(Level.environment_kind == 3 ? 8 : 3);
    BlueS[cell].state = state;
    add_trob(room, cell, 1, 19);
}

void add_trob(byte room, sbyte cell, sbyte state, byte kind)
{
    Trob *found = get_trob_room_cell(room, cell);

    if (!found) {
        if (trob_count < 20) {
            current_trob.room = room;
            current_trob.cell = cell;
            current_trob.state = state;
            current_trob.kind = kind;
            trobs[trob_count] = current_trob;
            ++trob_count;
        }
    } else {
        found->state = state;
    }
}

void * _cdecl memset(void *, int, unsigned int);
#pragma intrinsic(memset)

extern unsigned int RedrawBuf[30];
#define MoveBuf move_marks
#define FRedrawBuf foreground_marks
#define FloorBuf floor_marks
#define HalfBuf half_marks
extern unsigned char ObjBuf[30];
#define TopBuf top_marks

void ZeroBuffers(void)
{
    memset(RedrawBuf, 0, sizeof(RedrawBuf));
    memset(MoveBuf, 0, sizeof(MoveBuf));
    memset(FRedrawBuf, 0, sizeof(FRedrawBuf));
    memset(FloorBuf, 0, sizeof(FloorBuf));
    memset(HalfBuf, 0, sizeof(HalfBuf));
    memset(ObjBuf, 0, sizeof(ObjBuf));
    memset(TopBuf, 0, sizeof(TopBuf));
}

extern byte ReadCell(byte room, sbyte column, sbyte row);
extern int IsWall(byte tile);

sbyte find_wall_column(byte room, sbyte column, sbyte row, sbyte step)
{
    do {
        column += step;
        if (IsWall(ReadCell(room, column, row)))
            break;
    } while (column >= 0 && column <= 10);
    return column;
}

extern int far _fastcall get_slide_rock_x(unsigned char room, signed char row);

int far _fastcall probe_jump_anchor(signed char room, signed char row)
{
    int x;

    if (Level.environment_kind == 2)
        x = get_slide_rock_x(room, row);
    else
        x = 0;
    return x;
}

int get_link_next(int link)
{
    return Level.tile_links[link].next;
}

int GetLinkTimer(int link)
{
    return Level.tile_links[link].timer;
}

int ChangeLinkTimer(int link, byte timer)
{
    return Level.tile_links[link].timer = timer;
}

int get_link_cell(int link)
{
    return Level.tile_links[link].cell;
}

int link_has_previous(int link)
{
    if (Level.tile_links[link].prev != 0xfd)
        return 1;
    return 0;
}

int link_has_next(int link)
{
    if (Level.tile_links[link].next != 0xfd)
        return 1;
    return 0;
}

byte get_link_room(int link)
{
    return Level.tile_links[link].room;
}

void push_plate(sbyte kind, int link)
{
    sbyte previous_timer;
    sbyte cell;
    byte room;
    unsigned state;

    if (!kind)
        kind = Cell.kind;
    if (link == -1)
        link = Cell.modifier & 255;
    previous_timer = (sbyte)GetLinkTimer(link);
    cell = Cell.index;
    state = Cell.modifier | 0x0800U;
    room = Cell.room;
    if (previous_timer != 31 && (ChangeLinkTimer(link, 5), previous_timer < 2)) {
        add_trob(Cell.room, Cell.index, 1, kind);
        if (Char.frame != 185 && (Cell.room != 4 || Level.level_number != 13))
            AddSound(3);
        redraw_plate();
        trigger_link(link, (byte)kind);
        drew_pressed_tile = 1;
    } else {
        trigger_link(link, (byte)kind);
    }
    Level.cell_info[room - 1][cell].modifier = state;
}

void jam_plate(void)
{
    int link = Cell.modifier & 255;

    ChangeLinkTimer(link, 31);
    push_plate(Cell.kind, link);
}

void animate_plate(void)
{
    if (current_trob.state >= 0) {
        int link = BlueSpec.state & 255;
        sbyte timer = GetLinkTimer(link);
        if (timer < 31) {
            ChangeLinkTimer(link, timer - 1);
            if (timer < 2) {
                current_trob.state = -1;
                redraw_plate();
                BlueSpec.state &= ~0x0800U;
                return;
            }
        } else
            current_trob.state = -1;
    }
}

#pragma pack(1)
typedef struct {
    int x, y;                                         /* 00,02 */
    unsigned char room;                               /* 04 */
    int velocity, auxiliary;                          /* 05,07 */
    signed char kind, row;                             /* 09-0a */
    unsigned int flags;                               /* 0b */
} MOBREC;
#pragma pack()


void close_exit(byte room, sbyte cell)
{
    if (Level.environment_kind != 1) {
        RoomObject *objects = BlueS;
        objects[cell].state = (objects[cell].state & 0xff00U) + 42;
        add_trob(room, cell, 4, 17);
    }
}

void make_space(byte room, sbyte cell);
void add_mob(void);
extern int MobI;
void PassThru(void);
void mark_mob_redraw(sbyte layer);
extern sbyte IndexCell1(sbyte column, sbyte row);
extern MOBREC Mob;
extern MOBREC mobs[];
extern int mob_count;
extern sbyte GetRow(int);

void animate_loose_floor(void)
{
    int frame;
    int kind;
    sbyte row;

    ++BlueSpec.state;
    frame = BlueSpec.state & 15;
    if (current_trob.state != -1) {
        if (BlueSpec.state & 0x40) {
            if (frame >= 4) {
                current_trob.state = -1;
                BlueSpec.state = (BlueSpec.state & 0xff00U) | (BlueSpec.state & 0xb0U);
            }
        } else if (frame >= 12) {
            kind = Level.environment_kind == 3 ? (BlueSpec.state & 0x80) >> 7 : 3;
            make_space(current_trob.room, current_trob.cell);
            BlueSpec.state = kind + 3;
            Mob.x = (current_trob.cell % 10) * 32;
            row = current_trob.cell / 10;
            Mob.y = row * 63 + 66;
            Mob.room = current_trob.room;
            Mob.auxiliary = 0;
            Mob.velocity = current_trob.state;
            Mob.kind = kind;
            Mob.row = row;
            Mob.flags = 0;
            add_mob();
            current_trob.state = -1;
        }
    }
    redraw_loose();
}

void make_space(unsigned char room, signed char cell)
{
    int index = cell;
    unsigned state;

    room_kinds[index] = 0;
    if (Level.level_number == 5 && room == 3)
        state = 0xc000;
    else
        state = (BlueS[*(volatile int *)&index].state & 0x80) + 3;
    BlueS[*(volatile int *)&index].state = state;
}

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

extern void shake_trapdoor(void);
extern void shake_bones(byte room, sbyte row);

void shake_loose_floor(void);

void break_loose_floor(sbyte sound)
{
    if (Char.character_kind == 1 && sound == 0) {
        shake_loose_floor();
    } else {
        unsigned state = Cell.modifier & ~0x40U;
        if (sound)
            state = (state & ~4U) | 11;
        BlueS[Cell.index].state = state;
        add_trob(Cell.room, Cell.index, sound, 11);
        AddSound(20);
    }
}

void shake_loose_floor(void)
{
    if (!(Cell.modifier & 15)) {
        int counter = ((Cell.modifier & 0x30) >> 4) + 1;

        if ((Cell.column != Char.column && counter != 3) || Char.character_kind == 1) {
            int index;
            RoomObject *objects;

            Cell.modifier &= ~0x30U;
            index = Cell.index;
            objects = BlueS;
            Cell.modifier |= (counter | 4) << 4;
            objects[index].state = Cell.modifier | 0x40;
        }
        add_trob(Cell.room, Cell.index, 1, 11);
        AddSound(20);
    }
}

void shake_row(byte room, sbyte row)
{
    MOBREC saved = Mob;
    sbyte column = 0;

    do {
        switch (ReadCell(room, column, row)) {
        case 11:
            shake_loose_floor();
            break;
        case 15:
            fall_spikes();
            break;
        case 26:
            shake_trapdoor();
            break;
        }
    } while (++column < 10);

    if (Level.preferred_opponent == 2)
        shake_bones(room, row);
    Mob = saved;
}

void add_mob(void)
{
    if (mob_count < 30)
        mobs[mob_count++] = Mob;
}

extern void move_dart_mob(void);
extern void MobSpikes(void);
extern void move_rock_record(void);
extern void check_slide_rock_crush(void);
extern void move_trapdoor_record(void);
extern void move_fireball(void);
void animate_mob(void);
void mob_loose(void);
void check_loose_crush(void);
void make_rubble(void);
void knock_loose(void);

MOBREC *get_mob_type_item(byte kind, int occurrence)
{
    MOBREC *found = 0;
    int index, count;

    for (count = index = 0; index < mob_count && found == 0; ++index) {
        if ((byte)mobs[index].kind == kind && mobs[index].velocity != -1) {
            if (++count == occurrence)
                found = &mobs[index];
        }
    }
    return found;
}


void select_trob_cell(int cell)
{
    BlueSpec = BlueS[cell];
    drawn_tile_kind = room_kinds[cell];
}

struct {
    int heights[13];
    int widths[13];
} mob_dimensions = {
    {18, 18, 2, 16, 27, 39, 69, 69, 12, 12, 37, 37, 18},
    {50, 50, 4, 57, 27, 31, 52, 52, 24, 24, 69, 57, 17}
};

#define mob_heights mob_dimensions.heights
#define mob_widths mob_dimensions.widths

void anim_mobs(void)
{
    struct {
        MOBREC *destination;
        int kept, index;

    } work;

    {
        int count = mob_count;
        for (MobI = 0; MobI < count; ++MobI) {
            Mob = mobs[MobI];
            animate_mob();
            mobs[MobI] = Mob;
        }
    }

    work.index = work.kept = 0;
    if (work.index < mob_count) {
        int *speed = &mobs[0].velocity;

        work.destination = mobs;
        do {
            if (*speed != -1) {
                MOBREC *next = work.destination++;
                ++work.kept;
                *next = *(MOBREC *)((byte *)speed - (unsigned)&((MOBREC *)0)->velocity);
            }
            speed = (int *)((byte *)speed + sizeof(MOBREC));
        } while (++work.index < mob_count);
    }
    mob_count = work.kept;
}

extern void step_bridge_mob(void);

void animate_mob(void)
{
    switch ((byte)Mob.kind) {
    case 0:
    case 1:
    case 3:
        mob_loose();
        if (Mob.velocity <= 0)
            ++Mob.velocity;
        check_loose_crush();
        break;
    case 2:
        move_dart_mob();
        return;
    case 4:
        MobSpikes();
        return;
    case 6:
        move_rock_record();
        check_slide_rock_crush();
        return;
    case 10:
        move_trapdoor_record();
        return;
    case 11:
        step_bridge_mob();
        return;
    case 12:
        move_fireball();
        return;
    }
}

void mob_loose(void)
{
    byte kind;

    if (Mob.velocity >= 0) {
        if (Mob.velocity < 29)
            Mob.velocity += 3;
        Mob.y += Mob.velocity;

        if (!Mob.room) {
            if (mob_heights[(byte)Mob.kind] + 192 <= Mob.y) {
                Mob.velocity = -2;
                return;
            }
        } else if ((Mob.row + 1) * 63 <= Mob.y) {
            kind = ReadCell(Mob.room, Mob.x / 32, Mob.row);
            if (kind == 11)
                knock_loose();
            else if (kind == 15)
                fall_spikes();

            if (kind != 11 && kind != 15 && !IS_SIX_KIND(kind)) {
                AddSound(2);
                shake_row(Mob.room, Mob.row);
                Mob.y = (Mob.row + 1) * 63;
                Mob.velocity = -2;
                mobs[MobI].flags = Mob.flags = 1;
                make_rubble();
                return;
            }
            PassThru();
        }
    }
}

void make_rubble(void)
{
    struct {
        int index;
        sbyte row, column;
        sbyte plate;
        byte kind;
        sbyte cell;
        byte room;
    } work;

    work.plate = 0;
    work.kind = ReadCell(Mob.room, Mob.x / 32, Mob.row);
    work.room = Cell.room;
    work.column = Cell.column;
    work.row = Cell.row;
    work.cell = Cell.index;

    switch (work.kind) {
    case 5:
        work.plate = (&Level.cell_kinds[0][0])[30 * work.room - 30 + work.cell] = 14;
    case 6:
    case 34:
        push_plate(work.plate, -1);
        if (work.kind == 34)
            Level.cell_info[work.room - 1][work.cell].modifier = Cell.modifier | 0x1800;
        ReadCell(Mob.room, Mob.x / 32, Mob.row);
        Cell.modifier &= 0xff00U;
        if (work.kind == 5)
            Cell.modifier |= 4;
        else if (work.kind == 6)
            Cell.modifier |= 8;
        work.kind = Cell.kind;
    case 1:
    case 2:
    case 10:
    case 36:
        if (work.kind == 10) {
            Trob *flask = get_trob_room_cell(Cell.room, Cell.index);
            if (flask) {
                current_trob = *flask;
                redraw_flask();
            }
        }
        goto reread_cell;
    case 19:
    case 32:
    reread_cell:
        work.kind = ReadCell(work.room, work.column, work.row);
        if (work.kind == 19 || work.kind == 32) {
            (&Level.cell_kinds[0][0])[30 * work.room - 30 + work.cell] = 32;
        } else {
            if (work.kind == 10 || work.kind == 1)
                Cell.modifier = 0;
            work.index = work.room * 30 + work.cell;
            (&(&Level.cell_kinds[0][0])[30 * 0 - 30 + 0])[work.index] = 14;
            if (Level.environment_kind == 4)
                (&Level.cell_info[0][0])[work.index - 30].modifier = Cell.modifier | 0x40;
            else if ((Level.environment_kind == 3 || Level.environment_kind == 2) && work.kind != 1)
                (&Level.cell_info[0][0])[work.index - 30].modifier = 0;
        }
        break;
    case 12:
    case 13:
        Level.cell_info[work.room - 1][work.cell].modifier = Cell.modifier | 0x20;
        break;
    default:
        return;
    }
    mark_mob_redraw(0);
}

void knock_loose(void)
{
    make_space(Cell.room, Cell.index);
    Mob.velocity >>= 1;
    mobs[MobI] = Mob;
    Mob.y = Mob.row * 63 + 66;
    mark_mob_redraw(0);
    Mob.y = mobs[MobI].y + 6;
    PassThru();
    add_mob();
    Mob = mobs[MobI];
    mark_mob_redraw(0);
}



void PassThru(void)
{
    ++Mob.row;
    if (Mob.row >= 3) {
        Mob.y -= 192;
        Mob.row = 0;
        Mob.room = Level.room_links[Mob.room - 1].below;
    }
}

#pragma pack(1)
typedef struct {
    signed char cell;
    int x;
    unsigned char fields_before_action[12];
    unsigned char action;
} OpponentPosition;
#pragma pack()

void add_mob_to_frame(void);
void add_loose_to_frame(void);
void compute_mob_rect(MoveRect *rect);
void add_mob_obj(int y);
int opp_under_mob(OpponentPosition *opp);
int loose_hits_actor(void);

extern void animate_bridge_mob(void);
extern void move_dart_to_tile(void);
extern void ATMSpikes(void);
extern void move_slide_rock(void);
extern void move_trapdoor(void);
extern void update_fireball_view(void);
extern MoveRect NullRect;
extern void OFFSETRECT(MoveRect far *, MoveRect far *, int, int);
extern FCHARREC FChar;

int is_loose_pushing_plate(void)
{
    byte kind = Level.environment_kind == 3 ? 1 : 3;
    int occurrence = 0;
    int pushing = 0;
    MOBREC *mob;

    do {
        mob = get_mob_type_item(kind, ++occurrence);
        if (mob && mob->room == Cell.room && mob->row == Cell.row
                && mob->x / 32 == Cell.column && mob->flags)
            pushing = 1;
    } while (mob && !pushing);
    return pushing;
}

void add_mobs(void)
{
    int index = 0;
    MOBREC *entry;

    if (index < mob_count) {
        entry = mobs;
        do {
            Mob = *entry;
            add_mob_to_frame();
            *entry = Mob;
            ++entry;
        } while (++index < mob_count);
    }
}

void add_mob_to_frame(void)
{
    switch ((byte)Mob.kind) {
    case 0:
    case 1:
    case 3:
        add_loose_to_frame();
        break;
    case 2:
        move_dart_to_tile();
        return;
    case 4:
        ATMSpikes();
        return;
    case 6:
        move_slide_rock();
        return;
    case 10:
        move_trapdoor();
        return;
    case 11:
        animate_bridge_mob();
        return;
    case 12:
        update_fireball_view();
        return;
    }
}

void add_loose_to_frame(void)
{
    int visible = 1;
    int y = Mob.y;
    sbyte column, row, cell;
    MoveRect rect;
    int x;
    int height = mob_heights[(byte)Mob.kind];

    if (Room.number == Mob.room) {
        if (height + 191 <= Mob.y)
            visible = 0;
    } else if (Room.below == Mob.room) {
        if (height > y)
            y = Mob.y + 191;
        else
            visible = 0;
    } else if (Room.above == Mob.room) {
        if (191 - height <= Mob.y)
            y = Mob.y - 191;
        else
            visible = 0;
    } else if (Room.left == Mob.room && mob_widths[(byte)Mob.kind] + (x = Mob.x) > 320) {
        Mob.x = x - 320;
        if (Room.left == Mob.room)
            Mob.room = Room.number;
        else if (Room.above_left == Mob.room)
            Mob.room = Room.above;
    } else {
        visible = 0;
    }

    if (visible) {
        FChar.index = IndexCell1(column = Mob.x / 32, row = GetRow(y));
        mark_mob_redraw(1);
        compute_mob_rect(&rect);
        add_mob_obj(y);
        ++column;
        cell = index_cell(column, row);
        if (Room.below != Mob.room)
            mark_floor(cell, &rect);
        cell = GetRow(y - height);
        if (cell != row)
            mark_floor(index_cell(column, cell), &rect);
    }
}

void compute_mob_rect(MoveRect *rect)
{
    int x, y;

    rect->edges[0] = Mob.y - mob_heights[(byte)Mob.kind];
    rect->edges[1] = Mob.x;
    rect->edges[2] = Mob.y + 1;
    rect->edges[3] = rect->edges[1] + mob_widths[(byte)Mob.kind];
    if (Room.number != Mob.room) {
        if (Room.left == Mob.room || Room.above_left == Mob.room
                || Room.below_left == Mob.room || Room.right == Mob.room
                || Room.above_right == Mob.room || Room.below_right == Mob.room)
            x = -320;
        else
            x = 0;
        if (Room.above == Mob.room || Room.above_left == Mob.room
                || Room.above_right == Mob.room)
            y = -192;
        else if (Room.below == Mob.room || Room.below_left == Mob.room
                || Room.below_right == Mob.room)
            y = 192;
        else
            y = 0;
        if (x || y)
            OFFSETRECT(rect, rect, x, y);
        else
            *rect = NullRect;
    }
}

void mark_mob_redraw(sbyte layer)
{
    MoveRect rect;
    CellMark mark;

    compute_mob_rect(&rect);
    if (layer == 1)
        mark = mark_foreground;
    else if (layer == 2)
        mark = mark_floor;
    else
        mark = mark_move;
    mark_bufs_for_rect(mark, &rect, -1);
}

#pragma pack(1)
typedef struct {
    int x, y;
    byte bank;
    int flags;
    int scratch;
    byte reserved;
    byte image;
    MoveRect rect;
    byte layer;
    int sequence;
    sbyte order;
} MobDraw;
#pragma pack()

extern int mob_draw_count;
extern MobDraw mob_draw_list[];
extern void set_object_cell(sbyte index);

void add_mob_obj(int y)
{
    int index = mob_draw_count++;
    int zero;

    mob_draw_list[index].image = (byte)Mob.kind | 0x80;
    mob_draw_list[index].x = Mob.x;
    mob_draw_list[index].y = y;
    mob_draw_list[index].bank = 4;
    mob_draw_list[index].flags = Mob.flags;
    compute_mob_rect(&mob_draw_list[index].rect);
    zero = 0;
    mob_draw_list[index].sequence = zero;
    mob_draw_list[index].scratch = zero;
    mob_draw_list[index].order = -1;
    set_object_cell(index);
}

extern void crush_actor(void);
extern void load_actor_slot(sbyte index);
extern void save_actor_slot(void);
extern OpponentPosition *GetOppInfoIdx(int room, int index);
extern void CrushOppInfo(OpponentPosition *opp, byte room);
extern sbyte near_crush_offset[], far_crush_offset[];
extern int get_edge_distance(void);

void check_loose_crush(void)
{
    sbyte index;
    sbyte count;

    if (!Mob.room)
        return;
    copy_kid_to_char();
    if (loose_hits_actor()) {
        crush_actor();
        Kid = Char;
    }

    index = 0;
    count = Level.room_objects[Mob.room - 1].opponent_count;
    for (; index < count; ++index) {
        if (Mob.room == (byte)Kid.room) {
            load_actor_slot(index);
            if (loose_hits_actor()) {
                crush_actor();
                save_actor_slot();
            }
        } else {
            OpponentPosition *guard = GetOppInfoIdx(Mob.room, index);
            if (opp_under_mob(guard))
                CrushOppInfo(guard, Mob.room);
        }
    }
}

int loose_hits_actor(void)
{
    int hit = 0;
    int y;
    sbyte column;

    if (Char.character_kind == 11)
        y = Char.y;
    else if (Char.character_kind == 7 || Char.character_kind == 8)
        y = Char.y - 20;
    else
        y = Char.y - 30;

    if (Char.alive < 0 && (byte)Char.room == Mob.room
            && y < Mob.y && Char.row == Mob.row) {
        column = Mob.x / 32;
        hit = column == Char.column;
        if (!hit && Char.character_kind == 11) {
            int reach = get_edge_distance();
            int edge;

            edge = Char.column + far_crush_offset[Char.direction];
            if (edge == column) {
                hit = reach > 8;
            } else if (column == Char.column + near_crush_offset[Char.direction]) {
                hit = reach < 4;
            }
        }
    }
    return hit;
}


int opp_under_mob(OpponentPosition *opp)
{
    int result = 0;
    int height = (signed char)(opp->cell / 10) * 63 + 56;

    if (Level.preferred_opponent != 5 || opp->action != 8)
        height -= 30;
    if (height < Mob.y) {
        if (Mob.x / 32 == (opp->x - 144) / 32)
            result = 1;
        else
            result = 0;
    }
    return result;
}

void add_gate_sound(int sound)
{
    AddSound(sound);
    if (sound == 9 && Level.level_number == 5 && current_trob.room == 7
            && !query_sound(10065)) {
        if (Level.level_number != 5 || (byte)Kid.room != 7 || Kid.column >= 3)
            AddSong(65);
    }
}

Trob *get_trob_room_cell(byte room, int cell)
{
    Trob *result = 0;
    int i;

    for (i = 0; i < trob_count && !result; ++i) {
        if (trobs[i].room == room && trobs[i].cell == cell)
            result = &trobs[i];
    }
    return result;
}

Trob *get_trob_by_id(byte kind, sbyte id)
{
    sbyte found = 0;
    Trob *result = 0;
    int i;

    for (i = 0; i < trob_count && !result; ++i) {
        if (trobs[i].kind == kind && trobs[i].state != -1 && ++found == id)
            result = &trobs[i];
    }
    return result;
}

int is_visible_room_cell(byte room, sbyte cell)
{
    int visible;

    if (Room.number == room || (Room.left == room && cell % 10 == 9)
            || (Room.below == room && cell < 10)
            || (Room.below_left == room && cell == 9))
        visible = 1;
    else
        visible = 0;
    return visible;
}

unsigned char CharObjForePriority[15] = {
    127, 127, 127, 100, 127, 50, 127, 127, 127, 100, 127, 127, 127, 200, 200
};

unsigned char NonCharObjForePriority[13] = {
    127, 127, 127, 127, 200, 60, 225, 50, 200, 80, 210, 210, 60
};
