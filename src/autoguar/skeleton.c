#pragma pack(1)
typedef struct {
    signed char cell;
    int x;
    signed char facing;
    unsigned char state;
    int sequence;
    int position;
    unsigned char reserved[4];
    signed char motion, secondary_motion;
    unsigned char kind, health;
    int shake_delay;
    int timer;
    signed char row;
    unsigned char shakes;
} Opponent;
#pragma pack()

typedef struct {
    signed char bytes[64];
} Actor;
typedef struct {
    signed char count;
    unsigned char reserved[115];
} Room;

extern Actor Char, Kid;
extern Room room_info[];
extern unsigned char guard_room, guard_kind, skeleton_kind;
extern Opponent near *far pascal GetOppInfoIdx(int room, int id);
extern void far pascal load_actor_slot(signed char id);
extern void far pascal select_animation_sequence(int sequence);
extern void far pascal save_actor_slot(void);

extern signed char far pascal tile_below(void);
extern int far pascal tile_is_solid(signed char tile);
extern int far pascal edge_distance(void);
extern int far pascal offset_char_x(int amount);
extern void far pascal play_sound(int id);
extern void far pascal set_guard_life(signed char id, int flags, int health);
void far pascal set_skeleton_timer(signed char id, int duration);
void far pascal set_skeleton_shakes(signed char id);

void far pascal bones_fall(void)
{
    int distance;

    select_animation_sequence(120);
    play_sound(76);
    Char.bytes[15] = 1;
    Char.bytes[16] = 0;
    if (Char.bytes[17] < 0)
        Char.bytes[17] = 0;

    if (Char.bytes[1] == 0 && tile_is_solid(tile_below())) {
        distance = 32 - edge_distance();
        if (distance <= 35) {
            distance -= 35;
            *(int near *)&Char.bytes[2] = offset_char_x(-distance);
        }
    }
    save_actor_slot();
    set_guard_life(Char.bytes[0], 0, Char.bytes[19]);
    set_skeleton_timer(Char.bytes[0], 120);
    set_skeleton_shakes(Char.bytes[0]);
    load_actor_slot(Char.bytes[0]);
}

extern volatile unsigned char STATE_LEVEL;

int far pascal IsExitChasingSkel(unsigned char value)
{
    int result = 0;

    if (STATE_LEVEL == 4 && value >= 22 && value <= 28 && value != 23) {
        result = 1;
    }
    return result;
}

void far pascal set_skeleton_shakes(signed char id)
{
    Opponent near *opponent = GetOppInfoIdx(guard_room, id);
    if (opponent) {
        if (opponent->state)
            opponent->shakes = (opponent->state + 1) % 2;
        else
            opponent->shakes = 3;
    }
}

void far pascal set_skeleton_timer(signed char id, int duration)
{
    Opponent near *opponent;
    if (guard_kind == 2 || skeleton_kind == 10) {
        opponent = GetOppInfoIdx(guard_room, id);
        if (opponent)
            opponent->timer = duration;
    }
}

#define CW(n) (*(int near *)&Char.bytes[n])

void far pascal shake_bones(unsigned char room, signed char row)
{
    signed char index, count;
    Actor saved;
    Opponent near *opponent;

    saved = Char;
    index = 0;
    count = room_info[room].count;
    if (count > 0) {
        do {
            load_actor_slot(index);
            if (Char.bytes[10] == row && CW(7) == 206 && CW(36) != 5) {
                opponent = GetOppInfoIdx((unsigned char)Char.bytes[14], Char.bytes[0]);
                if (opponent->shake_delay == 0) {
                    select_animation_sequence(112);
                    save_actor_slot();
                }
            }
        } while (++index < count);
    }
    Char = saved;
}

extern signed char far pascal tile_in_front(signed char distance);
extern signed char far pascal tile_below(void);
extern int far pascal tile_is_solid(signed char tile);
extern int far pascal edge_distance(void);
extern int far pascal offset_char_x(int amount);
extern int far pascal decrement_life(signed char amount);
extern void far pascal kill_current_skeleton(void);
extern void far pascal play_sound(int id);

Opponent near *far pascal initialize_skeleton(Opponent near *opponent)
{
    int distance;

    if (opponent->shake_delay == 1) {
        opponent->sequence = 63;
        Char.bytes[16] = 1;
    } else {
        if (Char.bytes[0] == 0 && opponent->shake_delay == 0)
            opponent->sequence = 112;
        else
            opponent->sequence = 119;
        opponent->timer = opponent->shake_delay;
        set_skeleton_shakes(Char.bytes[0]);
        Char.bytes[16] = 0;

        if ((Char.bytes[1] == -1 && tile_is_solid(tile_in_front(1))) ||
            (Char.bytes[1] == 0 && tile_is_solid(tile_below()))) {
            distance = edge_distance();
            if (Char.bytes[1] == 0)
                distance = 32 - distance;
            if (distance <= 35) {
                distance = 35 - distance;
                if (Char.bytes[1] == -1)
                    distance = -distance;
                CW(2) = offset_char_x(distance);
            }
        }
    }
    opponent->position = 0;
    CW(52) = 8;
    return opponent;
}

void far pascal impale_skeleton(void)
{
    if (decrement_life(1)) {
        kill_current_skeleton();
        return;
    }
    select_animation_sequence(126);
    play_sound(72);
}


extern signed char far pascal read_tile(unsigned char room, signed char column, signed char row);
extern int far pascal IsExitOpen(void);
extern int far pascal sequence_available(int sequence);
extern void far pascal request_if_unset(int request);
extern int far pascal random_below(int limit);

int far pascal skeleton_finished_shaking(signed char id)
{
    int result;
    int reset_timer;
    Opponent near *opponent = GetOppInfoIdx(guard_room, id);

    if (opponent) {
        result = 0;
        reset_timer = result;
        if (opponent->shake_delay != -1) {
            if (IsExitChasingSkel(Char.bytes[14])) {
                read_tile(22, 4, 2);
                if (IsExitOpen() && Kid.bytes[14] == Char.bytes[14] &&
                    *(int near *)&Kid.bytes[2] < CW(2)) {
                    if (!sequence_available(10106))
                        request_if_unset(106);
                    result = 1;
                } else {
                    reset_timer = opponent->timer == 0 && CW(25) == 112;
                }
            } else if (opponent->shakes != 0) {
                --opponent->shakes;
                reset_timer = 1;
            } else {
                result = 1;
            }
        }
        if (reset_timer)
            set_skeleton_timer(id, random_below(30) + 15);
    }
    return result;
}


typedef struct {
    int frame;
    signed char x;
    signed char y;
} SpriteFrame;

extern Actor Char, Kid;
extern unsigned int sprite_frame_index, sprite_table_handle;
extern int sprite_y, sprite_frame;
#define flame_offset (Char.bytes[58])
extern int far pascal offset_char_x(int delta);
extern int far pascal offset_sprite_x(int delta);
extern SpriteFrame far *far pascal resolve_sprite_table(unsigned int handle);

#define ACTOR_X(actor) (*(int near *)&(actor).bytes[2])

void far pascal face_kid_for_skeleton(void)
{
    if (ACTOR_X(Kid) > ACTOR_X(Char)) {
        if (Char.bytes[1] == 0)
            return;
        Char.bytes[1] = 0;
    } else {
        if (Char.bytes[1] == -1)
            return;
        Char.bytes[1] = -1;
    }
    ACTOR_X(Char) = offset_char_x(10);
}

void far pascal setup_flame_sword(void)
{
    SpriteFrame far *table = resolve_sprite_table(sprite_table_handle);
    offset_sprite_x(table[sprite_frame_index].x);
    sprite_y += table[sprite_frame_index].y;

    if (sprite_frame == 4 || sprite_frame == 19 || sprite_frame == 20) {
        if (flame_offset != 0)
            sprite_frame = 18 - flame_offset;
        if (flame_offset <= -4) {
            flame_offset = 0;
            return;
        }
        --flame_offset;
    }
}


extern void far pascal play_sound(int sound);
extern void far pascal reset_char_y(void);
extern int far pascal decrement_life(signed char points);
extern void far pascal select_animation_sequence(int sequence);
extern void far pascal set_skeleton_timer(signed char id, int duration);
extern void far pascal request_if_unset(int effect);
int far pascal reduce_flame_sword_life(signed char points);
void far pascal drop_flame_sword(void);

int far pascal block_flame_sword(void)
{
    int sequence = reduce_flame_sword_life(1) ? -1 : 103;
    play_sound(12);
    reset_char_y();
    return sequence;
}

int far pascal reduce_flame_sword_life(signed char points)
{
    int result;
    if (decrement_life(points)) {
        drop_flame_sword();
        result = 1;
    } else {
        result = 0;
    }
    return result;
}

void far pascal drop_flame_sword(void)
{
    select_animation_sequence(107);
    set_skeleton_timer(Char.bytes[0], 240);
    request_if_unset(192);
    Char.bytes[17] = 0;
    Char.bytes[15] = 1;
}





extern signed char room_column[];
extern int guard_counter, effect_x, effect_y;

extern signed char far pascal get_col(int x);
extern signed char far pascal FirstInRow(signed char row);
extern void far pascal reset_char_y(void);
extern void far pascal select_animation_sequence(int sequence);
extern void far pascal request_if_unset(int effect);
extern void far pascal set_opponent_life(Opponent near *opponent);
extern void far pascal ClearCharVel(void);
extern void far pascal save_actor_slot(void);
extern void far pascal copy_kid_to_char(void);

#define CHAR_WORD(n) (*(int near *)&Char.bytes[n])
#define KID_WORD(n) (*(int near *)&Kid.bytes[n])



extern void far pascal remove_char(void);
extern SpriteFrame far *far pascal resolve_sprite_table(unsigned int handle);
extern int far pascal offset_sprite_x(int x);
extern void far pascal make_sprite_clip(void);
extern void far _fastcall set_sprite_style(unsigned char style);
typedef void (far pascal *SmokeDrawer)(void);
extern void far pascal draw_smoke(void);
extern void far pascal submit_guard_sprite(SmokeDrawer draw, void near *clip, signed char id);
#define smoke_frame sprite_frame_index
#define smoke_table_handle sprite_table_handle
extern int sprite_frame, sprite_y;
extern unsigned char sprite_style, sprite_clip[];

extern void far pascal offset_sprite_clip(void far *destination, void far *source, int dx, int dy);

void far pascal float_flame_sword(void)
{
    int row_y;
    int amount, drift, dy;

    if (sprite_frame == 4 || sprite_frame == 19 || sprite_frame == 20) {
        amount = 4;
        if ((row_y = Char.bytes[10] * 63) + 54 >= CW(4)) {
            drift = amount = 1;
        } else {
            drift = -2;
            if (*(volatile int near *)&row_y + 58 <= CW(4))
                amount = 1;
        }

        dy = random_below(amount) + drift;
        CW(4) += dy;
        sprite_y += dy;
        offset_sprite_clip(sprite_clip, sprite_clip, 0, dy);
    }
}

void far pascal check_jinnee_column(void)
{
    if (Char.bytes[9] < -1 || Char.bytes[9] > 10)
        remove_char();
}

void far pascal setup_jinnee_smoke(void)
{
    SpriteFrame far *table;
    SpriteFrame far *smoke;
    unsigned int frame;

    if (smoke_frame >= 171 && smoke_frame <= 199) {
        frame = smoke_frame;
        table = resolve_sprite_table(smoke_table_handle);
        smoke = &table[frame];
        if (smoke->frame != -1) {
            sprite_frame = smoke->frame;
            offset_sprite_x(smoke->x);
            sprite_y += smoke->y;
            sprite_style = 3;
            make_sprite_clip();
            set_sprite_style(9);
            submit_guard_sprite(draw_smoke, sprite_clip, Char.bytes[0]);
        }
    }
}

void far pascal TriggerJinnee(void)
{
    Opponent near *opponent;
    signed char column;
    signed char row;
    int x;

    Kid = Char;
    x = 85 * room_column[Kid.bytes[1]] + KID_WORD(2);
    column = get_col(x);
    row = Kid.bytes[10];
    opponent = GetOppInfoIdx((unsigned char)Kid.bytes[14],
                            Char.bytes[0] = room_info[(unsigned char)Kid.bytes[14]].count++);
    Char.bytes[6] = 12;
    opponent->kind = 9;
    Char.bytes[14] = Kid.bytes[14];
    Char.bytes[10] = row;
    opponent->cell = FirstInRow(row) + column;
    reset_char_y();
    CHAR_WORD(2) = opponent->x = x;
    Char.bytes[9] = column;
    Char.bytes[1] = opponent->facing = -1;
    column = 0;
    opponent->motion = column;
    Char.bytes[56] = column;
    opponent->secondary_motion = column;
    Char.bytes[57] = column;
    Char.bytes[35] = column;
    (*(volatile int near *)&Char.bytes[42]) = 0;
    CHAR_WORD(36) = CHAR_WORD(40) = CHAR_WORD(38) = 0;
    CHAR_WORD(52) = 4;
    Char.bytes[15] = 1;
    Char.bytes[16] = 1;
    opponent->sequence = 180;
    opponent->position = 0;
    select_animation_sequence(180);
    request_if_unset(196);
    opponent->health = 6;
    set_opponent_life(opponent);
    opponent->state = 10;
    Char.bytes[17] = -1;
    guard_counter = 0;
    effect_x = 0;
    effect_y = 0;
    ClearCharVel();
    Char.bytes[11] = 1;
    save_actor_slot();
    copy_kid_to_char();
}



