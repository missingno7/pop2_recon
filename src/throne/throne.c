/* THRONE.C, overlay 16. DOS entry order. */
#pragma pack(1)
typedef unsigned char Byte;
typedef unsigned int Handle;
typedef struct { Byte unused[5]; Byte foreground; } GateDraw;
typedef struct { Byte bytes[12025]; } RoomState;
typedef struct { Byte bytes[64]; } CharState;
typedef struct { int top, left, bottom, right; } Rect;
#pragma pack()

extern Byte *throne_tiles;                   /* DS:0228 */
extern signed char opponent_sequence;        /* DS:5c89 */
extern int throne_running;                   /* DS:2cf8 */
extern RoomState room_state;                 /* DS:2d02 */
extern Byte room_lighting;                   /* DS:5e34 */
extern Byte char_direction, char_room;        /* DS:5c01,5c0e */
extern Byte room_number;                     /* DS:6ccd */
extern Byte char_image_x, char_old_image_x;   /* DS:5c12,5c14 */
extern Byte throne_sound;                    /* DS:0a18 */
extern Rect throne_rect;                     /* DS:5c9b */
extern CharState current_char, opponent;      /* DS:5c00,5c80 */
extern Byte scene_flag;                      /* DS:6290 */

extern void far _fastcall draw_tile_image(Byte, Byte);
extern void set_resource(int, unsigned int, Byte, Byte), start_sound(int);
extern Handle allocate_handle(long);
extern RoomState far *locate_handle(Handle);
extern void free_handle(Handle);
extern int check_scene_state(int);
extern void update_scene(void), load_opponent(void), sync_char(void);
extern void set_sequence(int);
extern void play_scene(Byte);
extern void setup_throne_room(Byte);
typedef void far pascal DrawCallback(void);
extern DrawCallback draw_under_actor;
extern void redraw_cells(DrawCallback *, Rect *, Byte);
extern void save_kid(void), reset_char(void);

void draw_throne_room_gate_in_fore(void)
{
    GateDraw *gate = (GateDraw *)(throne_tiles + 203);
    gate->foreground = 1;
    draw_tile_image(7, 1);
    gate->foreground = 0;
}

void load_throne_room(void)
{
    set_resource(3500, 64, 160, 2);
    start_sound(opponent_sequence < 9 ? 253 : 152);
}

void unload_throne_room(void)
{
    if (!throne_running)
        set_resource(3500, 64, 160, 0);
}

void play_throne_room_nis(void)
{
    Handle saved_handle;
    RoomState far *saved;

    saved_handle = allocate_handle(sizeof(RoomState));
    saved = locate_handle(saved_handle);
    *saved = room_state;
    if (check_scene_state(6) == 2)
        room_lighting = 0;
    update_scene();
    load_opponent();
    char_direction = 0;
    set_sequence(231);
    room_number = char_room = 9;
    sync_char();
    char_old_image_x = char_image_x;
    play_scene(throne_sound);
    saved = locate_handle(saved_handle);
    room_state = *saved;
    free_handle(saved_handle);
    setup_throne_room(9);
    redraw_cells(draw_under_actor, &throne_rect, 10);
    save_kid();
    opponent = current_char;
    reset_char();
    scene_flag = 0;
}
