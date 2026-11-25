/* MOVECAVE.C, DOS cave movement overlay. */
#include <string.h>
#include <malloc.h>
#include "types.h"
#include "graf.h"
#include "shape.h"
#include "level.h"
#include "char.h"
#include "movescyt.h"
#include "act.h"
#include "game.h"
#include "prince.h"
#include "draw.h"
#include "misc.h"
#include "auto.h"
#include "collide.h"
#include "ctrl.h"
#include "fade.h"
#include "gfxerr.h"
#include "bridge.h"
#include "move.h"
#include "movecave.h"
#include "sound.h"
#include "tables.h"
#include "top.h"
#include "util.h"


static LAVAINFO *lava_slots[4];


static Rect dart_rect = {54, 16, 59, 23};
static Rect lava_rect = {48, 5, 62, 64};
static LAVASEED lava_seeds[15] = {
    { 0, 2, { -6, 31}, 1},
    { 0, 1, {-10, 32}, 1},
    { 0, 3, { -5, 33}, 0},
    { 2, 5, { -7, 13}, 1},
    { 2, 5, { -9, 43}, 0},
    { 3, 5, { -7, 37}, 1},
    { 5, 5, { -5, 21}, 1},
    { 6, 1, { -8, 11}, 0},
    { 7, 4, { -1, 32}, 1},
    {11, 4, { -6, 36}, 0},
    {12, 5, {-10, 27}, 0},
    {16, 0, { -3,  0}, 1},
    {18, 1, { -3, 16}, 1},
    {20, 0, {-10, 24}, 0},
    {21, 0, { -7, 26}, 0}
};

static sbyte lava_sequence_0[] = {0, 1, 2, 3, 4, 5, 6, 14, 13, -1};
static sbyte lava_sequence_1[] = {0, 1, 2, 3, -2, 4, 16, 12, -1};


int TriggerGateWithRevPlate(sbyte room, sbyte cell)
{
    int cell_index, pressed, result;
    CELLREC previous_tile;
    word low, high, flags, state;
    LEVELCELL *previous_cells;
    sbyte *previous_number;
    TROBREC *trob;

    result = -1;
    cell_index = cell;
    state = BlueS[cell].modifier;
    low = state & 255;
    high = state & 0xff00;
    pressed = IsLoosePushingPlate();
    trob = GetTrobRoomCell(room, cell_index);
    if (trob != 0) {
        if (trob->state == 0 || trob->state == 1) {
            if (pressed)
                flags = Cell.modifier;
            else {
                previous_tile = Cell;
                previous_number = BlueT;
                previous_cells = BlueS;
                flags = ReadCell(Char.room, Char.column, Char.row) == 34
                    ? Cell.modifier : 0;
                BlueS = previous_cells;
                BlueT = previous_number;
                Cell = previous_tile;
            }
            if (flags & 0x0800) {
                BlueS[cell].modifier = high + 250;
            } else {
                if (low > 200)
                    BlueS[cell].modifier = high + 200;
                trob->state = 4;
            }
        } else if (trob->state == 2 && pressed)
            trob->state = 3;
    } else if (low != 255) {
        if (low == 0 && !(Char.character_kind == 4 && Char.row != 0 && Level.level_number == 5 &&
                         (Char.room == 10 || Char.room == 7 || Char.room == 12))) {
            if (!pressed) {
                if (!(Cell.modifier & 0x0800))
                    result = 2;
            } else
                result = 3;
        } else if (low != 0 && low != 36 && Kid.alive < 0)
            result = 4;
    }
    return result;
}

void AnimDart(void)
{
    word frame = BlueSpec.modifier & 15;

    if (frame < 6) {
        ++BlueSpec.modifier;
        if (frame < 3)
            RedrawDart();
    } else {
        BlueSpec.modifier &= 255 << 8 | 0xf0;
        Trob.state = -1;
    }
}

void ATMDart(void)
{
    if (Mob.velocity != -1 && Mob.velocity != -2) {
        FChar.cell = IndexCell1(Mob.x / 32, GetRow(Mob.y));
        if (Room.number == Mob.room) {
            MarkMobRedraw(1);
            AddMobObj(Mob.y);
            Mob.kind = 2;
        }
    }
}

void CheckDartHit(void)
{
    Rect hit, bounds;
    int index;
    MOBREC *dart_mob;

    if (!(Char.frame >= 108 && Char.frame <= 112 && Char.sequence == 79)) {
        index = 1;
        do {
            dart_mob = GetMobTypeItem(2, index++);
            if (dart_mob && dart_mob->velocity != -1) {
                bounds.right = dart_mob->x + 14;
                bounds.left = bounds.right - 4;
                bounds.bottom = dart_mob->y;
                bounds.top = dart_mob->y - 2;
                if (!SectRect(&hit, &bounds, &Char.bounds) &&
                    (Char.bounds.left > bounds.left ||
                    Char.bounds.top - 2 > bounds.bottom ||
                    Char.bounds.bottom - 2 < bounds.top ||
                    bounds.left - dart_mob->auxiliary >= Char.bounds.right ||
                    bounds.bottom - dart_mob->velocity >= Char.bounds.bottom))
                    continue;
                HitCharWithDart();
                dart_mob->velocity = -1;
            }
        } while (dart_mob);
    }
}

void MobDart(void)
{
    int x, y;
    byte tile;

    if (Mob.velocity == -2) {
        Mob.velocity = -1;
        return;
    }
    if (Mob.velocity != -1) {
        y = Mob.y + Mob.velocity;
        x = Mob.auxiliary + Mob.x;
        if (x > 320) {
            if ((byte)(Room.left) == Mob.room) {
                Mob.room = Room.number;
                x -= 320;
            } else {
                if (!Level.room_links[Mob.room - 1].right)
                    AddSound(82);
                Mob.velocity = -2;
            }
        }
        if (Mob.velocity != -1 && y < 3) {
            if (Room.below == Mob.room) {
                Mob.room = Room.number;
                y += 192;
            } else {
                if (!Level.room_links[Mob.room - 1].above)
                    AddSound(82);
                Mob.velocity = -2;
            }
        }
        if (Mob.velocity != -2) {
            int next_x = x;

            if (63 * Mob.row + 3 >= y) {
                if (!IsSpace(ReadCell(Mob.room, next_x / 32, --Mob.row)))
                    Mob.velocity = -2;
            } else if (next_x / 32 != Mob.x / 32) {
                tile = ReadCell(Mob.room, next_x / 32, Mob.row);
                if (IsWall(tile) || (tile == 4 && GateBarr()))
                    Mob.velocity = -2;
            }
            if (Mob.velocity == -2)
                AddSound(82);
            Mob.y = y;
            Mob.x = next_x;
        }
    }
}

void HitCharWithDart(void)
{
    int edge;

    if (Char.alive < 0) {
        if (!DecLifePts(1)) {
            Char.sequence_group = 8;
            if (Char.character_kind == 0)
                AddSound(31);
            else
                AddSound(72);
        } else {
            edge = GetDist();
            if (IsSpace(GetBehind()) && edge >= 4) {
                Char.x = AddCharX(edge - 40);
                RereadCells();
                ++Char.row;
                JumpSeq(18);
            } else {
                StabKillChar();
                ResetCharY();
                Char.vertical_step = 0;
            }
        }
        AddSound(81);
        SetKidDeathMethod(0);
    }
}

void RedrawDart(void)
{
    if (CheckTrobVis())
        RedrawTrob(&dart_rect);
    else
        Trob.state = 1;
}

void TriggerDart(sbyte room, sbyte cell)
{
    MOBREC saved;
    word frame;
    int x;

    frame = BlueS[cell].modifier & 15;
    saved = Mob;
    if (!frame) {
        AddTrob(room, cell, 1, 36);
        AddSound(80);
        Mob.auxiliary = 18;
        Mob.velocity = -11;
        x = Col2PixelX[Trob.cell % 10 + 1] + 30;
        if (x > 320) {
            if (Room.left == (byte)room) {
                room = Room.number;
                x -= 320;
            } else
                Mob.velocity = -1;
        }
        if (Mob.velocity != -1) {
            Mob.x = x;
            Mob.room = room;
            Mob.y = Row2PixelY[Trob.cell / 10 + 1] - 15;
            Mob.kind = 2;
            Mob.row = Trob.cell / 10;
            AddMob();
        }
    }
    Mob = saved;
}

static sbyte lava_sequence_2[] = {3, 4, 5, 6, 14, 13, -1};
static sbyte lava_sequence_3[] = {0, 1, 2, 3, 4, 5, 6, 14, 6, 5, 4, 3, 2, 1, 0, -1};
static sbyte lava_sequence_4[] = {0, 1, 2, 3, 4, 15, 12, -2, 7, 8, 9, 10, 11, -1};
static sbyte lava_sequence_5[] = {0, 1, 2, 3, 4, 5, 6, 14, 13, -2, 16, 9, 10, 11, -1};

sbyte *BubbleScript[6] = {
    lava_sequence_0, lava_sequence_1, lava_sequence_2,
    lava_sequence_3, lava_sequence_4, lava_sequence_5
};

static const char too_many_lava_pools[] = "All global lava info structs have been used.";

void AnimLava(void)
{
    sbyte frame, i, next;
    LAVAINFO *pool;
    LAVABUBBLE *bubble;
    int persistent;

    i = Trob.state & 7;
    pool = lava_slots[i];
    if (CheckTrobVis()) {
        frame = pool->frame & 63;
        if (frame != 23 && !((pool->frame & 128) && frame == 3)) {
            ++pool->frame;
            if (!(pool->frame & 128) || frame >= 3) {
                if (pool->frame & 128) {
                    if (frame == 4) {
                        pool->frame &= 127;
                        AddSound(97);
                    }
                    frame -= 3;
                }
                for (i = 0; i < 10; ++i) {
                    bubble = &pool->bubbles[i];
                    if (bubble->active) {
                        ++bubble->frame;
                        next = BubbleScript[bubble->kind][bubble->frame];
                        if (next == -1)
                            bubble->active = 0;
                        else if (next == -2) {
                            bubble->layer = 1;
                            ++bubble->frame;
                        }
                    }
                }
                for (i = 0; i < 15; ++i) {
                    if (lava_seeds[i].frame > frame)
                        break;
                    if (lava_seeds[i].frame == frame)
                        TriggerLavaBubble(pool, &lava_seeds[i]);
                }
            }
        } else {
            persistent = pool->frame & 64;
            ResetLavaInfo(pool, 0);
            if (frame == 3) {
                pool->frame = 0x84;
                if (persistent)
                    pool->frame = 0xc4;
            }
        }
        RedrawLava();
    } else {
        _nfree(pool);
        lava_slots[i] = 0;
    }
}

void CheckLavaBoil(void)
{
    word kind;
    int y, center, reach;
    kind = ReadCell(Char.room, Char.column, Char.row);
    y = 63 * Char.row + 41;
    center = GetBaseX();
    reach = ColEdgeX[Char.column + 5] - center + 26;
    if ((kind == 23 || (kind == 24 && reach >= 0)) &&
        (Frame.flags & 64) && y < Char.y && Char.sequence_group != 5)
        DoLavaBoil();
}

void DoLavaBoil(void)
{
    LAVAINFO *pool;
    TROBREC *trob;

    pool = GetLavaInfoPtr(Char.room, Char.column, Char.row);
    if (pool) {
        pool->frame = 0x80;
        if (Char.character_kind == 0 && Rnd(3) == 0) {
            JumpSeq(116);
            pool->frame |= 64; AddSound(99);
        } else {
            if (Char.character_kind == 0) AddSound(99);
            else AddSound(98);
            JumpSeq(115);
        }
        Char.alert_delay = 0;
        Char.repeat = 1;
        Char.sequence_group = 5;
        ClearCharVel();
        DecLifePts(100);
        AnimChar();
        trob = GetLavaTrob(Char.room, Char.column, Char.row);
        if (trob) {
            Trob = *trob;
            RedrawLava();
        }
    }
}

byte GetFreeLavaInfoIdx(void)
{
    byte slot = 0;
    do {
        if (!lava_slots[slot])
            break;
        ++slot;
    } while (slot < 4);
    if (slot == 4) {
        show_message(too_many_lava_pools, 1);
        --slot;
    }
    return slot;
}

LAVAINFO *GetLavaInfoPtr(sbyte room, sbyte col, sbyte row)
{
    LAVAINFO *result;
    TROBREC *trob;

    trob = GetLavaTrob(room, col, row);
    if (trob)
        result = lava_slots[trob->state];
    else
        result = 0;
    return result;
}

TROBREC *GetLavaTrob(sbyte room, sbyte col, sbyte row)
{
    sbyte cell;
    byte kind;
    TROBREC *trob;

    kind = ReadCell(room, col, row);
    cell = Cell.index;
    if (kind == 24)
        --cell;
    trob = GetTrobRoomCell(room, cell);
    select_room_arrays(Room.number);
    return trob;
}

void InitLavaGlobals(void)
{
    int i;

    for (i = 0; i < 4; ++i) {
        if (lava_slots[i]) {
            _nfree(lava_slots[i]);
            lava_slots[i] = 0;
        }
    }
}

void RedrawLava(void)
{
    Rect bounds;

    if (lava_slots[Trob.state]->frame & 128)
        lava_rect.top = 16;
    else if (lava_slots[Trob.state]->frame & 64)
        lava_rect.top = 39;
    else
        lava_rect.top = 48;
    if (ComputeTrobMoveRect(&bounds, &lava_rect)) {
        MarkMove(Trob.cell, &bounds);
        MarkMove(CheckTrobRight(), &bounds);
        if (Trob.cell % 10 < 8) {
            ++Trob.cell;
            MarkMove(CheckTrobRight(), &bounds);
            --Trob.cell;
        }
    }
}

void ResetLavaInfo(LAVAINFO *pool, int type)
{
    memset(pool, 0, sizeof(*pool));
    switch (type) {
    default:
        pool->frame = 0;
        TriggerLavaBubble(pool, &lava_seeds[0]);
        TriggerLavaBubble(pool, &lava_seeds[1]);
        TriggerLavaBubble(pool, &lava_seeds[2]);
        TriggerLavaBubble(pool, &lava_seeds[13]);
        pool->bubbles[3].frame = 5;
        TriggerLavaBubble(pool, &lava_seeds[14]);
        pool->bubbles[4].frame = 4;
        break;
    case 1:
        pool->frame = 3;
        TriggerLavaBubble(pool, &lava_seeds[5]);
        TriggerLavaBubble(pool, &lava_seeds[4]);
        pool->bubbles[1].frame = 1;
        TriggerLavaBubble(pool, &lava_seeds[3]);
        pool->bubbles[1].frame = 1;
        TriggerLavaBubble(pool, &lava_seeds[2]);
        pool->bubbles[3].frame = 3;
        TriggerLavaBubble(pool, &lava_seeds[1]);
        pool->bubbles[3].frame = 3;
        TriggerLavaBubble(pool, &lava_seeds[0]);
        pool->bubbles[3].frame = 3;
        break;
    case 2:
        pool->frame = 11;
        TriggerLavaBubble(pool, &lava_seeds[10]);
        TriggerLavaBubble(pool, &lava_seeds[9]);
        pool->bubbles[1].frame = 4;
        TriggerLavaBubble(pool, &lava_seeds[8]);
        pool->bubbles[2].frame = 5;
        break;
    }
}

void TriggerLava(sbyte room, sbyte cell)
{
    byte slot;
    if (!GetTrobRoomCell(room, cell)) {
        slot = GetFreeLavaInfoIdx();
        lava_slots[slot] = _nmalloc(sizeof(LAVAINFO));
        ResetLavaInfo(lava_slots[slot], cell % 3);
        AddTrob(room, cell, slot, 23);
    }
}

void TriggerLavaBubble(LAVAINFO *pool, LAVASEED *seed)
{
    int index;

    for (index = 0; index < 10; ++index) {
        if (!pool->bubbles[index].active)
            break;
    }
    if (index < 10) {
        pool->bubbles[index].active = 1;
        pool->bubbles[index].frame = 0;
        pool->bubbles[index].kind = seed->kind;
        pool->bubbles[index].pos = seed->pos;
        pool->bubbles[index].layer = seed->layer;
    }
}

int CaveMoveSquishingGate(int sequence)
{
    sbyte cell;

    cell = FirstInRow(Kid.row) + Kid.column;
    if (Kid.room != Trob.room) {
        if (Room.left == Trob.room)
            cell += 10;
        else if (Room.right == Trob.room)
            cell -= 10;
        else
            cell = 30;
    }
    if (cell == Trob.cell || Trob.cell - cell + 1 == 0) {
        if (Kid.sequence_group == 3) {
            sequence = 12;
            AddGateSound(9);
        } else if (Kid.frame == 264) {
            sequence -= GateInc[Trob.state] + 2;
        } else if (Kid.frame == 265) {
            sequence = 36;
        } else if (Kid.frame == 266) {
            Trob.state = -1;
            RedrawGate();
        }
    }
    return sequence;
}

word CaveGetTorchFrame(word previous)
{
    word frame = Rnd(8);
    if (frame == previous) {
        ++frame;
        if (frame >= 9)
            frame = 0;
    }
    return frame;
}

void cave_overlay_entry(void)
{
}

void CaveFrameFunc(void)
{
    if ((Level.level_number == 5 && Room.number == 10) ||
        (Level.level_number == 5 && Room.number == 7) ||
        (Level.level_number == 5 && Room.number == 12))
        BridgeFrameFunc();
}

void InitBridgeGlobals(void)
{
    SetPalette(3500, 64, 160, 0);
    Bridge.broken = 0;
    memset(Bridge.floors, 0, sizeof(Bridge.floors));
}
