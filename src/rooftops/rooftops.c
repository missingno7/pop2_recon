/* ROOFTOPS.C: DOS overlay 4. Descriptive names mark unresolved symbols. */
typedef signed char sbyte;
typedef unsigned char ubyte;
typedef unsigned int uword;

#pragma pack(1)
typedef struct {
    int top, left, bottom, right;
} Rect;

typedef struct {
    sbyte kind;
    int y, x;
    ubyte pass;
    ubyte reserved[3];
    int image_set;
    Rect bounds;
    ubyte tail[6];
} DrawPiece;

typedef struct {
    sbyte piece_count;
    ubyte bank;
    ubyte reserved[26];
    DrawPiece pieces[80];
} DrawList;

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

typedef struct {
    sbyte kind;
    ubyte variant;
    int reserved;
    uword frame;
} RoofAnimation;

typedef struct {
    ubyte reserved[18];
    int top;
    int left;
    int bottom;
} Peel;

typedef struct {
    Peel near *peel;
    ubyte reserved[4];
} PeelSlot;

typedef struct {
    sbyte opponent_count;
    ubyte reserved[115];
} Room;

typedef struct {
    ubyte index;
    ubyte reserved;
} SplashImage;
#pragma pack()

extern sbyte water_character;
extern sbyte splash_frame;
extern int splash_x;
extern ubyte splash_low;
extern sbyte palette_bank;
extern ubyte palette_theme;
extern ubyte animation_room;
extern ubyte viewed_room;
extern int fade_active, FrameNum;
extern uword BlueSpec;
extern sbyte animation_tile, animation_remove;
extern int water_clip_bottom, character_clip_bottom, pillar_foreground;
extern int character_width;
extern int delay_count, active_sound_channels, ship_grab_height;
extern CHARREC Char;
extern CHARREC Kid;
extern Rect draw_clip, NullRect, GamePlayRect;
extern SplashImage splash_images[];
extern Room rooms[];
extern PeelSlot custom_peels[];
extern ubyte palette_data[];
extern char quit_prompt[], restart_prompt[];

extern DrawList near *draw_list;
typedef void (far pascal *RedrawCallback)(void);
extern void far pascal redraw_object(void), restore_object(void);
extern int far pascal SECTRECT(Rect far *, Rect far *, Rect far *);
extern void far pascal mark_bufs_for_rect(RedrawCallback, Rect near *, sbyte);
extern void far pascal decrement_life_points(sbyte), play_cutscene_sound(sbyte);
extern int far pascal load_palette(uword, int, ubyte, sbyte);
extern void far pascal CycleColors(int, int, int);
extern int far pascal query_sound(char near *);
extern void far pascal AddSound(int), AddLeaveLevelSong(void), play_song(int);
extern void far pascal wait_sound(int), wait_timer(int), step_scene(void);
extern void far pascal CheckMeters(void), copy_kid_to_char(void);
extern void far pascal reset_char_y(void), JumpSeq(int), flag_sound_resource(char near *);
extern void far pascal load_actor_slot(sbyte), save_actor_slot(void);
extern int far pascal check_trob_visibility(void);
extern void far *far pascal RESOLVE_HANDLE_TO_FAR_PAYLOAD(int);
extern Rect far *far pascal MAKESHAPERECT(Rect far *, int, int, void far *);
extern void far _fastcall draw_custom_room_image(ubyte, ubyte);
extern sbyte far pascal RestoreKeepCustomPeel(ubyte);

void far pascal check_character_water(void);
void far pascal cycle_roof_colors(void);
void far pascal watch_kid(void), watch_opponents(void);
void far pascal trigger_character_water(void);
Rect near *far pascal make_water_test_rect(Rect near *);
int far pascal draw_pillar_in_back(DrawPiece near *);
char far pascal SELECT_PAIR_CODE(char, char);
int far pascal RoofClimbingIntoShip(void);
int far pascal RoofHangingFromShip(void);
void far pascal update_ship_peel(sbyte, int);

int far _fastcall keep_image_in_back(DrawPiece near *piece)
{
    Rect intersection;
    int result;

    if ((piece->kind == 35 || piece->kind == 22 ||
         piece->kind == 72 || piece->kind == 23) &&
        SECTRECT(&intersection, &piece->bounds, (Rect far *)&Kid.bounds) &&
        (int)Kid.sequence != 68 &&
        (Kid.action == 3 || Kid.action == 4))
        result = 1;
    else
        result = 0;
    return result;
}

void far pascal LoadRoofCode(void)
{
    sbyte inactive = -1;

    splash_frame = 0;
    palette_bank = inactive;
    water_character = inactive;
}

void far pascal check_character_water(void)
{
    if (water_character == -1) {
        if (Char.y >= 172 && (int)Char.sequence != 68 && (int)Char.sequence != 15 &&
            (int)Char.sequence != 59 && Char.action != 2)
            trigger_character_water();
    } else if (Char.slot == water_character) {
        if (splash_frame < 7) {
            if (draw_list->bank != 18 && draw_list->bank != 19)
                return;
            mark_bufs_for_rect(redraw_object, &draw_list->pieces[splash_images[splash_frame].index].bounds, -1);
        } else if (splash_frame >= 8) {
            splash_frame = 0;
            water_character = -1;
            if (Char.row >= 2) {
                Char.frame = 185;
                decrement_life_points(100);
                play_cutscene_sound(15);
            }
            return;
        }
        ++splash_frame;
    }
}

void far pascal check_roof_palette(sbyte room)
{
    sbyte bank = -1;

    if (palette_theme == 5 && room != 19 && room != 16 && room != 15) {
        if (palette_bank != 0)
            bank = palette_bank = 0;
    } else if (palette_bank != 1) {
        bank = palette_bank = 1;
    }
    if (bank != -1)
        load_palette((uword)palette_data, 64, 176, bank);
}

void far pascal check_roof_room(void)
{
    check_roof_palette(viewed_room);
}

void far pascal clip_character_at_water(void)
{
    int limit;

    if ((Char.room == 19 || Char.room == 16) && Char.slot == water_character) {
        limit = 172;
        if (Char.action == 9) limit = 186;
        if (limit > water_clip_bottom) limit = water_clip_bottom;
        water_clip_bottom = limit;
        if (limit < character_clip_bottom)
            character_clip_bottom = water_clip_bottom;
    }
}

void far pascal cycle_roof_colors(void)
{
    if (!fade_active && (viewed_room == 19 || viewed_room == 16 || viewed_room == 15) &&
        FrameNum % 3 == 0) {
        CycleColors(230, 3, 0);
        CycleColors(233, 3, 0);
        CycleColors(236, 2, 0);
    }
}

void far pascal roof_frame(void)
{
    cycle_roof_colors();
    watch_kid();
    watch_opponents();
}

void far pascal kill_kid_on_wharf(void)
{
    int hp;

    if (Char.x > 513) {
        if ((active_sound_channels & 1) && !query_sound(quit_prompt) && !query_sound(restart_prompt)) {
            AddSound(32);
            return;
        }
        wait_sound((int)quit_prompt);
        delay_count = 30;
        wait_timer(0);
        play_song(47);
        hp = Char.life_points;
        if (hp > 0) {
            do {
                delay_count = 48;
                decrement_life_points(1);
                Kid = Char;
                step_scene();
                CheckMeters();
                copy_kid_to_char();
                wait_timer(0);
            } while (--hp);
        }
        reset_char_y();
        JumpSeq(71);
        flag_sound_resource(restart_prompt);
    }
}

Rect near *far pascal make_water_test_rect(Rect near *rect)
{
    if (splash_frame && splash_frame <= 7) {
        if (splash_low == 1)
            *(unsigned int *)&rect->top = 186;
        else
            rect->top = 172;
        rect->left = splash_x - 16;
        rect->bottom = rect->top + 1;
        rect->right = splash_x + 16;
    } else {
        *rect = NullRect;
    }
    return rect;
}

void far pascal trigger_character_water(void)
{
    if (!splash_frame) {
        water_character = Char.slot;
        splash_frame = 1;
        splash_low = (Char.action == 9) ? 1 : 0;
        splash_x = Char.x - 130;
        if (Char.direction == 0)
            splash_x -= character_width / 2;
        else
            splash_x += character_width / 2;
        AddSound(48);
    }
}

void far pascal watch_kid(void)
{
    if (Kid.room == 19 || Kid.room == 16 || water_character == 10) {
        copy_kid_to_char();
        check_character_water();
        if (Kid.room == 19 && (RoofHangingFromShip() || RoofClimbingIntoShip())) {
            --Char.x;
            if (RoofClimbingIntoShip() && Char.frame == 149)
                AddLeaveLevelSong();
        }
        Kid = Char;
    }
}

void far pascal watch_opponents(void)
{
    sbyte index;
    sbyte count = rooms[viewed_room].opponent_count;

    if (count) {
        index = 0;
        if (count > 0) {
            do {
                load_actor_slot(index);
                if (Char.room == 19 || Char.room == 16 || index == water_character) {
                    check_character_water();
                    save_actor_slot();
                }
            } while (++index < count);
        }
    }
}

void far pascal setup_splash(void)
{
    Rect clip;
    DrawPiece near *piece;
    ubyte image;

    if (Char.slot == water_character && splash_frame != 0 && splash_frame < 7 &&
        (draw_list->bank == 18 || draw_list->bank == 19)) {
        clip = draw_clip;
        image = splash_images[splash_frame].index;
        piece = &draw_list->pieces[image];
        piece->y = piece->bounds.top - piece->bounds.bottom + 176;
        if (Char.action == 9)
            piece->y += 14;
        piece->x = (piece->bounds.left - piece->bounds.right) / 2 + splash_x;
        MAKESHAPERECT(&piece->bounds, piece->y, piece->x, RESOLVE_HANDLE_TO_FAR_PAYLOAD(piece->image_set));
        draw_clip = piece->bounds;
        piece->pass = 1;
        draw_custom_room_image(image, piece->pass);
        piece->pass = 11;
        if (Char.action != 9)
            mark_bufs_for_rect(restore_object, &piece->bounds, -1);
        draw_clip = clip;
    }
}

void far pascal animate_roof_glass(void)
{
    if (BlueSpec >= 11) {
        animation_remove = -1;
        return;
    }
    ++BlueSpec;
    if (BlueSpec == 1)
        AddSound(49);
    mark_bufs_for_rect(redraw_object, &draw_list->pieces[(ubyte)(BlueSpec + 30)].bounds, -1);
}

void far pascal draw_roof_glass(RoofAnimation near *animation)
{
    Rect clip;
    ubyte image;

    if (animation->kind == 11 && animation->frame < 11) {
        clip = draw_clip;
        draw_clip = GamePlayRect;
        image = (ubyte)(animation->frame + 31);
        draw_custom_room_image(image, 11);
        if (image + 1 < draw_list->piece_count)
            mark_bufs_for_rect(restore_object, &draw_list->pieces[image + 1].bounds, -1);
        draw_clip = clip;
    }
}

void far pascal AnimRoofPillar(void)
{
    Rect water;
    sbyte image;
    int step;

    if (check_trob_visibility()) {
        step = BlueSpec & 15;
        if (step == 7)
            step = 0;
        else
            ++step;
        BlueSpec &= 0xfff0;
        BlueSpec |= step;
        make_water_test_rect(&water);
        image = SELECT_PAIR_CODE(animation_room, animation_tile % 10);
        if (image && (!splash_frame ||
            !SECTRECT(&water, &water, &draw_list->pieces[(ubyte)image].bounds))) {
            mark_bufs_for_rect(redraw_object, &draw_list->pieces[(ubyte)image].bounds, -1);
            mark_bufs_for_rect(redraw_object, &draw_list->pieces[(ubyte)(image + 4)].bounds, -1);
        }
    }
}

void far pascal draw_roof_pillar(RoofAnimation near *animation)
{
    ubyte image;
    uword offset;
    ubyte pass;

    if (animation->kind == 11 || (animation->kind == 0 && pillar_foreground)) {
        image = SELECT_PAIR_CODE(viewed_room, animation->variant);
        if (image) {
            image += (animation->frame >> 1) & 7;
            draw_custom_room_image(image, 11);
            image = image + 4;
            offset = 25 * *(volatile ubyte *)&image;
            pass = !draw_pillar_in_back((DrawPiece near *)((char near *)draw_list + 28 + *(volatile uword *)&offset));
            ((DrawPiece *)((char *)draw_list + *(volatile uword *)&offset + 28))->pass = pass;
            draw_custom_room_image(image, pass);
            ((DrawPiece *)((char *)draw_list + *(volatile uword *)&offset + 28))->pass = 11;
        }
    }
}

int far pascal draw_pillar_in_back(DrawPiece near *piece)
{
    Rect water;
    int result = 0;

    if (splash_frame && splash_low == 1 && piece->kind >= 6 && piece->kind <= 11) {
        make_water_test_rect(&water);
        if (SECTRECT(&water, &water, &piece->bounds))
            result = 1;
    }
    return result;
}

char far pascal SELECT_PAIR_CODE(char first, char second)
{
    char result = 0;
    if (first == 19)
        result = second == 5 ? 39 : 47;
    else if (first == 16)
        result = second == 1 ? 48 : second == 4 ? 61 : 69;
    return result;
}

void far pascal animate_roof_ship(void)
{
    Rect bounds;
    sbyte slot;
    DrawPiece near *piece;

    ++BlueSpec;
    if (BlueSpec > 176) {
        animation_remove = -1;
        return;
    }
    if (viewed_room == animation_room) {
        piece = &draw_list->pieces[55];
        piece->x = -BlueSpec;
        MAKESHAPERECT(&bounds, piece->y, piece->x, RESOLVE_HANDLE_TO_FAR_PAYLOAD(piece->image_set));
        ++bounds.right;
        mark_bufs_for_rect(redraw_object, &bounds, -1);
        slot = RestoreKeepCustomPeel(piece->kind);
        update_ship_peel(slot, bounds.right);

        piece = &draw_list->pieces[(sbyte)(((BlueSpec - 1) & 3) + 62)];
        piece->x = 1 - BlueSpec;
        MAKESHAPERECT(&bounds, piece->y, piece->x, RESOLVE_HANDLE_TO_FAR_PAYLOAD(piece->image_set));
        ++bounds.right;
        mark_bufs_for_rect(redraw_object, &bounds, -1);
    }
}

void far pascal draw_roof_ship(RoofAnimation near *animation)
{
    Rect clip;
    DrawPiece near *piece;
    uword position;
    int ship_x;
    sbyte image;

    if (animation->kind == 11 && animation->frame <= 176) {
        clip = draw_clip;
        draw_clip = GamePlayRect;
        position = animation->frame;
        piece = &draw_list->pieces[55];
        piece->x = -position;
        MAKESHAPERECT(&piece->bounds, piece->y, piece->x, RESOLVE_HANDLE_TO_FAR_PAYLOAD(piece->image_set));
        draw_custom_room_image(55, 11);

        piece = &draw_list->pieces[61];
        ship_x = position;
        piece->x = 45 - ship_x;
        MAKESHAPERECT(&piece->bounds, piece->y, piece->x, RESOLVE_HANDLE_TO_FAR_PAYLOAD(piece->image_set));
        piece->pass = 1;
        draw_custom_room_image(61, 1);
        piece->pass = 11;

        image = (position & 3) + 62;
        piece = &draw_list->pieces[image];
        piece->x = -ship_x;
        MAKESHAPERECT(&piece->bounds, piece->y, piece->x, RESOLVE_HANDLE_TO_FAR_PAYLOAD(piece->image_set));
        draw_custom_room_image(image, 11);
        draw_clip = clip;
    }
}


int far pascal RoofClimbingIntoShip(void)
{
    int result;
    if (Char.room == 19 && (int)Char.sequence == 59)
        result = 1;
    else
        result = 0;
    return result;
}

int far pascal grab_roof_ship(void)
{
    int result = 0;
    int height;

    if (Char.room == 19 && Char.row < 3) {
        height = ship_grab_height;
        if (Char.x < 208 && height < 32)
            result = 1;
    }
    return result;
}


int far pascal RoofHangingFromShip(void)
{
    int result;

    if (Char.room == 19 && Char.x < 208 &&
        (Char.action == 2 || Char.action == 6 || Char.frame == 80))
        result = 1;
    else
        result = 0;
    return result;
}

void far pascal update_ship_peel(sbyte slot, int bottom)
{
    Peel near *peel;

    if (slot >= 0) {
        peel = custom_peels[slot].peel;
        if (peel->top < bottom && peel->bottom > bottom)
            peel->bottom = bottom;
    }
}
