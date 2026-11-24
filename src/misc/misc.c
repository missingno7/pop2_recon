#pragma pack(1)
typedef unsigned char byte;
typedef signed char sbyte;

typedef struct {
    int left, top, right, bottom;
} Rect;

typedef struct {
    unsigned int modifier, extra;
} TileInfo;

typedef struct {
    byte tile;
    TileInfo info;
} Tile;

typedef struct {
    unsigned char number;                             /* 00 */
    unsigned char left, right, above, below;           /* 01-04 */
    unsigned char above_left, above_right;             /* 05-06 */
    unsigned char below_left, below_right;             /* 07-08 */
    unsigned char custom;                             /* 09 */
} ROOMREC;
extern sbyte view_row, view_col;
extern Rect GamePlayRect, cell_clip;
typedef struct {
    unsigned int modifier, unknown_02;
} LEVELCELL;

typedef struct {
    unsigned char left, right, above, below;
} ROOMLINKS;

extern ROOMREC Room;
typedef struct {
    signed char opponent_count;
    unsigned char opponent_data[5][23];
} ROOMOBJECTS;

typedef struct {
    signed char room, cell;
} REBIRTHPOINT;

typedef struct {
    signed char count;
    unsigned char spawn_flags;
    unsigned char header[2];
    unsigned char points[3][10];
} ROOMGENERATORS;

typedef struct {
    unsigned char cell_kinds[32][30];                  /* 0000 */
    LEVELCELL cell_info[32][30];                       /* 03c0 */
    unsigned char unknown_12c0[0x500];                 /* 12c0 */
    ROOMLINKS room_links[32];                          /* 17c0 */
    unsigned char room_count;                         /* 1840 */
    unsigned char unknown_1841[4];
    unsigned char environment_kind;                  /* 1845 */
    unsigned char unknown_1846;
    signed char level_number;                         /* 1847 */
    int background_overlays[8];                       /* 1848 */
    unsigned char unknown_1858[4];
    int actor_phase;                                  /* 185c */
    unsigned char unknown_185e[2];
    signed char entry_room, entry_cell, entry_facing;    /* 1860-1862 */
    unsigned char unknown_1863[3];
    unsigned char preferred_opponent;                /* 1866 */
    ROOMOBJECTS room_objects[32];                      /* 1867 */
    REBIRTHPOINT rebirth_points[2];                    /* 26e7 */
    unsigned char unknown_26eb[0x0e];
    ROOMGENERATORS generators[32];                     /* 26f9 */
    unsigned char unknown_2b39[0x3c0];                 /* 2b39-2ef8 */
} LEVELREC;
extern LEVELREC Level;
typedef int COL_TO_PIXEL_X[13];                       /* DS:0da4 */
extern COL_TO_PIXEL_X Col2PixelX;
typedef int ROW_TO_PIXEL_Y[5];                        /* DS:0dbe */
extern ROW_TO_PIXEL_Y Row2PixelY;
extern Tile above_tiles[], below_tiles[], left_tiles[], right_tiles[];
extern Tile CellLeft, CellRight;
extern TileInfo BlueSpec;
extern byte drawn_tile_kind;
extern byte *tile_override;
extern int force_full_clip;
extern byte room_tiles[][30];
extern byte *room_kinds;
extern TileInfo *BlueS;

extern int SECTRECT(Rect far *, Rect far *, Rect far *);
extern void OFFSETRECT(Rect far *, Rect far *, int, int);
extern int is_open_tile(byte);
extern int tile_has_side_edge(byte);
extern sbyte FirstInRow(sbyte);
extern void * _cdecl memset(void *, int, unsigned int);
#pragma intrinsic(memset)

void select_room_arrays(byte room)
{
    if (room) {
        room_kinds = room_tiles[room];
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
    rect->top = Col2PixelX[col + 1];
    rect->bottom = rect->top + 32;
    rect->right = Row2PixelY[row + 1];
    rect->left = rect->right - 63;
    SECTRECT(rect, rect, &GamePlayRect);
    if (Room.custom && !row)
        rect->left -= 3;
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

    if (view_row != -1) {
        query.saved_below = Room.below;
        if (view_row == 0) {
            room = Room.above;
            query.side_room = Room.above_left;
            Room.below = Room.number;
        } else {
            room = Room.number;
            query.side_room = Room.left;
        }
        row = view_row - 1;
        missing = (!Room.custom && Level.environment_kind != 4 && Level.environment_kind != 5) ? 1 : 0;
        select_room_arrays(room);
        for (col = 1; col < 11; ++col) {
            GetObject(room, col - 1, row, &above_tiles[col].tile,
                      info = &above_tiles[col].info, missing);
            if (Room.custom && view_row == 0 && Level.environment_kind == 3 &&
                is_open_tile(room_tiles[Room.number][col - 1])) {
                above_tiles[col].info.modifier = (above_tiles[col].info.modifier & 0x3fff) | 0x4000;
            }
        }
        select_room_arrays(query.side_room);
        GetObject(query.side_room, 9, row, &above_tiles[0].tile, &above_tiles[0].info, missing);
        if (tile_override && view_row == 0)
            above_tiles[0].info.modifier = above_tiles[col].info.modifier | 0xc000;
        select_room_arrays(Room.number);
        Room.below = query.saved_below;
    }
}

void GetBelowAry(void)
{
    sbyte col, row;
    byte room, side_room;

    if (view_row == 2) {
        room = Room.below;
        side_room = Room.below_left;
        row = 0;
    } else {
        room = Room.number;
        side_room = Room.left;
        row = view_row + 1;
    }
    select_room_arrays(room);
    for (col = 1; col < 11; ++col)
        GetObject(room, col - 1, row, &below_tiles[col].tile, &below_tiles[col].info, 0);
    select_room_arrays(side_room);
    GetObject(side_room, 9, row, &below_tiles[0].tile, &below_tiles[0].info,
              room ? 20 : 0);
    select_room_arrays(Room.number);
}

byte GetObject(byte room, sbyte col, sbyte row, byte *tile, TileInfo *info, byte missing)
{
    int cell;

    if (row == -1)
        row = 2;
    if (col == -1) {
        *tile = left_tiles[row].tile;
        *info = left_tiles[row].info;
    } else if (col == 10) {
        *tile = right_tiles[row].tile;
        *info = right_tiles[row].info;
    } else if (room) {
        cell = FirstInRow(row) + col;
        *tile = room_kinds[cell];
        *info = BlueS[cell];
    } else {
        *tile = missing;
        if (Level.environment_kind == 3 && (missing == 1 || missing == 20) && Room.below) {
            if (!tile_has_side_edge(room_tiles[Room.below][col])) {
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
        tile = room_tiles[room][FirstInRow(row) + col];
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
        tile = room_tiles[room][FirstInRow(row) + col];
    else
        tile = 1;
    return tile;
}

byte PieceBelowId(sbyte col, sbyte row)
{
    byte room, tile;

    if (row < 2) {
        room = (row == 0 && view_row == -1) ? Room.below : Room.number;
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
        tile = room_tiles[room][FirstInRow(row) + col];
    else
        tile = 20;
    return tile;
}

byte PieceLeftId(sbyte col, sbyte row)
{
    byte tile, room;

    if (col != 0) {
        room = (row == 0 && view_row == -1) ? Room.below : Room.number;
        --col;
    } else {
        room = (row == -1 && view_row == -1) ? Room.above_left : Room.left;
        col = 9;
    }
    if (room)
        tile = room_tiles[room][FirstInRow(row) + col];
    else
        tile = 20;
    return tile;
}

byte PieceRightId(sbyte col, sbyte row)
{
    byte room, tile;

    if (col != 9) {
        room = (row == 0 && view_row == -1) ? Room.below : Room.number;
        ++col;
    } else {
        room = (row == -1 && view_row == -1) ? Room.above_right : Room.right;
        col = 0;
    }
    if (row == 3) {
        room = room == Room.right ? Room.below_right : Room.below;
        row = 0;
    }
    if (room)
        tile = room_tiles[room][FirstInRow(row) + col];
    else
        tile = 20;
    return tile;
}

void SetLocVars(void)
{
    byte missing;

    if ((view_row != 2 || force_full_clip) && view_row != -1)
        missing = 20;
    else if (Level.environment_kind != 4 && Level.environment_kind != 5)
        missing = 1;
    else
        missing = 0;
    if (force_full_clip == 0)
        ComputeClipRect(&cell_clip, view_col, view_row);
    else
        cell_clip = GamePlayRect;
    GetObject(Room.number, view_col, view_row, &drawn_tile_kind, &BlueSpec, missing);
    GetObject(Room.number, view_col - 1, view_row, &CellLeft.tile, &CellLeft.info, missing);
    GetObject(Room.number, view_col + 1, view_row, &CellRight.tile, &CellRight.info, missing);
    if (view_row == -1 && tile_override) {
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
        GetObject(Room.left, 9, row, &left_tiles[row].tile,
                  state.info = &left_tiles[row].info, state.missing);
        if (tile_override && (tile_override[1] != 31 || row != 2))
            state.info->modifier |= 0xc000;
        else
            state.info->modifier &= 0x3fff;
    }
    select_room_arrays(Room.right);
    for (row = 0; row < 3; ++row)
        GetObject(Room.right, 0, row, &right_tiles[row].tile, &right_tiles[row].info, state.missing);
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

extern ImageTable *first_image_table;
extern ImageTable *second_image_table;
extern ImageTable *background_shapes;
extern int fallback_images_released;
extern const char low_memory_message[];
static const char fatal_memory_message[] = "Insufficient near Memory.";

extern int query_sound(int);
extern void dispatch_sound(int);
extern void PrinceMarkImgsPurgeable(ImageTable *, int, int);
extern void MARK_FIXED(unsigned int *);
extern void save_cleanup_state(int, const char far *, void *);
extern void abort_with_message(int, const char far *);

int release_discardable_images(void)
{
    int i;

    if (query_sound(0)) {
        dispatch_sound(0);
    } else if (first_image_table && !(*first_image_table->image[0].handle & 0x4000)) {
        PrinceMarkImgsPurgeable(first_image_table, 0, first_image_table->count - 1);
    } else if (second_image_table && !(*second_image_table->image[0].handle & 0x4000)) {
        PrinceMarkImgsPurgeable(second_image_table, 0, second_image_table->count - 1);
        for (i = 131; i <= 132; ++i)
            if (second_image_table->image[i].handle)
                MARK_FIXED(second_image_table->image[i].handle);
        for (i = 216; i <= 218; ++i)
            if (second_image_table->image[i].handle)
                MARK_FIXED(second_image_table->image[i].handle);
    } else if (background_shapes && !fallback_images_released) {
        PrinceMarkImgsPurgeable(background_shapes, 0, background_shapes->count - 1);
        fallback_images_released = 1;
    } else {
        save_cleanup_state(0, low_memory_message, (void *)0);
        abort_with_message(1, low_memory_message);
    }
    return 1;
}

int report_fatal_memory(void)
{
    abort_with_message(1, fatal_memory_message);
    return 1;
}
