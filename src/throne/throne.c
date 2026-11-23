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
extern Byte viewed_room;                   /* DS:5e34 */
extern Byte char_direction, char_room;        /* DS:5c01,5c0e */
extern Byte CutRoom;                     /* DS:6ccd */
extern Byte char_image_x, char_old_image_x;   /* DS:5c12,5c14 */
extern Byte LevelIdx;                    /* DS:0a18 */
extern Rect throne_rect;                     /* DS:5c9b */
extern CharState Char, Kid;      /* DS:5c00,5c80 */
extern Byte scene_flag;                      /* DS:6290 */

extern void far _fastcall draw_tile_image(Byte, Byte);
extern void set_resource(int, unsigned int, Byte, Byte), AddSong(int);
extern Handle ALLOCATE_PAYLOAD_HANDLE(long);
extern RoomState far *RESOLVE_HANDLE_TO_FAR_PAYLOAD(Handle);
extern void DISPOSE_HANDLE(Handle);
extern int play_nis(int);
extern void reset_level_overlays(void), copy_kid_to_char(void), reset_char_y(void);
extern void JumpSeq(int);
extern void load_level(Byte);
extern void LoadCustomRoomData(Byte);
typedef void far pascal DrawCallback(void);
extern DrawCallback draw_under_actor;
extern void mark_bufs_for_rect(DrawCallback *, Rect *, Byte);
extern void AnimChar(void), ClrAll(void);

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
    if (opponent_sequence < 9) {
        AddSong(253);
        return;
    }
    AddSong(152);
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

    saved_handle = ALLOCATE_PAYLOAD_HANDLE(sizeof(RoomState));
    saved = RESOLVE_HANDLE_TO_FAR_PAYLOAD(saved_handle);
    *saved = room_state;
    if (play_nis(6) == 2)
        viewed_room = 0;
    reset_level_overlays();
    copy_kid_to_char();
    char_direction = 0;
    JumpSeq(231);
    CutRoom = char_room = 9;
    reset_char_y();
    char_old_image_x = char_image_x;
    load_level(LevelIdx);
    saved = RESOLVE_HANDLE_TO_FAR_PAYLOAD(saved_handle);
    room_state = *saved;
    DISPOSE_HANDLE(saved_handle);
    LoadCustomRoomData(9);
    mark_bufs_for_rect(draw_under_actor, &throne_rect, 10);
    AnimChar();
    Kid = Char;
    ClrAll();
    scene_flag = 0;
}
