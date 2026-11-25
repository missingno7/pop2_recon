/* BRIDGE.C, DOS overlay 14. Names and packed layouts reflect the observed uses. */
#include "types.h"
#include "level.h"
#include "char.h"
#include "game.h"
#pragma pack(1)

typedef struct Gate {
    int timer;
    unsigned char state;
} Gate;

typedef struct TileAnim {
    unsigned char type;
    signed char col;
    int unknown;
    unsigned char modifier;
} TileAnim;

typedef struct Image {
    char unused;
    int y;
    char other[8];
    char rect[14];
} Image;

typedef struct ImageTable {
    char header[28];
    Image images[1];
} ImageTable;

typedef struct Tile {
    unsigned int modifier;
    unsigned int extra;
} Tile;

extern CHARREC Kid, Char, RoomOpp[5], Opp;

extern LEVELREC Level;
extern FRAMEREC Frame;
static unsigned char floor_mask;
typedef struct {
    int wait, broken;
    unsigned char floors[8];
} BRIDGEREC;
BRIDGEREC Bridge;
extern int Col2PixelX[13], Row2PixelY[5];
Rect BridgeRect = {106, 35, 126, 314};
static signed char bridge_y[] = {0, 1, 2, 3, 3, 2, 1, 0};
static signed char bridge_images[] = {7, 9, 6, 9, 10, 7, 9, 6, 3, 9};
static int floor_y_adjustment[] = {6, 20, 20, 20, 20, 20, 26, 36, 51, 87};
static int floor_x_adjustment[] = {8, 18, 24, 18, 24, 10, 20, 23, 19, 23};
extern Tile near *BlueS;
extern ImageTable near *CustomRoom;
extern int Rnd(int);
int abs(int);
#pragma intrinsic(abs)
extern void AddSound(int), AddSong(int);
extern void LoadKid(void), LoadShad(char), SaveShad(void), GetFrameInfo(void);
extern int AddCharX(int); /* Column-to-X helper at 02cc:0360. */
extern void RemoveChar(void); /* Removes the current character at 02cc:1bd6. */
extern void DoOppStartRun(void); /* Starts the RoomOpp running at overlay-12:2c3e:00d6. */
extern void DoTurnEngarde(void), JumpSeq(int);
extern void TriggerGateWithRevPlate(char, char);
extern Gate near *GetTrobRoomCell(char, int);
extern unsigned char ReadCell(char, char, char);
extern void SetPalette(int, unsigned int, unsigned char, unsigned char);
extern int SndOn(int);

extern void DrawCustomRoomImgAtInfo(char, TileAnim near *);
extern void OFFSETRECT(void far *, void far *, int, int);
extern void ComputeOffsetCellRect(Rect near *, void near *, int, int);
typedef void far pascal DrawCallback(char cell, Rect near *rect);
extern DrawCallback MarkMove, MarkFloor;
extern void MarkBufsForRect(DrawCallback *, Rect near *, char);
extern void AddTrob(char, char, char, char);
extern char far pascal FirstInRow(signed char);
extern signed char far pascal IndexCell1(signed char, signed char);
typedef struct {
    unsigned int width, height;
} ShapeSize;
extern ShapeSize far *GetCustomPieceSize(unsigned char, ShapeSize far *);
extern void MarkMobRedraw(char), ComputeMobRect(Rect near *), AddMobObj(int), AddMob(void);
extern void far _fastcall AddMid(char, int, void near *, int, char);

extern FCHARREC FChar;
extern unsigned int BlueSpec;

extern TROBREC Trob;

extern ROOMREC Room;
extern MOBREC Mob;
extern int MobHeight[13], MobWidth[13];

int BridgeWatchKid(void);
int BridgeWatchOpp(void);
void MarkBrgFloorsForChars(int, int);
void BridgeCheckBreak(int, int);
void TriggerBrgFloorMob(unsigned char, signed char);
int IsCharOnBridge(void);
void TweakCharYOnBridge(void);
int IsBrgGateOpen(void);
int BrgSkelToPlate(void);
void MarkBridgeForCol(char);
void RedrawBrgFloor(int, char);
int far _fastcall OffsetBrgFloorY(unsigned char, char);
void far _fastcall RestoreBrgFloorY(unsigned char, int);

void BridgeFrameFunc(void)
{
    int kid_on_bridge = BridgeWatchKid();
    int opponent_on_bridge = BridgeWatchOpp();
    MarkBrgFloorsForChars(kid_on_bridge, opponent_on_bridge);
    BridgeCheckBreak(kid_on_bridge, opponent_on_bridge);
}

void BridgeCheckBreak(int kid_on_bridge, int opponent_on_bridge)
{
    signed char col;

    if (Bridge.broken == 0) {
        if (kid_on_bridge && opponent_on_bridge) {
            if (abs(Kid.column - RoomOpp[0].column) <= 3) {
                ++Bridge.wait;
                if (Bridge.wait >= 60 && Kid.column >= 4 && Kid.column <= 6 &&
                    RoomOpp[0].column >= 5 && RoomOpp[0].column <= 6 && Kid.sequence != 85)
                    Bridge.broken = 1;
            } else if (Bridge.wait != 0) {
                --Bridge.wait;
            }
        } else if (Bridge.wait > 1) {
            Bridge.wait -= 2;
        }
    } else if (Bridge.broken > 0) {
        if (Bridge.broken == 3) {
            col = 7;
            do {
                if (!Bridge.floors[col - 1]) {
                    TriggerBrgFloorMob(10, col + 10);
                    break;
                }
            } while (--col >= 3);
            if (col == 7) {
                AddSound(68);
                AddSong(66);
            }
            if (col < 3 || Kid.sequence == 85) {
                Bridge.broken = -1;
                LoadKid();
                if (Char.column <= 4 && Char.direction == 0)
                    Char.x = AddCharX(10);
                Kid = Char;
            }
        } else {
            ++Bridge.broken;
        }
    }
}

char far pascal BridgeGetCharCol(char kind, char value)
{
    if (Level.level_number == 5 && kind == 7)
        value -= 10;
    else if (Level.level_number == 5 && kind == 12)
        value += 10;
    return value;
}

int BridgeWatchKid(void)
{
    int on_bridge;

    LoadKid();
    on_bridge = IsCharOnBridge();
    if (on_bridge) {
        TweakCharYOnBridge();
        if (Char.frame == 15 || Char.frame == 185)
            on_bridge = 0;
    } else if (Char.room == 7 && Char.stance != -1 &&
               IsBrgGateOpen() && Char.column <= 5) {
        TriggerGateWithRevPlate(7, 13);
    }
    Kid = Char;
    return on_bridge;
}

int BridgeWatchOpp(void)
{
    int on_bridge;

    if (Level.room_objects[Room.number - 1].opponent_count) {
        LoadShad(0);
        on_bridge = IsCharOnBridge();
        if (on_bridge)
            TweakCharYOnBridge();
        SaveShad();
    } else {
        on_bridge = 0;
    }
    return on_bridge;
}

int BrgSkelMustAdvance(void)
{
    signed char col = BridgeGetCharCol(Char.room, Char.column);
    int advance;

    if (Char.direction == 0) {
        if (col < 2)
            advance = 1;
        else
            advance = 0;
        if (!advance)
            advance = BrgSkelToPlate();
    } else {
        if (col >= 8)
            advance = 1;
        else
            advance = 0;
    }
    return advance;
}

int BrgSkelProg(void)
{
    signed char col = BridgeGetCharCol(Char.room, Char.column);
    int keep_going = 1;

    if (Char.action != 5) {
        if (Char.action == 4 && (Char.row >= 3 || Level.level_number != 5 ||
                                 (Char.room != 10 && Char.room != 7 && Char.room != 12))) {
            RemoveChar();
            keep_going = 0;
        } else if (Char.direction == 0) {
            if (Char.sequence != 102 && Char.sequence != 216) {
                if (Char.x > Kid.x) {
                    if (BrgSkelToPlate()) {
                        DoOppStartRun();
                        keep_going = 0;
                    } else if (col >= 9) {
                        DoTurnEngarde();
                        keep_going = 0;
                    }
                }
            } else {
                if (col >= 9 || !IsBrgGateOpen()) {
                    Char.alert_delay = 2;
                    Char.stance = 1;
                    JumpSeq(101);
                }
                keep_going = 0;
            }
        } else if (BrgSkelToPlate()) {
            Char.direction = 0;
            Char.x = AddCharX(10);
            keep_going = 0;
        }
    }
    return keep_going;
}

int BrgSkelToPlate(void)
{
    signed char col = BridgeGetCharCol(Opp.room, Opp.column);
    int open = IsBrgGateOpen();
    if (open) {
        if (Opp.direction == 0 && col < 2)
            open = 1;
        else if (Opp.direction == -1 && col < 2)
            open = 1;
        else
            open = 0;
    }
    return open;
}

int IsBrgGateOpen(void)
{
    Gate near *gate = GetTrobRoomCell(7, 13);
    int open;

    if (gate && (gate->state == 0 || gate->state == 2))
        open = 1;
    else
        open = 0;
    return open;
}

int IsOKForBrgSkelToAdvance(void)
{
    signed char col = BridgeGetCharCol(Char.room, Char.column);
    int advance;
    if (Char.direction == 0) {
        if (col < 3)
            advance = 1;
        else
            advance = 0;
    } else {
        if (col > 3)
            advance = 1;
        else
            advance = 0;
    }
    return advance;
}

int IsCharOnBridge(void)
{
    if (ReadCell(Char.room, Char.column, Char.row) == 44)
        return 1;
    return 0;
}

void LoadBridgeCode(void)
{
    if (Level.level_number == 5 && (Room.number) == 10)
        SetPalette(3500, 64, 160, 1);
    floor_mask = 0;
    Bridge.wait = 0;
}

void MarkBrgFloorsForChars(int kid_on_bridge, int opponent_on_bridge)
{
    int delay;
    floor_mask = 0;
    delay = 2;
    if (kid_on_bridge && opponent_on_bridge) {
        floor_mask = 255;
        delay = 1;
    } else if (kid_on_bridge) {
        MarkBridgeForCol(Kid.column);
    } else if (opponent_on_bridge) {
        MarkBridgeForCol(RoomOpp[0].column);
    } else {
        delay = 0;
    }
    if (delay && !SndOn(10067) && !Rnd(delay * 40))
        AddSound(67);
}

void MarkBridgeForCol(char col)
{
    int right = col + 1;
    int left;

    right = (right > 8 ? 8 : right) - 1;
    left = col - 1;
    left = left < 1 ? 1 : left;
    col = left - 1;
    if (col <= right) {
        do {
            floor_mask |= 1 << col;
        } while (++col <= right);
    }
}

void TweakCharYOnBridge(void)
{
    int modifier;

    if (IsCharOnBridge() && (Char.frame < 34 || Char.frame > 44)) {
        Char.y = 63 * Char.row + 56;
        Char.y += bridge_y[Char.column - 1];
        GetFrameInfo();
        if (Frame.flags & 64) {
            modifier = Level.cell_info[9][(signed char)(Char.column + 10)].modifier & 15;
            if (modifier == 1)
                --Char.y;
            else if (modifier == 2)
                ++Char.y;
        }
    }
}

void UnloadBridgeCode(void)
{
    SetPalette(3500, 64, 160, 0);
}

void AnimBrgFloor(void)
{
    unsigned char phase, old_phase;

    if (Level.level_number == 5 && (Room.number) == 10) {
        old_phase = BlueSpec & 15;
        BlueSpec &= 0xfff0;
        if (!(floor_mask & (1 << (Trob.cell % (signed char)10 - 1)))) {
            phase = 0;
        } else {
            phase = Rnd(3);
            if (phase > 1) {
                if (old_phase)
                    phase = 0;
                else {
                    --phase;
                    BlueSpec |= phase;
                }
            } else {
                phase = old_phase;
            }
        }
        RedrawBrgFloor(phase, Trob.cell);
    } else {
        Trob.state = -1;
    }
}

void DrawBrgFloor(TileAnim near *tile)
{
    unsigned char image;
    int old_y;

    if (tile->type == 5 || tile->type == 2) {
        image = (tile->modifier & 15) + 3;
        old_y = OffsetBrgFloorY(image, tile->col);
        DrawCustomRoomImgAtInfo(image, tile);
        RestoreBrgFloorY(image, old_y);
    }
}

int far _fastcall OffsetBrgFloorY(unsigned char image, char col)
{
    Image near *entry = &CustomRoom->images[image];
    int old_y = entry->y;
    int delta = bridge_y[col - 1];

    entry->y += delta;
    OFFSETRECT(entry->rect, entry->rect, 0, delta);
    return old_y;
}

void far _fastcall RestoreBrgFloorY(unsigned char image, int old_y)
{
    Image near *entry = &CustomRoom->images[image];
    int delta = entry->y - old_y;

    entry->y = old_y;
    OFFSETRECT(entry->rect, entry->rect, 0, -delta);
}

void RedrawBrgFloor(int phase, char cell)
{
    Rect rect;
    unsigned char image = phase + 3;
    signed char col = cell % (signed char)10;
    int old_y = OffsetBrgFloorY(image, col);

    ComputeOffsetCellRect(&rect, CustomRoom->images[image].rect, col, cell / (signed char)10 + 1);
    --rect.top;
    MarkBufsForRect(&MarkMove, &rect, -1);
    RestoreBrgFloorY(image, old_y);
}

void TriggerBrgFloor(char room, char cell)
{
    Tile near *tiles = BlueS;
    int phase = tiles[cell].modifier & 0xfff0;
    tiles[cell].modifier = phase;

    AddTrob(room, cell, 1, 44);
    RedrawBrgFloor(phase, cell);
}

void ATMBrgFloor(void)
{
    int x = Mob.x, y = Mob.y;
    signed char room = Mob.room;
    signed char col, image;
    int phase;
    int old_x, old_y;
    ShapeSize bounds;
    Rect redraw;

    if (room != (signed char)Room.number) {
        if (Room.below == Mob.room) {
            y += 191;
            room = Room.number;
        } else if (Mob.room == Room.above) {
            y = Mob.y - 191;
            room = Room.number;
        } else if (Mob.room == Room.left) {
            x -= 320;
            room = Room.number;
        } else if ((signed char)Mob.room == (signed char)Room.right) {
            x += 320;
            room = Room.number;
        }
    }
    col = x / 32;
    FChar.index = IndexCell1(col, y / 63);
    if (FChar.index != 30) {
        if (col)
            --FChar.index;
        if (FChar.index < 20)
            FChar.index += 10;
    }
    FirstInRow(Mob.row);
    phase = Mob.flags & 15;
    image = bridge_images[phase];
    if (Level.level_number == 5 && room == 10) {
        old_y = Mob.y;
        Mob.y += floor_y_adjustment[phase];
        y += floor_y_adjustment[phase];
        old_x = Mob.x;
        Mob.x += floor_x_adjustment[phase];
        if (GetCustomPieceSize(image, &bounds)) {
            MobHeight[11] = bounds.width + 1;
            MobWidth[11] = bounds.height + 1;
        }
        MarkMobRedraw(0);
        if (Mob.velocity != -1) {
            ComputeMobRect(&redraw);
            redraw.left += 16;
            MarkBufsForRect(&MarkFloor, &redraw, -1);
            AddMobObj(y);
        }
        Mob.x = old_x;
        Mob.y = old_y;
    }
}

void DrawObjBrgFloor(int unused)
{
    AddMid(4, (unsigned char)bridge_images[FChar.image & 15] + 1,
               (*(void near **)&FChar.x), FChar.y - 1, 10);
}

void MobBrgFloor(void)
{
    int phase, speed;

    if (Level.level_number == 5 && (Room.number) == 10) {
        phase = Mob.flags & 15;
        speed = (Mob.flags & 0x30) >> 4;
        if ((speed == 1 && phase != 4) || (speed == 2 && phase != 9)) {
            Mob.flags &= 0xfff0;
            Mob.flags |= phase + 1;
        } else if (speed == 2 && phase == 9) {
            Mob.velocity = -1;
        }
    } else {
        Mob.velocity = -1;
    }
}

void TriggerBrgFloorMob(unsigned char room, signed char cell)
{
    signed char col, row;
    int speed;

    Level.cell_kinds[9][cell] = 0;
    Level.cell_info[9][cell].modifier = 0xc000;
    Mob.velocity = Mob.auxiliary = 0;
    col = cell % (signed char)10;
    row = cell / (signed char)10;
    Mob.x = Col2PixelX[col + 1];
    if (Rnd(1)) {
        Mob.flags = 5;
        speed = 2;
    } else {
        Mob.flags = 0;
        speed = 1;
    }
    Bridge.floors[col - 1] = speed;
    Mob.flags |= speed << 4;
    Mob.room = room;
    Mob.row = row;
    Mob.y = Row2PixelY[row + 1];
    Mob.y += bridge_y[col - 1];
    Mob.kind = 11;
    AddMob();
    MarkMobRedraw(0);
}
