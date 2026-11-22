/* MOVECAVE.C, DOS cave movement overlay. */
typedef signed char sbyte;
typedef unsigned char ubyte;
typedef unsigned int uword;

#pragma pack(1)
typedef struct {
    int sequence;
    ubyte action;
} Trob;

typedef struct {
    sbyte kind;
    uword flags;
    sbyte cell;
    ubyte other[7];
} TileState;

typedef struct {
    uword state;
    int other;
} CellState;

typedef struct {
    int y, x;
    sbyte room;
    int top, left;
} Projectile;

typedef struct {
    int x, y;
    ubyte room;
    int velocity_y, velocity_x;
    sbyte mode, row;
    sbyte state[2];
} DartState;

typedef struct {
    sbyte cell;
    sbyte room;
    sbyte slot;
    sbyte other;
} LavaTrob;

typedef struct {
    int top, left, bottom, right;
} Rect;

struct item {
    int active;
    char state;
    char type;
    char far *data;
    int tail;
};
struct list {
    char prefix;
    struct item entries[10];
};
struct input {
    char prefix;
    char value;
    char far *data;
    int tail;
};
#pragma pack()

extern CellState *cell_states;
extern int current_tile_number;
extern TileState tile_state;
extern sbyte actor_id, actor_room, actor_column, actor_row, actor_grab;
extern ubyte actor_cell, level_number;
extern uword actor_frame, actor_sequence;
extern sbyte actor_life, actor_action, actor_fall, actor_hit;
extern int actor_x, actor_y, actor_sequence_group;
extern Rect actor_bounds;
extern ubyte room_number;
extern ubyte down_room;
extern sbyte right_room, upper_room;
extern struct { ubyte link[4]; } room_links[];
extern sbyte guard_cell, guard_row, guard_room;
extern uword guard_frame;
extern int guard_sequence_group;

extern uword animation_word;
extern DartState dart;
extern LavaTrob current_lava;
#define animation_code current_lava.slot
extern sbyte current_row_flags;
extern sbyte current_floor_kind;
extern int cell_x[], cell_x_clipped[], floor_y[];
extern struct list *lava_slots[4];
extern sbyte *bubble_frames[];
extern sbyte gate_offsets[];
/* 16-bit bubble coordinates are copied together through the legacy input
   record by TriggerLavaBubble. */
struct BubbleSeed {
    sbyte frame, kind;
    int x, y, flags;
};

static struct {
    int dart_redraw[4];
    int lava_redraw[4];
    struct BubbleSeed bubbles[15];
} cave_motion = {
    {54, 16, 59, 23},
    {48, 5, 62, 64},
    {
        { 0, 2,  -6, 31, 1},
        { 0, 1, -10, 32, 1},
        { 0, 3,  -5, 33, 0},
        { 2, 5,  -7, 13, 1},
        { 2, 5,  -9, 43, 0},
        { 3, 5,  -7, 37, 1},
        { 5, 5,  -5, 21, 1},
        { 6, 1,  -8, 11, 0},
        { 7, 4,  -1, 32, 1},
        {11, 4,  -6, 36, 0},
        {12, 5, -10, 27, 0},
        {16, 0,  -3,  0, 1},
        {18, 1,  -3, 16, 1},
        {20, 0, -10, 24, 0},
        {21, 0,  -7, 26, 0}
    }
};

static const char _based(_segname("_CONST")) too_many_lava_pools[] = "All global lava info structs have been used.";
static struct {
    int ticks;
    ubyte state[8];
} bridge;

#define dart_redraw_type cave_motion.dart_redraw[0]
#define lava_redraw_type cave_motion.lava_redraw[0]
#define lava_bubbles ((struct input *)cave_motion.bubbles)

extern int reverse_plate_active(void);
extern Trob *find_trob(sbyte, int);
extern ubyte get_tile(sbyte, sbyte, sbyte);
extern void add_trob(sbyte, sbyte, sbyte, sbyte);
extern int should_redraw(void);
extern void mark_redraw_area(int *);
extern sbyte get_floor_kind(int);
extern int tile_at_pixel(int, int);
extern void set_exit_sound(sbyte);
extern void play_exit_sound(int);
extern Projectile *find_projectile(sbyte, int);
extern int intersect_rect(Rect far *, Rect far *, Rect far *);
extern void queue_move_sound(int);
extern int blocks_dart(ubyte);
extern int stops_dart(ubyte);
extern int gate_blocks_dart(void);
extern int change_hp(sbyte);
extern int get_actor_edge(void);
extern int get_actor_tile(void);
extern int snap_actor_x(int);
extern void move_actor_up(void);
extern void start_sequence(int);
extern void hit_actor(void);
extern void reset_actor_fall(void);
extern void set_actor_action(sbyte);
extern void add_dart_mob(void);
extern int get_actor_center(void);
extern unsigned int random_word(unsigned int);
extern void reset_actor_hp(void);
extern void swim_sequence(void);
extern void load_room(sbyte);
extern void redraw_object(sbyte, Rect *);
extern int get_tile_bounds(Rect *, int *);
extern sbyte get_adjacent_animation(void);
extern void set_gate_sequence(int);
extern sbyte row_to_cell(sbyte);
extern void finish_gate(void);
extern void frame_water_effect(void);
extern void initialize_cave_drawing(void);
extern void configure_cave_view(int, int, ubyte, ubyte);
extern void report_error(const char far *, int);
extern void *cdecl malloc(uword);
extern void cdecl free(void *);
extern void *cdecl memset(void *, int, uword);
#pragma intrinsic(memset)

void redraw_dart(void);
void hit_actor_with_dart(void);
void do_lava_boil(void);
struct list *get_lava_info(sbyte, sbyte, sbyte);
LavaTrob *get_lava_trob(sbyte, sbyte, sbyte);
void redraw_lava(void);
void reset_lava_info(struct list *, int);
void far pascal TriggerLavaBubble(struct list *, struct input *);

int trigger_reverse_gate(sbyte room, sbyte cell)
{
    struct {
        int cell_offset;
        TileState previous_tile;
        uword low;
        int result;
        uword high, flags;
        CellState *previous_cells;
        int previous_number;
        Trob *trob;
    } saved;
    register int cell_index;
    int pressed;
    uword state;

    saved.result = -1;
    cell_index = cell;
    saved.cell_offset = cell_index * 4;
    state = ((CellState *)((char *)cell_states + saved.cell_offset))->state;
    saved.low = state & 255;
    saved.high = state & 0xff00;
    pressed = reverse_plate_active();
    saved.trob = find_trob(room, cell_index);
    if (saved.trob != 0) {
        if (saved.trob->action <= 1) {
            if (pressed)
                saved.flags = tile_state.flags;
            else {
                saved.previous_tile = tile_state;
                saved.previous_number = current_tile_number;
                saved.previous_cells = cell_states;
                saved.flags = get_tile(actor_cell, actor_column, actor_row) == 34
                    ? tile_state.flags : 0;
                cell_states = saved.previous_cells;
                current_tile_number = saved.previous_number;
                tile_state = saved.previous_tile;
            }
            if (saved.flags & 0x0800) {
                ((CellState *)((char *)cell_states + saved.cell_offset))->state = saved.high + 250;
            } else {
                if (saved.low > 200)
                    ((CellState *)((char *)cell_states + saved.cell_offset))->state = saved.high + 200;
                saved.trob->action = 4;
            }
        } else if (saved.trob->action == 2 && pressed)
            saved.trob->action = 3;
    } else if (saved.low != 255) {
        if (saved.low == 0 && !(actor_id == 4 && level_number == 5 && actor_row == 1 &&
                         (actor_cell == 10 || actor_cell == 7 || actor_cell == 12))) {
            if (!pressed) {
                if (!(tile_state.flags & 0x0800))
                    saved.result = 2;
            } else
                saved.result = 3;
        } else if (saved.low != 0 && saved.low != 36 && actor_grab < 0)
            saved.result = 4;
    }
    return saved.result;
}

void animate_dart(void)
{
    uword frame = (ubyte)animation_word & 15;

    if (frame < 6) {
        ++animation_word;
        if (frame < 3)
            redraw_dart();
    } else {
        animation_word &= 255 << 8 | 0xf0;
        animation_code = -1;
    }
}

void move_dart_to_tile(void)
{
    if (dart.velocity_y != -1 && dart.velocity_y != -2) {
        current_floor_kind = get_floor_kind(tile_at_pixel(dart.x / 32, dart.y));
        if (room_number == dart.room) {
            set_exit_sound(1);
            play_exit_sound(dart.y);
            dart.mode = 2;
        }
    }
}

void check_dart_hit(void)
{
    Rect hit, bounds;
    int index;
    Projectile *dart_mob;

    if (!(actor_frame >= 108 && actor_frame <= 112 && actor_sequence == 79)) {
        index = 1;
        do {
            dart_mob = find_projectile(2, index++);
            if (dart_mob && dart_mob->top != -1) {
                bounds.right = dart_mob->y + 14;
                bounds.left = bounds.right - 4;
                bounds.bottom = dart_mob->x;
                bounds.top = dart_mob->x - 2;
                if (intersect_rect(&hit, &bounds, &actor_bounds))
                    goto hit_dart;
                if (actor_bounds.left > bounds.left ||
                    actor_bounds.top - 2 > bounds.bottom ||
                    actor_bounds.bottom - 2 < bounds.top ||
                    bounds.left - dart_mob->left >= actor_bounds.right ||
                    bounds.bottom - dart_mob->top >= actor_bounds.bottom)
                    continue;
            hit_dart:
                hit_actor_with_dart();
                dart_mob->top = -1;
            }
        } while (dart_mob);
    }
}

#pragma optimize("g", on)
void move_dart_mob(void)
{
    union {
        int x;
        ubyte tile[2];
    } motion;
    int next_col;
    register int y;

    if (dart.velocity_y == -2) {
        dart.velocity_y = -1;
        return;
    }
    if (dart.velocity_y != -1) {
        y = dart.y + dart.velocity_y;
        motion.x = dart.velocity_x + dart.x;
        if (motion.x > 320) {
            if ((ubyte)right_room == dart.room) {
                dart.room = room_number;
                motion.x -= 320;
            } else {
                if (!room_links[dart.room].link[0])
                    queue_move_sound(82);
                dart.velocity_y = -2;
            }
        }
        if (dart.velocity_y != -1 && y < 3) {
            if (down_room == dart.room) {
                dart.room = room_number;
                y += 192;
            } else {
                if (!room_links[dart.room].link[1])
                    queue_move_sound(82);
                dart.velocity_y = -2;
            }
        }
        if (dart.velocity_y != -2) {
            register int next_x = motion.x;
            if (63 * dart.row + 3 >= y) {
                if (!blocks_dart(get_tile(dart.room, next_x / 32, --dart.row)))
                    dart.velocity_y = -2;
            } else {
                *(volatile int *)&next_col = next_x / 32;
                if (dart.x / 32 != next_col) {
                    motion.tile[1] = get_tile(dart.room, next_col, dart.row);
                    if (stops_dart(motion.tile[1]) || (motion.tile[1] == 4 && gate_blocks_dart()))
                        dart.velocity_y = -2;
                }
            }
            if (dart.velocity_y == -2)
                queue_move_sound(82);
            dart.y = y;
            dart.x = next_x;
        }
    }
}
#pragma optimize("", on)

void hit_actor_with_dart(void)
{
    int edge;

    if (actor_life < 0) {
        if (!change_hp(1)) {
            actor_sequence_group = 8;
            if (actor_id == 0)
                queue_move_sound(31);
            else
                queue_move_sound(72);
        } else {
            edge = get_actor_edge();
            if (blocks_dart(get_actor_tile()) && edge >= 4) {
                actor_x = snap_actor_x(edge - 40);
                move_actor_up();
                ++actor_row;
                start_sequence(18);
            } else {
                hit_actor();
                reset_actor_fall();
                actor_fall = 0;
            }
        }
        queue_move_sound(81);
        set_actor_action(0);
    }
}

void redraw_dart(void)
{
    if (should_redraw())
        mark_redraw_area(&dart_redraw_type);
    else
        animation_code = 1;
}

void trigger_dart(sbyte room, sbyte cell)
{
    DartState saved;
    uword frame;
    int x;

    frame = (ubyte)cell_states[cell].state & 15;
    saved = dart;
    if (!frame) {
        add_trob(room, cell, 1, 36);
        queue_move_sound(80);
        dart.velocity_x = 18;
        dart.velocity_y = -11;
        x = cell_x_clipped[current_lava.cell % (sbyte)10] + 30;
        if (x > 320) {
            if (right_room == room) {
                room = room_number;
                x -= 320;
            } else
                dart.velocity_y = -1;
        }
        if (dart.velocity_y != -1) {
            dart.x = x;
            dart.room = room;
            dart.y = floor_y[current_lava.cell / (sbyte)10] - 15;
            dart.mode = 2;
            dart.row = current_lava.cell / (sbyte)10;
            add_dart_mob();
        }
    }
    dart = saved;
}

#pragma optimize("gl", on)
void animate_lava(void)
{
    sbyte frame, i, next;
    struct list *pool;
    struct item *bubble;
    int persistent;

    i = animation_code & 7;
    pool = lava_slots[i];
    if (should_redraw()) {
        frame = pool->prefix & 63;
        if (frame != 23 && !((pool->prefix & 128) && frame == 3)) {
            ++pool->prefix;
            if (!(pool->prefix & 128) || frame >= 3) {
                if (pool->prefix & 128) {
                    if (frame == 4) {
                        pool->prefix &= 127;
                        queue_move_sound(97);
                    }
                    frame -= 3;
                }
                for (i = 0; i < 10; ++i) {
                    bubble = &pool->entries[i];
                    if (bubble->active) {
                        ++bubble->state;
                        next = bubble_frames[bubble->type][bubble->state];
                        if (next == -1)
                            bubble->active = 0;
                        else if (next == -2) {
                            bubble->tail = 1;
                            ++bubble->state;
                        }
                    }
                }
                i = 0;
                do {
                    if (lava_bubbles[i].prefix > frame)
                        break;
                    if (lava_bubbles[i].prefix == frame)
                        TriggerLavaBubble(pool, &lava_bubbles[i]);
                    ++i;
                } while (i < 15);
            }
        } else {
            persistent = pool->prefix & 64;
            reset_lava_info(pool, 0);
            if (frame == 3) {
                pool->prefix = 0x84;
                if (persistent)
                    pool->prefix = 0xc4;
            }
        }
        redraw_lava();
    } else {
        free(pool);
        lava_slots[i] = 0;
    }
}

#pragma optimize("", on)

void check_lava_boil(void)
{
    uword kind;
    int y, center, reach;
    kind = get_tile(actor_cell, actor_column, actor_row);
    y = (sbyte)63 * actor_row + 41;
    center = get_actor_center();
    reach = cell_x[actor_column] - center + 26;
    if ((kind == 23 || (kind == 24 && reach >= 0)) &&
        (current_row_flags & 64) && y < actor_y && actor_sequence_group != 5)
        do_lava_boil();
}

void do_lava_boil(void)
{
    struct list *pool;
    LavaTrob *trob;

    pool = get_lava_info(actor_cell, actor_column, actor_row);
    if (pool) {
        pool->prefix = 0x80;
        if (actor_id == 0 && random_word(3) == 0) {
            start_sequence(116);
            pool->prefix |= 64; queue_move_sound(99);
        } else {
            if (actor_id == 0) queue_move_sound(99);
            else queue_move_sound(98);
            start_sequence(115);
        }
        actor_action = 0;
        actor_hit = 1;
        actor_sequence_group = 5;
        reset_actor_hp();
        change_hp(100);
        swim_sequence();
        trob = get_lava_trob(actor_cell, actor_column, actor_row);
        if (trob) {
            current_lava = *trob;
            redraw_lava();
        }
    }
}

ubyte get_free_lava_slot(void)
{
    ubyte slot = 0;
    do {
        if (!lava_slots[slot])
            break;
        ++slot;
    } while (slot < 4);
    if (slot == 4) {
        report_error(too_many_lava_pools, 1);
        --slot;
    }
    return slot;
}

#pragma optimize("g", on)
struct list *get_lava_info(sbyte room, sbyte col, sbyte row)
{
    struct list *result;
    LavaTrob *trob;

    trob = get_lava_trob(room, col, row);
    if (trob)
        result = lava_slots[trob->slot];
    else
        result = 0;
    return result;
}
#pragma optimize("", on)

LavaTrob *get_lava_trob(sbyte room, sbyte col, sbyte row)
{
    sbyte cell;
    ubyte kind;
    LavaTrob *trob;

    kind = get_tile(room, col, row);
    cell = tile_state.cell;
    if (kind == 24)
        --cell;
    trob = (LavaTrob *)find_trob(room, cell);
    load_room(room_number);
    return trob;
}

#pragma optimize("gl", on)
void clear_lava_slots(void)
{
    int i;

    for (i = 0; i < 4; ++i) {
        if (lava_slots[i]) {
            free(lava_slots[i]);
            lava_slots[i] = 0;
        }
    }
}
#pragma optimize("", on)

void redraw_lava(void)
{
    struct { sbyte flags; int bounds[4]; } redraw;

    redraw.flags = lava_slots[animation_code]->prefix;
    lava_redraw_type = redraw.flags & 128 ? 16 : (redraw.flags & 64 ? 39 : 48);
    if (get_tile_bounds((Rect *)redraw.bounds, &lava_redraw_type)) {
        redraw_object(current_lava.cell, (Rect *)redraw.bounds);
        redraw_object(get_adjacent_animation(), (Rect *)redraw.bounds);
        if (current_lava.cell % (sbyte)10 < 8) {
            ++current_lava.cell;
            redraw_object(get_adjacent_animation(), (Rect *)redraw.bounds);
            --current_lava.cell;
        }
    }
}

void reset_lava_info(struct list *pool, int type)
{
    memset(pool, 0, sizeof(*pool));
    switch (type) {
    default:
        pool->prefix = 0;
        TriggerLavaBubble(pool, &lava_bubbles[0]);
        TriggerLavaBubble(pool, &lava_bubbles[1]);
        TriggerLavaBubble(pool, &lava_bubbles[2]);
        TriggerLavaBubble(pool, &lava_bubbles[13]);
        pool->entries[3].state = 5;
        TriggerLavaBubble(pool, &lava_bubbles[14]);
        pool->entries[4].state = 4;
        break;
    case 1:
        pool->prefix = 3;
        TriggerLavaBubble(pool, &lava_bubbles[5]);
        TriggerLavaBubble(pool, &lava_bubbles[4]);
        pool->entries[1].state = 1;
        TriggerLavaBubble(pool, &lava_bubbles[3]);
        pool->entries[1].state = 1;
        TriggerLavaBubble(pool, &lava_bubbles[2]);
        pool->entries[3].state = 3;
        TriggerLavaBubble(pool, &lava_bubbles[1]);
        pool->entries[3].state = 3;
        TriggerLavaBubble(pool, &lava_bubbles[0]);
        pool->entries[3].state = 3;
        break;
    case 2:
        pool->prefix = 11;
        TriggerLavaBubble(pool, &lava_bubbles[10]);
        TriggerLavaBubble(pool, &lava_bubbles[9]);
        pool->entries[1].state = 4;
        TriggerLavaBubble(pool, &lava_bubbles[8]);
        pool->entries[2].state = 5;
        break;
    }
}

void trigger_lava(sbyte room, sbyte cell)
{
    ubyte slot;
    if (!find_trob(room, cell)) {
        slot = get_free_lava_slot();
        lava_slots[slot] = malloc(sizeof(struct list));
        reset_lava_info(lava_slots[slot], cell % (sbyte)3);
        add_trob(room, cell, slot, 23);
    }
}

void far pascal TriggerLavaBubble(struct list *destination,
                                      struct input *source)
{
    register int index;
    struct item *entry;

    index = 0;
    entry = destination->entries;
    do {
        if (entry->active == 0)
            break;
        ++entry;
        ++index;
    } while (index < 10);

    if (index < 10) {
        destination->entries[index].active = 1;
        destination->entries[index].state = 0;
        destination->entries[index].type = source->value;
        destination->entries[index].data = source->data;
        destination->entries[index].tail = source->tail;
    }
}


int move_cave_gate(int sequence)
{
    sbyte cell;

    cell = row_to_cell(guard_row) + guard_cell;
    if (guard_room != current_lava.room) {
        if (right_room == current_lava.room)
            cell += 10;
        else if (upper_room == current_lava.room)
            cell -= 10;
        else
            cell = 30;
    }
    if (cell == current_lava.cell || current_lava.cell - cell + 1 == 0) {
        if (guard_sequence_group == 3) {
            sequence = 12;
            set_gate_sequence(9);
        } else if (guard_frame == 264) {
            sequence -= gate_offsets[animation_code] + 2;
        } else if (guard_frame == 265) {
            sequence = 36;
        } else if (guard_frame == 266) {
            animation_code = -1;
            finish_gate();
        }
    }
    return sequence;
}

uword get_cave_torch_frame(uword previous)
{
    uword frame = random_word(8);
    if (frame == previous) {
        ++frame;
        if (frame >= 9)
            frame = 0;
    }
    return frame;
}

void cave_overlay_entry(void)
{
}

void cave_frame(void)
{
    if ((level_number == 5 && room_number == 10) ||
        (level_number == 5 && room_number == 7) ||
        (level_number == 5 && room_number == 12))
        frame_water_effect();
}

void initialize_bridge(void)
{
    initialize_cave_drawing();
    configure_cave_view(3500, 64, 160, 0);
    bridge.ticks = 0;
    memset(bridge.state, 0, sizeof(bridge.state));
}
