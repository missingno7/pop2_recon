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

typedef struct {
    signed char cell;
    int x;
    signed char facing, state;
    char reserved_5[4];
    signed char reserved_9, index, flags, health, motion;
    char reserved_e;
    signed char kind, max_health;
} Opponent;

typedef struct {
    signed char count;
    unsigned char reserved[115];
} RoomOpponents;
#pragma pack()

extern CHARREC Char, Kid, Opp, RoomOpp[5];
extern RoomOpponents room_opponents[];
extern signed char shadow_direction_offsets[];
extern signed char current_level, viewed_room, CutRoom;
extern int shadow_timer, Pause, FrameNum, flame_y, flame_x;

int abs(int value);
#pragma intrinsic(abs)

extern int far pascal AddCharX(int distance);
extern void far pascal load_kid_with_opponent(signed char index);
extern int far pascal install_palette_resource(unsigned int palette, int first, unsigned char count, unsigned char bank);
extern int far pascal HGetSnd(int id);
extern int far pascal decrement_life_points(signed char amount);
extern void far pascal JumpSeq(int sequence);
extern void far pascal AddSound(int id);
extern void far pascal AddSong(int id);
extern void far pascal set_kid_palette(signed char level);
extern int far pascal is_six_kind(signed char tile);
extern signed char far pascal get_behind(void);
extern signed char far pascal get_cells_ahead(signed char direction);
extern Opponent near *far pascal GetOppInfoIdx(int room, int index);
extern signed char far pascal FirstInRow(signed char row);
extern void far pascal AnimChar(void);
extern void far pascal save_actor_slot(void);
extern void far pascal copy_kid_to_char(void);
extern void far pascal draw_kid_meter(int health, int max_health);
extern void far pascal load_actor_slot(signed char index);
extern signed char far pascal register_actor_room(void);
extern void far pascal remove_char(void);
extern void far pascal SaveFChar(void);
extern int far pascal AddFCharX(int distance);
extern void far pascal MakeFCharClipRect(void);
extern void far _fastcall add_char_object(unsigned char effect);
extern void far pascal RestoreFChar(void);

int far pascal GetShadowToKidDist(void);
int far pascal IsShadowImbued(void);
void far pascal CreateKidAsOpp(void);
signed char far pascal GetKidIdxAsOpp(void);
int far pascal IsShadowBlinkOn(unsigned int value, int index);

int far pascal CheckShadowmanBackIntoKid(void)
{
    signed char index;
    int distance, result;
    CHARREC near *kid;

    index = Opp.slot;
    distance = GetShadowToKidDist();
    if (abs(distance) < 32) {
        if (Opp.direction != Char.direction) {
            Char.direction = ~Char.direction;
            Char.x = AddCharX(6);
        }
        Char.x -= distance;
        --Char.y;
        Char.field_0f = 0;
        kid = &Kid;
        *kid = Char;
        result = 71;
    } else {
        result = -1;
    }
    load_kid_with_opponent(index);
    return result;
}

void far pascal CheckShadowmanCreate(void)
{
    if (Char.character_kind == 0 && Char.sequence == 5) {
        if (++shadow_timer >= 4) {
            if (shadow_timer == 4) {
                install_palette_resource(2000, 48, 16, IsShadowImbued() != 0);
                HGetSnd(10261);
            }
            if (!decrement_life_points(1))
                --Char.life_initial;
            else
                JumpSeq(71);
            if (shadow_timer == 8) {
                if (Char.life_points > 4) {
                    CreateKidAsOpp();
                    Char.character_kind = 1;
                    Char.field_34 = 8;
                    JumpSeq(2);
                    AddSound(261);
                    AddSong(272);
                } else {
                    JumpSeq(71);
                    decrement_life_points(100);
                }
                shadow_timer = 0;
            }
            AddSound(93);
        }
    } else {
        shadow_timer = 1;
    }
}

void far pascal CheckShadowPaletteChange(void)
{
    signed char bank = -1;

    if (Kid.life_points > 2 && Kid.life_points + Kid.life_delta <= 2 && Kid.sequence != 242)
        bank = 0;
    else if (Kid.life_points <= 2 && Kid.life_points + Kid.life_delta > 2)
        bank = 1;
    if (bank != -1)
        install_palette_resource(2000, 48, 16, bank);
}

void far pascal CheckTurnSeqs(void)
{
    if (Char.frame == 15)
        shadow_timer = 0;
    else if (Char.frame >= 45 && Char.frame <= 52 &&
             !((Char.room == 1 || Char.room == 2) && current_level == 14)) {
        if (shadow_timer >= 4)
            Char.field_34 = IsShadowBlinkOn(Char.frame, shadow_timer) ? 8 : 0;
    } else if (Char.sequence != 5 && shadow_timer != 0) {
        set_kid_palette(current_level);
        shadow_timer = 0;
    }
}

void far pascal CreateKidAsOpp(void)
{
    CHARREC near *kid;
    Opponent near *opp;
    signed char index;

    if (is_six_kind(get_behind()))
        Char.x = AddCharX(16);
    else if (is_six_kind(get_cells_ahead(1)))
        Char.x = AddCharX(-16);

    kid = &Kid;
    *kid = Char;
    index = room_opponents[(unsigned char)Char.room].count++;
    opp = GetOppInfoIdx((unsigned char)Char.room, index);
    opp->cell = FirstInRow(Char.row) + Char.column;
    opp->x = Char.x;
    opp->facing = Char.direction;
    opp->state = 0;
    opp->reserved_9 = 0;
    opp->index = index;
    opp->flags = 0;
    opp->motion = 0;
    opp->health = Char.life_points;
    opp->kind = 10;
    opp->max_health = Char.life_initial;
    Char.slot = index;
    Char.character_kind = 0;
    Char.field_34 = 2;
    JumpSeq(71);
    AnimChar();
    save_actor_slot();
    copy_kid_to_char();
}

void far pascal CutToKidRoom(void)
{
    CHARREC near *kid;
    signed char room;

    kid = &Kid;
    *kid = Char;
    draw_kid_meter(Char.life_points, Char.life_initial);
    load_actor_slot(GetKidIdxAsOpp());
    Char.slot = 10;
    Char.field_34 = 0;
    Char.life_points = Kid.life_points;
    Char.life_initial = Kid.life_initial;
    Char.life_delta = Kid.life_delta;
    room = register_actor_room();
    if (viewed_room != room) {
        CutRoom = room;
        Char.room = room;
    }
    if ((room == 7 || room == 8) && current_level == 6)
        AddSound(262);
    Kid = Char;
}

signed char far pascal GetKidIdxAsOpp(void)
{
    signed char index = 0;
    signed char count = room_opponents[(unsigned char)Char.room].count;

    if (count > 0) {
        do {
            if (RoomOpp[index].character_kind == 0)
                break;
            index++;
        } while (index < count);
    }
    return index;
}

int far pascal GetShadowToKidDist(void)
{
    int distance;

    load_kid_with_opponent(GetKidIdxAsOpp());
    if (Opp.row == Char.row) {
        distance = Char.x - Opp.x;
        if (Opp.direction == Char.direction && Char.frame != 185)
            distance += 6 * shadow_direction_offsets[Char.direction];
    } else {
        distance = 999;
    }
    return distance;
}

int far pascal IsShadowBlinkOn(unsigned int value, int index)
{
    int i;
    int result;

    if (index >= 4 && value > 45U && value < 52U) {
        i = index;
        i -= 4;
        value -= 46U;
        if (i < 2) {
            result = (value % (3 - i) == 0);
        } else {
            result = (value + 1U) % 3U;
        }
    } else {
        result = 0;
    }
    return result;
}

int far pascal IsShadowImbued(void)
{
    int result;

    if (Char.sequence_group == 13 || (current_level == 6 && Char.life_points > 2))
        result = 1;
    else
        result = 0;
    return result;
}

void far pascal KidRiseFromShadowman(void)
{
    CHARREC near *kid;

    if (Opp.character_kind == 0) {
        kid = &Kid;
        *kid = Char;
        load_actor_slot(Opp.slot);
        remove_char();
        save_actor_slot();
        copy_kid_to_char();
        Char.character_kind = 0;
        Char.field_34 = 0;
        Char.field_0f = 1;
        JumpSeq(231);
        AnimChar();
        Kid = Char;
    }
}

typedef void (far pascal *ForegroundMarker)(signed char cell, void near *rect);
extern void far pascal mark_bufs_for_rect(ForegroundMarker effect, void near *rect, signed char actor);
extern void far pascal mark_foreground(signed char cell, void near *rect);
extern char flame_clip_rect[];

void far pascal SetupShadowFlame(void)
{
    if (IsShadowImbued()) {
        SaveFChar();
        flame_x = FrameNum % 9 + 290;
        AddFCharX(13 - flame_y / 2);
        MakeFCharClipRect();
        add_char_object(14);
        mark_bufs_for_rect(mark_foreground, flame_clip_rect, Char.slot);
        RestoreFChar();
    }
}

void far pascal reset_shadow_overlay_state(void)
{
    Pause = 0;
}
