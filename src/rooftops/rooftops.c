/* ROOFTOPS.C: DOS overlay 4. Descriptive names mark unresolved symbols. */
#include "types.h"
#include "graf.h"
#include "system.h"
#include "snd.h"
#include "level.h"
#include "shape.h"
#include "heap.h"
#include "lgraf.h"
#include "char.h"
#include "game.h"
#include "draw.h"
#include "custom.h"
#include "act.h"
#include "add.h"
#include "ctrl.h"
#include "fade.h"
#include "moveruin.h"
#include "prince.h"
#include "intrface.h"
#include "rooftops.h"
#include "move.h"
#include "sound.h"
#include "top.h"
#include "util.h"

#pragma pack(1)


typedef struct {
    byte index;
    byte reserved;
} SplashImage;
#pragma pack()


WATERSTATE InWater;
static sbyte palette_bank;


static SplashImage splash_images[] = {
    {29, 0}, {30, 0}, {31, 0},
    {29, 0}, {32, 0}, {33, 0}, {29, 0}
};


typedef void (*RedrawCallback)(void);


int _fastcall DrawRoofLedgeInBack(CUSTOMPIECE *piece)
{
    Rect intersection;
    int result;

    if ((piece->shape == 35 || piece->shape == 22 ||
         piece->shape == 72 || piece->shape == 23) &&
        SectRect(&intersection, &piece->clip, &Kid.bounds) &&
        Kid.sequence != 68 &&
        (Kid.action == 3 || Kid.action == 4))
        result = 1;
    else
        result = 0;
    return result;
}

void LoadRoofCode(void)
{
    sbyte inactive = -1;

    InWater.frame = 0;
    palette_bank = inactive;
    InWater.character = inactive;
}

void RoofCheckCharInWater(void)
{
    if (InWater.character == -1) {
        if (Char.y >= 172 && Char.sequence != 68 && Char.sequence != 15 &&
            Char.sequence != 59 && Char.action != 2)
            RoofTriggerCharInWater();
    } else if (Char.slot == InWater.character) {
        if (InWater.frame < 7) {
            if (CustomRoom->kind != 18 && CustomRoom->kind != 19)
                return;
            MarkBufsForRect(MarkMove, &CustomRoom->pieces[splash_images[InWater.frame - 1].index].clip, -1);
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

void RoofCheckPalette(sbyte room)
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

void RoofCheckRoomSpecific(void)
{
    RoofCheckPalette(Room.number);
}

void RoofClipChar(void)
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

void RoofCycleColors(void)
{
    if (!GameState.fade_active && (Room.number == 19 || Room.number == 16 || Room.number == 15) &&
        FrameNum % 3 == 0) {
        CycleColors(230, 3, 0);
        CycleColors(233, 3, 0);
        CycleColors(236, 2, 0);
    }
}

void RoofFrameFunc(void)
{
    RoofCycleColors();
    RoofWatchKid();
    RoofWatchOpps();
}

void RoofKillKidOnWharf(void)
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

Rect *RoofMakeWaterTestRect(Rect *rect)
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

void RoofTriggerCharInWater(void)
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

void RoofWatchKid(void)
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

void RoofWatchOpps(void)
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

void SetupSplash(void)
{
    Rect clip;
    CUSTOMPIECE *piece;
    byte image;

    if (Char.slot == InWater.character && InWater.frame != 0 && InWater.frame < 7 &&
        (CustomRoom->kind == 18 || CustomRoom->kind == 19)) {
        clip = CellClipRect;
        image = splash_images[InWater.frame - 1].index;
        piece = &CustomRoom->pieces[image];
        piece->pos.y = piece->clip.top - piece->clip.bottom + 176;
        if (Char.action == 9)
            piece->pos.y += 14;
        piece->pos.x = (piece->clip.left - piece->clip.right) / 2 + InWater.x;
        MakeShapeRect(&piece->clip, piece->pos.y, piece->pos.x, h2p(piece->handle));
        CellClipRect = piece->clip;
        piece->pass = 1;
        DrawCustomRoomImg(image, piece->pass);
        piece->pass = 11;
        if (Char.action != 9)
            MarkBufsForRect(MarkFRedraw, &piece->clip, -1);
        CellClipRect = clip;
    }
}

void AnimRoofGlass(void)
{
    if (BlueSpec.modifier >= 11) {
        Trob.state = -1;
        return;
    }
    ++BlueSpec.modifier;
    if (BlueSpec.modifier == 1)
        AddSound(49);
    MarkBufsForRect(MarkMove, &CustomRoom->pieces[(byte)(BlueSpec.modifier + 30)].clip, -1);
}

void DrawRoofGlass(DRAWCELL *animation)
{
    Rect clip;
    byte image;

    if (animation->pass == 11 && animation->info.modifier < 11) {
        clip = CellClipRect;
        CellClipRect = GamePlayRect;
        image = (byte)(animation->info.modifier + 31);
        DrawCustomRoomImg(image, 11);
        if (image + 1 < CustomRoom->count)
            MarkBufsForRect(MarkFRedraw, &CustomRoom->pieces[image + 1].clip, -1);
        CellClipRect = clip;
    }
}


void AnimRoofPillar(void)
{
    CUSTOMPIECE *piece;
    long position;
    Rect water, bounds;
    word step;

    if (CheckTrobVis()) {
        step = BlueSpec.modifier & 15;
        if (step == 7)
            step = 0;
        else
            ++step;
        BlueSpec.modifier &= 0xfff0;
        BlueSpec.modifier |= step;
        RoofMakeWaterTestRect(&water);
        piece = &CustomRoom->pieces[25];
        bounds = piece->clip;
        position = *(long *)&piece->pos.y;
        piece = CustomPieceOffsetToColRow(piece, Trob.cell % 10, Trob.cell / 10);
        if (!InWater.frame || !SectRect(&water, &water, &piece->clip)) {
            MarkBufsForRect(MarkMove, &piece->clip, -1);
            RoofOffsetPillarWaterToFore(piece);
            MarkBufsForRect(MarkMove, &piece->clip, -1);
        }
        *(long *)&piece->pos.y = position;
        piece->clip = bounds;
    }
}

void DrawRoofPillar(DRAWCELL *animation)
{
    CUSTOMPIECE *piece;
    struct {
        long position;
        sbyte kind;
        byte image;
    } saved;
    Rect bounds;

    if (animation->pass == 11 || (animation->pass == 0 && NewRoom)) {
        saved.kind = animation->pass;
        animation->pass = 0;
        saved.image = ((animation->info.modifier >> 1) & 7) + 25;
        DrawCustomRoomImgAtInfo(saved.image, animation);
        piece = &CustomRoom->pieces[saved.image];
        bounds = piece->clip;
        saved.position = *(long *)&piece->pos.y;
        piece = CustomPieceOffsetToColRow(piece, animation->column, animation->row);
        if (!DrawPillarInBack(piece))
            animation->pass = 1;
        *(long *)&piece->pos.y = saved.position;
        piece->clip = bounds;
        RoofOffsetPillarWaterToFore(piece);
        DrawCustomRoomImgAtInfo(saved.image, animation);
        *(long *)&piece->pos.y = saved.position;
        piece->clip = bounds;
        animation->pass = saved.kind;
    }
}

int DrawPillarInBack(CUSTOMPIECE *piece)
{
    Rect water;
    int result = 0;

    if (InWater.frame && InWater.low == 1 && piece->shape >= 6 && piece->shape <= 11) {
        RoofMakeWaterTestRect(&water);
        if (SectRect(&water, &water, &piece->clip))
            result = 1;
    }
    return result;
}

CUSTOMPIECE *RoofOffsetPillarWaterToFore(CUSTOMPIECE *piece)
{
    piece->pos.y += 12;
    piece->pos.x -= 30;
    OffsetRect(&piece->clip, &piece->clip, -30, 12);
    return piece;
}

void AnimRoofShip(void)
{
    Rect bounds;
    sbyte slot;
    CUSTOMPIECE *piece;

    ++BlueSpec.modifier;
    if (BlueSpec.modifier > 176) {
        Trob.state = -1;
        return;
    }
    if (Room.number == Trob.room) {
        piece = &CustomRoom->pieces[19];
        piece->pos.x = -BlueSpec.modifier;
        MakeShapeRect(&bounds, piece->pos.y, piece->pos.x, h2p(piece->handle));
        ++bounds.right;
        MarkBufsForRect(MarkMove, &bounds, -1);
        slot = RestoreKeepCustomPeel(piece->shape);
        UpdateShipPeel(slot, bounds.right);

        piece = &CustomRoom->pieces[(sbyte)(((BlueSpec.modifier - 1) & 3) + 21)];
        piece->pos.x = 1 - BlueSpec.modifier;
        MakeShapeRect(&bounds, piece->pos.y, piece->pos.x, h2p(piece->handle));
        ++bounds.right;
        MarkBufsForRect(MarkMove, &bounds, -1);
    }
}

void DrawRoofShip(DRAWCELL *animation)
{
    Rect clip;
    CUSTOMPIECE *piece;
    word position;
    int ship_x;
    sbyte image;

    if (animation->pass == 11 && animation->info.modifier <= 176) {
        clip = CellClipRect;
        CellClipRect = GamePlayRect;
        position = animation->info.modifier;
        piece = &CustomRoom->pieces[19];
        piece->pos.x = -position;
        MakeShapeRect(&piece->clip, piece->pos.y, piece->pos.x, h2p(piece->handle));
        DrawCustomRoomImg(19, 11);

        piece = &CustomRoom->pieces[20];
        ship_x = position;
        piece->pos.x = 45 - ship_x;
        MakeShapeRect(&piece->clip, piece->pos.y, piece->pos.x, h2p(piece->handle));
        piece->pass = 1;
        DrawCustomRoomImg(20, 1);
        piece->pass = 11;

        image = (position & 3) + 21;
        piece = &CustomRoom->pieces[image];
        piece->pos.x = -ship_x;
        MakeShapeRect(&piece->clip, piece->pos.y, piece->pos.x, h2p(piece->handle));
        DrawCustomRoomImg(image, 11);
        CellClipRect = clip;
    }
}

int RoofClimbingIntoShip(void)
{
    int result;
    if (Char.room == 19 && Char.sequence == 59)
        result = 1;
    else
        result = 0;
    return result;
}

int RoofGrabShip(void)
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

int RoofHangingFromShip(void)
{
    int result;

    if (Char.room == 19 && Char.x < 208 &&
        (Char.action == 2 || Char.action == 6 || Char.frame == 80))
        result = 1;
    else
        result = 0;
    return result;
}

void UpdateShipPeel(sbyte slot, int bottom)
{
    SAVEDRECT *peel;

    if (slot >= 0) {
        peel = PeelInfo.items[slot].saved;
        if (peel->rect.left < bottom && peel->rect.right > bottom)
            peel->rect.right = bottom;
    }
}

DRAWPROC DrawRoofFunc[] = {
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, DrawRoofPillar, DrawRoofShip, DrawRoofGlass
};
