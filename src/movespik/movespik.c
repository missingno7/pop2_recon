#include "types.h"
#include "graf.h"
#include "shape.h"
#include "level.h"
#include "char.h"
#include "game.h"
#include "draw.h"
#include "act.h"
#include "add.h"
#include "autoguar.h"
#include "ctrl.h"
#include "movetemp.h"
#include "fade.h"
#include "misc.h"
#include "move.h"
#include "collide.h"
#include "movespik.h"
#include "prince.h"
#include "sound.h"
#include "tables.h"
#include "top.h"

#pragma pack(1)
typedef union SpikeState {
    word word;
    int signed_word; /* Signed view of the same packed state word. */
    struct {
        byte phase;
        byte flags;
    } byte;
} SpikeState;


#pragma pack()


#pragma pack(1)

typedef union {
    word number;
    struct { byte low, high; } byte;
} SpikeImage;

#pragma pack()


static const int spike_images_a[] = {0, 116, 117, 118, 119, 120, 117, 116, 120};
static const int spike_images_b[] = {0, 121, 122, 123, 124, 125, 122, 121, 126};


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
        FChar.cell = FirstInRow(Mob.row) + column;
        AddMobObj(Mob.y);
        Mob.kind = 5;
        Mob.x -= 4;
        Mob.y += 7;
        AddMobObj(Mob.y);
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
        || IsDeadPos(Char.frame))
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
                    || (Level.environment_kind == 2 && SlideRockNearSpikes())) {
                    DoImpale();
                    mob = GetSpikeMobRoomRow(Char.room, Char.row);
                    if (mob) {
                        mob->flags &= 0xff00;
                        mob->flags |= 0xf0;
                        Level.cell_info[Cell.room - 1][Cell.index].modifier = mob->flags;
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
            x -= GetSwordWidth();
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

    if (GetSpikeMobRoomRow(room, row.value = index / 10) == 0) {
        state = Level.cell_info[room - 1][index].modifier;
        if ((byte)state == 0xf0) {
            Mob.x = (index % 10) * 32 + 46;
            /* Narrow the stored byte before multiplying. */
            Mob.y = (row.value = row.raw) * 63 + 47;
            Mob.row = row.value;
            Mob.room = room;
            Mob.auxiliary = 0;
            Mob.velocity = 0;
            Mob.kind = 4;
            Mob.flags = state;
            AddMob();
        }
    }
}

/* DoImpale */
void DoImpale(void)
{
    word *state;
    MOBREC *mob;
    word sequence;

    state = &Level.cell_info[Cell.room - 1][Cell.index].modifier;
    if (Char.character_kind != 4) {
        byte flags = ((byte *)&Cell.modifier)[1] & 8;
        *state = 0xf0 + (flags << 8);
    } else {
        *(int *)state = 0x8f;
    }

    mob = GetMobTypeItem(4, 1);
    if (mob && mob->room == Cell.room && mob->row == Cell.row)
        mob->flags = *state;

    ResetCharY();
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
        DecLifePts(100);
    } else if (Char.character_kind != 4) {
        return;
    } else {
        SkelDoImpale();
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
        AddMid(4, image, XCo, FChar.y, 10);
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
        mob = GetMobTypeItem(4, index++);
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
    LEVELCELL *room;

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
        room[Cell.index].modifier = (state += Cell.modifier & 0x800);
        Mob.flags = state;
    }
}

void TriggerSpikes(byte room, int index)
{
    word state, next;
    sbyte row;
    MOBREC *mob;

    state = BlueS[index].modifier & 255;
    if (state == 0 || (state & 0x80)) {
        if (state == 0) {
            Mob.x = (index % 10) * 32 + 46;
            row = index / 10;
            Mob.y = row * 63 + 47;
            Mob.row = row;
            Mob.room = room;
            Mob.velocity = Mob.auxiliary = 0;
            Mob.kind = 4;
            next = BlueS[index].modifier + 1;
            BlueS[index].modifier = next;
            Mob.flags = next;
            AddMob();
            AddSound(78);
        } else if (state != 0xf0) {
            next = (Cell.modifier & 0x800) + 0x8f;
            BlueS[index].modifier = next;
            mob = GetSpikeMobRoomRow(Char.room, Char.row);
            if (mob)
                mob->flags = next;
        }
    }
}
