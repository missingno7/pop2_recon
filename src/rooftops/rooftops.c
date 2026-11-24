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
    sbyte column, row;
    ubyte reserved;
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
    ubyte index;
    ubyte reserved;
} SplashImage;
#pragma pack()

extern sbyte splash_frame;
extern int splash_x;
extern ubyte splash_low;
extern sbyte palette_bank;
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

extern sbyte water_character;
extern CHARREC Char;
extern ROOMREC Room;
extern ubyte animation_room;
extern int fade_active;
extern unsigned long FrameNum;
extern uword BlueSpec;
extern sbyte animation_tile, animation_remove;
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

extern FCHARREC FChar;
extern int draw_pass_mode;
extern int character_width;
extern int delay_count, active_sound_channels;
extern CHARREC Kid;
extern Rect clip_rect, NullRect, GamePlayRect;
static SplashImage splash_images[] = {
    {0, 0}, {29, 0}, {30, 0}, {31, 0},
    {29, 0}, {32, 0}, {33, 0}, {29, 0}
};
extern PeelSlot custom_peels[];
extern char quit_prompt[], restart_prompt[];

extern DrawList near *draw_list;
typedef void (far pascal *RedrawCallback)(void);
extern void far pascal mark_move(void), mark_foreground(void);
extern int far pascal SECTRECT(Rect far *, Rect far *, Rect far *);
extern void far pascal mark_bufs_for_rect(RedrawCallback, Rect near *, sbyte);
extern void far pascal decrement_life_points(sbyte), SetKidDeathMethod(sbyte);
extern int far pascal set_palette(uword, int, ubyte, sbyte);
extern void far pascal CycleColors(int, int, int);
extern int far pascal query_sound(char near *);
extern void far pascal AddSound(int), AddLeaveLevelSong(void), play_song(int);
extern void far pascal wait_sound(int), wait_timer(int), ChangeMeters(void);
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
            mark_bufs_for_rect(mark_move, &draw_list->pieces[splash_images[splash_frame].index].bounds, -1);
        } else if (splash_frame >= 8) {
            splash_frame = 0;
            water_character = -1;
            if (Char.row >= 2) {
                Char.frame = 185;
                decrement_life_points(100);
                SetKidDeathMethod(15);
            }
            return;
        }
        ++splash_frame;
    }
}

void far pascal check_roof_palette(sbyte room)
{
    sbyte bank = -1;

    if (Level.environment_kind == 5 && room != 19 && room != 16 && room != 15) {
        if (palette_bank != 0)
            bank = palette_bank = 0;
    } else if (palette_bank != 1) {
        bank = palette_bank = 1;
    }
    if (bank != -1)
        set_palette(3500, 64, 176, bank);
}

void far pascal check_roof_room(void)
{
    check_roof_palette(Room.number);
}

void far pascal clip_character_at_water(void)
{
    int limit;

    if ((Char.room == 19 || Char.room == 16) && Char.slot == water_character) {
        limit = 172;
        if (Char.action == 9) limit = 186;
        FChar.clip_bottom = limit > FChar.clip_bottom ? FChar.clip_bottom : limit;
        if (FChar.clip_bottom < FChar.clip_top)
            FChar.clip_top = FChar.clip_bottom;
    }
}

void far pascal cycle_roof_colors(void)
{
    if (!fade_active && (Room.number == 19 || Room.number == 16 || Room.number == 15) &&
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
                ChangeMeters();
                CheckMeters();
                copy_kid_to_char();
                wait_timer(0);
            } while (--hp);
        }
        reset_char_y();
        JumpSeq(71);
        flag_sound_resource(restart_prompt);
        wait_sound((int)restart_prompt);
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
    sbyte count = Level.room_objects[Room.number - 1].opponent_count;
    sbyte index;

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
        clip = clip_rect;
        image = splash_images[splash_frame].index;
        piece = &draw_list->pieces[image];
        piece->y = piece->bounds.top - piece->bounds.bottom + 176;
        if (Char.action == 9)
            piece->y += 14;
        piece->x = (piece->bounds.left - piece->bounds.right) / 2 + splash_x;
        MAKESHAPERECT(&piece->bounds, piece->y, piece->x, RESOLVE_HANDLE_TO_FAR_PAYLOAD(piece->image_set));
        clip_rect = piece->bounds;
        piece->pass = 1;
        draw_custom_room_image(image, piece->pass);
        piece->pass = 11;
        if (Char.action != 9)
            mark_bufs_for_rect(mark_foreground, &piece->bounds, -1);
        clip_rect = clip;
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
    mark_bufs_for_rect(mark_move, &draw_list->pieces[(ubyte)(BlueSpec + 30)].bounds, -1);
}

void far pascal draw_roof_glass(RoofAnimation near *animation)
{
    Rect clip;
    ubyte image;

    if (animation->kind == 11 && animation->frame < 11) {
        clip = clip_rect;
        clip_rect = GamePlayRect;
        image = (ubyte)(animation->frame + 31);
        draw_custom_room_image(image, 11);
        if (image + 1 < draw_list->piece_count)
            mark_bufs_for_rect(mark_foreground, &draw_list->pieces[image + 1].bounds, -1);
        clip_rect = clip;
    }
}

extern void far pascal OFFSETRECT(Rect far *, Rect far *, int, int);
extern DrawPiece near *far pascal offset_custom_piece(DrawPiece near *, sbyte, sbyte);
extern void far pascal DrawCustomRoomImgAtInfo(ubyte, signed char near *);
DrawPiece near *far pascal offset_roof_pillar(DrawPiece near *);

void far pascal AnimRoofPillar(void)
{
    DrawPiece near *piece;
    long position;
    Rect water, bounds;
    unsigned int step;

    if (check_trob_visibility()) {
        step = BlueSpec & 15;
        if (step == 7)
            step = 0;
        else
            ++step;
        BlueSpec &= 0xfff0;
        BlueSpec |= step;
        make_water_test_rect(&water);
        piece = &draw_list->pieces[25];
        bounds = piece->bounds;
        position = *(long *)&piece->y;
        piece = offset_custom_piece(piece, animation_tile % 10, animation_tile / 10);
        if (!splash_frame || !SECTRECT(&water, &water, &piece->bounds)) {
            mark_bufs_for_rect(mark_move, &piece->bounds, -1);
            offset_roof_pillar(piece);
            mark_bufs_for_rect(mark_move, &piece->bounds, -1);
        }
        *(long *)&piece->y = position;
        piece->bounds = bounds;
    }
}

void far pascal draw_roof_pillar(RoofAnimation near *animation)
{
    DrawPiece near *piece;
    struct {
        long position;
        sbyte kind;
        ubyte image;
    } saved;
    Rect bounds;

    if (animation->kind == 11 || (animation->kind == 0 && draw_pass_mode)) {
        saved.kind = animation->kind;
        animation->kind = 0;
        saved.image = ((animation->frame >> 1) & 7) + 25;
        DrawCustomRoomImgAtInfo(saved.image, (signed char *)animation);
        piece = &draw_list->pieces[saved.image];
        bounds = piece->bounds;
        saved.position = *(long *)&piece->y;
        piece = offset_custom_piece(piece, animation->column, animation->row);
        if (!draw_pillar_in_back(piece))
            animation->kind = 1;
        *(long *)&piece->y = saved.position;
        piece->bounds = bounds;
        offset_roof_pillar(piece);
        DrawCustomRoomImgAtInfo(saved.image, (signed char *)animation);
        *(long *)&piece->y = saved.position;
        piece->bounds = bounds;
        animation->kind = saved.kind;
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

DrawPiece near *far pascal offset_roof_pillar(DrawPiece near *piece)
{
    piece->y += 12;
    piece->x -= 30;
    OFFSETRECT(&piece->bounds, &piece->bounds, -30, 12);
    return piece;
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
    if (Room.number == animation_room) {
        piece = &draw_list->pieces[19];
        piece->x = -BlueSpec;
        MAKESHAPERECT(&bounds, piece->y, piece->x, RESOLVE_HANDLE_TO_FAR_PAYLOAD(piece->image_set));
        ++bounds.right;
        mark_bufs_for_rect(mark_move, &bounds, -1);
        slot = RestoreKeepCustomPeel(piece->kind);
        update_ship_peel(slot, bounds.right);

        piece = &draw_list->pieces[(sbyte)(((BlueSpec - 1) & 3) + 21)];
        piece->x = 1 - BlueSpec;
        MAKESHAPERECT(&bounds, piece->y, piece->x, RESOLVE_HANDLE_TO_FAR_PAYLOAD(piece->image_set));
        ++bounds.right;
        mark_bufs_for_rect(mark_move, &bounds, -1);
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
        clip = clip_rect;
        clip_rect = GamePlayRect;
        position = animation->frame;
        piece = &draw_list->pieces[19];
        piece->x = -position;
        MAKESHAPERECT(&piece->bounds, piece->y, piece->x, RESOLVE_HANDLE_TO_FAR_PAYLOAD(piece->image_set));
        draw_custom_room_image(19, 11);

        piece = &draw_list->pieces[20];
        ship_x = position;
        piece->x = 45 - ship_x;
        MAKESHAPERECT(&piece->bounds, piece->y, piece->x, RESOLVE_HANDLE_TO_FAR_PAYLOAD(piece->image_set));
        piece->pass = 1;
        draw_custom_room_image(20, 1);
        piece->pass = 11;

        image = (position & 3) + 21;
        piece = &draw_list->pieces[image];
        piece->x = -ship_x;
        MAKESHAPERECT(&piece->bounds, piece->y, piece->x, RESOLVE_HANDLE_TO_FAR_PAYLOAD(piece->image_set));
        draw_custom_room_image(image, 11);
        clip_rect = clip;
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
        height = Level.cell_info[18][0].modifier;
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

typedef void (far pascal *LevelTileProc)(RoofAnimation near *);
LevelTileProc rooftops_level_spec[] = {
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, draw_roof_pillar, draw_roof_ship, draw_roof_glass
};
