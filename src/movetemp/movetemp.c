#include "types.h"
#include "level.h"
#include "char.h"
#include "game.h"

#pragma pack(1)
struct moving_rock {
    int x;
    int y;
    unsigned char room;
    char field_5[2];
    int mode;
    char field_9;
    signed char row;
    int screen_x;
};

#pragma pack()

extern int Rnd(int);
extern signed char FirstInRow(signed char);
extern signed char GetCharCol(int);
extern int IsWall(unsigned char);
extern void AddMobObj(int);
extern signed char GetRow(int);
extern unsigned char IndexCell1(signed char, signed char);
extern int IsVisibleRoomCell(unsigned char room, unsigned char cell);
extern void MarkMobRedraw(unsigned char cell);
extern void LoadKid(void);
extern void far _fastcall AddMid(unsigned char kind, int x, int y, int frame, unsigned char pass);
extern int IsAllFloorsInDirToRock(signed char row, int row_delta);
extern void HitCharWithSlideRock(void);
extern struct slide_rock *far _fastcall GetSlideRockOnRoomRow(unsigned char, signed char);

extern FCHARREC FChar;
extern int MobHeight[13], MobWidth[13];
static struct {
    int bolt_y_2, bolt_x_2;
    int bolt_y_3, bolt_x_3;
    int bolt_y_1;
    int trapdoor_x[17];
    int trapdoor_y[17];
    int trapdoor_shape[18];
} temple_draw_offsets = {
    -42, 39, -11, 40, -31,
    { 51, 5, 5, 16, 16, 16, 16, 9, 9, -5, 0, 5, 16, 9, -5, 0, 5 },
    { 0, 7, 7, 12, 12, 12, 12, 7, 7, -2, 0, 7, 12, 7, -1, 0, 7 },
    { 0, 91, 91, 92, 92, 92, 92, 93, 93, 94, 21, 91, 92, 93, 94, 21, 91, 21 }
};
extern CHARREC Kid;
extern unsigned char ColDetLastFrame[];

extern ROOMREC Room;
extern MOBREC Mob;
extern CHARREC Char;
extern LEVELREC Level;
extern int ColEdgeX[20];
extern int CDLeftEdge, CDRightEdge;

int TempGetTorchFrame(int active)
{
    int delay;
    if (active) delay = 0;
    else delay = Rnd(2) + 1;
    return delay;
}

void ATMSlideBolt(void)
{
    signed char row = Mob.row;
    int end = Mob.flags;
    if (end > Mob.x) {
        do {
            FChar.index = FirstInRow(row) + Mob.x / 32;
            if (Mob.kind == 8) ++FChar.index;
            AddMobObj(Mob.y); Mob.x += 24;
        } while (Mob.x < end);
    }
}

void ATMSlideRock(void)
{
    int x, y, sequence, old_speed;
    signed char row;
    if (Mob.velocity != -1) {
        row = GetRow(Mob.y); FChar.index = IndexCell1(Mob.x / 32, row);
        if (IsVisibleRoomCell(Mob.room, FChar.index)) {
            sequence = MobHeight[7];
            if (!IsAllFloorsInDirToRock(-1, 52)) {
                y = Mob.y; Mob.y -= 59; old_speed = MobWidth[6];
                MobWidth[6] = (int)(Mob.flags - Mob.x) <= old_speed ? (int)(Mob.flags - Mob.x) : old_speed;
                MobWidth[7] = (int)(Mob.flags - Mob.x) <= MobWidth[7] ? (int)(Mob.flags - Mob.x) : MobWidth[7];
                Mob.x += 5;
                MobHeight[7] = MobHeight[6] = 9; MarkMobRedraw(2);
                Mob.y = y; Mob.x -= 5; MobWidth[7] = MobWidth[6] = old_speed;
                MobHeight[7] = MobHeight[6] = sequence;
            } else
                MobHeight[7] = MobHeight[6] = 58;
            Mob.kind = 7; AddMobObj(Mob.y);
            x = Mob.x; Mob.x += 32; y = Mob.y;
            if (Mob.flags > Mob.x) {
                FChar.index = FirstInRow(row) + Mob.x / 32; Mob.kind = 6;
                --Mob.y; AddMobObj(Mob.y);
            }
            Mob.x = temple_draw_offsets.trapdoor_x[0] + x;
            if (Mob.x < Mob.flags) {
                Mob.y = temple_draw_offsets.bolt_y_1 + y; Mob.kind = 8; ATMSlideBolt();
            }
            Mob.x = temple_draw_offsets.bolt_x_2 + x;
            if (Mob.x < Mob.flags) {
                Mob.y = temple_draw_offsets.bolt_y_2 + y; Mob.kind = 9;
                ATMSlideBolt();
                Mob.x = temple_draw_offsets.bolt_x_3 + x;
                Mob.y = temple_draw_offsets.bolt_y_3 + y;
                ATMSlideBolt();
            }
            Mob.x = x; Mob.y = y;
            Mob.kind = 6; MobHeight[7] = MobHeight[6] = sequence;
        }
    }
}

int CanSlideRockStop(void)
{
    int result = 0;
    if (IsAllFloorsInDirToRock(-1, 32)) result = IsAllFloorsInDirToRock(0, 32);
    return result;
}

Rect *ComputeSlideRockRect(Rect *rect, unsigned char room, signed char row)
{
    struct moving_rock *rock = (struct moving_rock *)GetSlideRockOnRoomRow(room, row);
    if (rock) {
        rect->top = rock->y - 69; rect->left = rock->x;
        rect->bottom = rock->y + 1; rect->right = rect->left + 52;
    } else
        rect = 0;
    return rect;
}

void CheckSlideRockCrush(void)
{
    int kid_x, rock_x;
    LoadKid();
    kid_x = Char.direction == 0 ? Char.x : Char.x + 13;
    if (Mob.room == Char.room && Mob.row == Char.row) {
        rock_x = Mob.x + 140;
        if (Char.sequence_group == 4 || (rock_x < kid_x && rock_x + 16 > kid_x)) {
            HitCharWithSlideRock(); Kid = Char;
        }
    }
}

void DrawObjSlideRock(unsigned char kind)
{
    struct moving_rock *rock;
    int x, frame, right;
    rock = (struct moving_rock *)GetSlideRockOnRoomRow(Room.number, FChar.y / 63);
    right = rock ? rock->screen_x : 320;
    FChar.clip.right = right; x = right - FChar.x;
    switch (kind) {
    case 134: frame = 79; break;
    case 135: frame = 80; break;
    case 136: frame = x % 5 + 86; break;
    case 137: frame = x % 5 + 81; break;
    default: frame = -1; break;
    }
    if (frame != -1) AddMid(4, frame, FChar.x, FChar.y, 10);
}

int GetSlideRockBarrDist(signed char direction, unsigned char room, signed char row)
{
    struct moving_rock *rock = (struct moving_rock *)GetSlideRockOnRoomRow(room, row);
    int barrier;
    signed char column;
    unsigned char tile;

    if (rock) {
        barrier = rock->x + 140;
        if (direction == -1) {
            int right;
            barrier -= 3;
            column = GetCharCol(barrier - 32);
            tile = (&Level.cell_kinds[0][0])[(unsigned char)30 * rock->room - 30 +
                                    (signed char)(FirstInRow(rock->row) + column)];
            if (IsWall(tile) || tile == 4) {
                right = ColEdgeX[column + 6] + 24;
                if (Char.direction == 0) right += CDRightEdge - CDLeftEdge;
                barrier = right >= barrier ? right : barrier;
            }
        } else
            barrier += 35;
    } else if (direction == -1)
        barrier = ColEdgeX[19];
    else barrier = 0;
    return barrier;
}

void GetSlideRockDetData(signed char row, unsigned char *rooms, unsigned char *flags)
{
    struct moving_rock *rock;
    int left;
    struct {
        unsigned char *neighbor_flags;
        int right;
    } edge;
    signed char column;

    rock = (struct moving_rock *)GetSlideRockOnRoomRow(Char.room, row);
    if (rock) {
        left = rock->x + 137;
        if (left < CDRightEdge) {
            column = GetCharCol(left);
            if (column >= 0 && column < 10) {
                flags[column] |= 0x0f; rooms[column] = rock->room;
            }
            if (rock->mode != 100 && row == Char.row) {
                signed char neighbor = GetCharCol(left + 1);
                if (neighbor != column) {
                    edge.neighbor_flags = &ColDetLastFrame[neighbor];
                    ColDetLastFrame[column] |= *edge.neighbor_flags & 0x0f;
                    if (!(flags[neighbor] & 0x0f)) *edge.neighbor_flags &= 0xf0;
                }
            }
        }
        edge.right = left + 38;
        if (edge.right > CDLeftEdge) {
            column = GetCharCol(edge.right);
            if (column >= 0 && column < 10) {
                flags[column] |= 0xf0; rooms[column] = rock->room;
            }
            if (rock->mode != 100 && row == Char.row) {
                signed char neighbor = GetCharCol(edge.right + 1);
                if (neighbor != column) {
                    edge.neighbor_flags = &ColDetLastFrame[neighbor];
                    ColDetLastFrame[column] |= *edge.neighbor_flags & 0xf0;
                    if (!(flags[neighbor] & 0xf0)) *edge.neighbor_flags &= 0x0f;
                }
            }
        }
    }
}

struct slide_rock {
    char unused[4];
    unsigned char room;
    char middle[5];
    signed char row;
};

extern struct slide_rock *GetMobTypeItem(unsigned char kind, int index);

struct slide_rock *far _fastcall GetSlideRockOnRoomRow(unsigned char room, signed char row)
{
    struct slide_rock *rock;
    int finished = 0;
    int index = 1;

    do {
        rock = GetMobTypeItem(6, index++);
        if (!rock || (rock->room == room && rock->row == row)) finished = 1;
    } while (!finished);
    return rock;
}

extern unsigned char ReadCell(unsigned char, signed char, signed char);
extern int IsSpace(unsigned char);
extern void DecLifePts(unsigned char kind);
extern void AddSound(int);
extern void ResetSnd(int);
extern void SetKidDeathMethod(unsigned char);
extern void ResetCharY(void);
extern void JumpSeq(int);
extern void AnimChar(void);

void HitCharWithSlideRock(void)
{
    int sequence;
    unsigned char tile;
    signed char column;
    int x;
    if (Char.action != 5) {
        sequence = -1;
        if (Char.x - 144 > Mob.x) {
            Char.x = Mob.x + 140;
            if (Char.direction == -1) Char.x -= 13;
        }
        x = Char.x - 8;
        if (Char.direction == 0) x -= 13;
        column = GetCharCol(x);
        tile = ReadCell(Char.room, column, Char.row);
        if (!IsWall(tile) && tile != 4) {
            if (Char.alive < 0) {
                if (Char.stance != 1) sequence = Char.direction == 0 ? 47 : 48;
                else {
                    sequence = 93; Char.stance = 0;
                }
            } else
                Char.x -= 8;
        } else {
            Char.x = ColEdgeX[column + 6] + 8;
            if (Char.direction == 0) Char.x += 17;
            if (tile != 2) {
                if (Mob.auxiliary == 100) {
                    if (Char.alive < 0) {
                        DecLifePts(Char.life_points); Char.alive = 0;
                    }
                    Char.frame = 185;
                } else {
                    int rock_x = Mob.x - 10;
                    column = rock_x / 32;
                    tile = ReadCell(Char.room, column, Char.row);
                    if (!IsWall(tile) && tile != 4) {
                        if (Char.alive < 0) {
                            rock_x -= 5;
                            if (rock_x / 32 != column) {
                                tile = ReadCell(Char.room, rock_x / 32, Char.row);
                                if (IsWall(tile) || tile == 4) AddSound(1);
                            }
                        }
                    } else {
                        if (Char.alive < 0) {
                            ResetSnd(10001); AddSound(55);
                        }
                        SetKidDeathMethod(13);
                    }
                    Char.sequence_group = 4; sequence = 71;
                }
            }
        }
        if (sequence != -1) {
            ResetCharY(); Char.vertical_step = 0;
            JumpSeq(sequence); AnimChar();
        }
    }
}

int IsAllFloorsInDirToRock(signed char row_delta, int step)
{
    int result = 1;
    signed char first = (Mob.x + 6) / 32;
    signed char last, row;
    unsigned char tile;
    step = step < Mob.flags - Mob.x ? step : Mob.flags - Mob.x;
    last = (step + Mob.x) / 32;
    row = Mob.row + row_delta;
    if (last >= first) {
        do {
            if (!result) break;
            tile = ReadCell(Mob.room, first++, row);
            if (IsSpace(tile) || tile == 25) result = 0;
            else result = 1;
        } while (last >= first);
    }
    return result;
}

#pragma pack(1)
struct tile_animation {
    unsigned short state;
    char rest[2];
};
struct trapdoor_bounds {
    int top, left;
};
#pragma pack()

extern CELLREC Cell;
extern struct tile_animation *BlueS;
extern unsigned char *BlueT;
extern int SndOn(int id);
extern int CueSnd;
extern void DoImpale(void);
extern void Drop(void);
extern int SlideRockInCell(int margin);
extern int Pics[5];
extern int trapdoor_top, trapdoor_left;
extern void AddMob(void);
extern void AddSong(int song);
extern void AltarWatchKid(void);
extern unsigned long GetSetImgSize(int sheet, int shape, struct trapdoor_bounds far *bounds);
extern void ComputeMobRect(Rect *rect);
extern void MarkFloor(signed char cell, Rect *rect);
extern void MarkBufsForRect(void (far pascal *update)(signed char, Rect *),
                                    Rect *rect, signed char flags);
extern void TriggerTrapDoor(unsigned char room, signed char cell);

void MobSlideRock(void)
{
    int x, prior_column;
    signed char new_column;
    signed char prev_column;
    unsigned char tile;
    signed char column;

    if (Mob.auxiliary == 0 || Mob.auxiliary == 100) return;
    x = Mob.x;
    if (Mob.auxiliary == -1) --x;
    else if (CanSlideRockStop() && Mob.auxiliary >= 8) {
        Mob.auxiliary = 0; ResetSnd(10054);
        if (CueSnd == 54) CueSnd = -1;
    } else {
        Mob.x -= 8;
        if (CanSlideRockStop()) AddSound(53);
        Mob.x = x--;
        if (Mob.auxiliary <= 8) ++Mob.auxiliary;
    }
    if (x < 0) {
        Mob.room = Level.room_links[Mob.room - 1].left;
        x += 320;
    }
    do {
        if (!Mob.room) {
            Mob.velocity = -1;
            ResetSnd(10054);
            return;
        }
        if (Mob.auxiliary == 0 || Mob.auxiliary == 100) return;
        prior_column = x - 20;
        column = prior_column / 32;
        if (prior_column <= 0) --column;
        tile = ReadCell(Mob.room, column, Mob.row);
        if (Mob.velocity == 1) {
            Mob.velocity = 0;
            Mob.auxiliary = 100;
            ++Mob.y; Mob.x = x + 2;
            if (tile != 2) break;
            LoadKid(); DoImpale();
        } else {
            prev_column = (Mob.x - 4) / 32;
            if ((new_column = (x - 4) / 32) != prev_column || prev_column != column) {
                column = ReadCell(Mob.room, new_column, Mob.row);
                if ((IsWall(column) || column == 4 || tile == 2) && Mob.velocity == 0) {
                    Mob.velocity = 1; --Mob.y;
                    if (!SndOn(10055) && tile != 2) AddSound(8);
                }
            }
            LoadKid();
            if (Char.frame >= 87 && Char.frame <= 99) {
                column = (x - 8) / 32;
                if (Mob.row - Char.row + 1 == 0 && column == Char.column) Drop();
            }
        }
        Kid = Char;
    } while (0);
    if (Mob.auxiliary != 100) {
        Mob.x = x;
        if (Mob.auxiliary != 0) AddSound(54);
        return;
    }
    ResetSnd(10054);
}

extern unsigned char view_left, view_right, view_up, view_down;

int SlideRockNearSpikes(void)
{
    CELLREC saved = Cell;
    int result;
    ReadCell(Cell.room, Cell.column + 1, Cell.row);
    result = SlideRockInCell(16);
    if (!result) {
        ReadCell(Cell.room, Cell.column + 2, Cell.row);
        result = SlideRockInCell(16);
    }
    Cell = saved;
    return result;
}

int SlideRockInCell(int margin)
{
    int result = 0;
    int rock, left, right;
    rock = GetSlideRockBarrDist(0, Cell.room, Cell.row);
    if (rock) {
        left = Cell.column * 32;
        right = left + 32;
        rock -= 140;
        if (left - margin + 32 > rock || right - margin + 32 < rock) {
            rock = GetSlideRockBarrDist(-1, Cell.room, Cell.row);
            if (left - margin <= (rock -= 140) && right - margin >= rock)
                result = 1;
        } else
            result = 1;
    }
    return result;
}

int far _fastcall TempGetSlideRockX(unsigned char room, signed char row)
{
    int x = GetSlideRockBarrDist(0, room, row);
    if (x && Char.x < x)
        x = GetSlideRockBarrDist(-1, room, row);
    return x;
}

extern int Col2PixelX[13];
extern int Row2PixelY[5];

extern FRAMEREC Frame;

void TriggerSlideRock(unsigned char room, signed char cell, unsigned char action)
{
    struct { signed char row; signed char column; } location;
    struct moving_rock *rock;
    location.row = cell / (signed char)10;
    rock = (struct moving_rock *)GetSlideRockOnRoomRow(room, location.row);
    if (rock) {
        if (action == 1) {
            if (rock->mode >= 0 && rock->mode != 100)
                rock->mode = -1;
        } else if (action == 2 && rock->mode == -1)
            rock->mode = 1;
    } else if (action == 1) {
        int x;
        location.column = cell % (signed char)10;
        ReadCell(room, location.column, location.row);
        Mob.auxiliary = -1;
        Mob.velocity = 0;
        x = Col2PixelX[Cell.column + 1];
        if (x > 320 && (room == Room.left)) {
            room = Room.number;
            x -= 320;
        }
        Mob.flags = Mob.x = x;
        Mob.room = room;
        Mob.y = Row2PixelY[Cell.row + 1] - 5;
        Mob.row = location.row;
        Mob.kind = 6;
        AddMob();
    }
}

void ATMTrapDoor(void)
{
    int x = Mob.x, y = Mob.y;
    unsigned char room = Mob.room;
    signed char column;
    signed char absolute_column;
    int step, saved_x, saved_y;
    struct trapdoor_bounds bounds;
    struct {
        signed char saved_column;
        Rect rect;
    } draw;

    if (room != Room.number) {
        if (Mob.room == Room.below) {
            y += 191;
            room = Room.number;
        } else if (Mob.room == Room.above) {
            y -= 191;
            room = Room.number;
        } else if (Mob.room == Room.left) {
            x -= 320;
            room = Room.number;
        } else if (Mob.room == Room.right) {
            x += 320;
            room = Room.number;
        }
    }
    draw.saved_column = column = x / 32;
    FChar.index = IndexCell1(draw.saved_column, y / 63);
    if (FChar.index != 30) {
        if (column)
            --FChar.index;
        if (FChar.index < 20)
            FChar.index += 10;
    }
    absolute_column = FirstInRow(Mob.row) + Mob.x / 32;
    step = Mob.flags & 31;
    if (step > 16) {
        Mob.velocity = -1;
        Level.cell_info[Mob.room - 1][absolute_column].modifier &= 0xff00;
        (&Level.cell_kinds[0][0])[(unsigned char)30 * *(volatile unsigned char *)&Mob.room - 30 + absolute_column] = 26;
        step = 17;
        ResetSnd(10095);
        if (CueSnd == 95)
            CueSnd = -1;
        AddSound(3);
    }
    if (IsVisibleRoomCell(room, FirstInRow(Mob.row) + draw.saved_column)) {
        saved_y = Mob.y;
        Mob.y += temple_draw_offsets.trapdoor_y[step];
        y += temple_draw_offsets.trapdoor_y[step];
        saved_x = Mob.x;
        x += temple_draw_offsets.trapdoor_x[step];
        Mob.x = x;
        if (GetSetImgSize(Pics[4], temple_draw_offsets.trapdoor_shape[step] - 1, &bounds)) {
            int left_edge;
            MobHeight[10] = bounds.top + 1;
            left_edge = column * 32 - Mob.x + 57;
            ++bounds.left;
            if (left_edge > bounds.left)
                bounds.left = left_edge;
            MobWidth[10] = bounds.left;
        }
        MarkMobRedraw(0);
        if (Mob.velocity != -1) {
            ComputeMobRect(&draw.rect);
            draw.rect.left += 16;
            MarkBufsForRect(MarkFloor, &draw.rect, -1);
            AddMobObj(y);
        }
        Mob.x = saved_x;
        Mob.y = saved_y;
    }
}

void MobTrapDoor(void)
{
    if (Mob.velocity != -1) {
        if ((signed char)(Mob.flags & 31) <= 16)
            ++Mob.flags;
        if (!SndOn(10094) && !SndOn(10095))
            AddSound(95);
    }
}

void CheckTrapDoor(void)
{
    if ((Frame.flags & 0x40) &&
        (signed char)63 * Char.row - Char.y == -56 &&
        Char.character_kind != 1 &&
        ReadCell(Char.room, Char.column, Char.row) == 26 &&
        !(Cell.modifier & 31))
        TriggerTrapDoor(Cell.room, Cell.index);
}

void ShakeTrapDoor(void)
{
    if (!(Cell.modifier & 0x40)) {
        if (Cell.index % (signed char)10 != Char.column) {
            struct tile_animation *animation = BlueS;
            Cell.modifier |= 0x40;
            animation[Cell.index].state = Cell.modifier;
        }
        TriggerTrapDoor(Cell.room, Cell.index);
    }
}

void TriggerTrapDoor(unsigned char room, signed char cell)
{
    Level.cell_info[room - 1][cell].modifier = 0x81;
    BlueT[cell] = 0;
    Mob.velocity = Mob.auxiliary = 0;
    Mob.x = Col2PixelX[cell % (signed char)10 + 1];
    Mob.flags = (Cell.modifier & 0x40) ? 75 : 1;
    Mob.room = room;
    Mob.y = Row2PixelY[Cell.row + 1];
    Mob.row = cell / (signed char)10;
    Mob.kind = 10;
    AddMob();
    AddSound(94);
    MarkMobRedraw(0);
}

void DrawObjTrapDoor(unsigned char modifier)
{
    int shape = temple_draw_offsets.trapdoor_shape[FChar.image & 31];
    if (shape == 21) {
        AddMid(4, shape, FChar.x + 1, FChar.y - 2, 10);
        AddMid(4, 22, FChar.x, FChar.y - 1, 0);
    } else
        AddMid(4, shape, FChar.x, FChar.y - 1, 10);
}

void LoadInscriptionRoomFunc(void)
{
    if (Kid.column >= 9)
        AddSong(92);
}

void check_temple_event(void)
{
    if (Room.number == 4 && (Level.level_number) == 13)
        AltarWatchKid();
}

void TempFrameFunc(void)
{
}
