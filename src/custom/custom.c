/* DOS CUSTOM.C, resident frame 04b3. Function order follows the DOS image. */
#include "types.h"
#include "level.h"
#include "char.h"
#pragma pack(1)

typedef unsigned int ShapeHandle;
typedef struct { int x, y; } CustomPosition;

typedef struct {
    signed char shape;
    CustomPosition position;
    unsigned char pass, palette;
    int resource;
    ShapeHandle handle;
    Rect rect;
    unsigned char flags, peel_type;
    char tail[4];
} CustomPiece;

typedef struct {
    signed char count;
    unsigned char template_id;
    int shape_base, last_shape, reserved;
    int sound_first, sound_last;
    char header_tail[16];
    CustomPiece pieces[1];
} CUSTOMROOM;

typedef struct {
    unsigned char flags;
    unsigned int background, foreground;
} AboveCell;
CUSTOMROOM *CustomRoom = 0;

typedef struct {
    unsigned char sheet;
    signed char column;
    unsigned char flags;
    signed char row;
    ShapeHandle handle;
} PieceShapeInfo;

typedef union {
    void (far pascal *call)(void);
    unsigned long address;
} RoomCallback;

typedef struct {
    RoomCallback load, unload;
    unsigned char reserved[20];
} RoomTemplate;

typedef struct {
    unsigned int width, height;
} ShapeSize;

typedef struct {
    char region[4];
    int keep;
} PeelRecord;
#pragma pack()

extern LEVELREC Level;
extern ROOMREC Room;
extern struct {
    unsigned char kind, reserved, separate_mask, palette;
    ShapeHandle handle;
} HorseShpInfo;
/* Rect passed to StrokePortRect for the style-4 border. */
extern const int TopRowRect[4];
extern AboveCell Above[];
extern struct {
    int count;
    PeelRecord entries[40];
} PeelInfo;
extern void far pascal LoadCarpetCode(void);
extern void far pascal UnloadCarpetCode(void);
extern void far pascal LoadBridgeCode(void);
extern void far pascal UnloadBridgeCode(void);
extern void far pascal LoadDsrtCode(void);
extern void far pascal LoadRNThroneFunc(void);
extern void far pascal UnloadRNThroneFunc(void);
extern void far pascal LoadRNStart6(void);
extern void far pascal UnloadRNStart6(void);
extern void far pascal LoadPlateauRoomFunc(void);
extern void far pascal LoadAltarRoomFunc(void);
extern void far pascal UnloadAltarRoomFunc(void);
extern void far pascal LoadInscriptionRoomFunc(void);
extern void far pascal LoadChess(void);
extern void far pascal LoadCrystal(void);
extern void far pascal LoadTowerTop(void);

/* The reserved fields are zero in each 28-byte DOS template record. */
#define UNUSED_TEMPLATE_FIELDS { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }

RoomTemplate CVCarpetPlate = { { LoadCarpetCode }, { UnloadCarpetCode }, UNUSED_TEMPLATE_FIELDS };
RoomTemplate CVBridgePlate = { { LoadBridgeCode }, { UnloadBridgeCode }, UNUSED_TEMPLATE_FIELDS };
RoomTemplate DSExitPlate = { { LoadDsrtCode }, { 0 }, UNUSED_TEMPLATE_FIELDS };
RoomTemplate RNThronePlate = { { LoadRNThroneFunc }, { UnloadRNThroneFunc }, UNUSED_TEMPLATE_FIELDS };
RoomTemplate RNStart6Plate = { { LoadRNStart6 }, { UnloadRNStart6 }, UNUSED_TEMPLATE_FIELDS };
RoomTemplate RNPlateauPlate = { { LoadPlateauRoomFunc }, { 0 }, UNUSED_TEMPLATE_FIELDS };
RoomTemplate TMAltarPlate = { { LoadAltarRoomFunc }, { UnloadAltarRoomFunc }, UNUSED_TEMPLATE_FIELDS };
RoomTemplate TMInscriptionPlate = { { LoadInscriptionRoomFunc }, { 0 }, UNUSED_TEMPLATE_FIELDS };
RoomTemplate FNChessPlate = { { LoadChess }, { 0 }, UNUSED_TEMPLATE_FIELDS };
RoomTemplate FNCrystalPlate = { { LoadCrystal }, { 0 }, UNUSED_TEMPLATE_FIELDS };
RoomTemplate FNTowerTopPlate = { { LoadTowerTop }, { 0 }, UNUSED_TEMPLATE_FIELDS };
RoomTemplate NullPlate = { { 0 }, { 0 }, UNUSED_TEMPLATE_FIELDS };

#undef UNUSED_TEMPLATE_FIELDS

static RoomTemplate *template_table[] = {
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

RoomTemplate *CustomTemplate = &NullPlate;
extern unsigned int *Pics[5];
extern Rect NullRect;
extern Rect BridgeRect;
extern void far MarkMove(void);

extern void far _fastcall AddPeel(int left, int right, int top, int height,
                                   unsigned char id, unsigned char kind);
extern int GetPeelIdxForCharIdx(unsigned char id, int restore);
extern void EraseRect(const int far *rect);
extern void RoofCheckRoomSpecific(void);
extern void hFree(ShapeHandle handle);
extern void OFFSETRECT(Rect far *dst, Rect far *src, int dy, int dx);
extern void far _fastcall DrawCustomRoomImg(char index, char pass);
extern void far *MATERIALIZE_HANDLE(ShapeHandle handle);
extern void far *h2p(ShapeHandle handle);
extern unsigned int HGETRSRC(unsigned long type, int id);
extern void *nGetRsrc(unsigned long type, int id);
extern void far *hLock(unsigned int archive);
extern void hUnlock(unsigned int archive);
extern void RELOCATE_HANDLE_PAYLOAD(unsigned int archive);
extern int hGetState(unsigned int archive);
extern void hSetState(unsigned int archive, int position);
extern ShapeHandle PrinceUnpackShape(unsigned int resource, int format, void far *archive);
extern void LoadSounds(int first, int last);
extern void UnloadSounds(int first, int last);
extern void MAKESHAPERECT(Rect far *rect, int x, int y, void far *shape);
extern int EMPTYRECT(Rect far *);
extern void SECTRECT(Rect far *, Rect far *, Rect far *);
extern void AddDsrtStoneUpPeels(CustomPiece *);
extern void MarkBufsForRect(void (far *image)(void), Rect *rect, char pass);
extern void * _cdecl _nmalloc(unsigned int bytes);
extern void _cdecl _nfree(void *);
extern void * _cdecl memset(void *, int, unsigned int);
#pragma intrinsic(memset)

void LoadCustomRoomShapes(CUSTOMROOM *room);
void UnloadCustomRoomShapes(int all);
void UnloadCustomRoomData(void);
void SetCustomRoomTemplate(unsigned char type);

void AddCustomPiecePeel(CustomPiece *piece, Rect far *rect)
{
    AddPeel(rect->left, rect->right, rect->top, rect->bottom - rect->top,
             (unsigned char)(piece->shape + 100), piece->peel_type);
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
    if (HorseShpInfo.handle) {
        hFree(HorseShpInfo.handle);
        HorseShpInfo.handle = 0;
    }
}

CustomPiece *CustomPieceOffsetToColRow(CustomPiece *piece, signed char column, signed char row);

void DrawCustomRoomImgAtInfo(unsigned char index, signed char *info)
{
    CustomPiece *piece = &CustomRoom->pieces[index];
    Rect saved_rect;
    CustomPosition saved_position;
    unsigned char saved_pass;

    saved_rect = piece->rect;
    saved_position = piece->position;
    piece = CustomPieceOffsetToColRow(piece, info[1], info[2]);
    saved_pass = piece->pass;
    piece->pass = info[0];
    DrawCustomRoomImg(index, info[0]);
    piece->position = saved_position;
    piece->pass = saved_pass;
    piece->rect = saved_rect;
}

CustomPiece *CustomPieceOffsetToColRow(CustomPiece *piece, signed char column, signed char row)
{
    int dx = (row + 1) * 63;
    int dy;

    piece->position.x += dx;
    dy = column * 32;
    piece->position.y += dy;
    OFFSETRECT(&piece->rect, &piece->rect, dy, dx);
    return piece;
}

void GetCustomAboveAry(void)
{
    signed char i = 0;
    do {
        Above[i].flags = 0;
        Above[i].background = 0xC000;
        Above[i].foreground = 0;
        ++i;
    } while (i < 11);
}

void far *GetCustomPieceShape(CustomPiece *piece)
{
    typedef struct {
        unsigned int resource;
        void far *archive;
    } ShapeLookup;
    ShapeLookup lookup;

    if (!piece->handle || !MATERIALIZE_HANDLE(piece->handle)) {
        if (piece->handle)
            hFree(piece->handle);
        piece->handle = PrinceUnpackShape(
            lookup.resource = HGETRSRC(0x53484150UL,
                                            piece->shape + CustomRoom->shape_base),
            2, hLock(*Pics[4]));
        hUnlock(*Pics[4]);
    }
    return h2p(piece->handle);
}

PieceShapeInfo *far _fastcall GetCustomPieceShpInfo(int index, PieceShapeInfo *info)
{
    if (CustomRoom) {
        char zero = 0;
        info->sheet = 1;
        info->column = zero;
        info->row = zero;
        info->handle = CustomRoom->pieces[index].handle;
    } else {
        info = 0;
    }
    return info;
}

ShapeSize far *GetCustomPieceSize(unsigned char index, ShapeSize far *size)
{
    ShapeSize far *shape;

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

int IsCustomizedRoom(unsigned char room)
{
    LEVELCELL *tiles = Level.cell_info[room - 1];
    unsigned int flags = 0;
    signed char tile = 0;

    do {
        if (flags)
            break;
        flags = tiles[tile].modifier & 0xC000;
        ++tile;
    } while (tile < 30);
    return flags != 0;
}

void LoadCustomRoomData(unsigned char room_number_arg)
{
    CUSTOMROOM *room;
    register unsigned long load;

    if (Room.custom) {
        room = nGetRsrc(0x43555354UL, (room_number_arg + 159) * 25);
        if (room) {
            if (CustomRoom && room->template_id == CustomRoom->template_id) {
                /* The DOS routine repeats the template comparison on this path. */
                if (room->template_id == CustomRoom->template_id) {
                    LoadCustomRoomShapes(room);
                    UnloadCustomRoomShapes(1);
                    _nfree(CustomRoom);
                    CustomRoom = room;
                }
            } else {
                LoadCustomRoomShapes(room);
                UnloadCustomRoomData();
                CustomRoom = room;
                LoadSounds(room->sound_first, room->sound_last);
                SetCustomRoomTemplate(CustomRoom->template_id);
            }
            if ((load = CustomTemplate->load.address) != 0)
                ((void (far *)(void))load)();
        }
    } else if (CustomRoom) {
        UnloadCustomRoomData();
        if (((Level.level_number) == 5 && Room.number == 7) ||
            ((Level.level_number) == 5 && Room.number == 12))
            LoadBridgeCode();
    }
}

void LoadCustomRoomShapes(CUSTOMROOM *room)
{
    int i, old_count, bytes, shape_base, position;

    ShapeHandle *old_shapes = 0, *new_shapes;
    void far *archive;
    void far *bitmap;
    CustomPiece *piece;
    Rect rect;

    if (CustomRoom) {
        if (CustomRoom->shape_base == (shape_base = room->shape_base)) {
            old_count = 1 + (CustomRoom->last_shape >= room->last_shape ?
                             CustomRoom->last_shape : room->last_shape);
            bytes = old_count * sizeof(ShapeHandle);
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
        if (CustomRoom->template_id != room->template_id)
            UnloadCustomRoomData();
    }

    shape_base = room->shape_base;
    position = hGetState(*Pics[4]);
    RELOCATE_HANDLE_PAYLOAD(*Pics[4]);
    archive = hLock(*Pics[4]);
    bytes = (room->last_shape + 1) * sizeof(ShapeHandle);
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
                        unsigned int resource = HGETRSRC(0x53484150UL, piece->shape + shape_base);
                        new_shapes[piece->shape] = PrinceUnpackShape(resource, 2, archive);
                    }
                }
                piece->handle = new_shapes[piece->shape];
                bitmap = h2p(piece->handle);
                MAKESHAPERECT(&rect, piece->position.x, piece->position.y, bitmap);
                if (EMPTYRECT(&piece->rect)) {
                    piece->rect = rect;
                } else {
                    OFFSETRECT(&piece->rect, &piece->rect, piece->position.y, piece->position.x);
                    SECTRECT(&piece->rect, &piece->rect, &rect);
                }
                piece->resource = i + shape_base;
            } else {
                piece->handle = 0;
                piece->rect = NullRect;
                piece->resource = -1;
            }
        } while (++i < room->count);
    }
    hSetState(*Pics[4], position);
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
    if (CustomRoom && CustomRoom->template_id == 31) {
        result = 1;
    } else {
        result = 0;
    }
    return result;
}

int RestoreKeepCustomPeel(unsigned char id)
{
    int peel = GetPeelIdxForCharIdx((unsigned char)(id + 100), 1);
    if (peel != -1)
        PeelInfo.entries[peel].keep = 0;
    return peel;
}

void SaveCustomRoomPeels(void)
{
    int i;

    if (CustomRoom) {
        for (i = 0; i < CustomRoom->count; ++i) {
            if (CustomRoom->pieces[i].peel_type == 2) {
                AddCustomPiecePeel(&CustomRoom->pieces[i], &CustomRoom->pieces[i].rect);
            } else if (CustomRoom->pieces[i].peel_type == 3) {
                AddDsrtStoneUpPeels(&CustomRoom->pieces[i]);
            }
        }
    }
}

void SetCustomRoomTemplate(unsigned char type)
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
        UnloadSounds(CustomRoom->sound_first, CustomRoom->sound_last);
        if ((unload = CustomTemplate->unload.address) != 0)
            ((void (far *)(void))unload)();
        _nfree(CustomRoom);
        CustomRoom = 0;
        SetCustomRoomTemplate(36);
    }
}

void UnloadCustomRoomShapes(int all)
{
    CustomPiece *piece;
    signed char i;
    ShapeHandle *released;
    unsigned int bytes;

    if (CustomRoom) {
        bytes = (CustomRoom->last_shape + 1) * sizeof(ShapeHandle);
        released = _nmalloc(bytes);
        memset(released, 0, bytes);
        i = 0;
        if (CustomRoom->count > 0) do {
            piece = &CustomRoom->pieces[i];
            if (piece->handle && (all || (!piece->pass && !piece->flags))) {
                if (!released[piece->shape]) {
                    hFree(piece->handle);
                    released[piece->shape] = 1;
                }
                piece->handle = 0;
            }
            if (!all && piece->pass == 11) {
                if (CustomRoom->template_id == 33) {
                    if (CustomRoom->count - i - 1 == 0)
                        MarkBufsForRect(MarkMove, &BridgeRect, -1);
                } else {
                    MarkBufsForRect(MarkMove, &piece->rect, -1);
                }
            }
        } while (++i < CustomRoom->count);
        _nfree(released);
    }
}
