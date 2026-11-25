#include "types.h"
#include "level.h"
#include "char.h"
#include "game.h"

MOBREC Mob;
int TrobCt;
TROBREC Trob;
TROBREC Trobs[20];
int MobI;

static sbyte exit_close_speed[] = {5, 17, 99, 0};
static sbyte gate_close_speed[] = {0, 0, 0, 20, 40, 60, 80, 100, 120, 0};
sbyte GateInc[] = {-1, -4, 4, 4};

extern LEVELREC Level;
typedef struct {
    unsigned state;
    unsigned extra;
} RoomObject;

RoomObject *BlueS;
extern RoomObject BlueSpec;
byte *BlueT;
extern byte ObjId;

#define TrobStateOffset ((unsigned)&((TROBREC *)0)->state)

void AnimTrob(int index);

void AddMobs(void);
void AnimMobs(void);
void AnimTrobs(void);
int ChangeLinkTimer(int link, byte timer);

void AnimTrobs(void)
{
    int i;
    /* This work word holds the removal flag until compaction starts. */
    union {
        int removed;
        TROBREC *next;
    } pass;
    struct {
        int kept;
    } compact;

    if (TrobCt) {
        pass.removed = 0;
        for (i = 0; i < TrobCt; ++i) {
            Trob = Trobs[i];
            AnimTrob(i);
            if ((Trobs[i].state = Trob.state) < 0)
                pass.removed = 1;
        }
        if (pass.removed) {
            compact.kept = i = 0;
            if (i < TrobCt) {
                sbyte *state = &Trobs[0].state;
                pass.next = Trobs;
                do {
                    if (*state >= 0) {
                        TROBREC *dst = pass.next;
                        ++*(TROBREC * volatile *)&pass.next;
                        ++compact.kept;
                        *dst = *(TROBREC *)(state - TrobStateOffset);
                    }
                    state += sizeof(TROBREC);
                } while (++i < *(volatile int *)&TrobCt);
            }
            TrobCt = compact.kept;
        }
    }
}

extern void select_room_arrays(byte room);
void GetOS(int);
void AnimFlask(void);
void AnimLoose(void);
void AnimExit(void);
void AnimTorch(int);
void AnimGate(void);
void AnimPlate(void);
extern void AnimDsrtExit(void);
extern void AnimScythe(void);
extern void AnimLava(void);
extern void AnimCaveBars(void);
extern void AnimDsrtRocks(void);
extern void AnimDsrtPlanks(void);
extern void AnimDsrtStone(void);
extern void AnimFinlJaffarBlink(void);
extern void AnimDart(void);
extern void AnimRoofPillar(void);
extern void AnimRoofShip(void);
extern void AnimRoofGlass(void);
extern void AnimFinlSpider(void);
extern void AnimFinlCrystal(void);
extern void AnimFinlGargoyle(void);
extern void AnimSacredFlame(void);
extern void AnimBrgFloor(void);

/* Draft. External provider names and same-file prototypes remain provisional. */
void AnimTrob(int index)
{
    select_room_arrays(Trob.room);
    GetOS(Trob.cell);
    switch (Trob.kind) {
    default:
        Trob.state = -1;
        break;
    case 4:
        if (Level.environment_kind == 1)
            AnimDsrtExit();
        else
            AnimGate();
        break;
    case 5:
    case 6:
    case 34:
        AnimPlate();
        break;
    case 10:
        AnimFlask();
        break;
    case 11:
        AnimLoose();
        break;
    case 13:
        AnimScythe();
        break;
    case 17:
        AnimExit();
        break;
    case 19:
    case 32:
        AnimTorch(index);
        break;
    case 23:
        AnimLava();
        break;
    case 27:
        AnimCaveBars();
        break;
    case 28:
        AnimDsrtRocks();
        break;
    case 29:
        AnimDsrtPlanks();
        break;
    case 30:
        AnimDsrtStone();
        break;
    case 31:
        AnimFinlJaffarBlink();
        break;
    case 36:
        AnimDart();
        break;
    case 37:
        AnimRoofPillar();
        break;
    case 38:
        AnimRoofShip();
        break;
    case 39:
        AnimRoofGlass();
        break;
    case 40:
        AnimFinlSpider();
        break;
    case 41:
        AnimFinlCrystal();
        break;
    case 42:
        AnimFinlGargoyle();
        break;
    case 43:
        AnimSacredFlame();
        break;
    case 44:
        AnimBrgFloor();
        break;
    }
    BlueS[Trob.cell].state = BlueSpec.state;
}

typedef void (far *CellMark)(sbyte cell, Rect *rect);

static Rect temple_gate_rect = {-11, 17, 63, 57};
static Rect gate_rect = {-11, 32, 63, 57};
static Rect exit_rect_templates[] = {
    {1, 6, 51, 46}, {-5, 6, 51, 49}, {-5, 6, 51, 50}
};
static Rect flask_rect = {30, 18, 52, 31};
static Rect loose_rect_templates[] = {
    {52, 0, 72, 50}, {48, 0, 69, 57}
};
static Rect plate_rect = {49, 0, 67, 57};
Rect CaveTorchRect = {3, 32, 31, 51};
Rect TempTorchRect = {-4, 25, 18, 57};
static Rect temple_plate_rect = {51, 11, 63, 43};
static Rect clip_char_rect_templates[] = {
    {9, -32, 192, 46}, {6, -32, 192, 48}, {6, -32, 192, 50}
};

static Rect *loose_rects[5] = {
    0, 0, &loose_rect_templates[1], &loose_rect_templates[0], &loose_rect_templates[1]
};
static Rect *exit_rects[5] = {
    0, 0, &exit_rect_templates[2], &exit_rect_templates[0], &exit_rect_templates[1]
};
/* ClipChar in CTRL.C selects its template by level type. */
Rect *LevelExitClipRect[5] = {
    0, 0, &clip_char_rect_templates[2], &clip_char_rect_templates[0], &clip_char_rect_templates[1]
};

extern int IsSpace(byte);
extern sbyte CellToCol(byte room, int cell);
extern ROOMREC Room;
extern sbyte CellToRow(byte room, int cell);
extern Rect *ComputeOffsetCellRect(Rect *out, Rect *base, int col, int row);
Rect *ComputeTrobMoveRect(Rect *out, Rect *base);
sbyte CheckTrob(void);
sbyte CheckTrobLeft(void);
sbyte CheckTrobRight(void);
sbyte CheckTrobAbove(void);
sbyte CheckTrobAboveRight(void);
sbyte CheckTrobBelow(void);
void MarkMove(sbyte cell, Rect *rect);
void MarkBufsForRect(CellMark, Rect *, sbyte);
int IsVisibleRoomCell(byte, sbyte);

void RedrawTorch(void)
{
    struct { Rect rect; union { int cave; struct { byte pad; sbyte cell; } bytes; } tail; } work;
    work.tail.cave = (Level.environment_kind == 2);

    work.rect = *(work.tail.cave ? &TempTorchRect : &CaveTorchRect);
    if (ComputeTrobMoveRect(&work.rect, &work.rect)) {
        if (work.tail.cave) {
            MarkMove(Trob.cell, &work.rect);
            work.tail.bytes.cell = CheckTrobAbove(); MarkMove(work.tail.bytes.cell, &work.rect);
            MarkMove(CheckTrobAboveRight(), &work.rect);
        }
        MarkMove(CheckTrobRight(), &work.rect);
    }
}

void RedrawExit(void)
{
    struct { Rect rect; byte pad; sbyte cell; } work;

    if (ComputeTrobMoveRect(&work.rect, exit_rects[Level.environment_kind])) {
        MarkMove(Trob.cell, &work.rect);
        work.cell = CheckTrobRight(); MarkMove(work.cell, &work.rect);
        work.cell = CheckTrobAbove(); MarkMove(work.cell, &work.rect);
        MarkMove(CheckTrobAboveRight(), &work.rect);
    }
}

void RedrawFlask(void)
{
    Rect rect;

    if (ComputeTrobMoveRect(&rect, &flask_rect))
        MarkMove(CheckTrob(), &rect);
}

void RedrawPlate(void)
{
    Rect rect;
    Rect *source;

    if (Level.environment_kind == 3)
        source = &temple_plate_rect;
    else
        source = &plate_rect;
    if (ComputeTrobMoveRect(&rect, source))
        MarkBufsForRect(MarkMove, &rect, -1);
}

void RedrawLoose(void)
{
    struct { Rect rect; byte pad; sbyte cell; } work;

    if (ComputeTrobMoveRect(&work.rect, loose_rects[Level.environment_kind])) {
        MarkMove(work.cell = CheckTrob(), &work.rect);
        MarkMove(CheckTrobRight(), &work.rect);
        if (Level.environment_kind == 3)
            MarkMove(CheckTrobBelow(), &work.rect);
    }
}

void RedrawGate(void)
{
    struct { Rect rect; byte pad; sbyte cell; } work;

    if (Level.environment_kind == 3)
        work.rect = temple_gate_rect;
    else
        work.rect = gate_rect;
    work.cell = CheckTrobAboveRight();
    if (work.cell != 30 && work.cell >= 0 && !IsSpace((&Level.cell_kinds[0][0])[30 * Room.number - 30 + work.cell]))
        work.rect.top = 0;
    if (ComputeTrobMoveRect(&work.rect, &work.rect)) {
        MarkMove(Trob.cell, &work.rect);
        work.cell = CheckTrobRight(); MarkMove(work.cell, &work.rect);
        MarkMove(CheckTrobAboveRight(), &work.rect);
    }
}

void RedrawTrob(Rect *base)
{
    struct { Rect rect; byte pad; sbyte cell; } work;

    if (base && ComputeTrobMoveRect(&work.rect, base)) {
        MarkMove(work.cell = CheckTrob(), &work.rect);
        MarkMove(CheckTrobRight(), &work.rect);
    }
}

Rect *ComputeTrobMoveRect(Rect *out, Rect *base)
{
    Rect *result = out;
    sbyte column = CellToCol(Trob.room, Trob.cell);
    out = result;
    return ComputeOffsetCellRect(out, base, column,
                                 CellToRow(Trob.room, Trob.cell));
}

sbyte CheckTrob(void)
{
    sbyte cell = Trob.cell;

    if (Room.above == Trob.room) {
        if (cell >= 20 && cell < 30)
            cell = 19 - cell;
        else
            cell = 30;
    } else if (Room.number != Trob.room)
        cell = 30;
    return cell;
}

int CheckTrobVis(void)
{
    int visible = IsVisibleRoomCell(Trob.room, Trob.cell);
    if (!visible)
        Trob.state = -1;
    return visible;
}

sbyte CheckTrobLeft(void)
{
    sbyte cell = Trob.cell;

    if (Room.number == Trob.room) {
        if (cell % 10 == 0)
            cell = 30;
        else
            --cell;
    } else if (Room.right == Trob.room && cell % 10 == 0)
        cell += 9;
    else
        cell = 30;
    return cell;
}

sbyte CheckTrobRight(void)
{
    sbyte cell = Trob.cell;

    if (Room.number == Trob.room) {
        if (cell % 10 == 9)
            cell = 30;
        else
            ++cell;
    } else if (Room.left == Trob.room) {
        if (cell % 10 == 9)
            cell -= 9;
        else
            cell = 30;
    } else if (Room.above == Trob.room) {
        if (cell >= 20 && cell < 29)
            cell = 18 - cell;
        else
            cell = 30;
    } else if (Room.above_left == Trob.room)
        cell = cell == 29 ? -1 : 30;
    else
        cell = 30;
    return cell;
}

sbyte CheckTrobAbove(void)
{
    sbyte cell = Trob.cell;

    if (Room.number == Trob.room) {
        if (cell < 10)
            cell = -1 - cell;
        else
            cell -= 10;
    } else if (Room.below == Trob.room && cell < 10)
        cell += 20;
    else
        cell = 30;
    return cell;
}

sbyte CheckTrobAboveRight(void)
{
    sbyte cell = Trob.cell;

    if (Room.number == Trob.room) {
        if (cell % 10 == 9)
            cell = 30;
        else if (cell < 10)
            cell = -2 - cell;
        else
            cell -= 9;
    } else if (Room.left == Trob.room) {
        if (cell == 9)
            cell = -1;
        else if (cell % 10 == 9)
            cell -= 19;
        else
            cell = 30;
    } else if (Room.below == Trob.room) {
        if (cell < 9)
            cell += 21;
        else
            cell = 30;
    } else if (Room.below_left == Trob.room)
        cell = cell == 9 ? 20 : 30;
    else
        cell = 30;
    return cell;
}

sbyte CheckTrobBelow(void)
{
    sbyte cell = Trob.cell;

    if (Room.number == Trob.room) {
        if (cell < 20)
            cell += 10;
        else
            cell = 30;
    } else if (Room.above == Trob.room && cell >= 20)
        cell %= 10;
    else
        cell = 30;
    return cell;
}

extern int WeightlessTimer, AlertGuard;
extern void OnGround(void);
extern void FallOn(void);
extern void Falling(void);
extern sbyte GetAbove(void);
extern sbyte GetUnderFt(void);
extern void BreakTotalLoose(void);
extern byte CutRoom;
extern CHARREC Char;
void BreakLoose(sbyte);
void JamPlate(void);
void PushPlate(sbyte, int);

void CheckFloor(void)
{
    unsigned frame;

    if (Char.character_kind == 7 || Char.character_kind == 8 || Char.action == 9) {
        if (Char.vertical_step)
            Falling();
        return;
    }

    frame = Char.frame;
    switch (Char.action) {
    default:
        OnGround();
        break;
    case 2:
    case 6:
        break;
    case 3:
        if (frame >= 0x66 && frame < 0x6a)
            FallOn();
        break;
    case 4:
        Falling();
        break;
    case 5:
        if (frame == 0x6d)
            OnGround();
        break;
    }
}

extern FRAMEREC Frame;

void CheckPress(void)
{
    int action;
    unsigned frame;
    sbyte kind;

    if (Char.character_kind == 11 || Char.character_kind == 10 || (Char.character_kind == 0 && WeightlessTimer)
            || (Char.character_kind == 1 && Char.sequence >= 29 && Char.sequence <= 42))
        return;

    action = Char.action;
    frame = Char.frame;
    if ((frame >= 0x57 && frame <= 0x63) || (frame >= 0x87 && frame < 0x8d))
        kind = GetAbove();
    else if (action < 2 || action == 7 || action == 5 || action == 8) {
        kind = GetAbove();
        if (frame == 0x4f && kind == 11)
            BreakLoose(0);
        else if (frame == 0x4f && kind == 15)
            BreakTotalLoose();
        else
            if (!(Frame.flags & 0x40))
                kind = 0;
            else
                kind = GetUnderFt();
    } else
        kind = 0;

    if (kind == 5 || kind == 6 || kind == 34) {
        if (Char.alive >= 0 && Char.character_kind != 4) {
            JamPlate();
            return;
        }
        PushPlate(0, -1);
    } else if (kind == 11 || kind == 15) {
        AlertGuard = 1;
        if (kind == 11) {
            BreakLoose(0);
            return;
        }
        if (kind == 15)
            BreakTotalLoose();
    }
}

extern int FrameNum;

extern GAMESTATE GameState;
extern CHARREC Kid;
int GetTorchFrame(int);

void AnimFlask(void)
{
    if (ObjId == 10) {
        if (Trob.state < 0 || !CheckTrobVis())
            return;
        {
            unsigned state = BlueSpec.state & ~31U;
            int frame = BlueSpec.state & 31;
            if (frame >= 27)
                frame = 0;
            else
                ++frame;
            BlueSpec.state = state | frame;
            RedrawFlask();
        }
        return;
    }
    Trob.state = -1;
}

void AnimTorch(int index)
{
    if (CheckTrobVis()) {
        int frame = GetTorchFrame((byte)BlueSpec.state);
        BlueSpec.state = (BlueSpec.state & 0xff00U) + frame;
        if (!GameState.fade_active || ((FrameNum + index) & 1))
            RedrawTorch();
    }
}

extern struct {
    signed char argument, entrance_mode, sword_kind, death_countdown;
    int field_4, shadow_timer;
} KidInfo;
extern struct { unsigned char channels, volume; } Midi;

extern void AddSound(int), ResetSnd(int);
void StopGate(int);
void CheckSquishedFingers(void);
int DidKidEscapeOpp(void);
void AddGateSound(int);
extern int CaveMoveSquishingGate(int);

void AnimGate(void)
{
    unsigned modifier = BlueSpec.state;
    int height = modifier & 255;
    sbyte phase = Trob.state;
    int sound;

    if (phase >= 0) {
        if (phase >= 4) {
            if (phase < 8)
                Trob.state = ++phase;
            height -= gate_close_speed[phase];
            if (height <= 0) {
                sbyte col, row;
                if (Level.environment_kind == 3) {
                    row = Trob.cell / 10;
                    col = Trob.cell % 10;
                    if (Trob.room != Kid.room) {
                        if (Trob.room == Room.left)
                            col -= 10;
                        else if (Trob.room == Room.right)
                            col += 10;
                    }
                }
                if (Level.environment_kind == 3 && Kid.sequence == 118 && row == Kid.row
                        && (col == Kid.column || Kid.column - col == 1)) {
                    height = 36;
                    sound = 9;
                } else {
                    height = 0;
                    sound = Level.environment_kind != 3 ? 15 : 9;
                    CheckSquishedFingers();
                    if (DidKidEscapeOpp())
                        KidInfo.death_countdown = 4;
                }
                AddGateSound(sound);
                Trob.state = -1;
            }
        } else {
            if (height == 255)
                StopGate(255);
            else {
                height += GateInc[phase];
                if (phase == 0 || phase == 1) {
                    if (height <= 0) {
                        height = 0;
                        StopGate(height);
                        CheckSquishedFingers();
                    } else if (height < 200) {
                        AddGateSound(Level.environment_kind != 3 ? 4 : 7);
                        Trob.state = 1;
                        if (Level.environment_kind == 3)
                            height = CaveMoveSquishingGate(height);
                    } else if (Level.level_number == 5 && Trob.room == 7)
                        height -= GateInc[phase];
                } else if (height < 200) {
                    if (Level.environment_kind != 3) {
                        if (height % 8 == 0)
                            AddGateSound(4);
                    } else
                        AddGateSound(7);
                } else if (phase < 3) {
                    RedrawGate();
                    height = 250;
                    Trob.state = 0;
                    AddGateSound(Level.environment_kind != 3 ? 5 : 8);
                } else {
                    height = 255;
                    StopGate(height);
                }
            }
        }
        BlueSpec.state = (BlueSpec.state & 0xff00U) + height;
    }
    if (height <= 200)
        RedrawGate();
}

extern void LoadKid(void);
extern void JumpSeq(int);
extern sbyte FirstInRow(sbyte);

void StopGate(int height)
{
    int actor_cell;

    Trob.state = -1;
    AddGateSound(Level.environment_kind != 3 ? 6 : 8);
    LoadKid();
    actor_cell = FirstInRow(Char.row - 1) + Char.column;
    if (Trob.cell == actor_cell && Char.direction == -1
            && Char.action == 2 && height == 0) {
        JumpSeq(47);
        Kid = Char;
    }
}

void CheckSquishedFingers(void)
{
    if (Trob.room == Kid.room && (Kid.action == 2 || Kid.action == 6)) {
        sbyte actor_cell = FirstInRow(Kid.row) + Kid.column;
        if (Trob.cell - actor_cell == -10 || Trob.cell - actor_cell == -11)
            AddSound(31);
    }
}

extern CHARREC RoomOpp[5];
#pragma intrinsic(abs)

int DidKidEscapeOpp(void)
{
    int escaped = 0;
    sbyte count = Level.room_objects[Trob.room - 1].opponent_count;
    sbyte row = Trob.cell / 10;
    sbyte col = Trob.cell % 10;
    sbyte i;
    if (count && Kid.room == Trob.room && Kid.row == row
            && abs(col - Kid.column) < 5) {
        for (i = 0; i < count; ++i) {
            if (RoomOpp[i].row == row && RoomOpp[i].alive < 0 && RoomOpp[i].room == Kid.room) {
                if ((Kid.column > col && RoomOpp[i].column > col)
                        || (Kid.column <= col && RoomOpp[i].column <= col)) {
                    escaped = 0;
                    break;
                }
                escaped = 1;
            }
        }
    }
    return escaped;
}

void AnimExit(void)
{
    int height = BlueSpec.state & 255;

    if (Trob.state >= 0) {
        if (Trob.state >= 4) {
            ++Trob.state;
            height -= exit_close_speed[Trob.state - 4];
            if (height < 0) {
                height = 0;
                Trob.state = -1;
                AddSound(9);
            } else if (Trob.state == 4 && (Midi.channels & 1))
                AddSound(7);
        } else {
            ++height;
            if (height >= 42) {
                Trob.state = -1;
                if (Room.number != 4 || Level.level_number != 13) {
                    ResetSnd(0);
                    AddSound(8);
                }
            } else if (Room.number != 4 || Level.level_number != 13)
                AddSound(7);
        }
    }
    BlueSpec.state = (BlueSpec.state & 0xff00U) + height;
    RedrawExit();
}

extern int CaveGetTorchFrame(int frame), TempGetTorchFrame(int frame);
extern int EMPTYRECT(Rect far *rect);
extern sbyte IndexCell(sbyte column, sbyte row);
extern CELLREC Cell;

int IsExitOpen(void)
{
    if ((byte)Cell.modifier >= 42)
        return 1;
    return 0;
}

int GetTorchFrame(int frame)
{
    int result;
    if (Level.environment_kind == 3)
        result = CaveGetTorchFrame(frame);
    else
        result = TempGetTorchFrame(frame);
    return result;
}

typedef struct {
    int dirty;
    Rect rect;
} MarkedRect;

MarkedRect TopBuf[10];
MarkedRect MoveBuf[30];
MarkedRect FloorBuf[30];
MarkedRect FRedrawBuf[30];
int HalfBuf[30];

extern void ComputeClipRect(Rect far *out, int column, int row);
extern int SECTRECT(Rect far *out, Rect far *a, Rect far *b);
extern void UNIONRECT(Rect far *out, Rect far *a, Rect far *b);

void MarkRectBuf(MarkedRect *mark, int cell, Rect *clip);
void MarkOppUpdateForRect(Rect *clip, sbyte except);

void MarkTop(int cell, Rect *clip)
{
    Rect rect;

    if (cell <= -1 && cell >= -10) {
        ComputeClipRect(&rect, -((cell + 1) % 10), -1);
        if (!clip || SECTRECT(&rect, &rect, clip)) {
            ++cell;
            cell = -cell;
            if (TopBuf[cell].dirty)
                UNIONRECT(&TopBuf[cell].rect, &TopBuf[cell].rect, &rect);
            else {
                TopBuf[cell].dirty = 1;
                TopBuf[cell].rect = rect;
            }
        }
    }
}

void MarkMove(sbyte cell, Rect *clip)
{
    if (cell < 30) {
        if (cell >= 0)
            MarkRectBuf(&MoveBuf[cell], cell, clip);
        else
            MarkTop(cell, clip);
        MarkOppUpdateForRect(clip, -1);
    }
}

void MarkFloor(sbyte cell, Rect *clip)
{
    if (cell < 30) {
        if (cell >= 0) {
            MarkRectBuf(&FloorBuf[cell], cell, clip);
            return;
        }
        MarkTop(cell, clip);
    }
}

void MarkHalf(sbyte cell)
{
    if (cell < 30) {
        if (cell >= 0) {
            HalfBuf[cell] = 1;
            return;
        }
        MarkTop(cell, 0);
    }
}

void MarkRedraw(int cell)
{
    extern unsigned int RedrawBuf[];

    if (cell < 30) {
        if (cell >= 0)
            *(int *)((byte *)RedrawBuf + (cell << 1)) = 1;
        else
            MarkTop(cell, 0);
    }
}

void MarkCellRedraw(void)
{
    sbyte cell = Cell.index;

    if (Room.number == Cell.room)
        ;
    else if (Room.left == Cell.room)
        cell -= 9;
    else if (Room.right == Cell.room)
        cell += 9;
    else if (Room.above == Cell.room)
        cell -= 20;
    else if (Room.below == Cell.room)
        cell += 20;
    MarkRedraw(cell);
}

void MarkFRedraw(sbyte cell, Rect *clip)
{
    if (cell < 30) {
        if (cell >= 0) {
            MarkRectBuf(&FRedrawBuf[cell], cell, clip);
            return;
        }
        MarkTop(cell, clip);
    }
}

void MarkBufsForRect(CellMark mark, Rect *rect, sbyte except)
{
    struct {
        int top, left, bottom, right;
    } bounds;
    sbyte row, column;

    if (!EMPTYRECT(rect)) {
        if (rect->top >= 3)
            bounds.top = (rect->top - 3) / 63;
        else
            bounds.top = -1;
        if (rect->bottom >= 3)
            bounds.bottom = (rect->bottom - 3) / 63;
        else
            bounds.bottom = -1;
        bounds.left = rect->left / 32;
        bounds.right = rect->right / 32;
        for (row = bounds.top; row <= bounds.bottom; ++row) {
            for (column = bounds.left; column <= bounds.right; ++column)
                mark(IndexCell(column, row), rect);
        }
        MarkOppUpdateForRect(rect, except);
    }
}

void MarkOppUpdateForRect(Rect *clip, sbyte except)
{
    Rect hit;
    sbyte count = Level.room_objects[Room.number - 1].opponent_count;
    int i;

    for (i = 0; i < count; ++i) {
        if (RoomOpp[i].slot != except)
            RoomOpp[i].previous_mode |= SECTRECT(&hit, &RoomOpp[i].bounds, clip);
    }
}

void MarkRectBuf(MarkedRect *mark, int cell, Rect *clip)
{
    Rect rect;

    ComputeClipRect(&rect, cell % 10, cell / 10);
    if (SECTRECT(&rect, &rect, clip)) {
        if (mark->dirty)
            UNIONRECT(&mark->rect, &mark->rect, &rect);
        else {
            mark->dirty = 1;
            mark->rect = rect;
        }
    }
}

int GetLinkNext(int link);
int GetLinkCell(int link);
byte GetLinkRoom(int link);
TROBREC *GetTrobRoomCell(byte room, int cell);
void AddTrob(byte room, sbyte cell, sbyte state, byte kind);
sbyte TriggerObject(byte kind, byte room, sbyte cell, int action);
extern unsigned Rnd(unsigned limit);

void Trigger(int link, int action)
{
    byte room, kind;
    sbyte cell, state;

    if (GetLinkRoom(link) != -2 && link != 0xfd) {
        do {
            room = GetLinkRoom(link);
            select_room_arrays(room);
            cell = GetLinkCell(link);
            kind = BlueT[cell];
            state = TriggerObject(kind, room, cell, action);
            if (state >= 0)
                AddTrob(room, cell, state, kind);
            link = GetLinkNext(link);
        } while (link != 0xfd);
    }
}

void TriggerFlask(byte room, sbyte cell)
{
    unsigned state = (BlueS[cell].state & ~31U) | (Rnd(26) + 1);
    BlueS[cell].state = state;
    AddTrob(room, cell, 1, 10);
}

extern sbyte TriggerGateWithRevPlate(byte room, sbyte cell);
extern int IsDeadPos(unsigned int frame);

sbyte TriggerGate(byte room, sbyte cell, int action)
{
    sbyte state = -1;
    unsigned value = BlueS[cell].state;
    unsigned height = value & 255;
    unsigned modifier = value;
    unsigned flags = modifier & 0xff00U;

    switch (action) {
    case 5:
        if (height == 255)
            break;
        if (height >= 200) {
            BlueS[cell].state = flags + 250;
            if (Char.alive >= 0 || (CutRoom && CutRoom != Char.room)) {
                BlueS[cell].state = flags + 255;
                state = 3;
            }
        } else {
            BlueS[cell].state = flags + ((height + 3) & 0xfc);
            if (Char.alive >= 0 || (CutRoom && CutRoom != Char.room))
                state = 3;
            else
                state = 2;
        }
        break;
    case 6: {
        TROBREC *trob = GetTrobRoomCell(room, cell);
        if (IsDeadPos(Char.frame) && Char.alive >= 2)
            break;
        if ((!trob && height) || (trob && trob->state != 4))
            state = 4;
        break;
    }
    case 14:
        if (height < 200)
            state = 3;
        else
            BlueS[cell].state = flags + 255;
        break;
    case 34:
        state = TriggerGateWithRevPlate(room, cell);
        break;
    }
    return state;
}

extern int SndOn(int id);
extern void AddSong(int sound);
extern void TriggerSlideRock(byte room, sbyte cell, sbyte state);
extern sbyte TriggerCaveBars(byte room, sbyte cell);
extern void TriggerDart(byte, sbyte);

sbyte TriggerObject(byte kind, byte room, sbyte cell, int action)
{
    sbyte state;

    switch (kind) {
    case 4:
        state = TriggerGate(room, cell, action);
        break;
    case 17:
        if (room != Level.entry_room && Level.level_number == 8 && KidInfo.sword_kind != 1) {
            state = -1;
        } else {
            int height = BlueS[cell].state & 255;
            state = height ? -1 : 1;
            if (state == 1 && !SndOn(10027) && Level.level_number != 13)
                AddSong(27);
        }
        break;
    case 25:
        if (action == 5)
            state = 2;
        else
            state = 1;
        TriggerSlideRock(room, cell, state);
        state = -1;
        break;
    case 27:
        state = TriggerCaveBars(room, cell);
        break;
    case 36:
        if (!(Cell.modifier & 0x0800U))
            TriggerDart(room, cell);
        state = -1;
        break;
    default:
        state = -1;
        break;
    }
    return state;
}

void TriggerTorch(byte room, sbyte cell)
{
    unsigned state = (BlueS[cell].state & 0xff00U) + Rnd(Level.environment_kind == 3 ? 8 : 3);
    BlueS[cell].state = state;
    AddTrob(room, cell, 1, 19);
}

void AddTrob(byte room, sbyte cell, sbyte state, byte kind)
{
    TROBREC *found = GetTrobRoomCell(room, cell);

    if (!found) {
        if (TrobCt < 20) {
            Trob.room = room;
            Trob.cell = cell;
            Trob.state = state;
            Trob.kind = kind;
            Trobs[TrobCt] = Trob;
            ++TrobCt;
        }
    } else {
        found->state = state;
    }
}

void * _cdecl memset(void *, int, unsigned int);
#pragma intrinsic(memset)

unsigned int RedrawBuf[30];
unsigned char ObjBuf[30];

void ZeroBuffers(void)
{
    memset(RedrawBuf, 0, sizeof(RedrawBuf));
    memset(MoveBuf, 0, sizeof(MoveBuf));
    memset(FRedrawBuf, 0, sizeof(FRedrawBuf));
    memset(FloorBuf, 0, sizeof(FloorBuf));
    memset(HalfBuf, 0, sizeof(HalfBuf));
    memset(ObjBuf, 0, sizeof(ObjBuf));
    memset(TopBuf, 0, sizeof(TopBuf));
}

extern byte ReadCell(byte room, sbyte column, sbyte row);
extern int IsWall(byte tile);

sbyte Get1stWallColInDir(byte room, sbyte column, sbyte row, sbyte step)
{
    do {
        column += step;
        if (IsWall(ReadCell(room, column, row)))
            break;
    } while (column >= 0 && column <= 10);
    return column;
}

extern int far _fastcall TempGetSlideRockX(unsigned char room, signed char row);

int far _fastcall GetSlideRockX(signed char room, signed char row)
{
    int x;

    if (Level.environment_kind == 2)
        x = TempGetSlideRockX(room, row);
    else
        x = 0;
    return x;
}

int GetLinkNext(int link)
{
    return Level.links[link].next;
}

int GetLinkTimer(int link)
{
    return Level.links[link].timer;
}

int ChangeLinkTimer(int link, byte timer)
{
    return Level.links[link].timer = timer;
}

int GetLinkCell(int link)
{
    return Level.links[link].cell;
}

int LinkBefore(int link)
{
    if (Level.links[link].prev != 0xfd)
        return 1;
    return 0;
}

int LinkMore(int link)
{
    if (Level.links[link].next != 0xfd)
        return 1;
    return 0;
}

byte GetLinkRoom(int link)
{
    return Level.links[link].room;
}

void PushPlate(sbyte kind, int link)
{
    sbyte previous_timer;
    sbyte cell;
    byte room;
    unsigned state;

    if (!kind)
        kind = Cell.kind;
    if (link == -1)
        link = Cell.modifier & 255;
    previous_timer = GetLinkTimer(link);
    cell = Cell.index;
    state = Cell.modifier | 0x0800U;
    room = Cell.room;
    if (previous_timer != 31 && (ChangeLinkTimer(link, 5), previous_timer < 2)) {
        AddTrob(Cell.room, Cell.index, 1, kind);
        if (Char.frame != 185 && (Cell.room != 4 || Level.level_number != 13))
            AddSound(3);
        RedrawPlate();
        Trigger(link, (byte)kind);
        AlertGuard = 1;
    } else {
        Trigger(link, (byte)kind);
    }
    Level.cell_info[room - 1][cell].modifier = state;
}

void JamPlate(void)
{
    int link = Cell.modifier & 255;

    ChangeLinkTimer(link, 31);
    PushPlate(Cell.kind, link);
}

void AnimPlate(void)
{
    if (Trob.state >= 0) {
        int link = BlueSpec.state & 255;
        sbyte timer = GetLinkTimer(link);
        if (timer < 31) {
            ChangeLinkTimer(link, timer - 1);
            if (timer < 2) {
                Trob.state = -1;
                RedrawPlate();
                BlueSpec.state &= ~0x0800U;
                return;
            }
        } else
            Trob.state = -1;
    }
}

void CloseExit(byte room, sbyte cell)
{
    if (Level.environment_kind != 1) {
        RoomObject *objects = BlueS;
        objects[cell].state = (objects[cell].state & 0xff00U) + 42;
        AddTrob(room, cell, 4, 17);
    }
}

void MakeSpace(byte room, sbyte cell);
void AddMob(void);
void PassThru(void);
void MarkMobRedraw(sbyte layer);
extern sbyte IndexCell1(sbyte column, sbyte row);
static MOBREC mobs[30];
int MobCt;
extern sbyte GetRow(int value);

void AnimLoose(void)
{
    int frame;
    int kind;
    sbyte row;

    ++BlueSpec.state;
    frame = BlueSpec.state & 15;
    if (Trob.state != -1) {
        if (BlueSpec.state & 0x40) {
            if (frame >= 4) {
                Trob.state = -1;
                BlueSpec.state = (BlueSpec.state & 0xff00U) | (BlueSpec.state & 0xb0U);
            }
        } else if (frame >= 12) {
            kind = Level.environment_kind == 3 ? (BlueSpec.state & 0x80) >> 7 : 3;
            MakeSpace(Trob.room, Trob.cell);
            BlueSpec.state = kind + 3;
            Mob.x = (Trob.cell % 10) * 32;
            row = Trob.cell / 10;
            Mob.y = row * 63 + 66;
            Mob.room = Trob.room;
            Mob.auxiliary = 0;
            Mob.velocity = Trob.state;
            Mob.kind = kind;
            Mob.row = row;
            Mob.flags = 0;
            AddMob();
            Trob.state = -1;
        }
    }
    RedrawLoose();
}

void MakeSpace(unsigned char room, signed char cell)
{
    int index = cell;
    unsigned state;

    BlueT[index] = 0;
    if (Level.level_number == 5 && room == 3)
        state = 0xc000;
    else
        state = (BlueS[*(volatile int *)&index].state & 0x80) + 3;
    BlueS[*(volatile int *)&index].state = state;
}

extern void ShakeTrapDoor(void);
extern void ShakeBones(byte room, sbyte row);

void ShakeLooseFloor(void);

void BreakLoose(sbyte sound)
{
    if (Char.character_kind == 1 && sound == 0) {
        ShakeLooseFloor();
    } else {
        unsigned state = Cell.modifier & ~0x40U;
        if (sound)
            state = (state & ~4U) | 11;
        BlueS[Cell.index].state = state;
        AddTrob(Cell.room, Cell.index, sound, 11);
        AddSound(20);
    }
}

void ShakeLooseFloor(void)
{
    if (!(Cell.modifier & 15)) {
        int counter = ((Cell.modifier & 0x30) >> 4) + 1;

        if ((Cell.column != Char.column && counter != 3) || Char.character_kind == 1) {
            int index;
            RoomObject *objects;

            Cell.modifier &= ~0x30U;
            index = Cell.index;
            objects = BlueS;
            Cell.modifier |= (counter | 4) << 4;
            objects[index].state = Cell.modifier | 0x40;
        }
        AddTrob(Cell.room, Cell.index, 1, 11);
        AddSound(20);
    }
}

void ShakeEm(byte room, sbyte row)
{
    MOBREC saved = Mob;
    sbyte column = 0;

    do {
        switch (ReadCell(room, column, row)) {
        case 11:
            ShakeLooseFloor();
            break;
        case 15:
            BreakTotalLoose();
            break;
        case 26:
            ShakeTrapDoor();
            break;
        }
    } while (++column < 10);

    if (Level.preferred_opponent == 2)
        ShakeBones(room, row);
    Mob = saved;
}

void AddMob(void)
{
    if (MobCt < 30)
        mobs[MobCt++] = Mob;
}

extern void MobDart(void);
extern void MobSpikes(void);
extern void MobSlideRock(void);
extern void CheckSlideRockCrush(void);
extern void MobTrapDoor(void);
extern void MobFireball(void);
void AnimMob(void);
void MobLoose(void);
void CheckLooseCrush(void);
void MakeRubble(void);
void KnockLoose(void);

MOBREC *GetMobTypeItem(byte kind, int occurrence)
{
    MOBREC *found = 0;
    int index, count;

    for (count = index = 0; index < MobCt && found == 0; ++index) {
        if (mobs[index].kind == kind && mobs[index].velocity != -1) {
            if (++count == occurrence)
                found = &mobs[index];
        }
    }
    return found;
}

void GetOS(int cell)
{
    BlueSpec = BlueS[cell];
    ObjId = BlueT[cell];
}

int MobHeight[13] = {18, 18, 2, 16, 27, 39, 69, 69, 12, 12, 37, 37, 18};
int MobWidth[13] = {50, 50, 4, 57, 27, 31, 52, 52, 24, 24, 69, 57, 17};

void AnimMobs(void)
{
    struct {
        MOBREC *destination;
        int kept, index;

    } work;

    {
        int count = MobCt;
        for (MobI = 0; MobI < count; ++MobI) {
            Mob = mobs[MobI];
            AnimMob();
            mobs[MobI] = Mob;
        }
    }

    work.index = work.kept = 0;
    if (work.index < MobCt) {
        int *speed = &mobs[0].velocity;

        work.destination = mobs;
        do {
            if (*speed != -1) {
                MOBREC *next = work.destination++;
                ++work.kept;
                *next = *(MOBREC *)((byte *)speed - (unsigned)&((MOBREC *)0)->velocity);
            }
            speed = (int *)((byte *)speed + sizeof(MOBREC));
        } while (++work.index < MobCt);
    }
    MobCt = work.kept;
}

extern void MobBrgFloor(void);

void AnimMob(void)
{
    switch (Mob.kind) {
    case 0:
    case 1:
    case 3:
        MobLoose();
        if (Mob.velocity <= 0)
            ++Mob.velocity;
        CheckLooseCrush();
        break;
    case 2:
        MobDart();
        return;
    case 4:
        MobSpikes();
        return;
    case 6:
        MobSlideRock();
        CheckSlideRockCrush();
        return;
    case 10:
        MobTrapDoor();
        return;
    case 11:
        MobBrgFloor();
        return;
    case 12:
        MobFireball();
        return;
    }
}

void MobLoose(void)
{
    byte kind;

    if (Mob.velocity >= 0) {
        if (Mob.velocity < 29)
            Mob.velocity += 3;
        Mob.y += Mob.velocity;

        if (!Mob.room) {
            if (MobHeight[Mob.kind] + 192 <= Mob.y) {
                Mob.velocity = -2;
                return;
            }
        } else if ((Mob.row + 1) * 63 <= Mob.y) {
            kind = ReadCell(Mob.room, Mob.x / 32, Mob.row);
            if (kind == 11)
                KnockLoose();
            else if (kind == 15)
                BreakTotalLoose();

            if (kind != 11 && kind != 15 && !IsSpace(kind)) {
                AddSound(2);
                ShakeEm(Mob.room, Mob.row);
                Mob.y = (Mob.row + 1) * 63;
                Mob.velocity = -2;
                mobs[MobI].flags = Mob.flags = 1;
                MakeRubble();
                return;
            }
            PassThru();
        }
    }
}

void MakeRubble(void)
{
    struct {
        int index;
        sbyte row, column;
        sbyte plate;
        byte kind;
        sbyte cell;
        byte room;
    } work;

    work.plate = 0;
    work.kind = ReadCell(Mob.room, Mob.x / 32, Mob.row);
    work.room = Cell.room;
    work.column = Cell.column;
    work.row = Cell.row;
    work.cell = Cell.index;

    switch (work.kind) {
    case 5:
        work.plate = (&Level.cell_kinds[0][0])[30 * work.room - 30 + work.cell] = 14;
    case 6:
    case 34:
        PushPlate(work.plate, -1);
        if (work.kind == 34)
            Level.cell_info[work.room - 1][work.cell].modifier = Cell.modifier | 0x1800;
        ReadCell(Mob.room, Mob.x / 32, Mob.row);
        Cell.modifier &= 0xff00U;
        if (work.kind == 5)
            Cell.modifier |= 4;
        else if (work.kind == 6)
            Cell.modifier |= 8;
        work.kind = Cell.kind;
    case 1:
    case 2:
    case 10:
    case 36:
        if (work.kind == 10) {
            TROBREC *flask = GetTrobRoomCell(Cell.room, Cell.index);
            if (flask) {
                Trob = *flask;
                RedrawFlask();
            }
        }
    case 19:
    case 32:
        work.kind = ReadCell(work.room, work.column, work.row);
        if (work.kind == 19 || work.kind == 32) {
            (&Level.cell_kinds[0][0])[30 * work.room - 30 + work.cell] = 32;
        } else {
            if (work.kind == 10 || work.kind == 1)
                Cell.modifier = 0;
            work.index = work.room * 30 + work.cell;
            (&(&Level.cell_kinds[0][0])[30 * 0 - 30 + 0])[work.index] = 14;
            if (Level.environment_kind == 4)
                (&Level.cell_info[0][0])[work.index - 30].modifier = Cell.modifier | 0x40;
            else if ((Level.environment_kind == 3 || Level.environment_kind == 2) && work.kind != 1)
                (&Level.cell_info[0][0])[work.index - 30].modifier = 0;
        }
        break;
    case 12:
    case 13:
        Level.cell_info[work.room - 1][work.cell].modifier = Cell.modifier | 0x20;
        break;
    default:
        return;
    }
    MarkMobRedraw(0);
}

void KnockLoose(void)
{
    MakeSpace(Cell.room, Cell.index);
    Mob.velocity >>= 1;
    mobs[MobI] = Mob;
    Mob.y = Mob.row * 63 + 66;
    MarkMobRedraw(0);
    Mob.y = mobs[MobI].y + 6;
    PassThru();
    AddMob();
    Mob = mobs[MobI];
    MarkMobRedraw(0);
}

void PassThru(void)
{
    ++Mob.row;
    if (Mob.row >= 3) {
        Mob.y -= 192;
        Mob.row = 0;
        Mob.room = Level.room_links[Mob.room - 1].below;
    }
}

#pragma pack(1)
typedef struct {
    signed char cell;
    int x;
    unsigned char fields_before_action[12];
    unsigned char action;
} OpponentPosition;
#pragma pack()

void ATM(void);
void ATMLoose(void);
void ComputeMobRect(Rect *rect);
void AddMobObj(int y);
int OppInfoUnderMob(OpponentPosition *opp);
int CharUnderMob(void);

extern void ATMBrgFloor(void);
extern void ATMDart(void);
extern void ATMSpikes(void);
extern void ATMSlideRock(void);
extern void ATMTrapDoor(void);
extern void ATMFireball(void);
extern Rect NullRect;
extern void OFFSETRECT(Rect far *, Rect far *, int, int);
extern FCHARREC FChar;

int IsLoosePushingPlate(void)
{
    byte kind = Level.environment_kind == 3 ? 1 : 3;
    int occurrence = 0;
    int pushing = 0;
    MOBREC *mob;

    do {
        mob = GetMobTypeItem(kind, ++occurrence);
        if (mob && mob->room == Cell.room && mob->row == Cell.row
                && mob->x / 32 == Cell.column && mob->flags)
            pushing = 1;
    } while (mob && !pushing);
    return pushing;
}

void AddMobs(void)
{
    int index = 0;
    MOBREC *entry;

    if (index < MobCt) {
        entry = mobs;
        do {
            Mob = *entry;
            ATM();
            *entry = Mob;
            ++entry;
        } while (++index < MobCt);
    }
}

void ATM(void)
{
    switch (Mob.kind) {
    case 0:
    case 1:
    case 3:
        ATMLoose();
        break;
    case 2:
        ATMDart();
        return;
    case 4:
        ATMSpikes();
        return;
    case 6:
        ATMSlideRock();
        return;
    case 10:
        ATMTrapDoor();
        return;
    case 11:
        ATMBrgFloor();
        return;
    case 12:
        ATMFireball();
        return;
    }
}

void ATMLoose(void)
{
    int visible = 1;
    int y = Mob.y;
    sbyte column, row, cell;
    Rect rect;
    int x;
    int height = MobHeight[Mob.kind];

    if (Room.number == Mob.room) {
        if (height + 191 <= Mob.y)
            visible = 0;
    } else if (Room.below == Mob.room) {
        if (height > y)
            y = Mob.y + 191;
        else
            visible = 0;
    } else if (Room.above == Mob.room) {
        if (191 - height <= Mob.y)
            y = Mob.y - 191;
        else
            visible = 0;
    } else if (Room.left == Mob.room && MobWidth[Mob.kind] + (x = Mob.x) > 320) {
        Mob.x = x - 320;
        if (Room.left == Mob.room)
            Mob.room = Room.number;
        else if (Room.above_left == Mob.room)
            Mob.room = Room.above;
    } else {
        visible = 0;
    }

    if (visible) {
        FChar.index = IndexCell1(column = Mob.x / 32, row = GetRow(y));
        MarkMobRedraw(1);
        ComputeMobRect(&rect);
        AddMobObj(y);
        ++column;
        cell = IndexCell(column, row);
        if (Room.below != Mob.room)
            MarkFloor(cell, &rect);
        cell = GetRow(y - height);
        if (cell != row)
            MarkFloor(IndexCell(column, cell), &rect);
    }
}

void ComputeMobRect(Rect *rect)
{
    int x, y;

    rect->top = Mob.y - MobHeight[Mob.kind];
    rect->left = Mob.x;
    rect->bottom = Mob.y + 1;
    rect->right = rect->left + MobWidth[Mob.kind];
    if (Room.number != Mob.room) {
        if (Room.left == Mob.room || Room.above_left == Mob.room
                || Room.below_left == Mob.room || Room.right == Mob.room
                || Room.above_right == Mob.room || Room.below_right == Mob.room)
            x = -320;
        else
            x = 0;
        if (Room.above == Mob.room || Room.above_left == Mob.room
                || Room.above_right == Mob.room)
            y = -192;
        else if (Room.below == Mob.room || Room.below_left == Mob.room
                || Room.below_right == Mob.room)
            y = 192;
        else
            y = 0;
        if (x || y)
            OFFSETRECT(rect, rect, x, y);
        else
            *rect = NullRect;
    }
}

void MarkMobRedraw(sbyte layer)
{
    Rect rect;
    CellMark mark;

    ComputeMobRect(&rect);
    if (layer == 1)
        mark = MarkFRedraw;
    else if (layer == 2)
        mark = MarkFloor;
    else
        mark = MarkMove;
    MarkBufsForRect(mark, &rect, -1);
}

#pragma pack(1)
typedef struct {
    int x, y;
    byte bank;
    int flags;
    int scratch;
    byte reserved;
    byte image;
    Rect rect;
    byte layer;
    int sequence;
    sbyte order;
} MobDraw;
#pragma pack()

extern int Ct[5];
extern MobDraw ObjTable[];
extern void SetObjCell(sbyte index);

void AddMobObj(int y)
{
    int index = Ct[4]++;
    int zero;

    ObjTable[index].image = Mob.kind | 0x80;
    ObjTable[index].x = Mob.x;
    ObjTable[index].y = y;
    ObjTable[index].bank = 4;
    ObjTable[index].flags = Mob.flags;
    ComputeMobRect(&ObjTable[index].rect);
    zero = 0;
    ObjTable[index].sequence = zero;
    ObjTable[index].scratch = zero;
    ObjTable[index].order = -1;
    SetObjCell(index);
}

extern void CrushChar(void);
extern void LoadShad(sbyte index);
extern void SaveShad(void);
extern OpponentPosition *GetOppInfoIdx(int room, int index);
extern void CrushOppInfo(OpponentPosition *, byte);
extern signed char Plus1[2];
extern signed char Minus1[2];
extern int GetDist(void);

void CheckLooseCrush(void)
{
    sbyte index;
    sbyte count;

    if (!Mob.room)
        return;
    LoadKid();
    if (CharUnderMob()) {
        CrushChar();
        Kid = Char;
    }

    index = 0;
    count = Level.room_objects[Mob.room - 1].opponent_count;
    for (; index < count; ++index) {
        if (Mob.room == Kid.room) {
            LoadShad(index);
            if (CharUnderMob()) {
                CrushChar();
                SaveShad();
            }
        } else {
            OpponentPosition *guard = GetOppInfoIdx(Mob.room, index);
            if (OppInfoUnderMob(guard))
                CrushOppInfo(guard, Mob.room);
        }
    }
}

int CharUnderMob(void)
{
    int hit = 0;
    int y;
    sbyte column;

    if (Char.character_kind == 11)
        y = Char.y;
    else if (Char.character_kind == 7 || Char.character_kind == 8)
        y = Char.y - 20;
    else
        y = Char.y - 30;

    if (Char.alive < 0 && Char.room == Mob.room
            && y < Mob.y && Char.row == Mob.row) {
        column = Mob.x / 32;
        hit = column == Char.column;
        if (!hit && Char.character_kind == 11) {
            int reach = GetDist();
            int edge;

            edge = Char.column + Minus1[Char.direction + 1];
            if (edge == column) {
                hit = reach > 8;
            } else if (column == Char.column + Plus1[Char.direction + 1]) {
                hit = reach < 4;
            }
        }
    }
    return hit;
}

int OppInfoUnderMob(OpponentPosition *opp)
{
    int result = 0;
    int height = (signed char)(opp->cell / 10) * 63 + 56;

    if (Level.preferred_opponent != 5 || opp->action != 8)
        height -= 30;
    if (height < Mob.y) {
        if (Mob.x / 32 == (opp->x - 144) / 32)
            result = 1;
        else
            result = 0;
    }
    return result;
}

void AddGateSound(int sound)
{
    AddSound(sound);
    if (sound == 9 && Level.level_number == 5 && Trob.room == 7
            && !SndOn(10065)) {
        if (Level.level_number != 5 || Kid.room != 7 || Kid.column >= 3)
            AddSong(65);
    }
}

TROBREC *GetTrobRoomCell(byte room, int cell)
{
    TROBREC *result = 0;
    int i;

    for (i = 0; i < TrobCt && !result; ++i) {
        if (Trobs[i].room == room && Trobs[i].cell == cell)
            result = &Trobs[i];
    }
    return result;
}

TROBREC *GetTrobId(byte kind, sbyte id)
{
    sbyte found = 0;
    TROBREC *result = 0;
    int i;

    for (i = 0; i < TrobCt && !result; ++i) {
        if (Trobs[i].kind == kind && Trobs[i].state != -1 && ++found == id)
            result = &Trobs[i];
    }
    return result;
}

int IsVisibleRoomCell(byte room, sbyte cell)
{
    int visible;

    if (Room.number == room || (Room.left == room && cell % 10 == 9)
            || (Room.below == room && cell < 10)
            || (Room.below_left == room && cell == 9))
        visible = 1;
    else
        visible = 0;
    return visible;
}

unsigned char CharObjForePriority[15] = {
    127, 127, 127, 100, 127, 50, 127, 127, 127, 100, 127, 127, 127, 200, 200
};

unsigned char NonCharObjForePriority[13] = {
    127, 127, 127, 127, 200, 60, 225, 50, 200, 80, 210, 210, 60
};
