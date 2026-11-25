/* MOVECAVE.C, DOS cave movement overlay. */
#include "types.h"
#include "level.h"
#include "char.h"
#include "game.h"

#pragma pack(1)
typedef struct {
    int sequence;
    byte action;
} TrobRecord;

typedef struct {
    word state;
    int other;
} CellState;

typedef struct {
    int y, x;
    sbyte room;
    int top, left;
} Projectile;

typedef struct {
    sbyte cell;
    sbyte room;
    sbyte slot;
    sbyte other;
} LavaTrob;

struct item {
    int active;
    char state;
    char type;
    char far *data;
    int tail;
};
struct list {
    char prefix;
    struct item entries[10];
};
struct input {
    char prefix;
    char value;
    char far *data;
    int tail;
};
#pragma pack()

extern CellState *BlueS;
extern int BlueT;
extern CELLREC Cell;
#pragma pack(1)

extern CHARREC Char;

#pragma pack()
extern LEVELREC Level;
extern CHARREC Kid;

extern word BlueSpec;
extern MOBREC Mob;
extern LavaTrob Trob;
#define animation_code Trob.slot
extern FRAMEREC Frame;
extern FCHARREC FChar;
extern int ColEdgeX[20];
extern int Col2PixelX[13];
extern ROOMREC Room;
extern int Row2PixelY[5];
static struct list *lava_slots[4];
extern sbyte GateInc[];
/* 16-bit bubble coordinates are copied together through the legacy input
   record by TriggerLavaBubble. */
struct BubbleSeed {
    sbyte frame, kind;
    int x, y, flags;
};

static struct {
    int dart_redraw[4];
    int lava_redraw[4];
    struct BubbleSeed bubbles[15];
} cave_motion = {
    {54, 16, 59, 23},
    {48, 5, 62, 64},
    {
        { 0, 2,  -6, 31, 1},
        { 0, 1, -10, 32, 1},
        { 0, 3,  -5, 33, 0},
        { 2, 5,  -7, 13, 1},
        { 2, 5,  -9, 43, 0},
        { 3, 5,  -7, 37, 1},
        { 5, 5,  -5, 21, 1},
        { 6, 1,  -8, 11, 0},
        { 7, 4,  -1, 32, 1},
        {11, 4,  -6, 36, 0},
        {12, 5, -10, 27, 0},
        {16, 0,  -3,  0, 1},
        {18, 1,  -3, 16, 1},
        {20, 0, -10, 24, 0},
        {21, 0,  -7, 26, 0}
    }
};

static sbyte lava_sequence_0[] = {0, 1, 2, 3, 4, 5, 6, 14, 13, -1};
static sbyte lava_sequence_1[] = {0, 1, 2, 3, -2, 4, 16, 12, -1};
typedef struct {
    int wait, broken;
    unsigned char floors[8];
} BRIDGEREC;
extern BRIDGEREC Bridge;

#define dart_redraw_type cave_motion.dart_redraw[0]
#define lava_redraw_type cave_motion.lava_redraw[0]
#define lava_bubbles ((struct input *)cave_motion.bubbles)

extern int IsLoosePushingPlate(void);
extern TrobRecord *GetTrobRoomCell(sbyte, int);
extern byte ReadCell(sbyte, sbyte, sbyte);
extern void AddTrob(sbyte, sbyte, sbyte, sbyte);
extern int CheckTrobVis(void);
extern void RedrawTrob(int *);
extern sbyte IndexCell1(int);
extern int GetRow(int, int);
extern void MarkMobRedraw(sbyte);
extern void AddMobObj(int);
extern Projectile *GetMobTypeItem(sbyte, int);
extern int SECTRECT(Rect far *, Rect far *, Rect far *);
extern void AddSound(int);
extern int IsSpace(byte);
extern int IsWall(byte);
extern int GateBarr(void);
extern int DecLifePts(sbyte);
extern int GetDist(void);
extern int GetBehind(void);
extern int AddCharX(int);
extern void RereadCells(void);
extern void JumpSeq(int);
extern void StabKillChar(void);
extern void ResetCharY(void);
extern void SetKidDeathMethod(sbyte);
extern void AddMob(void);
extern int GetBaseX(void);
extern unsigned int Rnd(unsigned int);
extern void ClearCharVel(void);
extern void AnimChar(void);
extern void select_room_arrays(sbyte);
extern void MarkMove(sbyte, Rect *);
extern int ComputeTrobMoveRect(Rect *, int *);
extern sbyte CheckTrobRight(void);
extern void AddGateSound(int);
extern sbyte FirstInRow(sbyte);
extern void RedrawGate(void);
extern void BridgeFrameFunc(void);
extern void SetPalette(int, int, byte, byte);
extern void show_message(const char far *, int);
extern void *cdecl _nmalloc(word);
extern void cdecl _nfree(void *);
extern void *cdecl memset(void *, int, word);
#pragma intrinsic(memset)

void RedrawDart(void);
void HitCharWithDart(void);
void DoLavaBoil(void);
struct list *GetLavaInfoPtr(sbyte, sbyte, sbyte);
LavaTrob *GetLavaTrob(sbyte room, sbyte col, sbyte row);
void RedrawLava(void);
void ResetLavaInfo(struct list *pool, int type);
void far pascal TriggerLavaBubble(struct list *destination, struct input *source);

int TriggerGateWithRevPlate(sbyte room, sbyte cell)
{
    int cell_index, pressed, result;
    CELLREC previous_tile;
    word low, high, flags, state;
    CellState *previous_cells;
    int previous_number;
    TrobRecord *trob;

    result = -1;
    cell_index = cell;
    state = BlueS[cell].state;
    low = state & 255;
    high = state & 0xff00;
    pressed = IsLoosePushingPlate();
    trob = GetTrobRoomCell(room, cell_index);
    if (trob != 0) {
        if (trob->action == 0 || trob->action == 1) {
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
                BlueS[cell].state = high + 250;
            } else {
                if (low > 200)
                    BlueS[cell].state = high + 200;
                trob->action = 4;
            }
        } else if (trob->action == 2 && pressed)
            trob->action = 3;
    } else if (low != 255) {
        if (low == 0 && !(Char.character_kind == 4 && Char.row != 0 && (Level.level_number) == 5 &&
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
    word frame = BlueSpec & 15;

    if (frame < 6) {
        ++BlueSpec;
        if (frame < 3)
            RedrawDart();
    } else {
        BlueSpec &= 255 << 8 | 0xf0;
        animation_code = -1;
    }
}

void ATMDart(void)
{
    if (Mob.velocity != -1 && Mob.velocity != -2) {
        FChar.index = IndexCell1(GetRow(Mob.x / 32, Mob.y));
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
    Projectile *dart_mob;

    if (!(Char.frame >= 108 && Char.frame <= 112 && Char.sequence == 79)) {
        index = 1;
        do {
            dart_mob = GetMobTypeItem(2, index++);
            if (dart_mob && dart_mob->top != -1) {
                bounds.right = dart_mob->y + 14;
                bounds.left = bounds.right - 4;
                bounds.bottom = dart_mob->x;
                bounds.top = dart_mob->x - 2;
                if (!SECTRECT(&hit, &bounds, &Char.bounds) &&
                    (Char.bounds.left > bounds.left ||
                    Char.bounds.top - 2 > bounds.bottom ||
                    Char.bounds.bottom - 2 < bounds.top ||
                    bounds.left - dart_mob->left >= Char.bounds.right ||
                    bounds.bottom - dart_mob->top >= Char.bounds.bottom))
                    continue;
                HitCharWithDart();
                dart_mob->top = -1;
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
        RedrawTrob(&dart_redraw_type);
    else
        animation_code = 1;
}

void TriggerDart(sbyte room, sbyte cell)
{
    MOBREC saved;
    word frame;
    int x;

    frame = BlueS[cell].state & 15;
    saved = Mob;
    if (!frame) {
        AddTrob(room, cell, 1, 36);
        AddSound(80);
        Mob.auxiliary = 18;
        Mob.velocity = -11;
        x = Col2PixelX[Trob.cell % (sbyte)10 + 1] + 30;
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
            Mob.y = Row2PixelY[Trob.cell / (sbyte)10 + 1] - 15;
            Mob.kind = 2;
            Mob.row = Trob.cell / (sbyte)10;
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
    struct list *pool;
    struct item *bubble;
    int persistent;

    i = animation_code & 7;
    pool = lava_slots[i];
    if (CheckTrobVis()) {
        frame = pool->prefix & 63;
        if (frame != 23 && !((pool->prefix & 128) && frame == 3)) {
            ++pool->prefix;
            if (!(pool->prefix & 128) || frame >= 3) {
                if (pool->prefix & 128) {
                    if (frame == 4) {
                        pool->prefix &= 127;
                        AddSound(97);
                    }
                    frame -= 3;
                }
                for (i = 0; i < 10; ++i) {
                    bubble = &pool->entries[i];
                    if (bubble->active) {
                        ++bubble->state;
                        next = BubbleScript[bubble->type][bubble->state];
                        if (next == -1)
                            bubble->active = 0;
                        else if (next == -2) {
                            bubble->tail = 1;
                            ++bubble->state;
                        }
                    }
                }
                i = 0;
                do {
                    if (lava_bubbles[i].prefix > frame)
                        break;
                    if (lava_bubbles[i].prefix == frame)
                        TriggerLavaBubble(pool, &lava_bubbles[i]);
                    ++i;
                } while (i < 15);
            }
        } else {
            persistent = pool->prefix & 64;
            ResetLavaInfo(pool, 0);
            if (frame == 3) {
                pool->prefix = 0x84;
                if (persistent)
                    pool->prefix = 0xc4;
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
    y = (sbyte)63 * Char.row + 41;
    center = GetBaseX();
    reach = ColEdgeX[Char.column + 5] - center + 26;
    if ((kind == 23 || (kind == 24 && reach >= 0)) &&
        (Frame.flags & 64) && y < Char.y && Char.sequence_group != 5)
        DoLavaBoil();
}

void DoLavaBoil(void)
{
    struct list *pool;
    LavaTrob *trob;

    pool = GetLavaInfoPtr(Char.room, Char.column, Char.row);
    if (pool) {
        pool->prefix = 0x80;
        if (Char.character_kind == 0 && Rnd(3) == 0) {
            JumpSeq(116);
            pool->prefix |= 64; AddSound(99);
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

struct list *GetLavaInfoPtr(sbyte room, sbyte col, sbyte row)
{
    struct list *result;
    LavaTrob *trob;

    trob = GetLavaTrob(room, col, row);
    if (trob)
        result = lava_slots[trob->slot];
    else
        result = 0;
    return result;
}

LavaTrob *GetLavaTrob(sbyte room, sbyte col, sbyte row)
{
    sbyte cell;
    byte kind;
    LavaTrob *trob;

    kind = ReadCell(room, col, row);
    cell = Cell.index;
    if (kind == 24)
        --cell;
    trob = (LavaTrob *)GetTrobRoomCell(room, cell);
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
    struct { sbyte flags; int bounds[4]; } redraw;

    redraw.flags = lava_slots[animation_code]->prefix;
    if (redraw.flags & 128)
        lava_redraw_type = 16;
    else if (redraw.flags & 64)
        lava_redraw_type = 39;
    else
        lava_redraw_type = 48;
    if (ComputeTrobMoveRect((Rect *)redraw.bounds, &lava_redraw_type)) {
        MarkMove(Trob.cell, (Rect *)redraw.bounds);
        MarkMove(CheckTrobRight(), (Rect *)redraw.bounds);
        if (Trob.cell % (sbyte)10 < 8) {
            ++Trob.cell;
            MarkMove(CheckTrobRight(), (Rect *)redraw.bounds);
            --Trob.cell;
        }
    }
}

void ResetLavaInfo(struct list *pool, int type)
{
    memset(pool, 0, sizeof(*pool));
    switch (type) {
    default:
        pool->prefix = 0;
        TriggerLavaBubble(pool, &lava_bubbles[0]);
        TriggerLavaBubble(pool, &lava_bubbles[1]);
        TriggerLavaBubble(pool, &lava_bubbles[2]);
        TriggerLavaBubble(pool, &lava_bubbles[13]);
        pool->entries[3].state = 5;
        TriggerLavaBubble(pool, &lava_bubbles[14]);
        pool->entries[4].state = 4;
        break;
    case 1:
        pool->prefix = 3;
        TriggerLavaBubble(pool, &lava_bubbles[5]);
        TriggerLavaBubble(pool, &lava_bubbles[4]);
        pool->entries[1].state = 1;
        TriggerLavaBubble(pool, &lava_bubbles[3]);
        pool->entries[1].state = 1;
        TriggerLavaBubble(pool, &lava_bubbles[2]);
        pool->entries[3].state = 3;
        TriggerLavaBubble(pool, &lava_bubbles[1]);
        pool->entries[3].state = 3;
        TriggerLavaBubble(pool, &lava_bubbles[0]);
        pool->entries[3].state = 3;
        break;
    case 2:
        pool->prefix = 11;
        TriggerLavaBubble(pool, &lava_bubbles[10]);
        TriggerLavaBubble(pool, &lava_bubbles[9]);
        pool->entries[1].state = 4;
        TriggerLavaBubble(pool, &lava_bubbles[8]);
        pool->entries[2].state = 5;
        break;
    }
}

void TriggerLava(sbyte room, sbyte cell)
{
    byte slot;
    if (!GetTrobRoomCell(room, cell)) {
        slot = GetFreeLavaInfoIdx();
        lava_slots[slot] = _nmalloc(sizeof(struct list));
        ResetLavaInfo(lava_slots[slot], cell % (sbyte)3);
        AddTrob(room, cell, slot, 23);
    }
}

void far pascal TriggerLavaBubble(struct list *destination,
                                      struct input *source)
{
    int index;
    struct item *entry;

    index = 0;
    entry = destination->entries;
    do {
        if (entry->active == 0)
            break;
        ++entry;
        ++index;
    } while (index < 10);

    if (index < 10) {
        destination->entries[index].active = 1;
        destination->entries[index].state = 0;
        destination->entries[index].type = source->value;
        destination->entries[index].data = source->data;
        destination->entries[index].tail = source->tail;
    }
}

int CaveMoveSquishingGate(int sequence)
{
    sbyte cell;

    cell = FirstInRow(Kid.row) + Kid.column;
    if ((sbyte)Kid.room != Trob.room) {
        if (((signed char)Room.left) == Trob.room)
            cell += 10;
        else if (((signed char)Room.right) == Trob.room)
            cell -= 10;
        else
            cell = 30;
    }
    if (cell == Trob.cell || Trob.cell - cell + 1 == 0) {
        if (Kid.sequence_group == 3) {
            sequence = 12;
            AddGateSound(9);
        } else if (Kid.frame == 264) {
            sequence -= GateInc[animation_code] + 2;
        } else if (Kid.frame == 265) {
            sequence = 36;
        } else if (Kid.frame == 266) {
            animation_code = -1;
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
    if (((Level.level_number) == 5 && Room.number == 10) ||
        ((Level.level_number) == 5 && Room.number == 7) ||
        ((Level.level_number) == 5 && Room.number == 12))
        BridgeFrameFunc();
}

void InitBridgeGlobals(void)
{
    SetPalette(3500, 64, 160, 0);
    Bridge.broken = 0;
    memset(Bridge.floors, 0, sizeof(Bridge.floors));
}
