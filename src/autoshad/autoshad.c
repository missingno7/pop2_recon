#include "types.h"
#include "level.h"
#include "char.h"
#pragma pack(1)

typedef struct {
    signed char cell;
    int x;
    signed char facing, state;
    char reserved_5[4];
    signed char reserved_9, index, flags, health, motion;
    char reserved_e;
    signed char kind, max_health;
} Opponent;

#pragma pack()

extern CHARREC Opp, Char, Kid, RoomOpp[5];
extern LEVELREC Level;
extern signed char Minus1[2];
extern ROOMREC Room;
extern FCHARREC FChar;

extern KIDINFO KidInfo;
extern int Pause, ImWidth;
extern unsigned long FrameNum;

int abs(int value);
#pragma intrinsic(abs)

extern int far pascal AddCharX(int distance);
extern void far pascal LoadKidwOpp(signed char index);
extern void far pascal SetPalette(unsigned int palette, int first, unsigned char count, unsigned char bank);
extern int far pascal HGetSnd(int id);
extern int far pascal DecLifePts(signed char amount);
extern void far pascal JumpSeq(int sequence);
extern void far pascal AddSound(int id);
extern void far pascal AddSong(int id);
extern void far pascal SetKidPalette(signed char level);
extern int far pascal IsSpace(signed char tile);
extern signed char far pascal GetBehind(void);
extern signed char far pascal GetCellsInFront(signed char direction);
extern Opponent near *far pascal GetOppInfoIdx(int room, int index);
extern signed char far pascal FirstInRow(signed char row);
extern void far pascal AnimChar(void);
extern void far pascal SaveShad(void);
extern void far pascal LoadKid(void);
extern void far pascal DrawKidMeter(int health, int max_health);
extern void far pascal LoadShad(signed char index);
extern void far pascal RemoveChar(void);
extern void far pascal SaveFChar(void);
extern int far pascal AddFCharX(int distance);
extern void far pascal MakeFCharClipRect(void);
extern void far _fastcall AddCharObj(unsigned char effect);
extern void far pascal RestoreFChar(void);

int far pascal GetShadowToKidDist(void);
int far pascal IsShadowImbued(void);
void far pascal CreateKidAsOpp(void);
signed char far pascal GetKidIdxAsOpp(void);
int far pascal IsShadowBlinkOn(unsigned int value, int index);

int far pascal CheckShadowmanBackIntoKid(void)
{
    signed char index;
    int distance, result;
    CHARREC near *kid;

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

void far pascal CheckShadowmanCreate(void)
{
    if (Char.character_kind == 0 && Char.sequence == 5) {
        if (++KidInfo.shadow_timer >= 4) {
            if (KidInfo.shadow_timer == 4) {
                SetPalette(2000, 48, 16, IsShadowImbued() != 0);
                HGetSnd(10261);
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

extern void far pascal RestoreGamePlayPalette(void);
extern void near *far pascal GetMobTypeItem(unsigned char kind, int occurrence);

void far pascal CheckShadow(void)
{
    if (Char.room == 4 && Level.level_number == 13 && Char.sequence_group == 13) {
        RestoreGamePlayPalette();
        return;
    }
    if (Char.sequence == 242 && !GetMobTypeItem(12, 1) && Char.life_points <= 2)
        SetPalette(2000, 48, 16, 0);
}

void far pascal CheckShadowPaletteChange(void)
{
    signed char bank = -1;

    if (Kid.life_points > 2 && Kid.life_points + Kid.life_delta <= 2 && Kid.sequence != 242)
        bank = 0;
    else if (Kid.life_points <= 2 && Kid.life_points + Kid.life_delta > 2)
        bank = 1;
    if (bank != -1)
        SetPalette(2000, 48, 16, bank);
}

void far pascal CheckTurnSeqs(void)
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

void far pascal CreateKidAsOpp(void)
{
    CHARREC near *kid;
    Opponent near *opp;
    signed char index;

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
    opp->facing = Char.direction;
    opp->state = 0;
    opp->reserved_9 = 0;
    opp->index = index;
    opp->flags = 0;
    opp->motion = 0;
    opp->health = Char.life_points;
    opp->kind = 10;
    opp->max_health = Char.life_initial;
    Char.slot = index;
    Char.character_kind = 0;
    Char.image_bank = 2;
    JumpSeq(71);
    AnimChar();
    SaveShad();
    LoadKid();
}

void far pascal CutToKidRoom(void)
{
    CHARREC near *kid;
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

signed char far pascal GetKidIdxAsOpp(void)
{
    signed char index = 0;
    signed char count = Level.room_objects[Room.number - 1].opponent_count;

    if (count > 0) {
        do {
            if (RoomOpp[index].character_kind == 0)
                break;
            index++;
        } while (index < count);
    }
    return index;
}

int far pascal GetShadowToKidDist(void)
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

int far pascal IsShadowBlinkOn(unsigned int value, int index)
{
    int i;
    int result;

    if (index >= 4 && value > 45U && value < 52U) {
        i = index;
        i -= 4;
        value -= 46U;
        if (i < 2) {
            result = (value % (3 - i) == 0);
        } else {
            result = (value + 1U) % 3U;
        }
    } else {
        result = 0;
    }
    return result;
}

int far pascal IsShadowImbued(void)
{
    int result;

    if (Char.sequence_group == 13 || (Level.environment_kind == 6 && Char.life_points > 2))
        result = 1;
    else
        result = 0;
    return result;
}

void far pascal KidRiseFromShadowman(void)
{
    CHARREC near *kid;

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

typedef void (far pascal *ForegroundMarker)(signed char cell, void near *rect);
extern void far pascal MarkBufsForRect(ForegroundMarker effect, void near *rect, signed char actor);
extern void far pascal MarkFRedraw(signed char cell, void near *rect);

void far pascal SetupShadowFlame(void)
{
    if (IsShadowImbued()) {
        SaveFChar();
        FChar.image = FrameNum % 9 + 290;
        AddFCharX(13 - ImWidth / 2);
        MakeFCharClipRect();
        AddCharObj(14);
        MarkBufsForRect(MarkFRedraw, (char *)&FChar.clip.top, Char.slot);
        RestoreFChar();
    }
}

void far pascal reset_shadow_overlay_state(void)
{
    Pause = 0;
}
