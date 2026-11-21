/* AUTOGUAR.C, flame sword reaction routines at 2c3e:158c. */

typedef struct {
    signed char bytes[64];
} Actor;
extern Actor Char;

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
