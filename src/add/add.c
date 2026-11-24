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

extern ShapeItem *sprite_sets[];
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
extern LEVELREC Level;
extern int background_image_limits[];
extern void DISPOSE_HANDLE(unsigned int handle);
extern int far *MATERIALIZE_HANDLE(unsigned int handle);
extern void load_set_shape(unsigned int, int, ShapeItem far *, int);
extern void MARK_PURGEABLE(unsigned int handle);
extern void PrinceFreeShp(int);
extern TileImages *pieces;
extern BFItem far back_table[];
extern BFItem far front_table[];

extern void check_background_shape(int index);
extern int far *RESOLVE_HANDLE_TO_FAR_PAYLOAD(unsigned int handle);
extern void MAKESHAPERECT(Rect far *, int, int, int far *);
extern int SECTRECT(Rect far *result, Rect far *bounds, Rect far *clip);

int far _fastcall add_image_info(Byte set, ImagePlacement *placement, int image,
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

    shape = RESOLVE_HANDLE_TO_FAR_PAYLOAD(sprite_sets[set][image].handle);
    if (shape == 0 || *shape == 0) return 0;

    item->placement.y += image_y_offsets[y] - *shape;
    item->placement.x += image_x_offsets[x];
    MAKESHAPERECT(&bounds, item->placement.y, item->placement.x, shape);

    if (SECTRECT(&bounds, &bounds, &visible_region)) {
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

    if (i >= 130) return 0;
    added = add_image_info(4, &pieces[tile].background, image,
                           &back_table[i], x, y, order, layer);
    if (added) {
        back_table[i].mode = pieces[tile].mode;
        ++back_count;
    }
    return added;
}


int AddBFImgToBTable(Byte tile, int image, Offset x, Offset y,
                     Byte order, Byte layer)
{
    int i;
    int added = 0;

    if (image) {
        i = back_count;
        if (i >= 130) return 0;
        added = add_image_info(4, &pieces[tile].background_foreground, image,
                               &back_table[i], x, y, order, layer);
        if (added) {
            back_table[i].mode = pieces[tile].mode;
            ++back_count;
        }
    }
    return added;
}

int AddBFImgToFTable(Byte tile, int image, Offset x, Offset y,
                     Byte order, Byte layer)
{
    int i = front_count;
    int added;

    if (i >= 100) return 0;
    added = add_image_info(4, &pieces[tile].background_foreground, image,
                           &front_table[i], x, y, order, layer);
    if (added) {
        front_table[i].mode = pieces[tile].mode;
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
typedef struct {
    signed char index, flipped;                       /* 00-01 */
    int x, y, image;                                   /* 02,04,06 */
    unsigned char image_set;                          /* 08 */
    Rect clip;                              /* 09,0b,0d,0f */
    int depth;                                        /* 11 */
    unsigned char object_type;                        /* 13 */
} FCHARREC;
extern FCHARREC FChar;



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


extern void ConvertX(int x, int *destination);
extern CHARREC Char;
typedef struct {
    unsigned char number;                             /* 00 */
    unsigned char left, right, above, below;           /* 01-04 */
    unsigned char above_left, above_right;             /* 05-06 */
    unsigned char below_left, below_right;             /* 07-08 */
    unsigned char custom;                             /* 09 */
} ROOMREC;
extern ROOMREC Room;
extern void set_object_cell(Byte index);

void far _fastcall add_char_object(Byte kind)
{
    int index;
    CharacterObject *object;

    if (character_object_count < 30) {
        index = character_object_count++;
        object = &character_objects[index];
        object->kind = kind;
        ConvertX(FChar.x, &object->x);
        object->y = FChar.y;
        object->set = FChar.image_set;
        object->image = FChar.image;
        object->flags = FChar.flipped;
        object->clip = FChar.clip;
        object->depth = FChar.depth;
        set_object_cell((Byte)index);
        object->object_id = Char.frame;
        object->object_type = Char.slot;
    }
}

int add_middle_item(Byte tile, int image, Offset x, Offset y,
                    Byte order, Byte layer)
{
    Byte set;
    int i;
    int added;

    if (tile == 10 || tile == 60) set = 1;
    else set = tile == 22 ? 0 : 4;

    i = front_count;
    if (i >= 100) return 0;
    added = add_image_info(set, &pieces[tile].foreground, image,
                           &front_table[i], x, y, order, layer);
    if (added) {
        front_table[i].mode = pieces[tile].mode;
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
extern MidItem far mid_table[];
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
extern CustomRoom *custom_room;
extern ImageSize far *GetCustomPieceSize(Byte index, ImageSize far *size);
extern int far *get_set_image_size(ShapeSet *set, int index, ImageSize far *size);

int far _fastcall add_middle_image(Byte set, int image, int x, int y, Byte order)
{
    int index;
    int image_index;
    ImageSize size;
    int added;

    if (image != 0 && mid_count < 30) {
        image_index = image - 1;
        index = mid_count;
        mid_table[index].x = x;
        if ((Byte)Level.level_number == 5 && Room.number == 10 &&
            set == 4 && custom_room->count > image_index) {
            GetCustomPieceSize(image_index, &size);
        } else {
            if (!get_set_image_size((ShapeSet *)sprite_sets[set], image_index, &size) || !size.height) return 0;
        }

        mid_table[index].y = y - size.height + 1;
        mid_table[index].set = set;
        mid_table[index].image = image_index;
        mid_table[index].clip = FChar.clip;
        mid_table[index].order = order;
        mid_table[index].reserved = 0;
        if (FChar.flipped == 0 && (set != 4 || (signed char)FChar.object_type >= 0)) mid_table[index].visible = 1;
        else mid_table[index].visible = 0;
        mid_table[index].depth = 255;
        ++mid_count;
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
extern PeelEntry peel_entries[];
extern void free_record_pair(unsigned int handle);

extern int peel_count;
extern int EMPTYRECT(Rect far *rect);
extern unsigned int ALLOCATE_CLIPPED_PORT(Rect far *rect);

extern int find_character_peel(Offset actor, int occurrence);
extern void load_actor_slot(Offset actor);
extern int is_char_non_viewable(void);

extern void far mark_foreground(void);
extern void mark_bufs_for_rect(void (far *callback)(void), unsigned int handle, Offset actor);
extern void draw_record_bounds(unsigned int handle);
extern void DISPOSE_BITMAP_PORT(unsigned int handle);
extern int peel_restore_mode;

void far _fastcall add_peel(int y, int bottom, int x, int width, Offset actor, Offset mode)
{
    Rect rect;

    if (peel_count >= 40) return;
    rect.left = y; rect.right = bottom;
    rect.top = x;
    rect.bottom = x + width;
    if (EMPTYRECT(&rect)) return;

    peel_entries[peel_count].handle = ALLOCATE_CLIPPED_PORT(&rect);
    {
        Offset *character = &peel_entries[peel_count].details.character;
        *character = actor;
    }
    {
        Offset *peel_mode = &peel_entries[peel_count].details.mode;
        *peel_mode = mode;
    }
    if (mode != 2 && mode != 3) peel_entries[peel_count].details.retain = 0;
    else peel_entries[peel_count].details.retain = 1;
    ++peel_count;
}

void save_character_peel(Offset actor, Offset occurrence)
{
    int index = find_character_peel(actor, occurrence);
    if (index != -1)
        peel_entries[index].details.retain = 1;
}

void mark_peel_front_buffers(void)
{
    int remaining = peel_count;
    PeelDetails *details = &peel_entries[remaining].details;

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
    if (index < peel_count) {
        do {
            if (found != -1) break;
            if (actor == peel_entries[index].details.character) {
                ++occurrences;
                if (occurrence == occurrences) found = index;
            }
            ++index;
        } while (index < peel_count);
    }
    return found;
}

extern void add_dirty_rectangle(unsigned int handle);

void restore_peels(void)
{
    int keep[40];
    int index = peel_count;
    int *retain = &peel_entries[index].details.retain;
    int *saved = &keep[index];
    int remaining;

    for (; retain -= sizeof(PeelEntry) / sizeof(int), --saved, index--; ) {
        unsigned int handle;
        if ((*saved = *retain) != 0) continue;
        handle = ((unsigned int *)retain)[-2];
        if (peel_restore_mode == 0) add_dirty_rectangle(handle + 16);
        if (((Offset *)retain)[-1] == 2) {
            draw_record_bounds(handle);
            *saved = *retain = 1;
        } else {
            DISPOSE_BITMAP_PORT(handle);
        }
    }
    index = remaining = 0;
    if (index < peel_count) {
        do {
            if (keep[index]) {
                PeelEntry *source = &peel_entries[index];
                peel_entries[remaining] = *source;
                if (peel_entries[remaining].details.character < 100)
                    peel_entries[remaining].details.retain = 0;
                ++remaining;
            }
        } while (++index < peel_count);
    }
    peel_count = remaining;
}

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
typedef struct {
    int image, equipment_image;                       /* 00,02 */
    signed char x, y;                                 /* 04-05 */
    unsigned char flags;                              /* 06 */
} FRAMEREC;
extern FRAMEREC Frame;



extern Offset actor_clip_mode;




extern unsigned int object_position_handle;

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
extern void far _fastcall setup_sword(Byte kind);
extern void setup_flask(void);
extern int far _fastcall setup_opponent(void);

void add_kid(void)
{
    copy_kid_to_char();
    if (Char.room != 0 && (Byte)Char.room == Room.number) {
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
        else if ((Byte)Level.level_number == 5 && Char.room == 3 && Char.frame == 295)
            setup_carpet();
        else if ((unsigned int)Char.frame >= 306 && (unsigned int)Char.frame <= 318)
            setup_flame();
        else if ((unsigned int)Char.frame >= 272 && (unsigned int)Char.frame <= 281)
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
        if ((Byte)Char.room == Room.number) {
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
                goto temple_kid;
            else if (Level.environment_kind == 6)
                setup_guard_smoke();
        } else if (Level.environment_kind == 5) {
temple_kid:
            setup_splash();
        }
    } while (Level.room_objects[Room.number - 1].opponent_count > ++opponent);
}

void setup_char(void)
{
    int equipment;


    init_fchar_rect();
    get_frame_info();
    equipment = get_sword_frame_image((unsigned int)Frame.equipment_image);
    if (Char.frame == 0 || (Frame.image == -1 && equipment == -1))
        return;

    FChar.flipped = Char.direction;
    FChar.depth = Char.field_34;
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
    if (Char.character_kind == 4 && (unsigned int)Char.frame >= 206 && (unsigned int)Char.frame <= 209) {
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
    mark_bufs_for_rect(mark_foreground, (unsigned int)&FChar.clip, -1);
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
    if ((unsigned int)Frame.equipment_image) {
        unsigned int frame = (unsigned int)Frame.equipment_image;
        EquipmentPlacement far *entries = (EquipmentPlacement far *)RESOLVE_HANDLE_TO_FAR_PAYLOAD(object_position_handle);
        FChar.image = entries[frame].image;
        AddFCharX(entries[frame].x);
        FChar.y += entries[frame].y;
        MakeFCharClipRect();
        add_char_object(3);
        mark_bufs_for_rect(mark_foreground, (unsigned int)&FChar.clip, -1);
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

void far _fastcall setup_sword(Byte kind)
{
    unsigned int frame;
    EquipmentPlacement far *entries;

    if (is_char_non_viewable() || (unsigned int)Frame.equipment_image == 0)
        return;
    if ((unsigned int)Frame.equipment_image > 144 &&
        !((unsigned int)Frame.equipment_image >= 200 && (unsigned int)Frame.equipment_image < 251) &&
        !((unsigned int)Frame.equipment_image >= 292 && (unsigned int)Frame.equipment_image <= 304))
        return;
    if (Char.character_kind == 0 && Char.stance == 0 && Char.frame != 133 &&
        !((unsigned int)Char.frame >= 229 && (unsigned int)Char.frame <= 240) &&
        Char.frame != 323 && Char.frame != 324)
        return;

    frame = (unsigned int)Frame.equipment_image;
    entries = (EquipmentPlacement far *)RESOLVE_HANDLE_TO_FAR_PAYLOAD(object_position_handle);
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
        mark_bufs_for_rect(mark_foreground, (unsigned int)&FChar.clip, Char.slot);
}

void setup_flask(void)
{
    unsigned int frame;
    EquipmentPlacement far *entries;

    if ((unsigned int)Frame.equipment_image < 145 || (unsigned int)Frame.equipment_image > 168)
        return;
    frame = (unsigned int)Frame.equipment_image;
    entries = (EquipmentPlacement far *)RESOLVE_HANDLE_TO_FAR_PAYLOAD(object_position_handle);
    if (entries[frame].image == -1)
        return;
    FChar.image = entries[frame].image;
    AddFCharX(entries[frame].x);
    FChar.y += entries[frame].y;
    FChar.image_set = 1;
    MakeFCharClipRect();
    add_char_object(3);
    mark_bufs_for_rect(mark_foreground, (unsigned int)&FChar.clip, Char.slot);
}

void check_background_shape(int index)
{
    int image;

    if (index < 0)
        return;
    if ((image = background_shapes->images[index].handle) &&
        MATERIALIZE_HANDLE(background_shapes->images[index].handle))
        return;
    if ((image = background_shapes->images[index].handle) != 0) {
        DISPOSE_HANDLE(background_shapes->images[index].handle);
        background_shapes->images[index].handle = 0;
    }
    image = index + 1;
    if (index >= background_image_limits[Level.environment_kind])
        image += 200;
    load_set_shape(background_shapes->resource, image,
                   &background_shapes->images[index], 1);
    MARK_PURGEABLE(background_shapes->images[index].handle);
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
    if (kid_shapes->images[index].handle && MATERIALIZE_HANDLE(kid_shapes->images[index].handle))
        return;
    if (kid_shapes->images[index].handle) {
        DISPOSE_HANDLE(kid_shapes->images[index].handle);
        kid_shapes->images[index].handle = 0;
    }
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
           ? opponent_outfit_by_character[(Offset)Char.character_kind] : (Offset)Level.preferred_opponent;
    if (outfit != 5 && outfit != 6 && opponent_overlay_first[outfit] &&
        *opponent_overlay_first[outfit] <= index)
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

int far _fastcall setup_opponent(void)
{
    Byte kind;
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
        mark_bufs_for_rect(mark_foreground, (unsigned int)&Char.bounds, Char.slot);
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

