/* CUSTCVCA.C, overlay 13. DOS entry order. */
#include <string.h>
#include <malloc.h>
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
#include "add.h"
#include "ctrl.h"
#include "custcvca.h"
#include "fade.h"
#include "move.h"
#include "prince.h"
#include "sound.h"
#include "top.h"
#pragma pack(1)


typedef struct {
    SHAPEINFO images[10];
    int frame, sitting, saved;
} Carpet;


#pragma pack()

static Carpet *carpet;                /* DS:2cdc */


#define CARPET_SHAPE_ID 0x62d7
int carpet_x = 0xb0, carpet_y = 0x55;
static struct {
    int images[10];
    sbyte scroll[10];
    POINT steps[10];
    Rect bars;
} carpet_animation = {
    {0, 1, 2, 3, 4, 5, 6, 7, 1, 8},
    {-1, 0, 1, 1, 1, 1, -1, 0, 0, 0},
    {{7, -45}, {8, -46}, {5, -44}, {7, -39}, {7, -41},
     {7, -39}, {5, -43}, {7, -42}, {8, -46}, {8, -46}},
    {5, -88, 26, 7}
};
                 /* DS:6266 */


       /* DS:6ccf,6cce,6cd2 */

       /* 2fae:0000 */


                /* DS:5e92 */


               /* DS:5e48 */


         /* DS:6236,09fe */


void LoadCarpetCode(void)
{
    MapCarpetColors();
    LoadCarpetData();
    AddSong(33);
}

void UnloadCarpetCode(void)
{
    UnloadCarpetData();
    SetPalette(750, 32, 16, 0);
}

Handle CheckCarpetImg(int index)
{
    Handle copy_handle;
    SHAPELISTRSRC far *source, far *copy;

    if (carpet) {
        if (!carpet->images[index].image || !materialize_handle(carpet->images[index].image)) {
            if (carpet->images[index].image) {
                hFree(carpet->images[index].image);
                carpet->images[index].image = 0;
            }
            copy_handle = hNew(sizeof(SHAPELISTRSRC));
            source = h2p(Pics[2]->resource);
            copy = h2p(copy_handle);
            *copy = *source;
            *(byte far *)&copy->palette.banks |= 4;
            copy->palette.colors = 32;
            PrinceNewShpItem(copy_handle, index + 302, &carpet->images[index], 1);
            hPurge(carpet->images[index].image);
            hFree(copy_handle);
            PrinceFreeShp(CARPET_SHAPE_ID + index);
        }
    }
    return carpet->images[index].image;
}

void DrawCarpet(DRAWCELL *cell)
{
    BFITEM far *entry;
    SHAPEINFO ref;
    SHAPE far *shape;

    if (carpet && !carpet->frame && !cell->pass && NewRoom) {
        entry = &BackTable[Ct[0]];
        ++Ct[0];
        entry->pos.y = carpet_x;
        entry->pos.x = carpet_y;
        entry->pos.image = CARPET_SHAPE_ID;
        entry->image_set = 4;
        entry->column = ColNo;
        entry->row = RowNo;
        shape = GetCarpetShpInfo(&ref, 0);
        MakeShapeRect(&entry->clip, entry->pos.y, entry->pos.x, shape);
        entry->blit = 10;
        entry->peel = ObjId;
    }
}

void SaveUnderCarpet(void)
{
    SHAPEINFO ref;
    SHAPE far *shape;

    if (carpet && !carpet->saved) {
        shape = GetCarpetShpInfo(&ref, 0);
        AddPeel(carpet_y, carpet_y + shape->width, carpet_x,
                         shape->height, 101, 2);
        carpet->saved = 1;
    }
}

void DrawObjCarpet(byte unused)
{
    int slot, frame;
    SHAPE far *shape;
    SHAPEINFO ref;
    sbyte scroll;
    byte palette;

    if (carpet && carpet->frame) {
        slot = Ct[3]; MidTable[slot].x = FChar.x;
        frame = GetCarpetFrame(); shape = GetCarpetShpInfo(&ref, carpet_animation.images[frame] + 1);
        MidTable[slot].y = FChar.y - shape->height; MidTable[slot].image_set = 4;
        MidTable[slot].image = CARPET_SHAPE_ID + carpet_animation.images[frame] + 1; MidTable[slot].clip = FChar.clip;
        MidTable[slot].blit = 10;
        MidTable[slot].variant = 0; MidTable[slot].mirrored = 0; MidTable[slot].owner = 255;
        if (carpet_animation.scroll[frame]) {
            scroll = carpet_animation.scroll[frame];
            ObjTable[0].y += scroll; OffsetRect(&ObjTable[0].clip, &ObjTable[0].clip, 0, scroll);
        }
        if (carpet->sitting) {
            palette = carpet->frame - 4;
            if (palette < 8) SetPalette(CARPET_SHAPE_ID, 16, 32, palette);
        }
        ++Ct[3]; ++carpet->frame;
        if (carpet->saved) {
            RestoreKeepCustomPeel(1); carpet->saved = 0;
        }
    }
}

int GetCarpetFrame(void)
{
    int frame;

    if (carpet) {
        if (!carpet->sitting) {
            frame = (carpet->frame / 2) % 9;
        } else {
            frame = carpet->frame + 8;
            frame = frame < 9 ? frame : 9;
        }
    }
    return frame; /* DOS leaves the result uninitialized when carpet is null. */
}

SHAPE far * _fastcall GetCarpetShpInfo(SHAPEINFO *ref, int index)
{
    SHAPE far *shape = 0;

    if (carpet) {
        ref->mode = 1;
        ref->temporary = 0;
        ref->mirrored = 0;
        if (ref->image = CheckCarpetImg(index))
            shape = h2p(carpet->images[index].image);
    }
    return shape;
}

void LoadCarpetData(void)
{
    int index;
    carpet = _nmalloc(sizeof(Carpet));
    memset(carpet, 0, sizeof(Carpet));
    for (index = 0; index < 10; ++index) {
        CheckCarpetImg(index);
    }
}

void MapCarpetColors(void)
{
    SetPalette(CARPET_SHAPE_ID, 16, 32, 0);
}

void SetupCarpet(void)
{
    int frame;
    SHAPEINFO ref;
    SHAPE far *shape;

    if (carpet && carpet->frame) {
        SaveFChar();
        frame = GetCarpetFrame();
        FChar.image = CARPET_SHAPE_ID + carpet_animation.images[frame] + 1;
        FChar.y += carpet_animation.steps[frame].y;
        FChar.x += carpet_animation.steps[frame].x;
        if (!FChar.direction)
            FChar.x -= 20;
        init_fchar_rect();
        shape = GetCarpetShpInfo(&ref, FChar.image - CARPET_SHAPE_ID);
        MakeShapeRect(&FChar.clip, FChar.y - shape->height, FChar.x, shape);
        AddCharObj(13);
        MarkBufsForRect(MarkFRedraw, &FChar.clip, -1);
        RestoreFChar();
    }
}

void Sitting(void)
{
    SHAPEINFO ref;
    SHAPE far *shape;
    Rect rect;

    if (carpet) {
        if (!carpet->frame && Char.frame == 295) {
            carpet->frame = 1;
            ReadCell(3, 3, 2);
            MakeSpace(Cell.room, Cell.index);
            shape = GetCarpetShpInfo(&ref, 0);
            MakeShapeRect(&rect, carpet_x, carpet_y, shape);
            MarkBufsForRect(MarkMove, &rect, -1);
            return;
        }
        if (!carpet->sitting && Char.y < 55 && !IsCaveBarsOpen()) {
            carpet->frame = 1;
            carpet->sitting = 1;
            Char.y = 20;
            Char.sequence_group = 10;
            JumpSeq(129);
            AddSound(36);
            return;
        }
        if (carpet->sitting && carpet->frame >= 1) {
            DecLifePts(100);
            SetKidDeathMethod(9);
        }
    }
}

void TriggerCharOnCarpet(void)
{
    if (carpet) {
        JumpSeq(128);
        Char.x = 268;
        if (Char.direction == 0)
            Char.x = 288;
        if (IsCaveBarsOpen()) {
            AddSong(34);
            return;
        }
        AddSong(35);
    }
}

void UnloadCarpetData(void)
{
    int i;

    for (i = 0; i < 10; i++) {
        if (carpet->images[i].image)
            hFree(carpet->images[i].image);
    }
    ResetSnd(10033);
    UnloadSounds(33, 36);
    _nfree(carpet);
    carpet = 0;
}

void AnimCaveBars(void)
{
    word phase = BlueSpec.modifier & 7;
    if (!Trob.state) {
        if (phase < 3) {
            if (!phase)
                AddSound(78);
            BlueSpec.modifier &= ~7;
            BlueSpec.modifier |= phase + 1;
            RedrawCaveBars();
            return;
        }
    } else if (phase) {
        if (phase == 3)
            AddSound(79);
        BlueSpec.modifier &= ~7;
        BlueSpec.modifier |= phase - 1;
        RedrawCaveBars();
        return;
    }
    Trob.state = -1;
}

int IsCaveBarsOpen(void)
{
    ReadCell(3, 6, 0);
    if ((Cell.modifier & 7) >= 3)
        return 1;
    return 0;
}

void DrawCaveBars(DRAWCELL *tile)
{
    Rect saved;
    if (tile->pass == 11 && tile->column == ColNo) {
        if (!NewRoom) {
            saved = CellClipRect;
            CellClipRect = GamePlayRect;
        }
        DrawCustomRoomImg((tile->info.modifier & 7) + 2, 11);
        if (!NewRoom)
            CellClipRect = saved;
    }
}

void RedrawCaveBars(void)
{
    Rect rect;
    if (ComputeTrobMoveRect(&rect, &carpet_animation.bars))
        MarkBufsForRect(MarkMove, &rect, -1);
}

sbyte TriggerCaveBars(byte room, sbyte cell)
{
    sbyte result = -1;
    int state;
    if (!(((byte *)&Cell.modifier)[1] & 8)) {
        state = (int)((LEVELCELL *)Level.cell_info)[room * 30 + cell - 30].modifier;
        if (!state)
            result = 0;
        else if (state == 3)
            result = 1;
    }
    return result;
}
