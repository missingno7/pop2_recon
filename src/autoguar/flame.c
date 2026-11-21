/* AUTOGUAR.C, the actor-facing helper and flame sword setup. */

typedef struct {
    signed char bytes[64];
} Actor;

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
