/* THRONE.C, overlay 16. DOS entry order. */
#include "types.h"
#include "char.h"
#include "game.h"
#pragma pack(1)

typedef unsigned int Handle;
typedef struct { byte unused[5]; byte foreground; } GateDraw;
typedef struct { byte bytes[12025]; } RoomState;

#pragma pack()

extern byte *CustomRoom;                   /* DS:0228 */

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

extern void far _fastcall DrawCustomRoomImg(byte, byte);
extern void SetPalette(int, unsigned int, byte, byte), AddSong(int);
extern Handle hNew(long);
extern RoomState far *h2p(Handle);
extern void hFree(Handle);
extern int PlayNIS(int);
extern void reset_level_overlays(void), LoadKid(void), ResetCharY(void);
extern void JumpSeq(int);
extern void LoadLevel(byte);
extern void LoadCustomRoomData(byte);
typedef void far pascal DrawCallback(void);
extern DrawCallback MarkMove;
extern void MarkBufsForRect(DrawCallback *, Rect *, byte);
extern void AnimChar(void), ClrAll(void);
extern void LoadCodeOverlays(byte, byte), MarkRedraw(int);
extern int SndOn(int);
extern void UpdateAmbient(int *, int);
extern void OffsetRect(Rect far *, Rect far *, int, int);
extern void SetScriptRect(int, Rect far *);

void DrawThroneRoomGateInFore(void)
{
    GateDraw *gate = (GateDraw *)(CustomRoom + 203);
    gate->foreground = 1;
    DrawCustomRoomImg(7, 1);
    gate->foreground = 0;
}

void LoadRNThroneFunc(void)
{
    SetPalette(3500, 64, 160, 2);
    if (!GameState.throne_nis_played) {
        if (Kid.column < 9)
            AddSong(253);
        else if (!GameState.throne_song_added) {
            AddSong(152);
            GameState.throne_song_added = 1;
        }
    }
}

void UnloadRNThroneFunc(void)
{
    if (!GameState.playing_nis)
        SetPalette(3500, 64, 160, 0);
}

void PlayThroneRoomNIS(void)
{
    Handle saved_handle;
    int nis_result;
    RoomState far *saved;

    saved_handle = hNew(sizeof(RoomState));
    saved = h2p(saved_handle);
    *saved = Level;
    nis_result = PlayNIS(6);
    if (nis_result == 2)
        Room = 0;
    reset_level_overlays();
    LoadCodeOverlays(4, 5);
    LoadKid();
    Char.direction = 0;
    JumpSeq(231);
    CutRoom = Char.room = 9;
    ResetCharY();
    Char.life_delta = Char.life_points;
    LoadLevel(LevelIdx = 8);
    saved = h2p(saved_handle);
    Level = *saved;
    hFree(saved_handle);
    GameState.throne_nis_played = 1;
    if (nis_result != 2)
        LoadCustomRoomData(9);
    MarkBufsForRect(MarkMove, (Rect *)&Kid.bounds, 10);
    MarkRedraw(25);
    AnimChar();
    Kid = Char;
    ClrAll();
    Dir.action = 0;
    if (!SndOn(10255))
        AddSong(255);
    else
        UpdateAmbient(&ambient_state, 255);
}

void ThroneMoveQueenInFrontOfKid(Rect *rect, int font)
{
    OffsetRect(rect, rect, CDLeftEdge - rect->left - 135, -4);
    SetScriptRect(font, rect);
}
