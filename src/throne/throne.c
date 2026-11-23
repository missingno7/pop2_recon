/* THRONE.C, overlay 16. DOS entry order. */
#pragma pack(1)
typedef unsigned char Byte;
typedef unsigned int Handle;
typedef struct { Byte unused[5]; Byte foreground; } GateDraw;
typedef struct { Byte bytes[12025]; } RoomState;
typedef struct { int top, left, bottom, right; } Rect;
#pragma pack()

extern Byte *throne_tiles;                   /* DS:0228 */
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
#pragma pack()

extern CHARREC Kid;
extern int throne_running;                   /* DS:2cf8 */
extern RoomState room_state;                 /* DS:2d02 */
extern Byte viewed_room;                   /* DS:5e34 */
extern CHARREC Char;
extern Byte CutRoom;                     /* DS:6ccd */
extern Byte LevelIdx;                    /* DS:0a18 */
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
    if (Kid.column < 9) {
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
    Char.direction = 0;
    JumpSeq(231);
    CutRoom = *(Byte *)&Char.room = 9;
    reset_char_y();
    Char.life_delta = Char.life_points;
    load_level(LevelIdx);
    saved = RESOLVE_HANDLE_TO_FAR_PAYLOAD(saved_handle);
    room_state = *saved;
    DISPOSE_HANDLE(saved_handle);
    LoadCustomRoomData(9);
    mark_bufs_for_rect(draw_under_actor, (Rect *)&Kid.bounds, 10);
    AnimChar();
    Kid = Char;
    ClrAll();
    scene_flag = 0;
}
