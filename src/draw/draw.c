/* DRAW.C in original routine order. The verification receipt identifies the exact entries. */
typedef unsigned char RoomNumber;

int Ct[5];
#pragma pack(1)
typedef struct {
    unsigned char number;                             /* 00 */
    unsigned char left, right, above, below;           /* 01-04 */
    unsigned char above_left, above_right;             /* 05-06 */
    unsigned char below_left, below_right;             /* 07-08 */
    unsigned char custom;                             /* 09 */
} ROOMREC;
#pragma pack()
ROOMREC Room;
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
char RowNo;
char ColNo;

extern void clear_peels(void);
extern void init_fchar_rect(void);
extern void ZeroBuffers(void);
extern void select_room_arrays(RoomNumber room);
extern int IsCustomizedRoom(RoomNumber room);
extern void SetSideRoomInfo(void);
extern void GetAboveAry(void);
extern void GetBelowAry(void);
extern void SetLocVars(void);
extern void GetCustomAboveAry(void);
extern void redraw_room_sure(void);
extern void redraw_cell_sure(void);

void * cdecl memset(void *, int, unsigned int);
#pragma intrinsic(memset)

void full_redraw(void)
{
    clear_peels();
    memset(Ct, 0, sizeof Ct);
    init_fchar_rect();
    redraw_room_sure();
    ZeroBuffers();
}

void get_rooms(void)
{
    Room.above_left = Room.above_right = Room.below_left = Room.below_right = 0;
    if (Room.number) {
        select_room_arrays(Room.number);
        Room.left = Level.room_links[Room.number - 1].left;
        Room.right = Level.room_links[Room.number - 1].right;
        Room.above = Level.room_links[Room.number - 1].above;
        Room.below = Level.room_links[Room.number - 1].below;

        if (Room.above) {
            Room.above_left = Level.room_links[Room.above - 1].left;
            Room.above_right = Level.room_links[Room.above - 1].right;
        } else {
            if (Room.left)
                Room.above_left = Level.room_links[Room.left - 1].above;
            if (Room.right)
                Room.above_right = Level.room_links[Room.right - 1].above;
        }
        if (Room.below) {
            Room.below_left = Level.room_links[Room.below - 1].left;
            Room.below_right = Level.room_links[Room.below - 1].right;
        } else {
            if (Room.left)
                Room.below_left = Level.room_links[Room.left - 1].below;
            if (Room.right)
                Room.below_right = Level.room_links[Room.right - 1].below;
        }
        Room.custom = IsCustomizedRoom(Room.number);
    } else {
        Room.left = Room.right = Room.above = Room.below = 0;
    }
}

void redraw_room_sure(void)
{
    int saved_room;

    SetSideRoomInfo();
    RowNo = 3;
    while (RowNo--) {
        GetAboveAry();
        GetBelowAry();
        ColNo = 0;
        do {
            SetLocVars();
            redraw_cell_sure();
        } while (++ColNo < 10);
    }
    if (Room.custom == 0) {
        saved_room = Room.number;
        Room.number = Room.above;
        get_rooms();
        Room.below = (RoomNumber)saved_room;
        SetSideRoomInfo();
        RowNo = 2;
        GetAboveAry();
        GetBelowAry();
        RowNo = -1;
        ColNo = 0;
        do {
            SetLocVars();
            redraw_cell_sure();
        } while (++ColNo < 10);
        Room.number = (RoomNumber)saved_room;
        get_rooms();
    } else {
        GetCustomAboveAry();
    }
}

extern void draw_back_image(unsigned char pass);
extern void draw_fore_image(void);

void redraw_cell_sure(void)
{
    draw_back_image(0);
    draw_back_image(5);
    draw_back_image(11);
    draw_fore_image();
}

/* The following definitions are drafts. Several later same-file callees still need bodies. */
typedef struct DrawRect {
    int top, left, bottom, right;
} DrawRect;

typedef struct DirtyCell {
    int pending;
    DrawRect area;
} DirtyCell;

DrawRect CellClipRect;
extern int RedrawBuf[], HalfBuf[];
extern unsigned char ObjBuf[];
extern DirtyCell MoveBuf[], FloorBuf[];
extern DirtyCell FRedrawBuf[], TopBuf[];

extern int SECTRECT(DrawRect far *out, const DrawRect far *a,
                          const DrawRect far *b);
extern void UnionRect(DrawRect far *out, const DrawRect far *a,
                       const DrawRect far *b);
extern int compute_half_rect(DrawRect *area);
extern void draw_half_cell(void);
extern void draw_floor_buffer(void);
extern void draw_cell_objects(char column);

#define DIRTY_CELL(table, byte_offset) \
    (*(DirtyCell *)((char *)(table) + (byte_offset)))
#define WORD_FLAG(table, byte_offset) \
    (*(int *)((char *)(table) + (byte_offset)))

void redraw_cell_fast(char column)
{
    DrawRect old_clip;

    if (RedrawBuf[column]) {
        RedrawBuf[column] = 0;
        redraw_cell_sure();
    } else if (MoveBuf[column].pending) {
        MoveBuf[column].pending = 0;
        old_clip = CellClipRect;
        if (SECTRECT(&CellClipRect, &CellClipRect, &MoveBuf[column].area)) {
            draw_back_image(0);
            draw_back_image(5);
            draw_back_image(11);
            draw_fore_image();
        }
        CellClipRect = old_clip;
    }
    if (FloorBuf[column].pending) {
        FloorBuf[column].pending = 0;
        if (HalfBuf[column] && compute_half_rect(&old_clip))
            UnionRect(&FloorBuf[column].area, &FloorBuf[column].area, &old_clip);
        old_clip = CellClipRect;
        if (SECTRECT(&CellClipRect, &CellClipRect, &FloorBuf[column].area))
            draw_floor_buffer();
        CellClipRect = old_clip;
    } else if (HalfBuf[column]) {
        HalfBuf[column] = 0;
        draw_half_cell();
    }
    if (ObjBuf[column]) {
        draw_cell_objects(column);
        ObjBuf[column] = 0;
    }
    if (FRedrawBuf[column].pending) {
        FRedrawBuf[column].pending = 0;
        if (SECTRECT(&CellClipRect, &CellClipRect, &FRedrawBuf[column].area))
            draw_fore_image();
    }
}

void redraw_top_fast(char column)
{
    char old_row;
    DrawRect old_clip;

    if (TopBuf[column].pending) {
        TopBuf[column].pending = 0;
        old_row = RowNo;
        old_clip = CellClipRect;
        CellClipRect = TopBuf[column].area;
        RowNo = -1;
        draw_back_image(0);
        draw_back_image(5);
        draw_back_image(11);
        draw_fore_image();
        RowNo = old_row;
        CellClipRect = old_clip;
    }
}

/* Packed tile and image records used by the drawing passes. */
#pragma pack(1)
typedef struct TileInfo {
    unsigned int modifier, extra;
} TileInfo;

typedef struct Tile {
    signed char kind;
    TileInfo info;
} Tile;

typedef struct TileDrawArgs {
    char pass;
    signed char column, row, kind;
    TileInfo info;
} TileDrawArgs;

typedef struct RoomImage {
    unsigned char type;
    int y, x;
    char pass, flip;
    int shape, upper;
    DrawRect clip;
    unsigned char attributes, sheet;
    unsigned char reserved[4];
} RoomImage;

typedef struct RoomImages {
    signed char count;
    unsigned char flags;
    int first_shape;
    unsigned char reserved[8];
    int floor_image;
    int background_shape;
    unsigned char reserved_tail[12];
    RoomImage images[1];
} RoomImages;

typedef struct DrawEntry {
    unsigned char type, sheet;
    int shape, x, y;
    signed char column, row;
    DrawRect clip;
    unsigned char flip, attributes;
} DrawEntry;
#pragma pack()

typedef void (far pascal *TileDrawer)(TileDrawArgs *args);
typedef struct LooseItemDrawer {
    TileDrawer base, item;
} LooseItemDrawer;
LooseItemDrawer *DrawFunc;
TileDrawer DrawGateMoveFunc;
extern RoomImages *custom_room;
TileInfo BlueSpec;
signed char ObjId;
Tile CellLeft, CellRight, ScrRight[3], ScrLeft[3];
Tile Above[11], Below[11];
int NewRoom;
extern DrawRect GamePlayRect;
extern DrawEntry far BackTable[];
extern DrawEntry far ForeTable[];

extern int far pascal IsWall(char kind);
extern unsigned char back_image_start_pass(void);
extern void draw_custom_room_images(char);
extern void far _fastcall draw_custom_room_image(char, char);
extern void check_foreground_gate(void);
extern int NeedDrawRoomBelowCustom(void);
extern void load_tower_top(void);
extern int far _fastcall keep_image_in_back(RoomImage *image);
extern int draw_pillar_in_back(RoomImage *image);
extern void draw_horse(unsigned int sheet, unsigned int shape, char pass);
extern unsigned int far _fastcall horse_room_image(RoomNumber room, char pass, unsigned int *sheet);

void draw_back_image(unsigned char pass)
{
    unsigned char layer;
    struct { unsigned long address; TileDrawArgs args; } draw;

    layer = back_image_start_pass();
    if (NewRoom)
        CellClipRect = GamePlayRect;
    draw.args.pass = pass;
    do {
        unsigned char priority;
        switch (layer) {
        case 0:
            draw.args.column = ColNo - 1;
            draw.args.row = RowNo + 1;
            draw.args.kind = Below[ColNo].kind;
            draw.args.info = Below[ColNo].info;
            if (!NewRoom || ColNo == 0 || ColNo == 9 ||
                Level.environment_kind == 3 || draw.args.kind == 4 || ((unsigned char)Level.level_number) == 13)
                break;
            if (Level.environment_kind == 2 && (draw.args.kind == 19 || draw.args.kind == 9))
                break;
            layer += 2;
            break;
        case 1:
            draw.args.row = RowNo + 1;
            draw.args.column = ColNo;
            draw.args.kind = Below[ColNo + 1].kind;
            draw.args.info = Below[ColNo + 1].info;
            if (!IsWall(draw.args.kind))
                break;
            ++layer;
            goto draw_left;
        case 3:
            draw_custom_room_images(pass);
            draw.args.column = ColNo;
            draw.args.row = RowNo;
            draw.args.kind = ObjId;
            draw.args.info = BlueSpec;
            break;
        case 2:
        draw_left:
            draw.args.column = ColNo - 1;
            draw.args.row = RowNo;
            draw.args.kind = CellLeft.kind;
            draw.args.info = CellLeft.info;
            break;
        }
        priority = (draw.args.info.modifier >> 14) & 3;
        if (!custom_room || priority == 0 || pass == priority || (priority == 2 && pass == 5)) {
            draw.address = (unsigned long)((TileDrawer *)DrawFunc)[draw.args.kind];
            if (draw.address)
                ((TileDrawer)draw.address)(&draw.args);
        }
    } while (++layer < 4);
}

unsigned char back_image_start_pass(void)
{
    unsigned char pass;
    int custom_edge;

    custom_edge = NewRoom &&
        ((ObjId != 7 && IsWall(ObjId) &&
          CellLeft.kind != 7 && IsWall(CellLeft.kind)) ||
         (ColNo != 0 && RowNo != 2));
    if (custom_edge)
        pass = 3;
    else if ((NewRoom && ColNo == 0 && RowNo != 2) ||
             (custom_room && RowNo == 2 && !NeedDrawRoomBelowCustom()))
        pass = 2;
    else
        pass = 0;
    return pass;
}

void draw_custom_room_images(char pass)
{
    signed char image;
    unsigned int sheet;
    unsigned int draw_images = 0;

    if ((pass == 0 || pass == 5) && NewRoom &&
        ColNo == 0 && RowNo == 2) {
        draw_images = 1;
        if (Room.number == 1 && ((unsigned char)Level.level_number) == 14)
            load_tower_top();
    } else if ((pass == 0 && !NewRoom && custom_room &&
                custom_room->background_shape) ||
               ((pass == 1 || pass == 2) &&
                ((ColNo == 9 && RowNo == 0) || !NewRoom))) {
        draw_images = 1;
    }
    if (draw_images) {
        if (custom_room) {
            image = 0;
            if (custom_room->count > 0) {
                do {
                    draw_custom_room_image(image, pass);
                    ++image;
                } while (custom_room->count > image);
            }
        }
        if ((NewRoom && pass == 0) || pass == 1) {
            draw_images = horse_room_image(Room.number, pass, &sheet);
            if (draw_images)
                draw_horse(sheet, draw_images, pass);
        }
    }
}


void far _fastcall draw_custom_room_image(char index, char pass)
{
    RoomImage *image;
    DrawRect clip;
    DrawEntry far *entry;

    image = &custom_room->images[index];
    if (image->shape == -1 || image->pass != pass ||
        !SECTRECT(&clip, &image->clip, &CellClipRect))
        return;
    if (pass != 0 && pass != 5 && pass != 11) {
        entry = &ForeTable[Ct[1]++];
        if (Level.environment_kind != 5)
            goto queued;
        if ((Level.environment_kind == 5 && Room.number != 19 && Room.number != 16 &&
             Room.number != 15 && keep_image_in_back(image)) ||
            ((Room.number == 19 || Room.number == 16) && draw_pillar_in_back(image))) {
            entry = &BackTable[Ct[0]++];
            --Ct[1];
        }
    } else {
        if (!image->upper)
            return;
        entry = &BackTable[Ct[0]++];
    }
queued:
    entry->y = image->y;
    entry->x = image->x;
    entry->shape = image->shape;
    entry->type = 4;
    entry->column = ColNo;
    entry->row = RowNo;
    entry->clip = clip;
    entry->flip = image->flip;
    entry->sheet = image->sheet;
}

void draw_fore_image(void)
{
    unsigned char layer, priority;
    int skip_left;
    struct { unsigned long address; TileDrawArgs args; } draw;

    check_foreground_gate();
    if (NewRoom && ColNo)
        skip_left = 1;
    else
        skip_left = 0;
    layer = skip_left != 0;
    draw.args.pass = 1;
    do {
        switch (layer) {
        case 0:
            draw.args.column = ColNo - 1;
            draw.args.row = RowNo;
            draw.args.kind = CellLeft.kind;
            draw.args.info = CellLeft.info;
            break;
        case 1:
            draw.args.column = ColNo;
            draw.args.row = RowNo;
            draw.args.kind = ObjId;
            draw.args.info = BlueSpec;
            if (skip_left && Level.environment_kind != 3)
                ++layer;
            break;
        case 2:
            draw.args.row = RowNo - 1;
            draw.args.column = ColNo;
            draw.args.kind = Above[ColNo + 1].kind;
            draw.args.info = Above[ColNo + 1].info;
            break;
        }
        priority = (draw.args.info.modifier >> 14) & 3;
        if (!custom_room || !priority || priority == 1 || priority == 2) {
            draw.address = (unsigned long)((TileDrawer *)DrawFunc)[draw.args.kind];
            if (draw.address)
                ((TileDrawer)draw.address)(&draw.args);
        }
    } while (++layer < 3 && (layer < 2 || RowNo != -1));
    draw_custom_room_images(1);
}

#pragma pack(1)
typedef struct GateImage {
    unsigned char header[11];
    DrawRect clip;
} GateImage;
#pragma pack()

#pragma pack(1)
struct char_bounds { int top, left, bottom, right; };
typedef struct {
    signed char slot;                  /* 00 */
    signed char direction;             /* 01 */
    int x, y;                          /* 02, 04 */
    signed char character_kind;        /* 06 */
    unsigned int frame;                /* 07 */
    signed char column, row;           /* 09, 0a */
    signed char action;                /* 0b */
    signed char horizontal_step;       /* 0c */
    signed char vertical_step;         /* 0d */
    signed char room;                  /* 0e */
    signed char field_0f;              /* 0f */
    signed char stance;                /* 10 */
    signed char alive;                 /* 11 */
    signed char life_points;           /* 12 */
    signed char life_initial;          /* 13 */
    signed char life_delta;            /* 14 */
    int sequence_position;             /* 15 */
    int sequence_id;                   /* 17 */
    unsigned int sequence;             /* 19 */
    struct char_bounds bounds;                    /* 1b: top, left, bottom, right */
    signed char alert_delay;           /* 23 */
    int sequence_group;                /* 24 */
    int previous_x, previous_y;        /* 26, 28 */
    unsigned int previous_frame;       /* 2a */
    struct char_bounds previous_bounds;           /* 2c: top, left, bottom, right */
    int field_34;                      /* 34 */
    int previous_mode;                 /* 36 */
    unsigned char behavior;            /* 38 */
    signed char opponent_index;        /* 39 */
    unsigned char field_3a;            /* 3a */
    unsigned char unknown_3b[5];        /* 3b..3f */
} CHARREC;
#pragma pack()

extern signed char OrigLifePts;
extern CHARREC Kid;
extern CHARREC Char;
extern void DrawCaveGateSquishing(TileDrawArgs *args);
extern void copy_kid_to_char(void);
extern void load_actor_slot(char index);
extern GateImage *get_gate_image(char type, unsigned char index);
extern int EMPTYRECT(DrawRect far *rect);

void check_foreground_gate(void)
{
    signed char count, index;
    int found;
    TileDrawArgs args;
    DrawRect saved_clip;
    GateImage *image;

    if (Kid.frame >= 264 && Kid.frame <= 266) {
        if (Level.environment_kind == 3) {
            args.pass = 0;
            args.row = RowNo;
            if (ObjId == 4) {
                args.column = ColNo;
                args.info = BlueSpec;
                DrawCaveGateSquishing(&args);
            } else if (CellLeft.kind == 4) {
                args.column = ColNo - 1;
                args.info = CellLeft.info;
                DrawCaveGateSquishing(&args);
            }
        }
        return;
    }
    if (CellLeft.kind != 4)
        return;
    saved_clip = CellClipRect;
    copy_kid_to_char();
    found = CellLeft.kind == 4 && Char.row == RowNo &&
            (Char.column - ColNo == -1 || ColNo - Char.column == 2) &&
            Char.action != 2 && Char.action != 6;
    if (!found) {
        count = Level.room_objects[Room.number - 1].opponent_count;
        for (index = 0; !found && index < count; ++index) {
            load_actor_slot(index);
            found = CellLeft.kind == 4 && Char.row == RowNo &&
                    (Char.column - ColNo == -1 || ColNo - Char.column == 2) &&
                    Char.action != 2 && Char.action != 6;
        }
        if (found)
            image = get_gate_image(3, (unsigned char)Char.character_kind);
    } else
        image = get_gate_image(2, 0);
    if (found) {
        args.pass = 2;
        args.column = ColNo - 1;
        args.row = RowNo;
        args.info = CellLeft.info;
        if (DrawGateMoveFunc) {
            if (NewRoom)
                CellClipRect.top = 63 * args.row + 3;
            else if (image)
                SECTRECT(&CellClipRect, &CellClipRect, &image->clip);
            if (!EMPTYRECT(&CellClipRect))
                DrawGateMoveFunc(&args);
        }
    }
    CellClipRect = saved_clip;
}

extern void SaveCustomRoomPeels(void);
extern void draw_back_fore_item(unsigned char pass, unsigned char entry);
extern void draw_middle_item(int entry);

void dump_draw_table(unsigned char pass)
{
    unsigned char index, count;
    int prepared;

    if (!NewRoom || !custom_room)
        prepared = 1;
    else
        prepared = 0;
    index = 0;
    count = (unsigned char)Ct[pass];
    if (count) {
        do {
            if (pass == 3) {
                draw_middle_item(index);
            } else {
                if (!prepared && BackTable[index].sheet) {
                    SaveCustomRoomPeels();
                    prepared = 1;
                }
                draw_back_fore_item(pass, index);
            }
        } while (++index < count);
    }
}

#pragma pack(1)
typedef struct ShapeBitmap {
    int width, height, stride;
    unsigned char data[1];
} ShapeBitmap;

typedef struct ShapeRef {
    unsigned char kind, reserved, separate_mask, palette;
    unsigned int handle;
} ShapeRef;

typedef struct ShapeTable {
    unsigned int handle, reserved;
    int count;
} ShapeTable;

typedef struct MiddleEntry {
    int x, y;
    signed char type;
    int shape;
    DrawRect clip;
    unsigned char attributes;
    int mode;
    unsigned char palette, update_peel;
} MiddleEntry;
#pragma pack()

ShapeTable *Pics[5];
extern MiddleEntry far MidTable[];
extern ShapeBitmap far *resolve_handle_to_far_payload(unsigned int handle);
extern ShapeBitmap far *fetch_locked_shape_list(unsigned int handle);
extern unsigned int save_handle_flags(unsigned int handle);
extern void restore_handle_flags(unsigned int handle, unsigned int mark);
extern void get_set_shape_item(ShapeTable *table, int image, ShapeRef far *reference);
extern void FreeShpInfo(ShapeRef far *reference);
extern void AddCustomPiecePeel(RoomImage *image, DrawRect far *rect);
extern ShapeBitmap far *GetCustomPieceShape(RoomImage *image);
extern ShapeBitmap far *far _fastcall GetCustomPieceShpInfo(int image, ShapeRef *ref);
extern ShapeBitmap far *far _fastcall get_carpet_shape_info(int index, ShapeRef *ref);
extern ShapeBitmap far *far _fastcall load_horse_shape(int id, ShapeRef *ref);
extern void save_under_carpet(void);
extern void set_shape_variant(int variant);
extern unsigned int mark_purgeable(unsigned int handle);
extern void MIRRORMCGABLOCK(unsigned char far *data, int width, int stride);
extern void MIRRORRLEBLOCK(unsigned char far *data, int width, int stride);
extern void MIRROR4BITBLOCK(unsigned char far *data, int width, int height, int stride);
extern void push_port_clip(DrawRect far *rect);
extern void pop_port_clip(void);
extern void far _fastcall add_peel(int x, int height, int y, int width, char mode, char flags);
extern void draw_bitmap(ShapeBitmap far *image, ShapeBitmap far *mask, int x, int y,
                        int flip, int mode, unsigned int flags);

void draw_back_fore_item(unsigned char pass, unsigned char index)
{
    DrawEntry far *entry;
    ShapeBitmap far *image;
    ShapeBitmap far *mask;
    RoomImage *custom;
    ShapeRef ref;
    int mode;
    unsigned int flags;

    if (pass == 0)
        entry = &BackTable[index];
    else
        entry = &ForeTable[index];
    mode = 2;
    flags = 0;
    custom = 0;
    if (Pics[entry->type]->count >= entry->shape) {
        if (entry->type == 1) {
            mode = 3;
            flags = 0x4000;
        } else if (entry->type == 0) {
            mode = 3;
            flags = 0x8000;
        }
        get_set_shape_item(Pics[entry->type], entry->shape, &ref);
        image = resolve_handle_to_far_payload(ref.handle);
        if (!image)
            return;
    } else {
        if (!custom_room && (entry->shape < 0x6370 || entry->shape > 0x6372))
            return;
        if (custom_room) {
            custom = &custom_room->images[entry->shape - custom_room->first_shape];
            if (custom->sheet == 1)
                AddCustomPiecePeel(custom, &entry->clip);
        }
        if (entry->shape >= 0x6370 && entry->shape <= 0x6372) {
            image = load_horse_shape(entry->shape, &ref);
            entry->shape = 0;
        } else if (entry->shape >= 0x62d7 && entry->shape <= 0x62e2) {
            save_under_carpet();
            image = get_carpet_shape_info(entry->shape - 0x62d7, &ref);
            ref.palette = entry->attributes;
        } else if (custom_room) {
            image = GetCustomPieceShape(custom);
            ref.palette = entry->attributes;
        }
        ref.separate_mask = 0;
    }
    if (entry->attributes != ref.palette) {
        if (ref.separate_mask)
            MIRRORMCGABLOCK(image->data, image->width, image->stride);
        else if (ref.kind == 2)
            MIRROR4BITBLOCK(image->data, image->width, image->height, image->stride);
        else if (ref.kind == 1)
            MIRRORRLEBLOCK(image->data, image->width, image->stride);
        if (entry->shape < 0x6370 || entry->shape > 0x6372)
            ((unsigned char *)Pics[entry->type])[6 * entry->shape + 3] = entry->attributes;
    }
    if (ref.separate_mask) {
        mask = image;
        image = 0;
    } else {
        mask = 0;
    }
    push_port_clip(&entry->clip);
    draw_bitmap(image, mask, entry->x, entry->y, entry->flip, mode, flags);
    pop_port_clip();
    if (Pics[entry->type]->count > entry->shape)
        FreeShpInfo(&ref);
    else if (custom && custom->pass == 0 && custom->attributes == 0)
        mark_purgeable(custom->upper);
}

void draw_middle_item(int index)
{
    ShapeRef ref;
    ShapeBitmap far *image;
    signed char type;
    unsigned int mark, flags;
    int mode, flip, x;
    struct { int width, height; } peel;

    type = MidTable[index].type;
    if (MidTable[index].shape >= 0x62d7 && MidTable[index].shape <= 0x62e2) {
        get_carpet_shape_info(MidTable[index].shape - 0x62d7, &ref);
    } else if (((unsigned char)Level.level_number) == 5 && Room.number == 10 && type == 4 &&
               MidTable[index].shape < custom_room->count) {
        GetCustomPieceShpInfo(MidTable[index].shape, &ref);
    } else {
        if (type == 2 && MidTable[index].mode)
            set_shape_variant(MidTable[index].mode);
        get_set_shape_item(Pics[type], MidTable[index].shape + 1, &ref);
        if (type == 2 && MidTable[index].mode)
            set_shape_variant(2);
    }
    flip = MidTable[index].attributes;
    mark = save_handle_flags(ref.handle);
    image = fetch_locked_shape_list(ref.handle);
    x = MidTable[index].x;
    if (MidTable[index].palette)
        x -= image->height;
    if (type == 2) {
        flags = 0;
        mode = 1;
    } else if (type == 4) {
        flags = 0;
        mode = 2;
    } else {
        if (type == 0)
            flags = 0x8000;
        else if (type == 1)
            flags = 0x4000;
        else
            flags = MidTable[index].mode;
        mode = 3;
    }
    if (MidTable[index].palette != ref.palette) {
        if (mode == 1)
            MIRRORMCGABLOCK(image->data, image->width, image->stride);
        else if (mode == 2)
            MIRRORRLEBLOCK(image->data, image->width, image->stride);
        else
            MIRROR4BITBLOCK(image->data, image->width, image->height, image->stride);
        ((unsigned char *)Pics[type])[6 * MidTable[index].shape + 9] =
            MidTable[index].palette;
    }
    peel.height = x + image->height;
    if (peel.height > MidTable[index].clip.right)
        peel.height = MidTable[index].clip.right;
    peel.width = MidTable[index].clip.bottom - MidTable[index].y;
    if (peel.width > image->width)
        peel.width = image->width;
    add_peel(x, peel.height, MidTable[index].y, peel.width, MidTable[index].update_peel, 0);
    push_port_clip(&MidTable[index].clip);
    draw_bitmap(image, 0, x, MidTable[index].y, flip, mode, flags);
    pop_port_clip();
    restore_handle_flags(ref.handle, mark);
    FreeShpInfo(&ref);
}

typedef struct DrawDestination {
    unsigned char header[24];
    DrawRect clip;
} DrawDestination;

extern DrawDestination *Graf;
static int dirty_rectangle_count;
static struct {
    unsigned char far *saved_palette;
    signed char sorted_objects[26];
    unsigned char far *saved_text_palette;
} draw_scratch;
static const DrawRect half_tile_rectangles[10] = {
    {57, 0, 63, 16},
    {57, 0, 63, 16},
    {57, 0, 63, 17},
    {57, 0, 63, 20},
    {57, 0, 63, 15},
    {57, 0, 63, 16},
    {57, 0, 63, 19},
    {59, 0, 63, 14},
    {59, 0, 63, 13},
    {59, 0, 63, 13},
};
static const int horse_level9_origin[] = {22, 159};
static const int horse_level10_origin[] = {102, 119};
int horse_level14_origin[] = {37, 112};

extern void far pascal draw_temple_gate_movable(void near *cell);
extern void far pascal DrawCaveGateMovable(void near *cell);
extern void far pascal draw_ruin_gate_movable(void near *cell);

typedef void (far pascal *LevelTileProc)(void near *);
extern LevelTileProc desert_level_spec[], drawtemp_level_spec[];
extern LevelTileProc drawcave_level_spec[], drawruin_level_spec[];
extern LevelTileProc rooftops_level_spec[], final_level_spec[];

int near *level_start_table[] = {
    0, (int near *)desert_level_spec, (int near *)drawtemp_level_spec,
    (int near *)drawcave_level_spec, (int near *)drawruin_level_spec,
    (int near *)rooftops_level_spec, (int near *)final_level_spec
};

void far *level_sequence_table[] = {
    0, 0, (void far *)draw_temple_gate_movable, (void far *)DrawCaveGateMovable,
    (void far *)draw_ruin_gate_movable, 0, 0
};

static struct {
    char missing_shape[63];
    char rectangle_overflow[16];
} draw_messages = {
    "ERROR:  DrawIt().  x = %d, y = %d, mode = 0x%x, shapeType = %d",
    "DRects Overflow"
};
extern int cdecl sprintf(char *out, const char *fmt, ...);
extern void abort_with_message(int severity, const char far *message);
extern void paint_plain(ShapeBitmap far *image, int x, int y, int flip);
extern void paint_indexed_shape(ShapeBitmap far *image, int x, int y, int flip, unsigned int flags);
extern void PAINTRLESHAPE_ASM(ShapeBitmap far *image, int x, int y, int flip);
extern void paint_four_bit(ShapeBitmap far *mask, int x, int y, int color, int flip);
extern void add_dirty_rectangle(DrawRect *rect);

void draw_bitmap(ShapeBitmap far *image, ShapeBitmap far *mask, int x, int y,
                 int flip, int mode, unsigned int flags)
{
    typedef struct BitmapSize {
        int width, height;
    } BitmapSize;
    DrawRect area;
    BitmapSize size;
    char message[150];

    if (!image && !mask) {
        sprintf(message, draw_messages.missing_shape, x, y, flip, mode);
        abort_with_message(1, message);
    }
    if (image) {
        switch (flip) {
        case 0:
        case 10:
            if (mode == 2) {
                PAINTRLESHAPE_ASM(image, x, y, flip);
                break;
            }
            if (mode == 3) {
                paint_indexed_shape(image, x, y, flip, flags);
                break;
            }
        case 2:
        case 3:
        case 8:
            paint_plain(image, x, y, flip);
            break;
        }
        size = *(BitmapSize far *)image;
    } else {
        paint_four_bit(mask, x, y, 0, flip);
        size = *(BitmapSize far *)mask;
    }
    if (!NewRoom) {
        area.right = area.left = x;
        area.right += size.height;
        area.bottom = area.top = y;
        area.bottom += size.width;
        if (SECTRECT(&area, &area, &Graf->clip))
            add_dirty_rectangle(&area);
    }
}

extern void add_mobs(void);
extern void add_characters(void);
extern void mark_peel_front_buffers(void);
extern void redraw_room_fast(void);
extern signed char FirstInRow(signed char row);

void full_redraw_fast(void)
{
    add_mobs();
    add_characters();
    mark_peel_front_buffers();
    redraw_room_fast();
}

void redraw_room_fast(void)
{
    RoomNumber saved_room;

    SetSideRoomInfo();
    draw_cell_objects(30);
    RowNo = 3;
    goto next_row;
paint_row:
    GetAboveAry();
    GetBelowAry();
    ColNo = 0;
next_cell:
    SetLocVars();
    redraw_cell_fast((signed char)(FirstInRow(RowNo) + ColNo));
    if (++ColNo < 10)
        goto next_cell;
next_row:
    if (RowNo--)
        goto paint_row;
    if (!Room.custom) {
        saved_room = Room.number;
        Room.number = Room.above;
        get_rooms();
        SetSideRoomInfo();
        RowNo = 2;
        GetAboveAry();
        GetBelowAry();
        ColNo = 0;
        do {
            SetLocVars();
            redraw_top_fast(ColNo);
        } while (++ColNo < 10);
        Room.number = saved_room;
        get_rooms();
    } else {
        GetCustomAboveAry();
    }
    draw_cell_objects(-1);
}

extern DrawDestination *BPort;
extern DrawDestination ScreenPort;
extern int BlackOut;
static DrawRect dirty_rectangles[40];
extern void SET_RECORD_RANGE(DrawRect far *area, int style);
extern void sort_back_table(void);
extern void sort_fore_table(void);
extern void restore_peels(void);
extern DrawRect far *INSETRECT(DrawRect far *out, const DrawRect far *area, int x, int y);
extern void show_message(const char far *message, int severity);

void draw_all(void)
{
    dirty_rectangle_count = 0;
    Graf = BPort;
    if (BlackOut)
        SET_RECORD_RANGE(&GamePlayRect, 12);
    if (Level.environment_kind == 4 || Level.environment_kind == 2 || Level.environment_kind == 5)
        sort_back_table();
    if ((Room.number == 1 && ((unsigned char)Level.level_number) == 14) ||
        (Room.number == 22 && ((unsigned char)Level.level_number) == 10))
        sort_fore_table();
    restore_peels();
    dump_draw_table(0);
    dump_draw_table(3);
    dump_draw_table(1);
    Graf = &ScreenPort;
}

void add_dirty_rectangle(DrawRect *rect)
{
    DrawRect expanded;
    int index;

    for (index = 0; index < dirty_rectangle_count; index++) {
        INSETRECT(&expanded, rect, -1, -1);
        if (SECTRECT(&expanded, &expanded, &dirty_rectangles[index])) {
            UnionRect(&dirty_rectangles[index], &dirty_rectangles[index], rect);
            return;
        }
    }
    if (dirty_rectangle_count >= 40)
        show_message(draw_messages.rectangle_overflow, 1);
    else
        dirty_rectangles[dirty_rectangle_count++] = *rect;
}

extern int compute_slide_rock_rect(DrawRect *area, RoomNumber room, signed char row);
extern void mark_foreground(signed char cell, DrawRect *area);


void draw_half_cell(void)
{
    unsigned char priority;
    DrawRect area;
    struct {
        unsigned long address;
        DrawRect old_clip, row_clip;
        TileDrawArgs args;
    } draw;

    if (!compute_half_rect(&area))
        return;
    if (Level.environment_kind == 2 && compute_slide_rock_rect(&draw.row_clip, Room.number, RowNo) &&
        SECTRECT(&draw.row_clip, &draw.row_clip, &area))
        area.top = draw.row_clip.bottom - 1;
    draw.old_clip = CellClipRect;
    CellClipRect = area;
    draw.args.pass = 2;
    draw.args.column = ColNo;
    draw.args.row = RowNo;
    draw.args.info = BlueSpec;
    priority = (draw.args.info.modifier >> 14) & 3;
    draw.address = (unsigned long)((TileDrawer *)DrawFunc)[(unsigned char)ObjId];
    if (draw.address && (!Room.custom || priority == 2 || priority == 0)) {
        ((TileDrawer)draw.address)(&draw.args);
        mark_foreground((signed char)(FirstInRow(RowNo) + ColNo), &area);
    }
    if (custom_room && custom_room->floor_image)
        draw_custom_room_images(2);
    CellClipRect = draw.old_clip;
}

extern CHARREC RoomOpp[5];
extern DrawRect *ComputeOffsetCellRect(DrawRect *area, DrawRect *source, int column, int row);

int compute_half_rect(DrawRect *area)
{
    unsigned int image;
    signed char index;
    DrawRect source;

    if (Kid.frame >= 135 && Kid.frame < 145) {
        image = Kid.frame;
    } else {
        image = 0;
        for (index = 0; index < Level.room_objects[(RoomNumber)Char.room - 1].opponent_count && !image; ++index) {
            if (RoomOpp[index].character_kind == 1 &&
                RoomOpp[index].frame >= 135 &&
                RoomOpp[index].frame < 145)
                image = RoomOpp[index].frame;
        }
    }
    if (image) {
        source = half_tile_rectangles[image - 135];
        if (ObjId == 6)
            ++source.bottom;
        if ((Char.room == 7 || Char.room == 8) && Level.environment_kind == 6)
            source.top += 3;
        area = ComputeOffsetCellRect(area, &source, ColNo, RowNo);
    }
    return (int)area;
}

extern int far pascal IS_SIX_KIND(char kind);

void draw_floor_buffer(void)
{
    DrawRect old_clip, row_clip;
    TileDrawArgs args;
    unsigned char priority;
    int floor_edge, floor_y;

    old_clip = CellClipRect;
    floor_y = 63 * RowNo + 53;
    if (floor_y > CellClipRect.top)
        CellClipRect.top = floor_y;
    floor_edge = 0;
    if (Level.environment_kind == 2) {
        if (compute_slide_rock_rect(&row_clip, Room.number, RowNo) &&
            SECTRECT(&row_clip, &row_clip, &CellClipRect)) {
            CellClipRect.top = row_clip.bottom - 1;
            floor_edge = 1;
        } else if (compute_slide_rock_rect(&row_clip, Room.number, (signed char)(RowNo + 1)) &&
                   SECTRECT(&row_clip, &row_clip, &CellClipRect)) {
            floor_edge = 1;
        }
    }
    if (IS_SIX_KIND(CellLeft.kind)) {
        priority = (CellLeft.info.modifier >> 14) & 3;
        if (!Room.custom || priority == 2 || !priority) {
            if (((TileDrawer *)DrawFunc)[(unsigned char)CellLeft.kind]) {
                args.pass = 2;
                args.column = ColNo - 1;
                args.row = RowNo;
                args.kind = CellLeft.kind;
                args.info = CellLeft.info;
                ((TileDrawer *)DrawFunc)[(unsigned char)CellLeft.kind](&args);
            }
        }
        priority = (BlueSpec.modifier >> 14) & 3;
        if (!Room.custom || priority == 2 || !priority) {
            if (((TileDrawer *)DrawFunc)[(unsigned char)ObjId]) {
                args.pass = 2;
                args.column = ColNo;
                args.row = RowNo;
                args.kind = ObjId;
                args.info = BlueSpec;
                ((TileDrawer *)DrawFunc)[(unsigned char)ObjId](&args);
                mark_foreground((signed char)(FirstInRow(RowNo) + ColNo), &CellClipRect);
            }
        }
        if (custom_room && custom_room->floor_image)
            draw_custom_room_images(2);
    } else if (floor_edge && IS_SIX_KIND(ObjId) && IsWall(CellLeft.kind)) {
        priority = (CellLeft.info.modifier >> 14) & 3;
        if (!Room.custom || priority == 2) {
            if (((TileDrawer *)DrawFunc)[(unsigned char)CellLeft.kind]) {
                args.pass = 2;
                args.column = ColNo - 1;
                args.row = RowNo;
                args.kind = CellLeft.kind;
                args.info = CellLeft.info;
                ((TileDrawer *)DrawFunc)[(unsigned char)CellLeft.kind](&args);
            }
        }
    }
    CellClipRect = old_clip;
}

#pragma pack(1)
typedef struct CellObject {
    int x, y;
    unsigned char set;
    int image;
    unsigned char object_id;
    signed char cell;
    unsigned char flags, kind;
    DrawRect clip;
    signed char status;
    int depth;
    unsigned char type;
} CellObject;
#pragma pack()

extern CellObject ObjTable[];
static int sorted_object_total;
extern int EqualRect(DrawRect far *first, DrawRect far *second);
extern void sort_cell_objects(void);
extern void draw_cell_object(signed char index);

void draw_cell_objects(char column)
{
    signed char index, count;
    DrawRect merged_clip, object_clip, original_clip;

    count = (signed char)Ct[4];
    if (!count)
        return;
    original_clip = CellClipRect;
    merged_clip = CellClipRect;
    sorted_object_total = 0;
    if ((index = count - 1) >= 0) do {
        if (ObjTable[index].status == column ||
            (ObjTable[index].status >= 0 &&
             SECTRECT(&object_clip, &merged_clip, &ObjTable[index].clip))) {
            draw_scratch.sorted_objects[sorted_object_total++] = index;
            UnionRect(&object_clip, &merged_clip, &ObjTable[index].clip);
            if (!EqualRect(&object_clip, &merged_clip)) {
                index = count;
                sorted_object_total = 0;
                merged_clip = object_clip;
            }
        }
    } while (--index >= 0);
    if (sorted_object_total) {
        CellClipRect = merged_clip;
        sort_cell_objects();
        index = 0;
        if (sorted_object_total > 0) do {
            draw_cell_object(draw_scratch.sorted_objects[index]);
            ObjTable[draw_scratch.sorted_objects[index]].status = -2;
        } while (++index < sorted_object_total);
    }
    CellClipRect = original_clip;
}

extern int compare_objects(int first, int second);
extern unsigned char far _fastcall object_priority(unsigned char type);
extern unsigned char CharObjForePriority[15];
extern unsigned char NonCharObjForePriority[13];
extern unsigned char load_draw_object(signed char index);
extern void draw_loose_object(unsigned char type);

void sort_cell_objects(void)
{
    int swapped, index, last, value;

    last = sorted_object_total - 1;
    do {
        index = 0;
        swapped = 0;
        if (last > 0) {
            do {
                if (compare_objects(index, index + 1)) {
                    value = draw_scratch.sorted_objects[index];
                    draw_scratch.sorted_objects[index] = draw_scratch.sorted_objects[index + 1];
                    draw_scratch.sorted_objects[index + 1] = value;
                    swapped = 1;
                }
            } while (++index < last);
        }
    } while (swapped);
}

int compare_objects(int first, int second)
{
    int left_id, right_id;
    unsigned char left_type, right_type;
    unsigned char left_priority, right_priority;
    int swap;

    left_id = *(int *)&ObjTable[draw_scratch.sorted_objects[first]].object_id;
    left_type = ObjTable[draw_scratch.sorted_objects[first]].kind;
    right_id = *(int *)&ObjTable[draw_scratch.sorted_objects[second]].object_id;
    right_type = ObjTable[draw_scratch.sorted_objects[second]].kind;
    if (left_type == right_type) {
        if (ObjTable[draw_scratch.sorted_objects[first]].y != ObjTable[draw_scratch.sorted_objects[second]].y)
            swap = ObjTable[draw_scratch.sorted_objects[first]].y > ObjTable[draw_scratch.sorted_objects[second]].y;
        else
            swap = ObjTable[draw_scratch.sorted_objects[first]].x > ObjTable[draw_scratch.sorted_objects[second]].x;
    } else if (left_type != 7 && left_type != 8 &&
               (right_type == 7 || right_type == 8) &&
               (right_id == 181 || right_id == 166)) {
        swap = 1;
    } else if ((left_type == 7 || left_type == 8) &&
               right_type != 7 && right_type != 8 &&
               (left_id == 181 || left_id == 166)) {
        swap = 0;
    } else {
        left_priority = object_priority(left_type);
        right_priority = object_priority(right_type);
        if (left_priority != right_priority)
            swap = left_priority < right_priority;
        else if (ObjTable[draw_scratch.sorted_objects[first]].y != ObjTable[draw_scratch.sorted_objects[second]].y) {
            if (!(left_type & 0x80) && !(right_type & 0x80))
                swap = ObjTable[draw_scratch.sorted_objects[first]].y > ObjTable[draw_scratch.sorted_objects[second]].y;
            else
                swap = ObjTable[draw_scratch.sorted_objects[first]].y < ObjTable[draw_scratch.sorted_objects[second]].y;
        } else
            swap = ObjTable[draw_scratch.sorted_objects[first]].x > ObjTable[draw_scratch.sorted_objects[second]].x;
    }
    return swap;
}

unsigned char far _fastcall object_priority(unsigned char type)
{
    unsigned char result;

    if (type & 0x80)
        result = NonCharObjForePriority[type & 0x7f];
    else
        result = CharObjForePriority[type];
    return result;
}


GateImage *get_gate_image(char set, unsigned char type)
{
    CellObject *match = 0;
    int index = 0;
    int total = Ct[4];
    unsigned char *selector;

    if (total > 0) {
        selector = &ObjTable[0].set;
        do {
            if (match)
                break;
            if ((unsigned char)set == *selector && selector[6] == type)
                match = (CellObject *)(selector - 4);
            selector += sizeof(CellObject);
        } while (++index < total);
    }
    return (GateImage *)match;
}

#pragma pack(1)
typedef struct {
    signed char index, flipped;                       /* 00-01 */
    int x, y, image;                                   /* 02,04,06 */
    unsigned char image_set;                          /* 08 */
    int clip_top, clip_left, clip_bottom, clip_right;    /* 09,0b,0d,0f */
    int depth;                                        /* 11 */
    unsigned char object_type;                        /* 13 */
} FCHARREC;
#pragma pack()
FCHARREC FChar;
int XCo;
extern int far _fastcall add_middle_image(unsigned char set, int image, int x, int y, unsigned char order);
extern void draw_obj_carpet(unsigned char type);
extern void DrawObjSpikes(unsigned char type);
extern void draw_object_slide_rock(unsigned char type);
extern void draw_object_trapdoor(unsigned char type);
extern void draw_bridge_mob_object(unsigned char type);
extern void draw_fireball_object(unsigned char type);

void draw_cell_object(signed char index)
{
    unsigned char type = load_draw_object(index);

    switch (type) {
    case 0:
        if (FChar.image != -1) {
            add_middle_image(FChar.image_set, FChar.image + 1,
                             XCo, FChar.y, 10);
            if (FChar.depth) {
                MidTable[Ct[3] - 1].mode = FChar.depth;
            }
        }
        break;
    case 1: case 2: case 3:
    case 5: case 6: case 7: case 8: case 9: case 10: case 14:
        if (add_middle_image(FChar.image_set, FChar.image + 1,
                             XCo, FChar.y, 10)) {
            MidTable[Ct[3] - 1].mode = FChar.depth;
            MidTable[Ct[3] - 1].update_peel = FChar.object_type;
        }
        break;
    case 13:
        draw_obj_carpet(type);
        break;
    case 0x80: case 0x81: case 0x83:
        draw_loose_object(type);
        break;
    case 0x82:
        add_middle_image(FChar.image_set, 103, XCo, FChar.y, 10);
        return;
    case 0x84: case 0x85:
        DrawObjSpikes(type);
        return;
    case 0x86: case 0x87: case 0x88: case 0x89:
        draw_object_slide_rock(type);
        return;
    case 0x8a:
        draw_object_trapdoor(type);
        return;
    case 0x8b:
        draw_bridge_mob_object(type);
        return;
    case 0x8c:
        draw_fireball_object(type);
        break;
    }
}

void draw_loose_object(unsigned char type)
{
    int horizontal_gap, left_shape, right_shape, vertical_gap;

    switch (Level.environment_kind) {
    case 2:
        left_shape = 21;
        right_shape = 22;
        vertical_gap = horizontal_gap = 0;
        break;
    case 3:
        left_shape = (type & 127) + 66;
        right_shape = (type & 127) + 70;
        horizontal_gap = 1;
        vertical_gap = 4;
        FChar.clip_bottom += 5;
        break;
    case 4:
        left_shape = 22;
        right_shape = 26;
        vertical_gap = horizontal_gap = 0;
        break;
    default:
        left_shape = 0;
        break;
    }
    if (left_shape) {
        FChar.flipped = -1;
        add_middle_image(4, left_shape, XCo + horizontal_gap,
                         FChar.y, 10);
        add_middle_image(4, right_shape, XCo,
                         vertical_gap + FChar.y, 10);
    }
}

unsigned char load_draw_object(signed char index)
{
    CellObject *item = &ObjTable[index];

    FChar.index = item->status;
    XCo = FChar.x = item->x;
    FChar.y = item->y;
    FChar.image = item->image;
    FChar.image_set = item->set;
    FChar.flipped = item->flags;
    *(DrawRect *)&FChar.clip_top = item->clip;
    FChar.depth = item->depth;
    FChar.object_type = item->type;
    return item->kind;
}

void set_object_cell(signed char index)
{
    ObjTable[index].status = FChar.index;
    if (FChar.index < 30)
        ObjBuf[FChar.index] = 1;
}

extern void far *far cdecl _fmemmove(void far *out, const void far *src, unsigned count);
extern struct {
    int control_mode, fade_active, playing_nis, recording_mode, skip_opening, field_0a;
    int plateau_loaded, throne_song_added, throne_nis_played, escher_room_eight_seen, nis_room_restored;
} GameState;
extern void sort_foreground(void);

void sort_back_table(void)
{
    int dest, source;
    DrawEntry item;

    dest = source = 0;
    if (Ct[0] > 0) {
        do {
            if (!BackTable[source].sheet) {
                if (source != dest) {
                    item = BackTable[source];
                    _fmemmove(&BackTable[dest + 1], &BackTable[dest],
                                    (source - dest) * sizeof(DrawEntry));
                    BackTable[dest] = item;
                }
                ++dest;
            }
        } while (++source < Ct[0]);
    }
}

void sort_fore_table(void)
{
    int dest, source, shape;
    DrawEntry item;

    shape = 0;
    if (Level.environment_kind == 6) {
        if ((Room.number == 1 && ((unsigned char)Level.level_number) == 14) ||
            (Room.number == 22 && ((unsigned char)Level.level_number) == 10))
            shape = 0x6372;
        else
            shape = -1;
    }
    if (shape == -1) {
        sort_foreground();
        return;
    }
    if (shape) {
        dest = source = 0;
        if (Ct[1] > 0) {
            do {
                if (ForeTable[source].shape == shape) {
                    if (source != dest) {
                        item = ForeTable[source];
                        _fmemmove(&ForeTable[dest + 1], &ForeTable[dest],
                                        (source - dest) * sizeof(DrawEntry));
                        ForeTable[dest] = item;
                    }
                    ++dest;
                }
            } while (++source < Ct[1]);
        }
    }
}

extern DrawRect message_rect;
extern int RestartLevel;
extern int MsgTimer, MsgTimerStart;
extern char restart_key_message2[];
extern char restart_key_message1[];
extern void clear_message(int reset_timer);
extern void clear_life_bar(void);
extern void DRAW_NUL_TEXT_IN_RECT(DrawRect far *area, int color, int mode, const char far *text);
extern void STROKE_PORT_RECT(DrawRect far *area);
extern void PrinceCopy(DrawRect far *area);
extern char *cdecl strcpy(char *out, const char *in);
extern char *cdecl strupr(char *str);
#pragma intrinsic(strcpy)

void draw_message(const char *message)
{
    char text[100];

    strcpy(text, message); strupr(text);
    clear_message(1); DRAW_NUL_TEXT_IN_RECT(&message_rect, 0, 1, text);
}

void draw_restart_message(void)
{
    DrawRect area;

    if (!RestartLevel) {
        clear_life_bar();
        INSETRECT(&area, &message_rect, -message_rect.left, 0); area.right = 320;
        if (GameState.control_mode == 2) DRAW_NUL_TEXT_IN_RECT(&area, 0, 1, restart_key_message2);
        else DRAW_NUL_TEXT_IN_RECT(&area, 0, 1, restart_key_message1);
    }
}

void clear_life_bar(void)
{
    DrawRect area;

    INSETRECT(&area, &message_rect, -message_rect.left, 0);
    area.right = 320; STROKE_PORT_RECT(&area);
}

void clear_message(int reset_timer)
{
    DrawRect area;

    if (MsgTimerStart == 600) {
        INSETRECT(&area, &message_rect, -message_rect.left, 0);
        area.right = 320;
        SET_RECORD_RANGE(&area, 0);
    } else {
        SET_RECORD_RANGE(&message_rect, 0);
    }
    if (reset_timer) MsgTimer = MsgTimerStart = 0;
}

void copy_dirty_rectangles(void)
{
    while (dirty_rectangle_count--) PrinceCopy(&dirty_rectangles[dirty_rectangle_count]);
    dirty_rectangle_count = 0;
}

#pragma pack(1)
typedef struct HorseShapeHeader {
    int mode;
    unsigned char palette;
    unsigned char payload[96];
} HorseShapeHeader;
#pragma pack()

ShapeRef HorseShpInfo;
extern unsigned int HGetRsrc(unsigned long type, int shape);
extern unsigned int find_shape_list(int shape);
extern unsigned int convert_shape(HorseShapeHeader far *out, HorseShapeHeader far *header, int count);
extern ShapeBitmap far *materialize_handle(unsigned int handle);
extern void dispose_handle(unsigned int handle);
ShapeBitmap far *far _fastcall load_horse_shape(int id, ShapeRef *ref)
{
    HorseShapeHeader header;
    ShapeBitmap far *result;
    unsigned int handle, new_handle;
    HorseShapeHeader far *metadata;

    if (!HorseShpInfo.handle || !materialize_handle(HorseShpInfo.handle)) {
        if (HorseShpInfo.handle) dispose_handle(HorseShpInfo.handle);
        handle = HGetRsrc(0x53484150UL, id);
        if (handle) {
            metadata = (HorseShapeHeader far *)resolve_handle_to_far_payload(find_shape_list(0x61a9));
            header = *(HorseShapeHeader far *)((char far *)metadata + 4);
            header.palette = 0xf0; header.mode = -2;
            new_handle = convert_shape((HorseShapeHeader far *)fetch_locked_shape_list(handle), &header, 1);
            dispose_handle(handle);
            HorseShpInfo.kind = 1; HorseShpInfo.reserved = 0;
            HorseShpInfo.separate_mask = 0; HorseShpInfo.palette = 0;
            HorseShpInfo.handle = mark_purgeable(new_handle);
        }
    }
    *ref = HorseShpInfo;
    if (HorseShpInfo.handle) result = resolve_handle_to_far_payload(HorseShpInfo.handle);
    else result = 0;
    return result;
}

unsigned int far _fastcall horse_room_image(RoomNumber room, char pass, unsigned int *shape)
{
    unsigned int origin = 0;

    if (pass == 0) {
        if (room == 2 && ((unsigned char)Level.level_number) == 9) {
            origin = (unsigned int)horse_level9_origin;
            *shape = 0x6371;
            goto chosen;
        }
    } else if (pass == 1 && !GameState.playing_nis) {
        if (room == 1 && ((unsigned char)Level.level_number) == 14) {
            origin = (unsigned int)horse_level14_origin;
            *shape = 0x6372;
            goto chosen;
        }
        if (room == 22 && ((unsigned char)Level.level_number) == 10) {
            origin = (unsigned int)horse_level10_origin;
            *shape = 0x6370;
        }
    }
chosen:
    return origin;
}

void draw_horse(unsigned int shape, unsigned int origin, char pass)
{
    DrawEntry far *entry;
    int *point;

    if (!shape)
        return;
    point = (int *)origin;
    if (point) {
        if (pass == 0)
            entry = &BackTable[Ct[0]++];
        else
            entry = &ForeTable[Ct[1]++];
        entry->attributes = 0;
        entry->y = point[0];
        entry->x = point[1];
        entry->shape = shape;
        entry->type = 4;
        entry->column = ColNo;
        entry->row = RowNo;
        entry->clip = CellClipRect;
        entry->flip = 10;
        entry->sheet = ObjId;
    }
}



extern int *Pieces;
extern void add_middle_item(unsigned char sheet, int image, char column, char row,
                            unsigned char color, unsigned char flags);
extern unsigned char flask_bubble_color(unsigned int attributes);

void draw_flask(TileDrawArgs *args)
{
    int color, tall;
    unsigned char bubble, flags;
    TileDrawArgs base;

    if (DrawFunc->item && !custom_room) {
        base = *args;
        base.info.modifier &= 0xff00;
        DrawFunc->item(&base);
    }
    if (args->pass != 1)
        return;
    color = args->info.modifier & 0xe0;
    tall = color < 0xa0 && color >= 0x40;
    add_middle_item(10, tall + 1, args->column, args->row, 10, 0);
    if (!color)
        return;
    color = args->info.modifier & 0x1f;
    if (color == 28 || (color >= 10 && color <= 13) || color - 14 >= 10)
        return;
    bubble = flask_bubble_color(args->info.modifier);
    if (color > 13) {
        color -= 14;
        flags = 1;
    } else
        flags = 0;
    *(int *)((char *)Pieces + 0x485) = tall ? -17 : -14;
    add_middle_item(60, color + 3, args->column, args->row, bubble, flags);
    add_middle_item(60, color + 13, args->column, args->row, bubble + 1, flags);
}

unsigned char flask_bubble_color(unsigned int attributes)
{
    unsigned char color;

    switch ((unsigned int)attributes & 0xe0) {
    default:
        color = 0xec;
        break;
    case 0x20:
    case 0x40:
        color = 0xea;
        break;
    case 0xa0:
        color = 0xee;
        break;
    }
    return color;
}

extern DrawRect ScreenPortRect;
extern void PrincePaintShapeIdx(int table, int image, int x, int y, int flags, int mode);
extern void load_kid_with_opponent(signed char guard);
extern CHARREC Opp;
extern void DrawOppMeter(signed char guard, int life, int max_life);

void DrawKidMeter(int life, int max_life)
{
    int x, index;
    DrawRect rect;

    x = 2;
    for (index = 0; index < life; index++) {
        PrincePaintShapeIdx((int)Pics[2], 216, x, 193, 0, 1);
        x += 8;
    }
    for (index = life; index < max_life; index++) {
        PrincePaintShapeIdx((int)Pics[2], 217, x, 193, 0, 1);
        x += 8;
    }
    if (max_life >= 1 && Char.life_initial < OrigLifePts) {
        rect.top = 193;
        rect.left = x + (Kid.life_initial - max_life) * 8;
        rect.bottom = ScreenPortRect.bottom;
        rect.right = 98;
        STROKE_PORT_RECT(&rect);
    }
}

void draw_meters(void)
{
    if (Kid.opponent_index >= 0 && Kid.opponent_index < 5) {
        load_kid_with_opponent(Kid.opponent_index);
        DrawOppMeter(Opp.slot, Opp.life_points, Opp.life_initial);
    }
    DrawKidMeter(Kid.life_points, Kid.life_initial);
}

extern signed char pick_hidden_actor(unsigned char slot);
extern void load_actor_slot(signed char guard);

void DrawOppMeter(signed char guard, int life, int max_life)
{
    DrawRect rect;
    struct RectCorner { int bottom, right; };
    int index, x, half;

    if (guard == -1) {
        guard = pick_hidden_actor(1);
        if (guard == -1 && (unsigned char)Level.room_objects[Room.number - 1].opponent_count)
            guard = 0;
    }
    if (guard != -1)
        load_actor_slot(guard);
    if (guard == -1 || !(unsigned char)Level.room_objects[Room.number - 1].opponent_count ||
        Char.character_kind == 10 || Char.character_kind == 11 || Char.character_kind == 0 ||
        (Char.character_kind == 4 && Char.row != 0 && Level.level_number == 5 &&
         (Char.room == 10 || Char.room == 7 || Char.room == 12))) {
        rect.top = 193;
        rect.left = 218;
        *(struct RectCorner *)&rect.bottom =
            *(const struct RectCorner *)&ScreenPortRect.bottom;
        STROKE_PORT_RECT(&rect);
    } else if (MsgTimerStart != 600) {
        x = 308;
        if (Char.character_kind == 7 || Char.character_kind == 8) {
            half = life % 2;
            life /= 2;
        } else {
            half = 0;
        }
        for (index = 0; index < life; ++index) {
            PrincePaintShapeIdx((int)Pics[2], 131, x, 193, 0, 1);
            x -= 10;
        }
        if ((Char.character_kind == 7 || Char.character_kind == 8) && half) {
            PrincePaintShapeIdx((int)Pics[2], 132, x, 193, 0, 1);
            x -= 10;
            ++index;
        }
        rect.top = 193;
        rect.bottom = ScreenPortRect.bottom;
        for (index = life; index < 8; ++index) {
            rect.left = x;
            rect.right = x + 10;
            STROKE_PORT_RECT(&rect);
            x -= 10;
        }
    }
}

extern struct {
    signed char argument, entrance_mode, sword_kind, death_countdown;
    int field_4, shadow_timer;
} KidInfo;

void draw_sword(TileDrawArgs *args)
{
    TileDrawArgs base;
    unsigned char kind;

    if (DrawFunc->item && (Kid.room != 9 || ((unsigned char)Level.level_number) != 8)) {
        base = *args;
        base.info.modifier &= 0xfffc;
        DrawFunc->item(&base);
    }
    if (args->pass == 1) {
        kind = args->info.modifier & 3;
        add_middle_item(22, kind == (unsigned char)KidInfo.sword_kind || ((unsigned char)KidInfo.sword_kind == 255 && kind == 1) ? 11 : 46,
                        args->column, args->row, 10, 0);
    }
}

extern int far pascal IS_SIX_KIND(char kind);

int is_open_tile(char kind)
{
    if (!IsWall(kind) && !IS_SIX_KIND(kind))
        return 1;
    return 0;
}

int far pascal IsImpassable(char kind)
{
    if (kind == 11 || kind == 15 || kind == 26 || kind == 12 ||
        kind == 13 || kind == 23 || kind == 24)
        return 1;
    return 0;
}

int is_walkable_tile(char kind)
{
    if (is_open_tile(kind) && !IsImpassable(kind) && kind != 6 && kind != 34)
        return 1;
    return 0;
}

int far pascal IsMobFloor(char kind)
{
    if (kind == 11 || kind == 26 || kind == 15)
        return 1;
    return 0;
}

int far pascal IS_SIX_KIND(char kind)
{
    if (kind == 0 || kind == 9 || kind == 33 || kind == 35 || kind == 27 || kind == 37)
        return 1;
    return 0;
}

int far pascal IsWall(char kind)
{
    if (kind == 20 || kind == 2 || kind == 7 || kind == 25 || kind == 43)
        return 1;
    return 0;
}


extern int set_palette(int resource, int first, unsigned char count, unsigned char palette);
extern unsigned char far *ALLOCATE_PINNED_PAYLOAD(long bytes);
extern void CLEAR_PAYLOAD_HEADER(unsigned char far *palette);
extern void READ_DAC_BLOCK(int first, int count, unsigned char far *palette);
extern void WRITE_DAC_BLOCK(int first, int count, const unsigned char far *palette, int update);
extern unsigned int HGETRSRC(unsigned long type, int resource);
extern unsigned char get_total_pals(int resource);
extern void far *RESOLVE_HANDLE_TO_FAR_PAYLOAD(unsigned int resource);
extern void DISPOSE_HANDLE(unsigned int resource);
extern void far *far cdecl _fmemcpy(void far *dst, const void far *src, unsigned int bytes);

void set_default_palette(void)
{
    set_palette(10, 0, 16, 0);
}

void restore_game_play_palette(void)
{
    if (draw_scratch.saved_palette) {
        WRITE_DAC_BLOCK(16, 240, draw_scratch.saved_palette, 1);
        CLEAR_PAYLOAD_HEADER(draw_scratch.saved_palette); draw_scratch.saved_palette = 0;
    }
    if (draw_scratch.saved_text_palette) {
        WRITE_DAC_BLOCK(0, 16, draw_scratch.saved_text_palette, 0);
        CLEAR_PAYLOAD_HEADER(draw_scratch.saved_text_palette); draw_scratch.saved_text_palette = 0;
    }
}

void save_game_play_palette(void)
{
    DrawRect rect;

    if (!draw_scratch.saved_palette) {
        draw_scratch.saved_palette = ALLOCATE_PINNED_PAYLOAD(720L);
        READ_DAC_BLOCK(16, 240, draw_scratch.saved_palette);
        WRITE_DAC_BLOCK(16, 240, 0, 1);
        if (!EMPTYRECT((DrawRect *)&Kid.previous_bounds) &&
            SECTRECT(&rect, (DrawRect *)&Kid.previous_bounds, &GamePlayRect))
            STROKE_PORT_RECT(&rect);
    }
}

typedef struct PaletteColor {
    unsigned char red, green, blue;
} PaletteColor;

void fill_palette_with_color(unsigned char color)
{
    PaletteColor far *palette;
    PaletteColor far *next;
    int remaining;

    palette = (PaletteColor far *)ALLOCATE_PINNED_PAYLOAD(768L);
    READ_DAC_BLOCK(color, 1, (unsigned char far *)palette);
    next = palette + 1; remaining = 255;
    do { *next = *palette; ++next; } while (--remaining);
    if (!draw_scratch.saved_palette) {
        draw_scratch.saved_palette = ALLOCATE_PINNED_PAYLOAD(720L);
        READ_DAC_BLOCK(16, 240, draw_scratch.saved_palette);
    }
    if (!draw_scratch.saved_text_palette) {
        draw_scratch.saved_text_palette = ALLOCATE_PINNED_PAYLOAD(48L);
        READ_DAC_BLOCK(0, 16, draw_scratch.saved_text_palette);
    }
    WRITE_DAC_BLOCK(0, 256, (unsigned char far *)palette, 1);
    CLEAR_PAYLOAD_HEADER((unsigned char far *)palette);
}

int set_palette(int resource, int first, unsigned char count, unsigned char palette)
{
    unsigned char total;
    int success;
    unsigned int handle;
    unsigned char far *colors;

    total = get_total_pals(resource);
    handle = HGETRSRC(0x50414c53UL, resource);
    if (handle && total > palette) {
        colors = (unsigned char far *)RESOLVE_HANDLE_TO_FAR_PAYLOAD(handle);
        if (first < 16) {
            if (draw_scratch.saved_palette) {
                if (count - (total = count < 16 - first ? count : 16 - first) > 0)
                    _fmemcpy(draw_scratch.saved_palette, &colors[(count * palette + total) * 3],
                                       (count - total) * 3);
            } else {
                total = count;
            }
            WRITE_DAC_BLOCK(first, total, &colors[count * palette * 3], 1);
        } else if (draw_scratch.saved_palette) {
            _fmemcpy(draw_scratch.saved_palette + first * 3 - 48, &colors[count * palette * 3], count * 3);
        } else {
            WRITE_DAC_BLOCK(first, count, &colors[count * palette * 3], 1);
        }
        DISPOSE_HANDLE(handle);
        success = 1;
    } else {
        success = 0;
    }
    return success;
}

unsigned char get_total_pals(int resource)
{
    unsigned int handle;
    int total;

    handle = HGETRSRC(0x50414c43UL, resource);
    if (handle) {
        total = *(int far *)RESOLVE_HANDLE_TO_FAR_PAYLOAD(handle);
        DISPOSE_HANDLE(handle);
    } else {
        total = 0;
    }
    return (unsigned char)total;
}


void set_shape_variant(int variant)
{
    ((int far *)RESOLVE_HANDLE_TO_FAR_PAYLOAD(Pics[2]->handle))[2] = variant;
}

extern char PieceAboveId(RoomNumber room, unsigned char piece);
extern char PieceAboveRightId(RoomNumber room, unsigned char piece);

int visible_above_piece(RoomNumber room, unsigned char piece)
{
    int visible = 0;
    char above, upper_left;

    above = PieceAboveId(room, piece);
    if (IS_SIX_KIND(above) || above == 5 || above == 6 ||
        (above == 3 && ((unsigned char)Level.level_number) == 13)) {
        visible = 1;
    } else {
        upper_left = PieceAboveRightId(room, piece);
        if (IS_SIX_KIND(upper_left) || upper_left == 5 || upper_left == 6 ||
            (above == 3 && ((unsigned char)Level.level_number) == 13))
            visible = 1;
    }
    return visible;
}
