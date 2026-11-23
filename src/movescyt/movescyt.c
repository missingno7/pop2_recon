/* MOVESCYT.C, DOS scythe movement and collision. */
typedef signed char sbyte;
typedef unsigned char ubyte;
typedef unsigned int uword;

typedef struct {
    int top, left, bottom, right;
} Rect;

#pragma pack(1)
struct char_bounds { int top, left, bottom, right; };
typedef struct {
    signed char slot;                  /* 00 */
    signed char direction;             /* 01 */
    int x, y;                          /* 02, 04 */
    signed char character_kind;        /* 06 */
    unsigned int frame;                /* 07 */
    signed char column, row;           /* 09, 0a */
    signed char action;                /* 0b */
    signed char horizontal_step;       /* 0c */
    signed char vertical_step;         /* 0d */
    signed char room;                  /* 0e */
    signed char field_0f;              /* 0f */
    signed char stance;                /* 10 */
    signed char alive;                 /* 11 */
    signed char life_points;           /* 12 */
    signed char life_initial;          /* 13 */
    signed char life_delta;            /* 14 */
    int sequence_position;             /* 15 */
    int sequence_id;                   /* 17 */
    unsigned int sequence;             /* 19 */
    struct char_bounds bounds;                    /* 1b: top, left, bottom, right */
    signed char alert_delay;           /* 23 */
    int sequence_group;                /* 24 */
    int previous_x, previous_y;        /* 26, 28 */
    unsigned int previous_frame;       /* 2a */
    struct char_bounds previous_bounds;           /* 2c: top, left, bottom, right */
    int field_34;                      /* 34 */
    int previous_mode;                 /* 36 */
    unsigned char behavior;            /* 38 */
    signed char opponent_index;        /* 39 */
    unsigned char field_3a;            /* 3a */
    unsigned char unknown_3b[5];        /* 3b..3f */
} CHARREC;

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
extern CHARREC Char;
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
    if (Char.character_kind != 7 && Char.character_kind != 8 && Char.character_kind != 11 &&
        Char.alive < 0 && Char.row * 63 + 40 <= Char.y) {
        char kind = ReadCell(Char.room, Char.column, Char.row);
        if ((kind == 12 || kind == 13) && cell_x[Char.column] + 18 < Char.x)
            TriggerScythe(current_tile.room, current_tile.index);
    }
}

void far pascal CheckScytheHit(void)
{
    int done = 0;
    sbyte column, kind;
    Rect bounds;
    if (Char.character_kind != 0 && Char.character_kind != 2)
        return;
    if ((int)Char.sequence == 44)
        return;
    column = Char.column - 1;
    while (column <= Char.column + 1) {
        kind = ReadCell(Char.room, column, Char.row);
        if (kind == 12 || kind == 13) {
            if (GetScytheHitType(current_tile.modifier) &&
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
                BlueS[current_tile.index].modifier = (current_tile.modifier |= 0x40);
                if (kind == 12)
                    ReadCell(Char.room, column + 1, Char.row);
                else
                    ReadCell(Char.room, column - 1, Char.row);
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
    char kind = ReadCell(Char.room, Char.column, Char.row);
    if (kind == 12 && ScytheBladeExtended(current_tile.modifier)) {
        blocked = 1;
    } else if (kind == 13) {
        ReadCell(Char.room, Char.column - 1, Char.row);
        if (ScytheBladeExtended(current_tile.modifier) &&
            cell_x[Char.column] + 9 >= LeftEdge)
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
