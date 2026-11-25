#include <stdlib.h>
#include "types.h"
#include "snd.h"
#include "graf.h"
#include "shape.h"
#include "level.h"
#include "heap.h"
#include "char.h"
#include "game.h"
#include "draw.h"
#include "checknis.h"
#include "custcvca.h"
#include "diag.h"
#include "dlghelp.h"
#include "dialog.h"
#include "fade.h"
#include "anim.h"
#include "hnewshpi.h"
#include "intrface.h"
#include "lgraf.h"
#include "load.h"
#include "nis.h"
#include "prince.h"
#include "top.h"

sbyte quote_index = -1;


int CheckPlayNIS(int level, int previous_level)
{
    int sequence;
    int result;

    sequence = IsTimeForNIS(level, previous_level);
    if (sequence) {
        result = PlayNIS(sequence);
    } else {
        result = 0;
    }
    GameState.nis_room_restored = result == 2 ? 1 : 0;
    return result;
}

int EraseScreenAfterNIS(int x)
{
    int result;

    if (x == 1 || x == 100 || x == 2 || x == 3 || x == 6)
        result = 0;
    else
        result = 1;

    return result;
}

int EraseScreenBeforeNIS(int sequence)
{
    int pre_screen_sequence;
    int result;

    /* DOS tests an uninitialized stack word for the case-4 exception. */
    if (sequence == 1 || sequence == 100 || pre_screen_sequence == 4 || sequence == 6)
        result = 0;
    else
        result = 1;
    return result;
}


int IsLevelStartWithNISScreen(void)
{
    int result = 0;
    if (hRebirthSaved == 0L && Resume == 0 && GameState.nis_room_restored == 0) {
        if ((Level.entry_room == 27 && Level.level_number == 6) ||
            (Level.entry_room == 22 && Level.level_number == 10) ||
            (Level.entry_room == 1 && Level.level_number == 14) ||
            (CutRoom == 9 && Level.level_number == 8))
            result = 1;
    } else if (hRebirthSaved != 0L && Resume == 0 && GameState.nis_room_restored == 0 &&
               Kid.room == 9 && Level.level_number == 8) {
        result = 1;
    }
    return result;
}


int IsTimeForNIS(int level, int previous_level)
{
    int sequence = 0;
    struct {
        char *arg;
        sbyte quote;
    } options;

    if (StartLevel > 2 && !notice_state) {
        sequence = 100;
    } else if (StartLevel) {
        if (level == 0 || (previous_level != level && previous_level != -1)) {
            switch (level) {
            case 1:
                sequence = 9;
                break;
            case 2: sequence = 100; break;
            case 3: sequence = 10; break;
            case 5: sequence = 1; break;
            case 9: sequence = 2; break;
            case 13: sequence = 3; break;
            }
        }
        if (sequence == 0 && level >= 4) {
            if (quote_index == -1 && level >= 4) {
                sequence = 20;
                quote_index = 0;
            } else {
                options.quote = (1 - Minutes) / 9 + 7;
                if (options.quote > quote_index) {
                    sequence = options.quote + 20;
                    quote_index = options.quote;
                }
            }
        }
    }

    if (frame_diagnostics) {
        if ((options.arg = find_option("NIS")) != 0) {
            sequence = atoi(options.arg + 3);
            if (sequence == 1) {
                OpenRsrcVerify(LevelRsrcFiles[3]);
                SetSnd(10034);
            } else if (sequence == 5 || sequence == 11) {
                CloseRsrcFile(LevelRsrcFiles[6]);
                OpenRsrcVerify(LevelRsrcFiles[6]);
            } else {
                OpenRsrcVerify(LevelRsrcFiles[4]);
            }
        } else if ((options.arg = find_option("TREE")) != 0) {
            quote_index = atoi(options.arg + 4);
            sequence = quote_index + 19;
        }
    }
    return sequence;
}


int PlayNIS(int sequence)
{
    int result = 1;
    byte room;

    GameState.playing_nis = result;
    room = Level.environment_kind;
    if (sequence != 100) {
        UnloadLevel(0);
        if (sequence < 20 && sequence != 6)
            RemoveRebirthPt();
        if (BPort != 0) {
            DeallocBackPort(BPort);
            BPort = 0;
        }
    }
    if (EraseScreenBeforeNIS(sequence)) {
        SetDACBlock(0, 256, (const byte far *)0, 1);
        EraseRect(&ScreenPortRect);
    }
    if (sequence == 100) {
        CopyProtectionDialog();
    } else if (sequence >= 20) {
        TreeCheck(sequence - 19);
    } else if (sequence >= 1 && sequence <= 6) {
        if (Transition(sequence))
            result = 2;
        switch (sequence) {
        case 1: room = 4; break;
        case 2: room = 2; break;
        case 3: room = 6; break;
        }
    } else {
        if (NIS(sequence))
            result = 2;
    }
    if (EraseScreenAfterNIS(sequence))
        EraseRect(&ScreenPortRect);
    ResetStdPalette();
    SetKidPalette(room);
    GameState.playing_nis = 0;
    return result;
}


void DrawRoomFromNIS(sbyte room, sbyte level)
{
    byte old_room;
    PORT *saved_port;

    reset_level_overlays();
    old_room = Level.environment_kind;
    LevelIdx = room;
    NextLevel = room;
    saved_port = BPort;
    BPort = Graf;
    LoadLevelMap(room);
    OpenRsrcVerify(LevelRsrcFiles[Level.environment_kind]);
    LoadBGSet(!IsLevelStartWithNISScreen());
    if (Level.environment_kind != old_room)
        LoadPiecesData();
    LoadCodeOverlays(Level.environment_kind, Level.preferred_opponent);
    SetPrinceLevelSpecPtrs(Level.environment_kind);
    Room.number = 0;
    CutRoom = level;
    PrepCut(1);
    DoCleanCut();
    UnloadSet(4);
    UnloadSet(1);
    UnloadPiecesData();
    CloseRsrcFile(LevelRsrcFiles[Level.environment_kind]);
    Level.environment_kind = 0;
    BPort = saved_port;
    nis_noop();
}


void DrawDeadKid(int install_palette)
{
    Handle shapes;
    Handle frame;
    int x;
    SHAPE far *image;

    shapes = hGetSL(25001, 2);
    frame = hNewShpItem1(shapes, 80, 2);
    if (frame) {
        x = Kid.x - 137;
        image = h2p(frame);
        if (Kid.direction == 0) {
            Mirror4BitBlock(image->pixels, image->height, image->width, image->format);
            x -= 19;
        }
        paint_indexed_shape(image, x, Kid.y - 10, 10, 2);
        hFree(frame);
        hFree(shapes);
    }

    shapes = hGetSL(1000, 0x8000);
    frame = hNewShpItem1(shapes, 11, 2);
    if (frame) {
        paint_indexed_shape(h2p(frame), 170, 181, 10, 0x8000);
        hFree(frame);
        hFree(shapes);
    }
    if (install_palette)
        SetPalette(1000, 240, 16, 0);
}
