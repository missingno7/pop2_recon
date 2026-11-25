/* CUSTCVCA.C, overlay 13. DOS entry order. */
#include "types.h"
#include "level.h"
#include "char.h"
#include "game.h"
#pragma pack(1)

typedef unsigned int Handle;

typedef struct { int height, width; byte flags, unknown, colors; byte data[96]; } Shape;
typedef struct { byte sheet; signed char col; byte flags; signed char row; Handle handle; } ShapeRef;
typedef struct { byte data[4]; Handle handle; } CarpetImage;
typedef struct {
    CarpetImage images[10];
    int frame, sitting, saved;
} Carpet;
typedef struct {
    signed char sheet, flags;
    unsigned int colors;
    int x, y;
    byte col, row;
    Rect bounds;
    byte order, unused;
} CarpetDraw;
typedef struct {
    int x, y;
    byte sheet;
    unsigned int image;
    Rect clip;
    byte order;
    int reserved;
    byte invisible, depth;
} MidItem;
typedef struct { int dy, dx; } CarpetStep;
typedef struct { byte kind; byte row; byte unknown[2]; byte modifier; } TileAnim;
#pragma pack()

static Carpet *carpet;                /* DS:2cdc */
extern unsigned int *Pics[5];
#define CARPET_SHAPE_ID 0x62d7
int carpet_x = 0xb0, carpet_y = 0x55;
static struct {
    int images[10];
    signed char scroll[10];
    CarpetStep steps[10];
    Rect bars;
} carpet_animation = {
    {0, 1, 2, 3, 4, 5, 6, 7, 1, 8},
    {-1, 0, 1, 1, 1, 1, -1, 0, 0, 0},
    {{7, -45}, {8, -46}, {5, -44}, {7, -39}, {7, -41},
     {7, -39}, {5, -43}, {7, -42}, {8, -46}, {8, -46}},
    {5, -88, 26, 7}
};
extern int NewRoom;                 /* DS:6266 */
extern int Ct[5];
extern FCHARREC FChar;
extern byte ColNo, RowNo, ObjId;       /* DS:6ccf,6cce,6cd2 */
extern CarpetDraw far BackTable[];       /* 2fae:0000 */
extern MidItem far MidTable[];
#pragma pack(1)
typedef struct {
    int x, y;
    unsigned char kind;
    int image;
    unsigned char flags[4];
    Rect clip;
    unsigned char state;
} DrawObject;
#pragma pack()
extern DrawObject ObjTable[];                /* DS:5e92 */

extern CHARREC Char;
extern CELLREC Cell;
extern unsigned int BlueSpec;               /* DS:5e48 */

extern TROBREC Trob;
extern Rect CellClipRect, GamePlayRect;         /* DS:6236,09fe */
extern LEVELREC Level;

extern void AddSong(int), AddSound(int), UnloadSounds(int, int);
extern void SetPalette(unsigned int, unsigned int, byte, byte);
extern void far *MATERIALIZE_HANDLE(Handle);
extern void hFree(Handle), hPurge(Handle);
extern Handle hNew(long);
extern Shape far *h2p(Handle);
extern void PrinceNewShpItem(Handle, int, CarpetImage far *, int);
extern void PrinceFreeShp(int);
extern void ResetSnd(int);
extern void MAKESHAPERECT(Rect far *, int, int, Shape far *);
extern void far _fastcall AddPeel(int, int, int, int, byte, byte);
extern void RestoreKeepCustomPeel(byte);
extern void OFFSETRECT(Rect far *, Rect far *, int, int);
extern void SaveFChar(void), RestoreFChar(void), init_fchar_rect(void);
extern void far _fastcall AddCharObj(byte);
typedef void far pascal DrawCallback(void);
extern DrawCallback MarkMove, MarkFRedraw;
extern void MarkBufsForRect(DrawCallback *, Rect near *, signed char);
extern void ReadCell(signed char, signed char, signed char);
extern void MakeSpace(byte, byte);
extern void JumpSeq(int sequence);
extern void DecLifePts(byte amount), SetKidDeathMethod(byte value);
extern void far _fastcall DrawCustomRoomImg(byte index, byte pass);
extern int ComputeTrobMoveRect(Rect near *out, Rect near *base);
extern void * _cdecl _nmalloc(unsigned int);
extern void _cdecl _nfree(void *);
extern void * _cdecl memset(void *, int, unsigned int);
#pragma intrinsic(memset)

int CheckCarpetImg(int index);
void LoadCarpetData(void), UnloadCarpetData(void);
void MapCarpetColors(void), RedrawCaveBars(void);
int GetCarpetFrame(void), IsCaveBarsOpen(void);
Shape far * far _fastcall GetCarpetShpInfo(ShapeRef *ref, int index);

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

int CheckCarpetImg(int index)
{
    Handle copy_handle;
    Shape far *source, far *copy;
    unsigned int entry_offset[1];  /* Keeps the scaled offset in a stack slot across calls. */

    if (carpet) {
        entry_offset[0] = index * sizeof(CarpetImage);
#define ENTRY ((CarpetImage *)((char *)carpet + entry_offset[0]))
        if (!ENTRY->handle || !MATERIALIZE_HANDLE(ENTRY->handle)) {
            if (ENTRY->handle) {
                hFree(ENTRY->handle);
                ENTRY->handle = 0;
            }
            copy_handle = hNew(sizeof(Shape));
            source = h2p(*Pics[2]);
            copy = h2p(copy_handle);
            *copy = *source;
            copy->flags |= 4;
            copy->colors = 32;
            PrinceNewShpItem(copy_handle, index + 302, ENTRY, 1);
            hPurge(ENTRY->handle);
            hFree(copy_handle);
            PrinceFreeShp(CARPET_SHAPE_ID + index);
        }
    }
#undef ENTRY
    return carpet->images[index].handle;
}

void DrawCarpet(byte *tile)
{
    CarpetDraw far *entry;
    ShapeRef ref;
    Shape far *shape;

    if (carpet && !carpet->frame && !*tile && NewRoom) {
        entry = &BackTable[Ct[0]];
        ++Ct[0];
        entry->y = carpet_x;
        entry->x = carpet_y;
        entry->colors = CARPET_SHAPE_ID;
        entry->sheet = 4;
        entry->col = ColNo;
        entry->row = RowNo;
        shape = GetCarpetShpInfo(&ref, 0);
        MAKESHAPERECT(&entry->bounds, entry->y, entry->x, shape);
        entry->order = 10;
        entry->flags = ObjId;
    }
}

void SaveUnderCarpet(void)
{
    ShapeRef ref;
    Shape far *shape;

    if (carpet && !carpet->saved) {
        shape = GetCarpetShpInfo(&ref, 0);
        AddPeel(carpet_y, carpet_y + shape->width, carpet_x,
                         shape->height, 101, 2);
        carpet->saved = 1;
    }
}

void DrawObjCarpet(byte *unused)
{
    int slot, frame;
    Shape far *shape;
    ShapeRef ref;
    signed char scroll;
    byte palette;

    if (carpet && carpet->frame) {
        slot = Ct[3]; MidTable[slot].x = FChar.x;
        frame = GetCarpetFrame(); shape = GetCarpetShpInfo(&ref, carpet_animation.images[frame] + 1);
        MidTable[slot].y = FChar.y - shape->height; MidTable[slot].sheet = 4;
        MidTable[slot].image = CARPET_SHAPE_ID + carpet_animation.images[frame] + 1; MidTable[slot].clip = *(Rect *)&FChar.clip.top;
        MidTable[slot].order = 10;
        MidTable[slot].reserved = 0; MidTable[slot].invisible = 0; MidTable[slot].depth = 255;
        if (carpet_animation.scroll[frame]) {
            scroll = carpet_animation.scroll[frame];
            ObjTable[0].y += scroll; OFFSETRECT(&ObjTable[0].clip, &ObjTable[0].clip, 0, scroll);
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

Shape far * far _fastcall GetCarpetShpInfo(ShapeRef *ref, int index)
{
    Shape far *shape = 0;

    if (carpet) {
        char zero = 0;
        ref->sheet = 1;
        ref->col = zero;
        ref->row = zero;
        if (ref->handle = CheckCarpetImg(index))
            shape = h2p(carpet->images[index].handle);
    }
    return shape;
}

void LoadCarpetData(void)
{
    int index;
    carpet = _nmalloc(sizeof(Carpet));
    memset(carpet, 0, sizeof(Carpet));
    index = 0;
    do {
        CheckCarpetImg(index);
    } while (++index < 10);
}

void MapCarpetColors(void)
{
    SetPalette(CARPET_SHAPE_ID, 16, 32, 0);
}

void SetupCarpet(void)
{
    int frame;
    ShapeRef ref;
    Shape far *shape;

    if (carpet && carpet->frame) {
        SaveFChar();
        frame = GetCarpetFrame();
        FChar.image = CARPET_SHAPE_ID + carpet_animation.images[frame] + 1;
        FChar.y += carpet_animation.steps[frame].dy;
        FChar.x += carpet_animation.steps[frame].dx;
        if (!(FChar.flipped))
            FChar.x -= 20;
        init_fchar_rect();
        shape = GetCarpetShpInfo(&ref, FChar.image - CARPET_SHAPE_ID);
        MAKESHAPERECT((Rect *)&FChar.clip.top, FChar.y - shape->height, FChar.x, shape);
        AddCharObj(13);
        MarkBufsForRect(MarkFRedraw, (Rect *)&FChar.clip.top, -1);
        RestoreFChar();
    }
}

void Sitting(void)
{
    ShapeRef ref;
    Shape far *shape;
    Rect rect;

    if (carpet) {
        if (!carpet->frame && Char.frame == 295) {
            carpet->frame = 1;
            ReadCell(3, 3, 2);
            MakeSpace(Cell.room, (Cell.index));
            shape = GetCarpetShpInfo(&ref, 0);
            MAKESHAPERECT(&rect, carpet_x, carpet_y, shape);
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
        if (carpet->images[i].handle)
            hFree(carpet->images[i].handle);
    }
    ResetSnd(10033);
    UnloadSounds(33, 36);
    _nfree(carpet);
    carpet = 0;
}

void AnimCaveBars(void)
{
    unsigned int phase = BlueSpec & 7;
    if (!Trob.state) {
        if (phase < 3) {
            if (!phase)
                AddSound(78);
            BlueSpec &= ~7;
            BlueSpec |= phase + 1;
            RedrawCaveBars();
            return;
        }
    } else if (phase) {
        if (phase == 3)
            AddSound(79);
        BlueSpec &= ~7;
        BlueSpec |= phase - 1;
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

void DrawCaveBars(TileAnim *tile)
{
    Rect saved;
    if (tile->kind == 11 && tile->row == ColNo) {
        if (!NewRoom) {
            saved = CellClipRect;
            CellClipRect = GamePlayRect;
        }
        DrawCustomRoomImg((tile->modifier & 7) + 2, 11);
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

signed char TriggerCaveBars(byte room, signed char cell)
{
    signed char result = -1;
    int state;
    if (!(((byte *)&Cell.modifier)[1] & 8)) {
        state = (int)((LEVELCELL *)Level.cell_info)[room * (byte)30 + cell - 30].modifier;
        if (!state)
            result = 0;
        else if (state == 3)
            result = 1;
    }
    return result;
}
