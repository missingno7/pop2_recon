#include <stdlib.h>
#include "types.h"
#include "graf.h"
#include "snd.h"
#include "shape.h"
#include "level.h"
#include "char.h"
#include "game.h"
#include "sound.h"
#include "draw.h"
#include "bridge.h"
#include "act.h"
#include "add.h"
#include "auto.h"
#include "top.h"
#include "ctrl.h"
#include "fade.h"
#include "drawruin.h"
#include "load.h"
#include "move.h"
#include "util.h"
#include "autoshad.h"
#include "prince.h"
#include "tables.h"


int CheckShadowmanBackIntoKid(void)
{
    sbyte index;
    int distance, result;
    CHARREC *kid;

    index = Opp.slot;
    distance = GetShadowToKidDist();
    if (abs(distance) < 32) {
        if (Opp.direction != Char.direction) {
            Char.direction = ~Char.direction;
            Char.x = AddCharX(6);
        }
        Char.x -= distance;
        --Char.y;
        Char.repeat = 0;
        kid = &Kid;
        *kid = Char;
        result = 71;
    } else {
        result = -1;
    }
    LoadKidwOpp(index);
    return result;
}

void CheckShadowmanCreate(void)
{
    if (Char.character_kind == 0 && Char.sequence == 5) {
        if (++KidInfo.shadow_timer >= 4) {
            if (KidInfo.shadow_timer == 4) {
                SetPalette(2000, 48, 16, IsShadowImbued() != 0);
                hGetSnd(10261);
            }
            if (!DecLifePts(1))
                --Char.life_initial;
            else
                JumpSeq(71);
            if (KidInfo.shadow_timer == 8) {
                if (Char.life_points > 4) {
                    CreateKidAsOpp();
                    Char.character_kind = 1;
                    Char.image_bank = 8;
                    JumpSeq(2);
                    AddSound(261);
                    AddSong(272);
                } else {
                    JumpSeq(71);
                    DecLifePts(100);
                }
                KidInfo.shadow_timer = 0;
            }
            AddSound(93);
        }
    } else {
        KidInfo.shadow_timer = 1;
    }
}


void CheckShadow(void)
{
    if (Char.room == 4 && Level.level_number == 13 && Char.sequence_group == 13) {
        RestoreGamePlayPalette();
        return;
    }
    if (Char.sequence == 242 && !GetMobTypeItem(12, 1) && Char.life_points <= 2)
        SetPalette(2000, 48, 16, 0);
}

void CheckShadowPaletteChange(void)
{
    sbyte bank = -1;

    if (Kid.life_points > 2 && Kid.life_points + Kid.life_delta <= 2 && Kid.sequence != 242)
        bank = 0;
    else if (Kid.life_points <= 2 && Kid.life_points + Kid.life_delta > 2)
        bank = 1;
    if (bank != -1)
        SetPalette(2000, 48, 16, bank);
}

void CheckTurnSeqs(void)
{
    if (Char.frame == 15)
        KidInfo.shadow_timer = 0;
    else if (Char.frame >= 45 && Char.frame <= 52 &&
             !((Char.room == 1 || Char.room == 2) && Level.level_number == 14)) {
        if (KidInfo.shadow_timer >= 4)
            Char.image_bank = IsShadowBlinkOn(Char.frame, KidInfo.shadow_timer) ? 8 : 0;
    } else if (Char.sequence != 5 && KidInfo.shadow_timer != 0) {
        SetKidPalette(Level.environment_kind);
        KidInfo.shadow_timer = 0;
    }
}

void CreateKidAsOpp(void)
{
    CHARREC *kid;
    OPPONENT *opp;
    sbyte index;

    if (IsSpace(GetBehind()))
        Char.x = AddCharX(16);
    else if (IsSpace(GetCellsInFront(1)))
        Char.x = AddCharX(-16);

    kid = &Kid;
    *kid = Char;
    index = Level.room_objects[Char.room - 1].opponent_count++;
    opp = GetOppInfoIdx(Char.room, index);
    opp->cell = FirstInRow(Char.row) + Char.column;
    opp->x = Char.x;
    opp->direction = Char.direction;
    opp->skill = 0;
    opp->palette_kind = 0;
    opp->index = index;
    opp->combat_mode = 0;
    opp->behavior = 0;
    opp->life = Char.life_points;
    opp->kind = 10;
    opp->max_life = Char.life_initial;
    Char.slot = index;
    Char.character_kind = 0;
    Char.image_bank = 2;
    JumpSeq(71);
    AnimChar();
    SaveShad();
    LoadKid();
}

void CutToKidRoom(void)
{
    CHARREC *kid;
    CHARREC saved;

    kid = &Kid;
    *kid = Char;
    DrawKidMeter(Char.life_points, Char.life_initial);
    LoadShad(GetKidIdxAsOpp());
    saved = Char;
    Char.slot = 10;
    Char.image_bank = 0;
    Char.life_points = Kid.life_points;
    Char.life_initial = Kid.life_initial;
    Char.life_delta = Kid.life_delta;
    Kid = Char;
    Char = saved;
    RemoveChar();
    LoadKid();
}

sbyte GetKidIdxAsOpp(void)
{
    sbyte index = 0;
    sbyte count = Level.room_objects[Room.number - 1].opponent_count;

    if (count > 0) {
        do {
            if (RoomOpp[index].character_kind == 0)
                break;
            index++;
        } while (index < count);
    }
    return index;
}

int GetShadowToKidDist(void)
{
    int distance;

    LoadKidwOpp(GetKidIdxAsOpp());
    if (Opp.row == Char.row) {
        distance = Char.x - Opp.x;
        if (Opp.direction == Char.direction && Char.frame != 185)
            distance += 6 * Minus1[Char.direction + 1];
    } else {
        distance = 999;
    }
    return distance;
}

int IsShadowBlinkOn(word value, int index)
{
    int i;
    int result;

    if (index >= 4 && value > 45 && value < 52) {
        i = index;
        i -= 4;
        value -= 46;
        if (i < 2) {
            result = (value % (3 - i) == 0);
        } else {
            result = (value + 1) % 3;
        }
    } else {
        result = 0;
    }
    return result;
}

int IsShadowImbued(void)
{
    int result;

    if (Char.sequence_group == 13 || (Level.environment_kind == 6 && Char.life_points > 2))
        result = 1;
    else
        result = 0;
    return result;
}

void KidRiseFromShadowman(void)
{
    CHARREC *kid;

    if (Opp.character_kind == 0) {
        kid = &Kid;
        *kid = Char;
        LoadShad(Opp.slot);
        RemoveChar();
        SaveShad();
        LoadKid();
        Char.character_kind = 0;
        Char.image_bank = 0;
        Char.repeat = 1;
        JumpSeq(231);
        AnimChar();
        Kid = Char;
    }
}

typedef void (*ForegroundMarker)(sbyte cell, void near *rect);


void SetupShadowFlame(void)
{
    if (IsShadowImbued()) {
        SaveFChar();
        FChar.image = FrameNum % 9 + 290;
        AddFCharX(13 - ImWidth / 2);
        MakeFCharClipRect();
        AddCharObj(14);
        MarkBufsForRect(MarkFRedraw, &FChar.clip, Char.slot);
        RestoreFChar();
    }
}

void reset_shadow_overlay_state(void)
{
    Pause = 0;
}
