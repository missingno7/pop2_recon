/* CUSTCVCA.C, overlay 13. DOS entry order. */
#pragma pack(1)
typedef unsigned char Byte;
typedef unsigned int Handle;
typedef struct { int top, left, bottom, right; } Rect;
typedef struct { int height, width; Byte flags, unknown, colors; Byte data[96]; } Shape;
typedef struct { Byte sheet; signed char col; Byte flags; signed char row; Handle handle; } ShapeRef;
typedef struct { Byte data[4]; Handle handle; } CarpetImage;
typedef struct {
    CarpetImage images[10];
    int frame, sitting, saved;
} Carpet;
typedef struct {
    signed char sheet, flags;
    unsigned int colors;
    int x, y;
    Byte col, row;
    Rect bounds;
    Byte order, unused;
} CarpetDraw;
typedef struct {
    int x, y;
    Byte sheet;
    unsigned int image;
    Rect clip;
    Byte order;
    int reserved;
    Byte invisible, depth;
} MidItem;
typedef struct { int dy, dx; } CarpetStep;
typedef struct { Byte kind; Byte row; Byte unknown[2]; Byte modifier; } TileAnim;
#pragma pack()

extern Carpet *carpet;                /* DS:2cdc */
extern unsigned int *carpet_source;             /* DS:6242 */
#define CARPET_SHAPE_ID 0x62d7
extern int carpet_y, carpet_x;                   /* DS:1d9c,1d9a */
static struct {
    int images[10];
    signed char scroll[10];
    CarpetStep steps[10];
    Rect bars;
} carpet_animation = {
    {0, 1, 2, 3, 4, 5, 6, 7, 1, 8},
    {-1, 0, 1, 1, 1, 1, -1, 0, 0, 0},
    {{7, -45}, {8, -46}, {5, -44}, {7, -39}, {7, -41},
     {7, -39}, {5, -43}, {7, -42}, {8, -46}, {8, -46}},
    {5, -88, 26, 7}
};
extern int carpet_view_enabled;                 /* DS:6266 */
extern int custom_count, mid_count;              /* DS:6248,624e */
extern int actor_x, actor_y;                     /* DS:6254,6256 */
extern Byte actor_depth;                        /* DS:6253 */
extern unsigned int actor_image;                  /* DS:6258 */
extern Rect actor_clip;                         /* DS:625b */
extern Byte draw_col, draw_row, draw_flags;       /* DS:6ccf,6cce,6cd2 */
extern CarpetDraw far carpet_draw_queue[];       /* 2fae:0000 */
extern _segment mid_table_selector;             /* DS:284c */
extern MidItem _based(mid_table_selector) mid_rows[]; /* 2fae:11f8 */
extern int camera_y;                            /* DS:5e94 */
extern Rect view_rect;                          /* DS:5e9d */
extern volatile Byte char_direction;             /* DS:5c01 */
extern volatile int char_x, char_y;               /* DS:5c02,5c04 */
extern int char_frame, char_sequence;            /* DS:5c07,5c24 */
extern Byte tile_col, tile_row, tile_modifier, tile_flags; /* DS:6274,6273,6271,6272 */
extern unsigned int bar_modifier;               /* DS:5e48 */
extern signed char bar_direction;                /* DS:67d0 */
extern Rect current_clip, cave_bars_clip;         /* DS:6236,09fe */
typedef struct { int state, timer; } CaveTile;
extern CaveTile cave_tiles[];                   /* DS:304a */

extern void start_sound(int), play_sound(int), init_room_sound(int, int);
extern void set_resource(unsigned int, unsigned int, Byte, Byte);
extern void far *probe_handle(Handle);
extern void free_handle(Handle), mark_purgeable(Handle);
extern Handle allocate_handle(long);
extern Shape far *locate_shape(Handle);
extern void create_shape(Handle, int, CarpetImage far *, int);
extern void free_shape_slot(int);
extern void unload_palette(int);
extern void shape_rect(Rect far *, int, int, Shape far *);
extern void far _fastcall save_under_shape(int, int, int, int, Byte, Byte);
extern void set_floor_restore(Byte);
extern void translate_rect(Rect far *, Rect far *, int, int);
extern void enter_char_context(void), leave_char_context(void), update_char_pos(void);
extern void far _fastcall set_draw_mode(Byte);
typedef void far pascal DrawCallback(void);
extern DrawCallback draw_under_actor, draw_carpet_foreground;
extern void redraw_cells(DrawCallback *, Rect near *, signed char);
extern void load_tile(signed char, signed char, signed char);
extern void trigger_tile(Byte, Byte);
extern void set_sequence(int);
extern void move_char(Byte), set_char_state(Byte);
extern void far _fastcall draw_cave_bars_image(Byte, Byte);
extern int get_tile_rect(Rect near *, Rect near *);
extern void * _cdecl malloc(unsigned int);
extern void _cdecl free(void *);
extern void * _cdecl memset(void *, int, unsigned int);
#pragma intrinsic(memset)

int check_carpet_image(int);
void load_carpet_data(void), unload_carpet_data(void);
void map_carpet_colors(void), redraw_cave_bars(void);
int get_carpet_frame(void), cave_bars_open(void);
Shape far * far _fastcall get_carpet_shape_info(ShapeRef *, int);

void LoadCarpetCode(void)
{
    map_carpet_colors();
    load_carpet_data();
    start_sound(33);
}

void UnloadCarpetCode(void)
{
    unload_carpet_data();
    set_resource(750, 32, 16, 0);
}

int check_carpet_image(int index)
{
    Handle copy_handle;
    Shape far *source, far *copy;
    unsigned int entry_offset[1];  /* Keeps the scaled offset in a stack slot across calls. */

    if (carpet) {
        entry_offset[0] = index * sizeof(CarpetImage);
#define ENTRY ((CarpetImage *)((char *)carpet + entry_offset[0]))
        if (ENTRY->handle && probe_handle(ENTRY->handle))
            goto return_handle;
        if (ENTRY->handle) {
            free_handle(ENTRY->handle);
            ENTRY->handle = 0;
        }
        copy_handle = allocate_handle(sizeof(Shape));
        source = locate_shape(*carpet_source);
        copy = locate_shape(copy_handle);
        *copy = *source;
        copy->flags |= 4;
        copy->colors = 32;
        create_shape(copy_handle, index + 302, ENTRY, 1);
        mark_purgeable(ENTRY->handle);
        free_handle(copy_handle);
        free_shape_slot(CARPET_SHAPE_ID + index);
    }
return_handle:
#undef ENTRY
    return carpet->images[index].handle;
}

void draw_carpet(Byte *tile)
{
    CarpetDraw far *entry;
    ShapeRef ref;
    Shape far *shape;

    if (carpet && !carpet->frame && !*tile && carpet_view_enabled) {
        entry = &carpet_draw_queue[custom_count];
        ++custom_count;
        entry->y = carpet_x;
        entry->x = carpet_y;
        entry->colors = CARPET_SHAPE_ID;
        entry->sheet = 4;
        entry->col = draw_col;
        entry->row = draw_row;
        shape = get_carpet_shape_info(&ref, 0);
        shape_rect(&entry->bounds, entry->y, entry->x, shape);
        entry->order = 10;
        entry->flags = draw_flags;
    }
}

void save_under_carpet(void)
{
    ShapeRef ref;
    Shape far *shape;

    if (carpet && !carpet->saved) {
        shape = get_carpet_shape_info(&ref, 0);
        save_under_shape(carpet_y, carpet_y + shape->width, carpet_x,
                         shape->height, 101, 2);
        carpet->saved = 1;
    }
}

void draw_obj_carpet(Byte *unused)
{
    int slot, frame[1];  /* Draft: keep the frame index on the stack across calls. */
    Shape far *shape;
    ShapeRef ref;
    signed char scroll;
    Byte palette;

    if (carpet && carpet->frame) {
        slot = mid_count; mid_rows[slot].x = actor_x;
        frame[0] = get_carpet_frame(); shape = get_carpet_shape_info(&ref, carpet_animation.images[frame[0]] + 1);
        mid_rows[slot].y = actor_y - shape->height; mid_rows[slot].sheet = 4;
        mid_rows[slot].image = CARPET_SHAPE_ID + carpet_animation.images[frame[0]] + 1; mid_rows[slot].clip = actor_clip;
        mid_rows[slot].order = 10;
        mid_rows[slot].reserved = 0; mid_rows[slot].invisible = 0; mid_rows[slot].depth = 255;
        scroll = carpet_animation.scroll[frame[0]]; if (scroll) {
            camera_y += scroll; translate_rect(&view_rect, &view_rect, 0, scroll);
        }
        if (carpet->sitting) {
            palette = carpet->frame - 4;
            if (palette < 8) set_resource(CARPET_SHAPE_ID, 16, 32, palette);
        }
        ++mid_count; ++carpet->frame;
        if (carpet->saved) {
            set_floor_restore(1); carpet->saved = 0;
        }
    }
}

int get_carpet_frame(void)
{
    int frame;
    if (carpet) {
        if (!carpet->sitting)
            frame = (carpet->frame / 2) % 9;
        else
            goto seated;
        goto finish;
    }
    /* The DOS null path reads an uninitialized frame slot. */
    return frame;
seated:
    frame = carpet->frame + 8;
    if (frame > 9)
        frame = 9;
finish:
    return frame;
}

Shape far * far _fastcall get_carpet_shape_info(ShapeRef *ref, int index)
{
    if (carpet) {
        char zero = 0;
        ref->sheet = 1;
        ref->col = zero;
        ref->row = zero;
        ref->handle = check_carpet_image(index);
        index *= sizeof(CarpetImage);
        return locate_shape(((CarpetImage *)((char *)carpet + index))->handle);
    }
    return 0;
}

void load_carpet_data(void)
{
    register int index;
    carpet = malloc(sizeof(Carpet));
    memset(carpet, 0, sizeof(Carpet));
    index = 0;
    do {
        check_carpet_image(index);
    } while (++index < 10);
    init_room_sound(33, 36);
}

void map_carpet_colors(void)
{
    set_resource(CARPET_SHAPE_ID, 16, 32, 0);
}

void setup_carpet(void)
{
    int frame;
    ShapeRef ref;
    Shape far *shape;

    if (carpet && carpet->frame) {
        enter_char_context();
        frame = get_carpet_frame();
        actor_image = CARPET_SHAPE_ID + carpet_animation.images[frame] + 1;
        actor_y += carpet_animation.steps[frame].dy;
        actor_x += carpet_animation.steps[frame].dx;
        if (!actor_depth)
            actor_x -= 20;
        update_char_pos();
        shape = get_carpet_shape_info(&ref, actor_image - CARPET_SHAPE_ID);
        shape_rect(&actor_clip, actor_y - shape->height, actor_x, shape);
        set_draw_mode(13);
        redraw_cells(draw_carpet_foreground, &actor_clip, -1);
        leave_char_context();
    }
}

void sitting_on_carpet(void)
{
    ShapeRef ref;
    Shape far *shape;
    Rect rect;

    if (carpet) {
        if (!carpet->frame && char_frame == 295) {
            carpet->frame = 1;
            load_tile(3, 3, 2);
            trigger_tile(tile_col, tile_row);
            shape = get_carpet_shape_info(&ref, 0);
            shape_rect(&rect, carpet_x, carpet_y, shape);
            redraw_cells(draw_under_actor, &rect, -1);
            return;
        }
        if (!carpet->sitting && char_y < 55 && !cave_bars_open()) {
            carpet->frame = 1;
            carpet->sitting = 1;
            char_y = 20;
            char_sequence = 10;
            set_sequence(129);
            play_sound(36);
            return;
        }
        if (carpet->sitting && carpet->frame >= 1) {
            move_char(100);
            set_char_state(9);
        }
    }
}

void trigger_char_on_carpet(void)
{
    if (carpet) {
        set_sequence(128);
        char_x = 268;
        if (char_direction == 0)
            char_x = 288;
        if (cave_bars_open()) {
            start_sound(34);
            return;
        }
        start_sound(35);
    }
}

void unload_carpet_data(void)
{
    int offset;
    Handle handle;
    volatile Handle held; /* Draft handle lifetime hypothesis. */
    offset = 0;
    do {
        held = handle = ((CarpetImage *)((char *)carpet + offset))->handle;
        if (handle)
            free_handle(handle);
        offset += sizeof(CarpetImage);
    } while (offset < 10 * sizeof(CarpetImage));
    unload_palette(10033);
    init_room_sound(33, 36);
    free(carpet);
    carpet = 0;
}

void animate_cave_bars(void)
{
    unsigned int phase = bar_modifier & 7;
    if (!bar_direction) {
        if (phase < 3) {
            if (!phase)
                play_sound(78);
            bar_modifier &= ~7;
            bar_modifier |= phase + 1;
            redraw_cave_bars();
            return;
        }
    } else if (phase) {
        if (phase == 3)
            play_sound(79);
        bar_modifier &= ~7;
        bar_modifier |= phase - 1;
        redraw_cave_bars();
        return;
    }
    bar_direction = -1;
}

int cave_bars_open(void)
{
    load_tile(3, 6, 0);
    if ((tile_modifier & 7) >= 3)
        return 1;
    return 0;
}

void draw_cave_bars(TileAnim *tile)
{
    Rect saved;
    if (tile->kind == 11 && tile->row == draw_col) {
        if (!carpet_view_enabled) {
            saved = current_clip;
            current_clip = cave_bars_clip;
        }
        draw_cave_bars_image((tile->modifier & 7) + 2, 11);
        if (!carpet_view_enabled)
            current_clip = saved;
    }
}

void redraw_cave_bars(void)
{
    Rect rect;
    if (get_tile_rect(&rect, &carpet_animation.bars))
        redraw_cells(draw_under_actor, &rect, -1);
}

signed char trigger_cave_bars(Byte room, signed char cell)
{
    signed char result = -1;
    int state;
    if (!(tile_flags & 8)) {
        state = cave_tiles[room * (Byte)30 + cell].state;
        if (!state)
            result = 0;
        else if (state == 3)
            result = 1;
    }
    return result;
}
