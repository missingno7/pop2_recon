/* BRIDGE.C, DOS overlay 14. Names and packed layouts reflect the observed uses. */
#pragma pack(1)

typedef struct Character {
    signed char state, direction;
    int x, y;
    signed char action;
    unsigned int frame;
    signed char col, row, sequence, unknown_c, unknown_d, room;
    signed char unknown_f, alive;
    char unknown_11[8];
    unsigned int sequence_id;
    char unknown_1b[8];
    unsigned char control;
    char other[28];
} Character;

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

extern Character Char, Kid, RoomOpp, Opp;
extern volatile unsigned char STATE_LEVEL;
extern signed char viewed_room, room_below, room_above, room_left, room_right;
extern unsigned char room_flags[], bridge_room_flags;
extern unsigned char floor_mask;
extern int bridge_wait, bridge_break;
extern unsigned char falling_floors[];
extern signed char bridge_y[];
extern signed char bridge_images[];
extern int floor_x[], row_y[];
static int floor_y_adjustment[] = {6, 20, 20, 20, 20, 20, 26, 36, 51, 87};
static int floor_x_adjustment[] = {8, 18, 24, 18, 24, 10, 20, 23, 19, 23};
extern Tile tile_state[];
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
extern void near *foreground_tiles;
extern int foreground_index;
extern unsigned char foreground_modifier;
extern unsigned int BlueSpec;
extern signed char animation_cell, animation_active;
extern int mob_x, mob_y, mob_index, mob_slot;
extern signed char mob_room, mob_type, mob_row;
extern unsigned int mob_modifier;
extern signed char floor_cell;
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
            if (abs(Kid.col - RoomOpp.col) <= 3) {
                ++bridge_wait;
                if (bridge_wait >= 60 && Kid.col >= 4 && Kid.col <= 6 &&
                    RoomOpp.col >= 5 && RoomOpp.col <= 6 && Kid.sequence_id != 85)
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
            if (col < 3 || Kid.sequence_id == 85) {
                bridge_break = -1;
                copy_kid_to_char();
                if (Char.col <= 4 && Char.direction == 0)
                    Char.x = AddCharX(10);
                Kid = Char;
            }
        } else {
            ++bridge_break;
        }
    }
}

extern volatile unsigned char STATE_LEVEL;

char far pascal BridgeGetCharCol(char kind, char value)
{
    if (STATE_LEVEL == 5 && kind == 7)
        value -= 10;
    else if (STATE_LEVEL == 5 && kind == 12)
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
    } else if (Char.room == 7 && Char.alive != -1 &&
               bridge_gate_open() && Char.col <= 5) {
        trigger_gate_with_reverse_plate(7, 13);
    }
    Kid = Char;
    return on_bridge;
}

int bridge_watch_opponent(void)
{
    int on_bridge;

    if (room_flags[(unsigned char)viewed_room * 116]) {
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
    signed char col = BridgeGetCharCol(Char.room, Char.col);
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
    signed char col = BridgeGetCharCol(Char.room, Char.col);
    int keep_going = 1;

    if (Char.sequence != 5) {
        if (Char.row >= 3 && Char.sequence == 4) {
            remove_char();
            keep_going = 0;
        } else if (Char.direction == 0) {
            if (Char.sequence_id == 102 || Char.sequence_id == 216) {
                if (col >= 9) {
                    Char.control = 2;
                    Char.alive = 1;
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
    signed char col = BridgeGetCharCol(Opp.room, Opp.col);
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
    signed char col = BridgeGetCharCol(Char.room, Char.col);
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
    if (ReadCell(Char.room, Char.col, Char.row) == 44)
        return 1;
    return 0;
}

void load_bridge_code(void)
{
    if (STATE_LEVEL == 5 && viewed_room == 10)
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
        mark_bridge_for_col(Kid.col);
    } else if (opponent_on_bridge) {
        mark_bridge_for_col(RoomOpp.col);
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
        Char.y += bridge_y[Char.col];
        get_frame_info();
        if (bridge_room_flags & 64) {
            modifier = tile_state[(signed char)(Char.col + 10)].modifier & 15;
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

    if (STATE_LEVEL == 5 && viewed_room == 10) {
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
    int x = mob_x, y = mob_y;
    signed char room = mob_room;
    signed char col, image;
    register int phase;
    int old_x, old_y;
    ShapeSize bounds;
    Rect redraw;

    if (room != viewed_room) {
        if (room_below == mob_room)
            y += 191;
        else if (mob_room == room_above)
            y -= 191;
        else if (mob_room == room_left)
            x -= 320;
        else if (mob_room == room_right)
            x += 320;
        else
            goto get_position;
        room = viewed_room;
    }
get_position:
    col = x / 32;
    floor_cell = IndexCell1(col, y / 63);
    if (floor_cell != 30) {
        if (col)
            --floor_cell;
        if (floor_cell < 20)
            floor_cell += 10;
    }
    FirstInRow(mob_row);
    phase = mob_modifier & 15;
    image = bridge_images[phase];
    if (STATE_LEVEL == 5 && room == 10) {
        old_y = mob_y;
        mob_y += floor_y_adjustment[phase];
        y += floor_y_adjustment[phase];
        old_x = mob_x;
        mob_x += floor_x_adjustment[phase];
        if (GetCustomPieceSize(image, &bounds)) {
            mob_heights[11] = bounds.width + 1;
            mob_widths[11] = bounds.height + 1;
        }
        draw_mob(0);
        if (mob_index != -1) {
            get_mob_rect(&redraw);
            redraw.left += 16;
            mark_bufs_for_rect(&mark_bridge_floor, &redraw, -1);
            update_mob_y(y);
        }
        mob_x = old_x;
        mob_y = old_y;
    }
}

void draw_bridge_mob_object(int unused)
{
    draw_shape(4, (unsigned char)bridge_images[foreground_modifier & 15] + 1,
               foreground_tiles, foreground_index - 1, 10);
}

void step_bridge_mob(void)
{
    int phase, speed;

    if (STATE_LEVEL == 5 && viewed_room == 10) {
        phase = mob_modifier & 15;
        speed = (mob_modifier & 0x30) >> 4;
        if ((speed == 1 && phase != 4) || (speed == 2 && phase != 9)) {
            mob_modifier &= 0xfff0;
            mob_modifier |= phase + 1;
        } else if (speed == 2 && phase == 9) {
            mob_index = -1;
        }
    } else {
        mob_index = -1;
    }
}

extern unsigned char room_foregrounds[][30];

void trigger_bridge_floor_mob(unsigned char room, signed char cell)
{
    signed char col, row;
    int speed;

    room_foregrounds[10][cell] = 0;
    tile_state[cell].modifier = 0xc000;
    mob_index = mob_slot = 0;
    col = cell % (signed char)10;
    row = cell / (signed char)10;
    mob_x = floor_x[col];
    if (Rnd(1)) {
        mob_modifier = 5;
        speed = 2;
    } else {
        mob_modifier = 0;
        speed = 1;
    }
    falling_floors[col] = speed;
    mob_modifier |= speed << 4;
    mob_room = room;
    mob_row = row;
    mob_y = row_y[row];
    mob_y += bridge_y[col];
    mob_type = 11;
    add_mob();
    draw_mob(0);
}
