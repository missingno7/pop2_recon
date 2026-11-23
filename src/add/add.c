#pragma pack(1)

typedef unsigned char Byte;
typedef signed char Offset;

typedef struct {
    int image;
    int x;
    int y;
} ImagePlacement;

typedef struct {
    int top;
    int left;
    int bottom;
    int right;
} Rect;

typedef struct {
    Byte set;
    Byte mode;
    ImagePlacement placement;
    Byte flags;
    Byte palette;
    Rect clip;
    Byte order;
    Byte layer;
} BFItem;

typedef struct {
    int width;
    int height;
    unsigned int handle;
} ShapeItem;

typedef struct {
    Byte mode;
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

extern ShapeItem *shape_sets[];
extern ShapeSet *kid_shapes;
extern ShapeSet *opponent_shapes;
extern int image_y_offsets[];
extern int image_x_offsets[];
extern Rect visible_region;
extern Byte item_flags;
extern Byte item_palette;
extern int back_count;
extern int front_count;
extern ShapeSet *background_shapes;
extern Byte current_level;
extern int background_image_limits[];
extern void release_shape(unsigned int handle);
extern int far *MATERIALIZE_HANDLE(unsigned int handle);
extern void load_set_shape(unsigned int resource, int image,
                           ShapeItem far *entry, int slot);
extern void MARK_PURGEABLE(unsigned int handle);
extern void PrinceFreeShp(int id);
extern TileImages *tile_images;
extern BFItem far back_table[];
extern BFItem far front_table[];
extern _segment back_table_selector;
extern _segment front_table_selector;
extern BFItem _based(back_table_selector) back_rows[];
extern BFItem _based(front_table_selector) front_rows[];

static struct {
    char back[sizeof("BackTable overflow in AddBImgToBTable(). Quit?")];
    char back_foreground[sizeof("BackTable overflow in AddBFImgToBTable(). Quit?")];
    char front_foreground[sizeof("ForeTable overflow in AddBFImgToFTable. Quit?")];
    char front[sizeof("ForeTable Overflow. Quit?")];
} overflow = {
    "BackTable overflow in AddBImgToBTable(). Quit?",
    "BackTable overflow in AddBFImgToBTable(). Quit?",
    "ForeTable overflow in AddBFImgToFTable. Quit?",
    "ForeTable Overflow. Quit?"
};

extern void check_background_shape(int index);
extern int far *RESOLVE_HANDLE_TO_FAR_PAYLOAD(unsigned int handle);
extern void shape_rect(Rect far *bounds, int y, int x, int far *shape);
extern int clip_rect(Rect far *result, Rect far *bounds, Rect far *clip);
extern int confirm_quit(const char far *message);

int far _fastcall add_image_info(Byte set, int image, ImagePlacement *placement,
                               BFItem far *item, Offset x, Offset y,
                               Byte order, Byte layer)
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

    shape = RESOLVE_HANDLE_TO_FAR_PAYLOAD(shape_sets[set][image].handle);
    if (shape == 0 || *shape == 0) return 0;

    item->placement.y += image_y_offsets[y] - *shape;
    item->placement.x += image_x_offsets[x];
    shape_rect(&bounds, item->placement.y, item->placement.x, shape);

    if (clip_rect(&bounds, &bounds, &visible_region)) {
        item->set = set;
        item->flags = item_flags;
        item->palette = item_palette;
        item->clip = bounds;
        item->order = order;
        item->layer = layer;
        result = 1;
    } else {
        result = 0;
    }
    return result;
}

int AddBImgToBTable(Byte tile, int image, Offset x, Offset y, Byte order, Byte layer)
{
    int i = back_count;
    int added;

    if (i >= 130 && confirm_quit(overflow.back)) return 0;
    added = add_image_info(4, image, &tile_images[tile].background,
                           &back_table[i], x, y, order, layer);
    if (added) {
        back_rows[i].mode = tile_images[tile].mode;
        ++back_count;
    }
    return added;
}


int add_back_foreground_to_back(Byte tile, int image, Offset x, Offset y,
                     Byte order, Byte layer)
{
    int i;
    int added = 0;

    if (image) {
        i = back_count;
        if (i >= 130 && confirm_quit(overflow.back_foreground)) return 0;
        added = add_image_info(4, image,
                               &tile_images[tile].background_foreground,
                               &back_table[i], x, y, order, layer);
        if (added) {
            back_rows[i].mode = tile_images[tile].mode;
            ++back_count;
        }
    }
    return added;
}

int add_back_foreground_to_front(Byte tile, int image, Offset x, Offset y,
                     Byte order, Byte layer)
{
    int i = front_count;
    int added;

    if (i >= 100 && confirm_quit(overflow.front_foreground)) return 0;
    added = add_image_info(4, image,
                           &tile_images[tile].background_foreground,
                           &front_table[i], x, y, order, layer);
    if (added) {
        front_rows[i].mode = tile_images[tile].mode;
        ++front_count;
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
    Byte set;
    int image;
    int object_id;
    Byte flags;
    Byte kind;
    Rect clip;
    Byte status;
    int depth;
    Byte object_type;
} CharacterObject;

extern CharacterObject character_objects[];
extern int character_object_count;
extern int actor_x;
extern int actor_y;
extern int actor_image;
extern Byte actor_set;
extern Byte actor_flags;
extern Rect actor_clip;
extern int actor_depth;



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

extern CHARREC Char;

extern void ConvertX(int x, int *destination);
extern void set_object_cell(Byte index);

void far _fastcall add_char_object(Byte kind)
{
    int index;
    CharacterObject *object;

    if (character_object_count < 30) {
        index = character_object_count++;
        object = &character_objects[index];
        object->kind = kind;
        ConvertX(actor_x, &object->x);
        object->y = actor_y;
        object->set = actor_set;
        object->image = actor_image;
        object->flags = actor_flags;
        object->clip = actor_clip;
        object->depth = actor_depth;
        set_object_cell((Byte)index);
        object->object_id = Char.frame;
        object->object_type = Char.slot;
    }
}

int add_foreground_image(Byte tile, int image, Offset x, Offset y,
                    Byte order, Byte layer)
{
    Byte set;
    int i;
    int added;

    if (tile == 10 || tile == 60) set = 1;
    else set = tile == 22 ? 0 : 4;

    i = front_count;
    if (i >= 100 && confirm_quit(overflow.front)) return 0;
    added = add_image_info(set, image, &tile_images[tile].foreground,
                           &front_table[i], x, y, order, layer);
    if (added) {
        front_rows[i].mode = tile_images[tile].mode;
        ++front_count;
    }
    return added;
}

typedef struct {
    int x;
    int y;
    Byte set;
    int image;
    Rect clip;
    Byte order;
    int reserved;
    Byte visible;
    Byte depth;
} MidItem;

typedef struct {
    int height;
    int width;
} ImageSize;

extern int mid_count;
extern _segment mid_table_selector;
extern MidItem _based(mid_table_selector) mid_rows[];
extern Byte drawing_environment;
extern Byte viewed_room;
extern Offset *custom_shapes;
extern signed char background_depth;
extern void get_custom_shape_size(int index, ImageSize far *size);
extern int far *get_set_image_size(ShapeSet *set, int index, ImageSize far *size);

int far _fastcall add_middle_image(Byte set, int image, int x, int y, Byte order)
{
    int index;
    int image_index;
    ImageSize size;
    int added;
    MidItem _based(mid_table_selector) *row;

    if (image != 0 && mid_count < 30) {
        image_index = image - 1;
        index = mid_count;
        row = &mid_rows[index];
        row->x = x;
        if (drawing_environment == 5 && viewed_room == 10 &&
            set == 4 && *custom_shapes > image_index) {
            get_custom_shape_size(image_index, &size);
        } else {
            if (!get_set_image_size((ShapeSet *)shape_sets[set], image_index, &size) || !size.height) return 0;
        }

        row->y = y - size.height + 1;
        row->set = set;
        row->image = image_index;
        row->clip = actor_clip;
        row->order = order;
        row->reserved = 0;
        if (actor_flags == 0 && (set != 4 || background_depth >= 0)) row->visible = 1;
        else row->visible = 0;
        row->depth = 255;
        ++mid_count;
        added = 1;
    } else {
        added = 0;
    }
    return added;
}

typedef struct {
    unsigned int handle;
    int x;
    int y;
} PeelEntry;
extern PeelEntry peel_entries[];
extern int peel_count;
extern void free_record_pair(unsigned int handle);

/* Packed bytes within the first payload word of the peel record. */
#define PEEL_CHARACTER(entry) (((Offset *)&(entry).x)[0])
#define PEEL_MODE(entry) (((Offset *)&(entry).x)[1])

typedef struct {
    Offset character;
    Offset mode;
    int retain;
} PeelDetails;

extern int EMPTYRECT(Rect far *rect);
extern unsigned int ALLOCATE_CLIPPED_PORT(Rect far *rect);
extern int find_character_peel(Offset actor, int occurrence);
extern void load_actor_slot(Offset actor);
extern int is_char_non_viewable(void);

extern void far actor_redraw_callback(void);
extern void mark_bufs_for_rect(void (far *callback)(void), unsigned int handle, Offset actor);
extern void mark_peel_handle(unsigned int handle);
extern void retain_peel(unsigned int handle);
extern void restore_saved_pixels(unsigned int handle);
extern int peel_restore_mode;

void far _fastcall add_peel(int y, int bottom, int x, int width, Offset actor, Offset mode)
{
    Rect rect;

    if (peel_count >= 40) return;
    rect.left = y; rect.right = bottom;
    rect.top = x;
    rect.bottom = x + width;
    if (EMPTYRECT(&rect)) return;

    peel_entries[*(volatile int *)&peel_count].handle = ALLOCATE_CLIPPED_PORT(&rect);
    PEEL_CHARACTER(peel_entries[*(volatile int *)&peel_count]) = actor;
    PEEL_MODE(peel_entries[*(volatile int *)&peel_count]) = mode;
    if (mode != 2 && mode != 3) peel_entries[*(volatile int *)&peel_count].y = 0;
    else peel_entries[*(volatile int *)&peel_count].y = 1;
    ++peel_count;
}

void save_character_peel(Offset actor, Offset occurrence)
{
    int index = find_character_peel(actor, occurrence);
    if (index != -1)
        peel_entries[index].y = 1;
}

void mark_peel_front_buffers(void)
{
    int remaining = peel_count;
    PeelDetails *details = (PeelDetails *)&peel_entries[remaining].x;

    for (; details = (PeelDetails *)((char *)details - sizeof(PeelEntry)),
           remaining--; ) {
        volatile int redraw;
        if (details->retain)
            continue;
        redraw = 1;
        if (details->character >= 0 && details->character < 5) {
            load_actor_slot(details->character);
            if (Char.action == 9 || is_char_non_viewable())
                redraw = 0;
        }
        if (redraw)
            mark_bufs_for_rect(actor_redraw_callback,
                                  ((unsigned int *)details)[-1] + 16,
                                  details->character);
    }
}

int find_character_peel(Offset actor, int occurrence)
{
    int found = -1;
    int index, occurrences;
    PeelDetails *details;

    index = occurrences = 0;
    if (index < peel_count) {
        details = (PeelDetails *)&peel_entries[0].x;
        do {
            if (found != -1) break;
            if (actor == details->character) {
                ++occurrences;
                if (occurrence == occurrences) found = index;
            }
            details = (PeelDetails *)((char *)details + sizeof(PeelEntry));
            ++index;
        } while (index < peel_count);
    }
    return found;
}

void restore_peels(void)
{
    int keep[40];
    int index = peel_count;
    int *retain = &peel_entries[index].y;
    int *saved = &keep[index];
    int remaining;

    for (; retain -= sizeof(PeelEntry) / sizeof(int), --saved, index--; ) {
        unsigned int handle;
        if ((*saved = *retain) != 0) continue;
        handle = ((unsigned int *)retain)[-2];
        if (peel_restore_mode == 0) mark_peel_handle(handle + 16);
        if (((Offset *)retain)[-1] == 2) {
            retain_peel(handle);
            *saved = *retain = 1;
        } else {
            restore_saved_pixels(handle);
        }
    }
    index = remaining = 0;
    if (index < peel_count) {
        do {
            if (keep[index]) {
                peel_entries[remaining] = peel_entries[index];
                if (PEEL_CHARACTER(peel_entries[remaining]) < 100)
                    peel_entries[remaining].y = 0;
                ++remaining;
            }
        } while (++index < *(volatile int *)&peel_count);
    }
    peel_count = remaining;
}

#pragma optimize("g", off)
void clear_peels(void)
{
    int count = peel_count;
    PeelEntry *entry;
    if (count > 0) {
        entry = peel_entries + count;
        do {
            free_record_pair((--entry)->handle);
        } while (--count > 0);
    }
    peel_count = 0;
}
#pragma optimize("g", on)

BFItem far *far _fastcall get_back_table_image_row(int image, Byte flags, Byte palette)
{
    int index = 0;
    BFItem far *result = 0;

    if (index < back_count) {
        do {
            if (result) break;
            if (back_table[index].placement.image == image &&
                back_table[index].palette == palette && back_table[index].flags == flags)
                result = &back_table[index];
        } while (++index < back_count);
    }
    return result;
}




extern Byte actor_force_redraw;
extern struct {
    Byte before_guards[0x17bf];
    struct { Byte before_count[0x34], count, after_count[0x3f]; } rooms[32];
} Level;
extern unsigned int actor_equipment;
extern int actor_shape_index;
extern struct { Offset x, y; } actor_frame_delta;



extern Offset actor_clip_mode;




extern unsigned int object_position_handle;

extern void copy_kid_to_char(void);
extern void check_actor_room(void);
extern void get_frame_info(void);
extern void update_actor_frame(void);
extern int far _fastcall equipment_shape(unsigned int frame);
extern void adjust_actor_x(int delta);
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

extern void draw_temple_guard(void);
extern void setup_splash(void);
extern void adjust_ruins_sword(void);
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
extern void far _fastcall setup_sword(Byte kind);
extern void setup_flask(void);
extern int far _fastcall setup_opponent(Offset opponent);

void add_kid(void)
{
    copy_kid_to_char();
    if (Char.room != 0 && (Byte)Char.room == viewed_room) {
        setup_kid();
        if (Char.action == 99 || Char.sequence_group == 8) {
            setup_comix();
            if (Char.sequence_group == 8)
                Char.sequence_group = 0;
            else
                Char.action = 5;
        }

        if (current_level == 5)
            setup_splash();
        else if (current_level == 1)
            setup_sand();
        else if (drawing_environment == 5 && Char.room == 3 && Char.frame == 295)
            setup_carpet();
        else if ((unsigned int)Char.frame >= 306 && (unsigned int)Char.frame <= 318)
            setup_flame();
        else if ((unsigned int)Char.frame >= 272 && (unsigned int)Char.frame <= 281)
            setup_fireball();

        if (Char.character_kind == 1 && current_level == 6)
            SetupShadowFlame();
        setup_sword(3);
        setup_flask();
    } else if (current_level == 5) {
        setup_splash();
    }
}

void add_opponent(void)
{
    Offset opponent = 0;

    if ((Offset)Level.rooms[viewed_room].count <= 0)
        return;
    do {
        load_actor_slot(opponent);
        if ((Byte)Char.room == viewed_room) {
            if (!setup_opponent(opponent))
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
            else if (current_level == 5)
                goto temple_kid;
            else if (current_level == 6)
                draw_temple_guard();
        } else if (current_level == 5) {
temple_kid:
            setup_splash();
        }
    } while ((Offset)Level.rooms[viewed_room].count > ++opponent);
}

void setup_char(void)
{
    int equipment;


    init_fchar_rect();
    get_frame_info();
    equipment = equipment_shape(actor_equipment);
    if (Char.frame == 0 || (actor_shape_index == -1 && equipment == -1))
        return;

    actor_flags = Char.direction;
    actor_depth = Char.field_34;
    if (Char.character_kind == 0 || Char.character_kind == 6 || Char.character_kind == 1) {
        actor_set = 2;
        actor_image = actor_shape_index;
    } else {
        actor_set = 3;
        actor_image = Char.character_kind == 10 ? equipment : actor_shape_index;
    }

    if ((Char.character_kind == 7 || Char.character_kind == 8) && Char.sequence_group == 1) {
        auto_forward();
        return;
    }
    actor_y = actor_frame_delta.y + Char.y;
    actor_x = AddCharX(actor_frame_delta.x) - 130;
    if (Char.character_kind == 10) {
        setup_flame_sword();
        return;
    }
    if (Char.character_kind == 4 && (unsigned int)Char.frame >= 206 && (unsigned int)Char.frame <= 209) {
        actor_set = 4;
        actor_image += 104;
    }
}

extern CHARREC Opp;
extern Offset FChar;

void setup_comix(void)
{
    unsigned int frame = Char.frame;

    SaveFChar();
    FChar = -1;
    actor_set = 2;
    actor_image = 218;

    if (frame == 185 || (frame >= 106 && frame < 111))
        goto raised;
    if (Char.character_kind == 11) {
        actor_y += 5;
        adjust_actor_x(-8);
        actor_set = 3;
        actor_image = 119;
        goto render;
    }
    if (Char.character_kind == 12) {
        actor_y -= 11;
        adjust_actor_x(-8);
        goto render;
    }
    if (Char.character_kind == 0) {
        if (Opp.character_kind == 11 || Char.frame == 265)
            goto default_x;
        if (Char.frame == 138 || Char.frame == 139 || Char.frame == 140)
            goto lowered;
        if (Char.frame == 338) {
            actor_y -= 30;
            adjust_actor_x(29);
            goto render;
        }
        if (Char.frame == 340) {
            actor_y -= 30;
            adjust_actor_x(-17);
            goto render;
        }
        actor_y -= 15;
        goto default_x;
lowered:
        actor_y -= 10;
        adjust_actor_x(-5);
        goto render;
    }
    actor_y -= 11;
    adjust_actor_x(7);
    goto render;
raised:
    actor_y += 4;
default_x:
    adjust_actor_x(10);
render:
    MakeFCharClipRect();
    add_char_object(5);
    mark_bufs_for_rect(actor_redraw_callback, (unsigned int)&actor_clip, -1);
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
    if (actor_equipment) {
        unsigned int frame = actor_equipment;
        EquipmentPlacement far *entries = (EquipmentPlacement far *)RESOLVE_HANDLE_TO_FAR_PAYLOAD(object_position_handle);
        actor_image = entries[frame].image;
        adjust_actor_x(entries[frame].x);
        actor_y += entries[frame].y;
        MakeFCharClipRect();
        add_char_object(3);
        mark_bufs_for_rect(actor_redraw_callback, (unsigned int)&actor_clip, -1);
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
    check_actor_room();
    add_char_object(Char.character_kind);
}

void far _fastcall setup_sword(Byte kind)
{
    unsigned int frame;
    EquipmentPlacement far *entries;

    if (is_char_non_viewable() || actor_equipment == 0)
        return;
    if (actor_equipment > 144 &&
        !(actor_equipment >= 200 && actor_equipment < 251) &&
        !(actor_equipment >= 264 && actor_equipment <= 265) &&
        !(actor_equipment >= 292 && actor_equipment <= 304))
        return;
    if (Char.character_kind == 0 && Char.stance == 0 && Char.frame != 133 &&
        !((unsigned int)Char.frame >= 229 && (unsigned int)Char.frame <= 240) &&
        Char.frame != 323 && Char.frame != 324)
        return;

    frame = actor_equipment;
    entries = (EquipmentPlacement far *)RESOLVE_HANDLE_TO_FAR_PAYLOAD(object_position_handle);
    actor_image = entries[frame].image;
    if (actor_image == -1)
        return;
    adjust_actor_x(entries[frame].x);
    actor_y += entries[frame].y;
    actor_set = 0;
    MakeFCharClipRect();
    if (current_level == 5)
        adjust_ruins_sword();
    add_char_object(kind);
    if (Char.action != 9)
        mark_bufs_for_rect(actor_redraw_callback, (unsigned int)&actor_clip, Char.slot);
}

void setup_flask(void)
{
    unsigned int frame;
    EquipmentPlacement far *entries;

    if (actor_equipment < 145 || actor_equipment > 168)
        return;
    frame = actor_equipment;
    entries = (EquipmentPlacement far *)RESOLVE_HANDLE_TO_FAR_PAYLOAD(object_position_handle);
    if (entries[frame].image == -1)
        return;
    actor_image = entries[frame].image;
    adjust_actor_x(entries[frame].x);
    actor_y += entries[frame].y;
    actor_set = 1;
    MakeFCharClipRect();
    add_char_object(3);
    mark_bufs_for_rect(actor_redraw_callback, (unsigned int)&actor_clip, Char.slot);
}

void check_background_shape(int index)
{
    volatile int handle;
    int image;

    if (index < 0)
        return;
    if ((handle = background_shapes->images[index].handle) &&
        MATERIALIZE_HANDLE(background_shapes->images[index].handle))
        return;
    if ((handle = background_shapes->images[index].handle) != 0) {
        release_shape(background_shapes->images[index].handle);
        background_shapes->images[index].handle = 0;
    }
    image = index + 1;
    if (index >= background_image_limits[current_level])
        image += 200;
    load_set_shape(background_shapes->resource, image,
                   &background_shapes->images[index], 1);
    MARK_PURGEABLE(background_shapes->images[index].handle);
    PrinceFreeShp(image + 3500);
}

extern unsigned int HGETRSRC(unsigned long resource, int image);
extern Offset opponent_outfit_by_character[];
extern Offset opponent_outfit;
extern int *opponent_extra_image_limits[];
extern ShapeItem far *get_list_item(ShapeSet *set, int image, ImageSize far *size);
extern int far *PrinceGetnShapeItemSize(ShapeSet *set, int image, ImageSize far *size);

void check_kid_shape(int index)
{
    int image;

    if (index < 0 || MATERIALIZE_HANDLE(kid_shapes->images[index].handle))
        return;
    image = kid_shapes->first_image + index + 1;
    if (index >= 222)
        image -= 400;
    if ((index >= 131 && index <= 132) || (index >= 216 && index <= 218))
        load_set_shape(kid_shapes->resource, image, &kid_shapes->images[index], 1);
    else
        kid_shapes->images[index].handle = HGETRSRC(0x53484150UL, image);
    MARK_PURGEABLE(kid_shapes->images[index].handle);
}

void check_opponent_shape(int index)
{
    int image;
    Offset outfit;

    if (index < 0)
        return;
    if (!opponent_shapes->images[index].handle || MATERIALIZE_HANDLE(opponent_shapes->images[index].handle))
        return;
    image = index + 1;
    outfit = (Char.character_kind == 10 || Char.character_kind == 12)
           ? opponent_outfit_by_character[(Offset)Char.character_kind] : opponent_outfit;
    if (outfit != 5 && outfit != 6 && opponent_extra_image_limits[outfit] &&
        *opponent_extra_image_limits[outfit] <= index)
        image += 100;
    load_set_shape(opponent_shapes->resource, image, &opponent_shapes->images[index], 2);
    MARK_PURGEABLE(opponent_shapes->images[index].handle);
    PrinceFreeShp(image + 750);
}

int far *get_set_image_size(ShapeSet *set, int index, ImageSize far *size)
{
    if (set == kid_shapes)
        check_kid_shape(index);
    else if (set == opponent_shapes)
        check_opponent_shape(index);
    else if (set == background_shapes)
        check_background_shape(index);

    size = (ImageSize far *)PrinceGetnShapeItemSize(set, index + 1, size);
    return (int far *)size;
}

ShapeItem far *get_set_shape_item(ShapeSet *set, int image, ImageSize far *size)
{
    if (set == kid_shapes)
        check_kid_shape(image - 1);
    else if (set == opponent_shapes)
        check_opponent_shape(image - 1);
    else if (set == background_shapes)
        check_background_shape(image - 1);

    size = (ImageSize far *)get_list_item(set, image, size);
    return (ShapeItem far *)size;
}

int far _fastcall setup_opponent(Offset opponent)
{
    Byte kind;
    int added;

    reread_cells();
    setup_char();
    uneven_floor();
    get_char_edges();
    IndexChar();
    quick_floor();
    check_actor_room();

    if (!Char.alert_delay && Char.previous_frame == Char.frame &&
        Char.previous_x == Char.x && Char.previous_y == Char.y &&
        Char.previous_mode == 0 && Char.character_kind != 10 && Char.character_kind != 11) {
        save_character_peel(Char.slot, 1);
        save_character_peel(Char.slot, 2);
        added = 0;
        goto done;
    }
    if (Char.character_kind == 7 || Char.character_kind == 8 || Char.character_kind == 10 ||
        Char.character_kind == 6 || Char.character_kind == 1) {
        kind = Char.character_kind;
        if (kind == 10)
            float_flame_sword();
    } else {
        kind = 2;
    }
    if (Char.action != 9 && !is_char_non_viewable())
        mark_bufs_for_rect(actor_redraw_callback, (unsigned int)&Char.bounds, Char.slot);
    add_char_object(kind);
    Char.previous_x = Char.x;
    Char.previous_y = Char.y;
    Char.previous_mode = peel_restore_mode;
    save_actor_slot();
    added = 1;
done:
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

