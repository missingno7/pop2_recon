#include <string.h>
#include <stdlib.h>
#include "types.h"
#include "snd.h"
#include "graf.h"
#include "level.h"
#include "char.h"
#include "fade.h"
#include "game.h"
#include "collide.h"
#include "shape.h"
#include "moveruin.h"
#include "draw.h"
#include "autohead.h"
#include "custom.h"
#include "dlghelp.h"
#include "altar.h"
#include "move.h"
#include "auto.h"
#include "act.h"
#include "autoguar.h"
#include "movescyt.h"
#include "add.h"
#include "final.h"
#include "load.h"
#include "ctrl.h"
#include "prince.h"
#include "gfxerr.h"
#include "rooftops.h"
#include "anim.h"
#include "sound.h"
#include "tables.h"
#include "util.h"
#include "top.h"
#pragma pack(1)


#define ROOM_LEFT 0
#define ROOM_RIGHT 1
#define ROOM_ABOVE 2
#define ROOM_BELOW 3


#pragma pack()

#pragma pack(1)


void reset_overlay_state(void)
{
    Pause = 0;
}

void SetOppLifePts(OPPONENT *record)
{
    SetOppMaxLifePts(record);
    if ((Char.character_kind == 4 && Char.sequence == 119) ||
        (Char.character_kind == 10 && Char.sequence == 105)) {
        Char.life_points = 0;
        Char.life_delta = 0;
    } else {
        Char.life_points = Char.life_initial;
        Char.life_delta = Char.life_points;
    }
}

void SetOppMaxLifePts(OPPONENT *record)
{
    if (record->max_life == 0)
        Char.life_initial = 3;
    else
        Char.life_initial = record->max_life;
}


#define actor_index Char.slot


#define actor_room Char.room

#define actor_bank_mask Char.image_bank


void AddOpponent(void)
{
    OPPONENT *record;
    sbyte count;
    int index;

    CheckSideRoomsForVisibleOpps();
    index = 0;
    count = Level.room_objects[Room.number - 1].opponent_count;
    if (count > 0) {
        do {
            record = GetOppInfoIdx(Room.number, index);
            if (Level.preferred_opponent != 5 && Level.preferred_opponent != 6 &&
                record->kind != 1 && record->kind != 9 &&
                record->kind != 10 && record->kind != 3)
                record->kind = Level.preferred_opponent;

            Char.character_kind = OppCharId[record->kind];
            CheckLoadedOppSet(record->kind);

            actor_index = index;
            actor_room = Room.number;
            Char.row = record->cell / 10;
            if (record->cell < 0)
                --Char.row;

            if (Char.character_kind == 2 && (record->action == 100 || record->action == 100)) {
                Char.y = record->u.place.y;
            } else if (Char.character_kind == 6 && record->u.place.entry_y != 0) {
                Char.y = record->u.place.entry_y;
                if (record->action == 12) {
                    Char.horizontal_step = 2;
                    Char.vertical_step = 18;
                }
            } else {
                ResetCharY();
            }
            Char.x = record->x;
            Char.column = GetCharCol(Char.x);
            Char.direction = record->direction;
            Char.behavior = record->behavior;
            Char.opponent_index = record->behavior_type;
            Char.previous_frame = 0;
            Char.sequence_group = Char.previous_y = Char.previous_x = 0;
            Char.flame_offset = Char.alert_delay = 0;

            if (Char.character_kind == 0)
                actor_bank_mask = 2;
            else if (Char.character_kind == 6) {
                actor_bank_mask = 4;
                SetPalette(25001u, 32, 16, 7);
                if (Room.number == 6 && Level.environment_kind == 6)
                    Char.direction = Rnd(1) - 1;
            } else if (Level.preferred_opponent != 0 && Level.preferred_opponent != 5 && Level.preferred_opponent != 6)
                actor_bank_mask = 4;
            else
                PresetOppBanks(record->palette_kind);

            if (record->saved_sequence == 0) {
                if (Char.character_kind == 4)
                    record = SkelAddOpponent(record);
                else if (Char.character_kind == 7 || Char.character_kind == 8 || Char.character_kind == 11)
                    record = HeadAddOpponent(record);
                else if (Char.character_kind == 10) {
                    if (IsFloor(GetUnderFt())) {
                        record->action = 105;
                        Char.stance = 0;
                    } else {
                        record->action = 106;
                        Char.stance = 1;
                        Char.alert_delay = 3;
                        if (Char.x >= Kid.x)
                            Char.direction = -1;
                        else
                            Char.direction = 0;
                    }
                } else if (Char.character_kind == 6)
                    record->action = 2;
                else {
                    if (record->action != 22)
                        record->action = 77;
                    Char.stance = 0;
                }
                JumpSeq(record->action);
                SetOppLifePts(record);
            } else {
                Char.repeat = 1;
                Char.sequence_id = record->action;
                Char.sequence_position = record->saved_sequence;
                Char.sequence = record->action;
                Char.stance = record->combat_mode;
                Char.life_points = Char.life_delta = record->life;
                if (Char.character_kind != 0)
                    SetOppMaxLifePts(record);
            }

            AnimChar();
            if (IsDeadPos(Char.frame) && Char.character_kind != 0) {
                if (Char.character_kind == 2)
                    CheckMultipleDeadBodies();
                Char.alive = 7;
                Char.life_points = Char.repeat = 0;
                Char.life_delta = -Char.life_initial;
                if (Char.character_kind == 4 || Char.character_kind == 10)
                    SetSkelTimer(actor_index, 0);
            } else {
                Char.alive = -1;
                Refract = JustBlocked = AlertGuard = 0;
                if (Char.character_kind == 11 && Char.frame == 264)
                    Char.sequence_group = 6;
            }
            if (Char.character_kind == 6 && Char.sequence == 12)
                Char.action = 4;
            else {
                ClearCharVel();
                Char.action = Char.character_kind == 7 || Char.character_kind == 8 ? 0 : 1;
            }
            SaveShad();
        } while (count > ++index);
    }

    if (count == 0) {
        if (Level.level_number == 12 && (Room.number == 11 || Room.number == 19) &&
            LastOppTypeLoaded != 1)
            LoadOppSet(1);
        else if (Level.generators[Room.number - 1].count != 0 &&
                 LastOppTypeLoaded != Level.preferred_opponent &&
                 Level.preferred_opponent != 5 && Level.preferred_opponent != 6)
            LoadOppSet(Level.preferred_opponent);
    }
}

void CheckSideRoomsForVisibleOpps(void)
{
    int index, side, shift;
    byte room;
    sbyte count;
    OPPONENT *record;
    OPPONENT copy;

    for (side = 0; side < 2; ++side) {
        if (side == 0) {
            room = Room.left;
            shift = -320;
        } else {
            room = Room.right;
            shift = 320;
        }
        if (room == 0)
            continue;
        count = Level.room_objects[room - 1].opponent_count;
        for (index = 0; index < count; ++index) {
            record = GetOppInfoIdx(room, index);
            if ((side == 0 && record->x > 367) || (side == 1 && record->x < 208) ||
                ((room == 7 || room == 8) && Level.environment_kind == 6 && record->kind == 3)) {
                copy = *record;
                copy.x += shift;
                copy.index = Level.room_objects[Room.number - 1].opponent_count;
                AddOppToRoom(Room.number, &copy);
                RemoveOppFromRoom(room, index);
                --index;
                --count;
            }
        }
    }
}


int UpdateOppInfo(void)
{
    int removed = 0;
    OPPONENT *record, *prior;
    byte room;
    OPPONENT copy;

    if (Char.direction == 86 || Char.sequence_group == 5 || Char.sequence_group == 4 || Char.sequence == 196) {
        RemoveOppFromRoom(actor_room, actor_index);
        removed = 1;
    } else {
        record = GetOppInfoIdx(actor_room, actor_index);
        if (record != 0) {
            record->cell = IndexCell(Char.column, Char.row);
            if (record->cell != 30) {
                record->x = Char.x;
                record->direction = Char.direction;
                if (Char.alive < 0)
                    record->saved_sequence = removed;
                else {
                    record->saved_sequence = Char.sequence_position;
                    record->action = Char.sequence_id;
                }
                record->combat_mode = Char.stance;
                record->life = Char.life_points;
                record->kind = CharOppId[Char.character_kind];
                SetSpecialOppInfo(record);
                room = GetUnderFt();
                if (room == 5 || room == 6)
                    JamPlate();
            } else {
                room = RegisterCharInRoom();
                if (room != 0 && Char.character_kind != 12) {
                    prior = GetOppInfoIdx(actor_room, actor_index);
                    copy.cell = FirstInRow(Char.row);
                    copy.x = Char.x;
                    copy.direction = Char.direction;
                    copy.skill = prior->skill;
                    copy.action = Char.sequence_id;
                    copy.saved_sequence = Char.sequence_position;
                    copy.palette_kind = prior->palette_kind;
                    copy.combat_mode = Char.stance;
                    copy.life = Char.life_points;
                    copy.behavior = Char.behavior;
                    copy.behavior_type = Char.opponent_index;
                    copy.kind = prior->kind;
                    copy.max_life = prior->max_life;
                    copy.u = prior->u;
                    AddOppToRoom(room, &copy);
                } else if (Char.action == 9 && Level.environment_kind == 5) {
                    ResetSnd(10025);
                    if (Room.number != 19 && Room.number != 16)
                        AddSound(0);
                }
                RemoveOppFromRoom(actor_room, actor_index);
                removed = 1;
                if (Level.room_objects[Room.number - 1].opponent_count == 0)
                    RoomOpp[actor_index].direction = 86;
            }
        }
        Char.life_points = 0;
    }
    return removed;
}

void TransferOppInfo(void)
{
    OPPONENT copy;

    OPPONENT *record;

    record = GetOppInfoIdx(actor_room, actor_index);
    copy = *record;
    RemoveOppFromRoom(actor_room, actor_index);
    Cut(CutDir);
    copy.cell = FirstInRow(Char.row);
    copy.x = Char.x;
    copy.direction = Char.direction;
    copy.action = Char.sequence_id;
    copy.saved_sequence = Char.sequence_position;
    copy.combat_mode = Char.stance;
    copy.life = Char.life_points;
    copy.palette_kind = Char.image_bank == 4 ? OppBank[0] : OppBank[1];
    SetSpecialOppInfo(&copy);
    AddOppToRoom(actor_room, &copy);
}

void SetSpecialOppInfo(OPPONENT *record)
{
    if (Char.character_kind == 2) {
        if ((Char.sequence == 186) | (Char.sequence == 100))
            record->u.place.y = Char.y;
        if (Level.preferred_opponent == 7) {
            record->kind = 7;
            return;
        }
    } else if (Char.character_kind == 11) {
        SnakeUpdateOppInfo(record);
        return;
    } else if (Char.character_kind == 6) {
        JaffarUpdateOppInfo(record);
    }
}

void RemoveOppFromRoom(byte room, int index)
{
    sbyte count = Level.room_objects[room - 1].opponent_count;
    OPPONENT *record, *following;

    if (count > index) {
        if (count > index + 1) {
            do {
                record = GetOppInfoIdx(room, index);
                following = GetOppInfoIdx(room, index + 1);
                *record = *following;
                record->index = index;
                if (Room.number == room) {
                    RoomOpp[index] = RoomOpp[index + 1];
                    RoomOpp[index].slot = index;
                }
            } while (++index + 1 < count);
        }
        --Level.room_objects[room - 1].opponent_count;
    }
}

void AddOppToRoom(int room, OPPONENT *record)
{
    ROOMOBJECTS *list = &Level.room_objects[room - 1];
    sbyte index = list->opponent_count;

    if (index < 5) {
        record->index = index;
        ++Level.room_objects[room - 1].opponent_count;
        *GetOppInfoIdx(room, index) = *record;
    }
}

OPPONENT *GetOppInfoIdx(int room, int index)
{
    OPPONENT *result;

    if (Level.room_objects[room - 1].opponent_count > index)
        result = &Level.room_objects[room - 1].opponents[index];
    else
        result = 0;
    return result;
}

sbyte GetOppIdxClosestToKid(sbyte direction)
{
    sbyte best = -1, index, count;
    int best_dx, dx, best_dy, dy;
    CHARREC *actor;
    OPPONENT *record;

    if (Kid.room != 0) {
        index = 0;
        count = Level.room_objects[Kid.room - 1].opponent_count;
        for (; index < count; ++index) {
            record = GetOppInfoIdx(Room.number, index);
            actor = &RoomOpp[index];
            dy = abs(actor->y - Kid.y);
            if (actor->alive < 0 && actor->character_kind != 11 &&
                !((actor->character_kind == 7 || actor->character_kind == 8) && record->u.head.mode == 2)) {
                if (direction == 1)
                    dx = abs(actor->x - Kid.x);
                else if (direction == -1 && actor->x <= Kid.x)
                    dx = Kid.x - actor->x < 0 ? 0 : Kid.x - actor->x;
                else if (direction == 0 && actor->x > Kid.x)
                    dx = actor->x - Kid.x < 0 ? 0 : actor->x - Kid.x;
                else
                    dy = -1;
            } else
                dy = -1;

            if (dy >= 0 && dx >= 0) {
                if (best != -1) {
                    int difference;
                    best_dx = abs(RoomOpp[best].x - Kid.x);
                    best_dy = abs(RoomOpp[best].y - Kid.y);
                    difference = abs(dy - best_dy);
                    if (!(difference > 42 && dy < best_dy) &&
                        (difference >= 42 || dx >= best_dx))
                        continue;
                }
                best = index;
            }
        }
    }
    return best;
}

int IsCharClosestToKid(void)
{
    if (GetOppIdxClosestToKid(Kid.x >= Char.x ? -1 : 0) == actor_index)
        return 1;
    return 0;
}

void CheckOppGenPts(byte room)
{
    GENERATOR *point;

    if (Kid.alive < 0 && Kid.character_kind != 1) {
        point = GetOppGenPtOnRow(Kid.room, Kid.row);
        if (point && IsTimeToGenOpp(point, room)) {
            if (point->delay == 0) {
                GenerateOpponent(point, room);
                point->delay = point->reset_delay;
                --point->enabled;
            } else
                --point->delay;
        }
    }
}

void GenerateOpponent(GENERATOR *point, byte room)
{
    sbyte index = Level.room_objects[room - 1].opponent_count;
    sbyte row;
    OPPONENT *record;

    if (index < 5) {
        ++Level.room_objects[room - 1].opponent_count;
        record = GetOppInfoIdx(room, index);
        record->cell = FirstInRow(point->row) + point->column;
        if (point->column <= 5) {
            record->x = ColEdgeX[point->column + 3];
            record->direction = 0;
        } else {
            record->x = ColEdgeX[point->column + 7];
            record->direction = -1;
        }
        record->skill = Level.generators[room - 1].spawn_flags;
        record->palette_kind = OppBank[0];
        record->index = index;
        record->combat_mode = 0;
        record->behavior = point->behavior;
        record->u.skeleton.row = point->alternate_row;
        record->max_life = record->life = point->flags & 15;
        actor_index = index;
        Char.direction = record->direction;
        Char.x = record->x;
        Char.character_kind = OppCharId[Level.preferred_opponent];
        Char.column = point->column;
        Char.row = point->row;
        Char.behavior = point->behavior;
        actor_room = room;
        Char.life_delta = Char.life_points = Char.life_initial = record->life;
        Char.alive = -1;
        Char.image_bank = 4;
        ResetCharY();
        ClearCharVel();
        Char.sequence_group = 0;
        Char.repeat = 1;
        row = Kid.column;
        if (Kid.direction != Char.direction &&
            ((Kid.frame >= 1 && Kid.frame <= 14) ||
             (Kid.frame >= 49 && Kid.frame <= 56) ||
             (Kid.frame >= 34 && Kid.frame <= 44)))
            row -= Minus1[Kid.direction + 1];
        if (point->column + 3 >= row && point->column - 3 <= row &&
            (point->flags & 15) >= 6) {
            Char.stance = 1;
            JumpSeq(90);
            Char.alert_delay = 3;
        } else {
            Char.stance = 0;
            JumpSeq(84);
            Char.alert_delay = 0;
        }
        AnimChar();
        if (Kid.opponent_index == -1)
            Kid.opponent_index = actor_index;
        SaveShad();
    }
}


void *GetOppGenPtOnRow(byte room, sbyte column)
{
    sbyte index = 0;
    GENERATOR *point = 0;
    sbyte count = Level.generators[room - 1].count;

    for (; index < count; ++index) {
        point = GetOppGenPtIdx(room, index);
        if (IsActiveOppGenPt(point, column))
            break;
        point = 0;
    }
    return point;
}

int IsActiveOppGenPt(GENERATOR *point, sbyte column)
{
    int result;

    if ((point->row == column || point->alternate_row == column) &&
        abs(point->column - Kid.column) > 2 && point->enabled != 0)
        result = 1;
    else
        result = 0;
    return result;
}


int IsTimeToGenOpp(GENERATOR *point, byte room)
{
    sbyte count = Level.room_objects[room - 1].opponent_count;
    sbyte index, room_row;
    byte between;
    int allowed;

    if (count < 5 && LevelIdx == NextLevel && FrameNum % 3 != 0) {
        allowed = 1;
        index = 0;
        between = 0;
        room_row = Get1stWallColInDir(room, point->column, point->row,
                                 point->column >= 5 ? -1 : 1);
        if (count > 0) {
            do {
                if (!allowed)
                    break;
                if (RoomOpp[index].alive < 0 &&
                    ((point->column < Kid.column && RoomOpp[index].x < Kid.x) ||
                     (point->column > Kid.column && RoomOpp[index].x > Kid.x)) &&
                    ((point->column < room_row && Kid.column < room_row) ||
                     (point->column > room_row && Kid.column > room_row))) {
                    ++between;
                    if (point->limit <= between ||
                        (point->column < Kid.column && RoomOpp[index].column <= point->column) ||
                        (point->column > Kid.column && RoomOpp[index].column >= point->column))
                        allowed = 0;
                }
                ++index;
            } while (index < count);
        }
    } else
        allowed = 0;
    return allowed;
}

GENERATOR *GetOppGenPtIdx(byte room, sbyte index)
{
    GENERATOR *point;

    if (room && Level.generators[room - 1].count > index)
        point = &Level.generators[room - 1].points[index];
    else
        point = 0;
    return point;
}

int IsOppGenPtWithTumbleOn(byte room)
{
    int found = 0;
    sbyte count = Level.generators[Char.room - 1].count;
    sbyte index = 0;
    GENERATOR *point;

    for (; index < count && !found; ++index) {
        point = GetOppGenPtIdx(room, index);
        if (point && (point->flags & 128)) found = 1;
        else found = 0;
    }
    return found;
}


void _fastcall PresetOppBanks(byte kind)
{
    int slot;

    if (Level.preferred_opponent != 0 && Level.preferred_opponent != 5 && Level.preferred_opponent != 6) {
        Char.image_bank = Level.preferred_opponent == 2 ? 8 : 4;
        return;
    }
    Char.image_bank = 0;
    if (kind == 0)
        kind = 1;

    for (slot = 0; slot < 2 && Char.image_bank == 0; ++slot) {
        if (OppBank[slot] == kind)
            Char.image_bank = 4 << slot;
    }
    if (Char.image_bank == 0) {
        for (slot = 0; slot < 2 && Char.image_bank == 0; ++slot) {
            if (OppBank[slot] == 0) {
                OppBank[slot] = kind;
                Char.image_bank = 4 << slot;
            }
        }
    }
    if (Char.image_bank == 0)
        Char.image_bank = 4;
}

void SetOppBanks(void)
{
    int slot;
    int start;

    if (Level.preferred_opponent != 0 && Level.preferred_opponent != 5 && Level.preferred_opponent != 6) {
        if (Level.preferred_opponent == 2) {
            SetPalette(750, 48, 16, 0); return;
        }
    } else {
        for (slot = 0, start = 32; start < 64; ++slot, start += 16) {
            if (OppBank[slot] != 0) SetPalette(750, start, 16, OppBank[slot] - 1);
        }
    }
}


void CutOpponent(void)
{
    sbyte index = 0;
    sbyte count = Level.room_objects[Room.number - 1].opponent_count;

    for (; index < count; ++index) {
        LoadShad(index);
        if (Char.direction == 86 || Char.y < 384)
            continue;
        if (Level.environment_kind == 5 && InWater.character == actor_index)
            continue;

        if (Char.action != 4 && Char.action != 3 &&
            (Char.frame < 202u || Char.frame > 212u)) {
            if (!UpdateOppInfo())
                SaveShad();
        } else if (!Room.below) {
            UpdateOppInfo();
            if (Char.character_kind == 2 && !SndOn(10025))
                AddSound(25);
        }

        AddOppDeathSong();
        DrawOppMeter(actor_index, 0, Char.life_points);
        if (Kid.opponent_index == actor_index)
            Kid.opponent_index = -1;
    }
}

void CutCheck(void)
{
    if (CutTimer) {
        --CutTimer;
        return;
    }

    LoadKid();
    SetupChar();
    GetCharEdges();
    CutDir = CutChar();

    if (CutDir != -1 && actor_room != 0) {
        Kid = Char;
        CutRoom = actor_room;
        CutAllOpponents();
        return;
    }

    if (actor_room == 0 && Char.alive < 0) {
        if (Level.environment_kind == 5 && CutDir == 0) {
            Char.x = AddCharX(320);
            RoofTriggerCharInWater();
        } else {
            AddKidScream(Room.number);
            if (Level.environment_kind != 5) {
                DecLifePts(100);
                Char.frame = 185;
                SetKidDeathMethod(3);
                Kid = Char;
                ChangeMeters();
                LoadKid();
            }
        }
    }
    Kid = Char;
}

void CutAllOpponents(void)
{
    sbyte index = 0;
    sbyte count = Level.room_objects[Room.number - 1].opponent_count;
    int stay, removed;

    for (; index < count; ++index) {
        LoadShad(index);
        if (Char.direction == 86)
            continue;

        if ((((Char.frame >= 202 && Char.frame <= 212 && Char.behavior == 1) ||
              (Char.frame >= 186 && Char.frame <= 201 && Char.behavior != 0)) &&
             (Char.character_kind == 2 || Char.character_kind == 4)) ||
            IsCharInKidRoom() ||
            (Char.row && Level.level_number == 5 &&
             (actor_room == 10 || actor_room == 7 || actor_room == 12)) ||
            Char.character_kind == 0) {
            stay = 0;
        } else if (Char.character_kind == 6 &&
                   (actor_room == 7 || actor_room == 8) &&
                   Level.environment_kind == 6 &&
                   (CutRoom == 7 || CutRoom == 8) &&
                   Level.environment_kind == 6) {
            stay = 0;
        } else if (Char.alive >= 0 || !CanOppFollowKid()) {
            stay = 1;
        } else {
            if (CutDir == 0) {
                stay = Char.x >= 208;
            } else if (CutDir == 1) {
                stay = Char.x < 371;
            } else if (CutDir == 2) {
                stay = Char.row >= 0;
            } else {
                stay = Char.row < 3;
            }
        }

        if (stay) {
            removed = UpdateOppInfo();
        } else {
            TransferOppInfo();
            removed = 1;
        }
        if (removed) {
            --index;
            --count;
        }
    }
}

int CanOppFollowKid(void)
{
    int allowed;

    if (Level.room_objects[Kid.room - 1].opponent_count < 5)
        allowed = 1;
    else
        allowed = 0;
    if (allowed) {
        if (Char.stance == 1 || Char.character_kind == 7 || Char.character_kind == 8)
            allowed = 1;
        else
            allowed = 0;
        if (allowed) {
            allowed = (Char.character_kind != 10);
            if (allowed) return allowed;
        }
    }
    return allowed;
}

int IsCharInKidRoom(void)
{
    CHARREC saved;
    byte destination;

    saved = Char;
    destination = RegisterCharInRoom();
    Char = saved;

    if (Kid.room == destination)
        return 1;
    return 0;
}

sbyte RegisterCharInRoom(void)
{
    byte room = actor_room;
    byte previous;

    if (Char.column < 0 || Char.column >= 10 || Char.row < 0 || Char.row >= 3) {
        do {
        previous = room;
        if (Char.row < 0) {
            room = Level.room_links[room - 1].above;
            if (room) {
                Char.y += 192;
                Char.row += 3;
            } else {
                room = previous;
                previous = 0;
            }
        } else if (Char.row >= 3) {
            room = Level.room_links[room - 1].below;
            if (room) {
                Char.y -= 192;
                Char.row -= 3;
            } else {
                room = previous;
                previous = 0;
            }
        }

        if (Char.column < 0) {
            room = Level.room_links[room - 1].left;
            if (room) {
                Char.x += 320;
                Char.column += 10;
            }
        } else if (Char.column >= 10) {
            room = Level.room_links[room - 1].right;
            if (room) {
                Char.x -= 320;
                Char.column -= 10;
            }
        } else if (!previous)
            room = 0;

        } while (room && (Char.column < 0 || Char.column >= 10 || Char.row < 0 || Char.row >= 3));
    }
    return room;
}

int Cut(int direction)
{
    actor_room = (&Level.room_links[actor_room - 1].left)[direction];
    if (direction == 0) {
        Char.x += 320;
        Char.column = GetCharCol(Char.x);
        direction = 1;
    } else if (direction == 1) {
        Char.x -= 320;
        Char.column = GetCharCol(Char.x);
        direction = 0;
    } else if (direction == 2) {
        Char.y += 189;
        Char.row = GetRow(Char.y);
        direction = 3;
    } else {
        Char.y -= 189;
        Char.row = GetRow(Char.y);
        direction = 2;
    }
    return direction;
}


int CutChar(void)
{
    int y = Char.y;
    int mode = Char.action;
    word action = Char.frame;
    int side = -1;

    if ((Room.number == 7 || Room.number == 8) && Level.environment_kind == 6 &&
        (RoomOpp[0].sequence == 243 || RoomOpp[1].sequence == 243)) {
        side = FinlGetCutDirToJaffarBurning();
    } else if (LevelIdx == NextLevel && Char.sequence != 70 &&
               (Char.character_kind != 1 || Char.frame != 185)) {
        if (mode != 5 && mode != 4 && mode != 3 && y < 10 && y >= -16) {
            if (Level.level_number != 5 || actor_room != 3)
                side = 2;
        } else if (y >= 231 &&
                   (Level.room_links[actor_room - 1].below || y >= ImHeight + 231) &&
                   (Level.environment_kind != 5 || actor_index != InWater.character)) {
            side = 3;
        } else if ((action >= 135 && action < 150) ||
                   (action >= 110 && action <= 119) ||
                   (action >= 150 && action < 163) ||
                   (action >= 166 && action < 169) || mode == 7) {
            side = -1;
        } else if (Char.direction == -1) {
            if (LeftEdge <= 125 &&
                (Level.room_links[actor_room - 1].left || LeftEdge <= 125 - ImWidth)) {
                if (Level.level_number == 9 && actor_room == 8 && Char.row == 0 && Char.frame == 40)
                    ++NextLevel;
                else
                    side = 0;
            } else if (LeftEdge >= 450 &&
                       (Level.room_links[actor_room - 1].right || LeftEdge >= ImWidth + 449) &&
                       (Level.environment_kind != 1 || actor_room != 3)) {
                side = 1;
            }
        } else {
            ReadCell(actor_room, 9, Char.row);
            if (RightEdge >= 454 &&
                (Level.room_links[actor_room - 1].right || RightEdge >= ImWidth + 454) &&
                (Level.environment_kind != 1 || actor_room != 3)) {
                side = 1;
            } else if (RightEdge <= 129 &&
                       (Level.room_links[actor_room - 1].left || RightEdge <= 130 - ImWidth)) {
                side = 0;
            }
        }
    }
    switch (side) {
      case 0:
        if (actor_room == 1 && Level.level_number == 2)
            side = -1;
        break;
      case 1:
        if (Level.environment_kind == 5 && actor_room == 15) {
            side = -1;
            if (Char.sequence != 71)
                RoofKillKidOnWharf();
        } else if (actor_room == 3 && Level.environment_kind == 6)
            side = -1;
        break;
      case 3:
        if ((actor_room == 11 && Level.environment_kind == 5) ||
            (actor_room == 3 && Level.environment_kind == 6) ||
            (actor_room == 5 && Level.environment_kind == 6) ||
            (Level.level_number == 5 && actor_room == 10) ||
            (actor_room == 4 && Level.level_number == 13) ||
            (Level.level_number == 13 && actor_room == 13) ||
            (((actor_room == 16 && Level.level_number == 9) ||
              (actor_room == 27 && Level.level_number == 6)) &&
             (Char.sequence == 68 || Char.sequence == 25 || Char.sequence == 10))) {
            side = -1;
            if ((Level.level_number == 5 && actor_room == 10) ||
                (actor_room == 4 && Level.level_number == 13) ||
                (Level.level_number == 13 && actor_room == 13)) {
                DecLifePts(100);
                actor_room = 0;
            }
        }
        break;
    }

    if (side != -1)
        Cut(side);
    if (side == 0 && Room.number == 27 && Level.level_number == 6)
        CutKidInsideLevel6();
    return side;
}

void AutoCtrl(void)
{
    sbyte kind;

    AutoRelease();
    kind = Char.character_kind;
    if (JustBlocked)
        --JustBlocked;
    if (GuardTimer)
        --GuardTimer;
    if (Refract)
        --Refract;

    switch (kind) {
      default:
        if (Char.character_kind != 0) GuardProg();
        return;
      case 1:
        ShadowProg(); return;
      case 4:
      case 10:
        SkelProg();
        return;
      case 6:
        JaffarProg();
        return;
      case 7:
      case 8:
        HeadProg();
        return;
      case 11:
        SnakeProg();
        return;
    }
}


void AutoRelease(void)
{
    memset(&Dir, 0, sizeof(Dir));
    memset(&Joy, 0, sizeof(Joy));
}

void ShadowProg(void)
{
    if (actor_room == 4 && Level.level_number == 13)
        AltarShadowProg();
}

void GuardProg(void)
{
    if (Char.alive < 0 && Char.life_points != 0 &&
        (Char.life_points + Char.life_delta) != 0) {
        if (Char.stance == 0) {
            if (Char.frame >= 186u && Char.frame <= 212u)
                OppChasing();
            else
                OppAlert();

            if (AlertGuard != 0 &&
                Level.room_objects[actor_room - 1].opponent_count - actor_index == 1) {
                AlertGuard = 0;
                return;
            }
        } else if (Kid.sequence_group != 13) {
            EnGarde();
            return;
        } else if (Level.sword_overlays[2] != 0 && Char.sequence != 110) {
            JumpSeq(110);
        }
    }
}


static int opponent_recovery_time[] = {
    20, 20, 20, 20, 10, 10, 10, 10, 0, 10, 0
};


void CheckStab(void)
{
    sbyte index = 0;
    sbyte count = Level.room_objects[Room.number - 1].opponent_count;

    for (; index < count; ++index) {
        LoadShad(index);
        if (Char.action == 99) {
            LoadShadwOpp(index);
            if (Opp.action == 99)
                Opp.action = 1;
            StabChar();
            Refract = opponent_recovery_time[
                Level.room_objects[actor_room - 1].opponents[actor_index].skill];
            SaveShadwOpp();
        }
    }

    if (Kid.action == 99) {
        for (index = 0; index < count; ++index) {
            LoadShad(index);
            if (Char.character_kind == 7 || Char.character_kind == 8) {
                if (Char.frame == 167 || Char.frame == 181)
                    break;
            } else if (IsTheStrikePos(Char.character_kind, Char.frame))
                break;
        }
        if (index < count) {
            LoadKidwOpp(index);
            StabChar();
            SaveKidwOpp();
        }
    }
}

void StabChar(void)
{
    sbyte damage;
    int next_action;
    int sound, default_action;

    if (Char.alive >= 0)
        return;
    if (Char.character_kind == 0 && Char.sequence == 5 && (Level.environment_kind == 2 || Level.environment_kind == 6))
        SetKidPalette(Level.environment_kind);

    do {
        if ((byte)Char.stance == 255 || Char.sequence == 10 || Char.sequence == 16 ||
            Char.sequence == 28 || Char.sequence == 14) {
            DecLifePts(100);
            StabKillChar();
        } else {
            if (Char.character_kind == 7 || Char.character_kind == 8) {
                next_action = 153;
                damage = KidInfo.sword_kind == 2 ? 1 : 2;
                if (damage < Char.life_points)
                    HeadSetToFleeMode();
            } else if (Char.character_kind == 11 || Opp.character_kind == 11) {
                damage = 100;
                next_action = default_action;
            } else {
                damage = (Char.character_kind == 6 || Opp.character_kind == 6) ? 100 : 1;
                next_action = Opp.direction != Char.direction ? 74 : 94;
                if (Opp.character_kind == 7 || Opp.character_kind == 8) {
                    if (Char.stance != 1 &&
                        !(KidFrames[Char.frame].flags & 64))
                        next_action = 45;
                    else if (Char.frame >= 246u && Char.frame <= 261u) {
                        if (IsWall(GetBehind()))
                            Char.x = AddCharX(16);
                        else if (IsWall(GetCellsInFront(1)))
                            Char.x = AddCharX(-16);
                    }
                }
                if (Char.character_kind == 0 && next_action != 45)
                    Char.stance = 1;
            }

            if (!DecLifePts(damage) && Char.character_kind != 11) {
                JumpSeq(next_action);
                ResetCharY();
                Char.vertical_step = 0;
                break;
            }
        }

        GetFrameInfo();
        next_action = GetDist();
        if ((IsSpace(GetUnderFt()) ||
             (IsSpace(GetBehind()) && next_action >= 4)) &&
            Char.character_kind != 7 && Char.character_kind != 8 && Char.character_kind != 11 && Char.character_kind != 6) {
            next_action -= 8;
            if (Cell.column != Char.column)
                next_action -= 32;
            Char.x = AddCharX(next_action);
            RereadCells();
            ++Char.row;
            JumpSeq(81);
        } else {
            StabKillChar();
            ResetCharY();
            Char.vertical_step = 0;
        }
    } while (0);

    if (Opp.character_kind != 7 && Opp.character_kind != 8 && Opp.character_kind != 11) {
        if (Char.character_kind == 0)
            sound = Opp.character_kind == 10 ? 194 : 14;
        else if (Char.character_kind == 4)
            sound = 72;
        else if (Char.character_kind == 7 || Char.character_kind == 8)
            sound = 86;
        else if (Char.character_kind == 11)
            sound = 70;
        else
            sound = 21;
        AddSound(sound);
    }
    AnimChar();
    if (Char.sequence == 85) {
        RereadCells();
        damage = GetUnderFt();
        if (IsSpace(damage) || IsMobFloor(damage)) {
            Char.x = AddCharX(-24);
            RereadCells();
        }
        PutDeadBodyOnSolidGround();
    }
}

void StabKillChar(void)
{
    int next_action;
    sbyte index;

    Char.alert_delay = 0;
    Char.repeat = 1;
    next_action = -1;
    if (Char.character_kind == 4) {
        BonesFall();
    } else if (Char.character_kind == 7 || Char.character_kind == 8) {
        Char.direction = ~Kid.direction;
        Char.life_points = 0;
        next_action = 154;
    } else if (Char.character_kind == 11) {
        next_action = 174;
    } else if (Char.character_kind == 6) {
        next_action = 196;
    } else {
        index = GetBehind();
        if (Char.character_kind != 0 && Char.direction == -1 &&
            (index == 4 || GetUnderFt() == 4)) {
            index = GetCellCol();
            Char.x = ColEdgeX[index - (Cell.kind != 4) + 5] + 14;
            Char.x = AddCharX(20);
        }

        if (Char.character_kind == 0 && Char.frame >= 246u && Char.frame <= 261u) {
            next_action = 98;
            if (Opp.character_kind == 11)
                AddSound(69);
            else if (Opp.character_kind == 7 || Opp.character_kind == 8)
                AddSound(89);
            else
                AddSound(14);
        } else if (Opp.character_kind == 11) {
            next_action = 97;
            AddSound(69);
        } else {
            if (Opp.character_kind == 6)
                AllOtherJaffarsShimmerAway();
            next_action = 85;
            if (Char.character_kind != 0 && DoOppTumbleSeq()) {
                next_action = 185;
                Char.x = AddCharX(-8);
            }
        }
        if (Char.character_kind == 0) {
            switch (Opp.character_kind) {
              case 2:
                index = 10 + ((Level.preferred_opponent == 0) ? 4 : 0);
                break;
              case 6:
                index = 17;
                break;
              case 7:
              case 8:
                index = 6;
                break;
              case 10:
                index = 12;
                break;
              case 11:
                index = 7;
                break;
              default:
                index = 2;
                break;
            }
            SetKidDeathMethod(index);
        }
    }
    if (Opp.character_kind == 0 || Opp.character_kind == 1) {
        SaveShadwOpp();
        Opp.opponent_index = GetOppIdxClosestToKid(Opp.direction);
    }
    if (next_action != -1)
        JumpSeq(next_action);
}

void CheckStrike(void)
{
    sbyte index, count;

    if (Kid.frame != 0 && (Kid.frame < 219 || Kid.frame >= 229)) {
        index = 0;
        count = Level.room_objects[Room.number - 1].opponent_count;
        for (; index < count; ++index) {
            LoadShadwOpp(index);
            TestStrike();
            SaveShadwOpp();
            LoadKidwOpp(index);
            TestStrike();
            SaveKidwOpp();
        }
    }
}

void TestStrike(void)
{
    int distance, minimum, maximum;

    if (Char.stance != 1 || Char.row != Opp.row ||
        (abs(Char.y - Opp.y) >= 7 &&
         Char.character_kind != 7 && Char.character_kind != 8 && Opp.character_kind != 7 && Opp.character_kind != 8) ||
        !IsStrikingPos(Char.character_kind, Char.frame) || Opp.alive >= 0 ||
        Opp.character_kind == 10 || Opp.character_kind == 1 || Opp.action == 8 ||
        Opp.sequence == 110)
        return;
    if (Char.character_kind == 1 && Opp.character_kind == 0)
        return;

    distance = GetOppDist();
    if (Char.character_kind == 7 || Char.character_kind == 8) {
        minimum = 0;
        maximum = 28;
    } else {
        minimum = 0;
        maximum = 44;
    }

    if (Char.direction != Opp.direction && distance >= 0 && distance <= maximum &&
        Opp.character_kind != 7 && Opp.character_kind != 8 && Opp.character_kind != 11 &&
        Char.character_kind != 11 && (Opp.frame == 161 || Opp.frame == 150)) {
        Opp.frame = 161;
        if (Char.character_kind == 0)
            Char.opponent_index = Opp.slot;
        else {
            Opp.opponent_index = actor_index;
            JustBlocked = 4;
        }

        if (Char.character_kind == 7 || Char.character_kind == 8)
            distance = HeadBlocked();
        else if (Char.character_kind == 10)
            distance = FlameSwordBlocked();
        else
            distance = 69;
        if (distance != -1)
            JumpSeq(distance);
        AnimChar();
    } else if (IsTheStrikePos(Char.character_kind, Char.frame)) {
        GetCharStrikeRanges(&minimum, &maximum);
        if (distance >= minimum && distance <= maximum) {
            Opp.action = 99;
            if (Char.character_kind == 0)
                Char.opponent_index = Opp.slot;
        }
    } else if ((Char.character_kind == 7 || Char.character_kind == 8) && Char.sequence_group == 1 && JustBlocked == 0)
        Opp.action = 99;

    if (IsTheStrikePos(Char.character_kind, Char.frame) &&
        (Opp.frame != 161 || Opp.character_kind == 7 || Opp.character_kind == 8) &&
        Opp.action != 99) {
        if (Char.character_kind == 10)
            distance = 193;
        else
            distance = 13;
        AddSound(distance);
    }
}

void _fastcall GetCharStrikeRanges(int *minimum, int *maximum)
{
    *minimum = 0;
    *maximum = -1;

    if (Char.character_kind != 0 && Char.character_kind != 1) {
        if (Char.character_kind == 10) {
            *minimum = -10;
            *maximum = 38;
        } else if (Char.character_kind == 11) {
            if (63 * Opp.row + 51 <= Opp.y) {
                if (Opp.direction == Char.direction) {
                    *minimum = -2;
                    *maximum = 27;
                } else {
                    *minimum = -27;
                    *maximum = -4;
                    return;
                }
            }
        } else if (Char.character_kind == 6) {
            *minimum = 13;
            *maximum = 53;
            return;
        } else if (Opp.direction != Char.direction) {
            *minimum = 19;
            *maximum = 46;
            if (Char.character_kind == 4) {
                *maximum = 48;
                return;
            }
        } else if (Opp.stance == 1) {
            *minimum = 34;
            *maximum = 57;
            return;
        } else {
            *minimum = 22;
            *maximum = 42;
            return;
        }
    } else {
        if (Opp.character_kind == 11) {
            if (Opp.sequence_group != 6 && Opp.sequence != 171 && Char.sequence == 96) {
                if (Opp.direction == Char.direction) {
                    *minimum = 31;
                    *maximum = 75;
                } else {
                    *minimum = -7;
                    *maximum = 45;
                }
            }
        } else if (Opp.character_kind == 7 || Opp.character_kind == 8) {
            *minimum = 19;
            *maximum = 46;
        } else if (Opp.character_kind == 6) {
            if (Opp.direction != Char.direction) {
                *minimum = 13;
                *maximum = 57;
            } else {
                *minimum = 13;
                *maximum = 53;
            }
        } else {
            *minimum = 19;
            *maximum = 46;
            if (!Opp.alert_delay) {
                *minimum += 14;
                *maximum += 14;
            }
            if (Opp.character_kind == 4)
                *maximum += 2;
        }
        if (KidInfo.sword_kind == 2)
            *maximum -= 5;
    }
}

int IsStrikingPos(sbyte kind, word action)
{
    int result;

    if (kind == 7 || kind == 8) {
        result = (action >= 165 && action <= 167) || action == 181;
    } else if (kind == 10) {
        result = action == 223 || action == 224;
    } else if (kind == 11) {
        result = Char.sequence == 173 && (action == 274 || action == 280);
    } else {
        result = action == 153 || action == 154;
        if (!result && kind == 0)
            result = action == 244 || action == 245;
    }
    return result;
}

int IsTheStrikePos(sbyte kind, int value)
{
    int result;

    if (kind == 10) {
        if (value == 224)
            result = 1;
        else
            result = 0;
    } else if (kind == 11) {
        if (Char.sequence == 173 && value == 280)
            result = 1;
        else
            result = 0;
    } else {
        result = value == 154;
        if (!result && kind == 0)
            result = value == 245;
    }
    return result;
}

int IsAdvancableId(sbyte tile, word modifier)
{
    int allowed;

    if (!IsWall(tile) && (tile != 4 || (byte)modifier >= 112))
        allowed = 1;
    else
        allowed = 0;
    if (allowed && Char.character_kind != 10) {
        allowed = !IsImpassable(tile);
        if (allowed && Char.character_kind != 7 && Char.character_kind != 8)
            allowed = !IsSpace(tile);
    }
    return allowed;
}

int unused_auto_flag = 0;                            /* never referenced */
