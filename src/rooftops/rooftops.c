/* ROOFTOPS.C: DOS overlay 4. Descriptive names mark unresolved symbols. */
#include "types.h"
#include "level.h"
#include "char.h"
#include "game.h"

#pragma pack(1)

typedef struct {
    sbyte kind;
    int y, x;
    byte pass;
    byte reserved[3];
    int image_set;
    Rect bounds;
    byte tail[6];
} DrawPiece;

typedef struct {
    sbyte piece_count;
    byte bank;
    byte reserved[26];
    DrawPiece pieces[80];
} DrawList;

typedef struct {
    sbyte kind;
    sbyte column, row;
    byte reserved;
    word frame;
} RoofAnimation;

typedef struct {
    byte reserved[18];
    int top;
    int left;
    int bottom;
} PEELPORT;

typedef struct {
    PEELPORT near *peel;
    byte reserved[4];
} PeelSlot;

typedef struct {
    byte index;
    byte reserved;
} SplashImage;
#pragma pack()

typedef struct {
    signed char character, frame;
    int x;
    unsigned char low;
} WATERSTATE;
WATERSTATE InWater;
static sbyte palette_bank;

extern LEVELREC Level;

extern CHARREC Char;
extern ROOMREC Room;

extern TROBREC Trob;

extern GAMESTATE GameState;
extern unsigned long FrameNum;
extern word BlueSpec;

extern FCHARREC FChar;
extern int NewRoom;
extern int ImWidth;
extern int Timer[4];
extern struct { unsigned char channels, volume; } Midi;
extern CHARREC Kid;
extern Rect CellClipRect, NullRect, GamePlayRect;
static SplashImage splash_images[] = {
    {29, 0}, {30, 0}, {31, 0},
    {29, 0}, {32, 0}, {33, 0}, {29, 0}
};
extern struct {
    int count;
    PeelSlot entries[40];
} PeelInfo;

extern DrawList near *CustomRoom;
typedef void (far pascal *RedrawCallback)(void);
extern void far pascal MarkMove(void), MarkFRedraw(void);
extern int far pascal SECTRECT(Rect far *, Rect far *, Rect far *);
extern void far pascal MarkBufsForRect(RedrawCallback, Rect near *, sbyte);
extern void far pascal DecLifePts(sbyte), SetKidDeathMethod(sbyte);
extern int far pascal SetPalette(word, int, byte, sbyte);
extern void far pascal CycleColors(int, int, int);
extern int far pascal SndOn(int);
extern void far pascal AddSound(int), AddLeaveLevelSong(void), play_song(int);
extern void far pascal WaitSnd(int), WaitTimer(int), ChangeMeters(void);
extern void far pascal CheckMeters(void), LoadKid(void);
extern void far pascal ResetCharY(void), JumpSeq(int), flag_sound_resource(int);
extern void far pascal LoadShad(sbyte), SaveShad(void);
extern int far pascal CheckTrobVis(void);
extern void far *far pascal h2p(int);
extern Rect far *far pascal MAKESHAPERECT(Rect far *, int, int, void far *);
extern void far _fastcall DrawCustomRoomImg(byte, byte);
extern sbyte far pascal RestoreKeepCustomPeel(byte);

void far pascal RoofCheckCharInWater(void);
void far pascal RoofCycleColors(void);
void far pascal RoofWatchKid(void), RoofWatchOpps(void);
void far pascal RoofTriggerCharInWater(void);
Rect near *far pascal RoofMakeWaterTestRect(Rect near *);
int far pascal DrawPillarInBack(DrawPiece near *);
int far pascal RoofClimbingIntoShip(void);
int far pascal RoofHangingFromShip(void);
void far pascal UpdateShipPeel(sbyte, int);

int far _fastcall DrawRoofLedgeInBack(DrawPiece near *piece)
{
    Rect intersection;
    int result;

    if ((piece->kind == 35 || piece->kind == 22 ||
         piece->kind == 72 || piece->kind == 23) &&
        SECTRECT(&intersection, &piece->bounds, (Rect far *)&Kid.bounds) &&
        Kid.sequence != 68 &&
        (Kid.action == 3 || Kid.action == 4))
        result = 1;
    else
        result = 0;
    return result;
}

void far pascal LoadRoofCode(void)
{
    sbyte inactive = -1;

    InWater.frame = 0;
    palette_bank = inactive;
    InWater.character = inactive;
}

void far pascal RoofCheckCharInWater(void)
{
    if (InWater.character == -1) {
        if (Char.y >= 172 && Char.sequence != 68 && Char.sequence != 15 &&
            Char.sequence != 59 && Char.action != 2)
            RoofTriggerCharInWater();
    } else if (Char.slot == InWater.character) {
        if (InWater.frame < 7) {
            if (CustomRoom->bank != 18 && CustomRoom->bank != 19)
                return;
            MarkBufsForRect(MarkMove, &CustomRoom->pieces[splash_images[InWater.frame - 1].index].bounds, -1);
        } else if (InWater.frame >= 8) {
            InWater.frame = 0;
            InWater.character = -1;
            if (Char.row >= 2) {
                Char.frame = 185;
                DecLifePts(100);
                SetKidDeathMethod(15);
            }
            return;
        }
        ++InWater.frame;
    }
}

void far pascal RoofCheckPalette(sbyte room)
{
    sbyte bank = -1;

    if (Level.environment_kind == 5 && room != 19 && room != 16 && room != 15) {
        if (palette_bank != 0)
            bank = palette_bank = 0;
    } else if (palette_bank != 1) {
        bank = palette_bank = 1;
    }
    if (bank != -1)
        SetPalette(3500, 64, 176, bank);
}

void far pascal RoofCheckRoomSpecific(void)
{
    RoofCheckPalette(Room.number);
}

void far pascal RoofClipChar(void)
{
    int limit;

    if ((Char.room == 19 || Char.room == 16) && Char.slot == InWater.character) {
        limit = 172;
        if (Char.action == 9) limit = 186;
        FChar.clip.bottom = limit > FChar.clip.bottom ? FChar.clip.bottom : limit;
        if (FChar.clip.bottom < FChar.clip.top)
            FChar.clip.top = FChar.clip.bottom;
    }
}

void far pascal RoofCycleColors(void)
{
    if (!GameState.fade_active && (Room.number == 19 || Room.number == 16 || Room.number == 15) &&
        FrameNum % 3 == 0) {
        CycleColors(230, 3, 0);
        CycleColors(233, 3, 0);
        CycleColors(236, 2, 0);
    }
}

void far pascal RoofFrameFunc(void)
{
    RoofCycleColors();
    RoofWatchKid();
    RoofWatchOpps();
}

void far pascal RoofKillKidOnWharf(void)
{
    int hp;

    if (Char.x > 513) {
        if ((Midi.channels & 1) && !SndOn(10032) && !SndOn(10047)) {
            AddSound(32);
            return;
        }
        WaitSnd(10032);
        Timer[0] = 30;
        WaitTimer(0);
        play_song(47);
        hp = Char.life_points;
        if (hp > 0) {
            do {
                Timer[0] = 48;
                DecLifePts(1);
                Kid = Char;
                ChangeMeters();
                CheckMeters();
                LoadKid();
                WaitTimer(0);
            } while (--hp);
        }
        ResetCharY();
        JumpSeq(71);
        flag_sound_resource(10047);
        WaitSnd(10047);
    }
}

Rect near *far pascal RoofMakeWaterTestRect(Rect near *rect)
{
    if (InWater.frame && InWater.frame <= 7) {
        if (InWater.low == 1)
            rect->top = 186;
        else
            rect->top = 172;
        rect->left = InWater.x - 16;
        rect->bottom = rect->top + 1;
        rect->right = InWater.x + 16;
    } else {
        *rect = NullRect;
    }
    return rect;
}

void far pascal RoofTriggerCharInWater(void)
{
    if (!InWater.frame) {
        InWater.character = Char.slot;
        InWater.frame = 1;
        InWater.low = (Char.action == 9) ? 1 : 0;
        InWater.x = Char.x - 130;
        if (Char.direction == 0)
            InWater.x -= ImWidth / 2;
        else
            InWater.x += ImWidth / 2;
        AddSound(48);
    }
}

void far pascal RoofWatchKid(void)
{
    if (Kid.room == 19 || Kid.room == 16 || InWater.character == 10) {
        LoadKid();
        RoofCheckCharInWater();
        if (Kid.room == 19 && (RoofHangingFromShip() || RoofClimbingIntoShip())) {
            --Char.x;
            if (RoofClimbingIntoShip() && Char.frame == 149)
                AddLeaveLevelSong();
        }
        Kid = Char;
    }
}

void far pascal RoofWatchOpps(void)
{
    sbyte count = Level.room_objects[Room.number - 1].opponent_count;
    sbyte index;

    if (count) {
        index = 0;
        if (count > 0) {
            do {
                LoadShad(index);
                if (Char.room == 19 || Char.room == 16 || index == InWater.character) {
                    RoofCheckCharInWater();
                    SaveShad();
                }
            } while (++index < count);
        }
    }
}

void far pascal SetupSplash(void)
{
    Rect clip;
    DrawPiece near *piece;
    byte image;

    if (Char.slot == InWater.character && InWater.frame != 0 && InWater.frame < 7 &&
        (CustomRoom->bank == 18 || CustomRoom->bank == 19)) {
        clip = CellClipRect;
        image = splash_images[InWater.frame - 1].index;
        piece = &CustomRoom->pieces[image];
        piece->y = piece->bounds.top - piece->bounds.bottom + 176;
        if (Char.action == 9)
            piece->y += 14;
        piece->x = (piece->bounds.left - piece->bounds.right) / 2 + InWater.x;
        MAKESHAPERECT(&piece->bounds, piece->y, piece->x, h2p(piece->image_set));
        CellClipRect = piece->bounds;
        piece->pass = 1;
        DrawCustomRoomImg(image, piece->pass);
        piece->pass = 11;
        if (Char.action != 9)
            MarkBufsForRect(MarkFRedraw, &piece->bounds, -1);
        CellClipRect = clip;
    }
}

void far pascal AnimRoofGlass(void)
{
    if (BlueSpec >= 11) {
        Trob.state = -1;
        return;
    }
    ++BlueSpec;
    if (BlueSpec == 1)
        AddSound(49);
    MarkBufsForRect(MarkMove, &CustomRoom->pieces[(byte)(BlueSpec + 30)].bounds, -1);
}

void far pascal DrawRoofGlass(RoofAnimation near *animation)
{
    Rect clip;
    byte image;

    if (animation->kind == 11 && animation->frame < 11) {
        clip = CellClipRect;
        CellClipRect = GamePlayRect;
        image = (byte)(animation->frame + 31);
        DrawCustomRoomImg(image, 11);
        if (image + 1 < CustomRoom->piece_count)
            MarkBufsForRect(MarkFRedraw, &CustomRoom->pieces[image + 1].bounds, -1);
        CellClipRect = clip;
    }
}

extern void far pascal OFFSETRECT(Rect far *, Rect far *, int, int);
extern DrawPiece near *far pascal CustomPieceOffsetToColRow(DrawPiece near *, sbyte, sbyte);
extern void far pascal DrawCustomRoomImgAtInfo(byte, signed char near *);
DrawPiece near *far pascal RoofOffsetPillarWaterToFore(DrawPiece near *);

void far pascal AnimRoofPillar(void)
{
    DrawPiece near *piece;
    long position;
    Rect water, bounds;
    unsigned int step;

    if (CheckTrobVis()) {
        step = BlueSpec & 15;
        if (step == 7)
            step = 0;
        else
            ++step;
        BlueSpec &= 0xfff0;
        BlueSpec |= step;
        RoofMakeWaterTestRect(&water);
        piece = &CustomRoom->pieces[25];
        bounds = piece->bounds;
        position = *(long *)&piece->y;
        piece = CustomPieceOffsetToColRow(piece, Trob.cell % 10, Trob.cell / 10);
        if (!InWater.frame || !SECTRECT(&water, &water, &piece->bounds)) {
            MarkBufsForRect(MarkMove, &piece->bounds, -1);
            RoofOffsetPillarWaterToFore(piece);
            MarkBufsForRect(MarkMove, &piece->bounds, -1);
        }
        *(long *)&piece->y = position;
        piece->bounds = bounds;
    }
}

void far pascal DrawRoofPillar(RoofAnimation near *animation)
{
    DrawPiece near *piece;
    struct {
        long position;
        sbyte kind;
        byte image;
    } saved;
    Rect bounds;

    if (animation->kind == 11 || (animation->kind == 0 && NewRoom)) {
        saved.kind = animation->kind;
        animation->kind = 0;
        saved.image = ((animation->frame >> 1) & 7) + 25;
        DrawCustomRoomImgAtInfo(saved.image, (signed char *)animation);
        piece = &CustomRoom->pieces[saved.image];
        bounds = piece->bounds;
        saved.position = *(long *)&piece->y;
        piece = CustomPieceOffsetToColRow(piece, animation->column, animation->row);
        if (!DrawPillarInBack(piece))
            animation->kind = 1;
        *(long *)&piece->y = saved.position;
        piece->bounds = bounds;
        RoofOffsetPillarWaterToFore(piece);
        DrawCustomRoomImgAtInfo(saved.image, (signed char *)animation);
        *(long *)&piece->y = saved.position;
        piece->bounds = bounds;
        animation->kind = saved.kind;
    }
}

int far pascal DrawPillarInBack(DrawPiece near *piece)
{
    Rect water;
    int result = 0;

    if (InWater.frame && InWater.low == 1 && piece->kind >= 6 && piece->kind <= 11) {
        RoofMakeWaterTestRect(&water);
        if (SECTRECT(&water, &water, &piece->bounds))
            result = 1;
    }
    return result;
}

DrawPiece near *far pascal RoofOffsetPillarWaterToFore(DrawPiece near *piece)
{
    piece->y += 12;
    piece->x -= 30;
    OFFSETRECT(&piece->bounds, &piece->bounds, -30, 12);
    return piece;
}

void far pascal AnimRoofShip(void)
{
    Rect bounds;
    sbyte slot;
    DrawPiece near *piece;

    ++BlueSpec;
    if (BlueSpec > 176) {
        Trob.state = -1;
        return;
    }
    if (Room.number == Trob.room) {
        piece = &CustomRoom->pieces[19];
        piece->x = -BlueSpec;
        MAKESHAPERECT(&bounds, piece->y, piece->x, h2p(piece->image_set));
        ++bounds.right;
        MarkBufsForRect(MarkMove, &bounds, -1);
        slot = RestoreKeepCustomPeel(piece->kind);
        UpdateShipPeel(slot, bounds.right);

        piece = &CustomRoom->pieces[(sbyte)(((BlueSpec - 1) & 3) + 21)];
        piece->x = 1 - BlueSpec;
        MAKESHAPERECT(&bounds, piece->y, piece->x, h2p(piece->image_set));
        ++bounds.right;
        MarkBufsForRect(MarkMove, &bounds, -1);
    }
}

void far pascal DrawRoofShip(RoofAnimation near *animation)
{
    Rect clip;
    DrawPiece near *piece;
    word position;
    int ship_x;
    sbyte image;

    if (animation->kind == 11 && animation->frame <= 176) {
        clip = CellClipRect;
        CellClipRect = GamePlayRect;
        position = animation->frame;
        piece = &CustomRoom->pieces[19];
        piece->x = -position;
        MAKESHAPERECT(&piece->bounds, piece->y, piece->x, h2p(piece->image_set));
        DrawCustomRoomImg(19, 11);

        piece = &CustomRoom->pieces[20];
        ship_x = position;
        piece->x = 45 - ship_x;
        MAKESHAPERECT(&piece->bounds, piece->y, piece->x, h2p(piece->image_set));
        piece->pass = 1;
        DrawCustomRoomImg(20, 1);
        piece->pass = 11;

        image = (position & 3) + 21;
        piece = &CustomRoom->pieces[image];
        piece->x = -ship_x;
        MAKESHAPERECT(&piece->bounds, piece->y, piece->x, h2p(piece->image_set));
        DrawCustomRoomImg(image, 11);
        CellClipRect = clip;
    }
}

int far pascal RoofClimbingIntoShip(void)
{
    int result;
    if (Char.room == 19 && Char.sequence == 59)
        result = 1;
    else
        result = 0;
    return result;
}

int far pascal RoofGrabShip(void)
{
    int result = 0;
    int height;

    if (Char.room == 19 && Char.row < 3) {
        height = Level.cell_info[18][0].modifier;
        if (Char.x < 208 && height < 32)
            result = 1;
    }
    return result;
}

int far pascal RoofHangingFromShip(void)
{
    int result;

    if (Char.room == 19 && Char.x < 208 &&
        (Char.action == 2 || Char.action == 6 || Char.frame == 80))
        result = 1;
    else
        result = 0;
    return result;
}

void far pascal UpdateShipPeel(sbyte slot, int bottom)
{
    PEELPORT near *peel;

    if (slot >= 0) {
        peel = PeelInfo.entries[slot].peel;
        if (peel->top < bottom && peel->bottom > bottom)
            peel->bottom = bottom;
    }
}

typedef void (far pascal *LevelTileProc)(RoofAnimation near *);
LevelTileProc DrawRoofFunc[] = {
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, DrawRoofPillar, DrawRoofShip, DrawRoofGlass
};
