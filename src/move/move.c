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

void add_mobs(void);
void anim_mobs(void);
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
void select_trob_cell(int);
void animate_flask(void);
void animate_loose_floor(void);
void animate_exit(void);
void animate_torch(int);
void animate_gate(void);
void animate_plate(void);
extern void animate_desert_exit(void);
extern void AnimScythe(void);
extern void animate_lava(void);
extern void animate_cave_bars(void);
extern void animate_desert_rocks(void);
extern void animate_desert_planks(void);
extern void animate_desert_stone(void);
extern void animate_jaffar_blink(void);
extern void animate_dart(void);
extern void AnimRoofPillar(void);
extern void animate_roof_ship(void);
extern void animate_roof_glass(void);
extern void animate_spider(void);
extern void animate_crystal(void);
extern void animate_gargoyle(void);
extern void animate_sacred_flame(void);
extern void animate_bridge_floor(void);

/* Draft. External provider names and same-file prototypes remain provisional. */
void AnimTrob(int index)
{
    select_room_arrays(Trob.room);
    select_trob_cell(Trob.cell);
    switch (Trob.kind) {
    default:
        Trob.state = -1;
        break;
    case 4:
        if (Level.environment_kind == 1)
            animate_desert_exit();
        else
            animate_gate();
        break;
    case 5:
    case 6:
    case 34:
        animate_plate();
        break;
    case 10:
        animate_flask();
        break;
    case 11:
        animate_loose_floor();
        break;
    case 13:
        AnimScythe();
        break;
    case 17:
        animate_exit();
        break;
    case 19:
    case 32:
        animate_torch(index);
        break;
    case 23:
        animate_lava();
        break;
    case 27:
        animate_cave_bars();
        break;
    case 28:
        animate_desert_rocks();
        break;
    case 29:
        animate_desert_planks();
        break;
    case 30:
        animate_desert_stone();
        break;
    case 31:
        animate_jaffar_blink();
        break;
    case 36:
        animate_dart();
        break;
    case 37:
        AnimRoofPillar();
        break;
    case 38:
        animate_roof_ship();
        break;
    case 39:
        animate_roof_glass();
        break;
    case 40:
        animate_spider();
        break;
    case 41:
        animate_crystal();
        break;
    case 42:
        animate_gargoyle();
        break;
    case 43:
        animate_sacred_flame();
        break;
    case 44:
        animate_bridge_floor();
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
Rect *clip_char_rects[5] = {
    0, 0, &clip_char_rect_templates[2], &clip_char_rect_templates[0], &clip_char_rect_templates[1]
};

extern int IS_SIX_KIND(byte);
extern sbyte CellToCol(byte room, int cell);
extern ROOMREC Room;
extern sbyte CellToRow(byte room, int cell);
extern Rect *ComputeOffsetCellRect(Rect *out, Rect *base, int col, int row);
Rect *compute_trob_move_rect(Rect *out, Rect *base);
sbyte check_trob(void);
sbyte check_trob_left(void);
sbyte check_trob_right(void);
sbyte check_trob_above(void);
sbyte check_trob_above_right(void);
sbyte check_trob_below(void);
void mark_move(sbyte cell, Rect *rect);
void mark_bufs_for_rect(CellMark, Rect *, sbyte);
int is_visible_room_cell(byte, sbyte);

void redraw_torch(void)
{
    struct { Rect rect; union { int cave; struct { byte pad; sbyte cell; } bytes; } tail; } work;
    work.tail.cave = (Level.environment_kind == 2);

    work.rect = *(work.tail.cave ? &TempTorchRect : &CaveTorchRect);
    if (compute_trob_move_rect(&work.rect, &work.rect)) {
        if (work.tail.cave) {
            mark_move(Trob.cell, &work.rect);
            work.tail.bytes.cell = check_trob_above(); mark_move(work.tail.bytes.cell, &work.rect);
            mark_move(check_trob_above_right(), &work.rect);
        }
        mark_move(check_trob_right(), &work.rect);
    }
}

void redraw_exit(void)
{
    struct { Rect rect; byte pad; sbyte cell; } work;

    if (compute_trob_move_rect(&work.rect, exit_rects[Level.environment_kind])) {
        mark_move(Trob.cell, &work.rect);
        work.cell = check_trob_right(); mark_move(work.cell, &work.rect);
        work.cell = check_trob_above(); mark_move(work.cell, &work.rect);
        mark_move(check_trob_above_right(), &work.rect);
    }
}

void redraw_flask(void)
{
    Rect rect;

    if (compute_trob_move_rect(&rect, &flask_rect))
        mark_move(check_trob(), &rect);
}

void redraw_plate(void)
{
    Rect rect;
    Rect *source;

    if (Level.environment_kind == 3)
        source = &temple_plate_rect;
    else
        source = &plate_rect;
    if (compute_trob_move_rect(&rect, source))
        mark_bufs_for_rect(mark_move, &rect, -1);
}

void redraw_loose(void)
{
    struct { Rect rect; byte pad; sbyte cell; } work;

    if (compute_trob_move_rect(&work.rect, loose_rects[Level.environment_kind])) {
        mark_move(work.cell = check_trob(), &work.rect);
        mark_move(check_trob_right(), &work.rect);
        if (Level.environment_kind == 3)
            mark_move(check_trob_below(), &work.rect);
    }
}

void redraw_gate(void)
{
    struct { Rect rect; byte pad; sbyte cell; } work;

    if (Level.environment_kind == 3)
        work.rect = temple_gate_rect;
    else
        work.rect = gate_rect;
    work.cell = check_trob_above_right();
    if (work.cell != 30 && work.cell >= 0 && !IS_SIX_KIND((&Level.cell_kinds[0][0])[30 * Room.number - 30 + work.cell]))
        work.rect.top = 0;
    if (compute_trob_move_rect(&work.rect, &work.rect)) {
        mark_move(Trob.cell, &work.rect);
        work.cell = check_trob_right(); mark_move(work.cell, &work.rect);
        mark_move(check_trob_above_right(), &work.rect);
    }
}

void redraw_trob(Rect *base)
{
    struct { Rect rect; byte pad; sbyte cell; } work;

    if (base && compute_trob_move_rect(&work.rect, base)) {
        mark_move(work.cell = check_trob(), &work.rect);
        mark_move(check_trob_right(), &work.rect);
    }
}

Rect *compute_trob_move_rect(Rect *out, Rect *base)
{
    Rect *result = out;
    sbyte column = CellToCol(Trob.room, Trob.cell);
    out = result;
    return ComputeOffsetCellRect(out, base, column,
                                 CellToRow(Trob.room, Trob.cell));
}

sbyte check_trob(void)
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

int check_trob_visibility(void)
{
    int visible = is_visible_room_cell(Trob.room, Trob.cell);
    if (!visible)
        Trob.state = -1;
    return visible;
}

sbyte check_trob_left(void)
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

sbyte check_trob_right(void)
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

sbyte check_trob_above(void)
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

sbyte check_trob_above_right(void)
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

sbyte check_trob_below(void)
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
extern void on_ground(void);
extern void FallOn(void);
extern void Falling(void);
extern sbyte get_above(void);
extern sbyte get_under_foot(void);
extern void fall_spikes(void);
extern byte CutRoom;
extern CHARREC Char;
void break_loose_floor(sbyte);
void jam_plate(void);
void push_plate(sbyte, int);

void check_floor(void)
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
        on_ground();
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
            on_ground();
        break;
    }
}

extern FRAMEREC Frame;

void check_press(void)
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
        kind = get_above();
    else if (action < 2 || action == 7 || action == 5 || action == 8) {
        kind = get_above();
        if (frame == 0x4f && kind == 11)
            break_loose_floor(0);
        else if (frame == 0x4f && kind == 15)
            fall_spikes();
        else
            if (!(Frame.flags & 0x40))
                kind = 0;
            else
                kind = get_under_foot();
    } else
        kind = 0;

    if (kind == 5 || kind == 6 || kind == 34) {
        if (Char.alive >= 0 && Char.character_kind != 4) {
            jam_plate();
            return;
        }
        push_plate(0, -1);
    } else if (kind == 11 || kind == 15) {
        AlertGuard = 1;
        if (kind == 11) {
            break_loose_floor(0);
            return;
        }
        if (kind == 15)
            fall_spikes();
    }
}

extern int FrameNum;

extern GAMESTATE GameState;
extern CHARREC Kid;
int get_torch_frame(int);

void animate_flask(void)
{
    if (ObjId == 10) {
        if (Trob.state < 0 || !check_trob_visibility())
            return;
        {
            unsigned state = BlueSpec.state & ~31U;
            int frame = BlueSpec.state & 31;
            if (frame >= 27)
                frame = 0;
            else
                ++frame;
            BlueSpec.state = state | frame;
            redraw_flask();
        }
        return;
    }
    Trob.state = -1;
}

void animate_torch(int index)
{
    if (check_trob_visibility()) {
        int frame = get_torch_frame((byte)BlueSpec.state);
        BlueSpec.state = (BlueSpec.state & 0xff00U) + frame;
        if (!GameState.fade_active || ((FrameNum + index) & 1))
            redraw_torch();
    }
}

extern struct {
    signed char argument, entrance_mode, sword_kind, death_countdown;
    int field_4, shadow_timer;
} KidInfo;
extern struct { unsigned char channels, volume; } Midi;

extern void AddSound(int), dispatch_sound(int);
void stop_gate(int);
void check_gate_fingers(void);
int did_player_escape_guard(void);
void add_gate_sound(int);
extern int move_cave_gate(int);

void animate_gate(void)
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
                    check_gate_fingers();
                    if (did_player_escape_guard())
                        KidInfo.death_countdown = 4;
                }
                add_gate_sound(sound);
                Trob.state = -1;
            }
        } else {
            if (height == 255)
                stop_gate(255);
            else {
                height += GateInc[phase];
                if (phase == 0 || phase == 1) {
                    if (height <= 0) {
                        height = 0;
                        stop_gate(height);
                        check_gate_fingers();
                    } else if (height < 200) {
                        add_gate_sound(Level.environment_kind != 3 ? 4 : 7);
                        Trob.state = 1;
                        if (Level.environment_kind == 3)
                            height = move_cave_gate(height);
                    } else if (Level.level_number == 5 && Trob.room == 7)
                        height -= GateInc[phase];
                } else if (height < 200) {
                    if (Level.environment_kind != 3) {
                        if (height % 8 == 0)
                            add_gate_sound(4);
                    } else
                        add_gate_sound(7);
                } else if (phase < 3) {
                    redraw_gate();
                    height = 250;
                    Trob.state = 0;
                    add_gate_sound(Level.environment_kind != 3 ? 5 : 8);
                } else {
                    height = 255;
                    stop_gate(height);
                }
            }
        }
        BlueSpec.state = (BlueSpec.state & 0xff00U) + height;
    }
    if (height <= 200)
        redraw_gate();
}

extern void copy_kid_to_char(void);
extern void JumpSeq(int);
extern sbyte FirstInRow(sbyte);

void stop_gate(int height)
{
    int actor_cell;

    Trob.state = -1;
    add_gate_sound(Level.environment_kind != 3 ? 6 : 8);
    copy_kid_to_char();
    actor_cell = FirstInRow(Char.row - 1) + Char.column;
    if (Trob.cell == actor_cell && Char.direction == -1
            && Char.action == 2 && height == 0) {
        JumpSeq(47);
        Kid = Char;
    }
}

void check_gate_fingers(void)
{
    if (Trob.room == Kid.room && (Kid.action == 2 || Kid.action == 6)) {
        sbyte actor_cell = FirstInRow(Kid.row) + Kid.column;
        if (Trob.cell - actor_cell == -10 || Trob.cell - actor_cell == -11)
            AddSound(31);
    }
}

extern CHARREC RoomOpp[5];
#pragma intrinsic(abs)

int did_player_escape_guard(void)
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

void animate_exit(void)
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
                    dispatch_sound(0);
                    AddSound(8);
                }
            } else if (Room.number != 4 || Level.level_number != 13)
                AddSound(7);
        }
    }
    BlueSpec.state = (BlueSpec.state & 0xff00U) + height;
    redraw_exit();
}

extern int get_cave_torch_frame(int frame), choose_trapdoor_delay(int frame);
extern int EMPTYRECT(Rect far *rect);
extern sbyte index_cell(sbyte column, sbyte row);
extern CELLREC Cell;

int IsExitOpen(void)
{
    if ((byte)Cell.modifier >= 42)
        return 1;
    return 0;
}

int get_torch_frame(int frame)
{
    int result;
    if (Level.environment_kind == 3)
        result = get_cave_torch_frame(frame);
    else
        result = choose_trapdoor_delay(frame);
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

void mark_rect_buf(MarkedRect *mark, int cell, Rect *clip);
void mark_guard_updates(Rect *clip, sbyte except);

void mark_top(int cell, Rect *clip)
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

void mark_move(sbyte cell, Rect *clip)
{
    if (cell < 30) {
        if (cell >= 0)
            mark_rect_buf(&MoveBuf[cell], cell, clip);
        else
            mark_top(cell, clip);
        mark_guard_updates(clip, -1);
    }
}

void mark_floor(sbyte cell, Rect *clip)
{
    if (cell < 30) {
        if (cell >= 0) {
            mark_rect_buf(&FloorBuf[cell], cell, clip);
            return;
        }
        mark_top(cell, clip);
    }
}

void mark_half(sbyte cell)
{
    if (cell < 30) {
        if (cell >= 0) {
            HalfBuf[cell] = 1;
            return;
        }
        mark_top(cell, 0);
    }
}

void mark_redraw(int cell)
{
    extern unsigned int RedrawBuf[];

    if (cell < 30) {
        if (cell >= 0)
            *(int *)((byte *)RedrawBuf + (cell << 1)) = 1;
        else
            mark_top(cell, 0);
    }
}

void mark_cell_redraw(void)
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
    mark_redraw(cell);
}

void mark_foreground(sbyte cell, Rect *clip)
{
    if (cell < 30) {
        if (cell >= 0) {
            mark_rect_buf(&FRedrawBuf[cell], cell, clip);
            return;
        }
        mark_top(cell, clip);
    }
}

void mark_bufs_for_rect(CellMark mark, Rect *rect, sbyte except)
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
                mark(index_cell(column, row), rect);
        }
        mark_guard_updates(rect, except);
    }
}

void mark_guard_updates(Rect *clip, sbyte except)
{
    Rect hit;
    sbyte count = Level.room_objects[Room.number - 1].opponent_count;
    int i;

    for (i = 0; i < count; ++i) {
        if (RoomOpp[i].slot != except)
            RoomOpp[i].previous_mode |= SECTRECT(&hit, &RoomOpp[i].bounds, clip);
    }
}

void mark_rect_buf(MarkedRect *mark, int cell, Rect *clip)
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

int get_link_next(int link);
int get_link_cell(int link);
byte get_link_room(int link);
TROBREC *get_trob_room_cell(byte room, int cell);
void add_trob(byte room, sbyte cell, sbyte state, byte kind);
sbyte trigger_object(byte kind, byte room, sbyte cell, int action);
extern unsigned Rnd(unsigned limit);

void trigger_link(int link, int action)
{
    byte room, kind;
    sbyte cell, state;

    if (get_link_room(link) != -2 && link != 0xfd) {
        do {
            room = get_link_room(link);
            select_room_arrays(room);
            cell = get_link_cell(link);
            kind = BlueT[cell];
            state = trigger_object(kind, room, cell, action);
            if (state >= 0)
                add_trob(room, cell, state, kind);
            link = get_link_next(link);
        } while (link != 0xfd);
    }
}

void trigger_flask(byte room, sbyte cell)
{
    unsigned state = (BlueS[cell].state & ~31U) | (Rnd(26) + 1);
    BlueS[cell].state = state;
    add_trob(room, cell, 1, 10);
}

extern sbyte trigger_reverse_gate(byte room, sbyte cell);
extern int is_dead_pos(unsigned int frame);

sbyte trigger_gate(byte room, sbyte cell, int action)
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
        TROBREC *trob = get_trob_room_cell(room, cell);
        if (is_dead_pos(Char.frame) && Char.alive >= 2)
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
        state = trigger_reverse_gate(room, cell);
        break;
    }
    return state;
}

extern int query_sound(int id);
extern void AddSong(int sound);
extern void trigger_slide_rock(byte room, sbyte cell, sbyte state);
extern sbyte trigger_cave_bars(byte room, sbyte cell);
extern void trigger_dart(byte, sbyte);

sbyte trigger_object(byte kind, byte room, sbyte cell, int action)
{
    sbyte state;

    switch (kind) {
    case 4:
        state = trigger_gate(room, cell, action);
        break;
    case 17:
        if (room != Level.entry_room && Level.level_number == 8 && KidInfo.sword_kind != 1) {
            state = -1;
        } else {
            int height = BlueS[cell].state & 255;
            state = height ? -1 : 1;
            if (state == 1 && !query_sound(10027) && Level.level_number != 13)
                AddSong(27);
        }
        break;
    case 25:
        if (action == 5)
            state = 2;
        else
            state = 1;
        trigger_slide_rock(room, cell, state);
        state = -1;
        break;
    case 27:
        state = trigger_cave_bars(room, cell);
        break;
    case 36:
        if (!(Cell.modifier & 0x0800U))
            trigger_dart(room, cell);
        state = -1;
        break;
    default:
        state = -1;
        break;
    }
    return state;
}

void trigger_torch(byte room, sbyte cell)
{
    unsigned state = (BlueS[cell].state & 0xff00U) + Rnd(Level.environment_kind == 3 ? 8 : 3);
    BlueS[cell].state = state;
    add_trob(room, cell, 1, 19);
}

void add_trob(byte room, sbyte cell, sbyte state, byte kind)
{
    TROBREC *found = get_trob_room_cell(room, cell);

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

sbyte find_wall_column(byte room, sbyte column, sbyte row, sbyte step)
{
    do {
        column += step;
        if (IsWall(ReadCell(room, column, row)))
            break;
    } while (column >= 0 && column <= 10);
    return column;
}

extern int far _fastcall get_slide_rock_x(unsigned char room, signed char row);

int far _fastcall probe_jump_anchor(signed char room, signed char row)
{
    int x;

    if (Level.environment_kind == 2)
        x = get_slide_rock_x(room, row);
    else
        x = 0;
    return x;
}

int get_link_next(int link)
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

int get_link_cell(int link)
{
    return Level.links[link].cell;
}

int link_has_previous(int link)
{
    if (Level.links[link].prev != 0xfd)
        return 1;
    return 0;
}

int link_has_next(int link)
{
    if (Level.links[link].next != 0xfd)
        return 1;
    return 0;
}

byte get_link_room(int link)
{
    return Level.links[link].room;
}

void push_plate(sbyte kind, int link)
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
        add_trob(Cell.room, Cell.index, 1, kind);
        if (Char.frame != 185 && (Cell.room != 4 || Level.level_number != 13))
            AddSound(3);
        redraw_plate();
        trigger_link(link, (byte)kind);
        AlertGuard = 1;
    } else {
        trigger_link(link, (byte)kind);
    }
    Level.cell_info[room - 1][cell].modifier = state;
}

void jam_plate(void)
{
    int link = Cell.modifier & 255;

    ChangeLinkTimer(link, 31);
    push_plate(Cell.kind, link);
}

void animate_plate(void)
{
    if (Trob.state >= 0) {
        int link = BlueSpec.state & 255;
        sbyte timer = GetLinkTimer(link);
        if (timer < 31) {
            ChangeLinkTimer(link, timer - 1);
            if (timer < 2) {
                Trob.state = -1;
                redraw_plate();
                BlueSpec.state &= ~0x0800U;
                return;
            }
        } else
            Trob.state = -1;
    }
}

void close_exit(byte room, sbyte cell)
{
    if (Level.environment_kind != 1) {
        RoomObject *objects = BlueS;
        objects[cell].state = (objects[cell].state & 0xff00U) + 42;
        add_trob(room, cell, 4, 17);
    }
}

void make_space(byte room, sbyte cell);
void add_mob(void);
void PassThru(void);
void mark_mob_redraw(sbyte layer);
extern sbyte IndexCell1(sbyte column, sbyte row);
static MOBREC mobs[30];
int MobCt;
extern sbyte GetRow(int value);

void animate_loose_floor(void)
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
            make_space(Trob.room, Trob.cell);
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
            add_mob();
            Trob.state = -1;
        }
    }
    redraw_loose();
}

void make_space(unsigned char room, signed char cell)
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

extern void shake_trapdoor(void);
extern void shake_bones(byte room, sbyte row);

void shake_loose_floor(void);

void break_loose_floor(sbyte sound)
{
    if (Char.character_kind == 1 && sound == 0) {
        shake_loose_floor();
    } else {
        unsigned state = Cell.modifier & ~0x40U;
        if (sound)
            state = (state & ~4U) | 11;
        BlueS[Cell.index].state = state;
        add_trob(Cell.room, Cell.index, sound, 11);
        AddSound(20);
    }
}

void shake_loose_floor(void)
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
        add_trob(Cell.room, Cell.index, 1, 11);
        AddSound(20);
    }
}

void shake_row(byte room, sbyte row)
{
    MOBREC saved = Mob;
    sbyte column = 0;

    do {
        switch (ReadCell(room, column, row)) {
        case 11:
            shake_loose_floor();
            break;
        case 15:
            fall_spikes();
            break;
        case 26:
            shake_trapdoor();
            break;
        }
    } while (++column < 10);

    if (Level.preferred_opponent == 2)
        shake_bones(room, row);
    Mob = saved;
}

void add_mob(void)
{
    if (MobCt < 30)
        mobs[MobCt++] = Mob;
}

extern void move_dart_mob(void);
extern void MobSpikes(void);
extern void move_rock_record(void);
extern void check_slide_rock_crush(void);
extern void move_trapdoor_record(void);
extern void move_fireball(void);
void animate_mob(void);
void mob_loose(void);
void check_loose_crush(void);
void make_rubble(void);
void knock_loose(void);

MOBREC *get_mob_type_item(byte kind, int occurrence)
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

void select_trob_cell(int cell)
{
    BlueSpec = BlueS[cell];
    ObjId = BlueT[cell];
}

int MobHeight[13] = {18, 18, 2, 16, 27, 39, 69, 69, 12, 12, 37, 37, 18};
int MobWidth[13] = {50, 50, 4, 57, 27, 31, 52, 52, 24, 24, 69, 57, 17};

void anim_mobs(void)
{
    struct {
        MOBREC *destination;
        int kept, index;

    } work;

    {
        int count = MobCt;
        for (MobI = 0; MobI < count; ++MobI) {
            Mob = mobs[MobI];
            animate_mob();
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

extern void step_bridge_mob(void);

void animate_mob(void)
{
    switch (Mob.kind) {
    case 0:
    case 1:
    case 3:
        mob_loose();
        if (Mob.velocity <= 0)
            ++Mob.velocity;
        check_loose_crush();
        break;
    case 2:
        move_dart_mob();
        return;
    case 4:
        MobSpikes();
        return;
    case 6:
        move_rock_record();
        check_slide_rock_crush();
        return;
    case 10:
        move_trapdoor_record();
        return;
    case 11:
        step_bridge_mob();
        return;
    case 12:
        move_fireball();
        return;
    }
}

void mob_loose(void)
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
                knock_loose();
            else if (kind == 15)
                fall_spikes();

            if (kind != 11 && kind != 15 && !IS_SIX_KIND(kind)) {
                AddSound(2);
                shake_row(Mob.room, Mob.row);
                Mob.y = (Mob.row + 1) * 63;
                Mob.velocity = -2;
                mobs[MobI].flags = Mob.flags = 1;
                make_rubble();
                return;
            }
            PassThru();
        }
    }
}

void make_rubble(void)
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
        push_plate(work.plate, -1);
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
            TROBREC *flask = get_trob_room_cell(Cell.room, Cell.index);
            if (flask) {
                Trob = *flask;
                redraw_flask();
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
    mark_mob_redraw(0);
}

void knock_loose(void)
{
    make_space(Cell.room, Cell.index);
    Mob.velocity >>= 1;
    mobs[MobI] = Mob;
    Mob.y = Mob.row * 63 + 66;
    mark_mob_redraw(0);
    Mob.y = mobs[MobI].y + 6;
    PassThru();
    add_mob();
    Mob = mobs[MobI];
    mark_mob_redraw(0);
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

void add_mob_to_frame(void);
void add_loose_to_frame(void);
void compute_mob_rect(Rect *rect);
void add_mob_obj(int y);
int opp_under_mob(OpponentPosition *opp);
int loose_hits_actor(void);

extern void animate_bridge_mob(void);
extern void move_dart_to_tile(void);
extern void ATMSpikes(void);
extern void move_slide_rock(void);
extern void move_trapdoor(void);
extern void update_fireball_view(void);
extern Rect NullRect;
extern void OFFSETRECT(Rect far *, Rect far *, int, int);
extern FCHARREC FChar;

int is_loose_pushing_plate(void)
{
    byte kind = Level.environment_kind == 3 ? 1 : 3;
    int occurrence = 0;
    int pushing = 0;
    MOBREC *mob;

    do {
        mob = get_mob_type_item(kind, ++occurrence);
        if (mob && mob->room == Cell.room && mob->row == Cell.row
                && mob->x / 32 == Cell.column && mob->flags)
            pushing = 1;
    } while (mob && !pushing);
    return pushing;
}

void add_mobs(void)
{
    int index = 0;
    MOBREC *entry;

    if (index < MobCt) {
        entry = mobs;
        do {
            Mob = *entry;
            add_mob_to_frame();
            *entry = Mob;
            ++entry;
        } while (++index < MobCt);
    }
}

void add_mob_to_frame(void)
{
    switch (Mob.kind) {
    case 0:
    case 1:
    case 3:
        add_loose_to_frame();
        break;
    case 2:
        move_dart_to_tile();
        return;
    case 4:
        ATMSpikes();
        return;
    case 6:
        move_slide_rock();
        return;
    case 10:
        move_trapdoor();
        return;
    case 11:
        animate_bridge_mob();
        return;
    case 12:
        update_fireball_view();
        return;
    }
}

void add_loose_to_frame(void)
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
        mark_mob_redraw(1);
        compute_mob_rect(&rect);
        add_mob_obj(y);
        ++column;
        cell = index_cell(column, row);
        if (Room.below != Mob.room)
            mark_floor(cell, &rect);
        cell = GetRow(y - height);
        if (cell != row)
            mark_floor(index_cell(column, cell), &rect);
    }
}

void compute_mob_rect(Rect *rect)
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

void mark_mob_redraw(sbyte layer)
{
    Rect rect;
    CellMark mark;

    compute_mob_rect(&rect);
    if (layer == 1)
        mark = mark_foreground;
    else if (layer == 2)
        mark = mark_floor;
    else
        mark = mark_move;
    mark_bufs_for_rect(mark, &rect, -1);
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
extern void set_object_cell(sbyte index);

void add_mob_obj(int y)
{
    int index = Ct[4]++;
    int zero;

    ObjTable[index].image = Mob.kind | 0x80;
    ObjTable[index].x = Mob.x;
    ObjTable[index].y = y;
    ObjTable[index].bank = 4;
    ObjTable[index].flags = Mob.flags;
    compute_mob_rect(&ObjTable[index].rect);
    zero = 0;
    ObjTable[index].sequence = zero;
    ObjTable[index].scratch = zero;
    ObjTable[index].order = -1;
    set_object_cell(index);
}

extern void crush_actor(void);
extern void load_actor_slot(sbyte index);
extern void save_actor_slot(void);
extern OpponentPosition *GetOppInfoIdx(int room, int index);
extern void CrushOppInfo(OpponentPosition *, byte);
extern signed char Plus1[2];
extern signed char Minus1[2];
extern int get_edge_distance(void);

void check_loose_crush(void)
{
    sbyte index;
    sbyte count;

    if (!Mob.room)
        return;
    copy_kid_to_char();
    if (loose_hits_actor()) {
        crush_actor();
        Kid = Char;
    }

    index = 0;
    count = Level.room_objects[Mob.room - 1].opponent_count;
    for (; index < count; ++index) {
        if (Mob.room == Kid.room) {
            load_actor_slot(index);
            if (loose_hits_actor()) {
                crush_actor();
                save_actor_slot();
            }
        } else {
            OpponentPosition *guard = GetOppInfoIdx(Mob.room, index);
            if (opp_under_mob(guard))
                CrushOppInfo(guard, Mob.room);
        }
    }
}

int loose_hits_actor(void)
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
            int reach = get_edge_distance();
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

int opp_under_mob(OpponentPosition *opp)
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

void add_gate_sound(int sound)
{
    AddSound(sound);
    if (sound == 9 && Level.level_number == 5 && Trob.room == 7
            && !query_sound(10065)) {
        if (Level.level_number != 5 || Kid.room != 7 || Kid.column >= 3)
            AddSong(65);
    }
}

TROBREC *get_trob_room_cell(byte room, int cell)
{
    TROBREC *result = 0;
    int i;

    for (i = 0; i < TrobCt && !result; ++i) {
        if (Trobs[i].room == room && Trobs[i].cell == cell)
            result = &Trobs[i];
    }
    return result;
}

TROBREC *get_trob_by_id(byte kind, sbyte id)
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

int is_visible_room_cell(byte room, sbyte cell)
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
