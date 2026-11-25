/* THRONE.C, overlay 16. DOS entry order. */
#include "types.h"
#include "snd.h"
#include "graf.h"
#include "shape.h"
#include "level.h"
#include "heap.h"
#include "char.h"
#include "game.h"
#include "draw.h"
#include "custom.h"
#include "act.h"
#include "checknis.h"
#include "ctrl.h"
#include "fade.h"
#include "load.h"
#include "move.h"
#include "prince.h"
#include "sound.h"
#include "throne.h"
#include "top.h"
#include "util.h"


                   /* DS:0228 */


                 /* DS:2d02 */

                   /* DS:5e34 */


                     /* DS:6ccd */

                    /* DS:0a18 */


typedef void DrawCallback(void);


void DrawThroneRoomGateInFore(void)
{
    CUSTOMPIECE *piece = &CustomRoom->pieces[7];

    piece->pass = 1;
    DrawCustomRoomImg(7, 1);
    piece->pass = 0;
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
    LEVELREC far *saved;

    saved_handle = hNew(sizeof(LEVELREC));
    saved = h2p(saved_handle);
    *saved = Level;
    nis_result = PlayNIS(6);
    if (nis_result == 2)
        Room.number = 0;
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
    MarkBufsForRect(MarkMove, &Kid.bounds, 10);
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
