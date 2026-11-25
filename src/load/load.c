#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include "types.h"
#include "level.h"
#include "snd.h"
#include "resc.h"
#include "graf.h"
#include "shape.h"
#include "heap.h"
#include "char.h"
#include "game.h"
#include "util.h"
#include "draw.h"
#include "custom.h"
#include "movecave.h"
#include "autoguar.h"
#include "dlghelp.h"
#include "checknis.h"
#include "autohead.h"
#include "add.h"
#include "desert.h"
#include "dialog.h"
#include "auto.h"
#include "drawcave.h"
#include "collide.h"
#include "drawruin.h"
#include "drawtemp.h"
#include "english.h"
#include "fade.h"
#include "final.h"
#include "init.h"
#include "anim.h"
#include "intrface.h"
#include "lgraf.h"
#include "load.h"
#include "misc.h"
#include "move.h"
#include "moveruin.h"
#include "tables.h"
#include "prince.h"
#include "nis.h"
#include "rooftops.h"
#include "movetemp.h"
#include "sound.h"
#include "top.h"

byte OppBank[2];
Handle SwordFrames;


/* Resource openers replace the initial name pointers with file handles. */
const char *resource_files[3] = {
    "PRINCE.DAT", "KID.DAT", "SEQUENCE.DAT"
};
const char *LevelRsrcFiles[7] = {
    0, "DESERT.DAT", "TEMPLE.DAT", "CAVERNS.DAT",
    "RUINS.DAT", "ROOFTOPS.DAT", "FINAL.DAT"
};

int BGSetMarkedPurgeable = 0;
const int Level1stOverlay[7] = { -1, -1, 78, 99, 89, -1, -1 };

static const int temple_image_first[] = { 0, 22, 29, 33, 38, 40, 42, 45, 49, 90, -1 };
static const int temple_image_last[] = { 20, 27, 30, 34, 38, 40, 42, 45, 62, 125 };
static const int cavern_image_first[] = { 0, 15, 28, 33, 40, 44, 51, 63, 73, 99, -1 };
static const int cavern_image_last[] = { 5, 23, 29, 36, 41, 49, 51, 66, 90, 125 };
static const int ruin_image_first[] = { 0, 18, 21, 29, 35, 41, 48, 50, 52, 89, -1 };
static const int ruin_image_last[] = { 16, 18, 24, 31, 37, 42, 48, 50, 79, 113 };

static const int *background_image_first[7] = {
    0, 0, temple_image_first, cavern_image_first, ruin_image_first, 0, 0
};
static const int *background_image_last[7] = {
    0, 0, temple_image_last, cavern_image_last, ruin_image_last, 0, 0
};


typedef void (*LevelFrameProc)(void);


LevelFrameProc LevelFrameFunc[7] = {
    0, DsrtFrameFunc, check_temple_event, CaveFrameFunc, 0, RoofFrameFunc, FinlFrameFunc
};

sbyte LastOppTypeLoaded = -1;
static sbyte loaded_sword = 0;
static const char *opponent_resource_files[11] = {
    "GUARD.DAT", "FLAME.DAT", "SKELETON.DAT", "GUARD.DAT",
    0, "HEAD.DAT", "HEAD.DAT", "BIRD.DAT", "HEAD.DAT",
    "JINNEE.DAT", 0
};

static int guard_overlay_first[] = { 31, 47, 58, 74, -1 };
static int guard_overlay_last[] = { 46, 57, 73, 85 };
static int skeleton_overlay_first[] = { 33, 49, -1 };
static int skeleton_overlay_last[] = { 48, 52 };
static int head_overlay_first[] = { 0, -1 };
static int head_overlay_last[] = { 116 };
static int bird_overlay_first[] = { 40, 56, 67, 95, -1 };
static int bird_overlay_last[] = { 55, 66, 94, 98 };

int *Opp1stRange[11] = {
    guard_overlay_first, 0, skeleton_overlay_first, 0, 0, head_overlay_first, 0,
    bird_overlay_first, 0, 0, 0
};
static int *opponent_overlay_last[11] = {
    guard_overlay_last, 0, skeleton_overlay_last, 0, 0, head_overlay_last, 0,
    bird_overlay_last, 0, 0, 0
};

static int sword_overlay_first[1] = { 47 };
static int sword_overlay_last[1] = { 57 };
int Need1stImg[8] = { 115, 127, 90, 95, 100, 79, 91, 105 };
int NeedLastImg[8] = { 126, 161, 91, 114, 103, 90, 94, 108 };
static const int kid_overlay_first[11] = { 263, 223, 241, 244, 249, 269, 222, 271, 280, 290, 300 };
static const int kid_overlay_last[11] = { 268, 240, 243, 248, 262, 270, 222, 279, 289, 298, 313 };
static const int kid_image_first[9] = { 15, 44, 99, 120, 133, 160, 219, 222, -1 };
static const int kid_image_last[8] = { 32, 79, 111, 129, 144, 215, 220, 299 };
static const char map_option[] = "GAMEPLAY\0%s%s";


void LoadBGOverlays(void);
void LoadKidSet(void);
void LoadKidSetOverlays(void);
void LoadOppOverlays(sbyte kind);
void LoadSet(int set, word id, word mapping, int modes, int prepare);
Handle LoadSwordFrames(byte kind);
int NeedKidSetOverlay(int index);
int NeedSwordOverlay(sbyte index);
void SetInitials(void);
void SetKidSwordKind(void);
void switch_background_overlay(byte current, byte previous);
void UnloadOppOverlays(sbyte previous, sbyte current);

void OpenLevelRsrcFiles(byte current, byte previous)
{
    if (current != previous) {
        if (previous) {
            if (previous == 6 || current == 6)
                UnloadSet(2);
            CloseRsrcFile(LevelRsrcFiles[previous]);
        }
        OpenRsrcVerify(LevelRsrcFiles[current]);
    }
}


void CheckBPort(void)
{
    if (BPort == 0)
        BPort = AllocBackPort(&GamePlayRect);
}


void CheckLoadedOppSet(sbyte kind)
{
    if (kind != LastOppTypeLoaded && Level.preferred_opponent != 5 &&
        Level.preferred_opponent != 6 && Char.character_kind != 11 &&
        Char.character_kind != 0 && Char.character_kind != 6)
        LoadOppSet(kind);
}
void LoadBGSet(int palette)
{
    word limit = 0x3ff0;

    if (Level.environment_kind == 5)
        limit = 0x7ff0;
    else if (Level.environment_kind == 6)
        limit = 0xfff0;

    LoadSet(4, 3500, limit, 1, 0);
    if (palette)
        SetPalette(3500, 64, 160, 0);
    LoadBGOverlays();
    BGSetMarkedPurgeable = 0;
    if (background_image_first[Level.environment_kind])
        PrinceMarkRangesPurgeable(Pics[4], background_image_first[Level.environment_kind],
                             background_image_last[Level.environment_kind]);
}

void LoadBGOverlays(void)
{
    int i;

    for (i = 0; i < 8; ++i) {
        if (Level.background_overlays[i]) {
            if (Need1stImg[i] != -1)
                OverlayShapeList(Pics[4], 3700, Need1stImg[i] - 1,
                                   NeedLastImg[i] - 1, 1);
            if (background_intro_group[i] != -1)
                LoadSounds(background_intro_group[i], background_intro_number[i]);
        }
    }
}

void LoadFlaskSet(void)
{
    if (Level.flask_flag) {
        byte *entries = _nmalloc(35);
        int i;

        memset(entries, 2, 35);
        for (i = 2; i <= 21; ++i)
            entries[i] = 1;
        LoadSet(1, 3000, 0x4000, entries, 0);
        _nfree(entries);
        if (Level.environment_kind != 6)
            SetPalette(3000, 224, 16, 0);
    } else {
        UnloadSet(1);
    }
}


void LoadLevel(sbyte number)
{
    int starts_with_nis;
    byte previous;

    UnloadLevel(3);
    previous = Level.environment_kind;
    if (number != -2 && number != -1) {
        if (LevelIdx != number) {
            if (Resume == 0)
                RemoveRebirthPt();
            LevelIdx = number;
        }
    }
    NextLevel = LevelIdx;
    CheckBPort();
    LoadLevelMap(LevelIdx);
    starts_with_nis = IsLevelStartWithNISScreen();
    if (!starts_with_nis)
        EraseRect(&ScreenPortRect);
    LoadFlaskSet();
    OpenLevelRsrcFiles(Level.environment_kind, previous);
    LoadBGSet(!starts_with_nis);
    LoadKidSet();
    LoadKidSetOverlays();
    LoadOppSet(Level.preferred_opponent);
    SetKidSwordKind();
    LoadSwordSet(KidInfo.sword_kind);
    LoadSounds(0, 26);
    LoadPiecesData();
    if (previous != Level.environment_kind) {
        switch_background_overlay(Level.environment_kind, previous);
        SetKidPalette(Level.environment_kind);
    }
    LoadCodeOverlays(Level.environment_kind, Level.preferred_opponent);
    if (number != -2 && number != -1 && Level.environment_kind == 1)
        InitDsrtGlobals();
    SetPrinceLevelSpecPtrs(Level.environment_kind);
}

void LoadLevelMap(sbyte map)
{
    int id = map + 1999;
    if (find_option(map_option))
        id += 20;
    ReadRsrc(0L, id, &Level, 12025L);
    SetInitials();
    SetRebirthPtLevelStatus();
}

void LoadLevelMinimum(sbyte number)
{
    LoadLevelMap(number);
    SetKidSwordKind();
    LoadSwordSet(KidInfo.sword_kind);
    SaveGamePlayPalette();
    Graf = &ScreenPort;
    EraseRect(&ScreenPortRect);
}


void switch_background_overlay(byte current, byte previous)
{
    if (previous != current && background_scene_group[previous] != -1)
        UnloadSounds(background_scene_group[previous], background_scene_number[previous]);
    if (background_scene_group[Level.environment_kind] != -1)
        LoadSounds(background_scene_group[Level.environment_kind], background_scene_number[Level.environment_kind]);
}

void LoadCodeOverlays(byte background_kind, byte opponent_kind)
{
    switch (background_kind) {
    case 1:
        LoadDsrtCode();
        break;
    case 2:
        TempFrameFunc();
        LoadTempleDrawCode();
        break;
    case 3:
        load_cave_drawing();
        cave_overlay_entry();
        break;
    case 4:
        LoadRuinsDrawCode();
        LoadRuinsMoveCode();
        break;
    case 5:
        LoadRoofCode();
        break;
    case 6:
        LoadFinlCode();
        break;
    }
    if (opponent_kind == 5 || opponent_kind == 6 || opponent_kind == 8)
        unused_head_entry();
    else
        unused_guard_entry();
}

void LoadPiecesData(void)
{
    UnloadPiecesData();
    Pieces = nGetRsrc(0x50494543UL, 3500);
}


void LoadSwordSet(sbyte kind)
{
    word id;
    int mode;
    sbyte i;

    if (loaded_sword != kind) {
        if (SwordFrames) {
            hFree(SwordFrames);
            SwordFrames = 0;
        }
        UnloadSet(0);
    }
    if (SwordFrames == 0)
        SwordFrames = LoadSwordFrames(kind);

    id = 1000;
    if (kind == 2)
        id = 1200;
    mode = Room.number == 1 && Level.level_number == 14 ? 0 : 1;
    LoadSet(0, id, 0x8000, 2, mode);
    if (kind == 1) {
        for (i = 0; i < 1; ++i) {
            if (NeedSwordOverlay(i)) {
                if (!IsOverlayLoaded(Pics[0], sword_overlay_first[i]))
                    OverlayShapeList(Pics[0], 1100, sword_overlay_first[i],
                                       sword_overlay_last[i], 2);
            } else {
                UnloadOverlay(Pics[0], sword_overlay_first[i], sword_overlay_last[i]);
            }
        }
    }
    loaded_sword = kind;
}

Handle LoadSwordFrames(byte kind)
{
    word id = 1000;
    if (kind == 2)
        id = 1200;
    return hGetRsrc(0x4652414dUL, id);
}

int _fastcall GetSwordFrameImg(word index)
{
    int image;

    if (index)
        image = ((ATTACHFRAME far *)h2p(SwordFrames))[index].image;
    else
        image = -1;
    return image;
}

void SetPrinceLevelSpecPtrs(byte kind)
{
    DrawFunc = LevelDrawFuncs[kind];
    DrawGateMoveFunc = LevelDrawGateMoveFuncs[kind];
}

void _fastcall clear_special_sequence_flag(int i)
{
    LEVELCELL *info = &BlueS[i];

    switch (BlueT[i]) {
    case 7:
        info->modifier &= ~0x80;
        break;
    }
}

void SetInitials(void)
{
    byte room;
    int index;
    for (room = 1; room <= Level.room_count; ++room) {
        select_room_arrays(room);
        Room.left = Level.room_links[room - 1].left;
        Room.right = Level.room_links[room - 1].right;
        for (index = 0; index < 30; ++index)
            clear_special_sequence_flag(index);
    }
}

void LoadSet(int set, word id, word mapping, int modes, int prepare)
{
    if (Pics[set] == 0)
        Pics[set] = LoadShapeList(id, mapping, modes, prepare);
}

void LoadKidSet(void)
{
    byte *entries;
    int i, j;

    if (Level.environment_kind == 6)
        OpenRsrcVerify(LevelRsrcFiles[6]);
    entries = _nmalloc(314);
    memset(entries, 0, 314);
    for (i = 131; i <= 132; ++i)
        entries[i] = 1;
    for (j = 216; j <= 218; ++j)
        entries[j] = 1;
    LoadSet(2, 25001, 2, entries, 0);
    _nfree(entries);
    PrinceMarkRangesPurgeable(Pics[2], kid_image_first, kid_image_last);
}

void LoadKidSetOverlays(void)
{
    int i;
    for (i = 0; i < 11; ++i) {
        if (NeedKidSetOverlay(i)) {
            if (!IsOverlayLoaded(Pics[2], kid_overlay_first[i]))
                OverlayShapeList(Pics[2], 24601, kid_overlay_first[i],
                                   kid_overlay_last[i], 0);
        } else {
            UnloadOverlay(Pics[2], kid_overlay_first[i], kid_overlay_last[i]);
        }
    }
}

int NeedKidSetOverlay(int index)
{
    int needed;
    switch (index) {
    default:
        needed = 0;
        break;
    case 0:
        needed = Level.background_overlays[0];
        break;
    case 1:
        needed = Level.environment_kind == 4 || Level.environment_kind == 2;
        break;
    case 2:
        needed = Level.environment_kind == 3 || Level.preferred_opponent == 5 || Level.preferred_opponent == 6;
        break;
    case 3:
        needed = Level.background_overlays[3];
        break;
    case 5:
        needed = Level.preferred_opponent == 5 || Level.preferred_opponent == 6;
        break;
    case 6:
        needed = Level.level_number == 5;
        break;
    case 8:
        needed = Level.level_number == 13;
        break;
    case 9:
        needed = Level.level_number == 13 || Level.environment_kind == 6;
        break;
    case 4:
    case 7:
    case 10:
        needed = Level.environment_kind == 6;
        break;
    }
    return needed;
}

void SetKidPalette(sbyte kind)
{
    SetPalette(25001, 16, 16, (byte)(kind - 1));
}

int NeedOppOverlay(sbyte kind, int index)
{
    int needed = 0;
    if (kind != -1 && Opp1stRange[kind]) {
        int count;
        for (count = -1; Opp1stRange[kind][count] != -1; ++count)
            ;
        if (count >= index) {
            if (index < 3)
                needed = Level.sword_overlays[index];
            else if (kind == 7 || Level.environment_kind == 6)
                needed = 1;
        }
    }
    return needed;
}

int NeedSwordOverlay(sbyte index)
{
    int needed; /* The original sets this local only on the success path. */
    switch (index) {
    case 0:
        if (Level.sword_overlays[0])
            needed = 1;
        break;
    }
    return needed;
}

void LoadOppSet(sbyte kind)
{
    if (kind == -1 || kind != LastOppTypeLoaded || Pics[3] == 0) {
        UnloadSet(3);
        if (hOppFrames) {
            hFree(hOppFrames);
            hOppFrames = 0;
        }
        if (hHeadChompFrame) {
            hFree(hHeadChompFrame);
            hHeadChompFrame = 0;
        }
        if (LastOppTypeLoaded != -1 && kind != LastOppTypeLoaded)
            CloseRsrcFile(opponent_resource_files[LastOppTypeLoaded]);
    }
    if (LastOppTypeLoaded != -1 && Opp1stRange[LastOppTypeLoaded])
        UnloadOppOverlays(LastOppTypeLoaded, kind);

    if (kind != -1) {
        if (kind == LastOppTypeLoaded && Pics[3]) {
            if (kind == LastOppTypeLoaded)
                LoadOppOverlays(kind);
        } else {
            OpenRsrcVerify(opponent_resource_files[kind]);
            LoadSet(3, 750, 4, 2, 0);
            SetPalette(750, 32, 16, 0);
            if (Opp1stRange[kind]) {
                LoadOppOverlays(kind);
                PrinceMarkRangesPurgeable(Pics[3], Opp1stRange[kind],
                                     opponent_overlay_last[kind]);
            }
            change_song_range(kind);
            hOppFrames = hGetRsrc(0x4652414dUL, 750);
            if ((kind == 5 || kind == 6) && hHeadChompFrame == 0)
                hHeadChompFrame = hGetRsrc(0L, 755);
        }
    }
    LastOppTypeLoaded = kind;
}


void LoadOppOverlays(sbyte kind)
{
    int *last_base = opponent_overlay_last[kind];
    int *first_base = Opp1stRange[kind];
    if (first_base && kind != 5 && kind != 6) {
        int i = 0;
        if (*first_base == -1)
            return;
        do {
            if (NeedOppOverlay(kind, i)) {
                if (!IsOverlayLoaded(Pics[3], first_base[i]))
                    OverlayShapeList(Pics[3], 850, first_base[i], last_base[i], 2);
                if (i == 0 && (kind == 0 || kind == 5 || kind == 6)) {
                    if (!OppBank[0])
                        PresetOppBanks(1);
                    if (!OppBank[1])
                        PresetOppBanks(2);
                }
            } else {
                UnloadOverlay(Pics[3], first_base[i], last_base[i]);
            }
            ++i;
        } while (first_base[i] != -1);
    }
}


void PreloadRoomData(byte room)
{
    int visited[64];
    sbyte tile;

    memset(visited, 0, sizeof(visited));
    for (tile = 0; tile < 30; ++tile) {
        const byte *object_code = &Level.cell_kinds[room - 1][tile];
        const LEVELCELL *cell = &Level.cell_info[room - 1][tile];
        int object = *object_code;
        word info = cell->modifier;
        if (visited[*object_code] == 0) {
            visited[object] = 1;
            switch (object) {
            case 2:
                LoadSounds(background_intro_group[0], background_intro_number[0]);
                break;
            case 5:
            case 6:
            case 34:
                LoadSounds(3, 3);
                break;
            case 10:
                if ((info & 0xe0) == 0xc0) {
                    if (LastOppTypeLoaded != 9)
                        LoadOppSet(9);
                } else {
                    visited[object] = 0;
                }
                /* fall through */
            case 4:
                if (Level.environment_kind == 3)
                    LoadSounds(7, 9);
                else
                    LoadSounds(4, 6);
                break;
            case 11:
                LoadSounds(20, 20);
                LoadSounds(2, 2);
                break;
            case 13:
                LoadSounds(background_intro_group[3], background_intro_number[3]);
                break;
            case 23:
                LoadSounds(background_intro_group[1], background_intro_number[1]);
                break;
            case 25:
                LoadSounds(background_intro_group[5], background_intro_number[5]);
                break;
            case 26:
                LoadSounds(background_intro_group[6], background_intro_number[6]);
                break;
            case 36:
                LoadSounds(background_intro_group[4], background_intro_number[4]);
                break;
            }
        }
    }
    if (Level.room_objects[room - 1].opponent_count) {
        change_song_range(Level.preferred_opponent);
        LoadSounds(12, 14);
    }
    if (Kid.vertical_step > 0) {
        if (Kid.vertical_step < 33)
            hGetSnd(10016);
        else
            hGetSnd(10000);
        if (Kid.vertical_step < 31)
            hGetSnd(10001);
    }
}

int load_resource_error(Handle resource)
{
    char path[120];
    char drive[4], directory[256], basename[256], extension[256];

    hGetState(resource);
    _fstrcpy(path, (char far *)hLock(resource) + 16);
    _splitpath(path, drive, directory, basename, extension);
    sprintf(path, map_option + sizeof("GAMEPLAY"), basename, extension);
    report_resource_path(path);
    return 1;
}

void report_resource_path(const char far *path)
{
    char message[100];

    sprintf(message, resource_path_format, path);
    abort_with_message(1, message);
}


void SetKidSwordKind(void)
{
    if (hRebirthSaved)
        KidInfo.sword_kind = hRebirthSaved->sword_kind;
    else if (Level.level_number == 6)
        KidInfo.sword_kind = -1;
    else if (Level.level_number == 7 || Level.level_number == 8)
        KidInfo.sword_kind = 2;
    else
        KidInfo.sword_kind = 1;
}

void UnloadFrom(int first)
{
    int i;
    ZeroPeels();
    for (i = first; i < 5; ++i)
        UnloadSet(i);
    if (first <= 4 && CustomRoom)
        UnloadCustomRoomData();
}

void UnloadLevel(int first)
{
    int i;
    UnloadFrom(first);
    for (i = 0; i < 8; ++i) {
        if (Level.background_overlays[i])
            UnloadSounds(background_intro_group[i], background_intro_number[i]);
    }
    if (first <= 3 && hOppFrames) {
        if (hOppFrames) {
            hFree(hOppFrames);
            hOppFrames = 0;
        }
        if (hHeadChompFrame) {
            release_tagged_resource(0L, 755);
            hHeadChompFrame = 0;
        }
        if (LastOppTypeLoaded != -1) {
            CloseRsrcFile(opponent_resource_files[LastOppTypeLoaded]);
            LastOppTypeLoaded = -1;
        }
    }
    if (first <= 0 && SwordFrames) {
        hFree(SwordFrames);
        SwordFrames = 0;
        loaded_sword = 0;
    }
    UnloadPiecesData();
    UnloadCustomRoomData();
}

void UnloadOppOverlays(sbyte previous, sbyte current)
{
    if (Pics[3] && Level.preferred_opponent != 5 && Level.preferred_opponent != 6) {
        int *last_base = opponent_overlay_last[previous];
        int *first_base = Opp1stRange[previous];
        if (first_base) {
            int i = 0;
            if (*first_base == -1)
                return;
            do {
                if (current != previous || !NeedOppOverlay(current, i))
                    UnloadOverlay(Pics[3], first_base[i], last_base[i]);
                ++i;
            } while (first_base[i] != -1);
        }
    }
}

void UnloadPiecesData(void)
{
    if (Pieces) {
        _nfree(Pieces);
        Pieces = 0;
    }
}

void UnloadSet(int set)
{
    if (Pics[set]) {
        UnloadShapeList(Pics[set]);
        Pics[set] = 0;
    }
}
