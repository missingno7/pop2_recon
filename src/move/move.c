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

extern byte level_type;
typedef struct {
    unsigned state;
    unsigned extra;
} RoomObject;

extern RoomObject * volatile room_objects;
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
    int edges[4];
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
    struct { MoveRect rect; union { int cave; struct { byte pad; sbyte cell; } bytes; } tail; } work;
    work.tail.cave = (level_type == 2);

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

    if (compute_trob_move_rect(&work.rect, exit_rects[level_type])) {
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

    if (level_type == 3)
        source = &temple_plate_rect;
    else
        source = &plate_rect;
    if (compute_trob_move_rect(&rect, source))
        mark_bufs_for_rect(mark_move, &rect, -1);
}

void redraw_loose(void)
{
    struct { MoveRect rect; byte pad; sbyte cell; } work;

    if (compute_trob_move_rect(&work.rect, loose_rects[level_type])) {
        mark_move(work.cell = check_trob(), &work.rect);
        mark_move(check_trob_right(), &work.rect);
        if (level_type == 3)
            mark_move(check_trob_below(), &work.rect);
    }
}

void redraw_gate(void)
{
    struct { MoveRect rect; byte pad; sbyte cell; } work;

    if (level_type == 3)
        work.rect = temple_gate_rect;
    else
        work.rect = gate_rect;
    work.cell = check_trob_above_right();
    if (work.cell != 30 && work.cell >= 0 && !tile_has_gate(room_tiles[viewed_room][work.cell]))
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

int compute_trob_move_rect(register MoveRect *out, MoveRect *base)
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
            return cell = 30;
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

    if (actor_id == 7 || actor_id == 8 || actor_action == 9)
        goto falling;

    frame = actor_frame;
    switch (actor_action) {
    default:
        check_floor_below();
        return;
    case 2:
    case 6:
        break;
    case 3:
        if (frame >= 0x66 && frame < 0x6a) {
            check_climbing_floor();
            return;
        }
        break;
    case 4:
        goto update_fall;
    case 5:
        if (frame == 0x6d)
            check_floor_below();
        break;
    }
    return;

falling:
    if (!actor_floor_contact)
        return;
update_fall:
    check_special_floor();
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
            if (!(current_tile_flags & 0x40))
                kind = 0;
            else
                kind = locate_press_tile();
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

    if (phase >= 0) {
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
        } else {
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
        room_object.state = (room_object.state & 0xff00U) + height;
    }
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
        int delta = current_trob.cell - (sbyte)(row_first_cell(player_row) + player_column);
        if (delta != -10 && delta != -11)
            return;
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
                    goto caught;
                escaped = 1;
            }
        }
    }
    goto done;
caught:
    escaped = 0;
done:
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

extern unsigned pressed_state;
#define exit_height (*(byte *)&pressed_state)

int IsExitOpen(void)
{
    if (exit_height >= 42)
        return 1;
    return 0;
}

extern int temple_torch_frame(int frame), standard_torch_frame(int frame);

int get_torch_frame(int frame)
{
    int result;
    if (level_type == 3)
        result = temple_torch_frame(frame);
    else
        result = standard_torch_frame(frame);
    return result;
}

typedef struct {
    int dirty;
    MoveRect rect;
} MarkedRect;

extern MarkedRect top_marks[10], move_marks[30], floor_marks[30], foreground_marks[30];
extern int half_marks[30], redraw_marks[];
extern volatile byte marked_room;
extern sbyte marked_cell;

extern int rect_empty(MoveRect far *rect);
extern sbyte room_cell(sbyte column, sbyte row);
extern void get_cell_rect(MoveRect far *out, int column, int row);
extern int intersect_rect(MoveRect far *out, MoveRect far *a, MoveRect far *b);
extern void union_rect(MoveRect far *out, MoveRect far *a, MoveRect far *b);

void mark_rect_buf(MarkedRect *mark, int cell, MoveRect *clip);
void mark_guard_updates(MoveRect *clip, sbyte except);

void mark_top(int cell, MoveRect *clip)
{
    MoveRect rect;

    if (cell <= -1 && cell >= -10) {
        get_cell_rect(&rect, -((cell + 1) % 10), -1);
        if (!clip || intersect_rect(&rect, &rect, clip)) {
            ++cell;
            cell = -cell;
            if (top_marks[cell].dirty)
                union_rect(&top_marks[cell].rect, &top_marks[cell].rect, &rect);
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
    if (cell < 30) {
        if (cell >= 0)
            *(int *)((byte *)redraw_marks + (cell << 1)) = 1;
        else
            mark_top(cell, 0);
    }
}

void mark_cell_redraw(void)
{
    sbyte cell = marked_cell;

    if (viewed_room == marked_room)
        ;
    else if (right_room == marked_room)
        cell -= 9;
    else if (left_room == marked_room)
        cell += 9;
    else if (lower_room == marked_room)
        cell -= 20;
    else if (upper_room == marked_room)
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

    if (!rect_empty(rect)) {
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
                mark(room_cell(column, row), rect);
        }
        mark_guard_updates(rect, except);
    }
}

#pragma pack(1)
typedef struct {
    sbyte id;
    byte before_rect[26];
    MoveRect rect;
    byte before_update[19];
    int update;
    byte tail[8];
} GuardMark;
#pragma pack()

extern GuardMark guard_marks[];
extern RoomGuardCount room_guard_counts[];

void mark_guard_updates(MoveRect *clip, sbyte except)
{
    MoveRect hit;
    sbyte count = room_guard_counts[viewed_room].guard_count;
    int i;

    for (i = 0; i < count; ++i) {
        if (guard_marks[i].id != except)
            guard_marks[i].update |= intersect_rect(&hit, &guard_marks[i].rect, clip);
    }
}

void mark_rect_buf(MarkedRect *mark, int cell, MoveRect *clip)
{
    MoveRect rect;

    get_cell_rect(&rect, cell % 10, cell / 10);
    if (intersect_rect(&rect, &rect, clip)) {
        if (mark->dirty)
            union_rect(&mark->rect, &mark->rect, &rect);
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
extern unsigned random_below(unsigned limit);

void trigger_link(int link, int action)
{
    byte room, kind;
    sbyte cell, state;

    if ((int)get_link_room(link) != -2 && link != 0xfd) {
        do {
            room = get_link_room(link);
            select_room(room);
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
    unsigned phase = random_below(26) + 1;

    room_objects[cell].state = (room_objects[cell].state & ~31U) | phase;
    add_trob(room, cell, 1, 10);
}

extern byte gate_lock_room, actor_room;
extern sbyte trigger_style_gate(byte room, sbyte cell);

sbyte trigger_gate(byte room, sbyte cell, int action)
{
    sbyte state = -1;
    RoomObject *object = &room_objects[cell];
    unsigned value = object->state;
    unsigned height = value & 255;
    unsigned flags = value & 0xff00U;

    switch (action) {
    case 5:
        if (height == 255)
            break;
        if (height >= 200) {
            object->state = flags + 250;
            if (actor_direction >= 0 || (gate_lock_room && gate_lock_room != actor_room)) {
                room_objects[cell].state = flags + 255;
                state = 3;
            }
        } else {
            object->state = flags + ((height + 3) & 0xfc);
            if (actor_direction >= 0 || (gate_lock_room && gate_lock_room != actor_room))
                state = 3;
            else
                state = 2;
        }
        break;
    case 6:
        if (!get_trob_room_cell(room, cell) && height)
            state = 4;
        break;
    case 14:
        if (height < 200)
            state = 3;
        else
            object->state = flags + 255;
        break;
    case 34:
        state = trigger_style_gate(room, cell);
        break;
    }
    return state;
}

extern byte exit_room, exit_unlocked, trigger_flags;
extern const char exit_sound_name[];
extern int has_sound_override(const char *name);
extern void queue_move_sound(int sound);
extern void trigger_desert_floor(byte room, sbyte cell, sbyte state);
extern sbyte trigger_bridge_floor(byte room, sbyte cell);
extern void trigger_style_floor(byte room, sbyte cell);

sbyte trigger_object(byte kind, byte room, sbyte cell, int action)
{
    sbyte state;

    switch (kind) {
    case 4:
        state = trigger_gate(room, cell, action);
        break;
    case 17:
        if (room != exit_room && current_level == 8 && exit_unlocked != 1) {
            state = -1;
        } else {
            state = (byte)room_objects[cell].state < 1 ? 1 : -1;
            if (state == 1 && !has_sound_override(exit_sound_name) && current_level != 13)
                queue_move_sound(27);
        }
        break;
    case 25:
        state = action == 5 ? 2 : 1;
        trigger_desert_floor(room, cell, state);
        state = -1;
        break;
    case 27:
        state = trigger_bridge_floor(room, cell);
        break;
    case 36:
        if (!(trigger_flags & 8))
            trigger_style_floor(room, cell);
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
    unsigned phase = random_below(level_type == 3 ? 8 : 3);
    unsigned state = (room_objects[cell].state & 0xff00U) + phase;
    room_objects[cell].state = state;
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

extern byte get_room_tile(byte room, sbyte column, sbyte row);
extern int is_wall_tile(byte tile);

sbyte find_wall_column(byte room, sbyte column, sbyte row, sbyte step)
{
    do {
        column += step;
        if (is_wall_tile(get_room_tile(room, column, row)))
            break;
    } while (column >= 0 && column <= 10);
    return column;
}

extern int temple_slide_rock_x(void);

int get_slide_rock_x(void)
{
    int x;

    if (level_type == 2)
        x = temple_slide_rock_x();
    else
        x = 0;
    return x;
}

typedef struct {
    byte room;
    sbyte cell;
    byte timer, prev, next;
} TileLink;

static TileLink links[255];

int get_link_next(int link)
{
    return links[link].next;
}

int GetLinkTimer(int link)
{
    return links[link].timer;
}

int ChangeLinkTimer(int link, byte timer)
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

extern byte pressed_kind;
extern RoomObject room_records[][30];

void push_plate(sbyte kind, int link)
{
    sbyte previous_timer;
    sbyte cell;
    byte room;
    unsigned state;

    if (!kind)
        kind = pressed_kind;
    if (link == -1)
        link = pressed_state & 255;
    previous_timer = (sbyte)GetLinkTimer(link);
    if (previous_timer != 31) {
        cell = marked_cell;
        state = pressed_state | 0x0800U;
        room = marked_room;
        ChangeLinkTimer(link, 5);
        if (previous_timer < 2) {
            add_trob(marked_room, marked_cell, 1, kind);
            if (actor_frame != 185 && (marked_room != 4 || current_level != 13))
                play_move_sound(3);
            redraw_plate();
            trigger_link(link, (byte)kind);
            drew_pressed_tile = 1;
        } else {
            trigger_link(link, (byte)kind);
        }
        room_records[room][cell].state = state;
    } else {
        trigger_link(link, (byte)kind);
    }
}

void jam_plate(void)
{
    int link = pressed_state & 255;

    ChangeLinkTimer(link, 31);
    push_plate(pressed_kind, link);
}

void animate_plate(void)
{
    if (current_trob.state >= 0) {
        int link = room_object.state & 255;
        sbyte timer = GetLinkTimer(link);
        if (timer < 31) {
            ChangeLinkTimer(link, timer - 1);
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

void close_exit(byte room, sbyte cell)
{
    if (level_type != 1) {
        RoomObject *object = &room_objects[cell];
        object->state = ((unsigned)((byte *)&object->state)[1] << 8) + 42;
        add_trob(room, cell, 4, 17);
    }
}

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
void make_space(byte room, sbyte cell);
void add_mob(void);
extern Mob mobs[];
extern int mob_count;

void animate_loose_floor(void)
{
    int frame;
    int kind;
    sbyte row;

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
            make_space(current_trob.room, current_trob.cell);
            room_object.state = kind + 3;
            current_mob.x = (current_trob.cell % 10) * 32;
            row = current_trob.cell / 10;
            current_mob.y = row * 63 + 66;
            current_mob.room = current_trob.room;
            *(volatile int *)&current_mob.auxiliary = 0;
            current_mob.speed = current_trob.state;
            current_mob.kind = kind;
            current_mob.row = row;
            current_mob.flags = 0;
            add_mob();
            current_trob.state = -1;
        }
    }
    redraw_loose();
}

extern unsigned char current_level;

void make_space(unsigned char room, signed char cell)
{
    int index = cell;
    unsigned state;

    room_kinds[index] = 0;
    if (current_level == 5 && room == 3)
        state = 0xc000;
    else
        state = (room_objects[*(volatile int *)&index].state & 0x80) + 3;
    room_objects[*(volatile int *)&index].state = state;
}

extern sbyte pressed_source, pressed_row, actor_press_source;
extern byte shake_kind;
extern byte select_shake_cell(byte room, sbyte column, sbyte row);
extern void damage_crystal(void);
extern void shake_style_row(byte room, sbyte row);

void shake_loose_floor(void);

void break_loose_floor(sbyte sound)
{
    if (actor_id == 1 && sound == 0) {
        shake_loose_floor();
    } else {
        unsigned state = pressed_state & ~0x40U;
        if (sound)
            state = (state & ~4U) | 11;
        room_objects[marked_cell].state = state;
        add_trob(marked_room, marked_cell, sound, 11);
        play_move_sound(20);
    }
}

void shake_loose_floor(void)
{
    if (!(pressed_state & 15)) {
        int counter = ((pressed_state & 0x30) >> 4) + 1;

        if ((counter != 3 && pressed_source != actor_press_source) || actor_id == 1) {
            pressed_state &= ~0x30U;
            pressed_state |= (counter | 4) << 4;
            room_objects[marked_cell].state = pressed_state | 0x40;
        }
        add_trob(marked_room, marked_cell, 1, 11);
        play_move_sound(20);
    }
}

void shake_row(byte room, sbyte row)
{
    Mob saved = current_mob;
    sbyte column = 0;

    do {
        switch (select_shake_cell(room, column, row)) {
        case 11:
            shake_loose_floor();
            break;
        case 15:
            damage_press_tile();
            break;
        case 26:
            damage_crystal();
            break;
        }
    } while (++column < 10);

    if (shake_kind == 2)
        shake_style_row(room, row);
    current_mob = saved;
}

void add_mob(void)
{
    if (mob_count < 30)
        mobs[mob_count++] = current_mob;
}

Mob *get_mob_type_item(byte kind, int occurrence)
{
    Mob *found = 0;
    int index, count;

    for (count = index = 0; index < mob_count && found == 0; ++index) {
        if (kind == mobs[index].kind && mobs[index].speed != -1) {
            if (++count == occurrence)
                found = &mobs[index];
        }
    }
    return found;
}


void select_trob_cell(int cell)
{
    room_object = room_objects[cell];
    tile_kind = room_kinds[cell];
}

extern int mob_index;
static struct {
    int heights[13];
    int widths[13];
} mob_dimensions = {
    {18, 18, 2, 16, 27, 39, 69, 69, 12, 12, 37, 37, 18},
    {50, 50, 4, 57, 27, 31, 52, 52, 24, 24, 69, 57, 17}
};

#define mob_heights mob_dimensions.heights
#define mob_widths mob_dimensions.widths
extern void animate_mob_kind_2(void);
extern void animate_mob_kind_4(void);
extern void animate_mob_kind_6(void);
extern void check_mob_kind_6(void);
extern void animate_mob_kind_10(void);
extern void animate_mob_kind_11(void);
extern void animate_mob_kind_12(void);

void animate_mob(void);
void mob_loose(void);
void check_loose_crush(void);
void make_rubble(void);
void knock_loose(void);
void PassThru(void);
void mark_mob_redraw(sbyte layer);

void anim_mobs(void)
{
    struct {
        Mob *destination;
        int kept, index;
        union {
            int count;
            Mob *next;
        } pass;
    } work;

    mob_index = 0;
    work.pass.count = mob_count;
    for (; mob_index < work.pass.count; ++mob_index) {
        current_mob = mobs[mob_index];
        animate_mob();
        mobs[mob_index] = current_mob;
    }

    work.kept = work.index = 0;
    if (work.index < mob_count) {
        int *speed = &mobs[0].speed;

        work.destination = mobs;
        do {
            if (*speed != -1) {
                work.pass.next = work.destination++;
                ++work.kept;
                *work.pass.next = *(Mob *)((byte *)speed - (unsigned)&((Mob *)0)->speed);
            }
            speed = (int *)((byte *)speed + sizeof(Mob));
        } while (++work.index < mob_count);
    }
    mob_count = work.kept;
}

void animate_mob(void)
{
    switch (current_mob.kind) {
    case 0:
    case 1:
    case 3:
        mob_loose();
        if (current_mob.speed <= 0)
            ++current_mob.speed;
        check_loose_crush();
        break;
    case 2:
        animate_mob_kind_2();
        return;
    case 4:
        animate_mob_kind_4();
        return;
    case 6:
        animate_mob_kind_6();
        check_mob_kind_6();
        return;
    case 10:
        animate_mob_kind_10();
        return;
    case 11:
        animate_mob_kind_11();
        return;
    case 12:
        animate_mob_kind_12();
        return;
    }
}

void mob_loose(void)
{
    byte kind;

    if (current_mob.speed >= 0) {
        if (current_mob.speed < 29)
            current_mob.speed += 3;
        current_mob.y += current_mob.speed;

        if (!current_mob.room) {
            if (mob_heights[current_mob.kind] + 192 <= current_mob.y) {
                current_mob.speed = -2;
                return;
            }
        } else if ((current_mob.row + 1) * 63 <= current_mob.y) {
            kind = select_shake_cell(current_mob.room, current_mob.x / 32, current_mob.row);
            if (kind == 11)
                knock_loose();
            else if (kind == 15)
                damage_press_tile();

            if (kind != 11 && kind != 15 && !tile_has_gate(kind)) {
                play_move_sound(2);
                shake_row(current_mob.room, current_mob.row);
                current_mob.y = (current_mob.row + 1) * 63;
                current_mob.speed = -2;
                mobs[mob_index].flags = current_mob.flags = 1;
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
    work.kind = select_shake_cell(current_mob.room, current_mob.x / 32, current_mob.row);
    work.room = marked_room;
    work.column = pressed_source;
    work.row = pressed_row;
    work.cell = marked_cell;

    switch (work.kind) {
    case 5:
        work.plate = room_tiles[work.room][work.cell] = 14;
    case 6:
    case 34:
        push_plate(work.plate, -1);
        if (work.kind == 34)
            room_records[work.room][work.cell].state = pressed_state | 0x1800;
        select_shake_cell(current_mob.room, current_mob.x / 32, current_mob.row);
        *(byte *)&pressed_state = 0;
        if (work.kind == 5)
            *(byte *)&pressed_state |= 4;
        else if (work.kind == 6)
            *(byte *)&pressed_state |= 8;
        work.kind = pressed_kind;
    case 1:
    case 2:
    case 10:
    case 36:
        if (work.kind == 10) {
            Trob *flask = get_trob_room_cell(marked_room, marked_cell);
            if (flask) {
                current_trob = *flask;
                redraw_flask();
            }
        }
    case 19:
    case 32:
        work.kind = select_shake_cell(work.room, work.column, work.row);
        if (work.kind == 19 || work.kind == 32) {
            room_tiles[work.room][work.cell] = 32;
        } else {
            if (work.kind == 10 || work.kind == 1)
                pressed_state = 0;
            work.index = work.room * 30 + work.cell;
            (&room_tiles[0][0])[work.index] = 14;
            if (level_type == 4)
                (&room_records[0][0])[work.index].state = pressed_state | 0x40;
            else if ((level_type == 3 || level_type == 2) && work.kind != 1)
                (&room_records[0][0])[work.index].state = 0;
        }
        break;
    case 12:
    case 13:
        room_records[work.room][work.cell].state = pressed_state | 0x20;
        break;
    default:
        return;
    }
    mark_mob_redraw(0);
}

void knock_loose(void)
{
    make_space(marked_room, marked_cell);
    current_mob.speed >>= 1;
    mobs[mob_index] = current_mob;
    current_mob.y = current_mob.row * 63 + 66;
    mark_mob_redraw(0);
    current_mob.y = mobs[mob_index].y + 6;
    PassThru();
    add_mob();
    current_mob = mobs[mob_index];
    mark_mob_redraw(0);
}



typedef struct {
    unsigned char left, right, upper, lower;
} RoomLinks;

extern RoomLinks room_links[];

void PassThru(void)
{
    ++current_mob.row;
    if (current_mob.row >= 3) {
        current_mob.y -= 192;
        current_mob.row = 0;
        current_mob.room = room_links[current_mob.room].lower;
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

extern void add_mob_kind_2(void);
extern void add_mob_kind_4(void);
extern void add_mob_kind_6(void);
extern void add_mob_kind_10(void);
extern void add_mob_kind_11(void);
extern void add_mob_kind_12(void);
extern byte lower_left_room, upper_left_room;
extern MoveRect empty_mob_rect;
extern void offset_move_rect(MoveRect far *out, MoveRect far *base, int x, int y);
extern sbyte room_row_at_y(int y);
extern sbyte floor_row_for_draw(sbyte column, sbyte row);
extern sbyte mob_floor_row;

int is_loose_pushing_plate(void)
{
    byte kind = level_type == 3 ? 1 : 3;
    int occurrence = 0;
    int pushing = 0;
    Mob *mob;

    do {
        mob = get_mob_type_item(kind, ++occurrence);
        if (mob && mob->room == marked_room && mob->row == pressed_row
                && mob->x / 32 == pressed_source && mob->flags)
            pushing = 1;
    } while (mob && !pushing);
    return pushing;
}

void add_mobs(void)
{
    int index = 0;
    Mob *entry;

    if (index < mob_count) {
        entry = mobs;
        do {
            current_mob = *entry;
            add_mob_to_frame();
            *entry = current_mob;
            ++entry;
        } while (++index < mob_count);
    }
}

void add_mob_to_frame(void)
{
    switch (current_mob.kind) {
    case 0:
    case 1:
    case 3:
        add_loose_to_frame();
        break;
    case 2:
        add_mob_kind_2();
        return;
    case 4:
        add_mob_kind_4();
        return;
    case 6:
        add_mob_kind_6();
        return;
    case 10:
        add_mob_kind_10();
        return;
    case 11:
        add_mob_kind_11();
        return;
    case 12:
        add_mob_kind_12();
        return;
    }
}

void add_loose_to_frame(void)
{
    int visible = 1;
    int y = current_mob.y;
    struct {
        MoveRect rect;
        byte pad;
        sbyte cell, row, column;
    } work;

    int height = mob_heights[current_mob.kind];

    if (viewed_room == current_mob.room) {
        if (height + 191 <= y)
            visible = 0;
    } else if (upper_room == current_mob.room) {
        if (height > y)
            y += 191;
        else
            visible = 0;
    } else if (lower_room == current_mob.room) {
        if (191 - height <= y)
            y -= 191;
        else
            visible = 0;
    } else if (right_room == current_mob.room && mob_widths[current_mob.kind] + current_mob.x > 320) {
        current_mob.x = current_mob.x - 320;
        if (right_room == current_mob.room)
            current_mob.room = viewed_room;
        else if (lower_right_room == current_mob.room)
            current_mob.room = lower_room;
    } else {
        visible = 0;
    }

    if (visible) {
        mob_floor_row = floor_row_for_draw(work.column = current_mob.x / 32,
                                           work.row = room_row_at_y(y));
        mark_mob_redraw(1);
        compute_mob_rect(&work.rect);
        add_mob_obj(y);
        ++work.column;
        work.cell = room_cell(work.column, work.row);
        if (upper_room != current_mob.room)
            mark_floor(work.cell, &work.rect);
        work.cell = room_row_at_y(y - height);
        if (work.cell != work.row)
            mark_floor(room_cell(work.column, work.cell), &work.rect);
    }
}

void compute_mob_rect(MoveRect *rect)
{
    int x, y;

    rect->edges[0] = current_mob.y - mob_heights[current_mob.kind];
    rect->edges[1] = current_mob.x;
    rect->edges[2] = current_mob.y + 1;
    rect->edges[3] = rect->edges[1] + mob_widths[current_mob.kind];
    if (viewed_room != current_mob.room) {
        if (right_room == current_mob.room || lower_right_room == current_mob.room
                || upper_right_room == current_mob.room || left_room == current_mob.room
                || lower_left_room == current_mob.room || upper_left_room == current_mob.room)
            x = -320;
        else
            x = 0;
        if (lower_room == current_mob.room || lower_right_room == current_mob.room
                || lower_left_room == current_mob.room)
            y = -192;
        else if (upper_room == current_mob.room || upper_right_room == current_mob.room
                || upper_left_room == current_mob.room)
            y = 192;
        else
            y = 0;
        if (x || y)
            offset_move_rect(rect, rect, x, y);
        else
            *rect = empty_mob_rect;
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
extern void order_mob_object(sbyte index);

void add_mob_obj(int y)
{
    int index = mob_draw_count++;
    int zero;
    struct { MoveRect *rect; } work;

    mob_draw_list[index].image = current_mob.kind | 0x80;
    mob_draw_list[index].x = current_mob.x;
    mob_draw_list[index].y = y;
    mob_draw_list[index].bank = 4;
    mob_draw_list[index].flags = current_mob.flags;
    work.rect = &mob_draw_list[index].rect;
    compute_mob_rect(work.rect);
    zero = 0;
    mob_draw_list[index].sequence = zero;
    mob_draw_list[index].scratch = zero;
    mob_draw_list[index].order = -1;
    order_mob_object(index);
}

extern void load_player_actor(void);
extern void hit_actor_with_loose(void);
extern void load_guard_actor(sbyte index);
extern void save_guard_actor(void);
extern OpponentPosition *guard_for_mob(int room, int index);
extern void hit_unloaded_guard(OpponentPosition *opp, byte room);
extern int actor_y;
extern sbyte near_crush_offset[], far_crush_offset[];
extern int get_crush_reach(void);

void check_loose_crush(void)
{
    sbyte index = 0;
    sbyte count;

    load_player_actor();
    if (loose_hits_actor()) {
        hit_actor_with_loose();
        player_snapshot = actor_snapshot;
    }

    count = room_guard_counts[current_mob.room].guard_count;
    for (; index < count; ++index) {
        if (current_mob.room == player_room) {
            load_guard_actor(index);
            if (loose_hits_actor()) {
                hit_actor_with_loose();
                save_guard_actor();
            }
        } else {
            OpponentPosition *guard = guard_for_mob(current_mob.room, index);
            if (opp_under_mob(guard))
                hit_unloaded_guard(guard, current_mob.room);
        }
    }
}

int loose_hits_actor(void)
{
    int hit = 0;
    int y;
    sbyte column;

    if (actor_id == 11)
        y = actor_y;
    else if (actor_id == 7 || actor_id == 8)
        y = actor_y - 20;
    else
        y = actor_y - 30;

    if (actor_direction < 0 && actor_room == current_mob.room
            && y < current_mob.y && actor_row == current_mob.row) {
        column = current_mob.x / 32;
        hit = column == actor_column;
        if (!hit && actor_id == 11) {
            int reach = get_crush_reach();
            int edge;

            edge = actor_column + far_crush_offset[actor_facing];
            if (edge == column) {
                if (reach > 8)
                    hit = 1;
            } else if (column == actor_column + near_crush_offset[actor_facing]) {
                if (reach < 4)
                    hit = 1;
            }
        }
    }
    return hit;
}


extern unsigned char stage_mode;

int opp_under_mob(OpponentPosition *opp)
{
    int result = 0;
    int height = (signed char)(opp->cell / 10) * 63 + 56;

    if (stage_mode != 5 || opp->action != 8)
        height -= 30;
    if (height < current_mob.y) {
        if (current_mob.x / 32 == (opp->x - 144) / 32)
            result = 1;
        else
            result = 0;
    }
    return result;
}

extern int sound_playing(int resource);

void add_gate_sound(int sound)
{
    play_move_sound(sound);
    if (sound == 9 && current_level == 5 && current_trob.room == 7
            && !sound_playing(10065)) {
        if (current_level != 5 || player_room != 7 || player_column >= 3)
            queue_move_sound(65);
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

    if (viewed_room == room || (right_room == room && cell % 10 == 9)
            || (upper_room == room && cell < 10)
            || (upper_right_room == room && cell == 9))
        visible = 1;
    else
        visible = 0;
    return visible;
}
