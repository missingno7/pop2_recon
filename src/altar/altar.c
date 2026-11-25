/* ALTAR.C, overlay 15. DOS entry order. */
#include "types.h"
#include "level.h"
#include "char.h"
#include "game.h"
#pragma pack(1)

typedef unsigned int Handle;

typedef struct {
    signed char type;
    int x, y;
    byte pass, palette;
    int resource;
    Handle shape;
    Rect rect;
    byte flags, peel, unknown[4];
} RoomPiece;
typedef struct {
    byte count, type;
    int palette;
    char unknown[24];
    RoomPiece pieces[1];
} RoomData;
typedef struct {
    signed char position;
    int x;
    signed char direction;
    byte flag, unknown[4], mode, id, slot, enabled, flag2;
} Spawn;
typedef struct { int size; char unknown[2]; unsigned int flags; char bytes[97]; } Shape;
typedef struct { char data[4]; Handle handle; } ShapeRef;
typedef struct { byte type; char skip[3]; unsigned int modifier; } TileAnim;
typedef struct { byte frame, unknown; signed char col, dy; } FlameStep;
#pragma pack()

extern CHARREC Kid;
extern CHARREC Char;
extern CHARREC RoomOpp[5];

extern Directions Dir;
extern int NeedShadow;                        /* DS:0a16 */
extern RoomData *CustomRoom;
extern LEVELREC Level;
extern unsigned int BlueSpec;            /* DS:5e48 */

extern TROBREC Trob;
byte flame_image_map[6] = {0x24, 0x24, 0x25, 0x26, 0x26, 0x24};
extern Rect CellClipRect, GamePlayRect;
extern Handle *Pics[5];
extern Handle SwordFrames;             /* DS:6268 */
extern FRAMEREC Frame;
extern FCHARREC FChar;

extern void SaveShad(void), LoadKid(void), LoadShad(signed char), RemoveChar(void);
extern void ClrLifeBar(void), DrawKidMeter(int, int), AddSong(int), AddSound(int);
extern void JumpSeq(int), DoStepFwd(int), DoRunJump(int), FallOn(void);
extern int IsCharSwingingFwd(void);
extern void ClimbUp(void);
extern int AddCharX(int), FirstInRow(signed char), AddFCharX(int);
extern void DecLifePts(byte), SetKidDeathMethod(byte);
extern int SndOn(int);
extern Spawn *GetOppInfoIdx(unsigned int, int);
extern void ResetCharY(void), ClearCharVel(void), AnimChar(void);
extern void SetPalette(int, int, byte, byte);
extern void far _fastcall DrawCustomRoomImg(byte, byte);
typedef void far pascal DrawCallback(void);
extern DrawCallback MarkMove;
extern void MarkBufsForRect(DrawCallback *, void near *, signed char);
extern Handle hNew(long);
extern void far *h2p(Handle);
extern void hFree(Handle);
extern void PrinceNewShpItem(Handle, int, ShapeRef far *, int);
extern void PrinceFreeShp(int);
extern void MAKESHAPERECT(Rect far *, int, int, void far *);
extern void SaveFChar(void), RestoreFChar(void);

void AltarKidRisesFromDeath(void);
void CreateShadowman(void);

void AltarKidRisesFromDeath(void)
{
    signed char id = Char.slot;
    SaveShad();
    LoadKid();
    JumpSeq(231);
    Char.alive = -1;
    Char.life_points = Char.life_initial;
    ClrLifeBar();
    DrawKidMeter(Char.life_points, Char.life_initial);
    Char.sequence_group = 13;
    Char.stance = 0;
    Kid = Char;
    LoadShad(id);
    RemoveChar();
    AddSong(60);
}

extern void GetCharEdges(void);
extern DrawCallback MarkFloor;

void AltarShadowProg(void)
{
    int sequence = -1;
    int distance;
    if (Char.frame == 15) {
        if (Char.direction == -1) {
            if (Char.column >= 6) {
                if (Char.sequence_group == 13)
                    sequence = 71;
                else if (Char.column >= 8)
                    sequence = 1;
                else {
                    DoStepFwd(0);
                    if (Char.sequence == 44)
                        sequence = 3;
                }
            } else if (Char.sequence_group == 13 || Char.sequence == 232) {
                sequence = 5;
                AddSong(59);
            } else {
                sequence = 232;
            }
        } else if (Char.sequence_group != 13) {
            sequence = 5;
        } else if (Char.column < 6) {
            DoStepFwd(0);
            if (Char.sequence == 44)
                sequence = 3;
        } else {
            distance = Kid.x - Char.x;
            if (Kid.direction == Char.direction)
                distance += 6;
            if (distance)
                DoStepFwd(distance);
            else
                sequence = Kid.direction == Char.direction ? 71 : 5;
        }
    } else if (((Char.frame >= 1 && Char.frame <= 14) ||
                (Char.frame >= 49 && Char.frame <= 56)) &&
               Char.column >= 6) {
        DoRunJump(4);
    } else if (Char.frame >= 103 && Char.frame <= 106) {
        Dir.action = 255;
        FallOn();
    } else if (Char.frame == 91 && !IsCharSwingingFwd()) {
        ClimbUp();
        if (Char.sequence_group == 13)
            Char.y -= 2;
        if (Char.direction == 0) {
            GetCharEdges();
            MarkBufsForRect(MarkFloor, &Char.bounds, Char.slot);
        }
    } else if (Char.frame == 154) {
        Level.cell_info[3][11].modifier = 133;
    } else if (Char.frame == 153 && Char.sequence_group == 13) {
        SetPalette(CustomRoom->palette, 48, 16, 0);
    } else if (Char.sequence_group == 13 && Char.frame == 185) {
        AltarKidRisesFromDeath();
    } else if (Char.frame == 44) {
        sequence = 13;
        Char.x = AddCharX(-8);
    }
    if (sequence != -1)
        JumpSeq(sequence);
}

void AltarWatchKid(void)
{
    LoadKid();
    if (Char.row == 1 && Char.sequence != 230) {
        if (Char.x <= 218) {
            JumpSeq(230);
            Char.x = 210;
            DecLifePts(100);
            AddSound(57);
            SetKidDeathMethod(16);
            Level.cell_info[3][11].modifier = 133;
        } else if (Char.x <= 236 && !SndOn(10061)) {
            AddSong(61);
        } else if (Char.frame == 185 && Char.row == 1 &&
                   Char.sequence_group != 12 && NeedShadow == 1) {
            CreateShadowman();
            NeedShadow = -1;
        }
    }
    Kid = Char;
}

void CreateShadowman(void)
{
    Spawn *entry;
    signed char id, zero, clear;
    int room;
    if (Char.room == 4 && (Level.level_number) == 13) {
        Kid = Char;
        room = Char.room;
        id = Level.room_objects[room - 1].opponent_count++;
        entry = GetOppInfoIdx(room, id);
        entry->position = FirstInRow(Char.row) + Char.column;
        entry->x = Char.x;
        entry->direction = Char.direction;
        clear = 0;
        entry->flag = clear;
        entry->mode = clear;
        entry->id = id;
        zero = 0;
        entry->slot = zero;
        entry->flag2 = zero;
        entry->enabled = 1;
        Char.slot = id;
        Char.x = entry->x;
        Char.life_points = Char.character_kind = 1;
        Char.alive = -1;
        Char.image_bank = 8;
        ResetCharY();
        --Char.y;
        ClearCharVel();
        Char.sequence_group = 0;
        Char.alert_delay = Char.stance = 0;
        JumpSeq(231);
        AnimChar();
        AddSong(63);
        SaveShad();
        LoadKid();
    }
}

int IsAltarShadowActive(void)
{
    int result = 0;
    signed char i, count;
    i = 0;
    count = Level.room_objects[3].opponent_count;
    if (count > 0) {
        do {
            if (result)
                break;
            if (RoomOpp[i].character_kind == 1)
                result = 1;
        } while (++i < count);
    }
    return result;
}

void AnimSacredFlame(void)
{
    unsigned int phase;
    byte image;
    if (CustomRoom && CustomRoom->type == 32) {
        phase = BlueSpec;
        if (phase & 0x80) {
            if (!(phase & 15)) {
                phase = 0;
                image = 11;
            } else {
                --phase;
                image = flame_image_map[phase & 15];
            }
        } else {
            if (phase < 11)
                ++phase;
            else
                phase = 0;
            image = phase + 11;
        }
        MarkBufsForRect(MarkMove, &CustomRoom->pieces[image].rect, -1);
        BlueSpec = phase;
    } else {
        Trob.state = -1;
    }
}

void DrawTempSacredFlame(TileAnim *tile)
{
    struct { Rect saved; byte align, image; unsigned int modifier; } draw;
    RoomPiece *piece;
    if (tile->type == 11) {
        draw.saved = CellClipRect;
        CellClipRect = GamePlayRect;
        draw.modifier = tile->modifier;
        if (draw.modifier & 0x80) {
            draw.image = flame_image_map[draw.modifier & 0xff7f];
            piece = &CustomRoom->pieces[draw.image];
            piece->pass = 1;
            DrawCustomRoomImg(draw.image, 1);
            piece->pass = 11;
        } else {
            DrawCustomRoomImg(draw.modifier + 11, 11);
        }
        CellClipRect = draw.saved;
    }
}

void LoadAltarRoomFunc(void)
{
    int i, index;
    Handle handle;
    RoomPiece *piece;
    Shape far *source, far *copy;
    ShapeRef ref;

    handle = hNew(sizeof(Shape));
    source = h2p(*Pics[2]);
    copy = h2p(handle);
    *copy = *source;
    copy->flags = 0x4000;
    copy->size = CustomRoom->palette;
    for (i = 11; i <= 38; i++) {
        index = i;
        piece = &CustomRoom->pieces[index];
        hFree(piece->shape);
        ref.handle = 0;
        PrinceNewShpItem(handle, piece->type + 1, &ref, 1);
        piece->shape = ref.handle;
        PrinceFreeShp(piece->type + CustomRoom->palette);
        i = index;
    }
    hFree(handle);
    SetPalette(CustomRoom->palette, 224, 16, 0);
    SetPalette(2000, 48, 16, 0);
}

void SetupFlame(void)
{
    Rect saved;
    FlameStep far *steps;
    byte image;
    RoomPiece *piece;
    if (Char.room == 4 && (Level.level_number) == 13) {
        SaveFChar();
        saved = CellClipRect;
        steps = h2p(SwordFrames);
        image = steps[Frame.equipment_image].frame + 23;
        piece = &CustomRoom->pieces[image];
        piece->y = AddFCharX(steps[Frame.equipment_image].col);
        piece->x = steps[Frame.equipment_image].dy + FChar.y;
        MAKESHAPERECT(&piece->rect, piece->x, piece->y, h2p(piece->shape));
        CellClipRect = piece->rect;
        piece->pass = 0;
        DrawCustomRoomImg(image, 0);
        piece->pass = 11;
        CellClipRect = saved;
        RestoreFChar();
    }
}

void UnloadAltarRoomFunc(void)
{
    SetPalette(3000, 224, 16, 0);
}
