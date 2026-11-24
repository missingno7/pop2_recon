/* ROOFTOPS.C: DOS overlay 4. Descriptive names mark unresolved symbols. */
#include "types.h"
#include "level.h"
#include "char.h"
#include "game.h"

#pragma pack(1)

typedef struct {
    sbyte kind;
    int y, x;
    byte pass;
    byte reserved[3];
    int image_set;
    Rect bounds;
    byte tail[6];
} DrawPiece;

typedef struct {
    sbyte piece_count;
    byte bank;
    byte reserved[26];
    DrawPiece pieces[80];
} DrawList;

typedef struct {
    sbyte kind;
    sbyte column, row;
    byte reserved;
    word frame;
} RoofAnimation;

typedef struct {
    byte reserved[18];
    int top;
    int left;
    int bottom;
} Peel;

typedef struct {
    Peel near *peel;
    byte reserved[4];
} PeelSlot;

typedef struct {
    byte index;
    byte reserved;
} SplashImage;
#pragma pack()

typedef struct {
    signed char character, frame;
    int x;
    unsigned char low;
} WATERSTATE;
WATERSTATE InWater;
static sbyte palette_bank;

extern LEVELREC Level;

extern CHARREC Char;
extern ROOMREC Room;

extern TROBREC Trob;

extern GAMESTATE GameState;
extern unsigned long FrameNum;
extern word BlueSpec;

extern FCHARREC FChar;
extern int NewRoom;
extern int ImWidth;
extern int Timer[4];
extern struct { unsigned char channels, volume; } Midi;
extern CHARREC Kid;
extern Rect CellClipRect, NullRect, GamePlayRect;
static SplashImage splash_images[] = {
    {29, 0}, {30, 0}, {31, 0},
    {29, 0}, {32, 0}, {33, 0}, {29, 0}
};
extern struct {
    int count;
    PeelSlot entries[40];
} PeelInfo;

extern DrawList near *custom_room;
typedef void (far pascal *RedrawCallback)(void);
extern void far pascal mark_move(void), mark_foreground(void);
extern int far pascal SECTRECT(Rect far *, Rect far *, Rect far *);
extern void far pascal mark_bufs_for_rect(RedrawCallback, Rect near *, sbyte);
extern void far pascal decrement_life_points(sbyte), SetKidDeathMethod(sbyte);
extern int far pascal set_palette(word, int, byte, sbyte);
extern void far pascal CycleColors(int, int, int);
extern int far pascal query_sound(int);
extern void far pascal AddSound(int), AddLeaveLevelSong(void), play_song(int);
extern void far pascal wait_sound(int), wait_timer(int), ChangeMeters(void);
extern void far pascal CheckMeters(void), copy_kid_to_char(void);
extern void far pascal reset_char_y(void), JumpSeq(int), flag_sound_resource(int);
extern void far pascal load_actor_slot(sbyte), save_actor_slot(void);
extern int far pascal check_trob_visibility(void);
extern void far *far pascal RESOLVE_HANDLE_TO_FAR_PAYLOAD(int);
extern Rect far *far pascal MAKESHAPERECT(Rect far *, int, int, void far *);
extern void far _fastcall draw_custom_room_image(byte, byte);
extern sbyte far pascal RestoreKeepCustomPeel(byte);

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
        Kid.sequence != 68 &&
        (Kid.action == 3 || Kid.action == 4))
        result = 1;
    else
        result = 0;
    return result;
}

void far pascal LoadRoofCode(void)
{
    sbyte inactive = -1;

    InWater.frame = 0;
    palette_bank = inactive;
    InWater.character = inactive;
}

void far pascal check_character_water(void)
{
    if (InWater.character == -1) {
        if (Char.y >= 172 && Char.sequence != 68 && Char.sequence != 15 &&
            Char.sequence != 59 && Char.action != 2)
            trigger_character_water();
    } else if (Char.slot == InWater.character) {
        if (InWater.frame < 7) {
            if (custom_room->bank != 18 && custom_room->bank != 19)
                return;
            mark_bufs_for_rect(mark_move, &custom_room->pieces[splash_images[InWater.frame - 1].index].bounds, -1);
        } else if (InWater.frame >= 8) {
            InWater.frame = 0;
            InWater.character = -1;
            if (Char.row >= 2) {
                Char.frame = 185;
                decrement_life_points(100);
                SetKidDeathMethod(15);
            }
            return;
        }
        ++InWater.frame;
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

    if ((Char.room == 19 || Char.room == 16) && Char.slot == InWater.character) {
        limit = 172;
        if (Char.action == 9) limit = 186;
        FChar.clip.bottom = limit > FChar.clip.bottom ? FChar.clip.bottom : limit;
        if (FChar.clip.bottom < FChar.clip.top)
            FChar.clip.top = FChar.clip.bottom;
    }
}

void far pascal cycle_roof_colors(void)
{
    if (!GameState.fade_active && (Room.number == 19 || Room.number == 16 || Room.number == 15) &&
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
        if ((Midi.channels & 1) && !query_sound(10032) && !query_sound(10047)) {
            AddSound(32);
            return;
        }
        wait_sound(10032);
        Timer[0] = 30;
        wait_timer(0);
        play_song(47);
        hp = Char.life_points;
        if (hp > 0) {
            do {
                Timer[0] = 48;
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
        flag_sound_resource(10047);
        wait_sound(10047);
    }
}

Rect near *far pascal make_water_test_rect(Rect near *rect)
{
    if (InWater.frame && InWater.frame <= 7) {
        if (InWater.low == 1)
            rect->top = 186;
        else
            rect->top = 172;
        rect->left = InWater.x - 16;
        rect->bottom = rect->top + 1;
        rect->right = InWater.x + 16;
    } else {
        *rect = NullRect;
    }
    return rect;
}

void far pascal trigger_character_water(void)
{
    if (!InWater.frame) {
        InWater.character = Char.slot;
        InWater.frame = 1;
        InWater.low = (Char.action == 9) ? 1 : 0;
        InWater.x = Char.x - 130;
        if (Char.direction == 0)
            InWater.x -= ImWidth / 2;
        else
            InWater.x += ImWidth / 2;
        AddSound(48);
    }
}

void far pascal watch_kid(void)
{
    if (Kid.room == 19 || Kid.room == 16 || InWater.character == 10) {
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
                if (Char.room == 19 || Char.room == 16 || index == InWater.character) {
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
    byte image;

    if (Char.slot == InWater.character && InWater.frame != 0 && InWater.frame < 7 &&
        (custom_room->bank == 18 || custom_room->bank == 19)) {
        clip = CellClipRect;
        image = splash_images[InWater.frame - 1].index;
        piece = &custom_room->pieces[image];
        piece->y = piece->bounds.top - piece->bounds.bottom + 176;
        if (Char.action == 9)
            piece->y += 14;
        piece->x = (piece->bounds.left - piece->bounds.right) / 2 + InWater.x;
        MAKESHAPERECT(&piece->bounds, piece->y, piece->x, RESOLVE_HANDLE_TO_FAR_PAYLOAD(piece->image_set));
        CellClipRect = piece->bounds;
        piece->pass = 1;
        draw_custom_room_image(image, piece->pass);
        piece->pass = 11;
        if (Char.action != 9)
            mark_bufs_for_rect(mark_foreground, &piece->bounds, -1);
        CellClipRect = clip;
    }
}

void far pascal animate_roof_glass(void)
{
    if (BlueSpec >= 11) {
        Trob.state = -1;
        return;
    }
    ++BlueSpec;
    if (BlueSpec == 1)
        AddSound(49);
    mark_bufs_for_rect(mark_move, &custom_room->pieces[(byte)(BlueSpec + 30)].bounds, -1);
}

void far pascal draw_roof_glass(RoofAnimation near *animation)
{
    Rect clip;
    byte image;

    if (animation->kind == 11 && animation->frame < 11) {
        clip = CellClipRect;
        CellClipRect = GamePlayRect;
        image = (byte)(animation->frame + 31);
        draw_custom_room_image(image, 11);
        if (image + 1 < custom_room->piece_count)
            mark_bufs_for_rect(mark_foreground, &custom_room->pieces[image + 1].bounds, -1);
        CellClipRect = clip;
    }
}

extern void far pascal OFFSETRECT(Rect far *, Rect far *, int, int);
extern DrawPiece near *far pascal offset_custom_piece(DrawPiece near *, sbyte, sbyte);
extern void far pascal DrawCustomRoomImgAtInfo(byte, signed char near *);
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
        piece = &custom_room->pieces[25];
        bounds = piece->bounds;
        position = *(long *)&piece->y;
        piece = offset_custom_piece(piece, Trob.cell % 10, Trob.cell / 10);
        if (!InWater.frame || !SECTRECT(&water, &water, &piece->bounds)) {
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
        byte image;
    } saved;
    Rect bounds;

    if (animation->kind == 11 || (animation->kind == 0 && NewRoom)) {
        saved.kind = animation->kind;
        animation->kind = 0;
        saved.image = ((animation->frame >> 1) & 7) + 25;
        DrawCustomRoomImgAtInfo(saved.image, (signed char *)animation);
        piece = &custom_room->pieces[saved.image];
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

    if (InWater.frame && InWater.low == 1 && piece->kind >= 6 && piece->kind <= 11) {
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
        Trob.state = -1;
        return;
    }
    if (Room.number == Trob.room) {
        piece = &custom_room->pieces[19];
        piece->x = -BlueSpec;
        MAKESHAPERECT(&bounds, piece->y, piece->x, RESOLVE_HANDLE_TO_FAR_PAYLOAD(piece->image_set));
        ++bounds.right;
        mark_bufs_for_rect(mark_move, &bounds, -1);
        slot = RestoreKeepCustomPeel(piece->kind);
        update_ship_peel(slot, bounds.right);

        piece = &custom_room->pieces[(sbyte)(((BlueSpec - 1) & 3) + 21)];
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
    word position;
    int ship_x;
    sbyte image;

    if (animation->kind == 11 && animation->frame <= 176) {
        clip = CellClipRect;
        CellClipRect = GamePlayRect;
        position = animation->frame;
        piece = &custom_room->pieces[19];
        piece->x = -position;
        MAKESHAPERECT(&piece->bounds, piece->y, piece->x, RESOLVE_HANDLE_TO_FAR_PAYLOAD(piece->image_set));
        draw_custom_room_image(19, 11);

        piece = &custom_room->pieces[20];
        ship_x = position;
        piece->x = 45 - ship_x;
        MAKESHAPERECT(&piece->bounds, piece->y, piece->x, RESOLVE_HANDLE_TO_FAR_PAYLOAD(piece->image_set));
        piece->pass = 1;
        draw_custom_room_image(20, 1);
        piece->pass = 11;

        image = (position & 3) + 21;
        piece = &custom_room->pieces[image];
        piece->x = -ship_x;
        MAKESHAPERECT(&piece->bounds, piece->y, piece->x, RESOLVE_HANDLE_TO_FAR_PAYLOAD(piece->image_set));
        draw_custom_room_image(image, 11);
        CellClipRect = clip;
    }
}

int far pascal RoofClimbingIntoShip(void)
{
    int result;
    if (Char.room == 19 && Char.sequence == 59)
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
        peel = PeelInfo.entries[slot].peel;
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
