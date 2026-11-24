/* MOVESCYT.C, DOS scythe movement and collision. */
#include "types.h"
#include "level.h"
#include "char.h"
#include "game.h"

typedef struct {
    word modifier;
    word extra;
} RoomCell;

extern word BlueSpec;

extern TROBREC Trob;
extern CHARREC Char;

extern CELLREC Cell;
extern LEVELREC Level;
extern RoomCell near *BlueS;
extern int ColEdgeX[20];
extern int LeftEdge;

static Rect redraw_bounds = {26, -24, 43, 52};
static Rect first_blade = {26, 20, 33, 57};
static Rect second_blade = {26, 46, 33, 89};
int ScytheImg[20] = {
    0, 100, 97, 98, 97, 98, 97, 98, 97, 97,
    97, 97, 97, 97, 97, 97, 99, 100, 101, 0
};

extern void far pascal AddSound(int sound);
extern char far pascal ReadCell(byte room, sbyte column, sbyte row);
extern int far pascal SectRect(Rect far *out, const Rect far *left, const Rect far *right);
extern void far pascal JumpSeq(int seq);
extern void far pascal SetKidDeathMethod(sbyte method);
extern void far pascal AnimChar(void);
extern int far pascal decrement_life_points(sbyte amount);
extern Rect far *far pascal UnionRect(Rect far *out, const Rect far *left, const Rect far *right);
extern Rect far *far pascal OffsetRect(Rect far *out, const Rect far *in, int x, int y);
extern int far pascal check_trob_visibility(void);
extern int far pascal compute_trob_move_rect(Rect near *out, const Rect near *in);
extern void far pascal mark_move(sbyte cell, const Rect near *rect);
extern void far pascal mark_bufs_for_rect(void (far pascal *mark)(sbyte, const Rect near *),
                                             const Rect near *rect, sbyte cell);
extern void far pascal add_trob(byte room, sbyte index, sbyte mode, byte kind);

int far pascal GetScytheHitType();
int far pascal ScytheBladeExtended();
int far pascal ComputeBladeHitRect(Rect near *bounds);
void far pascal RedrawScythe(int bloody);
void far pascal TriggerScythe(byte room, sbyte index);

void far pascal AnimScythe(void)
{
    int frame = BlueSpec & 31;
    if (frame < 19) {
        ++BlueSpec;
        if (ScytheImg[frame] != ScytheImg[frame + 1])
            RedrawScythe(0);
        if (frame == 0)
            AddSound(84);
        else if (frame == 15)
            AddSound(83);
        else if (frame >= 16 && (BlueSpec & 0x40) && !(BlueSpec & 0x80)) {
            BlueSpec |= 0x80;
            RedrawScythe(1);
        }
    } else {
        BlueSpec &= ~31;
        Trob.state = -1;
    }
    ReadCell(Trob.room, Trob.cell % 10 - 1, Trob.cell / 10);
    *(byte near *)&Cell.modifier = 0;
    Level.cell_info[Cell.room - 1][Cell.index].modifier =
        (Cell.modifier |= (byte)BlueSpec);
}

void far pascal CheckScythe(void)
{
    if (Char.character_kind != 7 && Char.character_kind != 8 && Char.character_kind != 11 &&
        Char.alive < 0 && Char.row * 63 + 40 <= Char.y) {
        char kind = ReadCell(Char.room, Char.column, Char.row);
        if ((kind == 12 || kind == 13) && ColEdgeX[Char.column + 5] + 18 < Char.x)
            TriggerScythe(Cell.room, Cell.index);
    }
}

void far pascal CheckScytheHit(void)
{
    int done = 0;
    sbyte column, kind;
    Rect bounds;
    if (Char.character_kind != 0 && Char.character_kind != 2)
        return;
    if (Char.sequence == 44)
        return;
    column = Char.column - 1;
    while (column <= Char.column + 1) {
        kind = ReadCell(Char.room, column, Char.row);
        if (kind == 12 || kind == 13) {
            if (GetScytheHitType(Cell.modifier) &&
                ComputeBladeHitRect(&bounds) &&
                SectRect(&bounds, &bounds, (const Rect far *)&Char.bounds) &&
                (Char.frame < 267 || Char.frame > 271)) {
                if (Char.character_kind == 0) {
                    JumpSeq(113);
                    SetKidDeathMethod(5);
                } else {
                    JumpSeq(190);
                    Char.alive = 1;
                }
                AnimChar();
                decrement_life_points(100);
                AddSound(85);
                BlueS[Cell.index].modifier = (Cell.modifier |= 0x40);
                if (kind == 12)
                    ReadCell(Char.room, column + 1, Char.row);
                else
                    ReadCell(Char.room, column - 1, Char.row);
                BlueS[Cell.index].modifier = (Cell.modifier |= 0x40);
            }
            done = 1;
        }
        ++column;
        if (done)
            break;
    }
}

int far pascal ComputeBladeHitRect(Rect near *bounds)
{
    int valid = 1;
    int phase = Cell.modifier & 31;
    if (phase == 2)
        UnionRect(bounds, &first_blade, &second_blade);
    else if (phase == 16)
        *bounds = first_blade;
    else if (phase == 17)
        *bounds = second_blade;
    else
        valid = 0;
    if (valid)
        OffsetRect(bounds, bounds, (Cell.column + (Cell.kind == 13 ? 1 : 0)) << 5,
                   63 * Cell.row);
    return valid;
}

int far pascal GetScytheHitType(unsigned char value)
{
    unsigned int low5 = value & 31U;

    if (low5 == 2U || (low5 >= 16U && low5 <= 17U)) {
        low5 = 1U;
    } else {
        low5 = 0U;
    }

    return low5;
}

void far pascal RedrawScythe(int bloody)
{
    Rect bounds;
    if (check_trob_visibility()) {
        bounds = redraw_bounds;
        if (bloody) {
            bounds.right += 6;
            --bounds.top;
        }
        if (compute_trob_move_rect(&bounds, &bounds))
            mark_bufs_for_rect(mark_move, &bounds, -1);
    } else {
        Trob.state = 1;
    }
}

int far pascal ScytheBarrToStandUp(void)
{
    int blocked = 0;
    char kind = ReadCell(Char.room, Char.column, Char.row);
    if (kind == 12 && ScytheBladeExtended(Cell.modifier)) {
        blocked = 1;
    } else if (kind == 13) {
        ReadCell(Char.room, Char.column - 1, Char.row);
        if (ScytheBladeExtended(Cell.modifier) &&
            ColEdgeX[Char.column + 5] + 9 >= LeftEdge)
            blocked = 1;
    }
    return blocked;
}

int far pascal ScytheBladeExtended(unsigned char value)
{
    unsigned int low5 = value & 31U;
    return low5 >= 3U && low5 <= 15U;
}
void far pascal TriggerScythe(byte room, sbyte index)
{
    if (Cell.kind == 12) {
        ReadCell(room, Cell.column + 1, Cell.row);
        room = Cell.room;
        index = Cell.index;
    }
    if (!(Level.cell_info[room - 1][index].modifier & 31))
        add_trob(room, index, 1, 13);
}
