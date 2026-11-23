/* MOVESCYT.C, DOS scythe movement and collision. */
typedef signed char sbyte;
typedef unsigned char ubyte;
typedef unsigned int uword;

typedef struct {
    int top, left, bottom, right;
} Rect;

#pragma pack(1)
typedef struct {
    int y, x;
    ubyte action;
    uword frame;
    sbyte column, row;
    ubyte reserved_11[3];
    ubyte room;
    ubyte reserved_15[2];
    sbyte life;
    ubyte reserved_18[7];
    int sequence_group;
    Rect bounds;
} Character;

typedef struct {
    ubyte kind;
    uword modifier;
    sbyte index;
    ubyte room;
    sbyte column, row;
} Cell;
#pragma pack()

typedef struct {
    uword modifier;
    uword extra;
} RoomCell;

extern uword BlueSpec;
extern sbyte trob_index;
extern ubyte trob_room;
extern sbyte trob_mode;
extern Character actor;
extern Cell current_tile;
extern RoomCell room_cells[24][30];
extern RoomCell near *BlueS;
extern int cell_x[];
extern int LeftEdge;

static Rect redraw_bounds = {26, -24, 43, 52};
static Rect first_blade = {26, 20, 33, 57};
static Rect second_blade = {26, 46, 33, 89};
int ScytheImg[20] = {
    0, 100, 97, 98, 97, 98, 97, 98, 97, 97,
    97, 97, 97, 97, 97, 97, 99, 100, 101, 0
};

extern void far pascal AddSound(int sound);
extern char far pascal ReadCell(ubyte room, sbyte column, sbyte row);
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
extern void far pascal add_trob(ubyte room, sbyte index, sbyte mode, ubyte kind);

int far pascal GetScytheHitType();
int far pascal ScytheBladeExtended();
int far pascal ComputeBladeHitRect(Rect near *bounds);
void far pascal RedrawScythe(int bloody);
void far pascal TriggerScythe(ubyte room, sbyte index);

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
        trob_mode = -1;
    }
    ReadCell(trob_room, trob_index % 10 - 1, trob_index / 10);
    *(ubyte near *)&current_tile.modifier = 0;
    room_cells[current_tile.room][current_tile.index].modifier =
        (current_tile.modifier |= (ubyte)BlueSpec);
}

void far pascal CheckScythe(void)
{
    if (actor.action != 7 && actor.action != 8 && actor.action != 11 &&
        actor.life < 0 && actor.row * 63 + 40 <= actor.x) {
        char kind = ReadCell(actor.room, actor.column, actor.row);
        if ((kind == 12 || kind == 13) && cell_x[actor.column] + 18 < actor.y)
            TriggerScythe(current_tile.room, current_tile.index);
    }
}

void far pascal CheckScytheHit(void)
{
    int done = 0;
    sbyte column, kind;
    Rect bounds;
    if (actor.action != 0 && actor.action != 2)
        return;
    if (actor.sequence_group == 44)
        return;
    column = actor.column - 1;
    while (column <= actor.column + 1) {
        kind = ReadCell(actor.room, column, actor.row);
        if (kind == 12 || kind == 13) {
            if (GetScytheHitType(current_tile.modifier) &&
                ComputeBladeHitRect(&bounds) &&
                SectRect(&bounds, &bounds, &actor.bounds) &&
                (actor.frame < 267 || actor.frame > 271)) {
                if (actor.action == 0) {
                    JumpSeq(113);
                    SetKidDeathMethod(5);
                } else {
                    JumpSeq(190);
                    actor.life = 1;
                }
                AnimChar();
                decrement_life_points(100);
                AddSound(85);
                BlueS[current_tile.index].modifier = (current_tile.modifier |= 0x40);
                if (kind == 12)
                    ReadCell(actor.room, column + 1, actor.row);
                else
                    ReadCell(actor.room, column - 1, actor.row);
                BlueS[current_tile.index].modifier = (current_tile.modifier |= 0x40);
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
    int phase = current_tile.modifier & 31;
    if (phase == 2)
        UnionRect(bounds, &first_blade, &second_blade);
    else if (phase == 16)
        *bounds = first_blade;
    else if (phase == 17)
        *bounds = second_blade;
    else
        valid = 0;
    if (valid)
        OffsetRect(bounds, bounds, (current_tile.column + (current_tile.kind == 13 ? 1 : 0)) << 5,
                   63 * current_tile.row);
    return valid;
}

int far pascal GetScytheHitType(unsigned char value)
{
    unsigned int low5 = (unsigned int)value & 31U;

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
        trob_mode = 1;
    }
}

int far pascal ScytheBarrToStandUp(void)
{
    int blocked = 0;
    char kind = ReadCell(actor.room, actor.column, actor.row);
    if (kind == 12 && ScytheBladeExtended(current_tile.modifier)) {
        blocked = 1;
    } else if (kind == 13) {
        ReadCell(actor.room, actor.column - 1, actor.row);
        if (ScytheBladeExtended(current_tile.modifier) &&
            cell_x[actor.column] + 9 >= LeftEdge)
            blocked = 1;
    }
    return blocked;
}

int far pascal ScytheBladeExtended(unsigned char value)
{
    unsigned int low5 = (unsigned int)value & 31U;
    return low5 >= 3U && low5 <= 15U;
}
void far pascal TriggerScythe(ubyte room, sbyte index)
{
    if (current_tile.kind == 12) {
        ReadCell(room, current_tile.column + 1, current_tile.row);
        room = current_tile.room;
        index = current_tile.index;
    }
    if (!(room_cells[room][index].modifier & 31))
        add_trob(room, index, 1, 13);
}
