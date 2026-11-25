/* DOS CUSTOM.C, resident frame 04b3. Function order follows the DOS image. */
#include <string.h>
#include <malloc.h>
#include "types.h"
#include "resc.h"
#include "graf.h"
#include "shape.h"
#include "level.h"
#include "heap.h"
#include "char.h"
#include "game.h"
#include "draw.h"
#include "add.h"
#include "custom.h"
#include "altar.h"
#include "bridge.h"
#include "custcvca.h"
#include "desert.h"
#include "fade.h"
#include "final.h"
#include "move.h"
#include "moveruin.h"
#include "movetemp.h"
#include "prince.h"
#include "rooftops.h"
#include "sound.h"
#include "throne.h"
#include "top.h"
#include "recorder.h"
#include "util.h"


CUSTOMROOM *CustomRoom = 0;


/* The reserved fields are zero in each 28-byte DOS template record. */
#define UNUSED_TEMPLATE_FIELDS { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }

ROOMTEMPLATE CVCarpetPlate = { { LoadCarpetCode }, { UnloadCarpetCode }, UNUSED_TEMPLATE_FIELDS };
ROOMTEMPLATE CVBridgePlate = { { LoadBridgeCode }, { UnloadBridgeCode }, UNUSED_TEMPLATE_FIELDS };
ROOMTEMPLATE DSExitPlate = { { LoadDsrtCode }, { 0 }, UNUSED_TEMPLATE_FIELDS };
ROOMTEMPLATE RNThronePlate = { { LoadRNThroneFunc }, { UnloadRNThroneFunc }, UNUSED_TEMPLATE_FIELDS };
ROOMTEMPLATE RNStart6Plate = { { LoadRNStart6 }, { UnloadRNStart6 }, UNUSED_TEMPLATE_FIELDS };
ROOMTEMPLATE RNPlateauPlate = { { LoadPlateauRoomFunc }, { 0 }, UNUSED_TEMPLATE_FIELDS };
ROOMTEMPLATE TMAltarPlate = { { LoadAltarRoomFunc }, { UnloadAltarRoomFunc }, UNUSED_TEMPLATE_FIELDS };
ROOMTEMPLATE TMInscriptionPlate = { { LoadInscriptionRoomFunc }, { 0 }, UNUSED_TEMPLATE_FIELDS };
ROOMTEMPLATE FNChessPlate = { { LoadChess }, { 0 }, UNUSED_TEMPLATE_FIELDS };
ROOMTEMPLATE FNCrystalPlate = { { LoadCrystal }, { 0 }, UNUSED_TEMPLATE_FIELDS };
ROOMTEMPLATE FNTowerTopPlate = { { LoadTowerTop }, { 0 }, UNUSED_TEMPLATE_FIELDS };
ROOMTEMPLATE NullPlate = { { 0 }, { 0 }, UNUSED_TEMPLATE_FIELDS };

#undef UNUSED_TEMPLATE_FIELDS

static ROOMTEMPLATE *template_table[] = {
    &CVCarpetPlate, &NullPlate, &NullPlate,
    &NullPlate, &NullPlate, &NullPlate,
    &DSExitPlate, &NullPlate, &NullPlate,
    &NullPlate, &NullPlate, &NullPlate,
    &NullPlate, &NullPlate, &NullPlate,
    &NullPlate, &NullPlate, &NullPlate,
    &NullPlate, &NullPlate, &RNPlateauPlate,
    &RNPlateauPlate, &RNStart6Plate, &FNTowerTopPlate,
    &NullPlate, &FNChessPlate, &FNChessPlate,
    &FNChessPlate, &FNCrystalPlate, &FNCrystalPlate,
    &FNCrystalPlate, &TMInscriptionPlate, &TMAltarPlate,
    &CVBridgePlate, &RNThronePlate
};

ROOMTEMPLATE *CustomTemplate = &NullPlate;


void AddCustomPiecePeel(CUSTOMPIECE *piece, Rect far *rect)
{
    AddPeel(rect->left, rect->right, rect->top, rect->bottom - rect->top,
             (byte)(piece->shape + 100), piece->peel);
}

void CheckRoomSpecific(void)
{
    switch (Level.environment_kind) {
    case 4:
        if (!Room.above)
            EraseRect(TopRowRect);
        break;
    case 5:
        RoofCheckRoomSpecific();
        break;
    }
    if (HorseShpInfo.image) {
        hFree(HorseShpInfo.image);
        HorseShpInfo.image = 0;
    }
}


void DrawCustomRoomImgAtInfo(byte index, DRAWCELL *cell)
{
    CUSTOMPIECE *piece = &CustomRoom->pieces[index];
    Rect saved_rect;
    POINT saved_position;
    byte saved_pass;

    saved_rect = piece->clip;
    saved_position = piece->pos;
    piece = CustomPieceOffsetToColRow(piece, cell->column, cell->row);
    saved_pass = piece->pass;
    piece->pass = cell->pass;
    DrawCustomRoomImg(index, cell->pass);
    piece->pos = saved_position;
    piece->pass = saved_pass;
    piece->clip = saved_rect;
}

CUSTOMPIECE *CustomPieceOffsetToColRow(CUSTOMPIECE *piece, sbyte column, sbyte row)
{
    int dx = (row + 1) * 63;
    int dy;

    piece->pos.y += dx;
    dy = column * 32;
    piece->pos.x += dy;
    OffsetRect(&piece->clip, &piece->clip, dy, dx);
    return piece;
}

void GetCustomAboveAry(void)
{
    sbyte i = 0;
    do {
        Above[i].kind = 0;
        Above[i].info.modifier = 0xC000;
        Above[i].info.background = 0;
        ++i;
    } while (i < 11);
}

void far *GetCustomPieceShape(CUSTOMPIECE *piece)
{
    typedef struct {
        word resource;
        void far *archive;
    } ShapeLookup;
    ShapeLookup lookup;

    if (!piece->handle || !materialize_handle(piece->handle)) {
        if (piece->handle)
            hFree(piece->handle);
        piece->handle = PrinceUnpackShape(
            lookup.resource = hGetRsrc(0x53484150UL,
                                            piece->shape + CustomRoom->first_shape),
            2, hLock(Pics[4]->resource));
        hUnlock(Pics[4]->resource);
    }
    return h2p(piece->handle);
}

SHAPEINFO *_fastcall GetCustomPieceShpInfo(int index, SHAPEINFO *info)
{
    if (CustomRoom) {
        sbyte zero = 0;
        info->mode = 1;
        info->temporary = zero;
        info->mirrored = zero;
        info->image = CustomRoom->pieces[index].handle;
    } else {
        info = 0;
    }
    return info;
}

SHAPESIZE far *GetCustomPieceSize(byte index, SHAPESIZE far *size)
{
    SHAPESIZE far *shape;

    if (!CustomRoom)
        size = 0;
    else {
        shape = GetCustomPieceShape(&CustomRoom->pieces[index]);
        if (!shape)
            size = 0;
        else
            *size = *shape;
    }
    return size;
}

int IsCustomizedRoom(byte room)
{
    LEVELCELL *tiles = Level.cell_info[room - 1];
    word flags = 0;
    sbyte tile = 0;

    do {
        if (flags)
            break;
        flags = tiles[tile].modifier & 0xC000;
        ++tile;
    } while (tile < 30);
    return flags != 0;
}

void LoadCustomRoomData(byte room_number_arg)
{
    CUSTOMROOM *room;
    register unsigned long load;

    if (Room.custom) {
        room = nGetRsrc(0x43555354UL, (room_number_arg + 159) * 25);
        if (room) {
            if (CustomRoom && room->kind == CustomRoom->kind) {
                /* The DOS routine repeats the template comparison on this path. */
                if (room->kind == CustomRoom->kind) {
                    LoadCustomRoomShapes(room);
                    UnloadCustomRoomShapes(1);
                    _nfree(CustomRoom);
                    CustomRoom = room;
                }
            } else {
                LoadCustomRoomShapes(room);
                UnloadCustomRoomData();
                CustomRoom = room;
                LoadSounds(room->first_sound, room->last_sound);
                SetCustomRoomTemplate(CustomRoom->kind);
            }
            if ((load = CustomTemplate->load.address) != 0)
                ((void (*)(void))load)();
        }
    } else if (CustomRoom) {
        UnloadCustomRoomData();
        if ((Level.level_number == 5 && Room.number == 7) ||
            (Level.level_number == 5 && Room.number == 12))
            LoadBridgeCode();
    }
}

void LoadCustomRoomShapes(CUSTOMROOM *room)
{
    int i, old_count, bytes, shape_base, position;

    Handle *old_shapes = 0, *new_shapes;
    void far *archive;
    void far *bitmap;
    CUSTOMPIECE *piece;
    Rect rect;

    if (CustomRoom) {
        if (CustomRoom->first_shape == (shape_base = room->first_shape)) {
            old_count = 1 + (CustomRoom->last_shape >= room->last_shape ?
                             CustomRoom->last_shape : room->last_shape);
            bytes = old_count * sizeof(Handle);
            old_shapes = _nmalloc(bytes);
            memset(old_shapes, 0, bytes);
            for (i = 0; i < CustomRoom->count; ++i) {
                if (CustomRoom->pieces[i].handle) {
                    piece = &CustomRoom->pieces[i];
                    old_shapes[piece->shape] = CustomRoom->pieces[i].handle;
                    CustomRoom->pieces[i].handle = 0;
                }
            }
        }
        if (CustomRoom->kind != room->kind)
            UnloadCustomRoomData();
    }

    shape_base = room->first_shape;
    position = hGetState(Pics[4]->resource);
    relocate_handle_payload(Pics[4]->resource);
    archive = hLock(Pics[4]->resource);
    bytes = (room->last_shape + 1) * sizeof(Handle);
    new_shapes = _nmalloc(bytes);
    memset(new_shapes, 0, bytes);
    i = 0;
    if (room->count > 0) {
        do {
            if (room->pieces[i].shape != -1) {
                piece = &room->pieces[i];
                if (!new_shapes[piece->shape]) {
                    if (old_shapes && old_shapes[piece->shape]) {
                        new_shapes[piece->shape] = old_shapes[piece->shape];
                        old_shapes[piece->shape] = 0;
                    } else {
                        Handle resource = hGetRsrc(0x53484150UL, piece->shape + shape_base);
                        new_shapes[piece->shape] = PrinceUnpackShape(resource, 2, archive);
                    }
                }
                piece->handle = new_shapes[piece->shape];
                bitmap = h2p(piece->handle);
                MakeShapeRect(&rect, piece->pos.y, piece->pos.x, bitmap);
                if (EmptyRect(&piece->clip)) {
                    piece->clip = rect;
                } else {
                    OffsetRect(&piece->clip, &piece->clip, piece->pos.x, piece->pos.y);
                    SectRect(&piece->clip, &piece->clip, &rect);
                }
                piece->image = i + shape_base;
            } else {
                piece->handle = 0;
                piece->clip = NullRect;
                piece->image = -1;
            }
        } while (++i < room->count);
    }
    hSetState(Pics[4]->resource, position);
    if (old_shapes) {
        for (i = 0; i < old_count; ++i) {
            if (old_shapes[i])
                hFree(old_shapes[i]);
        }
        _nfree(old_shapes);
    }
    _nfree(new_shapes);
}

int NeedDrawRoomBelowCustom(void)
{
    int result;
    if (CustomRoom && CustomRoom->kind == 31) {
        result = 1;
    } else {
        result = 0;
    }
    return result;
}

int RestoreKeepCustomPeel(byte id)
{
    int peel = GetPeelIdxForCharIdx((byte)(id + 100), 1);
    if (peel != -1)
        PeelInfo.items[peel].keep = 0;
    return peel;
}

void SaveCustomRoomPeels(void)
{
    int i;

    if (CustomRoom) {
        for (i = 0; i < CustomRoom->count; ++i) {
            if (CustomRoom->pieces[i].peel == 2) {
                AddCustomPiecePeel(&CustomRoom->pieces[i], &CustomRoom->pieces[i].clip);
            } else if (CustomRoom->pieces[i].peel == 3) {
                AddDsrtStoneUpPeels(&CustomRoom->pieces[i]);
            }
        }
    }
}

void SetCustomRoomTemplate(byte type)
{
    if (type < 35) {
        CustomTemplate = template_table[type];
        return;
    }
    CustomTemplate = &NullPlate;
}

void UnloadCustomRoomData(void)
{
    register unsigned long unload;

    if (CustomRoom) {
        UnloadCustomRoomShapes(1);
        UnloadSounds(CustomRoom->first_sound, CustomRoom->last_sound);
        if ((unload = CustomTemplate->unload.address) != 0)
            ((void (*)(void))unload)();
        _nfree(CustomRoom);
        CustomRoom = 0;
        SetCustomRoomTemplate(36);
    }
}

void UnloadCustomRoomShapes(int all)
{
    CUSTOMPIECE *piece;
    sbyte i;
    int *released;
    word bytes;

    if (CustomRoom) {
        bytes = (CustomRoom->last_shape + 1) * sizeof(Handle);
        released = _nmalloc(bytes);
        memset(released, 0, bytes);
        i = 0;
        if (CustomRoom->count > 0) do {
            piece = &CustomRoom->pieces[i];
            if (piece->handle && (all || (!piece->pass && !piece->mirrored))) {
                if (!released[piece->shape]) {
                    hFree(piece->handle);
                    released[piece->shape] = 1;
                }
                piece->handle = 0;
            }
            if (!all && piece->pass == 11) {
                if (CustomRoom->kind == 33) {
                    if (CustomRoom->count - i - 1 == 0)
                        MarkBufsForRect(MarkMove, &BridgeRect, -1);
                } else {
                    MarkBufsForRect(MarkMove, &piece->clip, -1);
                }
            }
        } while (++i < CustomRoom->count);
        _nfree(released);
    }
}
