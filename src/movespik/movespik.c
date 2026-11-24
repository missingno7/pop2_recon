/* MOVESPIK.C, resident DOS frame 101f.
 * External names describe DOS uses. Structure members outside the accessed
 * fields are placeholders for the observed offsets and strides.
 */
typedef signed char SByte;
typedef unsigned char Byte;
typedef unsigned int Word;

#pragma pack(1)
typedef union SpikeState {
    Word word;
    int signed_word; /* Signed view of the same packed state word. */
    struct {
        Byte phase;
        Byte flags;
    } byte;
} SpikeState;

typedef struct TileState {
    SpikeState spikes;
    Word other;
} TileState;

#pragma pack()

#pragma pack(1)
typedef struct {
    int x, y;                                         /* 00,02 */
    unsigned char room;                               /* 04 */
    int velocity, auxiliary;                          /* 05,07 */
    signed char kind, row;                             /* 09-0a */
    SpikeState spikes;                               /* 0b */
} MOBREC;

typedef struct {
    unsigned char number;                             /* 00 */
    unsigned char left, right, above, below;           /* 01-04 */
    unsigned char above_left, above_right;             /* 05-06 */
    unsigned char below_left, below_right;             /* 07-08 */
    unsigned char custom;                             /* 09 */
} ROOMREC;
#pragma pack()

extern ROOMREC Room;
extern MOBREC Mob;

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
#pragma pack()
extern CHARREC Char;
#pragma pack(1)
typedef struct {
    SpikeState spikes;
    unsigned int unknown_02;
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
    signed char kind;                                 /* 00 */
    SpikeState modifier;                            /* 01 */
    signed char index;                                /* 03 */
    unsigned char room;                               /* 04 */
    signed char column, row;                          /* 05-06 */
    signed char first_col, last_col;                   /* 07-08 */
    unsigned char top_row, bottom_row;                 /* 09-0a */
} CELLREC;

typedef union {
    Word number;
    struct { Byte low, high; } byte;
} SpikeImage;

typedef struct {
    signed char index, flipped;                       /* 00-01 */
    int x, y;
    SpikeImage image;                                   /* 02,04,06 */
    unsigned char image_set;                          /* 08 */
    int clip_top, clip_left, clip_bottom, clip_right;    /* 09,0b,0d,0f */
    int depth;                                        /* 11 */
    unsigned char object_type;                        /* 13 */
} FCHARREC;
#pragma pack()

extern CELLREC Cell;
extern FCHARREC FChar;
extern int render_context;
extern int screen_edge;
extern int side_offset;
extern int ColEdgeX[20];
static const int spike_images_a[] = {0, 116, 117, 118, 119, 120, 117, 116, 120};
static const int spike_images_b[] = {0, 121, 122, 123, 124, 125, 122, 121, 126};
extern TileState *BlueS;

extern Byte FirstInRow(SByte row);
extern void add_mob_obj(int y);
extern int is_dead_pos(Word frame);
extern Byte ReadCell(Byte room, SByte column, SByte row);
extern int slide_rock_near_spikes(void);
extern SByte GetCellCol(void);
extern int get_sword_width(void);
extern void add_mob(void);
extern MOBREC *get_mob_type_item(Byte kind, int index);
extern void impale_skeleton(void);
extern void JumpSeq(Word sequence);
extern void AddSound(int sound);
extern void decrement_life_points(Byte amount);
extern void AnimChar(void);
extern void reset_char_y(void);
extern void select_room_arrays(Byte room);
/* The sprite entry consumes kind, image and context in AX, DX and BX. */
extern void _fastcall add_middle_image(Byte kind, int image, int context, int x, Byte height);

int GetSpikes(void);
MOBREC *GetSpikeMobRoomRow(Byte room, SByte row);
void DoImpale(void);
void TriggerSpikes(Byte room, int index);

/* ATMSpikes */
void ATMSpikes(void)
{
    SByte column;

    column = Mob.x / 32;
    if (Mob.room != Room.number) {
        if (Mob.room == Room.left && column == 10) {
            Mob.room = Room.number;
            column = 0;
            Mob.x -= 320;
        } else {
            Mob.velocity = -1;
        }
    }
    if (Mob.velocity != -1) {
        FChar.index = FirstInRow(Mob.row) + column;
        add_mob_obj(Mob.y);
        Mob.kind = 5;
        Mob.x -= 4;
        Mob.y += 7;
        add_mob_obj(Mob.y);
        Mob.kind = 4;
        Mob.x += 4;
        Mob.y -= 7;
    }
}

/* CheckImpale */
void CheckImpale(void)
{
    SByte column;
    MOBREC *mob;
    int x;
    SByte last_column;

    if (Char.sequence == 0x55 || Char.character_kind == 4 || Char.character_kind == 1
        || is_dead_pos(Char.frame))
        return;
    column = Char.column - 1;
    if (column <= Char.column) {
        do {
            if (ReadCell(Char.room, column, Char.row) == 2 && GetSpikes() == 2) {
                x = ColEdgeX[column + 6] + 14;
                if (Char.stance == 0)
                    x += 14;
                else
                    x += 10;
                if ((x > screen_edge
                        && Char.row * 63 + 26 <= Char.y
                        && (Char.frame < 0x79 || Char.frame > 0x84)
                        && Char.frame != 0x0f)
                    || (Level.environment_kind == 2 && slide_rock_near_spikes())) {
                    DoImpale();
                    mob = GetSpikeMobRoomRow(Char.room, Char.row);
                    if (mob) {
                        mob->spikes.word &= 0xff00;
                        mob->spikes.word |= 0xf0;
                        (&Level.cell_info[0][0])[Cell.room * 30 + Cell.index - 30].spikes.word = mob->spikes.word;
                    }
                }
            }
            last_column = Char.column;
            ++column;
        } while (column <= last_column);
    }
}

/* CheckSpikes */
void CheckSpikes(void)
{
    int x;

    if (Char.column >= 0 && Char.character_kind != 4
        && ReadCell(Char.room, Char.column - 1, Char.row) == 2
        && Char.row * 63 + 18 <= Char.y) {
        x = ColEdgeX[GetCellCol() + 6] + 14;
        x += Char.stance == 0 ? 16 : 2;
        if (Char.direction == 0) {
            x += side_offset;
            x -= get_sword_width();
        }
        if (x >= Char.x)
            TriggerSpikes(Cell.room, Cell.index);
    }
}

/* CheckSpikesJammed */
void CheckSpikesJammed(Byte room, int index)
{
    union { SByte value; Byte raw; Word storage; } row;
    Word state;
    int zero;

    if (GetSpikeMobRoomRow(room, row.value = index / 10) == 0) {
        state = (&Level.cell_info[0][0])[room * 30 + index - 30].spikes.word;
        if ((Byte)state == 0xf0) {
            Mob.x = (index % 10) * 32 + 46;
            /* Narrow the stored byte before multiplying. */
            Mob.y = (row.value = (SByte)row.raw) * 63 + 47;
            Mob.row = row.value;
            Mob.room = room;
            zero = 0;
            Mob.auxiliary = zero;
            Mob.velocity = zero;
            Mob.kind = 4;
            Mob.spikes.word = state;
            add_mob();
        }
    }
}

/* DoImpale */
void DoImpale(void)
{
    SpikeState *state;
    MOBREC *mob;
    Word sequence;

    state = &(&Level.cell_info[0][0])[Cell.room * 30 + Cell.index - 30].spikes;
    if (Char.character_kind != 4) {
        Byte flags = Cell.modifier.byte.flags & 8;
        state->word = 0xf0 + ((Word)flags << 8);
    } else {
        state->signed_word = 0x8f;
    }

    mob = get_mob_type_item(4, 1);
    if (mob && mob->room == Cell.room && mob->row == Cell.row)
        mob->spikes.word = state->word;

    reset_char_y();
    Char.vertical_step = 0;
    if (Char.character_kind == 0 || Char.character_kind == 2) {
        Char.field_0f = 1;
        Char.x = ColEdgeX[GetCellCol() + 6] + 27;
        if (Char.direction == 0) {
            sequence = Char.character_kind == 0 ? 0x36 : 0xbd;
            Char.direction = -1;
        } else {
            sequence = Char.character_kind == 0 ? 0x33 : 0xbc;
        }
        JumpSeq(sequence);
        AddSound(77);
        decrement_life_points(100);
    } else if (Char.character_kind != 4) {
        return;
    } else {
        impale_skeleton();
    }
    AnimChar();
}

/* DrawObjSpikes */
void DrawObjSpikes(Byte object)
{
    int image;

    if (FChar.image.number == 0xf0)
        FChar.image.number = 8;
    else if ((Byte)FChar.image.number & 0x80)
        FChar.image.number = 5;
    else
        FChar.image.number &= 0xff;

    if (object == 0x84)
        image = spike_images_a[FChar.image.number];
    else if (object == 0x85)
        image = spike_images_b[FChar.image.number];
    else
        return;

    if (image)
        add_middle_image(4, image, render_context, FChar.y, 10);
}

/* GetSpikeIdx */
int GetSpikeIdx(Word state)
{
    int index;
    if (state & 0x80) {
        index = 5;
    } else {
        state &= 7;
        index = state;
    }
    return index;
}

/* GetSpikeMobRoomRow */
MOBREC *GetSpikeMobRoomRow(Byte room, SByte row)
{
    int found = 0;
    int index = 1;
    MOBREC *mob;

    do {
        mob = get_mob_type_item(4, index++);
        if (!mob || (mob->room == room && mob->row == row))
            found = 1;
    } while (!found);
    return mob;
}

/* GetSpikes */
int GetSpikes(void)
{
    int state;
    int result;

    state = Cell.modifier.byte.phase;
    if (state && state != 0xf0) {
        if (state & 0x80)
            result = 1;
        else if (state < 5)
            result = 2;
        else
            result = 0;
    } else {
        result = 0;
    }
    return result;
}

/* MobSpikes */
void MobSpikes(void)
{
    Word state;
    TileState *room;

    ReadCell(Mob.room, Mob.x / 32 - 1, Mob.row);
    state = Cell.modifier.byte.phase;
    if (state != 0xf0) {
        if (state & 0x80) {
            --*(Byte *)&state; /* Decrement the low byte without changing the flags. */
            if (!(state & 0x7f)) {
                state = 6;
                AddSound(79);
            }
        } else {
            ++state;
            if (state == 5)
                state = 0x8f;
            else if (state == 8) {
                state = 0;
                Mob.velocity = -1;
            }
        }
        select_room_arrays(Cell.room);
        room = BlueS;
        room[Cell.index].spikes.word = (state += Cell.modifier.word & 0x800);
        Mob.spikes.word = state;
    }
}

/* TriggerSpikes */
void TriggerSpikes(Byte room, int index)
{
    Word next;
    SpikeState *spikes;
    Word state;
    SByte row;
    MOBREC *mob;
    int zero;

    spikes = &BlueS[index].spikes;
    /* Taking the addresses of the pointer locals affects MSC allocation. */
    state = (*(&spikes))->byte.phase;
    if (state == 0 || (state & 0x80)) {
        if (state == 0) {
            Mob.x = (index % 10) * 32 + 46;
            row = index / 10;
            Mob.y = row * 63 + 47;
            Mob.row = row;
            Mob.room = room;
            zero = 0;
            Mob.auxiliary = zero;
            Mob.velocity = zero;
            Mob.kind = 4;
            next = spikes->word + 1;
            spikes->word = next;
            Mob.spikes.word = next;
            add_mob();
            AddSound(78);
        } else if (state != 0xf0) {
            next = (Cell.modifier.word & 0x800) + 0x8f;
            spikes->word = next;
            mob = GetSpikeMobRoomRow(Char.room, Char.row);
            if (mob)
                (*(&mob))->spikes.word = next;
        }
    }
}
