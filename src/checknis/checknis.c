/* CHECKNIS.C reconstruction. External aliases describe the observed uses. */
#include "types.h"
#include "level.h"
#include "char.h"
#include "game.h"

signed char quote_index = -1;
extern int IsTimeForNIS(int, int);
extern int PlayNIS(int);

extern GAMESTATE GameState;

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

/* Field aliases, original long type, signedness, and purpose are hypotheses. */
extern LEVELREC Level;
extern unsigned long hRebirthSaved;
extern unsigned int Resume;

extern CHARREC Kid;

extern unsigned char CutRoom;

int far pascal IsLevelStartWithNISScreen(void)
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

extern signed char StartLevel;
extern struct { int done; } notice_state;
extern unsigned int Minutes;
extern int frame_diagnostics;
extern int LevelRsrcFiles[];
extern char *find_option(const char far *);
extern int cdecl atoi(const char *);
extern void OpenRsrcVerify(int);
extern void CloseRsrcFile(int);
extern void SetSnd(int sound);

int IsTimeForNIS(int level, int previous_level)
{
    int sequence = 0;
    struct {
        char *arg;
        signed char quote;
    } options;

    if (StartLevel > 2 && !notice_state.done) {
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
                options.quote = (1 - (int)Minutes) / 9 + 7;
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

extern int BPort;
extern char ScreenPortRect[];
extern void UnloadLevel(int mode);
extern void RemoveRebirthPt(void);
extern void DeallocBackPort(int sound);
extern void SetDACBlock(int first, int count, const unsigned char far *colors, int sync);
extern void EraseRect(const char far *rect);
extern void CopyProtectionDialog(void);
extern void TreeCheck(int quote);
extern int Transition(int sequence);
extern int NIS(int sequence);
extern void ResetStdPalette(void);
extern void SetKidPalette(unsigned char room);

int PlayNIS(int sequence)
{
    int result = 1;
    unsigned char room;

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
        SetDACBlock(0, 256, (const unsigned char far *)0, 1);
        EraseRect(ScreenPortRect);
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
        EraseRect(ScreenPortRect);
    ResetStdPalette();
    SetKidPalette(room);
    GameState.playing_nis = 0;
    return result;
}

extern signed char LevelIdx;
extern ROOMREC Room;
extern int NextLevel, Graf;
extern void reset_level_overlays(void);
extern void LoadLevelMap(signed char room);
extern void LoadBGSet(int visible);
extern void LoadPiecesData(void);
extern void LoadCodeOverlays(unsigned char room, unsigned char tile);
extern void SetPrinceLevelSpecPtrs(unsigned char room);
extern void PrepCut(int mode);
extern void DoCleanCut(void);
extern void UnloadSet(int pass);
extern void UnloadPiecesData(void);
extern void nis_noop(void);

void DrawRoomFromNIS(signed char room, signed char level)
{
    unsigned char old_room;
    int saved_sound;

    reset_level_overlays();
    old_room = Level.environment_kind;
    LevelIdx = room;
    NextLevel = room;
    saved_sound = BPort;
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
    BPort = saved_sound;
    nis_noop();
}

typedef struct {
    int first;
    int second;
    int third;
    unsigned char pixels[1];
} image_header;

extern unsigned int hGetSL(int resource_id, int mode);
extern int hNewShpItem1(int asset, int frame, int type);
extern image_header far *h2p(int image);
extern void Mirror4BitBlock(unsigned char far *pixels, int first, int second, int third);
extern void PAINT_INDEXED_SHAPE(image_header far *image, int x, int y, int mode, int plane);
extern void hFree(int image);

extern void SetPalette(int id, unsigned int size, unsigned char type, unsigned char bank);

void DrawDeadKid(int install_palette)
{
    unsigned int shapes;
    int frame, x;
    image_header far *image;

    shapes = hGetSL(25001, 2);
    frame = hNewShpItem1(shapes, 80, 2);
    if (frame) {
        x = Kid.x - 137;
        image = h2p(frame);
        if (Kid.direction == 0) {
            Mirror4BitBlock(image->pixels, image->first, image->second, image->third);
            x -= 19;
        }
        PAINT_INDEXED_SHAPE(image, x, Kid.y - 10, 10, 2);
        hFree(frame);
        hFree(shapes);
    }

    shapes = hGetSL(1000, 0x8000);
    frame = hNewShpItem1(shapes, 11, 2);
    if (frame) {
        PAINT_INDEXED_SHAPE(h2p(frame), 170, 181, 10, 0x8000);
        hFree(frame);
        hFree(shapes);
    }
    if (install_palette)
        SetPalette(1000, 240, 16, 0);
}
