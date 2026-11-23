/* DOS CUSTOM.C, resident frame 04a6. Function order follows the DOS image. */
#pragma pack(1)
typedef struct {
    int left, top, right, bottom;
} CustomRect;

typedef unsigned int ShapeHandle;
typedef struct {
    signed char shape;
    int x, y;
    unsigned char pass, palette;
    int resource;
    ShapeHandle handle;
    CustomRect rect;
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
} CustomRoom;

typedef struct {
    unsigned char flags;
    unsigned int background, foreground;
} AboveCell;

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

extern CustomRoom *custom_room;
#pragma pack(1)
typedef struct {
    unsigned int modifier, unknown_02;
} LEVELCELL;

typedef struct {
    unsigned char left, right, above, below;
} ROOMLINKS;

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
#pragma pack()
extern LEVELREC Level;
#pragma pack(1)
typedef struct {
    unsigned char number;                             /* 00 */
    unsigned char left, right, above, below;           /* 01-04 */
    unsigned char above_left, above_right;             /* 05-06 */
    unsigned char below_left, below_right;             /* 07-08 */
    unsigned char custom;                             /* 09 */
} ROOMREC;
#pragma pack()
extern ROOMREC Room;
extern ShapeHandle room_specific_handle;
/* Rectangle passed to StrokePortRect for the style-4 border. */
extern const int TopRowRect[4];
extern AboveCell custom_above[];
extern PeelRecord peel_records[];
extern void far pascal LoadCarpetCode(void);
extern void far pascal UnloadCarpetCode(void);
extern void far pascal load_bridge_code(void);
extern void far pascal unload_bridge_code(void);
extern void far pascal LoadDsrtCode(void);
extern void far pascal load_throne_room(void);
extern void far pascal unload_throne_room(void);
extern void far pascal load_ruins_start_palette(void);
extern void far pascal unload_ruins_start_palette(void);
extern void far pascal load_plateau_room(void);
extern void far pascal load_altar_room(void);
extern void far pascal unload_altar_room(void);
extern void far pascal load_inscription_room(void);
extern void far pascal load_chess(void);
extern void far pascal setup_crystal_room(void);
extern void far pascal load_tower_top(void);

enum {
    TEMPLATE_CARPET,
    TEMPLATE_BRIDGE,
    TEMPLATE_DESERT,
    TEMPLATE_THRONE,
    TEMPLATE_RUINS_PALETTE,
    TEMPLATE_PLATEAU,
    TEMPLATE_ALTAR,
    TEMPLATE_INSCRIPTION,
    TEMPLATE_CHESS,
    TEMPLATE_CRYSTAL,
    TEMPLATE_TOWER_TOP,
    TEMPLATE_DEFAULT
};

/* The reserved fields are zero in each 28-byte DOS template record. */
#define UNUSED_TEMPLATE_FIELDS { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }

static RoomTemplate templates[] = {
    { { LoadCarpetCode }, { UnloadCarpetCode }, UNUSED_TEMPLATE_FIELDS },
    { { load_bridge_code }, { unload_bridge_code }, UNUSED_TEMPLATE_FIELDS },
    { { LoadDsrtCode }, { 0 }, UNUSED_TEMPLATE_FIELDS },
    { { load_throne_room }, { unload_throne_room }, UNUSED_TEMPLATE_FIELDS },
    { { load_ruins_start_palette }, { unload_ruins_start_palette }, UNUSED_TEMPLATE_FIELDS },
    { { load_plateau_room }, { 0 }, UNUSED_TEMPLATE_FIELDS },
    { { load_altar_room }, { unload_altar_room }, UNUSED_TEMPLATE_FIELDS },
    { { load_inscription_room }, { 0 }, UNUSED_TEMPLATE_FIELDS },
    { { load_chess }, { 0 }, UNUSED_TEMPLATE_FIELDS },
    { { setup_crystal_room }, { 0 }, UNUSED_TEMPLATE_FIELDS },
    { { load_tower_top }, { 0 }, UNUSED_TEMPLATE_FIELDS },
    { { 0 }, { 0 }, UNUSED_TEMPLATE_FIELDS }
};

#undef UNUSED_TEMPLATE_FIELDS

static RoomTemplate *template_table[] = {
    &templates[TEMPLATE_CARPET], &templates[TEMPLATE_DEFAULT], &templates[TEMPLATE_DEFAULT],
    &templates[TEMPLATE_DEFAULT], &templates[TEMPLATE_DEFAULT], &templates[TEMPLATE_DEFAULT],
    &templates[TEMPLATE_DESERT], &templates[TEMPLATE_DEFAULT], &templates[TEMPLATE_DEFAULT],
    &templates[TEMPLATE_DEFAULT], &templates[TEMPLATE_DEFAULT], &templates[TEMPLATE_DEFAULT],
    &templates[TEMPLATE_DEFAULT], &templates[TEMPLATE_DEFAULT], &templates[TEMPLATE_DEFAULT],
    &templates[TEMPLATE_DEFAULT], &templates[TEMPLATE_DEFAULT], &templates[TEMPLATE_DEFAULT],
    &templates[TEMPLATE_DEFAULT], &templates[TEMPLATE_DEFAULT], &templates[TEMPLATE_PLATEAU],
    &templates[TEMPLATE_PLATEAU], &templates[TEMPLATE_RUINS_PALETTE], &templates[TEMPLATE_TOWER_TOP],
    &templates[TEMPLATE_DEFAULT], &templates[TEMPLATE_CHESS], &templates[TEMPLATE_CHESS],
    &templates[TEMPLATE_CHESS], &templates[TEMPLATE_CRYSTAL], &templates[TEMPLATE_CRYSTAL],
    &templates[TEMPLATE_CRYSTAL], &templates[TEMPLATE_INSCRIPTION], &templates[TEMPLATE_ALTAR],
    &templates[TEMPLATE_BRIDGE], &templates[TEMPLATE_THRONE]
};

RoomTemplate *room_template = &templates[TEMPLATE_DEFAULT];
extern unsigned int *background_shapes;
extern CustomRect empty_piece_rect;
extern struct Rect {
    int top, left, bottom, right;
} BridgeRect;
extern void far custom_floor_image(void);

extern void far _fastcall add_peel(int top, int bottom, int left, int width,
                                   unsigned char id, unsigned char kind);
extern int find_character_peel(unsigned char id, int restore);
extern void stroke_port_rect(const int far *rect);
extern void check_roof_room(void);
extern void final_noop(void);
extern void DISPOSE_HANDLE(ShapeHandle handle);
extern void OFFSETRECT(CustomRect far *dst, CustomRect far *src, int dy, int dx);
extern void far _fastcall draw_custom_room_image(char index, char pass);
extern void far *MATERIALIZE_HANDLE(ShapeHandle handle);
extern void far *RESOLVE_HANDLE_TO_FAR_PAYLOAD(ShapeHandle handle);
extern unsigned int HGETRSRC(unsigned long type, int id);
extern void *LOAD_RESOURCE_COPY(unsigned long type, int id);
extern void far *FETCH_LOCKED_SHAPE_LIST(unsigned int archive);
extern void CLEAR_HANDLE_OWNED(unsigned int archive);
extern void prepare_archive(unsigned int archive);
extern int archive_position(unsigned int archive);
extern void set_archive_position(unsigned int archive, int position);
extern ShapeHandle PrinceUnpackShape(unsigned int resource, int format, void far *archive);
extern void load_song_range(int first, int last);
extern void release_song_range(int first, int last);
extern void shape_rect(CustomRect far *rect, int x, int y, void far *shape);
extern int rect_is_empty(CustomRect far *);
extern void unite_rect(CustomRect far *, CustomRect far *, CustomRect far *);
extern void AddDsrtStoneUpPeels(CustomPiece *);
extern void mark_bufs_for_rect(void (far *image)(void), CustomRect *rect, char pass);
extern void * _cdecl _nmalloc(unsigned int bytes);
extern void _cdecl _nfree(void *);
extern void * _cdecl memset(void *, int, unsigned int);
#pragma intrinsic(memset)

void LoadCustomRoomShapes(CustomRoom *room);
void UnloadCustomRoomShapes(int all);
void UnloadCustomRoomData(void);
void SetCustomRoomTemplate(unsigned char type);

void AddCustomPiecePeel(CustomPiece *piece, CustomRect far *rect)
{
    add_peel(rect->top, rect->bottom, rect->left, rect->right - rect->left,
             (unsigned char)(piece->shape + 100), piece->peel_type);
}

void CheckRoomSpecific(void)
{
    switch (Level.environment_kind) {
    case 4:
        if (!Room.above)
            stroke_port_rect(TopRowRect);
        break;
    case 5:
        check_roof_room();
        break;
    case 6:
        final_noop();
        break;
    }
    if (room_specific_handle) {
        DISPOSE_HANDLE(room_specific_handle);
        room_specific_handle = 0;
    }
}

void DrawCustomRoomImgAtInfo(unsigned char index, signed char *info)
{
    CustomPiece *piece = &custom_room->pieces[index];
    struct {
        CustomRect rect;
        long position;
        int dy, dx;
    } saved;
    unsigned char saved_pass;

    saved.rect = piece->rect;
    saved.position = *(long *)&piece->x;

    piece->x += (saved.dx = (info[2] + 1) * 63);
    piece->y += (saved.dy = info[1] * 32);
    OFFSETRECT(&piece->rect, &piece->rect, saved.dy, saved.dx);
    saved_pass = piece->pass;
    piece->pass = info[0];
    draw_custom_room_image(index, info[0]);
    *(long *)&piece->x = saved.position;
    piece->pass = saved_pass;
    piece->rect = saved.rect;
}

void GetCustomAboveAry(void)
{
    signed char i = 0;
    do {
        custom_above[i].flags = 0;
        custom_above[i].background = 0xC000;
        custom_above[i].foreground = 0;
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
            DISPOSE_HANDLE(piece->handle);
        piece->handle = PrinceUnpackShape(
            lookup.resource = HGETRSRC(0x53484150UL,
                                            piece->shape + custom_room->shape_base),
            2, FETCH_LOCKED_SHAPE_LIST(*background_shapes));
        CLEAR_HANDLE_OWNED(*background_shapes);
    }
    return RESOLVE_HANDLE_TO_FAR_PAYLOAD(piece->handle);
}

PieceShapeInfo *far _fastcall GetCustomPieceShpInfo(int index, PieceShapeInfo *info)
{
    if (custom_room) {
        char zero = 0;
        info->sheet = 1;
        info->column = zero;
        info->row = zero;
        info->handle = custom_room->pieces[index].handle;
    } else {
        info = 0;
    }
    return info;
}

ShapeSize far *GetCustomPieceSize(unsigned char index, ShapeSize far *size)
{
    ShapeSize far *shape;

    if (!custom_room)
        size = 0;
    else {
        shape = GetCustomPieceShape(&custom_room->pieces[index]);
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
    CustomRoom *room;
    register unsigned long load;

    if (Room.custom) {
        room = LOAD_RESOURCE_COPY(0x43555354UL, (room_number_arg + 159) * 25);
        if (room) {
            if (custom_room && room->template_id == custom_room->template_id) {
                /* The DOS routine repeats the template comparison on this path. */
                if (room->template_id == custom_room->template_id) {
                    LoadCustomRoomShapes(room);
                    UnloadCustomRoomShapes(1);
                    _nfree(custom_room);
                    custom_room = room;
                }
            } else {
                LoadCustomRoomShapes(room);
                UnloadCustomRoomData();
                custom_room = room;
                load_song_range(room->sound_first, room->sound_last);
                SetCustomRoomTemplate(custom_room->template_id);
            }
            if ((load = room_template->load.address) != 0)
                ((void (far *)(void))load)();
        }
    } else if (custom_room) {
        UnloadCustomRoomData();
        if ((((unsigned char)Level.level_number) == 5 && Room.number == 7) ||
            (((unsigned char)Level.level_number) == 5 && Room.number == 12))
            load_bridge_code();
    }
}

#pragma optimize("l", on)
void LoadCustomRoomShapes(CustomRoom *room)
{
    int i, old_count, bytes, shape_base, position;
    unsigned int offset;
    CustomPiece *next;
    ShapeHandle *old_shapes = 0, *new_shapes, *handle;
    void far *archive;
    void far *bitmap;
    CustomPiece *piece;
    CustomRect rect;
    CustomRect far *clip;

    if (custom_room) {
        if (custom_room->shape_base == (shape_base = room->shape_base)) {
            old_count = 1 + (custom_room->last_shape >= room->last_shape ?
                             custom_room->last_shape : room->last_shape);
            bytes = old_count * sizeof(ShapeHandle);
            old_shapes = _nmalloc(bytes);
            memset(old_shapes, 0, bytes);
            i = 0;
            if (custom_room->count > 0) {
                offset = 0;
                do {
                    CustomRoom *indexed = (CustomRoom *)((char *)custom_room + offset);
                    if (indexed->pieces[0].handle) {
                        piece = &indexed->pieces[0];
                        old_shapes[piece->shape] = piece->handle;
                        indexed->pieces[0].handle = 0;
                    }
                    offset += sizeof(CustomPiece);
                } while (++i < custom_room->count);
            }
        }
        if (custom_room->template_id != room->template_id)
            UnloadCustomRoomData();
    }

    shape_base = room->shape_base;
    position = archive_position(*background_shapes);
    prepare_archive(*background_shapes);
    archive = FETCH_LOCKED_SHAPE_LIST(*background_shapes);
    bytes = (room->last_shape + 1) * sizeof(ShapeHandle);
    new_shapes = _nmalloc(bytes);
    memset(new_shapes, 0, bytes);
    i = 0;
    if (room->count > 0) {
        next = room->pieces;
        do {
            if (next->shape != -1) {
                piece = next;
                if (!new_shapes[piece->shape]) {
                    if (old_shapes && (handle = &old_shapes[piece->shape], *handle)) {
                        new_shapes[piece->shape] = *handle;
                        old_shapes[piece->shape] = 0;
                    } else {
                        unsigned int resource = HGETRSRC(0x53484150UL, piece->shape + shape_base);
                        new_shapes[piece->shape] = PrinceUnpackShape(resource, 2, archive);
                    }
                }
                piece->handle = new_shapes[piece->shape];
                bitmap = RESOLVE_HANDLE_TO_FAR_PAYLOAD(piece->handle);
                shape_rect(&rect, piece->x, piece->y, bitmap);
                if (rect_is_empty(clip = &piece->rect)) {
                    *clip = rect;
                } else {
                    OFFSETRECT(&piece->rect, &piece->rect, piece->y, piece->x);
                    unite_rect(&piece->rect, &piece->rect, &rect);
                }
                piece->resource = i + shape_base;
            } else {
                piece->handle = 0;
                piece->rect = empty_piece_rect;
                piece->resource = -1;
            }
            ++next;
        } while (++i < room->count);
    }
    set_archive_position(*background_shapes, position);
    if (old_shapes) {
        i = 0;
        if (old_count > 0) {
            do {
                handle = &old_shapes[i];
                if (*handle)
                    DISPOSE_HANDLE(*handle);
            } while (++i < old_count);
        }
        _nfree(old_shapes);
    }
    _nfree(new_shapes);
}
#pragma optimize("l", off)

int NeedDrawRoomBelowCustom(void)
{
    int result;
    if (custom_room && custom_room->template_id == 31) {
        result = 1;
    } else {
        result = 0;
    }
    return result;
}

int RestoreKeepCustomPeel(unsigned char id)
{
    int peel = find_character_peel((unsigned char)(id + 100), 1);
    if (peel != -1)
        peel_records[peel].keep = 0;
    return peel;
}

#pragma optimize("e", off)
void SaveCustomRoomPeels(void)
{
    register int offset, i;

    if (custom_room) {
        i = 0;
        if (custom_room->count > 0) {
            offset = 0;
            do {
                CustomRoom *at = (CustomRoom *)((char *)custom_room + offset);
                if (at->pieces[0].peel_type == 2) {
                    /* Form both arguments from the reloaded near base. */
                    AddCustomPiecePeel(
                        &((CustomRoom *)((unsigned)custom_room + offset))->pieces[0],
                        &((CustomRoom *)((unsigned)custom_room + offset))->pieces[0].rect);
                } else {
                    CustomRoom *other = custom_room;
                    other = (CustomRoom *)((char *)other + offset);
                    if (other->pieces[0].peel_type == 3)
                        AddDsrtStoneUpPeels(&((CustomRoom *)(
                            (char *)*(CustomRoom * volatile *)&custom_room + offset))->pieces[0]);
                }
                offset += sizeof(CustomPiece);
            } while (++i < custom_room->count);
        }
    }
}

#pragma optimize("e", on)

#pragma optimize("e", off)
void SetCustomRoomTemplate(unsigned char type)
{
    if (type < 35) {
        room_template = template_table[type];
        return;
    }
    room_template = &templates[TEMPLATE_DEFAULT];
}
#pragma optimize("e", on)

void UnloadCustomRoomData(void)
{
    register unsigned long unload;

    if (custom_room) {
        UnloadCustomRoomShapes(1);
        release_song_range(custom_room->sound_first, custom_room->sound_last);
        if ((unload = room_template->unload.address) != 0)
            ((void (far *)(void))unload)();
        _nfree(custom_room);
        custom_room = 0;
        SetCustomRoomTemplate(36);
    }
}

void UnloadCustomRoomShapes(int all)
{
    signed char i;
    CustomPiece *piece;
    ShapeHandle *released;
    unsigned int bytes;
    signed char count;

    if (custom_room) {
        bytes = (custom_room->last_shape + 1) * sizeof(ShapeHandle);
        released = _nmalloc(bytes);
        memset(released, 0, bytes);
        i = 0;
        if (custom_room->count > 0) do {
            piece = &custom_room->pieces[i];
            if (piece->handle && (all || (!piece->pass && !piece->flags))) {
                if (!released[piece->shape]) {
                    DISPOSE_HANDLE(piece->handle);
                    released[piece->shape] = 1;
                }
                piece->handle = 0;
            }
            if (!all && piece->pass == 11) {
                if (custom_room->template_id == 33) {
                    if (custom_room->count - i - 1 == 0)
                        mark_bufs_for_rect(custom_floor_image, (CustomRect *)&BridgeRect, -1);
                } else {
                    mark_bufs_for_rect(custom_floor_image, &piece->rect, -1);
                }
            }
        } while (count = custom_room->count, ++i < count);
        _nfree(released);
    }
}


