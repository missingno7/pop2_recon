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
extern int far *find_locked_shape(unsigned int handle);
extern void load_set_shape(unsigned int resource, int image,
                           ShapeItem far *entry, int slot);
extern void retain_shape(unsigned int handle);
extern void remember_loaded_image(int id);
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
extern int far *lock_shape(unsigned int handle);
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

    if (set == 4)
        check_background_shape(image - 1);

    shape = lock_shape(shape_sets[set][image].handle);
    if (shape == 0 || *shape == 0)
        return 0;

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

    if (i >= 130 && confirm_quit(overflow.back))
        return 0;
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
        if (i >= 130 && confirm_quit(overflow.back_foreground))
            return 0;
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

    if (i >= 100 && confirm_quit(overflow.front_foreground))
        return 0;
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
extern void add_auxiliary_character(void);
extern void add_remaining_objects(void);

void add_characters(void)
{
    add_kid();
    add_opponent();
    add_auxiliary_character();
    add_remaining_objects();
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
extern int actor_id;
extern Byte actor_type;

extern void ConvertX(int x, int *destination);
extern void update_char_object(Byte index);

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
        update_char_object((Byte)index);
        object->object_id = actor_id;
        object->object_type = actor_type;
    }
}

int add_foreground_image(Byte tile, int image, Offset x, Offset y,
                    Byte order, Byte layer)
{
    Byte set;
    int i;
    int added;

    if (tile == 10 || tile == 60)
        set = 1;
    else
        set = tile == 22 ? 0 : 4;

    i = front_count;
    if (i >= 100 && confirm_quit(overflow.front))
        return 0;
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
extern Byte special_scene;
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
        if (drawing_environment == 5 && special_scene == 10 &&
            set == 4 && *custom_shapes > image_index) {
            get_custom_shape_size(image_index, &size);
        } else {
            if (!get_set_image_size((ShapeSet *)shape_sets[set], image_index, &size) || !size.height)
                return 0;
        }

        row->y = y - size.height + 1;
        row->set = set;
        row->image = image_index;
        row->clip = actor_clip;
        row->order = order;
        row->reserved = 0;
        if (actor_flags == 0 && (set != 4 || background_depth >= 0))
            row->visible = 1;
        else
            row->visible = 0;
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
extern void free_peel(unsigned int handle);

/* Packed bytes within the first payload word of the peel record. */
#define PEEL_CHARACTER(entry) (((Offset *)&(entry).x)[0])
#define PEEL_MODE(entry) (((Offset *)&(entry).x)[1])

typedef struct {
    Offset character;
    Offset mode;
    int retain;
} PeelDetails;

extern int rectangle_is_empty(Rect far *rect);
extern unsigned int save_peel_rectangle(Rect far *rect);
extern int find_character_peel(Offset actor, int occurrence);
extern void select_actor(Offset actor);
extern int suppress_actor_callback(void);
extern Byte actor_state;
extern void far actor_redraw_callback(void);
extern void schedule_actor_redraw(void (far *callback)(void), unsigned int handle, Offset actor);
extern void mark_peel_handle(unsigned int handle);
extern void retain_peel(unsigned int handle);
extern void restore_saved_pixels(unsigned int handle);
extern int peel_restore_mode;

void far _fastcall add_peel(int y, int bottom, int x, int width, Offset actor, Offset mode)
{
    Rect rect;

    if (peel_count >= 40)
        return;
    rect.left = y;
    rect.right = bottom;
    rect.top = x;
    rect.bottom = x + width;
    if (rectangle_is_empty(&rect))
        return;

    peel_entries[*(volatile int *)&peel_count].handle = save_peel_rectangle(&rect);
    PEEL_CHARACTER(peel_entries[*(volatile int *)&peel_count]) = actor;
    PEEL_MODE(peel_entries[*(volatile int *)&peel_count]) = mode;
    if (mode != 2 && mode != 3)
        peel_entries[*(volatile int *)&peel_count].y = 0;
    else
        peel_entries[*(volatile int *)&peel_count].y = 1;
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
            select_actor(details->character);
            if (actor_state == 9 || suppress_actor_callback())
                redraw = 0;
        }
        if (redraw)
            schedule_actor_redraw(actor_redraw_callback,
                                  ((unsigned int *)details)[-1] + 16,
                                  details->character);
    }
}

int find_character_peel(Offset actor, int occurrence)
{
    int found = -1;
    int index = 0;
    register int occurrences = 0;
    PeelDetails *details;

    if (index < peel_count) {
        details = (PeelDetails *)&peel_entries[0].x;
        do {
            if (found != -1)
                break;
            if (actor == details->character) {
                ++occurrences;
                if (*(volatile int *)&occurrence == occurrences)
                    found = index;
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
    PeelEntry *entry;
    PeelEntry *destination;

    for (; retain -= sizeof(PeelEntry) / sizeof(int), --saved, index--; ) {
        unsigned int handle;
        if ((*saved = *retain) != 0)
            continue;
        handle = ((unsigned int *)retain)[-2];
        if (peel_restore_mode == 0)
            mark_peel_handle(handle + 16);
        if (((Offset *)retain)[-1] == 2) {
            retain_peel(handle);
            *saved = *retain = 1;
        } else {
            restore_saved_pixels(handle);
        }
    }
    remaining = index = 0;
    if (peel_count > index) {
        entry = peel_entries;
        saved = keep;
        destination = entry;
        do {
            if (*saved) {
                *destination = *entry;
                if (PEEL_CHARACTER(*entry) < 100)
                    destination->y = 0;
                ++destination;
                ++remaining;
            }
        } while (++entry, ++saved, index++ < peel_count - 1);
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
            free_peel((--entry)->handle);
        } while (--count > 0);
    }
    peel_count = 0;
}
#pragma optimize("g", on)

BFItem far *far _fastcall get_back_table_image_row(int image, Byte flags, Byte palette)
{
    int index;
    BFItem far *result = 0;

    for (index = 0; index < back_count && result == 0; ++index) {
        if (back_table[index].placement.image == image &&
            back_table[index].palette == palette && back_table[index].flags == flags)
            result = &back_table[index];
    }
    return result;
}

extern Byte actor_room;
extern Byte actor_action;
extern Byte actor_character;
extern Byte actor_sword;
extern Byte actor_force_redraw;
extern Byte active_guard_count[][116];
extern unsigned int actor_equipment;
extern int actor_shape_index;
extern Offset actor_frame_dx;
extern Offset actor_frame_dy;
extern int actor_screen_offset;
extern int actor_position_x;
extern int actor_position_y;
extern Offset actor_clip_mode;
extern Byte opponent_clip_changed;
extern int opponent_previous_frame;
extern int opponent_previous_x;
extern int opponent_previous_y;
extern int opponent_previous_mode;
extern Rect opponent_clip;
extern unsigned int object_position_handle;

extern void select_kid(void);
extern void check_actor_room(void);
extern void set_actor_position(void);
extern void update_actor_frame(void);
extern int far _fastcall equipment_shape(unsigned int frame);
extern void adjust_actor_x(int delta);
extern int actor_world_x(int delta);
extern void update_actor_clip(void);
extern void prepare_actor_display(void);
extern void clip_actor_left(void);
extern void clip_actor_right(void);
extern void adjust_kid_clip(void);
extern void adjust_actor_for_room(void);
extern void adjust_actor_clip_for_tiles(void);
extern void save_opponent_display_state(void);
extern void save_actor_object_state(void);
extern void restore_actor_object_state(void);

extern void draw_temple_guard(void);
extern void draw_temple_kid(void);
extern void adjust_ruins_sword(void);
extern void draw_rooftop_opponent(void);
extern void draw_drink_effect(void);
extern void draw_jump_effect(void);
extern void draw_temple_effect(void);
extern void draw_room_effect(void);
extern void draw_special_actor(void);
extern void draw_opponent_effect(void);
extern void draw_shadow_effect(void);

extern void setup_char(void);
extern void setup_comix(void);
extern void setup_fireball(void);
extern void setup_kid(void);
extern void far _fastcall setup_sword(Byte kind);
extern void setup_flask(void);
extern int far _fastcall setup_opponent(Offset opponent);

void add_kid(void)
{
    select_kid();
    if (actor_room != 0 && actor_room == special_scene) {
        setup_kid();
        if (actor_state == 99 || actor_screen_offset == 8) {
            setup_comix();
            if (actor_screen_offset == 8)
                actor_screen_offset = 0;
            else
                actor_state = 5;
        }

        if (current_level == 5)
            draw_temple_kid();
        else if (current_level == 1)
            draw_temple_effect();
        else if (drawing_environment == 5 && actor_room == 3 && actor_id == 295)
            draw_drink_effect();
        else if ((unsigned int)actor_id >= 306 && (unsigned int)actor_id <= 318)
            draw_jump_effect();
        else if ((unsigned int)actor_id >= 272 && (unsigned int)actor_id <= 281)
            setup_fireball();

        if (actor_character == 1 && current_level == 6)
            draw_room_effect();
        setup_sword(3);
        setup_flask();
    } else if (current_level == 5) {
        draw_temple_kid();
    }
}

void add_opponent(void)
{
    Offset opponent = 0;

    if ((Offset)active_guard_count[special_scene][0] <= 0)
        return;
    do {
        select_actor(opponent);
        if (actor_room == special_scene) {
            if (!setup_opponent(opponent))
                continue;
            if ((actor_state == 99 || actor_screen_offset == 8 ||
                 (actor_state == 9 && actor_id == 213)) &&
                actor_character != 7 && actor_character != 8) {
                if (actor_state == 99)
                    actor_state = 5;
                setup_comix();
            }
            if (actor_character != 10 && actor_character != 1)
                setup_sword(9);
            if (actor_character == 12)
                draw_opponent_effect();
            else if (actor_character == 1)
                draw_room_effect();
            else if (current_level == 5)
                goto temple_kid;
            else if (current_level == 6)
                draw_temple_guard();
        } else if (current_level == 5) {
temple_kid:
            draw_temple_kid();
        }
    } while ((Offset)active_guard_count[special_scene][0] > ++opponent);
}

void setup_char(void)
{
    int equipment;
    extern int actor_depth_source;

    add_auxiliary_character();
    set_actor_position();
    equipment = equipment_shape(actor_equipment);
    if (actor_id == 0 || (actor_shape_index == -1 && equipment == -1))
        return;

    actor_flags = actor_action;
    actor_depth = actor_depth_source;
    if (actor_character == 0 || actor_character == 6 || actor_character == 1) {
        actor_set = 2;
        actor_image = actor_shape_index;
    } else {
        actor_set = 3;
        actor_image = actor_character == 10 ? equipment : actor_shape_index;
    }

    if ((actor_character == 7 || actor_character == 8) && actor_screen_offset == 1) {
        draw_special_actor();
        return;
    }
    actor_y = actor_frame_dy + actor_position_y;
    actor_x = actor_world_x(actor_frame_dx) - 130;
    if (actor_character == 10) {
        draw_shadow_effect();
        return;
    }
    if (actor_character == 4 && (unsigned int)actor_id >= 206 && (unsigned int)actor_id <= 209) {
        actor_set = 4;
        actor_image += 104;
    }
}

extern Byte previous_actor_character;
extern Offset actor_object_type;

void setup_comix(void)
{
    unsigned int frame = actor_id;

    save_actor_object_state();
    actor_object_type = -1;
    actor_set = 2;
    actor_image = 218;

    if (frame == 185 || (frame >= 106 && frame < 111))
        goto raised;
    if (actor_character == 11) {
        actor_y += 5;
        adjust_actor_x(-8);
        actor_set = 3;
        actor_image = 119;
        goto render;
    }
    if (actor_character == 12) {
        actor_y -= 11;
        adjust_actor_x(-8);
        goto render;
    }
    if (actor_character == 0) {
        if (previous_actor_character == 11 || actor_id == 265)
            goto default_x;
        if (actor_id == 138 || actor_id == 139 || actor_id == 140)
            goto lowered;
        if (actor_id == 338) {
            actor_y -= 30;
            adjust_actor_x(29);
            goto render;
        }
        if (actor_id == 340) {
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
    update_actor_clip();
    add_char_object(5);
    schedule_actor_redraw(actor_redraw_callback, (unsigned int)&actor_clip, -1);
    restore_actor_object_state();
}

typedef struct {
    int image;
    Offset x;
    Offset y;
} EquipmentPlacement;

void setup_fireball(void)
{
    save_actor_object_state();
    if (actor_equipment) {
        unsigned int frame = actor_equipment;
        EquipmentPlacement far *entries = (EquipmentPlacement far *)lock_shape(object_position_handle);
        actor_image = entries[frame].image;
        adjust_actor_x(entries[frame].x);
        actor_y += entries[frame].y;
        update_actor_clip();
        add_char_object(3);
        schedule_actor_redraw(actor_redraw_callback, (unsigned int)&actor_clip, -1);
    }
    restore_actor_object_state();
}

void setup_kid(void)
{
    if (actor_id == 0)
        return;
    prepare_actor_display();
    setup_char();
    clip_actor_left();
    adjust_actor_for_room();
    clip_actor_right();
    adjust_kid_clip();
    adjust_actor_clip_for_tiles();
    check_actor_room();
    add_char_object(actor_character);
}

void far _fastcall setup_sword(Byte kind)
{
    unsigned int frame;
    EquipmentPlacement far *entries;

    if (suppress_actor_callback() || actor_equipment == 0)
        return;
    if (actor_equipment > 144 &&
        !(actor_equipment >= 200 && actor_equipment < 251) &&
        !(actor_equipment >= 264 && actor_equipment <= 265) &&
        !(actor_equipment >= 292 && actor_equipment <= 304))
        return;
    if (actor_character == 0 && actor_sword == 0 && actor_id != 133 &&
        !((unsigned int)actor_id >= 229 && (unsigned int)actor_id <= 240) &&
        actor_id != 323 && actor_id != 324)
        return;

    frame = actor_equipment;
    entries = (EquipmentPlacement far *)lock_shape(object_position_handle);
    actor_image = entries[frame].image;
    if (actor_image == -1)
        return;
    adjust_actor_x(entries[frame].x);
    actor_y += entries[frame].y;
    actor_set = 0;
    update_actor_clip();
    if (current_level == 5)
        adjust_ruins_sword();
    add_char_object(kind);
    if (actor_state != 9)
        schedule_actor_redraw(actor_redraw_callback, (unsigned int)&actor_clip, actor_type);
}

void setup_flask(void)
{
    unsigned int frame;
    EquipmentPlacement far *entries;

    if (actor_equipment < 145 || actor_equipment > 168)
        return;
    frame = actor_equipment;
    entries = (EquipmentPlacement far *)lock_shape(object_position_handle);
    if (entries[frame].image == -1)
        return;
    actor_image = entries[frame].image;
    adjust_actor_x(entries[frame].x);
    actor_y += entries[frame].y;
    actor_set = 1;
    update_actor_clip();
    add_char_object(3);
    schedule_actor_redraw(actor_redraw_callback, (unsigned int)&actor_clip, actor_type);
}

void check_background_shape(int index)
{
    unsigned int handle;
    int image;

    if (index < 0)
        return;
    if ((handle = background_shapes->images[index].handle) &&
        find_locked_shape(background_shapes->images[index].handle))
        return;
    if ((handle = background_shapes->images[index].handle) != 0) {
        release_shape(handle);
        background_shapes->images[index].handle = 0;
    }
    image = index + 1;
    if (index >= background_image_limits[current_level])
        image += 200;
    load_set_shape(background_shapes->resource, image,
                   &background_shapes->images[index], 1);
    retain_shape(background_shapes->images[index].handle);
    remember_loaded_image(image + 3500);
}

extern unsigned int find_named_shape(unsigned long resource, int image);
extern Offset opponent_outfit_by_character[];
extern Offset opponent_outfit;
extern int *opponent_extra_image_limits[];
extern ShapeItem far *read_shape_item(ShapeSet *set, int image, ImageSize far *size);
extern int far *read_shape_size(ShapeSet *set, int image, ImageSize far *size);

void check_kid_shape(int index)
{
    int image;

    if (index < 0 || find_locked_shape(kid_shapes->images[index].handle))
        return;
    image = kid_shapes->first_image + index + 1;
    if (index >= 222)
        image -= 400;
    if ((index >= 131 && index <= 132) || (index >= 216 && index <= 218))
        load_set_shape(kid_shapes->resource, image, &kid_shapes->images[index], 1);
    else
        kid_shapes->images[index].handle = find_named_shape(0x53484150UL, image);
    retain_shape(kid_shapes->images[index].handle);
}

void check_opponent_shape(int index)
{
    int image;
    unsigned int handle;
    Offset outfit;

    if (index < 0)
        return;
    handle = opponent_shapes->images[index].handle;
    if (!handle || find_locked_shape(handle))
        return;
    image = index + 1;
    outfit = (actor_character == 10 || actor_character == 12)
           ? opponent_outfit_by_character[(Offset)actor_character] : opponent_outfit;
    if (outfit != 5 && outfit != 6 && opponent_extra_image_limits[outfit] &&
        *opponent_extra_image_limits[outfit] <= index)
        image += 100;
    load_set_shape(opponent_shapes->resource, image, &opponent_shapes->images[index], 2);
    retain_shape(opponent_shapes->images[index].handle);
    remember_loaded_image(image + 750);
}

int far *get_set_image_size(ShapeSet *set, int index, ImageSize far *size)
{
    if (set == kid_shapes)
        check_kid_shape(index);
    else if (set == opponent_shapes)
        check_opponent_shape(index);
    else if (set == background_shapes)
        check_background_shape(index);

    size = (ImageSize far *)read_shape_size(set, index + 1, size);
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

    size = (ImageSize far *)read_shape_item(set, image, size);
    return (ShapeItem far *)size;
}

int far _fastcall setup_opponent(Offset opponent)
{
    Byte kind;

    prepare_actor_display();
    setup_char();
    clip_actor_left();
    adjust_actor_for_room();
    clip_actor_right();
    adjust_actor_clip_for_tiles();
    check_actor_room();

    if (!opponent_clip_changed && actor_id == opponent_previous_frame &&
        actor_position_x == opponent_previous_x && actor_position_y == opponent_previous_y &&
        opponent_previous_mode == 0 && actor_character != 10 && actor_character != 11) {
        save_character_peel(actor_type, 1);
        save_character_peel(actor_type, 2);
        return 0;
    }
    if (actor_character == 7 || actor_character == 8 || actor_character == 10 ||
        actor_character == 6 || actor_character == 1) {
        kind = actor_character;
        if (kind == 10)
            draw_special_actor();
    } else {
        kind = 2;
    }
    if (actor_state != 9 && !suppress_actor_callback())
        schedule_actor_redraw(actor_redraw_callback, (unsigned int)&opponent_clip, actor_type);
    add_char_object(kind);
    opponent_previous_x = actor_position_x;
    opponent_previous_y = actor_position_y;
    opponent_previous_mode = peel_restore_mode;
    save_opponent_display_state();
    return 1;
}

typedef struct {
    unsigned int handle;
    Byte state[62];
} Row;

extern Row rows[5];

/* Terminal ADD.C entry. Its one-past-end address still needs a witness. */
void clear_row_handles(void)
{
    Row *row = rows;
    do {
        ++row;
        row[-1].handle = 0;
    } while (row < rows + 5);
}

