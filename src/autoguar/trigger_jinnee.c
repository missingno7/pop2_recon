/* AUTOGUAR.C, final actor setup routine at 2c3e:1710. */

typedef struct {
    signed char bytes[64];
} CharacterRecord;

#pragma pack(1)
typedef struct {
    signed char cell;
    int x;
    signed char facing;
    unsigned char state;
    int start_sequence;
    int sequence_position;
    unsigned char reserved[4];
    signed char motion;
    signed char secondary_motion;
    unsigned char kind;
    unsigned char health;
} OpponentInfo;
#pragma pack()

extern CharacterRecord Char, Kid;
extern signed char room_column[];
typedef struct {
    signed char next_opponent;
    unsigned char tail[115];
} RoomInfo;
extern RoomInfo room_info[];
extern int guard_counter, effect_x, effect_y;

extern signed char far pascal get_col(int x);
extern OpponentInfo near *far pascal GetOppInfoIdx(int room, int id);
extern signed char far pascal FirstInRow(signed char row);
extern void far pascal reset_char_y(void);
extern void far pascal select_animation_sequence(int sequence);
extern void far pascal request_if_unset(int effect);
extern void far pascal set_opponent_life(OpponentInfo near *opponent);
extern void far pascal ClearCharVel(void);
extern void far pascal save_actor_slot(void);
extern void far pascal copy_kid_to_char(void);

#define CHAR_WORD(n) (*(int near *)&Char.bytes[n])
#define KID_WORD(n) (*(int near *)&Kid.bytes[n])

typedef struct {
    int frame;
    signed char x;
    signed char y;
} SmokeFrame;

extern void far pascal remove_char(void);
extern SmokeFrame far *far pascal resolve_smoke_table(unsigned int handle);
extern int far pascal move_sprite_x(int x);
extern void far pascal make_sprite_clip(void);
extern void far _fastcall set_sprite_style(unsigned char style);
typedef void (far pascal *SmokeDrawer)(void);
extern void far pascal draw_smoke(void);
extern void far pascal submit_guard_sprite(SmokeDrawer draw, void near *clip, signed char id);
extern unsigned int smoke_frame, smoke_table_handle;
extern int sprite_frame, sprite_y;
extern unsigned char sprite_style, sprite_clip[];

void far pascal check_jinnee_column(void)
{
    if (Char.bytes[9] < -1 || Char.bytes[9] > 10)
        remove_char();
}

void far pascal setup_jinnee_smoke(void)
{
    SmokeFrame far *table;
    SmokeFrame far *smoke;
    unsigned int frame;

    if (smoke_frame >= 171 && smoke_frame <= 199) {
        frame = smoke_frame;
        table = resolve_smoke_table(smoke_table_handle);
        smoke = &table[frame];
        if (smoke->frame != -1) {
            sprite_frame = smoke->frame;
            move_sprite_x(smoke->x);
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
    OpponentInfo near *opponent;
    signed char column;
    signed char row;
    int x;

    Kid = Char;
    x = 85 * room_column[Kid.bytes[1]] + KID_WORD(2);
    column = get_col(x);
    row = Kid.bytes[10];
    opponent = GetOppInfoIdx((unsigned char)Kid.bytes[14],
                            Char.bytes[0] = room_info[(unsigned char)Kid.bytes[14]].next_opponent++);
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
    opponent->start_sequence = 180;
    opponent->sequence_position = 0;
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


