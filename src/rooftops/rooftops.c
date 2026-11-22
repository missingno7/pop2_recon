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

typedef struct {
    sbyte id;
    sbyte fields[63];
} CharacterImage;

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
extern ubyte room_number;
extern int fade_active, frame_number;
extern uword roof_animation;
extern sbyte animation_tile, animation_remove;
extern int water_clip_bottom, character_clip_bottom, pillar_foreground;
extern int character_width;
extern int delay_count, scene_flags, ship_grab_height;
extern sbyte char_direction, char_life, char_hp;
extern int char_y;
extern signed char state_byte;
extern int state_word;
extern signed int POSITION_WORD;
extern unsigned char STATE_KIND;
extern int STATE_CODE;
extern ubyte kid_room;
extern int draw_char_frame;
extern sbyte draw_char_action;
extern Rect draw_clip, character_clip, empty_rect, ship_clip;
extern CharacterImage current_character, saved_kid;
extern DrawList near *draw_list;
extern SplashImage splash_images[];
extern Room rooms[];
extern PeelSlot custom_peels[];
extern ubyte palette_data[];
extern char quit_prompt[], restart_prompt[];

typedef void (far pascal *RedrawCallback)(void);
extern void far pascal redraw_object(void), restore_object(void);
extern int far pascal intersect_rect(Rect far *, Rect far *, Rect far *);
extern void far pascal mark_redraw(RedrawCallback, Rect near *, sbyte);
extern void far pascal change_hp(sbyte), play_cutscene_sound(sbyte);
extern int far pascal load_palette(uword, int, ubyte, sbyte);
extern void far pascal rotate_palette(int, int, int);
extern int far pascal show_prompt(char near *);
extern void far pascal play_sound(int), stop_ship_sound(void), misc_sound(int);
extern void far pascal set_sound(int), wait_timer(int), step_scene(void);
extern void far pascal CheckMeters(void), animate_character(void);
extern void far pascal reset_player(void), start_sequence(int), hide_prompt(char near *);
extern void far pascal select_opponent(sbyte), restore_character(void);
extern int far pascal update_animation(void);
extern void far *far pascal get_shape(int);
extern Rect far *far pascal offset_shape_rect(Rect far *, int, int, void far *);
extern void far _fastcall draw_image(ubyte, ubyte);
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
        intersect_rect(&intersection, &piece->bounds, &character_clip) &&
        draw_char_frame != 68 &&
        (draw_char_action == 3 || draw_char_action == 4))
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
        if (char_y >= 172 && state_word != 68 && state_word != 15 &&
            state_word != 59 && STATE_KIND != 2)
            trigger_character_water();
    } else if (current_character.id == water_character) {
        if (splash_frame < 7) {
            if (draw_list->bank != 18 && draw_list->bank != 19)
                return;
            mark_redraw(redraw_object, &draw_list->pieces[splash_images[splash_frame].index].bounds, -1);
        } else if (splash_frame >= 8) {
            splash_frame = 0;
            water_character = -1;
            if (char_hp >= 2) {
                STATE_CODE = 185;
                change_hp(100);
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
    check_roof_palette(room_number);
}

void far pascal clip_character_at_water(void)
{
    int limit;

    if ((state_byte == 19 || state_byte == 16) && current_character.id == water_character) {
        limit = 172;
        if (STATE_KIND == 9) limit = 186;
        if (limit > water_clip_bottom) limit = water_clip_bottom;
        water_clip_bottom = limit;
        if (limit < character_clip_bottom)
            character_clip_bottom = water_clip_bottom;
    }
}

void far pascal cycle_roof_colors(void)
{
    if (!fade_active && (room_number == 19 || room_number == 16 || room_number == 15) &&
        frame_number % 3 == 0) {
        rotate_palette(230, 3, 0);
        rotate_palette(233, 3, 0);
        rotate_palette(236, 2, 0);
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

    if (POSITION_WORD > 513) {
        if ((scene_flags & 1) && !show_prompt(quit_prompt) && !show_prompt(restart_prompt)) {
            play_sound(32);
            return;
        }
        set_sound((int)quit_prompt);
        delay_count = 30;
        wait_timer(0);
        misc_sound(47);
        hp = char_life;
        if (hp > 0) {
            do {
                delay_count = 48;
                change_hp(1);
                saved_kid = current_character;
                step_scene();
                CheckMeters();
                animate_character();
                wait_timer(0);
            } while (--hp);
        }
        reset_player();
        start_sequence(71);
        hide_prompt(restart_prompt);
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
        *rect = empty_rect;
    }
    return rect;
}

void far pascal trigger_character_water(void)
{
    if (!splash_frame) {
        water_character = current_character.id;
        splash_frame = 1;
        splash_low = (STATE_KIND == 9) ? 1 : 0;
        splash_x = POSITION_WORD - 130;
        if (char_direction == 0)
            splash_x -= character_width / 2;
        else
            splash_x += character_width / 2;
        play_sound(48);
    }
}

void far pascal watch_kid(void)
{
    if (kid_room == 19 || kid_room == 16 || water_character == 10) {
        animate_character();
        check_character_water();
        if (kid_room == 19 && (RoofHangingFromShip() || RoofClimbingIntoShip())) {
            --POSITION_WORD;
            if (RoofClimbingIntoShip() && STATE_CODE == 149)
                stop_ship_sound();
        }
        saved_kid = current_character;
    }
}

void far pascal watch_opponents(void)
{
    sbyte index;
    sbyte count = rooms[room_number].opponent_count;

    if (count) {
        index = 0;
        if (count > 0) {
            do {
                select_opponent(index);
                if (state_byte == 19 || state_byte == 16 || index == water_character) {
                    check_character_water();
                    restore_character();
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

    if (current_character.id == water_character && splash_frame != 0 && splash_frame < 7 &&
        (draw_list->bank == 18 || draw_list->bank == 19)) {
        clip = draw_clip;
        image = splash_images[splash_frame].index;
        piece = &draw_list->pieces[image];
        piece->y = piece->bounds.top - piece->bounds.bottom + 176;
        if (STATE_KIND == 9)
            piece->y += 14;
        piece->x = (piece->bounds.left - piece->bounds.right) / 2 + splash_x;
        offset_shape_rect(&piece->bounds, piece->y, piece->x, get_shape(piece->image_set));
        draw_clip = piece->bounds;
        piece->pass = 1;
        draw_image(image, piece->pass);
        piece->pass = 11;
        if (STATE_KIND != 9)
            mark_redraw(restore_object, &piece->bounds, -1);
        draw_clip = clip;
    }
}

void far pascal animate_roof_glass(void)
{
    if (roof_animation >= 11) {
        animation_remove = -1;
        return;
    }
    ++roof_animation;
    if (roof_animation == 1)
        play_sound(49);
    mark_redraw(redraw_object, &draw_list->pieces[(ubyte)(roof_animation + 30)].bounds, -1);
}

void far pascal draw_roof_glass(RoofAnimation near *animation)
{
    Rect clip;
    ubyte image;

    if (animation->kind == 11 && animation->frame < 11) {
        clip = draw_clip;
        draw_clip = ship_clip;
        image = (ubyte)(animation->frame + 31);
        draw_image(image, 11);
        if (image + 1 < draw_list->piece_count)
            mark_redraw(restore_object, &draw_list->pieces[image + 1].bounds, -1);
        draw_clip = clip;
    }
}

void far pascal AnimRoofPillar(void)
{
    Rect water;
    sbyte image;
    int step;

    if (update_animation()) {
        step = roof_animation & 15;
        if (step == 7)
            step = 0;
        else
            ++step;
        roof_animation &= 0xfff0;
        roof_animation |= step;
        make_water_test_rect(&water);
        image = SELECT_PAIR_CODE(animation_room, animation_tile % 10);
        if (image && (!splash_frame ||
            !intersect_rect(&water, &water, &draw_list->pieces[(ubyte)image].bounds))) {
            mark_redraw(redraw_object, &draw_list->pieces[(ubyte)image].bounds, -1);
            mark_redraw(redraw_object, &draw_list->pieces[(ubyte)(image + 4)].bounds, -1);
        }
    }
}

void far pascal draw_roof_pillar(RoofAnimation near *animation)
{
    ubyte image;
    uword offset;
    ubyte pass;

    if (animation->kind == 11 || (animation->kind == 0 && pillar_foreground)) {
        image = SELECT_PAIR_CODE(room_number, animation->variant);
        if (image) {
            image += (animation->frame >> 1) & 7;
            draw_image(image, 11);
            image = image + 4;
            offset = 25 * *(volatile ubyte *)&image;
            pass = !draw_pillar_in_back((DrawPiece near *)((char near *)draw_list + 28 + *(volatile uword *)&offset));
            ((DrawPiece *)((char *)draw_list + *(volatile uword *)&offset + 28))->pass = pass;
            draw_image(image, pass);
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
        if (intersect_rect(&water, &water, &piece->bounds))
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

    ++roof_animation;
    if (roof_animation > 176) {
        animation_remove = -1;
        return;
    }
    if (room_number == animation_room) {
        piece = &draw_list->pieces[55];
        piece->x = -roof_animation;
        offset_shape_rect(&bounds, piece->y, piece->x, get_shape(piece->image_set));
        ++bounds.right;
        mark_redraw(redraw_object, &bounds, -1);
        slot = RestoreKeepCustomPeel(piece->kind);
        update_ship_peel(slot, bounds.right);

        piece = &draw_list->pieces[(sbyte)(((roof_animation - 1) & 3) + 62)];
        piece->x = 1 - roof_animation;
        offset_shape_rect(&bounds, piece->y, piece->x, get_shape(piece->image_set));
        ++bounds.right;
        mark_redraw(redraw_object, &bounds, -1);
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
        draw_clip = ship_clip;
        position = animation->frame;
        piece = &draw_list->pieces[55];
        piece->x = -position;
        offset_shape_rect(&piece->bounds, piece->y, piece->x, get_shape(piece->image_set));
        draw_image(55, 11);

        piece = &draw_list->pieces[61];
        ship_x = position;
        piece->x = 45 - ship_x;
        offset_shape_rect(&piece->bounds, piece->y, piece->x, get_shape(piece->image_set));
        piece->pass = 1;
        draw_image(61, 1);
        piece->pass = 11;

        image = (position & 3) + 62;
        piece = &draw_list->pieces[image];
        piece->x = -ship_x;
        offset_shape_rect(&piece->bounds, piece->y, piece->x, get_shape(piece->image_set));
        draw_image(image, 11);
        draw_clip = clip;
    }
}

extern signed char state_byte;
extern int state_word;

int far pascal RoofClimbingIntoShip(void)
{
    int result;
    if (state_byte == 19 && state_word == 59)
        result = 1;
    else
        result = 0;
    return result;
}

int far pascal grab_roof_ship(void)
{
    int result = 0;
    int height;

    if (state_byte == 19 && char_hp < 3) {
        height = ship_grab_height;
        if (POSITION_WORD < 208 && height < 32)
            result = 1;
    }
    return result;
}

extern unsigned char STATE_BYTE;
extern signed int POSITION_WORD;
extern unsigned char STATE_KIND;
extern int STATE_CODE;

int far pascal RoofHangingFromShip(void)
{
    int result;

    if (STATE_BYTE == 19 && POSITION_WORD < 208 &&
        (STATE_KIND == 2 || STATE_KIND == 6 || STATE_CODE == 80))
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
