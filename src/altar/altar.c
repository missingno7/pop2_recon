/* ALTAR.C, overlay 15. DOS entry order. */
#include "types.h"
#include "snd.h"
#include "graf.h"
#include "shape.h"
#include "level.h"
#include "hnewshpi.h"
#include "heap.h"
#include "char.h"
#include "movescyt.h"
#include "game.h"
#include "draw.h"
#include "custom.h"
#include "custcvca.h"
#include "act.h"
#include "altar.h"
#include "auto.h"
#include "collide.h"
#include "ctrl.h"
#include "fade.h"
#include "load.h"
#include "move.h"
#include "prince.h"
#include "sound.h"
#include "top.h"


                        /* DS:0a16 */


            /* DS:5e48 */


byte flame_image_map[6] = {0x24, 0x24, 0x25, 0x26, 0x26, 0x24};


             /* DS:6268 */


typedef void DrawCallback(void);


void AltarKidRisesFromDeath(void)
{
    sbyte id = Char.slot;
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
        SetPalette(CustomRoom->first_shape, 48, 16, 0);
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
    OPPONENT *entry;
    sbyte id, clear;
    int room;
    if (Char.room == 4 && Level.level_number == 13) {
        Kid = Char;
        room = Char.room;
        id = Level.room_objects[room - 1].opponent_count++;
        entry = GetOppInfoIdx(room, id);
        entry->cell = FirstInRow(Char.row) + Char.column;
        entry->x = Char.x;
        entry->direction = Char.direction;
        clear = 0;
        entry->skill = clear;
        entry->palette_kind = clear;
        entry->index = id;
        entry->combat_mode = 0;
        entry->behavior = 0;
        entry->life = 1;
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
    sbyte i, count;
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
    word phase;
    byte image;
    if (CustomRoom && CustomRoom->kind == 32) {
        phase = BlueSpec.modifier;
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
        MarkBufsForRect(MarkMove, &CustomRoom->pieces[image].clip, -1);
        BlueSpec.modifier = phase;
    } else {
        Trob.state = -1;
    }
}

void DrawTempSacredFlame(DRAWCELL *tile)
{
    struct { Rect saved; byte align, image; word modifier; } draw;
    CUSTOMPIECE *piece;
    if (tile->pass == 11) {
        draw.saved = CellClipRect;
        CellClipRect = GamePlayRect;
        draw.modifier = tile->info.modifier;
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
    CUSTOMPIECE *piece;
    SHAPELISTRSRC far *source, far *copy;
    SHAPEINFO ref;

    handle = hNew(sizeof(SHAPELISTRSRC));
    source = h2p(Pics[2]->resource);
    copy = h2p(handle);
    *copy = *source;
    copy->palette.banks = 0x4000;
    copy->first_id = CustomRoom->first_shape;
    for (i = 11; i <= 38; i++) {
        index = i;
        piece = &CustomRoom->pieces[index];
        hFree(piece->handle);
        ref.image = 0;
        PrinceNewShpItem(handle, piece->shape + 1, &ref, 1);
        piece->handle = ref.image;
        PrinceFreeShp(piece->shape + CustomRoom->first_shape);
        i = index;
    }
    hFree(handle);
    SetPalette(CustomRoom->first_shape, 224, 16, 0);
    SetPalette(2000, 48, 16, 0);
}

void SetupFlame(void)
{
    Rect saved;
    ATTACHFRAME far *steps;
    byte image;
    CUSTOMPIECE *piece;
    if (Char.room == 4 && Level.level_number == 13) {
        SaveFChar();
        saved = CellClipRect;
        steps = h2p(SwordFrames);
        image = steps[Frame.equipment_image].image + 23;
        piece = &CustomRoom->pieces[image];
        piece->pos.x = AddFCharX(steps[Frame.equipment_image].x);
        piece->pos.y = steps[Frame.equipment_image].y + FChar.y;
        MakeShapeRect(&piece->clip, piece->pos.y, piece->pos.x, h2p(piece->handle));
        CellClipRect = piece->clip;
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
