#pragma pack(1)
typedef struct {
    signed char index, direction;
    int x, y;
    signed char kind;
    unsigned int frame;
    signed char column, row;
    char reserved_b[3];
    unsigned char room;
    signed char merged;
    char reserved_10[2];
    signed char health, max_health, health_delta;
    char reserved_15[4];
    int repeat_sequence;
    char reserved_1b[9];
    int animation_contact;
    char reserved_26[14];
    int shadow_flags;
    char reserved_36[10];
} Actor;

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

extern Actor Char, Kid, Opp, opponent_slots[];
extern RoomOpponents room_opponents[];
extern signed char shadow_direction_offsets[];
extern signed char current_level, active_room, pending_room;
extern int shadow_timer, shadow_overlay_state, flame_time, flame_y, flame_x;

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
extern signed char far pascal find_actor_room(void);
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
    Actor near *kid;

    index = Opp.index;
    distance = GetShadowToKidDist();
    if (abs(distance) < 32) {
        if (Opp.direction != Char.direction) {
            Char.direction = ~Char.direction;
            Char.x = AddCharX(6);
        }
        Char.x -= distance;
        --Char.y;
        Char.merged = 0;
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
    if (Char.kind == 0 && Char.repeat_sequence == 5) {
        if (++shadow_timer >= 4) {
            if (shadow_timer == 4) {
                install_palette_resource(2000, 48, 16, IsShadowImbued() != 0);
                HGetSnd(10261);
            }
            if (!decrement_life_points(1))
                --Char.max_health;
            else
                JumpSeq(71);
            if (shadow_timer == 8) {
                if (Char.health > 4) {
                    CreateKidAsOpp();
                    Char.kind = 1;
                    Char.shadow_flags = 8;
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

    if (Kid.health > 2 && Kid.health + Kid.health_delta <= 2 && Kid.repeat_sequence != 242)
        bank = 0;
    else if (Kid.health <= 2 && Kid.health + Kid.health_delta > 2)
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
            Char.shadow_flags = IsShadowBlinkOn(Char.frame, shadow_timer) ? 8 : 0;
    } else if (Char.repeat_sequence != 5 && shadow_timer != 0) {
        set_kid_palette(current_level);
        shadow_timer = 0;
    }
}

void far pascal CreateKidAsOpp(void)
{
    Actor near *kid;
    Opponent near *opp;
    signed char index;

    if (is_six_kind(get_behind()))
        Char.x = AddCharX(16);
    else if (is_six_kind(get_cells_ahead(1)))
        Char.x = AddCharX(-16);

    kid = &Kid;
    *kid = Char;
    index = room_opponents[Char.room].count++;
    opp = GetOppInfoIdx(Char.room, index);
    opp->cell = FirstInRow(Char.row) + Char.column;
    opp->x = Char.x;
    opp->facing = Char.direction;
    opp->state = 0;
    opp->reserved_9 = 0;
    opp->index = index;
    opp->flags = 0;
    opp->motion = 0;
    opp->health = Char.health;
    opp->kind = 10;
    opp->max_health = Char.max_health;
    Char.index = index;
    Char.kind = 0;
    Char.shadow_flags = 2;
    JumpSeq(71);
    AnimChar();
    save_actor_slot();
    copy_kid_to_char();
}

void far pascal CutToKidRoom(void)
{
    Actor near *kid;
    signed char room;

    kid = &Kid;
    *kid = Char;
    draw_kid_meter(Char.health, Char.max_health);
    load_actor_slot(GetKidIdxAsOpp());
    Char.index = 10;
    Char.shadow_flags = 0;
    Char.health = Kid.health;
    Char.max_health = Kid.max_health;
    Char.health_delta = Kid.health_delta;
    room = find_actor_room();
    if (active_room != room) {
        pending_room = room;
        Char.room = room;
    }
    if ((room == 7 || room == 8) && current_level == 6)
        AddSound(262);
    Kid = Char;
}

signed char far pascal GetKidIdxAsOpp(void)
{
    signed char index = 0;
    signed char count = room_opponents[Char.room].count;

    if (count > 0) {
        do {
            if (opponent_slots[index].kind == 0)
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

    if (Char.animation_contact == 13 || (current_level == 6 && Char.health > 2))
        result = 1;
    else
        result = 0;
    return result;
}

void far pascal KidRiseFromShadowman(void)
{
    Actor near *kid;

    if (Opp.kind == 0) {
        kid = &Kid;
        *kid = Char;
        load_actor_slot(Opp.index);
        remove_char();
        save_actor_slot();
        copy_kid_to_char();
        Char.kind = 0;
        Char.shadow_flags = 0;
        Char.merged = 1;
        JumpSeq(231);
        AnimChar();
        Kid = Char;
    }
}

typedef void (far pascal *ForegroundMarker)(signed char cell, void near *rect);
extern void far pascal draw_floor_foreground(ForegroundMarker effect, void near *rect, signed char actor);
extern void far pascal mark_foreground(signed char cell, void near *rect);
extern char flame_clip_rect[];

void far pascal SetupShadowFlame(void)
{
    if (IsShadowImbued()) {
        SaveFChar();
        flame_x = flame_time % 9 + 290;
        AddFCharX(13 - flame_y / 2);
        MakeFCharClipRect();
        add_char_object(14);
        draw_floor_foreground(mark_foreground, flame_clip_rect, Char.index);
        RestoreFChar();
    }
}

void far pascal reset_overlay_state(void)
{
    shadow_overlay_state = 0;
}
