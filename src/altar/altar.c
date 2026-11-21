/* ALTAR.C, overlay 15. DOS entry order. */
#pragma pack(1)
typedef unsigned char Byte;
typedef unsigned int Handle;
typedef struct { int top, left, bottom, right; } Rect;
typedef struct {
    signed char id, direction;
    int x, y;
    Byte action;
    unsigned int frame;
    signed char col, row;
    char unknown_b[3];
    Byte room;
    Byte unk_f, alive;
    signed char control, image, previous_image;
    char unk_14[5];
    int sequence;
    char unk_1b[8];
    Byte unk_23;
    int program;
    char unk_26[14];
    int state;
    char tail[10];
} Character;
typedef struct {
    signed char type;
    int x, y;
    Byte pass, palette;
    int resource;
    Handle shape;
    Rect rect;
    Byte flags, peel, unknown[4];
} RoomPiece;
typedef struct {
    Byte count, type;
    int palette;
    char unknown[24];
    RoomPiece pieces[1];
} RoomData;
typedef struct {
    signed char position;
    int x;
    signed char direction;
    Byte flag, unknown[4], mode, id, slot, enabled, flag2;
} Spawn;
typedef struct { char unknown[116]; } Room;
typedef struct { int size; char unknown[2]; unsigned int flags; char bytes[97]; } Shape;
typedef struct { char data[4]; Handle handle; } ShapeRef;
typedef struct { Byte type; char skip[3]; unsigned int modifier; } TileAnim;
typedef struct { Byte frame, unknown; signed char col, dy; } FlameStep;
#pragma pack()

extern Character current_char, opponent, room_chars[];
extern signed char char_action;
extern signed char char_image;
extern Byte scene_flag;                       /* DS:6290 */
extern unsigned int candle_sound;             /* DS:3256 */
extern int spawn_flag;                        /* DS:0a16 */
extern RoomData *room_data;                    /* DS:0228 */
extern Byte altar_tile;                       /* DS:4549 */
extern signed char room_char_count;           /* DS:46c5 */
extern signed char room_guard_counts[];       /* DS:44f5, stride 116 */
extern unsigned int tile_modifier;            /* DS:5e48 */
extern signed char tile_animation;            /* DS:67d0 */
extern Byte flame_image_map[];                 /* DS:1e2e */
extern Rect current_clip, flame_clip;         /* DS:6236,09fe */
extern Handle *shape_source;                  /* DS:6242 */
extern Handle flame_steps_handle;             /* DS:6268 */
extern int flame_step, actor_y;                /* DS:5e12,6256 */

extern void save_state(void), load_other(void), load_state(signed char), remove_character(void);
extern void update_char(void), position_char(int, int), start_sound(int), play_sound(int);
extern void set_sequence(int), walk_distance(int), set_action(int), finish_scene(void);
extern int advance_animation(void);
extern void process_animation(void);
extern int column_x(int), row_offset(signed char), transform_column(int);
extern void move_char(Byte), set_char_state(Byte);
extern int palette_loaded(int);
extern Spawn *get_spawn(unsigned int, int);
extern void calculate_y(void), align_char(void), advance_char(void);
extern void set_resource(int, int, Byte, Byte);
extern void far _fastcall draw_tile_image(Byte, Byte);
typedef void far pascal DrawCallback(void);
extern DrawCallback draw_under_actor;
extern void redraw_cells(DrawCallback *, Rect *, signed char);
extern Handle allocate_handle(long);
extern void far *locate_handle(Handle);
extern void free_handle(Handle);
extern void create_shape(Handle, int, ShapeRef far *, int);
extern void free_resource_shape(int);
extern void shape_rect(Rect far *, int, int, void far *);
extern void enter_char_context(void), leave_char_context(void);

void altar_kid_rises_from_death(void);
void create_shadowman(void);

void altar_kid_rises_from_death(void)
{
    signed char id = current_char.id;
    save_state();
    load_other();
    set_sequence(231);
    current_char.control = -1;
    char_image = current_char.previous_image;
    update_char();
    position_char(char_image, current_char.previous_image);
    current_char.program = 13;
    current_char.alive = 0;
    opponent = current_char;
    load_state(id);
    remove_character();
    start_sound(60);
}

void altar_shadow_progress(void)
{
    int sequence = -1;
    int distance;
    if (current_char.frame == 15) {
        if (current_char.direction == -1) {
            if (current_char.col >= 6) {
                if (current_char.program == 13)
                    goto turn;
                else if (current_char.col >= 8)
                    sequence = 1;
                else {
                    walk_distance(0);
                    if (current_char.sequence == 44)
                        sequence = 3;
                }
            } else if (current_char.program == 13 || current_char.sequence == 232) {
                sequence = 5;
                start_sound(59);
            } else {
                sequence = 232;
            }
        } else if (current_char.program != 13) {
            sequence = 5;
        } else if (current_char.col < 6) {
            walk_distance(0);
            if (current_char.sequence == 44)
                sequence = 3;
        } else {
            distance = opponent.x - current_char.x;
            if (opponent.direction == current_char.direction)
                distance += 6;
            if (distance)
                walk_distance(distance);
            else if (opponent.direction != current_char.direction)
                sequence = 5;
            else {
turn:
                sequence = 71;
            }
        }
    } else if (((current_char.frame >= 1 && current_char.frame <= 14) ||
                (current_char.frame >= 49 && current_char.frame <= 56)) &&
               current_char.col >= 6) {
        set_action(4);
    } else if (current_char.frame >= 103 && current_char.frame <= 106) {
        scene_flag = 255;
        finish_scene();
    } else if (current_char.frame == 91 && !advance_animation()) {
        process_animation();
        if (current_char.program == 13)
            current_char.y -= 2;
    } else if (current_char.frame == 154) {
        candle_sound = 133;
    } else if (current_char.frame == 153 && current_char.program == 13) {
        set_resource(room_data->palette, 48, 16, 0);
    } else if (current_char.program == 13 && current_char.frame == 185) {
        altar_kid_rises_from_death();
    } else if (current_char.frame == 44) {
        sequence = 13;
        current_char.x = column_x(-8);
    }
    if (sequence != -1)
        set_sequence(sequence);
}

void altar_watch_kid(void)
{
    load_other();
    if (current_char.row == 1 && current_char.sequence != 230) {
        if (current_char.x <= 218) {
            set_sequence(230);
            current_char.x = 210;
            move_char(100);
            play_sound(57);
            set_char_state(16);
            candle_sound = 133;
        } else if (current_char.x <= 236 && !palette_loaded(10061)) {
            start_sound(61);
        } else if (current_char.frame == 185 && current_char.row == 1 &&
                   current_char.program != 12 && spawn_flag == 1) {
            create_shadowman();
            spawn_flag = -1;
        }
    }
    opponent = current_char;
}

void create_shadowman(void)
{
    Spawn *entry;
    signed char id, zero, clear;
    int room;
    if (current_char.room == 4 && altar_tile == 13) {
        opponent = current_char;
        room = current_char.room;
        id = room_guard_counts[room * (int)sizeof(Room)]++;
        entry = get_spawn(room, id);
        entry->position = row_offset(current_char.row) + current_char.col;
        entry->x = current_char.x;
        entry->direction = current_char.direction;
        clear = 0;
        entry->flag = clear;
        entry->mode = clear;
        entry->id = id;
        zero = 0;
        entry->slot = zero;
        entry->flag2 = zero;
        entry->enabled = 1;
        current_char.id = id;
        current_char.x = entry->x;
        char_image = char_action = 1;
        current_char.control = -1;
        current_char.state = 8;
        calculate_y();
        --current_char.y;
        align_char();
        current_char.program = 0;
        current_char.unk_23 = current_char.alive = 0;
        set_sequence(231);
        advance_char();
        start_sound(63);
        save_state();
        load_other();
    }
}

int altar_shadow_active(void)
{
    int result = 0;
    signed char i, count;
    i = 0;
    count = room_char_count;
    if (count > 0) {
        do {
            if (result)
                break;
            if (room_chars[i].action == 1)
                result = 1;
        } while (++i < count);
    }
    return result;
}

void animate_sacred_flame(void)
{
    unsigned int phase;
    Byte image;
    if (room_data && room_data->type == 32) {
        phase = tile_modifier;
        if (phase & 0x80) {
            if (!(phase & 15)) {
                phase = 0;
                image = 11;
            } else {
                --phase;
                image = flame_image_map[phase & 15];
            }
        } else {
            if (phase < 11)
                ++phase;
            else
                phase = 0;
            image = phase + 11;
        }
        redraw_cells(draw_under_actor, &room_data->pieces[image].rect, -1);
        tile_modifier = phase;
    } else {
        tile_animation = -1;
    }
}

void draw_temp_sacred_flame(TileAnim *tile)
{
    struct { Rect saved; Byte align, image; unsigned int modifier; } draw;
    RoomPiece *piece;
    if (tile->type == 11) {
        draw.saved = current_clip;
        current_clip = flame_clip;
        draw.modifier = tile->modifier;
        if (draw.modifier & 0x80) {
            draw.image = flame_image_map[draw.modifier & 0xff7f];
            piece = &room_data->pieces[draw.image];
            piece->pass = 1;
            draw_tile_image(draw.image, 1);
            piece->pass = 11;
        } else {
            draw_tile_image((Byte)draw.modifier + 11, 11);
        }
        current_clip = draw.saved;
    }
}

void load_altar_room(void)
{
    int i;
    Handle handle;
    register RoomPiece *piece;
    Shape far *source, far *copy;
    ShapeRef ref;

    handle = allocate_handle(sizeof(Shape));
    source = locate_handle(*shape_source);
    copy = locate_handle(handle);
    *copy = *source;
    copy->flags = 0x4000;
    copy->size = room_data->palette;
    i = 11;
    do {
        piece = &room_data->pieces[i];
        free_handle(piece->shape);
        ref.handle = 0;
        create_shape(handle, piece->type + 1, &ref, 1);
        piece->shape = ref.handle;
        free_resource_shape(piece->type + room_data->palette);
    } while (++i <= 38);
    free_handle(handle);
    set_resource(room_data->palette, 224, 16, 0);
    set_resource(2000, 48, 16, 0);
}

void setup_flame(void)
{
    Rect saved;
    FlameStep far *steps;
    Byte image;
    RoomPiece *piece;
    if (current_char.room == 4 && altar_tile == 13) {
        enter_char_context();
        saved = current_clip;
        steps = locate_handle(flame_steps_handle);
        image = steps[flame_step].frame + 23;
        piece = &room_data->pieces[image];
        piece->y = transform_column(steps[flame_step].col);
        piece->x = steps[flame_step].dy + actor_y;
        shape_rect(&piece->rect, piece->x, piece->y, locate_handle(piece->shape));
        current_clip = piece->rect;
        piece->pass = 0;
        draw_tile_image(image, 0);
        piece->pass = 11;
        current_clip = saved;
        leave_char_context();
    }
}

void unload_altar_room(void)
{
    set_resource(3000, 224, 16, 0);
}
