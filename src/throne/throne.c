/* THRONE.C, overlay 16. DOS entry order. */
#pragma pack(1)
typedef unsigned char Byte;
typedef unsigned int Handle;
typedef struct { Byte unused[5]; Byte foreground; } GateDraw;
typedef struct { Byte bytes[12025]; } RoomState;
typedef struct { int top, left, bottom, right; } Rect;
#pragma pack()

extern Byte *custom_room;                   /* DS:0228 */
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
typedef struct {
    int control_mode, fade_active, playing_nis, recording_mode, skip_opening, field_0a;
    int plateau_loaded, throne_song_added, throne_nis_played, escher_room_eight_seen, nis_room_restored;
} GAMESTATE;
extern GAMESTATE GameState;
extern int CDLeftEdge;
extern struct {
    unsigned char enabled;
    signed char cue;
    int song, delay, reserved[2];
} ambient_state;
extern RoomState Level;                 /* DS:2d02 */
extern Byte Room;                   /* DS:5e34 */
extern CHARREC Char;
extern Byte CutRoom;                     /* DS:6ccd */
extern Byte LevelIdx;                    /* DS:0a18 */
typedef struct {
    signed char forward, back, up, down, action;
} Directions;
extern Directions Dir;

extern void far _fastcall draw_custom_room_image(Byte, Byte);
extern void set_palette(int, unsigned int, Byte, Byte), AddSong(int);
extern Handle ALLOCATE_PAYLOAD_HANDLE(long);
extern RoomState far *RESOLVE_HANDLE_TO_FAR_PAYLOAD(Handle);
extern void DISPOSE_HANDLE(Handle);
extern int play_nis(int);
extern void reset_level_overlays(void), copy_kid_to_char(void), reset_char_y(void);
extern void JumpSeq(int);
extern void load_level(Byte);
extern void LoadCustomRoomData(Byte);
typedef void far pascal DrawCallback(void);
extern DrawCallback mark_move;
extern void mark_bufs_for_rect(DrawCallback *, Rect *, Byte);
extern void AnimChar(void), ClrAll(void);
extern void load_code_overlays(Byte, Byte), mark_redraw(int);
extern int query_sound(int);
extern void remember_ambient_song(int *, int);
extern void OffsetRect(Rect far *, Rect far *, int, int);
extern void store_scene_rect(int, Rect far *);

void draw_throne_room_gate_in_fore(void)
{
    GateDraw *gate = (GateDraw *)(custom_room + 203);
    gate->foreground = 1;
    draw_custom_room_image(7, 1);
    gate->foreground = 0;
}

void load_throne_room(void)
{
    set_palette(3500, 64, 160, 2);
    if (!GameState.throne_nis_played) {
        if (Kid.column < 9)
            AddSong(253);
        else if (!GameState.throne_song_added) {
            AddSong(152);
            GameState.throne_song_added = 1;
        }
    }
}

void unload_throne_room(void)
{
    if (!GameState.playing_nis)
        set_palette(3500, 64, 160, 0);
}

void play_throne_room_nis(void)
{
    Handle saved_handle;
    int nis_result;
    RoomState far *saved;

    saved_handle = ALLOCATE_PAYLOAD_HANDLE(sizeof(RoomState));
    saved = RESOLVE_HANDLE_TO_FAR_PAYLOAD(saved_handle);
    *saved = Level;
    nis_result = play_nis(6);
    if (nis_result == 2)
        Room = 0;
    reset_level_overlays();
    load_code_overlays(4, 5);
    copy_kid_to_char();
    Char.direction = 0;
    JumpSeq(231);
    CutRoom = *(Byte *)&Char.room = 9;
    reset_char_y();
    Char.life_delta = Char.life_points;
    load_level(LevelIdx = 8);
    saved = RESOLVE_HANDLE_TO_FAR_PAYLOAD(saved_handle);
    Level = *saved;
    DISPOSE_HANDLE(saved_handle);
    GameState.throne_nis_played = 1;
    if (nis_result != 2)
        LoadCustomRoomData(9);
    mark_bufs_for_rect(mark_move, (Rect *)&Kid.bounds, 10);
    mark_redraw(25);
    AnimChar();
    Kid = Char;
    ClrAll();
    Dir.action = 0;
    if (!query_sound(10255))
        AddSong(255);
    else
        remember_ambient_song(&ambient_state, 255);
}

void offset_throne_rect(Rect *rect, int font)
{
    OffsetRect(rect, rect, CDLeftEdge - rect->left - 135, -4);
    store_scene_rect(font, rect);
}
