/* DOS DESERT.C, overlay 5. Definitions follow DOS code order. */
#include "types.h"
#include "level.h"
#include "char.h"
#include "game.h"

#pragma pack(1)

typedef struct {
    sbyte shape;
    int x, y;
    sbyte pass;
    byte reserved[3];
    word handle;
    Rect rect;
    byte tail[6];
} Piece;

/* Animated pieces use the high byte of y and the pass byte as a state word. */
typedef struct {
    byte reserved[4];
    word flags;
} PackedPieceState;

#define piece_state(piece) (((PackedPieceState *)(piece))->flags)

typedef struct {
    sbyte count;
    byte template_id;
    int shape_base;
    byte header[24];
    Piece pieces[1];
} DesertRoom;

typedef struct {
    byte reserved[16];
    Rect bounds;
} PEELPORT;

typedef struct {
    PEELPORT *peel;
    int reserved;
    int active;
} PeelRecord;
#pragma pack()

extern struct { unsigned char channels, volume; } Midi;
extern FCHARREC FChar;

static sbyte current_column, previous_column;
extern CHARREC Char;
extern LEVELREC Level;
extern word BlueSpec;

extern GAMESTATE GameState;
extern unsigned long FrameNum;
extern ROOMREC Room;
extern AmbientState ambient_state;
extern CHARREC Kid;
extern int NextLevel, CueSnd;

extern Directions Dir;
static Rect stone_bounds = {49, 11, 70, 54};
static sbyte desert_variant = -1;
extern DesertRoom *CustomRoom;
extern word *Pics[5];
extern Rect CellClipRect, GamePlayRect;
extern int ImWidth;
extern TROBREC Trob;
extern struct {
    int count;
    PeelRecord entries[40];
} PeelInfo;

extern void AddSong(int), AddSound(int);
extern void CycleColors(int, int, int);
extern int SndOn(int);
extern void ResetSnd(int);
extern int GetTrobId(sbyte, sbyte);
extern void LoadKid(void), ResetCharY(void);
extern void JumpSeq(int);
extern int Rnd(int);
extern void far *hLock(word);
extern void hUnlock(word), hFree(word);
extern word HGETRSRC(unsigned long, int);
extern word PrinceUnpackShape(word, int, void far *);
extern void far *h2p(word);
extern Rect far *MAKESHAPERECT(Rect far *, int, int, void far *);
extern void MarkMove(sbyte, Rect *);
extern void MarkBufsForRect(void (far *)(sbyte, Rect *), Rect *, sbyte);
extern void far _fastcall DrawCustomRoomImg(sbyte, sbyte);
extern int GetTrobRoomCell(sbyte, int);
extern void AddTrob(sbyte, sbyte, sbyte, sbyte);
extern int CheckTrobVis(void);
extern sbyte RestoreKeepCustomPeel(byte);
extern void DrawCustomRoomImgAtInfo(byte, Piece *);
extern void OFFSETRECT(Rect far *, Rect far *, int, int);
extern void far _fastcall AddPeel(int, int, int, int, byte, byte);
extern int ComputeTrobMoveRect(Rect *, Rect *);
extern sbyte CheckTrobRight(void), CheckTrobBelow(void);
extern int GetPeelIdxForCharIdx(byte, int);

void DsrtCheckExit(void);
void DsrtCycleColors(void);
void DsrtWatchKid(void);
int IsMarkStonePos(int);
word IsDsrtExitOpen(void);
void TriggerDsrtExit(sbyte, sbyte);
void AnimDsrtStone(void);
void RedrawStone(void);
void RemoveLastStonePeel(sbyte);
int SinkDsrtStone(sbyte);
int TriggerStone(byte, sbyte, sbyte);

void AddDsrtAmbient(void)
{
    if (Midi.channels & 2)
        AddSong(251);
}

void DsrtClipChar(void)
{
    FChar.clip.bottom = FChar.clip.bottom > 126 ? 126 : FChar.clip.bottom;
    if (FChar.clip.bottom < FChar.clip.top)
        FChar.clip.top = FChar.clip.bottom;
}

void DsrtCheckExit(void)
{
    word flags = Level.cell_info[0][10].modifier;
    word frame;
    int keep_open;
    sbyte cell;

    if ((flags & 31) != 0)
        return;
    frame = (flags >> 8) & 31;
    keep_open = current_column - previous_column != -2; cell = 0;
    if (keep_open) {
        while (cell < 6) {
            int state = Level.cell_info[0][cell + 12].modifier & 15;
            if ((state && current_column != cell) ||
                (!state && current_column == cell && frame < 10))
                keep_open = 0;
            ++cell;
            if (!keep_open)
                break;
        }
    }

    if (keep_open) {
        if (frame >= 20) {
            TriggerDsrtExit(1, 10);
        } else {
            if (frame == 6)
                AddSong(37);
            else if (frame == 9)
                AddSound(45);
            else if (frame == 10)
                SinkDsrtStone(current_column + 12);
            ++frame;
            flags &= 0xe0ff;
            frame <<= 8;
            flags |= frame;
        }
    } else {
        flags &= 0xe0ff;
    }
    Level.cell_info[0][10].modifier = flags;
}

void DsrtCycleColors(void)
{
    if (!GameState.fade_active && Room.number != 1 && FrameNum % 3 == 0) {
        CycleColors(176, 13, 0);
        CycleColors(196, 4, 0);
        CycleColors(191, 5, 0);
    }
}

void DsrtFrameFunc(void)
{
    DsrtCycleColors();
    DsrtWatchKid();
    DsrtCheckExit();
    if (Room.number != 1) {
        if (!SndOn(10038))
            AddSound(38);
        if (SndOn(10251))
            ResetSnd(10251);
    } else if (SndOn(10251)) {
        if (SndOn(10038))
            ResetSnd(10038);
    } else if (Kid.sequence_group != 7 && Kid.alive < 0 &&
               !SndOn(ambient_state.song + 10000) &&
               !GetTrobId(30, 1) && !IsDsrtExitOpen()) {
        AddDsrtAmbient();
    }
}

void DsrtWatchKid(void)
{
    LoadKid();
    if (Char.room == 1) {
        if (Char.column != previous_column) {
            if (IsMarkStonePos(Char.frame)) {
                if (previous_column >= 2) {
                    if (previous_column <= 7) {
                        if (SinkDsrtStone(previous_column + 10) &&
                            current_column - previous_column == -2)
                            AddSong(41);
                    }
                }
            }
        }
        if (IsMarkStonePos(Char.frame))
            previous_column = Char.column;
        if (Char.x < 130) {
            if (Level.level_number == NextLevel) {
                ++NextLevel;
                JumpSeq(2);
            } else {
                JumpSeq(2);
                CueSnd = -1;
            }
        }
    } else if (Char.room == 3) {
        if (Char.x >= 489) {
            if (Dir.up || Dir.down || (!Char.direction && Dir.forward)) {
                JumpSeq(2);
                ResetCharY();
            }
            if (!SndOn(10046) && (Dir.action < 0 || !Rnd(20)))
                AddSound(46);
        }
    }
    Kid = Char;
}

void InitDsrtGlobals(void)
{
    desert_variant = -1;
}

int IsMarkStonePos(int code)
{
    int selected;

    if (code == 15 || code == 109 || code == 7 || code == 46 ||
        (code >= 11 && code <= 14) || code == 38 || code == 44 || code == 26 ||
        code == 158)
        selected = 1;
    else if (code == 170 || code == 171)
        selected = 1;
    else
        selected = 0;
    return selected;
}

void LoadDsrtCode(void)
{
    Piece *piece;
    void far *archive;
    int resource;

    previous_column = -1;
    if (CustomRoom != 0) {
        if (CustomRoom->template_id == 6) {
            if (desert_variant == -1) {
                desert_variant = Rnd(2);
                current_column = Rnd(4) + 1;
            }
            if (desert_variant != 2) {
                archive = hLock(*Pics[4]);
                piece = &CustomRoom->pieces[15];
                hFree(piece->handle);
                resource = desert_variant + CustomRoom->shape_base + 15;
                piece->handle = HGETRSRC(0x53484150UL, resource);
                piece->handle = PrinceUnpackShape(piece->handle, 2, archive);
                piece = &CustomRoom->pieces[16];
                hFree(piece->handle);
                piece->handle = HGETRSRC(0x53484150UL, resource + 3);
                piece->handle = PrinceUnpackShape(piece->handle, 2, archive);
                hUnlock(*Pics[4]);
            }
            }
    }
}

void SetupSand(void)
{
    Rect saved_clip;
    byte index;
    Piece *piece;
    Rect *bounds;

    if (Char.sequence == 27) {
        saved_clip = CellClipRect;
        index = (byte)(FrameNum & 1) + 3;
        piece = &CustomRoom->pieces[index];
        piece->x = 119;
        piece->y = FChar.x;
        if (!(FChar.flipped))
            piece->y -= ImWidth;
        bounds = &piece->rect;
        MAKESHAPERECT(bounds, piece->x, piece->y, h2p(piece->handle));
        MarkBufsForRect(MarkMove, bounds, -1);
        CellClipRect = *bounds;
        DrawCustomRoomImg(index, piece->pass = 1);
        piece->pass = 11;
        CellClipRect = saved_clip;
    }
}

void AnimDsrtExit(void)
{
    word frame;
    Piece *piece;
    Rect *bounds;

    frame = BlueSpec & 31;
    if (frame == 20) {
        Trob.state = -1;
        ResetSnd(10007);
    } else {
        ++frame;
        if (!SndOn(10045))
            AddSound(7);
    }
    BlueSpec &= ~31;
    BlueSpec |= frame;
    piece = &CustomRoom->pieces[2];
    piece->rect.left -= 2;
    bounds = &piece->rect;
    MarkMove(Trob.cell, bounds);
    MarkMove(Trob.cell + 1, bounds);
    MarkMove(Trob.cell - 10, bounds);
    MarkMove(Trob.cell - 9, bounds);
    RestoreKeepCustomPeel(piece->shape);
}

void DrawDsrtExit(Piece *piece)
{
    int frame;

    if (piece->shape == 11) {
        frame = piece_state(piece) & 31;
        CustomRoom->pieces[2].y = (4 - frame) * 2;
        CustomRoom->pieces[15].y = 27 - frame * 2;
        CustomRoom->pieces[15].rect.left = 0;
        DrawCustomRoomImg(2, 11);
        DrawCustomRoomImg(15, 11);
    }
}

word IsDsrtExitOpen(void)
{
    return Level.cell_info[0][10].modifier & 31;
}

void TriggerDsrtExit(sbyte room, sbyte cell)
{
    if (!GetTrobRoomCell(room, cell)) {
        AddTrob(room, cell, 1, 4);
        AddSong(39);
    }
}

void AnimDsrtPlanks(void)
{
    word frame;

    if (CheckTrobVis()) {
        frame = BlueSpec & 15;
        if (frame == 7)
            frame = 0;
        else
            ++frame;
        BlueSpec &= ~15;
        BlueSpec |= frame;
        MarkMove(Trob.cell, &CustomRoom->pieces[1].rect);
    }
}

void DrawDsrtPlanks(Piece *piece)
{
    Rect saved_clip;
    sbyte index;

    if (piece->shape == 11) {
        index = ((piece_state(piece) >> 1) & 7) + 1;
        saved_clip = CellClipRect;
        CellClipRect = GamePlayRect;
        DrawCustomRoomImg(index, 11);
        CellClipRect = saved_clip;
    }
}

void AnimDsrtRocks(void)
{
    word frame;

    if (CheckTrobVis()) {
        frame = BlueSpec & 15;
        if (frame == 7)
            frame = 0;
        else
            ++frame;
        BlueSpec &= ~15;
        BlueSpec |= frame;
        MarkMove(Trob.cell, &CustomRoom->pieces[1].rect);
    }
}

void DrawDsrtRocks(Piece *piece)
{
    Rect saved_clip;
    sbyte index;

    if (piece->shape == 11) {
        index = ((piece_state(piece) >> 1) & 7) + 1;
        saved_clip = CellClipRect;
        CellClipRect = GamePlayRect;
        DrawCustomRoomImg(index, 11);
        CellClipRect = saved_clip;
    }
}

void AddDsrtStoneUpPeels(Piece *piece)
{
    Rect bounds;
    int x, y, count;
    byte id;

    id = piece->shape + 100;
    y = 126;
    x = 64;
    count = 6;
    do {
        OFFSETRECT(&bounds, &piece->rect, x, y);
        AddPeel(bounds.left, bounds.right, bounds.top,
                 bounds.bottom - bounds.top, id, 3);
        x += 32;
    } while (--count);
}

void AnimDsrtStone(void)
{
    word delay = (BlueSpec & 0x1f0) >> 4;
    word frame;

    if (delay == 0) {
        frame = BlueSpec & 15;
        if (Trob.state == 3 && frame == 0)
            AddSound(42);

        if ((frame == 10 && Trob.state == 3) ||
            (frame == 0 && Trob.state == 1)) {
            if (Trob.state == 3 && !GetTrobId(30, 2))
                AddDsrtAmbient();
            Trob.state = -1;
        } else {
            BlueSpec &= ~15;
            BlueSpec |= frame + Trob.state - 2;
        }

        if (Trob.state > 0)
            RedrawStone();
    } else if (Kid.sequence != 27 && Kid.alive < 0) {
        BlueSpec &= ~0x1f0;
        BlueSpec |= --delay << 4;
    }
}

void DrawDsrtStone(Piece *piece)
{
    word frame;

    if (piece->shape == 11 || piece->shape == 2) {
        frame = piece_state(piece) & 15;
        if (frame) {
            DrawCustomRoomImgAtInfo(frame + 4, piece);
            if (frame > 7 && (sbyte)piece->x - current_column == 2)
                DrawCustomRoomImgAtInfo(16, piece);
        }
    }
}

void RedrawStone(void)
{
    Rect bounds;

    if (ComputeTrobMoveRect(&bounds, &stone_bounds)) {
        MarkMove(Trob.cell, &bounds);
        MarkMove(CheckTrobRight(), &bounds);
        MarkMove(CheckTrobBelow(), &bounds);
    }
}

void RemoveLastStonePeel(sbyte column)
{
    int x = (column + 1) << 5;
    sbyte matches = 0;
    int occurrence = 1;
    int slot;
    Rect far *bounds;

    do {
        slot = GetPeelIdxForCharIdx(114, occurrence);
        if (slot != -1) {
            bounds = &PeelInfo.entries[slot].peel->bounds;
            if (bounds->left == x || bounds->right == x) {
                PeelInfo.entries[slot].active = 0;
                if (++matches == 2)
                    slot = -1;
            } else if (bounds->left < x && bounds->right > x &&
                       bounds->top == 114 && bounds->bottom == 127) {
                PeelInfo.entries[slot].active = 0;
                slot = -1;
            }
        }
        ++occurrence;
    } while (slot + 1);
}

int SinkDsrtStone(sbyte cell)
{
    int started;
    word flags;

    started = TriggerStone(1, cell, 1);
    if (started) {
        flags = BlueSpec;
        BlueSpec = Level.cell_info[0][cell].modifier;
        AnimDsrtStone();
        Level.cell_info[0][cell].modifier = BlueSpec;
        BlueSpec = flags;
    }
    return started;
}

void TriggerAllStonesDown(void)
{
    if (previous_column != -1 && previous_column >= 2 && previous_column <= 7)
        SinkDsrtStone(previous_column + 10);
    SinkDsrtStone(Char.column + 10);
}

int TriggerStone(byte room, sbyte cell, sbyte action)
{
    word original = Level.cell_info[room - 1][cell].modifier;
    int frame = original & 15;
    int started = 1;

    if (action == 3 && frame == 0) {
        original &= ~0x1f0;
        original |= (Rnd(10) + 21) << 4;
        Level.cell_info[room - 1][cell].modifier = original;
    } else if (action == 1) {
        if (frame) {
            AddSound(43);
        } else {
            started = GetTrobRoomCell(room, cell) != 0;
            if (original & 0x1f0) {
                original &= ~0x1f0;
                Level.cell_info[room - 1][cell].modifier = original;
            }
        }
    } else {
        started = 0;
        if (action == 3 && (sbyte)(cell % 10) == 7 && !GetTrobId(30, 1))
            AddDsrtAmbient();
    }

    if (started) {
        AddTrob(room, cell, action, 30);
        if (action == 1)
            RemoveLastStonePeel((sbyte)(cell % 10));
    }
    return started;
}

typedef void (far pascal *LevelTileProc)(Piece near *);
LevelTileProc DrawDsrtFunc[] = {
    0, 0, 0, 0, DrawDsrtExit,
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, DrawDsrtRocks, DrawDsrtPlanks,
    DrawDsrtStone
};
