/* THRONE.C, overlay 16. DOS entry order. */
#include "types.h"
#include "char.h"
#include "game.h"
#pragma pack(1)

typedef unsigned int Handle;
typedef struct { byte unused[5]; byte foreground; } GateDraw;
typedef struct { byte bytes[12025]; } RoomState;

#pragma pack()

extern byte *custom_room;                   /* DS:0228 */

extern CHARREC Kid;

extern GAMESTATE GameState;
extern int CDLeftEdge;
extern AmbientState ambient_state;
extern RoomState Level;                 /* DS:2d02 */
extern byte Room;                   /* DS:5e34 */
extern CHARREC Char;
extern byte CutRoom;                     /* DS:6ccd */
extern byte LevelIdx;                    /* DS:0a18 */

extern Directions Dir;

extern void far _fastcall draw_custom_room_image(byte, byte);
extern void set_palette(int, unsigned int, byte, byte), AddSong(int);
extern Handle ALLOCATE_PAYLOAD_HANDLE(long);
extern RoomState far *RESOLVE_HANDLE_TO_FAR_PAYLOAD(Handle);
extern void DISPOSE_HANDLE(Handle);
extern int play_nis(int);
extern void reset_level_overlays(void), copy_kid_to_char(void), reset_char_y(void);
extern void JumpSeq(int);
extern void load_level(byte);
extern void LoadCustomRoomData(byte);
typedef void far pascal DrawCallback(void);
extern DrawCallback mark_move;
extern void mark_bufs_for_rect(DrawCallback *, Rect *, byte);
extern void AnimChar(void), ClrAll(void);
extern void load_code_overlays(byte, byte), mark_redraw(int);
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
    CutRoom = Char.room = 9;
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
