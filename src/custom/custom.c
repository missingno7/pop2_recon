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
    void (far *call)(void);
    unsigned long address;
} RoomCallback;

typedef struct {
    RoomCallback load, unload;
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
extern unsigned char level_style, room_style, room_number;
extern unsigned char room_specific_ready, custom_room_enabled;
extern ShapeHandle room_specific_handle;
extern char room_specific_resource[];
extern AboveCell custom_above[];
extern PeelRecord peel_records[];
extern RoomTemplate *template_table[];
extern RoomTemplate default_template, *room_template;
extern unsigned int *custom_shape_archive;
extern CustomRect empty_piece_rect;
static CustomRect custom_floor_rect = {106, 35, 126, 314};
extern void far custom_floor_image(void);

extern void far _fastcall add_peel(int top, int bottom, int left, int width,
                                   unsigned char id, unsigned char kind);
extern int restore_peel(unsigned char id, int restore);
extern void load_room_specific_resource(char far *);
extern void check_rooftops_room(void);
extern void check_style_six_room(void);
extern void free_shape(ShapeHandle handle);
extern void offset_rect(CustomRect far *dst, CustomRect far *src, int dy, int dx);
extern void far _fastcall draw_custom_room_image(char index, char pass);
extern void far *probe_shape(ShapeHandle handle);
extern void far *locate_shape(ShapeHandle handle);
extern unsigned int find_resource(unsigned long type, int id);
extern void *load_room_resource(unsigned long type, int id);
extern void far *lock_archive(unsigned int archive);
extern void unlock_archive(unsigned int archive);
extern void prepare_archive(unsigned int archive);
extern int archive_position(unsigned int archive);
extern void set_archive_position(unsigned int archive, int position);
extern ShapeHandle load_shape(unsigned int resource, int format, void far *archive);
extern void init_room_sound(int first, int last);
extern void release_room_sound(int first, int last);
extern void reset_custom_room_sound(void);
extern void shape_rect(CustomRect far *rect, int x, int y, void far *shape);
extern int rect_is_empty(CustomRect far *);
extern void unite_rect(CustomRect far *, CustomRect far *, CustomRect far *);
extern void save_special_peel(CustomPiece *);
extern void draw_floor_foreground(void (far *image)(void), CustomRect *rect, char pass);
extern void * _cdecl malloc(unsigned int bytes);
extern void _cdecl free(void *);
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
    switch (level_style) {
    case 4:
        if (!room_specific_ready)
            load_room_specific_resource(room_specific_resource);
        break;
    case 5:
        check_rooftops_room();
        break;
    case 6:
        check_style_six_room();
        break;
    }
    if (room_specific_handle) {
        free_shape(room_specific_handle);
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
    offset_rect(&piece->rect, &piece->rect, saved.dy, saved.dx);
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

    if (!piece->handle || !probe_shape(piece->handle)) {
        if (piece->handle)
            free_shape(piece->handle);
        piece->handle = load_shape(
            lookup.resource = find_resource(0x53484150UL,
                                            piece->shape + custom_room->shape_base),
            2, lock_archive(*custom_shape_archive));
        unlock_archive(*custom_shape_archive);
    }
    return locate_shape(piece->handle);
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

typedef struct {
    unsigned int flags;
    unsigned int data;
} CustomTile;

extern CustomTile custom_tiles[][30];

int IsCustomizedRoom(unsigned char room)
{
    CustomTile *tiles = custom_tiles[room];
    unsigned int flags = 0;
    signed char tile = 0;

    do {
        if (flags)
            break;
        flags = tiles[tile].flags & 0xC000;
        ++tile;
    } while (tile < 30);
    return flags != 0;
}


void LoadCustomRoomData(unsigned char room_number_arg)
{
    CustomRoom *room;
    register unsigned long load;

    if (custom_room_enabled) {
        room = load_room_resource(0x43555354UL, (room_number_arg + 159) * 25);
        if (room) {
            if (custom_room && room->template_id == custom_room->template_id) {
                /* The DOS routine repeats the template comparison on this path. */
                if (room->template_id == custom_room->template_id) {
                    LoadCustomRoomShapes(room);
                    UnloadCustomRoomShapes(1);
                    free(custom_room);
                    custom_room = room;
                }
            } else {
                LoadCustomRoomShapes(room);
                UnloadCustomRoomData();
                custom_room = room;
                init_room_sound(room->sound_first, room->sound_last);
                SetCustomRoomTemplate(custom_room->template_id);
            }
            if ((load = room_template->load.address) != 0)
                ((void (far *)(void))load)();
        }
    } else if (custom_room) {
        UnloadCustomRoomData();
        if ((room_style == 5 && room_number == 7) ||
            (room_style == 5 && room_number == 12))
            reset_custom_room_sound();
    }
}

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

    if (custom_room) {
        if (custom_room->shape_base == (shape_base = room->shape_base)) {
            old_count = 1 + (custom_room->last_shape >= room->last_shape ?
                             custom_room->last_shape : room->last_shape);
            bytes = old_count * sizeof(ShapeHandle);
            old_shapes = malloc(bytes);
            memset(old_shapes, 0, bytes);
            i = 0;
            if (custom_room->count > 0) {
                offset = 0;
                do {
                    CustomRoom *indexed = (CustomRoom *)((char *)custom_room + offset);
                    if (indexed->pieces[0].handle) {
                        piece = &indexed->pieces[0];
                        old_shapes[piece->shape] = indexed->pieces[0].handle;
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
    position = archive_position(*custom_shape_archive);
    prepare_archive(*custom_shape_archive);
    archive = lock_archive(*custom_shape_archive);
    bytes = (room->last_shape + 1) * sizeof(ShapeHandle);
    new_shapes = malloc(bytes);
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
                        unsigned int resource = find_resource(0x53484150UL, piece->shape + shape_base);
                        new_shapes[piece->shape] = load_shape(resource, 2, archive);
                    }
                }
                piece->handle = new_shapes[piece->shape];
                bitmap = locate_shape(piece->handle);
                shape_rect(&rect, piece->x, piece->y, bitmap);
                if (rect_is_empty(&piece->rect)) {
                    piece->rect = rect;
                } else {
                    offset_rect(&piece->rect, &piece->rect, piece->y, piece->x);
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
    set_archive_position(*custom_shape_archive, position);
    if (old_shapes) {
        i = 0;
        if (old_count > 0) {
            do {
                if (old_shapes[i])
                    free_shape(old_shapes[i]);
            } while (++i < old_count);
        }
        free(old_shapes);
    }
    free(new_shapes);
}

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
    int peel = restore_peel((unsigned char)(id + 100), 1);
    if (peel != -1)
        peel_records[peel].keep = 0;
    return peel;
}

void SaveCustomRoomPeels(void)
{
    int i, offset;

    if (custom_room) {
        i = 0;
        if (custom_room->count > 0) {
            offset = 0;
            do {
                CustomRoom *at = (CustomRoom *)((char *)custom_room + offset);
                if (at->pieces[0].peel_type == 2) {
                    AddCustomPiecePeel(&((CustomRoom *)((char *)*(CustomRoom * volatile *)&custom_room + offset))->pieces[0], &at->pieces[0].rect);
                } else if (((CustomRoom *)((char *)*(CustomRoom * volatile *)&custom_room + offset))->pieces[0].peel_type == 3) {
                    save_special_peel(&((CustomRoom *)((char *)*(CustomRoom * volatile *)&custom_room + offset))->pieces[0]);
                }
                offset += sizeof(CustomPiece);
            } while (++i < custom_room->count);
        }
    }
}

#pragma optimize("e", off)
void SetCustomRoomTemplate(unsigned char type)
{
    if (type < 35) {
        room_template = template_table[type];
        return;
    }
    room_template = &default_template;
}
#pragma optimize("e", on)

void UnloadCustomRoomData(void)
{
    register unsigned long unload;

    if (custom_room) {
        UnloadCustomRoomShapes(1);
        release_room_sound(custom_room->sound_first, custom_room->sound_last);
        if ((unload = room_template->unload.address) != 0)
            ((void (far *)(void))unload)();
        free(custom_room);
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
        released = malloc(bytes);
        memset(released, 0, bytes);
        i = 0;
        if (custom_room->count > 0) do {
            piece = &custom_room->pieces[i];
            if (piece->handle && (all || (!piece->pass && !piece->flags))) {
                if (!released[piece->shape]) {
                    free_shape(piece->handle);
                    released[piece->shape] = 1;
                }
                piece->handle = 0;
            }
            if (!all && piece->pass == 11) {
                if (custom_room->template_id == 33) {
                    if (custom_room->count - i - 1 == 0)
                        draw_floor_foreground(custom_floor_image, &custom_floor_rect, -1);
                } else {
                    draw_floor_foreground(custom_floor_image, &piece->rect, -1);
                }
            }
        } while (count = custom_room->count, ++i < count);
        free(released);
    }
}


