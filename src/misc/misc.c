#include "types.h"
#include "level.h"
#pragma pack(1)

typedef struct {
    unsigned int modifier, extra;
} TileInfo;

typedef struct {
    byte tile;
    TileInfo info;
} Tile;

extern sbyte RowNo, ColNo;
extern Rect GamePlayRect, CellClipRect;

extern ROOMREC Room;

extern LEVELREC Level;
typedef int COL_TO_PIXEL_X[13];                       /* DS:0da4 */
extern COL_TO_PIXEL_X Col2PixelX;
typedef int ROW_TO_PIXEL_Y[5];                        /* DS:0dbe */
extern ROW_TO_PIXEL_Y Row2PixelY;
extern Tile Above[], Below[], ScrLeft[], ScrRight[];
extern Tile CellLeft, CellRight;
extern TileInfo BlueSpec;
extern byte ObjId;
extern byte *custom_room;
extern int NewRoom;
extern byte *BlueT;
extern TileInfo *BlueS;

extern int SECTRECT(Rect far *, Rect far *, Rect far *);
extern void OFFSETRECT(Rect far *, Rect far *, int, int);
extern int is_open_tile(byte kind);
extern int IsWall(byte);
extern sbyte FirstInRow(sbyte);
extern void * _cdecl memset(void *, int, unsigned int);
#pragma intrinsic(memset)

sbyte CellToCol(byte room, int cell);
sbyte CellToRow(byte room, int cell);
void ComputeClipRect(Rect far *rect, int col, int row);

void select_room_arrays(byte room)
{
    if (room) {
        BlueT = Level.cell_kinds[room - 1];
        BlueS = (TileInfo *) &Level.cell_info[room - 1][0];
    }
}

sbyte CellToCol(byte room, int cell)
{
    sbyte col = cell % 10;

    if (room == Room.number || room == Room.above || room == Room.below) {
        /* Keep the local column. */
    } else if (col == 9 &&
               (room == Room.left || room == Room.below_left || room == Room.above_left)) {
        col = -1;
    } else if (col == 0 &&
               (room == Room.right || room == Room.below_right || room == Room.above_right)) {
        col = 11;
    } else {
        col = 30;
    }
    return col;
}

sbyte CellToRow(byte room, int cell)
{
    sbyte row = cell / 10;

    if (Room.number == room || Room.left == room || Room.right == room) {
        /* Keep the local row. */
    } else if (Room.above == room && row == 2) {
        row = -1;
    } else if (Room.below == room && row == 0) {
        row = 3;
    } else {
        row = 30;
    }
    return row;
}

void ComputeClipRect(Rect far *rect, int col, int row)
{
    rect->left = Col2PixelX[col + 1];
    rect->right = rect->left + 32;
    rect->bottom = Row2PixelY[row + 1];
    rect->top = rect->bottom - 63;
    SECTRECT(rect, rect, &GamePlayRect);
    if (Room.custom && !row)
        rect->top -= 3;
}

Rect *ComputeOffsetCellRect(Rect *rect, Rect *base, int col, int row)
{
    if (col == 30 || row == 30) {
        rect = 0;
    } else {
        OFFSETRECT(rect, base, col * 32, row * 63);
        if (!SECTRECT(rect, rect, &GamePlayRect))
            rect = 0;
    }
    return rect;
}

byte GetObject(byte room, sbyte col, sbyte row, byte *tile, TileInfo *info, byte missing);

void GetAboveAry(void)
{
    sbyte col, row;
    byte room, missing;
    struct {
        byte saved_below;
        byte side_room;
    } query;
    TileInfo *info;

    if (RowNo != -1) {
        query.saved_below = Room.below;
        if (RowNo == 0) {
            room = Room.above;
            query.side_room = Room.above_left;
            Room.below = Room.number;
        } else {
            room = Room.number;
            query.side_room = Room.left;
        }
        row = RowNo - 1;
        missing = (!Room.custom && Level.environment_kind != 4 && Level.environment_kind != 5) ? 1 : 0;
        select_room_arrays(room);
        for (col = 1; col < 11; ++col) {
            GetObject(room, col - 1, row, &Above[col].tile,
                      info = &Above[col].info, missing);
            if (Room.custom && RowNo == 0 && Level.environment_kind == 3 &&
                is_open_tile(Level.cell_kinds[Room.number - 1][col - 1])) {
                Above[col].info.modifier = (Above[col].info.modifier & 0x3fff) | 0x4000;
            }
        }
        select_room_arrays(query.side_room);
        GetObject(query.side_room, 9, row, &Above[0].tile, &Above[0].info, missing);
        if (custom_room && RowNo == 0)
            Above[0].info.modifier = Above[col].info.modifier | 0xc000;
        select_room_arrays(Room.number);
        Room.below = query.saved_below;
    }
}

void GetBelowAry(void)
{
    sbyte col, row;
    byte room, side_room;

    if (RowNo == 2) {
        room = Room.below;
        side_room = Room.below_left;
        row = 0;
    } else {
        room = Room.number;
        side_room = Room.left;
        row = RowNo + 1;
    }
    select_room_arrays(room);
    for (col = 1; col < 11; ++col)
        GetObject(room, col - 1, row, &Below[col].tile, &Below[col].info, 0);
    select_room_arrays(side_room);
    GetObject(side_room, 9, row, &Below[0].tile, &Below[0].info,
              room ? 20 : 0);
    select_room_arrays(Room.number);
}

byte GetObject(byte room, sbyte col, sbyte row, byte *tile, TileInfo *info, byte missing)
{
    int cell;

    if (row == -1)
        row = 2;
    if (col == -1) {
        *tile = ScrLeft[row].tile;
        *info = ScrLeft[row].info;
    } else if (col == 10) {
        *tile = ScrRight[row].tile;
        *info = ScrRight[row].info;
    } else if (room) {
        cell = FirstInRow(row) + col;
        *tile = BlueT[cell];
        *info = BlueS[cell];
    } else {
        *tile = missing;
        if (Level.environment_kind == 3 && (missing == 1 || missing == 20) && Room.below) {
            if (!IsWall(Level.cell_kinds[Room.below - 1][col])) {
                info->modifier = ((col & 7) < 1 ? 1 : (col & 7)) << 8;
                info->extra = 0;
            } else {
                memset(info, 0, sizeof(*info));
            }
        } else if (Level.environment_kind == 2) {
            info->modifier = 0;
            info->extra = 9;
        } else if (Level.environment_kind == 4) {
            info->modifier = 0;
            info->extra = 0;
        } else {
            info->modifier = 0;
            info->extra = ((col % 2) << 8) | 9;
        }
    }
    return *tile;
}

byte PieceAboveId(sbyte col, sbyte row)
{
    byte room, tile;

    if (row > 0) {
        room = Room.number;
        --row;
    } else {
        room = Room.above;
        row = 2;
    }
    if (col < 0) {
        room = Level.room_links[room - 1].left;
        col += 10;
    } else if (col >= 10) {
        room = Level.room_links[room - 1].right;
        col -= 10;
    }
    if (room)
        tile = Level.cell_kinds[room - 1][FirstInRow(row) + col];
    else
        tile = 1;
    return tile;
}

byte PieceAboveRightId(sbyte col, sbyte row)
{
    byte room, tile;

    if (row > 0) {
        room = Room.number;
        --row;
    } else {
        room = Room.above;
        row = 2;
    }
    if (col < -1) {
        room = Level.room_links[room - 1].left;
        col += 9;
    } else if (col >= 9) {
        room = Level.room_links[room - 1].right;
        col -= 9;
    } else {
        ++col;
    }
    if (room)
        tile = Level.cell_kinds[room - 1][FirstInRow(row) + col];
    else
        tile = 1;
    return tile;
}

byte PieceBelowId(sbyte col, sbyte row)
{
    byte room, tile;

    if (row < 2) {
        room = (row == 0 && RowNo == -1) ? Room.below : Room.number;
        ++row;
    } else {
        row = 0;
        room = Room.below;
    }
    if (col < 0) {
        room = Level.room_links[room - 1].left;
        col += 10;
    } else if (col >= 10) {
        room = Level.room_links[room - 1].right;
        col -= 10;
    }
    if (room)
        tile = Level.cell_kinds[room - 1][FirstInRow(row) + col];
    else
        tile = 20;
    return tile;
}

byte PieceLeftId(sbyte col, sbyte row)
{
    byte tile, room;

    if (col != 0) {
        room = (row == 0 && RowNo == -1) ? Room.below : Room.number;
        --col;
    } else {
        room = (row == -1 && RowNo == -1) ? Room.above_left : Room.left;
        col = 9;
    }
    if (room)
        tile = Level.cell_kinds[room - 1][FirstInRow(row) + col];
    else
        tile = 20;
    return tile;
}

byte PieceRightId(sbyte col, sbyte row)
{
    byte room, tile;

    if (col != 9) {
        room = (row == 0 && RowNo == -1) ? Room.below : Room.number;
        ++col;
    } else {
        room = (row == -1 && RowNo == -1) ? Room.above_right : Room.right;
        col = 0;
    }
    if (row == 3) {
        room = room == Room.right ? Room.below_right : Room.below;
        row = 0;
    }
    if (room)
        tile = Level.cell_kinds[room - 1][FirstInRow(row) + col];
    else
        tile = 20;
    return tile;
}

void SetLocVars(void)
{
    byte missing;

    if ((RowNo != 2 || NewRoom) && RowNo != -1)
        missing = 20;
    else if (Level.environment_kind != 4 && Level.environment_kind != 5)
        missing = 1;
    else
        missing = 0;
    if (NewRoom == 0)
        ComputeClipRect(&CellClipRect, ColNo, RowNo);
    else
        CellClipRect = GamePlayRect;
    GetObject(Room.number, ColNo, RowNo, &ObjId, &BlueSpec, missing);
    GetObject(Room.number, ColNo - 1, RowNo, &CellLeft.tile, &CellLeft.info, missing);
    GetObject(Room.number, ColNo + 1, RowNo, &CellRight.tile, &CellRight.info, missing);
    if (RowNo == -1 && custom_room) {
        BlueSpec.modifier |= 0xc000;
        CellLeft.info.modifier |= 0xc000;
        CellRight.info.modifier |= 0xc000;
    }
}

void SetSideRoomInfo(void)
{
    sbyte row;
    struct {
        TileInfo *info;
        byte missing;
    } state;
    state.missing = Room.number ? 20 : 0;

    select_room_arrays(Room.left);
    for (row = 0; row < 3; ++row) {
        GetObject(Room.left, 9, row, &ScrLeft[row].tile,
                  state.info = &ScrLeft[row].info, state.missing);
        if (custom_room && (custom_room[1] != 31 || row != 2))
            state.info->modifier |= 0xc000;
        else
            state.info->modifier &= 0x3fff;
    }
    select_room_arrays(Room.right);
    for (row = 0; row < 3; ++row)
        GetObject(Room.right, 0, row, &ScrRight[row].tile, &ScrRight[row].info, state.missing);
    select_room_arrays(Room.number);
}

/* The resident relocation sites of both memory handlers name the MISC frame. */

typedef struct {
    unsigned int *handle;
    byte metadata[4];
} ImageEntry;

typedef struct {
    byte header[4];
    int count;
    byte metadata[4];
    ImageEntry image[219];
} ImageTable;

extern ImageTable *Pics[5];
extern int background_image_count;
extern const char OutOfFarMem[];
unsigned int opponent_frames = 0;
static const char fatal_memory_message[] = "Insufficient near Memory.";

extern int query_sound(int);
extern void dispatch_sound(int);
extern void PrinceMarkImgsPurgeable(ImageTable *, int, int);
extern void MARK_FIXED(unsigned int *);
extern void dump_heap(int, const char far *, void *);
extern void abort_with_message(int, const char far *);

int release_discardable_images(void)
{
    int i;

    if (query_sound(0)) {
        dispatch_sound(0);
    } else if (Pics[3] && !(*Pics[3]->image[0].handle & 0x4000)) {
        PrinceMarkImgsPurgeable(Pics[3], 0, Pics[3]->count - 1);
    } else if (Pics[2] && !(*Pics[2]->image[0].handle & 0x4000)) {
        PrinceMarkImgsPurgeable(Pics[2], 0, Pics[2]->count - 1);
        for (i = 131; i <= 132; ++i)
            if (Pics[2]->image[i].handle)
                MARK_FIXED(Pics[2]->image[i].handle);
        for (i = 216; i <= 218; ++i)
            if (Pics[2]->image[i].handle)
                MARK_FIXED(Pics[2]->image[i].handle);
    } else if (Pics[4] && !background_image_count) {
        PrinceMarkImgsPurgeable(Pics[4], 0, Pics[4]->count - 1);
        background_image_count = 1;
    } else {
        dump_heap(0, OutOfFarMem, (void *)0);
        abort_with_message(1, OutOfFarMem);
    }
    return 1;
}

int report_fatal_memory(void)
{
    abort_with_message(1, fatal_memory_message);
    return 1;
}
