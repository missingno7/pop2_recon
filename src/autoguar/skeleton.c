#pragma pack(1)
typedef struct {
    unsigned char head[4];
    unsigned char state;
    int sequence;
    int position;
    unsigned char reserved[8];
    int shake_delay;
    int timer;
    unsigned char reserved2;
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

extern Actor Char;
extern Room rooms[];
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
    count = rooms[room].count;
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
