#include "types.h"
#include "level.h"
#include "char.h"
#pragma pack(1)

typedef signed char Offset;

typedef struct {
    int image;
    int x;
    int y;
} ImagePlacement;

typedef struct {
    byte set;
    byte mode;
    ImagePlacement placement;
    byte flags;
    byte palette;
    Rect clip;
    byte order;
    byte layer;
} BFItem;

typedef struct {
    int width;
    int height;
    unsigned int handle;
} ShapeItem;

typedef struct {
    byte mode;
    ImagePlacement background;
    ImagePlacement background_foreground;
    ImagePlacement foreground;
} TileImages;

typedef struct {
    unsigned int resource;
    int first_image;
    int unused;
    ShapeItem images[1];
} ShapeSet;

extern ShapeSet *Pics[5];
extern int Row2PixelY[5];
extern int Col2PixelX[13];
extern Rect CellClipRect;
extern byte ColNo;
extern byte RowNo;
extern int Ct[5];

extern LEVELREC Level;
extern int background_image_limits[];
extern void DISPOSE_HANDLE(unsigned int);
extern int far *MATERIALIZE_HANDLE(unsigned int);
extern void PrinceNewShpItem(unsigned int, int, ShapeItem far *, int);
extern void MARK_PURGEABLE(unsigned int);
extern void PrinceFreeShp(int);
extern TileImages *pieces;
BFItem far BackTable[130];
BFItem far ForeTable[100];
typedef struct {
    int x;
    int y;
    byte set;
    int image;
    Rect clip;
    byte order;
    int reserved;
    byte visible;
    byte depth;
} MidItem;
MidItem far MidTable[30];

extern void check_background_shape(int);
extern int far *RESOLVE_HANDLE_TO_FAR_PAYLOAD(unsigned int);
extern void MAKESHAPERECT(Rect far *, int, int, int far *);
extern int SECTRECT(Rect far *, Rect far *, Rect far *);

int far _fastcall add_image_info(byte set, ImagePlacement *placement, int image,
                               BFItem far *item, Offset x, Offset y,
                               byte order, byte layer)
{
    Rect bounds;
    int far *shape;
    int result;

    item->placement = *placement;
    if (image != -2) {
        item->placement.image = image;
    } else {
        image = item->placement.image;
    }

    if (set == 4) check_background_shape(image - 1);

    shape = RESOLVE_HANDLE_TO_FAR_PAYLOAD(((ShapeItem *)Pics[set])[image].handle);
    if (shape == 0 || *shape == 0) return 0;

    item->placement.y += Row2PixelY[y + 1] - *shape;
    item->placement.x += Col2PixelX[x + 1];
    MAKESHAPERECT(&bounds, item->placement.y, item->placement.x, shape);

    if (SECTRECT(&bounds, &bounds, &CellClipRect)) {
        item->set = set;
        item->flags = ColNo;
        item->palette = RowNo;
        item->clip = bounds;
        item->order = order;
        item->layer = layer;
        result = 1;
    } else {
        result = 0;
    }
    return result;
}

int AddBImgToBTable(byte tile, int image, Offset x, Offset y, byte order, byte layer)
{
    int i = Ct[0];
    int added;

    if (i >= 130) return 0;
    added = add_image_info(4, &pieces[tile].background, image,
                           &BackTable[i], x, y, order, layer);
    if (added) {
        BackTable[i].mode = pieces[tile].mode;
        ++Ct[0];
    }
    return added;
}

int AddBFImgToBTable(byte tile, int image, Offset x, Offset y,
                     byte order, byte layer)
{
    int i;
    int added = 0;

    if (image) {
        i = Ct[0];
        if (i >= 130) return 0;
        added = add_image_info(4, &pieces[tile].background_foreground, image,
                               &BackTable[i], x, y, order, layer);
        if (added) {
            BackTable[i].mode = pieces[tile].mode;
            ++Ct[0];
        }
    }
    return added;
}

int AddBFImgToFTable(byte tile, int image, Offset x, Offset y,
                     byte order, byte layer)
{
    int i = Ct[1];
    int added;

    if (i >= 100) return 0;
    added = add_image_info(4, &pieces[tile].background_foreground, image,
                           &ForeTable[i], x, y, order, layer);
    if (added) {
        ForeTable[i].mode = pieces[tile].mode;
        ++Ct[1];
    }
    return added;
}

extern void add_kid(void);
extern void add_opponent(void);
extern void init_fchar_rect(void);
extern void CheckMeters(void);

void add_characters(void)
{
    add_kid();
    add_opponent();
    init_fchar_rect();
    CheckMeters();
}

typedef struct {
    int x;
    int y;
    byte set;
    int image;
    int object_id;
    byte flags;
    byte kind;
    Rect clip;
    byte status;
    int depth;
    byte object_type;
} CharacterObject;

CharacterObject ObjTable[30];

extern FCHARREC FChar;

#pragma pack(1)

extern void ConvertX(int, int *);
extern CHARREC Char;

extern ROOMREC Room;
extern void set_object_cell(byte);

void far _fastcall add_char_object(byte kind)
{
    int index;
    CharacterObject *object;

    if (Ct[4] < 30) {
        index = Ct[4]++;
        object = &ObjTable[index];
        object->kind = kind;
        ConvertX(FChar.x, &object->x);
        object->y = FChar.y;
        object->set = FChar.image_set;
        object->image = FChar.image;
        object->flags = FChar.flipped;
        object->clip = FChar.clip;
        object->depth = FChar.depth;
        set_object_cell(index);
        object->object_id = Char.frame;
        object->object_type = Char.slot;
    }
}

int add_middle_item(byte tile, int image, Offset x, Offset y,
                    byte order, byte layer)
{
    byte set;
    int i;
    int added;

    if (tile == 10 || tile == 60) set = 1;
    else set = tile == 22 ? 0 : 4;

    i = Ct[1];
    if (i >= 100) return 0;
    added = add_image_info(set, &pieces[tile].foreground, image,
                           &ForeTable[i], x, y, order, layer);
    if (added) {
        ForeTable[i].mode = pieces[tile].mode;
        ++Ct[1];
    }
    return added;
}

typedef struct {
    int height;
    int width;
} ImageSize;

typedef unsigned int ShapeHandle;
typedef struct {
    signed char shape;
    int x, y;
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
} CustomRoom;
extern CustomRoom *custom_room;
extern ImageSize far *GetCustomPieceSize(byte, ImageSize far *);
extern int far *get_set_image_size(ShapeSet *set, int index, ImageSize far *size);

int far _fastcall add_middle_image(byte set, int image, int x, int y, byte order)
{
    int index;
    int image_index;
    ImageSize size;
    int added;

    if (image != 0 && Ct[3] < 30) {
        image_index = image - 1;
        index = Ct[3];
        MidTable[index].x = x;
        if (Level.level_number == 5 && Room.number == 10 &&
            set == 4 && custom_room->count > image_index) {
            GetCustomPieceSize(image_index, &size);
        } else {
            if (!get_set_image_size(Pics[set], image_index, &size) || !size.height) return 0;
        }

        MidTable[index].y = y - size.height + 1;
        MidTable[index].set = set;
        MidTable[index].image = image_index;
        MidTable[index].clip = FChar.clip;
        MidTable[index].order = order;
        MidTable[index].reserved = 0;
        if (FChar.flipped == 0 && (set != 4 || (signed char)FChar.object_type >= 0)) MidTable[index].visible = 1;
        else MidTable[index].visible = 0;
        MidTable[index].depth = 255;
        ++Ct[3];
        added = 1;
    } else {
        added = 0;
    }
    return added;
}

typedef struct {
    Offset character;
    Offset mode;
    int retain;
} PeelDetails;

typedef struct {
    unsigned int handle;
    PeelDetails details;
} PeelEntry;
struct {
    int count;
    PeelEntry entries[40];
} PeelInfo;
extern void free_record_pair(unsigned int handle);

extern int EMPTYRECT(Rect far *rect);
extern unsigned int ALLOCATE_CLIPPED_PORT(Rect far *rect);

extern int find_character_peel(Offset actor, int occurrence);
extern void load_actor_slot(Offset actor);
extern int is_char_non_viewable(void);

extern void far mark_foreground(void);
extern void mark_bufs_for_rect(void (far *callback)(void), unsigned int handle, Offset actor);
extern void draw_record_bounds(unsigned int handle);
extern void DISPOSE_BITMAP_PORT(unsigned int handle);
extern int NewRoom;

void far _fastcall add_peel(int left, int right, int top, int height, Offset actor, Offset mode)
{
    Rect rect;

    if (PeelInfo.count >= 40) return;
    rect.left = left; rect.right = right;
    rect.top = top;
    rect.bottom = top + height;
    if (EMPTYRECT(&rect)) return;

    PeelInfo.entries[PeelInfo.count].handle = ALLOCATE_CLIPPED_PORT(&rect);
    {
        Offset *character = &PeelInfo.entries[PeelInfo.count].details.character;
        *character = actor;
    }
    {
        Offset *peel_mode = &PeelInfo.entries[PeelInfo.count].details.mode;
        *peel_mode = mode;
    }
    if (mode != 2 && mode != 3) PeelInfo.entries[PeelInfo.count].details.retain = 0;
    else PeelInfo.entries[PeelInfo.count].details.retain = 1;
    ++PeelInfo.count;
}

void save_character_peel(Offset actor, Offset occurrence)
{
    int index = find_character_peel(actor, occurrence);
    if (index != -1)
        PeelInfo.entries[index].details.retain = 1;
}

void mark_peel_front_buffers(void)
{
    int remaining = PeelInfo.count;
    PeelDetails *details = &PeelInfo.entries[remaining].details;

    for (; details = (PeelDetails *)((char *)details - sizeof(PeelEntry)),
           remaining--; ) {
        int redraw;
        if (details->retain)
            continue;
        redraw = 1;
        if (details->character >= 0 && details->character < 5) {
            load_actor_slot(details->character);
            if (Char.action == 9 || is_char_non_viewable())
                redraw = 0;
        }
        if (redraw)
            mark_bufs_for_rect(mark_foreground,
                                  ((unsigned int *)details)[-1] + 16,
                                  details->character);
    }
}

int find_character_peel(Offset actor, int occurrence)
{
    int found = -1;
    int index, occurrences;

    index = occurrences = 0;
    if (index < PeelInfo.count) {
        do {
            if (found != -1) break;
            if (actor == PeelInfo.entries[index].details.character) {
                ++occurrences;
                if (occurrence == occurrences) found = index;
            }
            ++index;
        } while (index < PeelInfo.count);
    }
    return found;
}

extern void add_dirty_rectangle(unsigned int handle);

void restore_peels(void)
{
    int keep[40];
    int index = PeelInfo.count;
    int *retain = &PeelInfo.entries[index].details.retain;
    int *saved = &keep[index];
    int remaining;

    for (; retain -= sizeof(PeelEntry) / sizeof(int), --saved, index--; ) {
        unsigned int handle;
        if ((*saved = *retain) != 0) continue;
        handle = ((unsigned int *)retain)[-2];
        if (NewRoom == 0) add_dirty_rectangle(handle + 16);
        if (((Offset *)retain)[-1] == 2) {
            draw_record_bounds(handle);
            *saved = *retain = 1;
        } else {
            DISPOSE_BITMAP_PORT(handle);
        }
    }
    index = remaining = 0;
    if (index < PeelInfo.count) {
        do {
            if (keep[index]) {
                PeelEntry *source = &PeelInfo.entries[index];
                PeelInfo.entries[remaining] = *source;
                if (PeelInfo.entries[remaining].details.character < 100)
                    PeelInfo.entries[remaining].details.retain = 0;
                ++remaining;
            }
        } while (++index < PeelInfo.count);
    }
    PeelInfo.count = remaining;
}

void clear_peels(void)
{
    int count = PeelInfo.count;
    PeelEntry *entry;
    if (count > 0) {
        entry = PeelInfo.entries + count;
        do {
            free_record_pair((--entry)->handle);
        } while (--count > 0);
    }
    PeelInfo.count = 0;
}

BFItem far *far _fastcall get_back_table_image_row(int image, byte flags, byte palette)
{
    int index = 0;
    BFItem far *result = 0;

    if (index < Ct[0]) {
        do {
            if (result) break;
            if (BackTable[index].placement.image == image &&
                BackTable[index].palette == palette && BackTable[index].flags == flags)
                result = &BackTable[index];
        } while (++index < Ct[0]);
    }
    return result;
}

extern byte actor_force_redraw;

extern FRAMEREC Frame;

extern Offset actor_clip_mode;

extern unsigned int SwordFrames;

extern void copy_kid_to_char(void);
extern void ClipChar(void);
extern void get_frame_info(void);
extern void update_actor_frame(void);
extern int far _fastcall get_sword_frame_image(unsigned int frame);
extern void AddFCharX(int delta);
extern int AddCharX(int delta);
extern void MakeFCharClipRect(void);
extern void reread_cells(void);
extern void uneven_floor(void);
extern void IndexChar(void);
extern void quick_foreground(void);
extern void get_char_edges(void);
extern void quick_floor(void);
extern void save_actor_slot(void);
extern void SaveFChar(void);
extern void RestoreFChar(void);

extern void setup_guard_smoke(void);
extern void setup_splash(void);
extern void clip_character_at_water(void);
extern void draw_rooftop_opponent(void);
extern void setup_carpet(void);
extern void setup_flame(void);
extern void setup_sand(void);
extern void SetupShadowFlame(void);
extern void auto_forward(void);
extern void setup_jinnee_smoke(void);
extern void setup_flame_sword(void);
extern void float_flame_sword(void);

extern void setup_char(void);
extern void setup_comix(void);
extern void setup_fireball(void);
extern void setup_kid(void);
extern void far _fastcall setup_sword(byte kind);
extern void setup_flask(void);
extern int far _fastcall setup_opponent(void);

void add_kid(void)
{
    copy_kid_to_char();
    if (Char.room != 0 && Char.room == Room.number) {
        setup_kid();
        if (Char.action == 99 || Char.sequence_group == 8) {
            setup_comix();
            if (Char.sequence_group == 8)
                Char.sequence_group = 0;
            else
                Char.action = 5;
        }

        if (Level.environment_kind == 5)
            setup_splash();
        else if (Level.environment_kind == 1)
            setup_sand();
        else if (Level.level_number == 5 && Char.room == 3 && Char.frame == 295)
            setup_carpet();
        else if (Char.frame >= 306 && Char.frame <= 318)
            setup_flame();
        else if (Char.frame >= 272 && Char.frame <= 281)
            setup_fireball();

        if (Char.character_kind == 1 && Level.environment_kind == 6)
            SetupShadowFlame();
        setup_sword(3);
        setup_flask();
    } else if (Level.environment_kind == 5) {
        setup_splash();
    }
}

void add_opponent(void)
{
    Offset opponent = 0;

    if (Level.room_objects[Room.number - 1].opponent_count <= 0)
        return;
    do {
        load_actor_slot(opponent);
        if (Char.room == Room.number) {
            if (!setup_opponent())
                continue;
            if ((Char.action == 99 || Char.sequence_group == 8 ||
                 (Char.action == 9 && Char.frame == 213)) &&
                Char.character_kind != 7 && Char.character_kind != 8) {
                if (Char.action == 99)
                    Char.action = 5;
                setup_comix();
            }
            if (Char.character_kind != 10 && Char.character_kind != 1)
                setup_sword(9);
            if (Char.character_kind == 12)
                setup_jinnee_smoke();
            else if (Char.character_kind == 1)
                SetupShadowFlame();
            else if (Level.environment_kind == 5)
                setup_splash();
            else if (Level.environment_kind == 6)
                setup_guard_smoke();
        } else if (Level.environment_kind == 5) {
            setup_splash();
        }
    } while (Level.room_objects[Room.number - 1].opponent_count > ++opponent);
}

void setup_char(void)
{
    int equipment;

    init_fchar_rect();
    get_frame_info();
    equipment = get_sword_frame_image(Frame.equipment_image);
    if (Char.frame == 0 || (Frame.image == -1 && equipment == -1))
        return;

    FChar.flipped = Char.direction;
    FChar.depth = Char.image_bank;
    if (Char.character_kind == 0 || Char.character_kind == 6 || Char.character_kind == 1) {
        FChar.image_set = 2;
        FChar.image = Frame.image;
    } else {
        FChar.image_set = 3;
        FChar.image = Char.character_kind == 10 ? equipment : Frame.image;
    }

    if ((Char.character_kind == 7 || Char.character_kind == 8) && Char.sequence_group == 1) {
        auto_forward();
        return;
    }
    FChar.y = Frame.y + Char.y;
    FChar.x = AddCharX(Frame.x) - 130;
    if (Char.character_kind == 10) {
        setup_flame_sword();
        return;
    }
    if (Char.character_kind == 4 && Char.frame >= 206 && Char.frame <= 209) {
        FChar.image_set = 4;
        FChar.image += 104;
    }
}

extern CHARREC Opp;

void setup_comix(void)
{
    unsigned int frame = Char.frame;

    SaveFChar();
    FChar.index = -1;
    FChar.image_set = 2;
    FChar.image = 218;

    if (frame != 185 && (frame < 106 || frame >= 111)) {
        if (Char.character_kind == 11) {
            FChar.y += 5;
            AddFCharX(-8);
            FChar.image_set = 3;
            FChar.image = 119;
        } else if (Char.character_kind == 12) {
            FChar.y -= 11;
            AddFCharX(-8);
        } else if (Char.character_kind == 0) {
            if (Opp.character_kind == 11 || Char.frame == 265) {
                AddFCharX(10);
            } else if (Char.frame == 138 || Char.frame == 139 || Char.frame == 140) {
                FChar.y -= 10;
                AddFCharX(-5);
            } else if (Char.frame == 338) {
                FChar.y -= 30;
                AddFCharX(29);
            } else if (Char.frame == 340) {
                FChar.y -= 30;
                AddFCharX(-17);
            } else {
                FChar.y -= 15;
                AddFCharX(10);
            }
        } else {
            FChar.y -= 11;
            AddFCharX(7);
        }
    } else {
        FChar.y += 4;
        AddFCharX(12);
    }
    MakeFCharClipRect();
    add_char_object(5);
    mark_bufs_for_rect(mark_foreground, &FChar.clip, -1);
    RestoreFChar();
}

typedef struct {
    int image;
    Offset x;
    Offset y;
} EquipmentPlacement;

void setup_fireball(void)
{
    SaveFChar();
    if (Frame.equipment_image) {
        unsigned int frame = Frame.equipment_image;
        EquipmentPlacement far *entries = (EquipmentPlacement far *)RESOLVE_HANDLE_TO_FAR_PAYLOAD(SwordFrames);
        FChar.image = entries[frame].image;
        AddFCharX(entries[frame].x);
        FChar.y += entries[frame].y;
        MakeFCharClipRect();
        add_char_object(3);
        mark_bufs_for_rect(mark_foreground, &FChar.clip, -1);
    }
    RestoreFChar();
}

void setup_kid(void)
{
    if (Char.frame == 0)
        return;
    reread_cells();
    setup_char();
    uneven_floor();
    get_char_edges();
    IndexChar();
    quick_foreground();
    quick_floor();
    ClipChar();
    add_char_object(Char.character_kind);
}

void far _fastcall setup_sword(byte kind)
{
    unsigned int frame;
    EquipmentPlacement far *entries;

    if (is_char_non_viewable() || Frame.equipment_image == 0)
        return;
    if (Frame.equipment_image > 144 &&
        !(Frame.equipment_image >= 200 && Frame.equipment_image < 251) &&
        !(Frame.equipment_image >= 292 && Frame.equipment_image <= 304))
        return;
    if (Char.character_kind == 0 && Char.stance == 0 && Char.frame != 133 &&
        !(Char.frame >= 229 && Char.frame <= 240) &&
        Char.frame != 323 && Char.frame != 324)
        return;

    frame = Frame.equipment_image;
    entries = (EquipmentPlacement far *)RESOLVE_HANDLE_TO_FAR_PAYLOAD(SwordFrames);
    FChar.image = entries[frame].image;
    if (FChar.image == -1)
        return;
    AddFCharX(entries[frame].x);
    FChar.y += entries[frame].y;
    FChar.image_set = 0;
    MakeFCharClipRect();
    if (Level.environment_kind == 5)
        clip_character_at_water();
    add_char_object(kind);
    if (Char.action != 9)
        mark_bufs_for_rect(mark_foreground, &FChar.clip, Char.slot);
}

void setup_flask(void)
{
    unsigned int frame;
    EquipmentPlacement far *entries;

    if (Frame.equipment_image < 145 || Frame.equipment_image > 168)
        return;
    frame = Frame.equipment_image;
    entries = (EquipmentPlacement far *)RESOLVE_HANDLE_TO_FAR_PAYLOAD(SwordFrames);
    if (entries[frame].image == -1)
        return;
    FChar.image = entries[frame].image;
    AddFCharX(entries[frame].x);
    FChar.y += entries[frame].y;
    FChar.image_set = 1;
    MakeFCharClipRect();
    add_char_object(3);
    mark_bufs_for_rect(mark_foreground, &FChar.clip, Char.slot);
}

void check_background_shape(int index)
{
    int image;

    if (index < 0)
        return;
    if ((image = Pics[4]->images[index].handle) &&
        MATERIALIZE_HANDLE(Pics[4]->images[index].handle))
        return;
    if ((image = Pics[4]->images[index].handle) != 0) {
        DISPOSE_HANDLE(Pics[4]->images[index].handle);
        Pics[4]->images[index].handle = 0;
    }
    image = index + 1;
    if (index >= background_image_limits[Level.environment_kind])
        image += 200;
    PrinceNewShpItem(Pics[4]->resource, image,
                   &Pics[4]->images[index], 1);
    MARK_PURGEABLE(Pics[4]->images[index].handle);
    PrinceFreeShp(image + 3500);
}

extern unsigned int HGETRSRC(unsigned long resource, int image);
extern Offset opponent_outfit_by_character[];
extern int *opponent_overlay_first[];
extern ShapeItem far *get_list_item(ShapeSet *set, int image, ImageSize far *size);
extern int far *PrinceGetnShapeItemSize(ShapeSet *set, int image, ImageSize far *size);

void check_kid_shape(int index)
{
    int image;

    if (index < 0)
        return;
    if (Pics[2]->images[index].handle && MATERIALIZE_HANDLE(Pics[2]->images[index].handle))
        return;
    if (Pics[2]->images[index].handle) {
        DISPOSE_HANDLE(Pics[2]->images[index].handle);
        Pics[2]->images[index].handle = 0;
    }
    image = Pics[2]->first_image + index + 1;
    if (index >= 222)
        image -= 400;
    if ((index >= 131 && index <= 132) || (index >= 216 && index <= 218))
        PrinceNewShpItem(Pics[2]->resource, image, &Pics[2]->images[index], 1);
    else
        Pics[2]->images[index].handle = HGETRSRC(0x53484150UL, image);
    MARK_PURGEABLE(Pics[2]->images[index].handle);
}

void check_opponent_shape(int index)
{
    int image;
    Offset outfit;

    if (index < 0)
        return;
    if (!Pics[3]->images[index].handle || MATERIALIZE_HANDLE(Pics[3]->images[index].handle))
        return;
    image = index + 1;
    outfit = (Char.character_kind == 10 || Char.character_kind == 12)
           ? opponent_outfit_by_character[(Offset)Char.character_kind] : (Offset)Level.preferred_opponent;
    if (outfit != 5 && outfit != 6 && opponent_overlay_first[outfit] &&
        *opponent_overlay_first[outfit] <= index)
        image += 100;
    PrinceNewShpItem(Pics[3]->resource, image, &Pics[3]->images[index], 2);
    MARK_PURGEABLE(Pics[3]->images[index].handle);
    PrinceFreeShp(image + 750);
}

int far *get_set_image_size(ShapeSet *set, int index, ImageSize far *size)
{
    if (set == Pics[2])
        check_kid_shape(index);
    else if (set == Pics[3])
        check_opponent_shape(index);
    else if (set == Pics[4])
        check_background_shape(index);

    size = (ImageSize far *)PrinceGetnShapeItemSize(set, index + 1, size);
    return (int far *)size;
}

ShapeItem far *get_set_shape_item(ShapeSet *set, int image, ImageSize far *size)
{
    if (set == Pics[2])
        check_kid_shape(image - 1);
    else if (set == Pics[3])
        check_opponent_shape(image - 1);
    else if (set == Pics[4])
        check_background_shape(image - 1);

    size = (ImageSize far *)get_list_item(set, image, size);
    return (ShapeItem far *)size;
}

int far _fastcall setup_opponent(void)
{
    byte kind;
    int added;

    reread_cells();
    setup_char();
    uneven_floor();
    get_char_edges();
    IndexChar();
    quick_floor();
    ClipChar();

    if (!Char.alert_delay && Char.previous_frame == Char.frame &&
        Char.previous_x == Char.x && Char.previous_y == Char.y &&
        Char.previous_mode == 0 && Char.character_kind != 10 && Char.character_kind != 11) {
        save_character_peel(Char.slot, 1);
        save_character_peel(Char.slot, 2);
        added = 0;
    } else {
        if (Char.character_kind == 7 || Char.character_kind == 8 || Char.character_kind == 10 ||
            Char.character_kind == 6 || Char.character_kind == 1) {
            kind = Char.character_kind;
            if (kind == 10)
                float_flame_sword();
        } else {
            kind = 2;
        }
        if (Char.action != 9 && !is_char_non_viewable())
            mark_bufs_for_rect(mark_foreground, &Char.bounds, Char.slot);
        add_char_object(kind);
        Char.previous_x = Char.x;
        Char.previous_y = Char.y;
        Char.previous_mode = NewRoom;
        save_actor_slot();
        added = 1;
    }
    return added;
}

extern CHARREC RoomOpp[5];

/* Terminal ADD.C entry. Its one-past-end address still needs a witness. */
void clear_row_handles(void)
{
    char *row = (char *)&RoomOpp[0].previous_x;
    do {
        row += sizeof(CHARREC);
        *(int *)(row - sizeof(CHARREC)) = 0;
    } while (row < (char *)&RoomOpp[0].previous_x + sizeof(RoomOpp));
}
