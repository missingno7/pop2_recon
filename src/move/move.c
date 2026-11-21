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
static Trob trobs[20];

extern byte level_type;
typedef struct {
    unsigned state;
    unsigned extra;
} RoomObject;

extern RoomObject *room_objects;
extern RoomObject room_object;
extern byte *room_kinds;
extern byte tile_kind;

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
                sbyte *state = (sbyte *)&trobs + TrobStateOffset;
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

extern void select_room(byte room);
void select_trob_cell(int cell);
void animate_flask(void);
void animate_loose_floor(void);
void animate_exit(void);
void animate_torch(int index);
void animate_gate(void);
void animate_plate(void);
extern void animate_level_gate(void);
extern void animate_special_floor(void);
extern void animate_kind_23(void);
extern void animate_kind_27(void);
extern void animate_kind_28(void);
extern void animate_kind_29(void);
extern void animate_kind_30(void);
extern void animate_kind_31(void);
extern void animate_kind_36(void);
extern void animate_kind_37(void);
extern void animate_kind_38(void);
extern void animate_kind_39(void);
extern void animate_kind_40(void);
extern void animate_kind_41(void);
extern void animate_kind_42(void);
extern void animate_kind_43(void);
extern void animate_kind_44(void);

/* Draft. External provider names and same-file prototypes remain provisional. */
void AnimTrob(int index)
{
    select_room(current_trob.room);
    select_trob_cell(current_trob.cell);
    switch (current_trob.kind) {
    default:
        current_trob.state = -1;
        break;
    case 4:
        if (level_type == 1)
            animate_level_gate();
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
        animate_special_floor();
        break;
    case 17:
        animate_exit();
        break;
    case 19:
    case 32:
        animate_torch(index);
        break;
    case 23:
        animate_kind_23();
        break;
    case 27:
        animate_kind_27();
        break;
    case 28:
        animate_kind_28();
        break;
    case 29:
        animate_kind_29();
        break;
    case 30:
        animate_kind_30();
        break;
    case 31:
        animate_kind_31();
        break;
    case 36:
        animate_kind_36();
        break;
    case 37:
        animate_kind_37();
        break;
    case 38:
        animate_kind_38();
        break;
    case 39:
        animate_kind_39();
        break;
    case 40:
        animate_kind_40();
        break;
    case 41:
        animate_kind_41();
        break;
    case 42:
        animate_kind_42();
        break;
    case 43:
        animate_kind_43();
        break;
    case 44:
        animate_kind_44();
        break;
    }
    room_objects[current_trob.cell].state = room_object.state;
}

typedef struct {
    int edge[4];
} MoveRect;

typedef void (far *CellMark)(sbyte cell, MoveRect *rect);

extern MoveRect torch_rect, cave_torch_rect;
extern MoveRect gate_rect, temple_gate_rect;
extern MoveRect flask_rect, plate_rect, temple_plate_rect;
extern MoveRect *exit_rects[], *loose_rects[];
extern byte viewed_room, left_room, right_room, upper_room, lower_room;
extern byte upper_right_room, lower_right_room;
extern byte room_tiles[][30];
extern int tile_has_gate(byte tile);
extern sbyte inspect_trob_cell(byte room, int cell);
extern sbyte place_trob_rect(MoveRect *out, MoveRect *base, int tile,
                            byte room, int cell);
extern int visible_trob_column(int column);
int compute_trob_move_rect(MoveRect *out, MoveRect *base);
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
    MoveRect rect;
    sbyte cell;
    int cave = (level_type == 2);

    rect = *(cave ? &cave_torch_rect : &torch_rect);
    if (compute_trob_move_rect(&rect, &rect)) {
        if (cave) {
            mark_move(current_trob.cell, &rect);
            cell = check_trob_above();
            mark_move(cell, &rect);
            mark_move(check_trob_above_right(), &rect);
        }
        mark_move(check_trob_right(), &rect);
    }
}

void redraw_exit(void)
{
    MoveRect rect;
    sbyte cell;

    if (compute_trob_move_rect(&rect, exit_rects[level_type])) {
        mark_move(current_trob.cell, &rect);
        cell = check_trob_right();
        mark_move(cell, &rect);
        cell = check_trob_above();
        mark_move(cell, &rect);
        mark_move(check_trob_above_right(), &rect);
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
    MoveRect *base = level_type == 3 ? &temple_plate_rect : &plate_rect;

    if (compute_trob_move_rect(&rect, base))
        mark_bufs_for_rect(mark_move, &rect, -1);
}

void redraw_loose(void)
{
    MoveRect rect;
    sbyte cell;

    if (compute_trob_move_rect(&rect, loose_rects[level_type])) {
        cell = check_trob();
        mark_move(cell, &rect);
        mark_move(check_trob_right(), &rect);
        if (level_type == 3)
            mark_move(check_trob_below(), &rect);
    }
}

void redraw_gate(void)
{
    MoveRect rect;
    sbyte cell;

    if (level_type == 3)
        rect = temple_gate_rect;
    else
        rect = gate_rect;
    cell = check_trob_above_right();
    if (cell != 30 && cell >= 0 && !tile_has_gate(room_tiles[viewed_room][cell]))
        rect.edge[0] = 0;
    if (compute_trob_move_rect(&rect, &rect)) {
        mark_move(current_trob.cell, &rect);
        cell = check_trob_right();
        mark_move(cell, &rect);
        mark_move(check_trob_above_right(), &rect);
    }
}

void redraw_trob(MoveRect *base)
{
    MoveRect rect;
    sbyte cell;

    if (base && compute_trob_move_rect(&rect, base)) {
        cell = check_trob();
        mark_move(cell, &rect);
        mark_move(check_trob_right(), &rect);
    }
}

int compute_trob_move_rect(MoveRect *out, MoveRect *base)
{
    sbyte tile = inspect_trob_cell(current_trob.room, current_trob.cell);
    return visible_trob_column(place_trob_rect(out, base, tile,
                                            current_trob.room, current_trob.cell));
}

sbyte check_trob(void)
{
    sbyte cell = current_trob.cell;

    if (lower_room == current_trob.room) {
        if (cell >= 20 && cell < 30)
            cell = 19 - cell;
        else
            cell = 30;
    } else if (viewed_room != current_trob.room)
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

    if (viewed_room == current_trob.room) {
        if (cell % 10 == 0)
            cell = 30;
        else
            --cell;
    } else if (left_room == current_trob.room && cell % 10 == 0)
        cell += 9;
    else
        cell = 30;
    return cell;
}

sbyte check_trob_right(void)
{
    sbyte cell = current_trob.cell;

    if (viewed_room == current_trob.room) {
        if (cell % 10 == 9)
            cell = 30;
        else
            ++cell;
    } else if (right_room == current_trob.room) {
        if (cell % 10 == 9)
            cell -= 9;
        else
            cell = 30;
    } else if (lower_room == current_trob.room) {
        if (cell >= 20 && cell < 29)
            cell = 18 - cell;
        else
            cell = 30;
    } else
        cell = lower_right_room == current_trob.room && cell == 29 ? -1 : 30;
    return cell;
}

sbyte check_trob_above(void)
{
    sbyte cell = current_trob.cell;

    if (viewed_room == current_trob.room) {
        if (cell < 10)
            cell = -1 - cell;
        else
            cell -= 10;
    } else if (upper_room == current_trob.room && cell < 10)
        cell += 20;
    else
        cell = 30;
    return cell;
}

sbyte check_trob_above_right(void)
{
    sbyte cell = current_trob.cell;

    if (viewed_room == current_trob.room) {
        if (cell % 10 == 9)
            cell = 30;
        else if (cell < 10)
            cell = -2 - cell;
        else
            cell -= 9;
    } else if (right_room == current_trob.room) {
        if (cell == 9)
            cell = -1;
        else if (cell % 10 == 9)
            cell -= 19;
        else
            cell = 30;
    } else if (upper_room == current_trob.room) {
        if (cell < 9)
            cell += 21;
        else
            cell = 30;
    } else
        cell = upper_right_room == current_trob.room && cell == 9 ? 20 : 30;
    return cell;
}

sbyte check_trob_below(void)
{
    sbyte cell = current_trob.cell;

    if (viewed_room == current_trob.room) {
        if (cell < 20)
            cell += 10;
        else
            cell = 30;
    } else if (lower_room == current_trob.room && cell >= 20)
        cell %= 10;
    else
        cell = 30;
    return cell;
}

extern volatile byte actor_id;
extern unsigned actor_frame;
extern sbyte actor_action, actor_floor_contact, actor_direction;
extern int floor_override, actor_step, drew_pressed_tile;
extern byte current_tile_flags;
extern void check_floor_below(void);
extern void check_climbing_floor(void);
extern void check_special_floor(void);
extern sbyte locate_actor_tile(void);
extern sbyte locate_press_tile(void);
extern void damage_press_tile(void);
void break_loose_floor(sbyte sound);
void jam_plate(void);
void push_plate(sbyte sound, int link);

void check_floor(void)
{
    unsigned frame;

    if (actor_id == 7 || actor_id == 8 || actor_action == 9) {
        if (actor_floor_contact)
            check_special_floor();
    } else {
        frame = actor_frame;
        switch (actor_action) {
        case 2:
        case 6:
            break;
        case 3:
            if (frame >= 0x66 && frame < 0x6a)
                check_climbing_floor();
            return;
        case 4:
            check_special_floor();
            break;
        case 5:
            if (frame != 0x6d)
                break;
        default:
            check_floor_below();
            return;
        }
    }
}

void check_press(void)
{
    int action;
    unsigned frame;
    sbyte kind;

    if (actor_id == 11 || actor_id == 10 || (actor_id == 0 && floor_override)
            || (actor_id == 1 && actor_step >= 29 && actor_step <= 42))
        return;

    action = actor_action;
    frame = actor_frame;
    if ((frame >= 0x57 && frame <= 0x63) || (frame >= 0x87 && frame < 0x8d))
        kind = locate_actor_tile();
    else if (action < 2 || action == 7 || action == 5 || action == 8) {
        kind = locate_actor_tile();
        if (frame == 0x4f && kind == 11)
            break_loose_floor(0);
        else if (frame == 0x4f && kind == 15)
            damage_press_tile();
        else
            kind = (current_tile_flags & 0x40) ? locate_press_tile() : 0;
    } else
        kind = 0;

    if (kind == 5 || kind == 6 || kind == 34) {
        if (actor_direction >= 0 && actor_id != 4) {
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
            damage_press_tile();
    }
}

extern int skip_alternate_torches, animation_tick;
int get_torch_frame(int frame);

void animate_flask(void)
{
    if (tile_kind != 10) {
        current_trob.state = -1;
    } else if (current_trob.state >= 0 && check_trob_visibility()) {
        unsigned state = room_object.state & ~31U;
        int frame = room_object.state & 31;
        if (frame >= 27)
            frame = 0;
        else
            ++frame;
        room_object.state = state | frame;
        redraw_flask();
        return;
    }
}

void animate_torch(int index)
{
    if (check_trob_visibility()) {
        int frame = get_torch_frame((byte)room_object.state);
        room_object.state = (room_object.state & 0xff00U) + frame;
        if (!skip_alternate_torches || animation_tick % 2 + index)
            redraw_torch();
    }
}

extern byte current_level, collision_state, exit_flags;
extern int exit_sound_suppressed;
extern sbyte gate_close_speed[], gate_open_speed[], exit_close_speed[];
extern byte player_room;
extern unsigned player_frame;
extern sbyte player_column, player_row, player_action;
extern int limit_gate_opening(int height);
extern void play_move_sound(int sound), finish_exit_opening(int value);
void stop_gate(int height);
void check_gate_fingers(void);
int did_player_escape_guard(void);
void add_gate_sound(int sound);

void animate_gate(void)
{
    int height = room_object.state & 255;
    sbyte phase = current_trob.state;
    int sound;

    if (phase >= 4) {
        if (phase < 8)
            current_trob.state = ++phase;
        height -= gate_close_speed[phase];
        if (height <= 0) {
            sbyte row;
            sbyte col;
            if (level_type == 3) {
                row = current_trob.cell / 10;
                col = current_trob.cell % 10;
                if (current_trob.room != player_room) {
                    if (current_trob.room == right_room)
                        col -= 10;
                    else if (current_trob.room == left_room)
                        col += 10;
                }
            }
            if (level_type == 3 && player_frame == 118 && row == player_row
                    && (col == player_column || player_column - col == 1)) {
                height = 36;
                sound = 9;
            } else {
                height = 0;
                sound = level_type == 3 ? 9 : 15;
                check_gate_fingers();
                if (did_player_escape_guard())
                    collision_state = 4;
            }
            add_gate_sound(sound);
            current_trob.state = -1;
        }
    } else if (phase >= 0) {
        if (height == 255)
            stop_gate(height);
        else {
            height += gate_open_speed[phase];
            if (phase == 0 || phase == 1) {
                if (height <= 0) {
                    height = 0;
                    stop_gate(height);
                    check_gate_fingers();
                } else if (height < 200) {
                    add_gate_sound(level_type == 3 ? 7 : 4);
                    current_trob.state = 1;
                    if (level_type == 3)
                        height = limit_gate_opening(height);
                } else if (current_level == 5 && current_trob.room == 7)
                    height = 0;
            } else if (height < 200) {
                if (level_type == 3)
                    add_gate_sound(7);
                else if (height % 8 == 0)
                    add_gate_sound(4);
            } else if (phase < 3) {
                redraw_gate();
                height = 250;
                current_trob.state = 0;
                add_gate_sound(level_type == 3 ? 8 : 5);
            } else {
                height = 255;
                stop_gate(height);
            }
        }
    }
    if (phase >= 0)
        room_object.state = (room_object.state & 0xff00U) + height;
    if (height <= 200)
        redraw_gate();
}

typedef struct {
    byte record[64];
} ActorSnapshot;

extern ActorSnapshot actor_snapshot, player_snapshot;
extern sbyte actor_column, actor_row, actor_facing;
extern void refresh_actor_floor(void), set_gate_reaction(int sequence);
extern sbyte row_first_cell(sbyte row);

void stop_gate(int height)
{
    int actor_cell;

    current_trob.state = -1;
    add_gate_sound(level_type == 3 ? 8 : 6);
    refresh_actor_floor();
    actor_cell = row_first_cell(actor_row - 1) + actor_column;
    if (current_trob.cell == actor_cell && actor_facing == -1
            && actor_action == 2 && height == 0) {
        set_gate_reaction(47);
        player_snapshot = actor_snapshot;
    }
}

void check_gate_fingers(void)
{
    if (player_room == current_trob.room && (player_action == 2 || player_action == 6)) {
        int cell = (sbyte)(row_first_cell(player_row) + player_column);
        int delta = current_trob.cell - cell;
        if (delta == -10 || delta == -11)
            play_move_sound(31);
    }
}

typedef struct {
    byte before_column[9];
    sbyte column, row;
    byte before_room[3];
    byte room;
    byte before_direction[2];
    sbyte direction;
    byte tail[46];
} GuardPosition;

typedef struct {
    sbyte guard_count;
    byte tail[115];
} RoomGuardCount;

extern GuardPosition guards[];
extern RoomGuardCount room_guard_counts[];

int did_player_escape_guard(void)
{
    int escaped = 0;
    sbyte count = room_guard_counts[current_trob.room].guard_count;
    sbyte row = current_trob.cell / 10;
    sbyte col = current_trob.cell % 10;
    sbyte i;
    int distance = col - player_column;

    if (count && player_room == current_trob.room && player_row == row
            && (distance < 0 ? -distance : distance) < 5) {
        for (i = 0; i < count; ++i) {
            GuardPosition *guard = &guards[i];
            if (guard->row == row && guard->direction < 0 && guard->room == player_room) {
                if ((player_column > col && guard->column > col)
                        || (player_column <= col && guard->column <= col))
                    return 0;
                escaped = 1;
            }
        }
    }
    return escaped;
}

void animate_exit(void)
{
    int height = room_object.state & 255;

    if (current_trob.state >= 4) {
        ++current_trob.state;
        height -= exit_close_speed[current_trob.state];
        if (height < 0) {
            height = 0;
            current_trob.state = -1;
            play_move_sound(9);
        } else if (current_trob.state == 4 && (exit_flags & 1) && !exit_sound_suppressed)
            play_move_sound(7);
    } else if (current_trob.state >= 0) {
        ++height;
        if (height >= 42) {
            current_trob.state = -1;
            if (viewed_room != 4 || current_level != 13) {
                finish_exit_opening(0);
                play_move_sound(8);
            }
        } else if (viewed_room != 4 || current_level != 13)
            play_move_sound(7);
    }
    room_object.state = (room_object.state & 0xff00U) + height;
    redraw_exit();
}

extern byte exit_height;

int is_exit_open(void)
{
    if (exit_height >= 42)
        return 1;
    return 0;
}

extern int temple_torch_frame(int frame), standard_torch_frame(int frame);

int get_torch_frame(int frame)
{
    return level_type == 3 ? temple_torch_frame(frame) : standard_torch_frame(frame);
}

/* The rectangle marking, trigger and queue setup bodies at 0d38 through 14dc are pending. */

typedef struct {
    byte room;
    sbyte cell;
    byte timer, prev, next;
} TileLink;

extern TileLink links[];

int get_link_next(int link)
{
    return links[link].next;
}

int get_link_timer(int link)
{
    return links[link].timer;
}

int change_link_timer(int link, byte timer)
{
    return links[link].timer = timer;
}

int get_link_cell(int link)
{
    return links[link].cell;
}

int link_has_previous(int link)
{
    if (links[link].prev != 0xfd)
        return 1;
    return 0;
}

int link_has_next(int link)
{
    if (links[link].next != 0xfd)
        return 1;
    return 0;
}

byte get_link_room(int link)
{
    return links[link].room;
}

/* PushPlate and JamPlate at 15b4 through 16ac are pending. */

void animate_plate(void)
{
    if (current_trob.state >= 0) {
        int link = room_object.state & 255;
        sbyte timer = get_link_timer(link);
        if (timer < 31) {
            change_link_timer(link, timer - 1);
            if (timer < 2) {
                current_trob.state = -1;
                redraw_plate();
                room_object.state &= ~0x0800U;
                return;
            }
        } else
            current_trob.state = -1;
    }
}

/* CloseExit at 16fa is pending. */

#pragma pack(1)
typedef struct {
    int x, y;
    byte room;
    int speed, auxiliary;
    byte kind;
    sbyte row;
    int flags;
} Mob;
#pragma pack()

extern Mob current_mob;
void clear_loose_cell(byte room, sbyte cell);
void add_mob(void);

void animate_loose_floor(void)
{
    int frame;
    int kind;

    ++room_object.state;
    frame = room_object.state & 15;
    if (current_trob.state != -1) {
        if (room_object.state & 0x40) {
            if (frame >= 4) {
                current_trob.state = -1;
                room_object.state = (room_object.state & 0xff00U) | (room_object.state & 0xb0U);
            }
        } else if (frame >= 12) {
            kind = level_type == 3 ? (room_object.state & 0x80) >> 7 : 3;
            clear_loose_cell(current_trob.room, current_trob.cell);
            room_object.state = kind + 3;
            current_mob.x = (current_trob.cell % 10) * 32;
            current_mob.row = current_trob.cell / 10;
            current_mob.y = current_mob.row * 63 + 66;
            current_mob.room = current_trob.room;
            current_mob.auxiliary = 0;
            current_mob.speed = current_trob.state;
            current_mob.kind = kind;
            current_mob.flags = 0;
            add_mob();
            current_trob.state = -1;
        }
    }
    redraw_loose();
}

/* Loose-floor and mobile-list bodies at 17ec through 1a10 are pending. */

void select_trob_cell(int cell)
{
    room_object = room_objects[cell];
    tile_kind = room_kinds[cell];
}

/* Mobile animation and redraw bodies at 1a42 through 255c are pending. */

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
    Trob *result = 0;
    sbyte found = 0;
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

    if (room == viewed_room || (room == right_room && cell % 10 == 9)
            || (room == upper_room && cell < 10)
            || (room == upper_right_room && cell == 9))
        visible = 1;
    else
        visible = 0;
    return visible;
}
