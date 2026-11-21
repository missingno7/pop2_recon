/* DOS DESERT.C, overlay 5. Definitions follow DOS code order. */
typedef signed char sbyte;
typedef unsigned char ubyte;
typedef unsigned int uword;

#pragma pack(1)
typedef struct {
    int left, top, right, bottom;
} Rect;

typedef struct {
    sbyte shape;
    int x, y;
    ubyte pass;
    ubyte reserved[3];
    uword handle;
    Rect rect;
    ubyte tail[6];
} Piece;

/* Animated pieces use the high byte of y and the pass byte as a state word. */
typedef struct {
    ubyte reserved[4];
    uword flags;
} PackedPieceState;

#define piece_state(piece) (((PackedPieceState *)(piece))->flags)

typedef struct {
    sbyte count;
    ubyte template_id;
    int shape_base;
    ubyte header[24];
    Piece pieces[1];
} Room;

typedef struct {
    sbyte type, direction;
    int x;
    ubyte reserved_a[3];
    int frame;
    sbyte column;
    ubyte reserved_b[4];
    sbyte room;
    ubyte reserved_c[2];
    sbyte alive;
    ubyte reserved_d[7];
    int sequence;
    ubyte reserved_e[9];
    int action;
    ubyte reserved_f[26];
} Actor;

typedef struct {
    uword flags;
    uword tile;
} RoomCell;

typedef struct {
    ubyte reserved[16];
    Rect bounds;
} Peel;

typedef struct {
    Peel *peel;
    int reserved;
    int active;
} PeelRecord;
#pragma pack()

extern ubyte sound_flags;
extern Rect actor_clip;
/* The column byte has its own DGROUP operand at actor + 9. */
extern sbyte actor_column, desert_column, previous_column;
extern uword exit_flags, animation_flags;
extern RoomCell room_cells[], stone_cells[], exit_cells[];
extern int fade_active, frame_number;
extern ubyte room_number;
extern int sound_room;
extern Actor actor, saved_kid;
extern sbyte level_number;
extern int next_level, fall_transition;
extern sbyte move_up, move_down, move_forward, move_action;
extern sbyte desert_variant;
extern Room *custom_room;
extern uword *custom_shape_archive;
extern Rect draw_clip, screen_clip;
extern int sprite_x, sprite_width;
extern ubyte sprite_flipped;
extern sbyte animation_tile, animation_remove;
extern Rect stone_bounds;
extern PeelRecord peel_records[];

extern void queue_move_sound(int), play_sound(int);
extern void rotate_palette(int, int, int);
extern int sound_playing(int);
extern void stop_sound(int);
extern int query_animation(sbyte, sbyte);
extern void copy_kid_to_char(void), reset_player(void);
extern void start_sequence(int);
extern int bounded_random(int);
extern void far *lock_archive(uword);
extern void unlock_archive(uword), free_shape(uword);
extern uword get_resource(unsigned long, int);
extern uword load_shape(uword, int, void far *);
extern void far *locate_shape(uword);
extern Rect far *shape_rect(Rect far *, int, int, void far *);
extern void redraw_object(sbyte, Rect *);
/* The callback address and direct calls have different relocation forms. */
extern void redraw_callback(sbyte, Rect *);
extern void mark_redraw(void (far *)(sbyte, Rect *), Rect *, sbyte);
extern void far _fastcall draw_image(sbyte, sbyte);
extern int find_tile_animation(sbyte, int);
extern void start_tile_animation(sbyte, sbyte, sbyte, sbyte);
extern int update_animation(void);
extern sbyte RestoreKeepCustomPeel(ubyte);
extern void DrawCustomRoomImgAtInfo(ubyte, Piece *);
extern void offset_rect(Rect far *, Rect far *, int, int);
extern void far _fastcall add_peel(int, int, int, int, ubyte, ubyte);
extern int get_tile_bounds(Rect *, Rect *);
extern sbyte get_adjacent_animation(void), get_other_animation(void);
extern int restore_peel(ubyte, int);

void check_desert_exit(void);
void cycle_desert_colors(void);
void watch_desert_kid(void);
int is_selected_code(int);
uword is_desert_exit_open(void);
void trigger_desert_exit(sbyte, sbyte);
void animate_desert_stone(void);
void redraw_stone(void);
void remove_last_stone_peel(sbyte);
int sink_desert_stone(sbyte);
int trigger_stone_at(ubyte, sbyte, sbyte);

void add_desert_ambient(void)
{
    if (sound_flags & 2)
        queue_move_sound(251);
}

void clip_desert_character(void)
{
    actor_clip.right = actor_clip.right > 126 ? 126 : actor_clip.right;
    if (actor_clip.right < actor_clip.left)
        actor_clip.left = actor_clip.right;
}

void check_desert_exit(void)
{
    uword flags = exit_flags;
    uword frame;
    int keep_open;
    sbyte cell;

    if ((flags & 31) != 0)
        return;
    frame = (flags >> 8) & 31;
    keep_open = desert_column - previous_column != -2;
    cell = 0;
    if (keep_open) {
        do {
            int state = exit_cells[cell].flags & 15;
            if ((state && cell != desert_column) ||
                (!state && cell == desert_column && frame < 10))
                keep_open = 0;
            ++cell;
            if (!keep_open)
                break;
        } while (cell < 6);
    }

    if (!keep_open) {
        flags &= 0xe0ff;
    } else if (frame >= 20) {
        trigger_desert_exit(1, 10);
    } else {
        if (frame == 6)
            queue_move_sound(37);
        else if (frame == 9)
            play_sound(45);
        else if (frame == 10)
            sink_desert_stone(desert_column + 12);
        flags &= 0xe0ff;
        flags |= ++frame << 8;
    }
    exit_flags = flags;
}

void cycle_desert_colors(void)
{
    if (!fade_active && room_number != 1 && frame_number % 3 == 0) {
        rotate_palette(176, 13, 0);
        rotate_palette(196, 4, 0);
        rotate_palette(191, 5, 0);
    }
}

void desert_frame(void)
{
    cycle_desert_colors();
    watch_desert_kid();
    check_desert_exit();
    if (room_number != 1) {
        if (!sound_playing(10038))
            play_sound(38);
        if (sound_playing(10251))
            stop_sound(10251);
    } else if (sound_playing(10251)) {
        if (sound_playing(10038))
            stop_sound(10038);
    } else if (saved_kid.action != 7 && saved_kid.alive < 0 &&
               !sound_playing(sound_room + 10000) &&
               !query_animation(30, 1) && !is_desert_exit_open()) {
        add_desert_ambient();
    }
}

void watch_desert_kid(void)
{
    copy_kid_to_char();
    if (actor.room == 1) {
        if (actor_column != previous_column) {
            if (is_selected_code(actor.frame)) {
                if (previous_column >= 2) {
                    if (previous_column <= 7) {
                        if (sink_desert_stone(previous_column + 10) &&
                            desert_column - previous_column == -2)
                            queue_move_sound(41);
                    }
                }
            }
        }
        if (is_selected_code(actor.frame))
            previous_column = actor_column;
        if (actor.x < 130) {
            if (level_number == next_level) {
                ++next_level;
                start_sequence(2);
            } else {
                start_sequence(2);
                fall_transition = -1;
            }
        }
    } else if (actor.room == 3) {
        if (actor.x >= 489) {
            if (move_up || move_down || (!actor.direction && move_forward)) {
                start_sequence(2);
                reset_player();
            }
            if (!sound_playing(10046) && (move_action < 0 || !bounded_random(20)))
                play_sound(46);
        }
    }
    saved_kid = actor;
}

void reset_desert_variant(void)
{
    desert_variant = -1;
}

int is_selected_code(int code)
{
    int selected;

    if (code == 15 || code == 109 || code == 7 || code == 11 ||
        code == 13 || code == 38 || code == 44 || code == 28 ||
        code == 158 || ((uword)code - 170) <= 1)
        selected = 1;
    else
        selected = 0;
    return selected;
}

void LoadDsrtCode(void)
{
    Piece *piece;
    void far *archive;
    int resource;

    previous_column = -1;
    if (custom_room != 0) {
        if (custom_room->template_id == 6) {
            if (desert_variant == -1) {
                desert_variant = (sbyte)bounded_random(2);
                desert_column = (sbyte)bounded_random(4) + 1;
            }
            if (desert_variant != 2) {
                archive = lock_archive(*custom_shape_archive);
                piece = &custom_room->pieces[15];
                free_shape(piece->handle);
                resource = desert_variant + custom_room->shape_base + 15;
                piece->handle = get_resource(0x53484150UL, resource);
                piece->handle = load_shape(piece->handle, 2, archive);
                piece = &custom_room->pieces[16];
                free_shape(piece->handle);
                piece->handle = get_resource(0x53484150UL, resource + 3);
                piece->handle = load_shape(piece->handle, 2, archive);
                unlock_archive(*custom_shape_archive);
            }
            }
    }
}

void setup_sand(void)
{
    Rect saved_clip;
    ubyte index;
    Piece *piece;
    Rect *bounds;

    if (actor.sequence == 27) {
        saved_clip = draw_clip;
        index = (ubyte)(frame_number % 2) + 3;
        piece = &custom_room->pieces[index];
        piece->x = 119;
        piece->y = sprite_x;
        if (!sprite_flipped)
            piece->y -= sprite_width;
        bounds = &piece->rect;
        shape_rect(bounds, piece->x, piece->y, locate_shape(piece->handle));
        mark_redraw(redraw_callback, bounds, -1);
        draw_clip = *bounds;
        draw_image(index, piece->pass = 1);
        piece->pass = 11;
        draw_clip = saved_clip;
    }
}

void animate_desert_exit(void)
{
    uword frame;
    Piece *piece;
    Rect *bounds;

    if (update_animation()) {
        frame = animation_flags & 31;
        if (frame == 20) {
            animation_remove = -1;
            stop_sound(10007);
        } else {
            ++frame;
            if (!sound_playing(10045))
                play_sound(7);
        }
        animation_flags &= ~31;
        animation_flags |= frame;
        piece = &custom_room->pieces[2];
        piece->rect.top -= 2;
        bounds = &piece->rect;
        redraw_object(animation_tile, bounds);
        redraw_object(animation_tile + 1, bounds);
        redraw_object(animation_tile - 10, bounds);
        redraw_object(animation_tile - 9, bounds);
        RestoreKeepCustomPeel(piece->shape);
    }
}

void draw_desert_exit(Piece *piece)
{
    int frame;

    if (piece->shape == 11) {
        frame = piece_state(piece) & 31;
        custom_room->pieces[2].y = (4 - frame) * 2;
        custom_room->pieces[15].y = 27 - frame * 2;
        custom_room->pieces[15].rect.top = 0;
        draw_image(2, 11);
        draw_image(15, 11);
    }
}

uword is_desert_exit_open(void)
{
    return (ubyte)exit_flags & 31;
}

void trigger_desert_exit(sbyte room, sbyte cell)
{
    if (!find_tile_animation(room, cell)) {
        start_tile_animation(room, cell, 1, 4);
        queue_move_sound(39);
    }
}

void animate_desert_planks(void)
{
    uword frame;

    if (update_animation()) {
        frame = (ubyte)animation_flags & 15;
        if (frame == 7)
            frame = 0;
        else
            ++frame;
        animation_flags &= ~15;
        animation_flags |= frame;
        redraw_object(animation_tile, &custom_room->pieces[1].rect);
    }
}

void draw_desert_planks(Piece *piece)
{
    Rect saved_clip;
    sbyte index;

    if (piece->shape == 11) {
        index = ((piece_state(piece) >> 1) & 7) + 1;
        saved_clip = draw_clip;
        draw_clip = screen_clip;
        draw_image(index, 11);
        draw_clip = saved_clip;
    }
}

void animate_desert_rocks(void)
{
    uword frame;

    if (update_animation()) {
        frame = (ubyte)animation_flags & 15;
        if (frame == 7)
            frame = 0;
        else
            ++frame;
        animation_flags &= ~15;
        animation_flags |= frame;
        redraw_object(animation_tile, &custom_room->pieces[1].rect);
    }
}

void draw_desert_rocks(Piece *piece)
{
    Rect saved_clip;
    sbyte index;

    if (piece->shape == 11) {
        index = ((piece_state(piece) >> 1) & 7) + 1;
        saved_clip = draw_clip;
        draw_clip = screen_clip;
        draw_image(index, 11);
        draw_clip = saved_clip;
    }
}

void AddDsrtStoneUpPeels(Piece *piece)
{
    Rect bounds;
    int x, y, count;
    ubyte id;

    id = piece->shape + 100;
    y = 126;
    x = 64;
    count = 6;
    do {
        offset_rect(&bounds, &piece->rect, x, y);
        add_peel(bounds.top, bounds.bottom, bounds.left,
                 bounds.right - bounds.left, id, 3);
        x += 32;
    } while (--count);
}

void animate_desert_stone(void)
{
    uword delay = (animation_flags & 0x1f0) >> 4;
    uword frame;

    if (delay == 0) {
        frame = animation_flags & 15;
        if (animation_remove == 3 && frame == 0)
            play_sound(42);

        if ((frame == 10 && animation_remove == 3) ||
            (frame == 0 && animation_remove == 1)) {
            if (animation_remove == 3 && !query_animation(30, 2))
                add_desert_ambient();
            animation_remove = -1;
        } else {
            animation_flags &= ~15;
            animation_flags |= frame + animation_remove - 2;
        }

        if (animation_remove > 0)
            redraw_stone();
    } else if (saved_kid.sequence != 27 && saved_kid.alive < 0) {
        animation_flags &= ~0x1f0;
        animation_flags |= --delay << 4;
    }
}

void draw_desert_stone(Piece *piece)
{
    uword frame;

    if (piece->shape == 11 || piece->shape == 2) {
        frame = piece_state(piece) & 15;
        if (frame) {
            DrawCustomRoomImgAtInfo(frame + 4, piece);
            if (frame > 7 && (sbyte)piece->x - desert_column == 2)
                DrawCustomRoomImgAtInfo(16, piece);
        }
    }
}

void redraw_stone(void)
{
    Rect bounds;

    if (get_tile_bounds(&bounds, &stone_bounds)) {
        redraw_object(animation_tile, &bounds);
        redraw_object(get_adjacent_animation(), &bounds);
        redraw_object(get_other_animation(), &bounds);
    }
}

void remove_last_stone_peel(sbyte column)
{
    int x = (column + 1) << 5;
    sbyte matches = 0;
    int occurrence = 1;
    int slot;
    Rect far *bounds;
    /* Keep the draft record pointer spilled under this compiler. */
    PeelRecord * volatile record;

    while ((slot = restore_peel(114, occurrence++)) != -1) {
        record = &peel_records[slot];
        bounds = &record->peel->bounds;
        if (bounds->top == x || bounds->bottom == x) {
            record->active = 0;
            if (++matches == 2)
                break;
        } else if (bounds->top < x && bounds->bottom > x &&
                   bounds->left == 114 && bounds->right == 127) {
            record->active = 0;
            break;
        }
    }
}

int sink_desert_stone(sbyte cell)
{
    int started;
    uword flags;

    started = trigger_stone_at(1, cell, 1);
    if (started) {
        flags = animation_flags;
        animation_flags = stone_cells[cell].flags;
        animate_desert_stone();
        stone_cells[cell].flags = animation_flags;
        animation_flags = flags;
    }
    return started;
}

void trigger_all_stones_down(void)
{
    if (previous_column != -1 && previous_column >= 2 && previous_column <= 7)
        sink_desert_stone(previous_column + 10);
    sink_desert_stone(actor_column + 10);
}

#pragma optimize("s", on)

int trigger_stone_at(ubyte room, sbyte cell, sbyte action)
{
    int index = cell;
    uword *flags = &room_cells[index + 30 * room].flags;
    uword original = *flags;
    int frame = original & 15;
    int started = 1;

    if (action == 3 && frame == 0) {
        original &= ~0x1f0;
        *flags = original | ((bounded_random(10) + 21) << 4);
    } else if (action == 1) {
        if (frame) {
            play_sound(43);
        } else {
            started = find_tile_animation(room, index) == 0;
            if (original & 0x1f0) {
                original &= ~0x1f0;
                *flags = original;
            }
        }
    } else {
        started = 0;
        if (action == 3 && (sbyte)(cell % 10) == 7 && !query_animation(30, 1))
            add_desert_ambient();
    }

    if (started) {
        start_tile_animation(room, cell, action, 30);
        if (action == 1)
            remove_last_stone_peel((sbyte)(cell % 10));
    }
    return started;
}
#pragma optimize("s", off)

