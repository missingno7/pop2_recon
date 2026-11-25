#include "types.h"
#include "resc.h"
#include "graf.h"
#include "shape.h"
#include "level.h"
#include "heap.h"
#include "char.h"
#include "game.h"
#include "draw.h"
#include "custom.h"
#include "add.h"
#include "altar.h"
#include "autoguar.h"
#include "autoshad.h"
#include "collide.h"
#include "ctrl.h"
#include "custcvca.h"
#include "desert.h"
#include "fade.h"
#include "final.h"
#include "lgraf.h"
#include "load.h"
#include "act.h"
#include "move.h"
#include "rooftops.h"
#include "recorder.h"
#include "prince.h"
#include "tables.h"
#include "top.h"
#pragma pack(1)


BFITEM far BackTable[130];
BFITEM far ForeTable[100];

MIDITEM far MidTable[30];


int _fastcall AddImgInfoToBFItem(byte set, IMAGEPOS *placement, int image,
                               BFITEM far *item, sbyte x, sbyte y,
                               byte order, byte layer)
{
    Rect bounds;
    int far *shape;
    int result;

    item->pos = *placement;
    if (image != -2) {
        item->pos.image = image;
    } else {
        image = item->pos.image;
    }

    if (set == 4) CheckBGSetImg(image - 1);

    shape = h2p(Pics[set]->items[image - 1].image);
    if (shape == 0 || *shape == 0) return 0;

    item->pos.y += Row2PixelY[y + 1] - *shape;
    item->pos.x += Col2PixelX[x + 1];
    MakeShapeRect(&bounds, item->pos.y, item->pos.x, shape);

    if (SectRect(&bounds, &bounds, &CellClipRect)) {
        item->image_set = set;
        item->column = ColNo;
        item->row = RowNo;
        item->clip = bounds;
        item->blit = order;
        item->mirrored = layer;
        result = 1;
    } else {
        result = 0;
    }
    return result;
}

int AddBImgToBTable(byte tile, int image, sbyte x, sbyte y, byte order, byte layer)
{
    int i = Ct[0];
    int added;

    if (i >= 130) return 0;
    added = AddImgInfoToBFItem(4, &Pieces[tile].back, image,
                           &BackTable[i], x, y, order, layer);
    if (added) {
        BackTable[i].peel = Pieces[tile].peel;
        ++Ct[0];
    }
    return added;
}

int AddBFImgToBTable(byte tile, int image, sbyte x, sbyte y,
                     byte order, byte layer)
{
    int i;
    int added = 0;

    if (image) {
        i = Ct[0];
        if (i >= 130) return 0;
        added = AddImgInfoToBFItem(4, &Pieces[tile].back_fore, image,
                               &BackTable[i], x, y, order, layer);
        if (added) {
            BackTable[i].peel = Pieces[tile].peel;
            ++Ct[0];
        }
    }
    return added;
}

int AddBFImgToFTable(byte tile, int image, sbyte x, sbyte y,
                     byte order, byte layer)
{
    int i = Ct[1];
    int added;

    if (i >= 100) return 0;
    added = AddImgInfoToBFItem(4, &Pieces[tile].back_fore, image,
                           &ForeTable[i], x, y, order, layer);
    if (added) {
        ForeTable[i].peel = Pieces[tile].peel;
        ++Ct[1];
    }
    return added;
}


void AddChars(void)
{
    Kid1();
    Opponent1();
    init_fchar_rect();
    CheckMeters();
}


OBJITEM ObjTable[30];


#pragma pack(1)


void _fastcall AddCharObj(byte kind)
{
    int index;
    OBJITEM *object;

    if (Ct[4] < 30) {
        index = Ct[4]++;
        object = &ObjTable[index];
        object->kind = kind;
        ConvertX(FChar.x, &object->x);
        object->y = FChar.y;
        object->image_set = FChar.image_set;
        object->image = FChar.image;
        object->direction = FChar.direction;
        object->clip = FChar.clip;
        object->variant = FChar.variant;
        SetObjCell(index);
        object->frame = Char.frame;
        object->owner = Char.slot;
    }
}

int AddFImgToFTable(byte tile, int image, sbyte x, sbyte y, byte order, byte layer)
{
    byte set;
    int i;
    int added;

    if (tile == 10 || tile == 60) set = 1;
    else set = tile == 22 ? 0 : 4;

    i = Ct[1];
    if (i >= 100) return 0;
    added = AddImgInfoToBFItem(set, &Pieces[tile].fore, image,
                           &ForeTable[i], x, y, order, layer);
    if (added) {
        ForeTable[i].peel = Pieces[tile].peel;
        ++Ct[1];
    }
    return added;
}


int _fastcall AddMid(byte set, int image, int x, int y, byte order)
{
    int index;
    int image_index;
    SHAPESIZE size;
    int added;

    if (image != 0 && Ct[3] < 30) {
        image_index = image - 1;
        index = Ct[3];
        MidTable[index].x = x;
        if (Level.level_number == 5 && Room.number == 10 &&
            set == 4 && CustomRoom->count > image_index) {
            GetCustomPieceSize(image_index, &size);
        } else {
            if (!GetSetImgSize(Pics[set], image_index, &size) || !size.height) return 0;
        }

        MidTable[index].y = y - size.height + 1;
        MidTable[index].image_set = set;
        MidTable[index].image = image_index;
        MidTable[index].clip = FChar.clip;
        MidTable[index].blit = order;
        MidTable[index].variant = 0;
        if (FChar.direction == 0 && (set != 4 || (sbyte)FChar.owner >= 0)) MidTable[index].mirrored = 1;
        else MidTable[index].mirrored = 0;
        MidTable[index].owner = 255;
        ++Ct[3];
        added = 1;
    } else {
        added = 0;
    }
    return added;
}


PEELINFO PeelInfo;


void _fastcall AddPeel(int left, int right, int top, int height, sbyte owner, sbyte kind)
{
    Rect rect;

    if (PeelInfo.count >= 40) return;
    rect.left = left; rect.right = right;
    rect.top = top;
    rect.bottom = top + height;
    if (EmptyRect(&rect)) return;

    PeelInfo.items[PeelInfo.count].saved = SaveRect(&rect);
    PeelInfo.items[PeelInfo.count].owner = owner;
    PeelInfo.items[PeelInfo.count].kind = kind;
    if (kind != 2 && kind != 3) PeelInfo.items[PeelInfo.count].keep = 0;
    else PeelInfo.items[PeelInfo.count].keep = 1;
    ++PeelInfo.count;
}


void SavePeel(sbyte owner, sbyte occurrence)
{
    int index = GetPeelIdxForCharIdx(owner, occurrence);
    if (index != -1)
        PeelInfo.items[index].keep = 1;
}


void MarkPeelForeBufs(void)
{
    int index = PeelInfo.count;

    while (index--) {
        int redraw;
        if (PeelInfo.items[index].keep)
            continue;
        redraw = 1;
        if (PeelInfo.items[index].owner >= 0 && PeelInfo.items[index].owner < 5) {
            LoadShad(PeelInfo.items[index].owner);
            if (Char.action == 9 || IsCharNonViewable())
                redraw = 0;
        }
        if (redraw)
            MarkBufsForRect(MarkFRedraw, &PeelInfo.items[index].saved->rect, PeelInfo.items[index].owner);
    }
}


int GetPeelIdxForCharIdx(sbyte owner, int occurrence)
{
    int found = -1;
    int index, occurrences;

    for (index = occurrences = 0; index < PeelInfo.count; ++index) {
        if (found != -1) break;
        if (owner == PeelInfo.items[index].owner) {
            ++occurrences;
            if (occurrence == occurrences) found = index;
        }
    }
    return found;
}


void Peel(void)
{
    int keep[40];
    int index = PeelInfo.count;
    int remaining;

    while (index--) {
        SAVEDRECT *saved;
        if ((keep[index] = PeelInfo.items[index].keep) != 0) continue;
        saved = PeelInfo.items[index].saved;
        if (NewRoom == 0) AddDRect(&saved->rect);
        if (PeelInfo.items[index].kind == 2) {
            RestoreRectKeep(saved);
            keep[index] = PeelInfo.items[index].keep = 1;
        } else {
            RestoreRect(saved);
        }
    }
    remaining = 0;
    for (index = 0; index < PeelInfo.count; ++index) {
        if (keep[index]) {
            PeelInfo.items[remaining] = PeelInfo.items[index];
            if (PeelInfo.items[remaining].owner < 100)
                PeelInfo.items[remaining].keep = 0;
            ++remaining;
        }
    }
    PeelInfo.count = remaining;
}


void ZeroPeels(void)
{
    int index = PeelInfo.count;

    while (index > 0)
        DeallocMapSave(PeelInfo.items[--index].saved);
    PeelInfo.count = 0;
}

BFITEM far *_fastcall GetBackTableImgRow(int image, sbyte column, sbyte row)
{
    int index = 0;
    BFITEM far *result = 0;

    for (; index < Ct[0]; ++index) {
        if (result) break;
        if (BackTable[index].pos.image == image &&
            BackTable[index].row == row && BackTable[index].column == column)
            result = &BackTable[index];
    }
    return result;
}


void Kid1(void)
{
    LoadKid();
    if (Char.room != 0 && Char.room == Room.number) {
        SetupKid();
        if (Char.action == 99 || Char.sequence_group == 8) {
            SetupComix();
            if (Char.sequence_group == 8)
                Char.sequence_group = 0;
            else
                Char.action = 5;
        }

        if (Level.environment_kind == 5)
            SetupSplash();
        else if (Level.environment_kind == 1)
            SetupSand();
        else if (Level.level_number == 5 && Char.room == 3 && Char.frame == 295)
            SetupCarpet();
        else if (Char.frame >= 306 && Char.frame <= 318)
            SetupFlame();
        else if (Char.frame >= 272 && Char.frame <= 281)
            SetupFireball();

        if (Char.character_kind == 1 && Level.environment_kind == 6)
            SetupShadowFlame();
        SetupSword(3);
        SetupFlask();
    } else if (Level.environment_kind == 5) {
        SetupSplash();
    }
}

void Opponent1(void)
{
    sbyte opponent = 0;

    if (Level.room_objects[Room.number - 1].opponent_count <= 0)
        return;
    do {
        LoadShad(opponent);
        if (Char.room == Room.number) {
            if (!SetupOpponent())
                continue;
            if ((Char.action == 99 || Char.sequence_group == 8 ||
                 (Char.action == 9 && Char.frame == 213)) &&
                Char.character_kind != 7 && Char.character_kind != 8) {
                if (Char.action == 99)
                    Char.action = 5;
                SetupComix();
            }
            if (Char.character_kind != 10 && Char.character_kind != 1)
                SetupSword(9);
            if (Char.character_kind == 12)
                SetupJinneeSmoke();
            else if (Char.character_kind == 1)
                SetupShadowFlame();
            else if (Level.environment_kind == 5)
                SetupSplash();
            else if (Level.environment_kind == 6)
                SetupGuardSmoke();
        } else if (Level.environment_kind == 5) {
            SetupSplash();
        }
    } while (Level.room_objects[Room.number - 1].opponent_count > ++opponent);
}

void SetupChar(void)
{
    int equipment;

    init_fchar_rect();
    GetFrameInfo();
    equipment = GetSwordFrameImg(Frame.equipment_image);
    if (Char.frame == 0 || (Frame.image == -1 && equipment == -1))
        return;

    FChar.direction = Char.direction;
    FChar.variant = Char.image_bank;
    if (Char.character_kind == 0 || Char.character_kind == 6 || Char.character_kind == 1) {
        FChar.image_set = 2;
        FChar.image = Frame.image;
    } else {
        FChar.image_set = 3;
        FChar.image = Char.character_kind == 10 ? equipment : Frame.image;
    }

    if ((Char.character_kind == 7 || Char.character_kind == 8) && Char.sequence_group == 1) {
        AutoFwd();
        return;
    }
    FChar.y = Frame.y + Char.y;
    FChar.x = AddCharX(Frame.x) - 130;
    if (Char.character_kind == 10) {
        SetupFlameSword();
        return;
    }
    if (Char.character_kind == 4 && Char.frame >= 206 && Char.frame <= 209) {
        FChar.image_set = 4;
        FChar.image += 104;
    }
}


void SetupComix(void)
{
    word frame = Char.frame;

    SaveFChar();
    FChar.cell = -1;
    FChar.image_set = 2;
    FChar.image = 218;

    if (frame != 185 && (frame < 106 || frame >= 111)) {
        if (Char.character_kind == 11) {
            FChar.y += 5;
            AddFCharX(-8);
            FChar.image_set = 3;
            FChar.image = 119;
        } else if (Char.character_kind == 12) {
            FChar.y -= 11;
            AddFCharX(-8);
        } else if (Char.character_kind == 0) {
            if (Opp.character_kind == 11 || Char.frame == 265) {
                AddFCharX(10);
            } else if (Char.frame == 138 || Char.frame == 139 || Char.frame == 140) {
                FChar.y -= 10;
                AddFCharX(-5);
            } else if (Char.frame == 338) {
                FChar.y -= 30;
                AddFCharX(29);
            } else if (Char.frame == 340) {
                FChar.y -= 30;
                AddFCharX(-17);
            } else {
                FChar.y -= 15;
                AddFCharX(10);
            }
        } else {
            FChar.y -= 11;
            AddFCharX(7);
        }
    } else {
        FChar.y += 4;
        AddFCharX(12);
    }
    MakeFCharClipRect();
    AddCharObj(5);
    MarkBufsForRect(MarkFRedraw, &FChar.clip, -1);
    RestoreFChar();
}


void SetupFireball(void)
{
    SaveFChar();
    if (Frame.equipment_image) {
        word frame = Frame.equipment_image;
        ATTACHFRAME far *entries = (ATTACHFRAME far *)h2p(SwordFrames);
        FChar.image = entries[frame].image;
        AddFCharX(entries[frame].x);
        FChar.y += entries[frame].y;
        MakeFCharClipRect();
        AddCharObj(3);
        MarkBufsForRect(MarkFRedraw, &FChar.clip, -1);
    }
    RestoreFChar();
}

void SetupKid(void)
{
    if (Char.frame == 0)
        return;
    RereadCells();
    SetupChar();
    UnevenFloor();
    GetCharEdges();
    IndexChar();
    QuickFG();
    QuickFloor();
    ClipChar();
    AddCharObj(Char.character_kind);
}

void _fastcall SetupSword(byte kind)
{
    word frame;
    ATTACHFRAME far *entries;

    if (IsCharNonViewable() || Frame.equipment_image == 0)
        return;
    if (Frame.equipment_image > 144 &&
        !(Frame.equipment_image >= 200 && Frame.equipment_image < 251) &&
        !(Frame.equipment_image >= 292 && Frame.equipment_image <= 304))
        return;
    if (Char.character_kind == 0 && Char.stance == 0 && Char.frame != 133 &&
        !(Char.frame >= 229 && Char.frame <= 240) &&
        Char.frame != 323 && Char.frame != 324)
        return;

    frame = Frame.equipment_image;
    entries = (ATTACHFRAME far *)h2p(SwordFrames);
    FChar.image = entries[frame].image;
    if (FChar.image == -1)
        return;
    AddFCharX(entries[frame].x);
    FChar.y += entries[frame].y;
    FChar.image_set = 0;
    MakeFCharClipRect();
    if (Level.environment_kind == 5)
        RoofClipChar();
    AddCharObj(kind);
    if (Char.action != 9)
        MarkBufsForRect(MarkFRedraw, &FChar.clip, Char.slot);
}

void SetupFlask(void)
{
    word frame;
    ATTACHFRAME far *entries;

    if (Frame.equipment_image < 145 || Frame.equipment_image > 168)
        return;
    frame = Frame.equipment_image;
    entries = (ATTACHFRAME far *)h2p(SwordFrames);
    if (entries[frame].image == -1)
        return;
    FChar.image = entries[frame].image;
    AddFCharX(entries[frame].x);
    FChar.y += entries[frame].y;
    FChar.image_set = 1;
    MakeFCharClipRect();
    AddCharObj(3);
    MarkBufsForRect(MarkFRedraw, &FChar.clip, Char.slot);
}

void CheckBGSetImg(int index)
{
    int image;

    if (index < 0)
        return;
    if (Pics[4]->items[index].image && materialize_handle(Pics[4]->items[index].image))
        return;
    if (Pics[4]->items[index].image) {
        hFree(Pics[4]->items[index].image);
        Pics[4]->items[index].image = 0;
    }
    image = index + 1;
    if (index >= Level1stOverlay[Level.environment_kind])
        image += 200;
    PrinceNewShpItem(Pics[4]->resource, image, &Pics[4]->items[index], 1);
    hPurge(Pics[4]->items[index].image);
    PrinceFreeShp(image + 3500);
}


void CheckKidSetImg(int index)
{
    int image;

    if (index < 0)
        return;
    if (Pics[2]->items[index].image && materialize_handle(Pics[2]->items[index].image))
        return;
    if (Pics[2]->items[index].image) {
        hFree(Pics[2]->items[index].image);
        Pics[2]->items[index].image = 0;
    }
    image = Pics[2]->id + index + 1;
    if (index >= 222)
        image -= 400;
    if ((index >= 131 && index <= 132) || (index >= 216 && index <= 218))
        PrinceNewShpItem(Pics[2]->resource, image, &Pics[2]->items[index], 1);
    else
        Pics[2]->items[index].image = hGetRsrc(0x53484150UL, image);
    hPurge(Pics[2]->items[index].image);
}

void CheckOppSetImg(int index)
{
    int image;
    sbyte outfit;

    if (index < 0)
        return;
    if (!Pics[3]->items[index].image || materialize_handle(Pics[3]->items[index].image))
        return;
    image = index + 1;
    outfit = (Char.character_kind == 10 || Char.character_kind == 12)
           ? CharOppId[Char.character_kind] : Level.preferred_opponent;
    if (outfit != 5 && outfit != 6 && Opp1stRange[outfit] &&
        *Opp1stRange[outfit] <= index)
        image += 100;
    PrinceNewShpItem(Pics[3]->resource, image, &Pics[3]->items[index], 2);
    hPurge(Pics[3]->items[index].image);
    PrinceFreeShp(image + 750);
}

SHAPESIZE far *GetSetImgSize(SHAPELIST *set, int index, SHAPESIZE far *size)
{
    if (set == Pics[2])
        CheckKidSetImg(index);
    else if (set == Pics[3])
        CheckOppSetImg(index);
    else if (set == Pics[4])
        CheckBGSetImg(index);

    size = PrinceGetnShapeItemSize(set, index + 1, size);
    return size;
}

SHAPEINFO far *GetSetShapeItem(SHAPELIST *set, int image, SHAPEINFO far *item)
{
    if (set == Pics[2])
        CheckKidSetImg(image - 1);
    else if (set == Pics[3])
        CheckOppSetImg(image - 1);
    else if (set == Pics[4])
        CheckBGSetImg(image - 1);

    item = PrinceGetnShapeItem(set, image, item);
    return item;
}

int _fastcall SetupOpponent(void)
{
    byte kind;
    int added;

    RereadCells();
    SetupChar();
    UnevenFloor();
    GetCharEdges();
    IndexChar();
    QuickFloor();
    ClipChar();

    if (!Char.alert_delay && Char.previous_frame == Char.frame &&
        Char.previous_x == Char.x && Char.previous_y == Char.y &&
        Char.previous_mode == 0 && Char.character_kind != 10 && Char.character_kind != 11) {
        SavePeel(Char.slot, 1);
        SavePeel(Char.slot, 2);
        added = 0;
    } else {
        if (Char.character_kind == 7 || Char.character_kind == 8 || Char.character_kind == 10 ||
            Char.character_kind == 6 || Char.character_kind == 1) {
            kind = Char.character_kind;
            if (kind == 10)
                FlameSwordFloatY();
        } else {
            kind = 2;
        }
        if (Char.action != 9 && !IsCharNonViewable())
            MarkBufsForRect(MarkFRedraw, &Char.bounds, Char.slot);
        AddCharObj(kind);
        Char.previous_x = Char.x;
        Char.previous_y = Char.y;
        Char.previous_mode = NewRoom;
        SaveShad();
        added = 1;
    }
    return added;
}


/* Terminal ADD.C entry. Its one-past-end address still needs a witness. */
void ForceOppRedraw(void)
{
    char *row = (char *)&RoomOpp[0].previous_x;
    do {
        row += sizeof(CHARREC);
        *(int *)(row - sizeof(CHARREC)) = 0;
    } while (row < (char *)&RoomOpp[0].previous_x + sizeof(RoomOpp));
}
