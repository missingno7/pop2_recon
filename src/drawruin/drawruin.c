/* DRAWRUIN.C, DOS overlay 8 code frame 2a74. */
#include "types.h"
#include "level.h"
#include "char.h"

#pragma pack(1)
typedef struct RuinCell {
    byte pass;
    sbyte column, row;
    byte foreground;
    unsigned int modifier;
    byte background, background_modifier;
} RuinCell;
#pragma pack()

typedef struct BackImage {
    byte placement[10];
    Rect clip;
} BackImage;

typedef int (far pascal *AddImage)(byte, int, sbyte, sbyte, byte, byte);

extern int NewRoom;
extern Rect CellClipRect;
extern BackImage far *far _fastcall GetBackTableImgRow(int image, byte column, byte row);
extern int SECTRECT(Rect far *, const Rect far *, const Rect far *);
extern void UNIONRECT(Rect far *, const Rect far *, const Rect far *);
extern int AddBImgToBTable(byte, int, sbyte, sbyte, byte, byte);
extern int AddBFImgToBTable(byte, int, sbyte, sbyte, byte, byte);
extern int AddBFImgToFTable(byte, int, sbyte, sbyte, byte, byte);
extern int AddFImgToFTable(byte, int, sbyte, sbyte, byte, byte);
extern byte PieceRightId(sbyte, sbyte);

void DrawRuinFlaw(RuinCell *cell);

void DrawRuinBackWall(RuinCell *cell)
{
    int image;
    byte background_modifier, tile, layer;
    int fallback;
    BackImage far *shape;
    Rect overlap;

    background_modifier = cell->background_modifier & 15;
    image = (*(unsigned int *)&cell->background) & 31;
    if (image) {
        if (!NewRoom) {
            shape = GetBackTableImgRow(image, cell->column - 1, cell->row);
            if (shape && SECTRECT(&overlap, &shape->clip, &CellClipRect)) {
                fallback = 0;
                UNIONRECT(&shape->clip, &shape->clip, &CellClipRect);
                if (image >= 1 && image <= 5) {
                    shape = GetBackTableImgRow(7, cell->column - 1, cell->row);
                    if (shape && SECTRECT(&overlap, &shape->clip, &CellClipRect))
                        UNIONRECT(&shape->clip, &shape->clip, &CellClipRect);
                }
            } else {
                shape = 0;
                fallback = 1;
            }
        } else {
            fallback = 0;
            shape = 0;
        }

        if (!shape && (fallback || !background_modifier ||
                       (cell->column == -1 && background_modifier <= 3) || cell->row == 3)) {
            if (image >= 1 && image <= 5) {
                tile = image == 5 && (background_modifier == 2 || background_modifier == 3 ||
                                      background_modifier == 6 || background_modifier == 7)
                     ? background_modifier - 2 : background_modifier;
                tile += 36;
                AddBImgToBTable(tile, 7, cell->column, cell->row, 0, 0);
                if (image == 5 && NewRoom)
                    AddBImgToBTable(tile, 7, cell->column + 2, cell->row, 0, 0);
                layer = 10;
            } else {
                layer = 0;
            }
            if (image)
                AddBImgToBTable(background_modifier + 36, image, cell->column, cell->row, layer, 0);
        }
    }
}

void DrawRuinBlock(RuinCell *cell)
{
    int image;

    switch (cell->pass) {
    case 0:
        if (NewRoom)
            DrawRuinBackWall(cell);
        break;
    case 5:
        image = (cell->modifier & 12) >> 2;
        if (image != 2 && PieceRightId(cell->column, cell->row) != 20) {
            image = image != 3 ? image + 12 : 53;
            AddBImgToBTable(20, image, cell->column, cell->row, 10, 0);
        }
        /* Fall through to the block and its flaw. */
    case 1:
        AddFImgToFTable(20, (cell->modifier & 3) + 14, cell->column, cell->row, 0, 0);
        DrawRuinFlaw(cell);
        break;
    }
}

void DrawRuinBones(RuinCell *cell)
{
    AddImage add_image = 0;

    switch (cell->pass) {
    case 0:
        DrawRuinBackWall(cell);
        break;
    case 1:
        add_image = AddFImgToFTable;
        break;
    case 2:
        add_image = AddBFImgToFTable;
        break;
    case 5:
        add_image = AddBFImgToBTable;
        break;
    }
    if (add_image) {
        add_image(21, -2, cell->column, cell->row, 10, 0);
        DrawRuinFlaw(cell);
    }
}

typedef struct ShapePosition {
    int x, y, flags;
} ShapePosition;

static union RuinTables {
    struct {
        int crevice_flags[4];
        int loose_odd[12], loose_even[12];
        ShapePosition flaw_positions[15], exit_positions[2];
    } rows;
    int words[79];
} ruin_tables = {
    {
        { 0, 2, 2, 0 },
        { 0, 1, 0, 2, 2, 0, 0, 0, 1, 1, 1, 0 },
        { 0, 2, 0, 1, 1, 0, 0, 2, 2, 2, 0, 2 },
        {
            { 76, 0, -32 },
            { 77, 22, -7 },
            { 67, 0, -3 },
            { 67, -32, -3 },
            { 68, -17, -6 },
            { 69, -8, 18 },
            { 70, 4, -32 },
            { 71, -16, -5 },
            { 72, 1, -4 },
            { 73, 4, 31 },
            { 74, 2, -25 },
            { 75, 4, -27 },
            { 78, -27, -7 },
            { 79, -16, -20 },
            { 80, 7, -31 }
        },
        {
            { 62, -15, -16 },
            { 65, 6, -60 }
        }
    }
};
#define exit_positions (ruin_tables.rows.exit_positions)

/* Loose selectors index the shared word storage across the full nibble. */
enum {
    LOOSE_ODD_WORD = sizeof ruin_tables.rows.crevice_flags / sizeof(int),
    LOOSE_EVEN_WORD = LOOSE_ODD_WORD + sizeof ruin_tables.rows.loose_odd / sizeof(int)
};

void DrawRuinCrevice(RuinCell *cell)
{
    int pattern, revealed;
    RuinCell block;

    pattern = cell->modifier & 3;
    revealed = cell->modifier & 0x80;
    switch (cell->pass) {
    case 0:
        if (NewRoom)
            DrawRuinBackWall(cell);
        break;
    case 1:
        if (revealed) {
            AddFImgToFTable(7, pattern + 58, cell->column, cell->row, 10, 0);
        } else {
            block = *cell;
            block.modifier &= ~3;
            block.modifier |= ruin_tables.rows.crevice_flags[pattern];
            DrawRuinBlock(&block);
        }
        DrawRuinFlaw(cell);
        break;
    case 2:
        if (revealed)
            AddBFImgToFTable(7, pattern + 54, cell->column, cell->row, 10, 0);
        break;
    case 5:
        if (revealed) {
            AddBFImgToBTable(7, pattern + 54, cell->column, cell->row, 10, 0);
        } else {
            block = *cell;
            block.modifier |= pattern ? 8 : 12;
            DrawRuinBlock(&block);
        }
        break;
    }
}

void DrawRuinExit1(RuinCell *cell)
{
    AddImage add_image = 0;
    byte layer = 10;
    int flaw = 1;

    switch (cell->pass) {
    case 0:
        if (NewRoom) {
            DrawRuinBackWall(cell);
            add_image = AddBImgToBTable;
        }
        break;
    case 1:
        add_image = AddFImgToFTable;
        layer = 0;
        break;
    case 2:
        add_image = AddBFImgToFTable;
        flaw = 0;
        break;
    case 5:
        add_image = AddBFImgToBTable;
        break;
    default:
        flaw = 0;
        break;
    }
    if (add_image) {
        add_image(16, -2, cell->column, cell->row, layer, 0);
    }
    if (flaw)
        DrawRuinFlaw(cell);
}

#pragma pack(1)
typedef struct RuinPositions {
    char before_flaw[0x156];
    byte flip_flaw;
    ShapePosition front_flaw, back_flaw;
    char before_gate[0x420 - 0x163];
    int gate_y;
    char before_exit[0x440 - 0x422];
    int exit_y;
    ShapePosition exit;
} RuinPositions;
#pragma pack()

typedef struct MovingExit {
    byte reserved[2];
    signed char height;
} MovingExit;

extern RuinPositions *Pieces;
extern Rect GamePlayRect;
extern ROOMREC Room;

extern CHARREC Kid;
extern signed char FirstInRow(byte, sbyte);
extern MovingExit *GetTrobRoomCell(int);

void DrawRuinExitMovable(RuinCell *cell);

void DrawRuinExit2(RuinCell *cell)
{
    AddImage add_image = 0;
    byte layer = 10;
    int flaw = 0;

    switch (cell->pass) {
    case 0:
        if (NewRoom) {
            DrawRuinBackWall(cell);
        } else {
            Pieces->exit = exit_positions[0];
            AddBFImgToBTable(57, -2, cell->column, cell->row, 10, 0);
            add_image = 0;
        }
        break;
    case 1:
        add_image = AddFImgToFTable;
        layer = 0;
        flaw = 1;
        break;
    case 2:
        add_image = AddBFImgToFTable;
        break;
    case 5:
        if (!NewRoom)
            add_image = AddBFImgToBTable;
        flaw = 1;
        break;
    }
    if (add_image)
        add_image(17, -2, cell->column, cell->row, layer, 0);
    if (flaw)
        DrawRuinFlaw(cell);
    if (cell->pass == 0)
        DrawRuinExitMovable(cell);
}

void DrawRuinExitMovable(RuinCell *cell)
{
    AddImage add_image;
    MovingExit *moving;
    Rect saved_clip, door_clip;

    if (cell->pass == 0) {
        saved_clip = CellClipRect;
        door_clip = GamePlayRect;
        door_clip.top = cell->row * 63 - 5;
        if (SECTRECT(&CellClipRect, &CellClipRect, &door_clip)) {
            moving = GetTrobRoomCell(cell->column + FirstInRow(Room.number, cell->row));
            if (moving && moving->height >= 4)
                AddBImgToBTable(17, -2, cell->column, cell->row, 10, 0);
            Pieces->exit_y = -(int)(19U + ((cell->modifier & 0xff) < 42 ? cell->modifier & 0xff : 42));
            AddBImgToBTable(57, -2, cell->column, cell->row, 10, 0);
            add_image = Kid.frame >= 217 && Kid.frame <= 226 ? AddBFImgToFTable : AddBFImgToBTable;
            Pieces->exit = exit_positions[1];
            add_image(57, -2, cell->column, cell->row, 10, 0);
        }
        CellClipRect = saved_clip;
    }
}

extern sbyte ColNo, RowNo;
extern int Ct[5];

#pragma pack(1)
typedef struct DrawEntry {
    byte type, sheet;
    int shape, x, y;
    sbyte column, row;
    Rect clip;
    byte flip, attributes;
} DrawEntry;
#pragma pack()
extern DrawEntry far ForeTable[];

void DrawRuinFlaw(RuinCell *cell)
{
    int flaw, flags;

    if (cell->pass == 5) {
        flaw = cell->modifier >> 8 & 15;
        if (flaw == 11 || flaw == 12) {
            Pieces->front_flaw = ruin_tables.rows.flaw_positions[flaw - 11];
            Pieces->flip_flaw = flaw == 12 ? 1 : 0;
            AddBImgToBTable(18, -2, cell->column, cell->row, 10, 0);
        } else {
            flaw = cell->modifier & 0x3000;
            if (flaw) {
                flaw >>= 12;
                Pieces->back_flaw = ruin_tables.rows.flaw_positions[flaw + 11];
                AddBFImgToBTable(18, -2, cell->column, cell->row, 10, 0);
            }
        }
    } else if (cell->pass == 1) {
        flags = cell->modifier & 0xf00;
        if (flags && (unsigned)flags < 0xb00) {
            flaw = ((byte *)&flags)[1];
            if (flaw != 2 || !NewRoom) {
                int emitted;
                Pieces->back_flaw = ruin_tables.rows.flaw_positions[flaw + 1];
                if (flaw == 3 && !NewRoom &&
                    ColNo == cell->column && RowNo == cell->row)
                    emitted = 0;
                else
                    emitted = AddBFImgToFTable(18, -2, cell->column, cell->row, 10, 0);
                if (emitted && flaw == 4)
                    ForeTable[Ct[1] - 1].clip = GamePlayRect;
            }
        }
    }
}

void DrawRuinFloor(RuinCell *cell)
{
    switch (cell->pass) {
    case 0:
        DrawRuinBackWall(cell);
        break;
    case 1:
        AddFImgToFTable(1, (cell->modifier & 3) + 26, cell->column, cell->row, 0, 0);
        DrawRuinFlaw(cell);
        break;
    case 2:
        AddBFImgToFTable(1, (cell->modifier & 3) + 22, cell->column, cell->row, 10, 0);
        break;
    case 5:
        AddBFImgToBTable(1, (cell->modifier & 3) + 22, cell->column, cell->row, 10, 0);
        DrawRuinFlaw(cell);
        break;
    }
}

void DrawRuinGateMovable(RuinCell *cell);

void DrawRuinGate(RuinCell *cell)
{
    AddImage add_image = 0;

    switch (cell->pass) {
    case 0:
        DrawRuinBackWall(cell);
        break;
    case 1:
        add_image = AddFImgToFTable;
        break;
    case 2:
        add_image = AddBFImgToFTable;
        break;
    case 5:
        add_image = AddBFImgToBTable;
        break;
    }
    if (add_image) {
        add_image(4, -2, cell->column, cell->row, 10, 0);
        DrawRuinGateMovable(cell);
        DrawRuinFlaw(cell);
    }
}

extern LEVELREC Level;
extern void DrawThroneRoomGateInFore(void);

void DrawRuinGateMovable(RuinCell *cell)
{
    AddImage add_image;
    Rect gate_clip, saved_clip;

    saved_clip = CellClipRect;
    gate_clip = GamePlayRect;
    gate_clip.top = cell->row * 63 - 10;
    if (SECTRECT(&CellClipRect, &CellClipRect, &gate_clip)) {
        unsigned int height = (cell->modifier & 0xfc) >> 2;
        Pieces->gate_y = -(4 + (height < 50 ? height : 50));
        switch (cell->pass) {
        case 2:
            add_image = AddBFImgToFTable;
            break;
        case 5:
            add_image = AddBFImgToBTable;
            break;
        default:
            add_image = 0;
            break;
        }
        if (add_image) {
            if (Room.number == 9 && (Level.level_number) == 8) {
                if (cell->pass == 2)
                    DrawThroneRoomGateInFore();
            } else {
                add_image(55, -2, cell->column, cell->row, 10, 0);
            }
        }
    }
    CellClipRect = saved_clip;
}

void DrawRuinLoose(RuinCell *cell)
{
    int image;
    RuinCell floor;
    image = cell->modifier & 15;
    if (image != 13)
        image = cell->column % 2 ? ruin_tables.words[LOOSE_ODD_WORD + image]
                                 : ruin_tables.words[LOOSE_EVEN_WORD + image];
    if (!image) {
        floor = *cell;
        floor.modifier &= ~3;
    }
    switch (cell->pass) {
    case 0:
        if (image != 13) {
            if (image)
                DrawRuinBackWall(cell);
            else
                DrawRuinFloor(&floor);
        } else {
            DrawRuinBackWall(cell);
        }
        break;
    case 1: {
        RuinCell *tile = cell;
        if (image == 13) {
            DrawRuinFlaw(tile);
            break;
        }
        if (!image) {
            DrawRuinFloor(&floor);
            break;
        }
        AddFImgToFTable(11, image + 43, tile->column, tile->row, 10, 0);
        DrawRuinFlaw(tile);
        break;
    }
    case 2:
        if (image != 13) {
            if (image) {
                RuinCell *tile = cell;
                AddBFImgToFTable(11, image + 41, tile->column, tile->row, 10, 0);
            } else {
                DrawRuinFloor(&floor);
            }
        }
        break;
    case 5: {
        RuinCell *tile = cell;
        if (image == 13) {
            DrawRuinFlaw(tile);
            break;
        }
        if (!image) {
            DrawRuinFloor(&floor);
            break;
        }
        AddBFImgToBTable(11, image + 41, tile->column, tile->row, 10, 0);
        DrawRuinFlaw(tile);
        break;
    }
    }
}

void DrawRuinPillarBot(RuinCell *cell)
{
    AddImage add_image = 0;

    switch (cell->pass) {
    case 0:
        DrawRuinBackWall(cell);
        add_image = AddBImgToBTable;
        break;
    case 1:
        add_image = AddFImgToFTable;
        break;
    }
    if (add_image) {
        add_image(8, -2, cell->column, cell->row, 10, 0);
        DrawRuinFlaw(cell);
    }
}

void DrawRuinPillarTop(RuinCell *cell)
{
    AddImage add_image;

    switch (cell->pass) {
    case 0:
        DrawRuinBackWall(cell);
        add_image = AddBImgToBTable;
        break;
    case 1:
        add_image = AddFImgToFTable;
        break;
    default:
        add_image = 0;
        break;
    }
    if (add_image) {
        add_image(9, -2, cell->column, cell->row, 10, 0);
        DrawRuinFlaw(cell);
    }
}

void DrawRuinDownPlate(RuinCell *cell)
{
    byte layer;
    int image;
    AddImage add_image = 0;
    byte tile = 5 + ((cell->modifier & 0x800) != 0);

    switch (cell->pass) {
    case 0:
        DrawRuinBackWall(cell);
        break;
    case 1:
        image = 52;
        layer = 0;
        add_image = AddFImgToFTable;
        break;
    case 5:
        add_image = AddBFImgToBTable;
        /* Shared back and front plate image. */
    case 2:
        if (!add_image)
            add_image = AddBFImgToFTable;
        image = 51;
        layer = 10;
        break;
    }
    if (add_image) {
        add_image(tile, image, cell->column, cell->row, layer, 0);
    }
}

void DrawRuinUpPlate(RuinCell *cell)
{
    byte layer;
    int image;
    AddImage add_image = 0;
    byte tile = 5 + ((cell->modifier & 0x800) != 0);

    switch (cell->pass) {
    case 0:
        DrawRuinBackWall(cell);
        break;
    case 1:
        add_image = AddFImgToFTable;
        image = 50;
        layer = 0;
        break;
    case 5:
        add_image = AddBFImgToBTable;
        /* Shared back and front plate image. */
    case 2:
        if (!add_image)
            add_image = AddBFImgToFTable;
        image = 49;
        layer = 10;
        break;
    }
    if (add_image) {
        add_image(tile, image, cell->column, cell->row, layer, 0);
    }
}

void DrawRuinPosts(RuinCell *cell)
{
    AddImage add_image = 0;
    int image = cell->modifier & 3;

    switch (cell->pass) {
    case 0:
        DrawRuinBackWall(cell);
        break;
    case 1:
        add_image = AddFImgToFTable;
        image += 39;
        break;
    case 2:
        add_image = AddBFImgToFTable;
        image += 36;
        break;
    case 5:
        add_image = AddBFImgToBTable;
        image += 36;
        break;
    }
    if (add_image) {
        add_image(3, image, cell->column, cell->row, 10, 0);
        DrawRuinFlaw(cell);
    }
}

void DrawRuinRubblePieces(RuinCell *cell);

void DrawRuinRubble(RuinCell *cell)
{
    RuinCell floor;
    unsigned int flags;

    if (cell->modifier & 0x40) {
        floor = *cell;
        floor.modifier = (flags = cell->modifier & ~0x40);
        if (flags & 4) {
            DrawRuinUpPlate(&floor);
            cell->modifier &= ~0x800;
        } else if (flags & 8) {
            DrawRuinDownPlate(&floor);
            cell->modifier &= ~0x800;
        } else {
            DrawRuinFloor(&floor);
        }
    } else if (cell->pass == 0) {
        DrawRuinBackWall(cell);
    }
    DrawRuinRubblePieces(cell);
    DrawRuinFlaw(cell);
}

void DrawRuinRubblePieces(RuinCell *cell)
{
    AddImage add_image = 0;
    int image = (cell->modifier & 0x30) >> 4;

    switch (cell->pass) {
    case 1:
        image += 33;
        add_image = AddFImgToFTable;
        break;
    case 2:
        image += 30;
        add_image = AddBFImgToFTable;
        break;
    case 5:
        image += 30;
        add_image = AddBFImgToBTable;
        break;
    }
    if (add_image) {
        add_image(14, image, cell->column, cell->row, 10, 0);
        DrawRuinFlaw(cell);
    }
}

extern byte ObjId;
extern int VisibleAbovePiece(sbyte, sbyte);
extern int IsSpace(byte);
extern int Pause;

void DrawRuinSpace(RuinCell *cell)
{
    int back_wall;

    if (cell->pass == 0) {
        back_wall = 0;
        if (cell->row == RowNo)
            back_wall = 1;
        else if (cell->row - RowNo == 1)
            back_wall = VisibleAbovePiece(cell->column, cell->row);
        if (!back_wall && RowNo == -1 && IsSpace(ObjId))
            back_wall = 1;
        if (back_wall)
            DrawRuinBackWall(cell);
    }
    DrawRuinFlaw(cell);
}

void LoadRuinsDrawCode(void)
{
    Pause = 0;
}

extern CELLREC Cell;
extern CHARREC Char;
extern byte ReadCell(sbyte, sbyte, sbyte);
extern byte GetCellsInFront(byte);
extern int IsVisibleRoomCell(sbyte, sbyte);
extern void MarkCellRedraw(void);

void RuinSetCreviceRevealed(int reveal);

void RuinChangeCrevices(int reveal, sbyte room, sbyte column, sbyte row)
{
    sbyte stop;
    byte tile;

    stop = column + 3 >= 10 ? column + 3 : 10;
    column = column - 2 <= 0 ? column - 2 : 0;
    do {
        tile = ReadCell(room, column, row);
    } while (stop != ++column && tile != 7);
    if (tile == 7) {
        while (ReadCell((Cell.room), Cell.column - 1, Cell.row) == 7)
            ;
        ReadCell((Cell.room), Cell.column + 1, Cell.row);
        do
            RuinSetCreviceRevealed(reveal);
        while (ReadCell((Cell.room), Cell.column + 1, Cell.row) == 7);
    }
}

void RuinCheckCrevices(void)
{
    if (Char.sequence != 17 && GetCellsInFront(1) == 7 && (Cell.modifier & 3) != 3)
        RuinChangeCrevices(1, Char.room, Char.column, Char.row);
}

void RuinSetCreviceRevealed(int reveal)
{
    int changed;
    unsigned int flags;

    flags = ((LEVELCELL *)Level.cell_info)[Cell.room * 30 + Cell.index - 30].modifier;
    if (reveal) {
        changed = (flags & 0x80) == 0;
        if (changed)
            flags |= 0x80;
    } else {
        changed = flags & 0x80;
        if (changed)
            flags &= ~0x80;
    }
    if (changed) {
        if (IsVisibleRoomCell((Cell.room), Cell.index))
            MarkCellRedraw();
        if (!(flags & 3) && Cell.column != 9) {
            ReadCell((Cell.room), Cell.column + 1, Cell.row);
            MarkCellRedraw();
            ReadCell((Cell.room), Cell.column - 1, Cell.row);
        }
        ((LEVELCELL *)Level.cell_info)[Cell.room * 30 + Cell.index - 30].modifier = flags;
    }
}

extern void far pascal DrawFlask(RuinCell near *);
extern void far pascal DrawScythe1(RuinCell near *);
extern void far pascal DrawScythe2(RuinCell near *);
extern void far pascal DrawSword(RuinCell near *);

typedef void (far pascal *LevelTileProc)(RuinCell near *);
LevelTileProc DrawRuinFunc[] = {
    DrawRuinSpace, DrawRuinFloor, 0, DrawRuinPosts, DrawRuinGate,
    DrawRuinUpPlate, DrawRuinDownPlate, DrawRuinCrevice, DrawRuinPillarBot, DrawRuinPillarTop,
    DrawFlask, DrawRuinLoose, DrawScythe1, DrawScythe2, DrawRuinRubble,
    DrawRuinFloor, DrawRuinExit1, DrawRuinExit2, 0, 0,
    DrawRuinBlock, DrawRuinBones, DrawSword
};
