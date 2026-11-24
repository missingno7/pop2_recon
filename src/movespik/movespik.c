/* MOVESPIK.C, resident DOS frame 101f.
 * External names describe DOS uses. Structure members outside the accessed
 * fields are placeholders for the observed offsets and strides.
 */
#include "types.h"
#include "level.h"
#include "char.h"

#pragma pack(1)
typedef union SpikeState {
    word word;
    int signed_word; /* Signed view of the same packed state word. */
    struct {
        byte phase;
        byte flags;
    } byte;
} SpikeState;

typedef struct TileState {
    SpikeState spikes;
    word other;
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

#pragma pack()

extern ROOMREC Room;
extern MOBREC Mob;

extern CHARREC Char;

extern LEVELREC Level;

#pragma pack(1)

typedef union {
    word number;
    struct { byte low, high; } byte;
} SpikeImage;

#pragma pack()

extern CELLREC Cell;
extern FCHARREC FChar;
extern int XCo;
extern int CDLeftEdge;
extern int ImWidth;
extern int ColEdgeX[20];
static const int spike_images_a[] = {0, 116, 117, 118, 119, 120, 117, 116, 120};
static const int spike_images_b[] = {0, 121, 122, 123, 124, 125, 122, 121, 126};
extern TileState *BlueS;

extern byte FirstInRow(sbyte row);
extern void add_mob_obj(int y);
extern int is_dead_pos(word frame);
extern byte ReadCell(byte room, sbyte column, sbyte row);
extern int slide_rock_near_spikes(void);
extern sbyte GetCellCol(void);
extern int get_sword_width(void);
extern void add_mob(void);
extern MOBREC *get_mob_type_item(byte kind, int index);
extern void impale_skeleton(void);
extern void JumpSeq(word sequence);
extern void AddSound(int sound);
extern void decrement_life_points(byte amount);
extern void AnimChar(void);
extern void reset_char_y(void);
extern void select_room_arrays(byte room);
/* The sprite entry consumes kind, image and context in AX, DX and BX. */
extern void _fastcall add_middle_image(byte kind, int image, int context, int x, byte height);

int GetSpikes(void);
MOBREC *GetSpikeMobRoomRow(byte room, sbyte row);
void DoImpale(void);
void TriggerSpikes(byte room, int index);

/* ATMSpikes */
void ATMSpikes(void)
{
    sbyte column;

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
    sbyte column;
    MOBREC *mob;
    int x;
    sbyte last_column;

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
                if ((x > CDLeftEdge
                        && Char.row * 63 + 26 <= Char.y
                        && (Char.frame < 0x79 || Char.frame > 0x84)
                        && Char.frame != 0x0f)
                    || (Level.environment_kind == 2 && slide_rock_near_spikes())) {
                    DoImpale();
                    mob = GetSpikeMobRoomRow(Char.room, Char.row);
                    if (mob) {
                        mob->spikes.word &= 0xff00;
                        mob->spikes.word |= 0xf0;
                        (&Level.cell_info[0][0])[Cell.room * 30 + Cell.index - 30].modifier = mob->spikes.word;
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
            x += ImWidth;
            x -= get_sword_width();
        }
        if (x >= Char.x)
            TriggerSpikes(Cell.room, Cell.index);
    }
}

/* CheckSpikesJammed */
void CheckSpikesJammed(byte room, int index)
{
    union { sbyte value; byte raw; word storage; } row;
    word state;
    int zero;

    if (GetSpikeMobRoomRow(room, row.value = index / 10) == 0) {
        state = (&Level.cell_info[0][0])[room * 30 + index - 30].modifier;
        if ((byte)state == 0xf0) {
            Mob.x = (index % 10) * 32 + 46;
            /* Narrow the stored byte before multiplying. */
            Mob.y = (row.value = row.raw) * 63 + 47;
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
    word sequence;

    state = (SpikeState *)&(&Level.cell_info[0][0])[Cell.room * 30 + Cell.index - 30].modifier;
    if (Char.character_kind != 4) {
        byte flags = ((byte *)&Cell.modifier)[1] & 8;
        state->word = 0xf0 + (flags << 8);
    } else {
        state->signed_word = 0x8f;
    }

    mob = get_mob_type_item(4, 1);
    if (mob && mob->room == Cell.room && mob->row == Cell.row)
        mob->spikes.word = state->word;

    reset_char_y();
    Char.vertical_step = 0;
    if (Char.character_kind == 0 || Char.character_kind == 2) {
        Char.repeat = 1;
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
void DrawObjSpikes(byte object)
{
    int image;

    if (FChar.image == 0xf0)
        FChar.image = 8;
    else if (FChar.image & 0x80)
        FChar.image = 5;
    else
        FChar.image &= 0xff;

    if (object == 0x84)
        image = spike_images_a[FChar.image];
    else if (object == 0x85)
        image = spike_images_b[FChar.image];
    else
        return;

    if (image)
        add_middle_image(4, image, XCo, FChar.y, 10);
}

/* GetSpikeIdx */
int GetSpikeIdx(word state)
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
MOBREC *GetSpikeMobRoomRow(byte room, sbyte row)
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

    state = *(byte *)&Cell.modifier;
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
    word state;
    TileState *room;

    ReadCell(Mob.room, Mob.x / 32 - 1, Mob.row);
    state = *(byte *)&Cell.modifier;
    if (state != 0xf0) {
        if (state & 0x80) {
            --*(byte *)&state; /* Decrement the low byte without changing the flags. */
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
        room[Cell.index].spikes.word = (state += Cell.modifier & 0x800);
        Mob.spikes.word = state;
    }
}

/* TriggerSpikes */
void TriggerSpikes(byte room, int index)
{
    word next;
    SpikeState *spikes;
    word state;
    sbyte row;
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
            next = (Cell.modifier & 0x800) + 0x8f;
            spikes->word = next;
            mob = GetSpikeMobRoomRow(Char.room, Char.row);
            if (mob)
                (*(&mob))->spikes.word = next;
        }
    }
}
