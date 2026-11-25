#include <string.h>
#include <stdio.h>
#include "types.h"
#include "resc.h"
#include "system.h"
#include "level.h"
#include "shape.h"
#include "graf.h"
#include "heap.h"
#include "char.h"
#include "game.h"
#include "draw.h"
#include "custcvca.h"
#include "auto.h"
#include "add.h"
#include "ctrl.h"
#include "custom.h"
#include "desert.h"
#include "bridge.h"
#include "drawcave.h"
#include "drawtemp.h"
#include "fade.h"
#include "movescyt.h"
#include "english.h"
#include "anim.h"
#include "act.h"
#include "altar.h"
#include "misc.h"
#include "drawruin.h"
#include "gfxerr.h"
#include "final.h"
#include "init.h"
#include "lgraf.h"
#include "intrface.h"
#include "move.h"
#include "movespik.h"
#include "movetemp.h"
#include "prince.h"
#include "top.h"
#include "rooftops.h"
#include "tables.h"
#include "xlatgraf.h"

SHAPEINFO HorseShpInfo;
int Ct[5];
DRAWPROC *DrawFunc;
DRAWPROC DrawGateMoveFunc;
ROOMREC Room;
LEVELCELL BlueSpec;
sbyte ObjId;
Rect CellClipRect;
TILE CellLeft;
TILE CellRight;
TILE ScrRight[3];
TILE ScrLeft[3];
TILE Above[11];
TILE Below[11];
int NewRoom;
sbyte RowNo;
FCHARREC FChar;
SHAPELIST *Pics[5];
sbyte ColNo;
int XCo;


void DoSure(void)
{
    ZeroPeels();
    memset(Ct, 0, sizeof Ct);
    init_fchar_rect();
    Sure();
    ZeroBuffers();
}

void GetRooms(void)
{
    Room.above_left = Room.above_right = Room.below_left = Room.below_right = 0;
    if (Room.number) {
        select_room_arrays(Room.number);
        Room.left = Level.room_links[Room.number - 1].left;
        Room.right = Level.room_links[Room.number - 1].right;
        Room.above = Level.room_links[Room.number - 1].above;
        Room.below = Level.room_links[Room.number - 1].below;

        if (Room.above) {
            Room.above_left = Level.room_links[Room.above - 1].left;
            Room.above_right = Level.room_links[Room.above - 1].right;
        } else {
            if (Room.left)
                Room.above_left = Level.room_links[Room.left - 1].above;
            if (Room.right)
                Room.above_right = Level.room_links[Room.right - 1].above;
        }
        if (Room.below) {
            Room.below_left = Level.room_links[Room.below - 1].left;
            Room.below_right = Level.room_links[Room.below - 1].right;
        } else {
            if (Room.left)
                Room.below_left = Level.room_links[Room.left - 1].below;
            if (Room.right)
                Room.below_right = Level.room_links[Room.right - 1].below;
        }
        Room.custom = IsCustomizedRoom(Room.number);
    } else {
        Room.left = Room.right = Room.above = Room.below = 0;
    }
}

void Sure(void)
{
    int saved_room;

    SetSideRoomInfo();
    RowNo = 3;
    while (RowNo--) {
        GetAboveAry();
        GetBelowAry();
        for (ColNo = 0; ColNo < 10; ++ColNo) {
            SetLocVars();
            RedrawCellSure();
        }
    }
    if (Room.custom == 0) {
        saved_room = Room.number;
        Room.number = Room.above;
        GetRooms();
        Room.below = saved_room;
        SetSideRoomInfo();
        RowNo = 2;
        GetAboveAry();
        GetBelowAry();
        RowNo = -1;
        for (ColNo = 0; ColNo < 10; ++ColNo) {
            SetLocVars();
            RedrawCellSure();
        }
        Room.number = saved_room;
        GetRooms();
    } else {
        GetCustomAboveAry();
    }
}


void RedrawCellSure(void)
{
    DrawBackImg(0);
    DrawBackImg(5);
    DrawBackImg(11);
    DrawForeImg();
}


#define DIRTY_CELL(table, byte_offset) \
    (*(DIRTYCELL *)((char *)(table) + (byte_offset)))
#define WORD_FLAG(table, byte_offset) \
    (*(int *)((char *)(table) + (byte_offset)))

void RedrawCellFast(sbyte column)
{
    Rect old_clip;

    if (RedrawBuf[column]) {
        RedrawBuf[column] = 0;
        RedrawCellSure();
    } else if (MoveBuf[column].dirty) {
        MoveBuf[column].dirty = 0;
        old_clip = CellClipRect;
        if (SectRect(&CellClipRect, &CellClipRect, &MoveBuf[column].rect)) {
            DrawBackImg(0);
            DrawBackImg(5);
            DrawBackImg(11);
            DrawForeImg();
        }
        CellClipRect = old_clip;
    }
    if (FloorBuf[column].dirty) {
        FloorBuf[column].dirty = 0;
        if (HalfBuf[column] && ComputeHalfRect(&old_clip))
            UnionRect(&FloorBuf[column].rect, &FloorBuf[column].rect, &old_clip);
        old_clip = CellClipRect;
        if (SectRect(&CellClipRect, &CellClipRect, &FloorBuf[column].rect))
            DrawFloorBuf();
        CellClipRect = old_clip;
    } else if (HalfBuf[column]) {
        HalfBuf[column] = 0;
        DrawHalf();
    }
    if (ObjBuf[column]) {
        DrawObjs(column);
        ObjBuf[column] = 0;
    }
    if (FRedrawBuf[column].dirty) {
        FRedrawBuf[column].dirty = 0;
        if (SectRect(&CellClipRect, &CellClipRect, &FRedrawBuf[column].rect))
            DrawForeImg();
    }
}

void RedrawTopFast(sbyte column)
{
    sbyte old_row;
    Rect old_clip;

    if (TopBuf[column].dirty) {
        TopBuf[column].dirty = 0;
        old_row = RowNo;
        old_clip = CellClipRect;
        CellClipRect = TopBuf[column].rect;
        RowNo = -1;
        DrawBackImg(0);
        DrawBackImg(5);
        DrawBackImg(11);
        DrawForeImg();
        RowNo = old_row;
        CellClipRect = old_clip;
    }
}


void DrawBackImg(byte pass)
{
    byte layer;
    struct { unsigned long address; DRAWCELL args; } draw;

    layer = GetBackImgStartPass();
    if (NewRoom)
        CellClipRect = GamePlayRect;
    draw.args.pass = pass;
    do {
        byte priority;
        switch (layer) {
        case 0:
            draw.args.column = ColNo - 1;
            draw.args.row = RowNo + 1;
            draw.args.kind = Below[ColNo].kind;
            draw.args.info = Below[ColNo].info;
            if (!NewRoom || ColNo == 0 || ColNo == 9 ||
                Level.environment_kind == 3 || draw.args.kind == 4 || Level.level_number == 13)
                break;
            if (Level.environment_kind == 2 && (draw.args.kind == 19 || draw.args.kind == 9))
                break;
            layer += 2;
            break;
        case 1:
            draw.args.row = RowNo + 1;
            draw.args.column = ColNo;
            draw.args.kind = Below[ColNo + 1].kind;
            draw.args.info = Below[ColNo + 1].info;
            if (!IsWall(draw.args.kind))
                break;
            ++layer;
            /* fall through */
        case 2:
            draw.args.column = ColNo - 1;
            draw.args.row = RowNo;
            draw.args.kind = CellLeft.kind;
            draw.args.info = CellLeft.info;
            break;
        case 3:
            DrawCustomRoomImgs(pass);
            draw.args.column = ColNo;
            draw.args.row = RowNo;
            draw.args.kind = ObjId;
            draw.args.info = BlueSpec;
            break;
        }
        priority = (draw.args.info.modifier >> 14) & 3;
        if (!CustomRoom || priority == 0 || pass == priority || (priority == 2 && pass == 5)) {
            draw.address = (unsigned long)DrawFunc[draw.args.kind];
            if (draw.address)
                ((DRAWPROC)draw.address)(&draw.args);
        }
    } while (++layer < 4);
}

byte GetBackImgStartPass(void)
{
    byte pass;
    int custom_edge;

    custom_edge = NewRoom &&
        ((ObjId != 7 && IsWall(ObjId) &&
          CellLeft.kind != 7 && IsWall(CellLeft.kind)) ||
         (ColNo != 0 && RowNo != 2));
    if (custom_edge)
        pass = 3;
    else if ((NewRoom && ColNo == 0 && RowNo != 2) ||
             (CustomRoom && RowNo == 2 && !NeedDrawRoomBelowCustom()))
        pass = 2;
    else
        pass = 0;
    return pass;
}

void DrawCustomRoomImgs(sbyte pass)
{
    sbyte image;
    word sheet;
    int draw_images = 0;
    const POINT *origin;

    if ((pass == 0 || pass == 5) && NewRoom &&
        ColNo == 0 && RowNo == 2) {
        draw_images = 1;
        if (Room.number == 1 && Level.level_number == 14)
            LoadTowerTop();
    } else if ((pass == 0 && !NewRoom && CustomRoom &&
                CustomRoom->background_shape) ||
               ((pass == 1 || pass == 2) &&
                ((ColNo == 9 && RowNo == 0) || !NewRoom))) {
        draw_images = 1;
    }
    if (draw_images) {
        if (CustomRoom) {
            image = 0;
            if (CustomRoom->count > 0) {
                do {
                    DrawCustomRoomImg(image, pass);
                    ++image;
                } while (CustomRoom->count > image);
            }
        }
        if ((NewRoom && pass == 0) || pass == 1) {
            origin = GetHorsePtInPlane(Room.number, pass, &sheet);
            if (origin)
                DrawHorse(sheet, origin, pass);
        }
    }
}


void _fastcall DrawCustomRoomImg(sbyte index, sbyte pass)
{
    CUSTOMPIECE *image;
    Rect clip;
    BFITEM far *entry;

    image = &CustomRoom->pieces[index];
    if (image->image == -1 || image->pass != pass ||
        !SectRect(&clip, &image->clip, &CellClipRect))
        return;
    if (pass != 0 && pass != 5 && pass != 11) {
        entry = &ForeTable[Ct[1]++];
        if (Level.environment_kind == 5 &&
            ((Level.environment_kind == 5 && Room.number != 19 && Room.number != 16 &&
              Room.number != 15 && DrawRoofLedgeInBack(image)) ||
             ((Room.number == 19 || Room.number == 16) && DrawPillarInBack(image)))) {
            entry = &BackTable[Ct[0]++];
            --Ct[1];
        }
    } else {
        if (!image->handle)
            return;
        entry = &BackTable[Ct[0]++];
    }
    entry->pos.y = image->pos.y;
    entry->pos.x = image->pos.x;
    entry->pos.image = image->image;
    entry->image_set = 4;
    entry->column = ColNo;
    entry->row = RowNo;
    entry->clip = clip;
    entry->blit = image->blit;
    entry->peel = image->peel;
}

void DrawForeImg(void)
{
    byte layer, priority;
    int skip_left;
    struct { unsigned long address; DRAWCELL args; } draw;

    CheckGateInFore();
    if (NewRoom && ColNo)
        skip_left = 1;
    else
        skip_left = 0;
    layer = skip_left != 0;
    draw.args.pass = 1;
    do {
        switch (layer) {
        case 0:
            draw.args.column = ColNo - 1;
            draw.args.row = RowNo;
            draw.args.kind = CellLeft.kind;
            draw.args.info = CellLeft.info;
            break;
        case 1:
            draw.args.column = ColNo;
            draw.args.row = RowNo;
            draw.args.kind = ObjId;
            draw.args.info = BlueSpec;
            if (skip_left && Level.environment_kind != 3)
                ++layer;
            break;
        case 2:
            draw.args.row = RowNo - 1;
            draw.args.column = ColNo;
            draw.args.kind = Above[ColNo + 1].kind;
            draw.args.info = Above[ColNo + 1].info;
            break;
        }
        priority = (draw.args.info.modifier >> 14) & 3;
        if (!CustomRoom || !priority || priority == 1 || priority == 2) {
            draw.address = (unsigned long)DrawFunc[draw.args.kind];
            if (draw.address)
                ((DRAWPROC)draw.address)(&draw.args);
        }
    } while (++layer < 3 && (layer < 2 || RowNo != -1));
    DrawCustomRoomImgs(1);
}


void CheckGateInFore(void)
{
    sbyte count, index;
    int found;
    DRAWCELL args;
    Rect saved_clip;
    OBJITEM *image;

    if (Kid.frame >= 264 && Kid.frame <= 266) {
        if (Level.environment_kind == 3) {
            args.pass = 0;
            args.row = RowNo;
            if (ObjId == 4) {
                args.column = ColNo;
                args.info = BlueSpec;
                DrawCaveGateSquishing(&args);
            } else if (CellLeft.kind == 4) {
                args.column = ColNo - 1;
                args.info = CellLeft.info;
                DrawCaveGateSquishing(&args);
            }
        }
        return;
    }
    if (CellLeft.kind != 4)
        return;
    saved_clip = CellClipRect;
    LoadKid();
    found = CellLeft.kind == 4 && Char.row == RowNo &&
            (Char.column - ColNo == -1 || ColNo - Char.column == 2) &&
            Char.action != 2 && Char.action != 6;
    if (!found) {
        count = Level.room_objects[Room.number - 1].opponent_count;
        for (index = 0; !found && index < count; ++index) {
            LoadShad(index);
            found = CellLeft.kind == 4 && Char.row == RowNo &&
                    (Char.column - ColNo == -1 || ColNo - Char.column == 2) &&
                    Char.action != 2 && Char.action != 6;
        }
        if (found)
            image = GetObjTableType(3, Char.character_kind);
    } else
        image = GetObjTableType(2, 0);
    if (found) {
        args.pass = 2;
        args.column = ColNo - 1;
        args.row = RowNo;
        args.info = CellLeft.info;
        if (DrawGateMoveFunc) {
            if (NewRoom)
                CellClipRect.top = 63 * args.row + 3;
            else if (image)
                SectRect(&CellClipRect, &CellClipRect, &image->clip);
            if (!EmptyRect(&CellClipRect))
                DrawGateMoveFunc(&args);
        }
    }
    CellClipRect = saved_clip;
}


void DumpTable(byte pass)
{
    byte index, count;
    int prepared;

    if (!NewRoom || !CustomRoom)
        prepared = 1;
    else
        prepared = 0;
    index = 0;
    count = Ct[pass];
    if (count) {
        do {
            if (pass == 3) {
                DrawMidItem(index);
            } else {
                if (!prepared && BackTable[index].peel) {
                    SaveCustomRoomPeels();
                    prepared = 1;
                }
                DrawBFItem(pass, index);
            }
        } while (++index < count);
    }
}


void DrawBFItem(byte pass, byte index)
{
    BFITEM far *entry;
    SHAPE far *image;
    SHAPE far *mask;
    CUSTOMPIECE *custom;
    SHAPEINFO ref;
    int mode;
    word flags;

    if (pass == 0)
        entry = &BackTable[index];
    else
        entry = &ForeTable[index];
    mode = 2;
    flags = 0;
    custom = 0;
    if (Pics[entry->image_set]->count >= entry->pos.image) {
        if (entry->image_set == 1) {
            mode = 3;
            flags = 0x4000;
        } else if (entry->image_set == 0) {
            mode = 3;
            flags = 0x8000;
        }
        GetSetShapeItem(Pics[entry->image_set], entry->pos.image, &ref);
        image = h2p(ref.image);
        if (!image)
            return;
    } else {
        if (!CustomRoom && (entry->pos.image < 0x6370 || entry->pos.image > 0x6372))
            return;
        if (CustomRoom) {
            custom = &CustomRoom->pieces[entry->pos.image - CustomRoom->first_shape];
            if (custom->peel == 1)
                AddCustomPiecePeel(custom, &entry->clip);
        }
        if (entry->pos.image >= 0x6370 && entry->pos.image <= 0x6372) {
            image = GetHorseShpInfo(entry->pos.image, &ref);
            entry->pos.image = 0;
        } else if (entry->pos.image >= 0x62d7 && entry->pos.image <= 0x62e2) {
            SaveUnderCarpet();
            image = GetCarpetShpInfo(&ref, entry->pos.image - 0x62d7);
            ref.mirrored = entry->mirrored;
        } else if (CustomRoom) {
            image = GetCustomPieceShape(custom);
            ref.mirrored = entry->mirrored;
        }
        ref.four_bit = 0;
    }
    if (entry->mirrored != ref.mirrored) {
        if (ref.four_bit)
            MirrorMCGABlock(image->pixels, image->height, image->format);
        else if (ref.mode == 2)
            Mirror4BitBlock(image->pixels, image->height, image->width, image->format);
        else if (ref.mode == 1)
            MirrorRLEBlock(image->pixels, image->height, image->format);
        if (entry->pos.image < 0x6370 || entry->pos.image > 0x6372)
            ((byte *)Pics[entry->image_set])[6 * entry->pos.image + 3] = entry->mirrored;
    }
    if (ref.four_bit) {
        mask = image;
        image = 0;
    } else {
        mask = 0;
    }
    ClipRect(&entry->clip);
    DrawIt(image, mask, entry->pos.x, entry->pos.y, entry->blit, mode, flags);
    ClipRestore();
    if (Pics[entry->image_set]->count > entry->pos.image)
        FreeShpInfo(&ref);
    else if (custom && custom->pass == 0 && custom->mirrored == 0)
        hPurge(custom->handle);
}

void DrawMidItem(int index)
{
    SHAPEINFO ref;
    SHAPE far *image;
    sbyte type;
    word mark, flags;
    int mode, flip, x;
    struct { int width, height; } peel;

    type = MidTable[index].image_set;
    if (MidTable[index].image >= 0x62d7 && MidTable[index].image <= 0x62e2) {
        GetCarpetShpInfo(&ref, MidTable[index].image - 0x62d7);
    } else if (Level.level_number == 5 && Room.number == 10 && type == 4 &&
               MidTable[index].image < CustomRoom->count) {
        GetCustomPieceShpInfo(MidTable[index].image, &ref);
    } else {
        if (type == 2 && MidTable[index].variant)
            TweakKidShpList(MidTable[index].variant);
        GetSetShapeItem(Pics[type], MidTable[index].image + 1, &ref);
        if (type == 2 && MidTable[index].variant)
            TweakKidShpList(2);
    }
    flip = MidTable[index].blit;
    mark = hGetState(ref.image);
    image = hLock(ref.image);
    x = MidTable[index].x;
    if (MidTable[index].mirrored)
        x -= image->width;
    if (type == 2) {
        flags = 0;
        mode = 1;
    } else if (type == 4) {
        flags = 0;
        mode = 2;
    } else {
        if (type == 0)
            flags = 0x8000;
        else if (type == 1)
            flags = 0x4000;
        else
            flags = MidTable[index].variant;
        mode = 3;
    }
    if (MidTable[index].mirrored != ref.mirrored) {
        if (mode == 1)
            MirrorMCGABlock(image->pixels, image->height, image->format);
        else if (mode == 2)
            MirrorRLEBlock(image->pixels, image->height, image->format);
        else
            Mirror4BitBlock(image->pixels, image->height, image->width, image->format);
        ((byte *)Pics[type])[6 * MidTable[index].image + 9] =
            MidTable[index].mirrored;
    }
    peel.height = x + image->width;
    if (peel.height > MidTable[index].clip.right)
        peel.height = MidTable[index].clip.right;
    peel.width = MidTable[index].clip.bottom - MidTable[index].y;
    if (peel.width > image->height)
        peel.width = image->height;
    AddPeel(x, peel.height, MidTable[index].y, peel.width, MidTable[index].owner, 0);
    ClipRect(&MidTable[index].clip);
    DrawIt(image, 0, x, MidTable[index].y, flip, mode, flags);
    ClipRestore();
    hSetState(ref.image, mark);
    FreeShpInfo(&ref);
}


static int dirty_rectangle_count;
static struct {
    byte far *saved_palette;
    sbyte sorted_objects[26];
    byte far *saved_text_palette;
} draw_scratch;
static const Rect half_tile_rectangles[10] = {
    {57, 0, 63, 16},
    {57, 0, 63, 16},
    {57, 0, 63, 17},
    {57, 0, 63, 20},
    {57, 0, 63, 15},
    {57, 0, 63, 16},
    {57, 0, 63, 19},
    {59, 0, 63, 14},
    {59, 0, 63, 13},
    {59, 0, 63, 13},
};
static const POINT horse_level9_origin = {22, 159};
static const POINT horse_level10_origin = {102, 119};
POINT horse_level14_origin = {37, 112};


DRAWPROC *LevelDrawFuncs[] = {
    0, DrawDsrtFunc, DrawTempFunc,
    DrawCaveFunc, DrawRuinFunc,
    DrawRoofFunc, DrawFinlFunc
};

DRAWPROC LevelDrawGateMoveFuncs[] = {
    0, 0, DrawTempGateMovable, DrawCaveGateMovable,
    DrawRuinGateMovable, 0, 0
};

static struct {
    char missing_shape[63];
    char rectangle_overflow[16];
} draw_messages = {
    "ERROR:  DrawIt().  x = %d, y = %d, mode = 0x%x, shapeType = %d",
    "DRects Overflow"
};


void DrawIt(SHAPE far *image, SHAPE far *mask, int x, int y,
                 int flip, int mode, word flags)
{
    typedef struct BitmapSize {
        int width, height;
    } BitmapSize;
    Rect area;
    BitmapSize size;
    char message[150];

    if (!image && !mask) {
        sprintf(message, draw_messages.missing_shape, x, y, flip, mode);
        abort_with_message(1, message);
    }
    if (image) {
        switch (flip) {
        case 0:
        case 10:
            if (mode == 2) {
                PaintRLEShape_asm(image, x, y, flip);
                break;
            }
            if (mode == 3) {
                paint_indexed_shape(image, x, y, flip, flags);
                break;
            }
        case 2:
        case 3:
        case 8:
            PaintShape(image, x, y, flip);
            break;
        }
        size = *(BitmapSize far *)image;
    } else {
        Paint4BitShape(mask, x, y, 0, flip);
        size = *(BitmapSize far *)mask;
    }
    if (!NewRoom) {
        area.right = area.left = x;
        area.right += size.height;
        area.bottom = area.top = y;
        area.bottom += size.width;
        if (SectRect(&area, &area, &Graf->clip))
            AddDRect(&area);
    }
}


void DoFast(void)
{
    AddMobs();
    AddChars();
    MarkPeelForeBufs();
    Fast();
}

void Fast(void)
{
    byte saved_room;

    SetSideRoomInfo();
    DrawObjs(30);
    RowNo = 3;
    while (RowNo--) {
        GetAboveAry();
        GetBelowAry();
        for (ColNo = 0; ColNo < 10; ++ColNo) {
            SetLocVars();
            RedrawCellFast((sbyte)(FirstInRow(RowNo) + ColNo));
        }
    }
    if (!Room.custom) {
        saved_room = Room.number;
        Room.number = Room.above;
        GetRooms();
        SetSideRoomInfo();
        RowNo = 2;
        GetAboveAry();
        GetBelowAry();
        for (ColNo = 0; ColNo < 10; ++ColNo) {
            SetLocVars();
            RedrawTopFast(ColNo);
        }
        Room.number = saved_room;
        GetRooms();
    } else {
        GetCustomAboveAry();
    }
    DrawObjs(-1);
}


static Rect dirty_rectangles[40];


void DrawAll(void)
{
    dirty_rectangle_count = 0;
    Graf = BPort;
    if (BlackOut)
        FillRect(&GamePlayRect, 12);
    if (Level.environment_kind == 4 || Level.environment_kind == 2 || Level.environment_kind == 5)
        SortBackTable();
    if ((Room.number == 1 && Level.level_number == 14) ||
        (Room.number == 22 && Level.level_number == 10))
        SortForeTable();
    Peel();
    DumpTable(0);
    DumpTable(3);
    DumpTable(1);
    Graf = &ScreenPort;
}

void AddDRect(Rect *rect)
{
    Rect expanded;
    int index;

    for (index = 0; index < dirty_rectangle_count; index++) {
        InsetRect(&expanded, rect, -1, -1);
        if (SectRect(&expanded, &expanded, &dirty_rectangles[index])) {
            UnionRect(&dirty_rectangles[index], &dirty_rectangles[index], rect);
            return;
        }
    }
    if (dirty_rectangle_count >= 40)
        show_message(draw_messages.rectangle_overflow, 1);
    else
        dirty_rectangles[dirty_rectangle_count++] = *rect;
}


void DrawHalf(void)
{
    byte priority;
    Rect area;
    struct {
        unsigned long address;
        Rect old_clip, row_clip;
        DRAWCELL args;
    } draw;

    if (!ComputeHalfRect(&area))
        return;
    if (Level.environment_kind == 2 && ComputeSlideRockRect(&draw.row_clip, Room.number, RowNo) &&
        SectRect(&draw.row_clip, &draw.row_clip, &area))
        area.top = draw.row_clip.bottom - 1;
    draw.old_clip = CellClipRect;
    CellClipRect = area;
    draw.args.pass = 2;
    draw.args.column = ColNo;
    draw.args.row = RowNo;
    draw.args.info = BlueSpec;
    priority = (draw.args.info.modifier >> 14) & 3;
    draw.address = (unsigned long)DrawFunc[(byte)ObjId];
    if (draw.address && (!Room.custom || priority == 2 || priority == 0)) {
        ((DRAWPROC)draw.address)(&draw.args);
        MarkFRedraw((sbyte)(FirstInRow(RowNo) + ColNo), &area);
    }
    if (CustomRoom && CustomRoom->floor_image)
        DrawCustomRoomImgs(2);
    CellClipRect = draw.old_clip;
}


int ComputeHalfRect(Rect *area)
{
    word image;
    sbyte index;
    Rect source;

    if (Kid.frame >= 135 && Kid.frame < 145) {
        image = Kid.frame;
    } else {
        image = 0;
        for (index = 0; index < Level.room_objects[Char.room - 1].opponent_count && !image; ++index) {
            if (RoomOpp[index].character_kind == 1 &&
                RoomOpp[index].frame >= 135 &&
                RoomOpp[index].frame < 145)
                image = RoomOpp[index].frame;
        }
    }
    if (image) {
        source = half_tile_rectangles[image - 135];
        if (ObjId == 6)
            ++source.bottom;
        if ((Char.room == 7 || Char.room == 8) && Level.environment_kind == 6)
            source.top += 3;
        area = ComputeOffsetCellRect(area, &source, ColNo, RowNo);
    }
    return area;
}


void DrawFloorBuf(void)
{
    Rect old_clip, row_clip;
    DRAWCELL args;
    byte priority;
    int floor_edge, floor_y;

    old_clip = CellClipRect;
    floor_y = 63 * RowNo + 53;
    if (floor_y > CellClipRect.top)
        CellClipRect.top = floor_y;
    floor_edge = 0;
    if (Level.environment_kind == 2) {
        if (ComputeSlideRockRect(&row_clip, Room.number, RowNo) &&
            SectRect(&row_clip, &row_clip, &CellClipRect)) {
            CellClipRect.top = row_clip.bottom - 1;
            floor_edge = 1;
        } else if (ComputeSlideRockRect(&row_clip, Room.number, (sbyte)(RowNo + 1)) &&
                   SectRect(&row_clip, &row_clip, &CellClipRect)) {
            floor_edge = 1;
        }
    }
    if (IsSpace(CellLeft.kind)) {
        priority = (CellLeft.info.modifier >> 14) & 3;
        if (!Room.custom || priority == 2 || !priority) {
            if (DrawFunc[(byte)CellLeft.kind]) {
                args.pass = 2;
                args.column = ColNo - 1;
                args.row = RowNo;
                args.kind = CellLeft.kind;
                args.info = CellLeft.info;
                DrawFunc[(byte)CellLeft.kind](&args);
            }
        }
        priority = (BlueSpec.modifier >> 14) & 3;
        if (!Room.custom || priority == 2 || !priority) {
            if (DrawFunc[(byte)ObjId]) {
                args.pass = 2;
                args.column = ColNo;
                args.row = RowNo;
                args.kind = ObjId;
                args.info = BlueSpec;
                DrawFunc[(byte)ObjId](&args);
                MarkFRedraw((sbyte)(FirstInRow(RowNo) + ColNo), &CellClipRect);
            }
        }
        if (CustomRoom && CustomRoom->floor_image)
            DrawCustomRoomImgs(2);
    } else if (floor_edge && IsSpace(ObjId) && IsWall(CellLeft.kind)) {
        priority = (CellLeft.info.modifier >> 14) & 3;
        if (!Room.custom || priority == 2) {
            if (DrawFunc[(byte)CellLeft.kind]) {
                args.pass = 2;
                args.column = ColNo - 1;
                args.row = RowNo;
                args.kind = CellLeft.kind;
                args.info = CellLeft.info;
                DrawFunc[(byte)CellLeft.kind](&args);
            }
        }
    }
    CellClipRect = old_clip;
}


static int sorted_object_total;


void DrawObjs(sbyte column)
{
    sbyte index, count;
    Rect merged_clip, object_clip, original_clip;

    count = Ct[4];
    if (!count)
        return;
    original_clip = CellClipRect;
    merged_clip = CellClipRect;
    sorted_object_total = 0;
    if ((index = count - 1) >= 0) do {
        if (ObjTable[index].cell == column ||
            (ObjTable[index].cell >= 0 &&
             SectRect(&object_clip, &merged_clip, &ObjTable[index].clip))) {
            draw_scratch.sorted_objects[sorted_object_total++] = index;
            UnionRect(&object_clip, &merged_clip, &ObjTable[index].clip);
            if (!EqualRect(&object_clip, &merged_clip)) {
                index = count;
                sorted_object_total = 0;
                merged_clip = object_clip;
            }
        }
    } while (--index >= 0);
    if (sorted_object_total) {
        CellClipRect = merged_clip;
        SortObjTable();
        index = 0;
        if (sorted_object_total > 0) do {
            DrawObj(draw_scratch.sorted_objects[index]);
            ObjTable[draw_scratch.sorted_objects[index]].cell = -2;
        } while (++index < sorted_object_total);
    }
    CellClipRect = original_clip;
}


void SortObjTable(void)
{
    int swapped, index, last, value;

    last = sorted_object_total - 1;
    do {
        index = 0;
        swapped = 0;
        if (last > 0) {
            do {
                if (CompareObj(index, index + 1)) {
                    value = draw_scratch.sorted_objects[index];
                    draw_scratch.sorted_objects[index] = draw_scratch.sorted_objects[index + 1];
                    draw_scratch.sorted_objects[index + 1] = value;
                    swapped = 1;
                }
            } while (++index < last);
        }
    } while (swapped);
}

int CompareObj(int first, int second)
{
    int left_id, right_id;
    byte left_type, right_type;
    byte left_priority, right_priority;
    int swap;

    left_id = ObjTable[draw_scratch.sorted_objects[first]].frame;
    left_type = ObjTable[draw_scratch.sorted_objects[first]].kind;
    right_id = ObjTable[draw_scratch.sorted_objects[second]].frame;
    right_type = ObjTable[draw_scratch.sorted_objects[second]].kind;
    if (left_type == right_type) {
        if (ObjTable[draw_scratch.sorted_objects[first]].y != ObjTable[draw_scratch.sorted_objects[second]].y)
            swap = ObjTable[draw_scratch.sorted_objects[first]].y > ObjTable[draw_scratch.sorted_objects[second]].y;
        else
            swap = ObjTable[draw_scratch.sorted_objects[first]].x > ObjTable[draw_scratch.sorted_objects[second]].x;
    } else if (left_type != 7 && left_type != 8 &&
               (right_type == 7 || right_type == 8) &&
               (right_id == 181 || right_id == 166)) {
        swap = 1;
    } else if ((left_type == 7 || left_type == 8) &&
               right_type != 7 && right_type != 8 &&
               (left_id == 181 || left_id == 166)) {
        swap = 0;
    } else {
        left_priority = GetObjForePriority(left_type);
        right_priority = GetObjForePriority(right_type);
        if (left_priority != right_priority)
            swap = left_priority < right_priority;
        else if (ObjTable[draw_scratch.sorted_objects[first]].y != ObjTable[draw_scratch.sorted_objects[second]].y) {
            if (!(left_type & 0x80) && !(right_type & 0x80))
                swap = ObjTable[draw_scratch.sorted_objects[first]].y > ObjTable[draw_scratch.sorted_objects[second]].y;
            else
                swap = ObjTable[draw_scratch.sorted_objects[first]].y < ObjTable[draw_scratch.sorted_objects[second]].y;
        } else
            swap = ObjTable[draw_scratch.sorted_objects[first]].x > ObjTable[draw_scratch.sorted_objects[second]].x;
    }
    return swap;
}

byte _fastcall GetObjForePriority(byte type)
{
    byte result;

    if (type & 0x80)
        result = NonCharObjForePriority[type & 0x7f];
    else
        result = CharObjForePriority[type];
    return result;
}


OBJITEM *GetObjTableType(sbyte set, byte type)
{
    OBJITEM *match = 0;
    int index = 0;
    int total = Ct[4];
    byte *selector;

    if (total > 0) {
        selector = &ObjTable[0].image_set;
        do {
            if (match)
                break;
            if ((byte)set == *selector && selector[6] == type)
                match = (OBJITEM *)(selector - 4);
            selector += sizeof(OBJITEM);
        } while (++index < total);
    }
    return match;
}


void DrawObj(sbyte index)
{
    byte type = LoadObject(index);

    switch (type) {
    case 0:
        if (FChar.image != -1) {
            AddMid(FChar.image_set, FChar.image + 1,
                             XCo, FChar.y, 10);
            if (FChar.variant) {
                MidTable[Ct[3] - 1].variant = FChar.variant;
            }
        }
        break;
    case 1: case 2: case 3:
    case 5: case 6: case 7: case 8: case 9: case 10: case 14:
        if (AddMid(FChar.image_set, FChar.image + 1,
                             XCo, FChar.y, 10)) {
            MidTable[Ct[3] - 1].variant = FChar.variant;
            MidTable[Ct[3] - 1].owner = FChar.owner;
        }
        break;
    case 13:
        DrawObjCarpet(type);
        break;
    case 0x80: case 0x81: case 0x83:
        DrawObjLoose(type);
        break;
    case 0x82:
        AddMid(FChar.image_set, 103, XCo, FChar.y, 10);
        return;
    case 0x84: case 0x85:
        DrawObjSpikes(type);
        return;
    case 0x86: case 0x87: case 0x88: case 0x89:
        DrawObjSlideRock(type);
        return;
    case 0x8a:
        DrawObjTrapDoor(type);
        return;
    case 0x8b:
        DrawObjBrgFloor(type);
        return;
    case 0x8c:
        DrawObjFireball(type);
        break;
    }
}

void DrawObjLoose(byte type)
{
    int horizontal_gap, left_shape, right_shape, vertical_gap;

    switch (Level.environment_kind) {
    case 2:
        left_shape = 21;
        right_shape = 22;
        vertical_gap = horizontal_gap = 0;
        break;
    case 3:
        left_shape = (type & 127) + 66;
        right_shape = (type & 127) + 70;
        horizontal_gap = 1;
        vertical_gap = 4;
        FChar.clip.bottom += 5;
        break;
    case 4:
        left_shape = 22;
        right_shape = 26;
        vertical_gap = horizontal_gap = 0;
        break;
    default:
        left_shape = 0;
        break;
    }
    if (left_shape) {
        FChar.direction = -1;
        AddMid(4, left_shape, XCo + horizontal_gap,
                         FChar.y, 10);
        AddMid(4, right_shape, XCo,
                         vertical_gap + FChar.y, 10);
    }
}

byte LoadObject(sbyte index)
{
    OBJITEM *item = &ObjTable[index];

    FChar.cell = item->cell;
    XCo = FChar.x = item->x;
    FChar.y = item->y;
    FChar.image = item->image;
    FChar.image_set = item->image_set;
    FChar.direction = item->direction;
    *&FChar.clip = item->clip;
    FChar.variant = item->variant;
    FChar.owner = item->owner;
    return item->kind;
}

void SetObjCell(sbyte index)
{
    ObjTable[index].cell = FChar.cell;
    if (FChar.cell < 30)
        ObjBuf[FChar.cell] = 1;
}


void SortBackTable(void)
{
    int dest, source;
    BFITEM item;

    dest = source = 0;
    if (Ct[0] > 0) {
        do {
            if (!BackTable[source].peel) {
                if (source != dest) {
                    item = BackTable[source];
                    _fmemmove(&BackTable[dest + 1], &BackTable[dest],
                                    (source - dest) * sizeof(BFITEM));
                    BackTable[dest] = item;
                }
                ++dest;
            }
        } while (++source < Ct[0]);
    }
}

void SortForeTable(void)
{
    int dest, source, shape;
    BFITEM item;

    shape = 0;
    if (Level.environment_kind == 6) {
        if ((Room.number == 1 && Level.level_number == 14) ||
            (Room.number == 22 && Level.level_number == 10))
            shape = 0x6372;
        else
            shape = -1;
    }
    if (shape == -1) {
        FinlSortForeTable();
        return;
    }
    if (shape) {
        dest = source = 0;
        if (Ct[1] > 0) {
            do {
                if (ForeTable[source].pos.image == shape) {
                    if (source != dest) {
                        item = ForeTable[source];
                        _fmemmove(&ForeTable[dest + 1], &ForeTable[dest],
                                        (source - dest) * sizeof(BFITEM));
                        ForeTable[dest] = item;
                    }
                    ++dest;
                }
            } while (++source < Ct[1]);
        }
    }
}


void DrawMessage(const char *message)
{
    char text[100];

    strcpy(text, message); strupr(text);
    ClrMessage(1); StringRect(&MsgRect, 0, 1, text);
}

void DrawRestartMessage(void)
{
    Rect area;

    if (!RestartLevel) {
        ClrLifeBar();
        InsetRect(&area, &MsgRect, -MsgRect.left, 0); area.right = 320;
        if (GameState.control_mode == 2) StringRect(&area, 0, 1, restart_key_message2);
        else StringRect(&area, 0, 1, restart_key_message1);
    }
}

void ClrLifeBar(void)
{
    Rect area;

    InsetRect(&area, &MsgRect, -MsgRect.left, 0);
    area.right = 320; EraseRect(&area);
}

void ClrMessage(int reset_timer)
{
    Rect area;

    if (MsgTimerStart == 600) {
        InsetRect(&area, &MsgRect, -MsgRect.left, 0);
        area.right = 320;
        FillRect(&area, 0);
    } else {
        FillRect(&MsgRect, 0);
    }
    if (reset_timer) MsgTimer = MsgTimerStart = 0;
}

void CopyDRects(void)
{
    while (dirty_rectangle_count--) PrinceCopy(&dirty_rectangles[dirty_rectangle_count]);
    dirty_rectangle_count = 0;
}

#pragma pack(1)
typedef struct HorseShapeHeader {
    int mode;
    byte palette;
    byte payload[96];
} HorseShapeHeader;
#pragma pack()


SHAPE far *_fastcall GetHorseShpInfo(int id, SHAPEINFO *ref)
{
    HorseShapeHeader header;
    SHAPE far *result;
    Handle handle, new_handle;
    HorseShapeHeader far *metadata;

    if (!HorseShpInfo.image || !materialize_handle(HorseShpInfo.image)) {
        if (HorseShpInfo.image) hFree(HorseShpInfo.image);
        handle = hGetRsrc(0x53484150UL, id);
        if (handle) {
            metadata = (HorseShapeHeader far *)h2p(hGetShpList(0x61a9));
            header = *(HorseShapeHeader far *)((char far *)metadata + 4);
            header.palette = 0xf0; header.mode = -2;
            new_handle = convert_shape((HorseShapeHeader far *)hLock(handle), &header, 1);
            hFree(handle);
            HorseShpInfo.mode = 1; HorseShpInfo.temporary = 0;
            HorseShpInfo.four_bit = 0; HorseShpInfo.mirrored = 0;
            HorseShpInfo.image = hPurge(new_handle);
        }
    }
    *ref = HorseShpInfo;
    if (HorseShpInfo.image) result = h2p(HorseShpInfo.image);
    else result = 0;
    return result;
}

const POINT * _fastcall GetHorsePtInPlane(byte room, sbyte pass, word *shape)
{
    const POINT *origin = 0;

    if (pass == 0) {
        if (room == 2 && Level.level_number == 9) {
            origin = &horse_level9_origin;
            *shape = 0x6371;
        }
    } else if (pass == 1 && !GameState.playing_nis) {
        if (room == 1 && Level.level_number == 14) {
            origin = &horse_level14_origin;
            *shape = 0x6372;
        } else if (room == 22 && Level.level_number == 10) {
            origin = &horse_level10_origin;
            *shape = 0x6370;
        }
    }
    return origin;
}

void DrawHorse(word shape, const POINT *origin, sbyte pass)
{
    BFITEM far *entry;

    if (!shape)
        return;
    if (origin) {
        if (pass == 0)
            entry = &BackTable[Ct[0]++];
        else
            entry = &ForeTable[Ct[1]++];
        entry->mirrored = 0;
        entry->pos.y = origin->y;
        entry->pos.x = origin->x;
        entry->pos.image = shape;
        entry->image_set = 4;
        entry->column = ColNo;
        entry->row = RowNo;
        entry->clip = CellClipRect;
        entry->blit = 10;
        entry->peel = ObjId;
    }
}


void DrawFlask(DRAWCELL *args)
{
    int color, tall;
    byte bubble, flags;
    DRAWCELL base;

    if (DrawFunc[1] && !CustomRoom) {
        base = *args;
        base.info.modifier &= 0xff00;
        DrawFunc[1](&base);
    }
    if (args->pass != 1)
        return;
    color = args->info.modifier & 0xe0;
    tall = color < 0xa0 && color >= 0x40;
    AddFImgToFTable(10, tall + 1, args->column, args->row, 10, 0);
    if (!color)
        return;
    color = args->info.modifier & 0x1f;
    if (color == 28 || (color >= 10 && color <= 13) || color - 14 >= 10)
        return;
    bubble = GetFlaskBubbleColor(args->info.modifier);
    if (color > 13) {
        color -= 14;
        flags = 1;
    } else
        flags = 0;
    *(int *)((char *)Pieces + 0x485) = tall ? -17 : -14;
    AddFImgToFTable(60, color + 3, args->column, args->row, bubble, flags);
    AddFImgToFTable(60, color + 13, args->column, args->row, bubble + 1, flags);
}

byte GetFlaskBubbleColor(word attributes)
{
    byte color;

    switch (attributes & 0xe0) {
    default:
        color = 0xec;
        break;
    case 0x20:
    case 0x40:
        color = 0xea;
        break;
    case 0xa0:
        color = 0xee;
        break;
    }
    return color;
}


void DrawKidMeter(int life, int max_life)
{
    int x, index;
    Rect rect;

    x = 2;
    for (index = 0; index < life; index++) {
        PrincePaintShapeIdx(Pics[2], 216, x, 193, 0, 1);
        x += 8;
    }
    for (index = life; index < max_life; index++) {
        PrincePaintShapeIdx(Pics[2], 217, x, 193, 0, 1);
        x += 8;
    }
    if (max_life >= 1 && Char.life_initial < OrigLifePts) {
        rect.top = 193;
        rect.left = x + (Kid.life_initial - max_life) * 8;
        rect.bottom = ScreenPortRect.bottom;
        rect.right = 98;
        EraseRect(&rect);
    }
}

void DrawMeters(void)
{
    if (Kid.opponent_index >= 0 && Kid.opponent_index < 5) {
        LoadKidwOpp(Kid.opponent_index);
        DrawOppMeter(Opp.slot, Opp.life_points, Opp.life_initial);
    }
    DrawKidMeter(Kid.life_points, Kid.life_initial);
}


void DrawOppMeter(sbyte guard, int life, int max_life)
{
    Rect rect;
    struct RectCorner { int bottom, right; };
    int index, x, half;

    if (guard == -1) {
        guard = GetOppIdxClosestToKid(1);
        if (guard == -1 && Level.room_objects[Room.number - 1].opponent_count)
            guard = 0;
    }
    if (guard != -1)
        LoadShad(guard);
    if (guard == -1 || !Level.room_objects[Room.number - 1].opponent_count ||
        Char.character_kind == 10 || Char.character_kind == 11 || Char.character_kind == 0 ||
        (Char.character_kind == 4 && Char.row != 0 && Level.level_number == 5 &&
         (Char.room == 10 || Char.room == 7 || Char.room == 12))) {
        rect.top = 193;
        rect.left = 218;
        *(struct RectCorner *)&rect.bottom =
            *(const struct RectCorner *)&ScreenPortRect.bottom;
        EraseRect(&rect);
    } else if (MsgTimerStart != 600) {
        x = 308;
        if (Char.character_kind == 7 || Char.character_kind == 8) {
            half = life % 2;
            life /= 2;
        } else {
            half = 0;
        }
        for (index = 0; index < life; ++index) {
            PrincePaintShapeIdx(Pics[2], 131, x, 193, 0, 1);
            x -= 10;
        }
        if ((Char.character_kind == 7 || Char.character_kind == 8) && half) {
            PrincePaintShapeIdx(Pics[2], 132, x, 193, 0, 1);
            x -= 10;
            ++index;
        }
        rect.top = 193;
        rect.bottom = ScreenPortRect.bottom;
        for (index = life; index < 8; ++index) {
            rect.left = x;
            rect.right = x + 10;
            EraseRect(&rect);
            x -= 10;
        }
    }
}


void DrawSword(DRAWCELL *args)
{
    DRAWCELL base;
    byte kind;

    if (DrawFunc[1] && (Kid.room != 9 || Level.level_number != 8)) {
        base = *args;
        base.info.modifier &= 0xfffc;
        DrawFunc[1](&base);
    }
    if (args->pass == 1) {
        kind = args->info.modifier & 3;
        AddFImgToFTable(22, kind == (byte)KidInfo.sword_kind || ((byte)KidInfo.sword_kind == 255 && kind == 1) ? 11 : 46,
                        args->column, args->row, 10, 0);
    }
}


int IsFloor(sbyte kind)
{
    if (!IsWall(kind) && !IsSpace(kind))
        return 1;
    return 0;
}

int IsImpassable(sbyte kind)
{
    if (kind == 11 || kind == 15 || kind == 26 || kind == 12 ||
        kind == 13 || kind == 23 || kind == 24)
        return 1;
    return 0;
}

int IsLandOnable(sbyte kind)
{
    if (IsFloor(kind) && !IsImpassable(kind) && kind != 6 && kind != 34)
        return 1;
    return 0;
}

int IsMobFloor(sbyte kind)
{
    if (kind == 11 || kind == 26 || kind == 15)
        return 1;
    return 0;
}

int IsSpace(sbyte kind)
{
    if (kind == 0 || kind == 9 || kind == 33 || kind == 35 || kind == 27 || kind == 37)
        return 1;
    return 0;
}

int IsWall(sbyte kind)
{
    if (kind == 20 || kind == 2 || kind == 7 || kind == 25 || kind == 43)
        return 1;
    return 0;
}


void ResetStdPalette(void)
{
    SetPalette(10, 0, 16, 0);
}

void RestoreGamePlayPalette(void)
{
    if (draw_scratch.saved_palette) {
        SetDACBlock(16, 240, draw_scratch.saved_palette, 1);
        clear_payload_header(draw_scratch.saved_palette); draw_scratch.saved_palette = 0;
    }
    if (draw_scratch.saved_text_palette) {
        SetDACBlock(0, 16, draw_scratch.saved_text_palette, 0);
        clear_payload_header(draw_scratch.saved_text_palette); draw_scratch.saved_text_palette = 0;
    }
}

void SaveGamePlayPalette(void)
{
    Rect rect;

    if (!draw_scratch.saved_palette) {
        draw_scratch.saved_palette = allocate_pinned_payload(720L);
        ReadDACBlock(16, 240, draw_scratch.saved_palette);
        SetDACBlock(16, 240, 0, 1);
        if (!EmptyRect(&Kid.previous_bounds) &&
            SectRect(&rect, &Kid.previous_bounds, &GamePlayRect))
            EraseRect(&rect);
    }
}

typedef struct PaletteColor {
    byte red, green, blue;
} PaletteColor;

void SetPaletteToColorIdx(byte color)
{
    PaletteColor far *palette;
    PaletteColor far *next;
    int remaining;

    palette = (PaletteColor far *)allocate_pinned_payload(768L);
    ReadDACBlock(color, 1, (byte far *)palette);
    next = palette + 1; remaining = 255;
    do { *next = *palette; ++next; } while (--remaining);
    if (!draw_scratch.saved_palette) {
        draw_scratch.saved_palette = allocate_pinned_payload(720L);
        ReadDACBlock(16, 240, draw_scratch.saved_palette);
    }
    if (!draw_scratch.saved_text_palette) {
        draw_scratch.saved_text_palette = allocate_pinned_payload(48L);
        ReadDACBlock(0, 16, draw_scratch.saved_text_palette);
    }
    SetDACBlock(0, 256, (byte far *)palette, 1);
    clear_payload_header((byte far *)palette);
}

int SetPalette(word resource, int first, byte count, byte palette)
{
    byte total;
    int success;
    Handle handle;
    byte far *colors;

    total = GetTotalPals(resource);
    handle = hGetRsrc(0x50414c53UL, resource);
    if (handle && total > palette) {
        colors = (byte far *)h2p(handle);
        if (first < 16) {
            if (draw_scratch.saved_palette) {
                if (count - (total = count < 16 - first ? count : 16 - first) > 0)
                    _fmemcpy(draw_scratch.saved_palette, &colors[(count * palette + total) * 3],
                                       (count - total) * 3);
            } else {
                total = count;
            }
            SetDACBlock(first, total, &colors[count * palette * 3], 1);
        } else if (draw_scratch.saved_palette) {
            _fmemcpy(draw_scratch.saved_palette + first * 3 - 48, &colors[count * palette * 3], count * 3);
        } else {
            SetDACBlock(first, count, &colors[count * palette * 3], 1);
        }
        hFree(handle);
        success = 1;
    } else {
        success = 0;
    }
    return success;
}

byte GetTotalPals(int resource)
{
    Handle handle;
    int total;

    handle = hGetRsrc(0x50414c43UL, resource);
    if (handle) {
        total = *(int far *)h2p(handle);
        hFree(handle);
    } else {
        total = 0;
    }
    return total;
}


void TweakKidShpList(int variant)
{
    ((int far *)h2p(Pics[2]->resource))[2] = variant;
}


int VisibleAbovePiece(byte room, byte piece)
{
    int visible = 0;
    sbyte above, upper_left;

    above = PieceAboveId(room, piece);
    if (IsSpace(above) || above == 5 || above == 6 ||
        (above == 3 && Level.level_number == 13)) {
        visible = 1;
    } else {
        upper_left = PieceAboveRightId(room, piece);
        if (IsSpace(upper_left) || upper_left == 5 || upper_left == 6 ||
            (above == 3 && Level.level_number == 13))
            visible = 1;
    }
    return visible;
}
