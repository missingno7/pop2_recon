#pragma pack(1)
typedef unsigned char byte;
typedef signed char sbyte;

typedef struct {
    int left, top, right, bottom;
} Rect;

typedef union {
    unsigned long value;
    struct {
        unsigned int low, high;
    } words;
    byte bytes[4];
} TileInfo;

typedef struct {
    byte tile;
    TileInfo info;
} Tile;

typedef struct {
    byte left, right, above, below;
} RoomLinks;

extern byte below_left_room, below_right_room, above_left_room, above_right_room;
extern byte clip_left_edge;
extern sbyte view_row, view_col;
extern byte level_style;
extern int row_y[], col_x[];
extern Rect screen_clip, cell_clip;
extern RoomLinks room_links[];
extern Tile above_tiles[], below_tiles[], left_tiles[], right_tiles[];
extern Tile left_tile, right_tile;
extern TileInfo current_tile_info;
extern byte current_tile;
extern byte *tile_override;
extern int force_full_clip;
extern byte room_tiles[][30];
extern TileInfo room_info[][30];
extern unsigned int selected_room;
extern byte *selected_tiles;
extern TileInfo *selected_info;
extern byte view_room, left_room, right_room, above_room, below_room;

extern int intersect_rect(Rect far *, Rect far *, Rect far *);
extern void offset_rect(Rect far *, Rect far *, int, int);
extern int tile_has_upper_edge(byte);
extern int tile_has_side_edge(byte);
extern sbyte row_to_cell(sbyte);
extern void * _cdecl memset(void *, int, unsigned int);
#pragma intrinsic(memset)

void select_room_arrays(byte room)
{
    volatile int index;
    index = room;
    selected_room = room;
    if (room != 0) {
        selected_tiles = (byte *)room_tiles + 30 * index;
        selected_info = (TileInfo *)((byte *)room_info + 120 * index);
    }
}

sbyte CellToCol(byte room, int cell)
{
    sbyte col = cell % 10;

    if (room == view_room || room == above_room || room == below_room) {
        /* Keep the local column. */
    } else if (col == 9 &&
               (room == left_room || room == above_left_room || room == below_left_room)) {
        col = -1;
    } else if (col == 0 &&
               (room == right_room || room == above_right_room || room == below_right_room)) {
        col = 11;
    } else {
        col = 30;
    }
    return col;
}

sbyte CellToRow(byte room, int cell)
{
    sbyte row = cell / 10;

    if (room == view_room || room == left_room || room == right_room) {
        /* Keep the local row. */
    } else if (room == above_room && row == 2) {
        row = -1;
    } else if (room == below_room && row == 0) {
        row = 3;
    } else {
        row = 30;
    }
    return row;
}

void ComputeClipRect(Rect far *rect, int col, int row)
{
    rect->top = row_y[col];
    rect->bottom = rect->top + 32;
    rect->right = col_x[row];
    rect->left = rect->right - 63;
    intersect_rect(rect, rect, &screen_clip);
    if (clip_left_edge && !row)
        rect->left -= 3;
}

Rect *ComputeOffsetCellRect(Rect *rect, Rect *base, int col, int row)
{
    if (col == 30 || row == 30) {
        rect = 0;
    } else {
        offset_rect(rect, base, col * 32, row * 63);
        if (!intersect_rect(rect, rect, &screen_clip))
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
        query.saved_below = below_room;
        if (view_row == 0) {
            room = above_room;
            query.side_room = below_left_room;
            below_room = view_room;
        } else {
            room = view_room;
            query.side_room = left_room;
        }
        row = view_row - 1;
        missing = (!clip_left_edge && level_style != 4 && level_style != 5) ? 1 : 0;
        select_room_arrays(room);
        for (col = 1; col < 11; ++col) {
            GetObject(room, col - 1, row, &above_tiles[col].tile,
                      info = &above_tiles[col].info, missing);
            if (clip_left_edge && view_row == 0 && level_style == 3 &&
                tile_has_upper_edge(room_tiles[view_room][col - 1])) {
                TileInfo *field = *(TileInfo * volatile *)&info;
                field->words.low = (field->words.low & 0x3fff) | 0x4000;
            }
        }
        select_room_arrays(query.side_room);
        GetObject(query.side_room, 9, row, &above_tiles[0].tile, &above_tiles[0].info, missing);
        if (tile_override && view_row == 0)
            above_tiles[0].info.words.low = above_tiles[col].info.words.low | 0xc000;
        select_room_arrays(view_room);
        below_room = query.saved_below;
    }
}

void GetBelowAry(void)
{
    sbyte col, row;
    byte room, side_room;

    if (view_row == 2) {
        room = below_room;
        side_room = above_left_room;
        row = 0;
    } else {
        room = view_room;
        side_room = left_room;
        row = view_row + 1;
    }
    select_room_arrays(room);
    for (col = 1; col < 11; ++col)
        GetObject(room, col - 1, row, &below_tiles[col].tile, &below_tiles[col].info, 0);
    select_room_arrays(side_room);
    GetObject(side_room, 9, row, &below_tiles[0].tile, &below_tiles[0].info,
              room ? 20 : 0);
    select_room_arrays(view_room);
}

byte GetObject(byte room, sbyte col, sbyte row, byte *tile, TileInfo *info, byte missing)
{
    int cell;

    if (row == -1)
        row = 2;
    if (col == -1) {
        *tile = left_tiles[row].tile;
        info->value = left_tiles[row].info.value;
    } else if (col == 10) {
        *tile = right_tiles[row].tile;
        info->value = right_tiles[row].info.value;
    } else if (room) {
        cell = row_to_cell(row) + col;
        *tile = selected_tiles[cell];
        info->value = selected_info[cell].value;
    } else {
        *tile = missing;
        if (level_style == 3 && (missing == 1 || missing == 20) && below_room) {
            if (!tile_has_side_edge(room_tiles[below_room][col])) {
                info->words.low = ((col & 7) < 1 ? 1 : (col & 7)) << 8;
                info->words.high = 0;
            } else {
                memset(info, 0, sizeof(*info));
            }
        } else if (level_style == 2) {
            info->words.low = 0;
            info->words.high = 9;
        } else if (level_style == 4) {
            info->words.low = 0;
            info->words.high = 0;
        } else {
            info->words.low = 0;
            info->words.high = ((col % 2) << 8) | 9;
        }
    }
    return *tile;
}

byte PieceAboveId(sbyte col, sbyte row)
{
    byte room, tile;

    if (row > 0) {
        room = view_room;
        --row;
    } else {
        room = above_room;
        row = 2;
    }
    if (col < 0) {
        room = room_links[room].left;
        col += 10;
    } else if (col >= 10) {
        room = room_links[room].right;
        col -= 10;
    }
    if (room)
        tile = room_tiles[room][row_to_cell(row) + col];
    else
        tile = 1;
    return tile;
}

byte PieceAboveRightId(sbyte col, sbyte row)
{
    byte room, tile;

    if (row > 0) {
        room = view_room;
        --row;
    } else {
        room = above_room;
        row = 2;
    }
    if (col < -1) {
        room = room_links[room].left;
        col += 9;
    } else if (col >= 9) {
        room = room_links[room].right;
        col -= 9;
    } else {
        ++col;
    }
    if (room)
        tile = room_tiles[room][row_to_cell(row) + col];
    else
        tile = 1;
    return tile;
}

byte PieceBelowId(sbyte col, sbyte row)
{
    byte room, tile;

    if (row < 2) {
        room = (row == 0 && view_row == -1) ? below_room : view_room;
        ++row;
    } else {
        row = 0;
        room = below_room;
    }
    if (col < 0) {
        room = room_links[room].left;
        col += 10;
    } else if (col >= 10) {
        room = room_links[room].right;
        col -= 10;
    }
    if (room)
        tile = room_tiles[room][row_to_cell(row) + col];
    else
        tile = 20;
    return tile;
}

byte PieceLeftId(sbyte col, sbyte row)
{
    byte tile, room;

    if (col != 0) {
        room = (row == 0 && view_row == -1) ? below_room : view_room;
        --col;
    } else {
        room = (row == -1 && view_row == -1) ? below_left_room : left_room;
        col = 9;
    }
    if (room)
        tile = room_tiles[room][row_to_cell(row) + col];
    else
        tile = 20;
    return tile;
}

byte PieceRightId(sbyte col, sbyte row)
{
    byte room, tile;

    if (col != 9) {
        room = (row == 0 && view_row == -1) ? below_room : view_room;
        ++col;
    } else {
        room = (row == -1 && view_row == -1) ? below_right_room : right_room;
        col = 0;
    }
    if (row == 3) {
        room = room == right_room ? above_right_room : below_room;
        row = 0;
    }
    if (room)
        tile = room_tiles[room][row_to_cell(row) + col];
    else
        tile = 20;
    return tile;
}

void SetLocVars(void)
{
    byte missing;

    if ((view_row != 2 || force_full_clip) && view_row != -1)
        missing = 20;
    else if (level_style != 4 && level_style != 5)
        missing = 1;
    else
        missing = 0;
    if (force_full_clip == 0)
        ComputeClipRect(&cell_clip, view_col, view_row);
    else
        cell_clip = screen_clip;
    GetObject(view_room, view_col, view_row, &current_tile, &current_tile_info, missing);
    GetObject(view_room, view_col - 1, view_row, &left_tile.tile, &left_tile.info, missing);
    GetObject(view_room, view_col + 1, view_row, &right_tile.tile, &right_tile.info, missing);
    if (view_row == -1 && tile_override) {
        current_tile_info.bytes[1] |= 0xc0;
        left_tile.info.bytes[1] |= 0xc0;
        right_tile.info.bytes[1] |= 0xc0;
    }
}

void SetSideRoomInfo(void)
{
    sbyte row;
    byte missing = view_room ? 20 : 0;
    TileInfo *info;

    select_room_arrays(left_room);
    for (row = 0; row < 3; ++row) {
        GetObject(left_room, 9, row, &left_tiles[row].tile,
                  info = &left_tiles[row].info, missing);
        if (tile_override && (tile_override[1] != 31 || row != 2))
            info->bytes[1] |= 0xc0;
        else
            info->bytes[1] &= 0x3f;
    }
    select_room_arrays(right_room);
    for (row = 0; row < 3; ++row)
        GetObject(right_room, 0, row, &right_tiles[row].tile, &right_tiles[row].info, missing);
    select_room_arrays(view_room);
}

/* The next two DOS entries have no witnessed callers. Their file membership is
   unresolved, but their original order and operation are visible. */

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

extern ImageTable * volatile first_image_table;
extern ImageTable * volatile second_image_table;
extern ImageTable * volatile fallback_image_table;
extern int fallback_images_released;
extern const char low_memory_message[];
extern const char fatal_memory_message[];

extern int resource_can_purge(int);
extern void purge_resource(int);
extern void mark_images_purgeable(ImageTable *, int, int);
extern void mark_image_purgeable(unsigned int *);
extern void save_cleanup_state(int, const char far *, int);
extern void shutdown_with_message(int, const char far *);

int release_discardable_images(void)
{
    int i;

    if (resource_can_purge(0)) {
        purge_resource(0);
    } else if (first_image_table && !(*first_image_table->image[0].handle & 0x4000)) {
        mark_images_purgeable(first_image_table, 0, first_image_table->count - 1);
    } else if (second_image_table && !(*second_image_table->image[0].handle & 0x4000)) {
        mark_images_purgeable(second_image_table, 0, second_image_table->count - 1);
        for (i = 131; i <= 132; ++i)
            if (second_image_table->image[i].handle)
                mark_image_purgeable(second_image_table->image[i].handle);
        for (i = 216; i <= 218; ++i)
            if (second_image_table->image[i].handle)
                mark_image_purgeable(second_image_table->image[i].handle);
    } else if (fallback_image_table && !fallback_images_released) {
        mark_images_purgeable(fallback_image_table, 0, fallback_image_table->count - 1);
        fallback_images_released = 1;
    } else {
        save_cleanup_state(0, low_memory_message, 0);
        shutdown_with_message(1, low_memory_message);
    }
    return 1;
}

int report_fatal_memory(void)
{
    shutdown_with_message(1, fatal_memory_message);
    return 1;
}
