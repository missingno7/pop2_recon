/* BRIDGE.C, DOS overlay 14. Names and packed layouts reflect the observed uses. */
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

typedef struct Gate {
    int timer;
    unsigned char state;
} Gate;

typedef struct TileAnim {
    unsigned char type;
    signed char col;
    int unknown;
    unsigned char modifier;
} TileAnim;

typedef struct Image {
    char unused;
    int y;
    char other[8];
    char rect[14];
} Image;

typedef struct ImageTable {
    char header[28];
    Image images[1];
} ImageTable;

typedef struct Tile {
    unsigned int modifier;
    unsigned int extra;
} Tile;

typedef struct Rect {
    int top, left, bottom, right;
} Rect;

extern CHARREC Char, Kid, RoomOpp[5], Opp;

typedef struct {
    unsigned char number;                             /* 00 */
    unsigned char left, right, above, below;           /* 01-04 */
    unsigned char above_left, above_right;             /* 05-06 */
    unsigned char below_left, below_right;             /* 07-08 */
    unsigned char custom;                             /* 09 */
} ROOMREC;

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

typedef struct {
    int image, equipment_image;                       /* 00,02 */
    signed char x, y;                                 /* 04-05 */
    unsigned char flags;                              /* 06 */
} FRAMEREC;

extern LEVELREC Level;
extern FRAMEREC Frame;
extern unsigned char floor_mask;
extern int bridge_wait, bridge_break;
extern unsigned char falling_floors[];
extern signed char bridge_y[];
extern signed char bridge_images[];
extern int Col2PixelX[13], Row2PixelY[5];
static int floor_y_adjustment[] = {6, 20, 20, 20, 20, 20, 26, 36, 51, 87};
static int floor_x_adjustment[] = {8, 18, 24, 18, 24, 10, 20, 23, 19, 23};
extern Tile near *BlueS;
extern ImageTable near *image_table;
extern int Rnd(int);
int abs(int);
#pragma intrinsic(abs)
extern void AddSound(int), AddSong(int);
extern void copy_kid_to_char(void), load_actor_slot(char), save_actor_slot(void), get_frame_info(void);
extern int AddCharX(int); /* Column-to-X helper at 02cc:0360. */
extern void remove_char(void); /* Removes the current character at 02cc:1bd6. */
extern void start_opponent_running(void); /* Starts the RoomOpp running at overlay-12:2c3e:00d6. */
extern void turn_engarde(void), JumpSeq(int);
extern void trigger_gate_with_reverse_plate(char, char);
extern Gate near *get_trob_room_cell(char, int);
extern unsigned char ReadCell(char, char, char);
extern void set_resource(int, unsigned int, unsigned char, unsigned char);
extern int query_sound(int);

extern void DrawCustomRoomImgAtInfo(char, TileAnim near *);
extern void OFFSETRECT(void far *, void far *, int, int);
extern void ComputeOffsetCellRect(Rect near *, void near *, int, int);
typedef void far pascal DrawCallback(char cell, Rect near *rect);
extern DrawCallback mark_bridge_top, mark_bridge_floor;
extern void mark_bufs_for_rect(DrawCallback *, Rect near *, char);
extern void add_trob(char, char, char, char);
extern char far pascal FirstInRow(signed char);
extern signed char far pascal IndexCell1(signed char, signed char);
typedef struct {
    unsigned int width, height;
} ShapeSize;
extern ShapeSize far *GetCustomPieceSize(unsigned char, ShapeSize far *);
extern void draw_mob(char), get_mob_rect(Rect near *), update_mob_y(int), add_mob(void);
extern void far _fastcall draw_shape(char, int, void near *, int, char);
typedef struct {
    signed char index, flipped;                       /* 00-01 */
    int x, y, image;                                   /* 02,04,06 */
    unsigned char image_set;                          /* 08 */
    int clip_top, clip_left, clip_bottom, clip_right;    /* 09,0b,0d,0f */
    int depth;                                        /* 11 */
    unsigned char object_type;                        /* 13 */
} FCHARREC;

extern FCHARREC FChar;
extern unsigned int BlueSpec;
extern signed char animation_cell, animation_active;
typedef struct {
    int x, y;                                         /* 00,02 */
    unsigned char room;                               /* 04 */
    int velocity, auxiliary;                          /* 05,07 */
    signed char kind, row;                             /* 09-0a */
    unsigned int flags;                               /* 0b */
} MOBREC;

extern MOBREC Mob;
extern ROOMREC Room;
extern int mob_heights[], mob_widths[];

int bridge_watch_kid(void);
int bridge_watch_opponent(void);
void mark_bridge_floors_for_chars(int, int);
void bridge_check_break(int, int);
void trigger_bridge_floor_mob(unsigned char, signed char);
int bridge_char_on_floor(void);
void tweak_char_y_on_bridge(void);
int bridge_gate_open(void);
int bridge_skeleton_at_plate(void);
void mark_bridge_for_col(char);
void redraw_bridge_floor(int, char);
int far _fastcall offset_bridge_floor_y(unsigned char, char);
void far _fastcall restore_bridge_floor_y(unsigned char, int);

void bridge_frame(void)
{
    int kid_on_bridge = bridge_watch_kid();
    int opponent_on_bridge = bridge_watch_opponent();
    mark_bridge_floors_for_chars(kid_on_bridge, opponent_on_bridge);
    bridge_check_break(kid_on_bridge, opponent_on_bridge);
}

void bridge_check_break(int kid_on_bridge, int opponent_on_bridge)
{
    signed char col;

    if (bridge_break == 0) {
        if (kid_on_bridge && opponent_on_bridge) {
            if (abs(Kid.column - RoomOpp[0].column) <= 3) {
                ++bridge_wait;
                if (bridge_wait >= 60 && Kid.column >= 4 && Kid.column <= 6 &&
                    RoomOpp[0].column >= 5 && RoomOpp[0].column <= 6 && Kid.sequence != 85)
                    bridge_break = 1;
            } else if (bridge_wait != 0) {
                --bridge_wait;
            }
        } else if (bridge_wait > 1) {
            bridge_wait -= 2;
        }
    } else if (bridge_break > 0) {
        if (bridge_break == 3) {
            col = 7;
            do {
                if (!falling_floors[col]) {
                    trigger_bridge_floor_mob(10, col + 10);
                    break;
                }
            } while (--col >= 3);
            if (col == 7) {
                AddSound(68);
                AddSong(66);
            }
            if (col < 3 || Kid.sequence == 85) {
                bridge_break = -1;
                copy_kid_to_char();
                if (Char.column <= 4 && Char.direction == 0)
                    Char.x = AddCharX(10);
                Kid = Char;
            }
        } else {
            ++bridge_break;
        }
    }
}

char far pascal BridgeGetCharCol(char kind, char value)
{
    if ((*(volatile unsigned char *)&Level.level_number) == 5 && kind == 7)
        value -= 10;
    else if ((*(volatile unsigned char *)&Level.level_number) == 5 && kind == 12)
        value += 10;
    return value;
}

int bridge_watch_kid(void)
{
    int on_bridge;

    copy_kid_to_char();
    on_bridge = bridge_char_on_floor();
    if (on_bridge) {
        tweak_char_y_on_bridge();
        if (Char.frame == 15 || Char.frame == 185)
            on_bridge = 0;
    } else if (Char.room == 7 && Char.stance != -1 &&
               bridge_gate_open() && Char.column <= 5) {
        trigger_gate_with_reverse_plate(7, 13);
    }
    Kid = Char;
    return on_bridge;
}

int bridge_watch_opponent(void)
{
    int on_bridge;

    if (Level.room_objects[Room.number - 1].opponent_count) {
        load_actor_slot(0);
        on_bridge = bridge_char_on_floor();
        if (on_bridge)
            tweak_char_y_on_bridge();
        save_actor_slot();
    } else {
        on_bridge = 0;
    }
    return on_bridge;
}

int bridge_skeleton_must_advance(void)
{
    signed char col = BridgeGetCharCol(Char.room, Char.column);
    int advance;

    if (Char.direction == 0) {
        if (col < 2)
            advance = 1;
        else
            advance = 0;
        if (!advance)
            advance = bridge_skeleton_at_plate();
    } else {
        if (col >= 8)
            advance = 1;
        else
            advance = 0;
    }
    return advance;
}

int bridge_skeleton_progress(void)
{
    signed char col = BridgeGetCharCol(Char.room, Char.column);
    int keep_going = 1;

    if (Char.action != 5) {
        if (Char.row >= 3 && Char.action == 4) {
            remove_char();
            keep_going = 0;
        } else if (Char.direction == 0) {
            if (Char.sequence == 102 || Char.sequence == 216) {
                if (col >= 9) {
                    Char.alert_delay = 2;
                    Char.stance = 1;
                    JumpSeq(101);
                }
                keep_going = 0;
            } else if (Kid.x < Char.x) {
                if (bridge_skeleton_at_plate()) {
                    start_opponent_running();
                    keep_going = 0;
                } else if (col >= 9) {
                    turn_engarde();
                    keep_going = 0;
                }
            }
        } else if (bridge_skeleton_at_plate()) {
            turn_engarde();
            keep_going = 0;
        }
    }
    return keep_going;
}

int bridge_skeleton_at_plate(void)
{
    signed char col = BridgeGetCharCol(Opp.room, Opp.column);
    int open = bridge_gate_open();
    if (open) {
        if (Opp.direction == 0 && col < 2)
            open = 1;
        else if (Opp.direction == -1 && col < 2)
            open = 1;
        else
            open = 0;
    }
    return open;
}

int bridge_gate_open(void)
{
    register Gate near *gate = get_trob_room_cell(7, 13);
    int open;

    if (gate && (gate->state == 0 || gate->state == 2))
        open = 1;
    else
        open = 0;
    return open;
}

int bridge_skeleton_can_advance(void)
{
    signed char col = BridgeGetCharCol(Char.room, Char.column);
    int advance;
    if (Char.direction == 0) {
        if (col < 3)
            advance = 1;
        else
            advance = 0;
    } else {
        if (col > 3)
            advance = 1;
        else
            advance = 0;
    }
    return advance;
}

int bridge_char_on_floor(void)
{
    if (ReadCell(Char.room, Char.column, Char.row) == 44)
        return 1;
    return 0;
}

void load_bridge_code(void)
{
    if ((*(volatile unsigned char *)&Level.level_number) == 5 && ((signed char)Room.number) == 10)
        set_resource(3500, 64, 160, 1);
    floor_mask = 0;
    bridge_wait = 0;
}

void mark_bridge_floors_for_chars(int kid_on_bridge, int opponent_on_bridge)
{
    int delay;
    floor_mask = 0;
    delay = 2;
    if (kid_on_bridge && opponent_on_bridge) {
        floor_mask = 255;
        delay = 1;
    } else if (kid_on_bridge) {
        mark_bridge_for_col(Kid.column);
    } else if (opponent_on_bridge) {
        mark_bridge_for_col(RoomOpp[0].column);
    } else {
        delay = 0;
    }
    if (delay && !query_sound(10067) && !Rnd(delay * 40))
        AddSound(67);
}

void mark_bridge_for_col(char col)
{
    int right = col + 1;
    int left;

    right = (right > 8 ? 8 : right) - 1;
    left = col - 1;
    left = left < 1 ? 1 : left;
    col = left - 1;
    if (col <= right) {
        do {
            floor_mask |= 1 << col;
        } while (++col <= right);
    }
}

void tweak_char_y_on_bridge(void)
{
    int modifier;

    if (bridge_char_on_floor() && (Char.frame < 34 || Char.frame > 44)) {
        Char.y = 63 * Char.row + 56;
        Char.y += bridge_y[Char.column];
        get_frame_info();
        if (Frame.flags & 64) {
            modifier = Level.cell_info[9][(signed char)(Char.column + 10)].modifier & 15;
            if (modifier == 1)
                --Char.y;
            else if (modifier == 2)
                ++Char.y;
        }
    }
}

void unload_bridge_code(void)
{
    set_resource(3500, 64, 160, 0);
}

void animate_bridge_floor(void)
{
    unsigned char phase, old_phase;

    if ((*(volatile unsigned char *)&Level.level_number) == 5 && ((signed char)Room.number) == 10) {
        old_phase = BlueSpec & 15;
        BlueSpec &= 0xfff0;
        if (!(floor_mask & (1 << (animation_cell % (signed char)10 - 1)))) {
            phase = 0;
        } else {
            phase = Rnd(3);
            if (phase > 1) {
                if (old_phase)
                    phase = 0;
                else {
                    --phase;
                    BlueSpec |= phase;
                }
            } else {
                phase = old_phase;
            }
        }
        redraw_bridge_floor(phase, animation_cell);
    } else {
        animation_active = -1;
    }
}

void draw_bridge_floor(TileAnim near *tile)
{
    unsigned char image;
    int old_y;

    if (tile->type == 5 || tile->type == 2) {
        image = (tile->modifier & 15) + 3;
        old_y = offset_bridge_floor_y(image, tile->col);
        DrawCustomRoomImgAtInfo(image, tile);
        restore_bridge_floor_y(image, old_y);
    }
}

int far _fastcall offset_bridge_floor_y(unsigned char image, char col)
{
    Image near *entry = &image_table->images[image];
    int old_y = entry->y;
    int delta = bridge_y[col];

    entry->y += delta;
    OFFSETRECT(entry->rect, entry->rect, 0, delta);
    return old_y;
}

void far _fastcall restore_bridge_floor_y(unsigned char image, int old_y)
{
    Image near *entry = &image_table->images[image];
    int delta = entry->y - old_y;

    entry->y = old_y;
    OFFSETRECT(entry->rect, entry->rect, 0, -delta);
}

void redraw_bridge_floor(int phase, char cell)
{
    Rect rect;
    unsigned char image = phase + 3;
    signed char col = cell % (signed char)10;
    int old_y = offset_bridge_floor_y(image, col);

    ComputeOffsetCellRect(&rect, image_table->images[image].rect, col, cell / (signed char)10 + 1);
    --rect.top;
    mark_bufs_for_rect(&mark_bridge_top, &rect, -1);
    restore_bridge_floor_y(image, old_y);
}

void trigger_bridge_floor(char room, char cell)
{
    Tile near *tiles = BlueS;
    int phase = tiles[cell].modifier & 0xfff0;
    tiles[cell].modifier = phase;

    add_trob(room, cell, 1, 44);
    redraw_bridge_floor(phase, cell);
}

/* Draft: the original uses DI for x and SI for y. */
void animate_bridge_mob(void)
{
    int x = Mob.x, y = Mob.y;
    signed char room = Mob.room;
    signed char col, image;
    register int phase;
    int old_x, old_y;
    ShapeSize bounds;
    Rect redraw;

    if (room != ((signed char)Room.number)) {
        if ((signed char)Room.below == (signed char)Mob.room)
            y += 191;
        else if ((signed char)Mob.room == (signed char)Room.above)
            y -= 191;
        else if ((signed char)Mob.room == (signed char)Room.left)
            x -= 320;
        else if ((signed char)Mob.room == (signed char)Room.right)
            x += 320;
        else
            goto get_position;
        room = ((signed char)Room.number);
    }
get_position:
    col = x / 32;
    FChar.index = IndexCell1(col, y / 63);
    if (FChar.index != 30) {
        if (col)
            --FChar.index;
        if (FChar.index < 20)
            FChar.index += 10;
    }
    FirstInRow(Mob.row);
    phase = Mob.flags & 15;
    image = bridge_images[phase];
    if ((*(volatile unsigned char *)&Level.level_number) == 5 && room == 10) {
        old_y = Mob.y;
        Mob.y += floor_y_adjustment[phase];
        y += floor_y_adjustment[phase];
        old_x = Mob.x;
        Mob.x += floor_x_adjustment[phase];
        if (GetCustomPieceSize(image, &bounds)) {
            mob_heights[11] = bounds.width + 1;
            mob_widths[11] = bounds.height + 1;
        }
        draw_mob(0);
        if (Mob.velocity != -1) {
            get_mob_rect(&redraw);
            redraw.left += 16;
            mark_bufs_for_rect(&mark_bridge_floor, &redraw, -1);
            update_mob_y(y);
        }
        Mob.x = old_x;
        Mob.y = old_y;
    }
}

void draw_bridge_mob_object(int unused)
{
    draw_shape(4, (unsigned char)bridge_images[(*(unsigned char *)&FChar.image) & 15] + 1,
               (*(void near **)&FChar.x), FChar.y - 1, 10);
}

void step_bridge_mob(void)
{
    int phase, speed;

    if ((*(volatile unsigned char *)&Level.level_number) == 5 && ((signed char)Room.number) == 10) {
        phase = Mob.flags & 15;
        speed = (Mob.flags & 0x30) >> 4;
        if ((speed == 1 && phase != 4) || (speed == 2 && phase != 9)) {
            Mob.flags &= 0xfff0;
            Mob.flags |= phase + 1;
        } else if (speed == 2 && phase == 9) {
            Mob.velocity = -1;
        }
    } else {
        Mob.velocity = -1;
    }
}

void trigger_bridge_floor_mob(unsigned char room, signed char cell)
{
    signed char col, row;
    int speed;

    Level.cell_kinds[9][cell] = 0;
    Level.cell_info[9][cell].modifier = 0xc000;
    Mob.velocity = Mob.auxiliary = 0;
    col = cell % (signed char)10;
    row = cell / (signed char)10;
    Mob.x = Col2PixelX[col + 1];
    if (Rnd(1)) {
        Mob.flags = 5;
        speed = 2;
    } else {
        Mob.flags = 0;
        speed = 1;
    }
    falling_floors[col] = speed;
    Mob.flags |= speed << 4;
    Mob.room = room;
    Mob.row = row;
    Mob.y = Row2PixelY[row + 1];
    Mob.y += bridge_y[col];
    Mob.kind = 11;
    add_mob();
    draw_mob(0);
}
