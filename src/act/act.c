#include <stdlib.h>
#include "types.h"
#include "resc.h"
#include "snd.h"
#include "graf.h"
#include "shape.h"
#include "level.h"
#include "char.h"
#include "game.h"
#include "auto.h"
#include "draw.h"
#include "custom.h"
#include "movecave.h"
#include "act.h"
#include "heap.h"
#include "autoguar.h"
#include "autoshad.h"
#include "bridge.h"
#include "collide.h"
#include "rooftops.h"
#include "autohead.h"
#include "custcvca.h"
#include "drawruin.h"
#include "fade.h"
#include "movescyt.h"
#include "ctrl.h"
#include "load.h"
#include "final.h"
#include "move.h"
#include "moveruin.h"
#include "prince.h"
#include "movetemp.h"
#include "sound.h"
#include "tables.h"
#include "altar.h"
#include "top.h"


void reset_actor_alert(void)
{
    Pause = 0;
}

void JumpSeq(int sequence)
{
    if (Char.character_kind == 1)
        CheckShadow();
    if (hGetRsrc(0x53455153UL, sequence)) {
        Char.sequence_id = sequence;
        Char.sequence = sequence;
        Char.sequence_position = 0;
        if ((Char.character_kind == 7 || Char.character_kind == 8) &&
            ((sequence >= 130 && sequence <= 139) || sequence == 166) &&
            Char.direction == 0) {
            Char.direction = -1;
            if (Char.frame == 158 || Char.frame == 150)
                Char.x = AddCharX(14);
            RereadCells();
        }
    }
}


void Falling(void)
{
    sbyte tile;

    if (!Screamed && Char.vertical_step >= 31) {
        if (Char.character_kind == 0) {
            AddKidScream(Char.room);
            Screamed = 1;
        } else if (Char.character_kind == 2) {
            AddSound(25);
            if (Char.row >= 6)
                KillFallingChar();
        }
    }
    if (63 * Char.row + 56 > Char.y) {
        FallOn();
        if ((Char.character_kind == 7 || Char.character_kind == 8) && Char.vertical_step != 0) {
            Char.row = GetRow(Char.y);
            return;
        }
    } else {
        tile = GetUnderFt();
        if (IsWall(tile))
            tile = InsideCell();
        if (!IsSpace(tile)) {
            if (tile != 11 && tile != 15 && tile != 26) {
                HitFloor();
                return;
            }
            if (tile == 11)
                BreakLoose(Char.vertical_step);
            else if (tile == 15)
                BreakTotalLoose();
            else if (tile == 26)
                ShakeTrapDoor();
            if ((Char.vertical_step >= 33 || Char.slot != 10) && Char.character_kind != 1)
                DecLifePts(100);
            Char.vertical_step /= 2;
            Char.y = Row2PixelY[Char.row + 1] - 7;
            if (CueSnd == 1)
                CueSnd = -1;
            ResetSnd(10001);
            Screamed = 0;
            return;
        }
        Char.row++;
    }
}


void HitFloor(void)
{
    sbyte tile;
    int sequence, hazard, fatal, hurt, sound, landed;
    int edge;

    hazard = 0; hurt = 0;
    fatal = 0; landed = 0;
    Screamed = 0;
    sound = -1;

    ResetCharY();
    tile = GetUnderFt();
    if (tile == 23 || tile == 24) {
        landed = 1;
    } else {
        edge = GetDist(); tile = GetCellsInFront(1);
        if (Char.character_kind != 7 && Char.character_kind != 8 &&
            (IsSpace(tile) || IsImpassable(tile)) && edge < 11)
            Char.x = AddCharX(edge - 13);
        if (Char.alive >= 0)
            fatal = 1;
    }

    tile = GetUnderFt();
    if (landed || tile == 23 || tile == 24) {
        DoLavaBoil(); hazard = 1;
    }
    if (!hazard) {
        if (Char.vertical_step >= 22 && Char.character_kind != 1) {
            if (Char.vertical_step < 33) {
                if (Char.slot != 10) {
                    fatal = 1;
                } else {
                    if (DecLifePts(1) ||
                        (Char.room == 11 && Level.environment_kind == 5 && Char.row >= 3)) {
                        hurt = 1;
                        DecLifePts(100);
                    } else {
                        sound = 16;
                        sequence = 20;
                    }
                }
            } else {
                fatal = 1;
            }
        } else {
            if ((Char.character_kind >= 2 || Char.stance == 1) &&
                Char.character_kind != 6 && Char.character_kind != 1) {
                sequence = Char.sequence == 186 ? 187 : 63;
                Char.stance = 1;
            } else {
                sequence = 17;
            }
            sound = Char.character_kind == 4 ? 76 : 17;
        }

        if (fatal) {
            DecLifePts(100); hurt = 1;
        }
        if (hurt) {
            if (Char.character_kind == 4) {
                BonesFall(); sequence = 112;
            } else {
                sound = 0;
                sequence = 22;
            }
            SetKidDeathMethod(3);
            PutDeadBodyOnSolidGround();
        }
        if (Char.character_kind == 7 || Char.character_kind == 8) {
            Char.x = AddCharX(6);
            sequence = 155;
        }
        JumpSeq(sequence);
        AnimChar();
        Char.vertical_step = 0;
        if (sound != -1) {
            AddSound(sound);
            if (Char.character_kind == 0)
                AlertGuard = 1;
        }
    }
}


/* Same-file helper at 25da:03d4. */
void PutDeadBodyOnSolidGround(void)
{
    sbyte tile;
    int edge, correction;

    edge = GetDist();
    if (edge < 20)
        tile = GetCellsInFront(1);
    else if (edge > 20)
        tile = GetBehind();
    else
        tile = 1;

    if (IsSpace(tile))
        correction = 1;
    else
        correction = IsMobFloor(tile) ? 2 : 0;
    if (correction) {
        Char.x = AddCharX(edge - 20);
        if (correction == 2 && Char.row >= 3) {
            if (edge - 20 < 0)
                edge = -1;
            else
                edge = 1;
            Char.x = AddCharX(18 * edge);
            if (tile == 26)
                Char.x = AddCharX(9 * edge);
        }
    }
}


/* 25da:047e */
void GenCtrl(void)
{
    word frame = Char.frame;
    int state;

    if (Char.alive < 0) {
        state = Char.action;
        if (state == 5 || state == 4 || state == 9 ||
            Char.sequence == 27 || Char.sequence == 110) {
            ClrAll();
            if (Level.level_number == 12 && RoomOpp[0].character_kind == 12 &&
                RoomOpp[0].direction != 86 && ColEdgeX[11] > RoomOpp[0].x)
                JumpSeq(212);
        } else if (Char.stance == 1) {
            FightCtrl();
        } else if (Char.character_kind >= 2) {
            GuardCtrl();
        } else if (frame == 15 || (frame >= 50 && frame <= 52)) {
            Standing();
        } else if (frame >= 45 && frame <= 49) {
            Turning();
        } else if (frame >= 1 && frame < 4) {
            Starting();
        } else if (frame >= 67 && frame < 70) {
            StJumpUp();
        } else if (frame < 15) {
            Running();
        } else if (frame >= 87 && frame <= 99) {
            if (Level.environment_kind != 1)
                Hanging();
        } else if (frame == 81 && Char.sequence == 68) {
            Drop();
        } else if (frame == 109) {
            Crouching();
        } else if ((frame >= 246 && frame <= 261) ||
                   (frame >= 256 && frame <= 263)) {
            Crawling();
        } else if (frame >= 217 && frame <= 226) {
            StairClimbing();
        } else if (IsDeadPos(frame) || (frame >= 179 && frame <= 183)) {
            Dead();
        } else if (Char.frame == 295) {
            Sitting();
        } else if (Char.frame >= 272 && Char.frame <= 281) {
            ThrowingFireball();
        } else if (Char.frame == 44 || Char.frame == 26) {
            ClrAll();
        }
    } else {
        if (Char.character_kind != 0 && Char.slot == Kid.opponent_index)
            Kid.opponent_index = GetOppIdxClosestToKid(1);
        if (Char.character_kind == 1) {
            if (Char.alive > 6)
                CutToKidRoom();
        } else if (Char.room == 3 && Level.environment_kind == 6 && Char.character_kind == 2) {
            FinlCheckDeadOpp();
        }
    }
}


void Crawling(void)
{
    sbyte tile;
    int gap;

    if (Char.frame >= 246 && Char.frame <= 261) {
        if (Level.environment_kind == 4)
            RuinCheckCrevices();
        tile = ReadCell(Char.room, Char.column, Char.row);
        if (tile != 7 && tile != 12 && tile != 13 &&
            GetCellsInFront(1) != 7 && GetCellsInFront(1) != 12 && GetCellsInFront(1) != 13 &&
            GetBehind() != 7 && GetBehind() != 12 && GetBehind() != 13 &&
            Char.sequence != 124) {
            JumpSeq(124);
            return;
        }
        tile = ReadCell(Char.room, Char.column, Char.row);
        if (Char.frame == 251) {
            if (Level.environment_kind == 4)
                RuinCheckCrevices();
            if (Dir.up && tile != 7) {
                tile = GetBehind();
                if (tile == 7) {
                    if (Char.direction == 0)
                        gap = Char.x - ColEdgeX[Char.column + 5] - 38;
                    else
                        gap = ColEdgeX[Char.column + 6] - Char.x - 10;
                } else if (tile == 12 || tile == 13) {
                    gap = ScytheBarrToStandUp() ? -32 : 32;
                } else {
                    gap = 32;
                }
                if (gap >= 0) {
                    JumpSeq(124);
                    Dir.up = ClrAll();
                    if (gap < 6) {
                        Char.x = AddCharX(-(gap -= 6));
                    }
                }
            } else {
                if (Dir.forward < 0 && Char.repeat) {
                    ClrAll();
                    JumpSeq(122);
                } else if (Dir.back < 0) {
                    ClrAll();
                    JumpSeq(123);
                }
            }
            if (Dir.forward == 0 && Char.repeat == 0) {
                Char.repeat = 1;
                return;
            }
        } else {
            ClrAll();
        }
    }
}

/* 25da:0802 */
void Crouching(void)
{
    sbyte tile, column;
    int gap;
    int sequence;

    if (Level.environment_kind == 4)
        RuinCheckCrevices();
    if (Level.level_number == 5 && Char.room == 3 &&
        Char.sequence != 17 && Char.sequence != 20 &&
        (GetUnderFt() == 18 || ReadCell(Char.room, Char.column - 1, Char.row) == 18)) {
        TriggerCharOnCarpet();
        return;
    }
    if (Char.sequence == 111 || TryPickUp()) {
        Dir.action = ClrAll();
        return;
    }

    sequence = -1;
    if (Dir.down == 0) {
        if (!GateBarrToStandUp() && (!Level.background_overlays[3] || !ScytheBarrToStandUp()))
            sequence = 49;
        else if (Dir.forward < 0)
            sequence = 79;
    } else if (Dir.forward < 0) {
        Dir.forward = ClrAll();
        tile = GetCellsInFront(1);
        if ((tile == 7 || tile == 12 || tile == 13) &&
            (tile != 7 || (Cell.modifier & 3) != 3)) {
            column = GetCellCol();
            if (Char.direction == 0 && Char.x - ColEdgeX[column + 5] > 6) {
                gap = ColEdgeX[column + 5] - Char.x + 6;
            } else if (Char.direction == -1 && Char.x - ColEdgeX[column + 6] < 22) {
                gap = Char.x - ColEdgeX[column + 6] - 22;
            } else {
                gap = 0;
            }
            if (gap)
                Char.x = AddCharX(gap);
            sequence = 125;
        } else {
            sequence = 79;
        }
    }
    if (Dir.down != 0)
        Dir.down = ClrAll();
    if (sequence != -1) {
        JumpSeq(sequence);
        return;
    }
    return;
}


/* 25da:0974 */
void Dead(void)
{
    sbyte slot;

    if (IsDeadPos(Char.frame)) {
        if (Level.environment_kind == 1 && Char.frame == 185 && Char.life_points) {
            DecLifePts(Char.life_points);
        } else if (Char.character_kind == 1) {
            if (!Char.repeat && (GetShadowToKidDist() < 0 ? -GetShadowToKidDist() : GetShadowToKidDist()) <= 1) {
                KidRiseFromShadowman();
                return;
            }
        }
    } else if (Char.character_kind == 0 && Char.slot != 10 && Char.frame != 179 && Char.frame != 183) {
        slot = Char.slot;
        SaveShad();
        LoadKid();
        if (!DecLifePts(1))
            Char.life_initial--;
        Kid = Char;
        LoadShad(slot);
        DecLifePts(1);
    }
}


/* 25da:0a68 */
void Standing(void)
{
    int act_on_input;
    int gap;

    if (Level.environment_kind == 2 || Level.environment_kind == 6)
        CheckTurnSeqs();
    act_on_input = 1;
    if (Dir.action == -1 && TryPickUp()) {
        act_on_input = 0;
    } else if (Char.slot == 10) {
        if ((Char.stance != -1 || (Level.environment_kind == 6 && Char.character_kind == 1)) &&
            Dir.action == -2 && !Dir.forward && !Dir.back &&
            !Dir.up && !Dir.down) {
            Dir.action = 2;
            ClrAll();
            DoEngarde();
            Char.opponent_index = GetOppIdxClosestToKid(Char.direction);
            if (Char.opponent_index == -1)
                Char.opponent_index = 0;
            act_on_input = 0;
        } else if (Opp.alert_delay >= 2 && Char.character_kind != 1 &&
                   Char.sequence_group != 13 && Opp.character_kind != 6) {
            gap = GetOppDist();
            if (gap >= -10 && gap < 208 && gap < 0 && gap >= -14) {
                DoTurn();
                act_on_input = 0;
            }
        }
    } else if (Dir.down < 0 && Dir.forward < 0) {
        DoEngarde();
        act_on_input = 0;
    }

    if (act_on_input) {
        if (Dir.action == -1 || Dir.action == 1) {
            act_on_input = 0;
            if (Dir.back < 0)
                DoTurn();
            else if (Dir.up < 0)
                Up();
            else if (Dir.down < 0)
                Down();
            else if (Dir.forward < 0 && !Dir.up && !Dir.down)
                DoStepFwd(0);
            else if (!Dir.forward)
                act_on_input = 1;
        }
        if (act_on_input) {
            if (Dir.forward < 0) {
                if (Dir.up < 0)
                    DoStandJump();
                else
                    DoStartRun();
            } else if (Dir.back < 0) {
                DoTurn();
            } else if (Dir.up < 0) {
                if (Dir.forward < 0)
                    DoStandJump();
                else
                    Up();
            } else if (Dir.down < 0) {
                Down();
            } else if (Dir.forward) {
                DoStartRun();
            }
        }
    }

    if (Char.sequence != 5 && Char.character_kind == 0 && Char.image_bank)
        Char.image_bank = 0;
}

/* 25da:0c1e */
void Up(void)
{
    if (Room.number != Level.entry_room && Char.character_kind == 0 &&
        (GetUnderFt() == 17 || GetBehind() == 17 || GetCellsInFront(1) == 17) &&
        IsExitOpen()) {
        Stairs();
        return;
    }
    if (Dir.forward) {
        DoStandJump();
        return;
    }
    DoJumpUp();
}


/* 25da:0c6c */
void Down(void)
{
    sbyte behind, under;
    int crouch, edge;
    word modifier;

    crouch = 1;
    Dir.down = 1;
    if (IsSpace(GetCellsInFront(1)) && GetDist() < 3) {
        Char.x = AddCharX(10);
        RereadCells();
        crouch = 0;
    } else if (IsSpace(GetBehind()) && GetDist() >= 8) {
        behind = GetBehind();
        modifier = Cell.modifier;
        under = GetUnderFt();
        if (IsLedge(behind, under, modifier, Cell.modifier) &&
            (Char.direction == 0 || GetUnderFt() != 4 || (Cell.modifier & 0xfffc) >= 24)) {
            edge = GetDist();
            edge -= 19;
            if (Char.direction == 0)
                edge -= 4;
            Char.x = AddCharX(edge);
            JumpSeq(68);
            crouch = 0;
        }
    }
    if (crouch)
        DoCrouch();
}

/* 25da:0d42 */
void Stairs(void)
{
    Char.x = ColEdgeX[Cell.column + 4] + 30;
    Char.direction = -1;
    JumpSeq(70);
}


/* 25da:0d62 */
void StairClimbing(void)
{
    sbyte cell = IndexCell(Char.column, Char.row);

    Trob.room = Char.room;
    Trob.cell = Level.cell_kinds[Trob.room - 1][cell] == 17 ? cell : cell + 1;
    if (Char.frame == 224)
        AddLeaveLevelSong();
    RedrawExit();
}

/* 25da:0dba */
void Turning(void)
{
    if (Char.slot == 10 && Char.sequence == 5)
        CheckTurnSeqs();
    if (Char.frame == 48 && Dir.action >= 0 &&
        Dir.forward && !Dir.up && !Dir.down) {
        JumpSeq(43);
        if (Char.character_kind == 0 && Char.image_bank)
            Char.image_bank = 0;
    }
}


/* 25da:0e0e */
void DoCrouch(void)
{
    int sequence = Char.character_kind == 1 ? CheckShadowmanBackIntoKid() : -1;
    if (sequence == -1)
        sequence = 50;
    JumpSeq(sequence);
    Dir.down = ClrAll();
}


/* 25da:0e3a */
void DoTurn(void)
{
    int edge, distance;
    Dir.back = ClrAll();
    Char.repeat = 1;
    edge = GetDist();
    if (GetCellsInFront(1) == 4 && GateBarr()) {
        if (Level.environment_kind == 3)
            distance = 7;
        else if (Room.number == 9 && Level.level_number == 8)
            distance = -100;
        else
            distance = 4;
        if (distance > edge) {
            edge -= distance;
            Char.x = AddCharX(edge);
        }
    }
    if (Level.environment_kind == 2 ||
        (Level.environment_kind == 6 &&
         !((Char.room == 1 || Char.room == 2) && Level.level_number == 14)))
        CheckShadowmanCreate();
    if (Char.sequence != 71)
        JumpSeq(5);
}


/* 25da:0eba */
void DoStartRun(void)
{
    int distance = GetFwdDist();
    if (StepTo == 1 && distance < 18) {
        if (Dir.forward < 0)
            DoStepFwd(0);
    } else {
        JumpSeq(1);
    }
    Dir.forward = ClrAll();
}


/* 25da:0ef0 */
void Running(void)
{
    if (!Dir.forward && !Dir.back && (Char.frame == 7 || Char.frame == 11)) {
        Dir.forward = ClrAll();
        JumpSeq(13);
    } else if (Dir.back) {
        Dir.back = ClrAll();
        JumpSeq(6);
    } else if (Dir.up) {
        if (Dir.up < 0)
            DoRunJump(4);
    } else if (Dir.down < 0) {
        Dir.down = ClrAll();
        JumpSeq(26);
    }
    if (Dir.forward < 0)
        Dir.forward = 1;
}


/* 25da:0f66 */
void DoStepFwd(int distance)
{
    int sequence, remainder;
    sbyte edge_marker;

    InDoStepFwd = 1;
    edge_marker = 1; Dir.forward = edge_marker; Dir.action = edge_marker;
    if (!distance)
        distance = GetFwdDist();
    if (StepTo == 1)
        distance--;
    if (distance > 0) {
        Char.repeat = edge_marker;
        if (distance < 28) {
            if (distance == 1)
                sequence = -1;
            else
                sequence = distance / 2 + 28;
            if ((remainder = distance % 2) == 0)
                distance = sequence;
            else {
                Char.x = AddCharX(1);
                distance = sequence;
            }
        } else {
            distance = 42;
        }
    } else if (Char.repeat &&
               (StepTo != 1 || Cell.kind == 12 || Cell.kind == 13)) {
        Char.repeat = 0; distance = 44;
    } else {
        distance = 42;
    }
    if (distance != -1)
        JumpSeq(distance); InDoStepFwd = 0;
}


int TryPickUp(void)
{
    CELLREC saved_tile;
    int here = 0;
    sbyte tile = GetUnderFt();

    if (tile == 10 || tile == 22) {
        saved_tile = Cell;
        here = 1;
        Cell = saved_tile;
    }
    if (here || GetCellsInFront(1) == 10 || GetCellsInFront(1) == 22)
        here = PickItUp();
    return here;
}

int PickItUp(void)
{
    sbyte item;
    int picked;
    sbyte foot;
    int edge;
    byte type;
    CELLREC saved_tile;

    item = Cell.kind;
    saved_tile = Cell;
    type = Cell.modifier & 3;
    picked = 1;
    edge = GetDist();
    foot = GetUnderFt();

    if (Char.frame != 109) {
        Char.x = AddCharX(foot == item
            ? (Char.direction == 0 ? edge - 25 : edge - 17)
            : (Char.direction == 0 ? edge + 1 : edge + 15)); DoCrouch();
    } else if (Char.sequence == 50 ||
               (Char.sequence != 20 && Dir.action == -1)) {
        if (Char.direction == 0) {
            if (edge < 24 && foot != item)
                Char.x = AddCharX(edge - 4);
        } else {
            Char.x = AddCharX(edge - 22);
        }
        Cell = saved_tile;
        if (item == 22) {
            if (Char.room == 9 && Level.level_number == 8 && !GameState.throne_nis_played) {
                Char.x = AddCharX(-14);
                if (Char.direction == -1)
                    Char.x = AddCharX(2);
                JumpSeq(237);
                AddSong(254);
            } else {
                JumpSeq(91);
                RemoveObj(-1);
                LoadSwordSet(type);
                KidInfo.sword_kind = type;
                if (Char.room != 9 || Level.level_number != 8)
                    AddSong(151);
            }
        } else if (item == 10) {
            RemoveObj((Cell.modifier & 0xe0) >> 5);
            JumpSeq(78);
        }
    } else {
        picked = 0;
    }
    return picked;
}

/* 25da:11dc */
void Starting(void)
{
    if (Dir.up < 0 && Dir.forward < 0) {
        byte zero = ClrAll();
        Dir.forward = zero;
        Dir.up = zero;
        DoStandJump();
    }
}

/* 25da:11fa */
void StJumpUp(void)
{
    if (Dir.forward)
        DoStandJump();
}

/* 25da:1206 */
void DoStandJump(void)
{
    sbyte held = 1;
    Dir.forward = held;
    Dir.up = held;
    JumpSeq(3);
}


/* 25da:1218 */
void DoJumpUp(void)
{
    sbyte above, front;
    word modifier;
    int anchor;

    Dir.up = ClrAll();
    above = GetAbove();
    modifier = Cell.modifier;
    front = GetAboveInFront();
    if (front == 4)
        Cell.modifier = 200;
    anchor = GetSlideRockX(Char.room, Char.row);
    if (anchor && abs(anchor - Char.x) > 32)
        anchor = 0;
    if (!anchor && IsLedge(above, front, modifier, Cell.modifier)) {
        DoJumpHang(0);
        return;
    }

    above = GetAboveBehind();
    modifier = Cell.modifier;
    front = GetAbove();
    if (front == 4)
        Cell.modifier = 200;
    if (!anchor && IsLedge(above, front, modifier, Cell.modifier)) {
        DoJumpBack();
        return;
    }
    DoJumpHigh();
}

/* 25da:12dc */
void DoJumpBack(void)
{
    int edge = GetDist();

    GetSlideRockX(Char.room, Char.row);
    if (edge < 16) {
        DoJumpHigh();
        return;
    }
    if (IsSpace(GetBehind())) {
        DoJumpEdge();
        return;
    }
    Char.x = AddCharX(edge - 32);
    RereadCells();
    DoJumpHang(1);
}

/* 25da:132e */
void DoJumpEdge(void)
{
    GetAbove();
    Char.x = AddCharX(GetDist() - 24);
    JumpSeq(16);
}

/* 25da:134e */
void DoJumpHigh(void)
{
    sbyte column;
    int edge;

    Dir.up = ClrAll();
    edge = GetFwdDist();
    if (edge < 4 && StepTo == 1)
        Char.x = AddCharX(edge - 6);
    column = GetCol(FaceDx(0) + GetBaseX() - 6);
    if (IsSpace(ReadCell(Char.room, column, Char.row - 1)) ||
        (Level.environment_kind == 5 && Char.room == 15) ||
        (Char.room == 7 && Level.environment_kind == 6 &&
         (Char.row != 2 || (Char.column != 2 && Char.column != 6))) ||
        (Char.room == 8 && Level.environment_kind == 6 &&
         (Char.row != 2 || Char.column == 3 || Char.column == 4)))
        JumpSeq(28);
    else
        JumpSeq(14);
}


/* 25da:141e */
void Hanging(void)
{
    sbyte tile;
    if (Char.alive < 0) {
        if (!Stunned && Dir.up) {
            ClimbUp();
        } else if (Dir.action && Char.sequence_group != 11) {
            tile = GetUnderFt();
            if ((Char.action != 6 && IsWall(tile)) ||
                (Char.direction == -1 && tile == 4) ||
                (Level.environment_kind == 5 && Char.room == 15))
                JumpSeq(25);
            else if (IsSpace(GetAbove()) && (Level.environment_kind != 5 || !RoofHangingFromShip()))
                Drop();
        } else {
            Char.sequence_group = 0;
            Drop();
        }
    } else
        Drop();
}

/* 25da:14c4 */
void ClimbUp(void)
{
    int sequence;
    byte zero;
    sbyte above;

    if (Level.level_number == 5 && Char.room == 10 && Char.stance != -1) {
        sequence = 236;
        Char.stance = -1;
        AddSound(19);
    } else {
        sequence = 10;
    }
    zero = ClrAll();
    Dir.action = zero;
    Dir.up = zero;
    above = GetAbove();
    if ((Char.character_kind != 1 && above == 4 && Char.direction == -1 &&
         (Cell.modifier & 0xfffc) < 24) ||
        (Level.environment_kind == 2 && SlideRockInCell(0))) {
        sequence = 73;
    } else if (Level.environment_kind == 5 && RoofHangingFromShip()) {
        sequence = 59;
    }
    JumpSeq(sequence);
    if (Char.direction == 0 || Level.environment_kind != 3)
        Char.x = AddCharX(-2);
}

/* 25da:156a */
void Drop(void)
{
    sbyte under, behind;
    int adjustment, back_drop, front_drop, sequence;

    Dir.down = ClrAll();
    adjustment = 0;
    behind = GetBehind();
    under = GetUnderFt();
    if (IsSpace(behind) && IsSpace(under)) {
        sequence = IsCharSwingingFwd() ? 99 : 23;
    } else {
        front_drop = IsLandOnable(under);
        back_drop = IsLandOnable(behind);
        if (!front_drop && !back_drop) {
            if (IsFloor(behind) && IsWall(under))
                back_drop = 1;
            else if (IsFloor(under) && IsWall(behind))
                front_drop = 1;
        }
        if (IsWall(under)) {
            adjustment = -14;
            sequence = back_drop ? 11 : 23;
        } else if (!front_drop && back_drop) {
            adjustment = -8;
            sequence = 11;
        } else if (front_drop && !back_drop) {
            adjustment = 6;
            sequence = 11;
        } else {
            sequence = front_drop && back_drop ? 11 : 23;
        }
        if (adjustment)
            Char.x = AddCharX(adjustment);
    }
    JumpSeq(sequence);
}


int IsCharSwingingFwd(void)
{
    int old_repeat = Char.sequence;
    int old_position = Char.sequence_position;
    int old_sequence = Char.sequence_id;
    word old_frame = Char.frame;

    AnimChar();
    Char.sequence_id = old_sequence;
    Char.sequence_position = old_position;
    Char.sequence = old_repeat;
    if (Char.frame <= old_frame)
        return 1;
    return 0;
}

/* 25da:16d2 */
void DoJumpHang(int back)
{
    int edge = GetDist();

    if (back && edge == 32)
        edge = 0;
    if (edge < 4 && GetFwdDist() < 4 && StepTo != 1) {
        Char.x = AddCharX(edge);
        JumpSeq(8);
    } else {
        if (Char.direction == -1)
            edge -= 7;
        else
            edge -= 9;
        Char.x = AddCharX(edge);
        JumpSeq(24);
    }
}


/* 25da:1738 */
#define RUNNING_JUMP_REACH 9
#define OPPONENT_JUMP_REACH 9

int DoRunJump(int sequence)
{
    sbyte tile, column;
    int reach, x, distance, gaps, scanning, found, result;

    if ((sequence == 4 && (Char.frame == 7 || Char.frame == 11)) ||
        (sequence == 100 && (Char.frame == 192 || Char.frame == 196))) {
        reach = sequence == 4 ? RUNNING_JUMP_REACH : OPPONENT_JUMP_REACH;
        x = AddCharX(reach);
        column = GetCharCol(x);
        gaps = found = 0;
        scanning = 1;
        while (scanning) {
            tile = ReadCell(Char.room, column += Plus1[Char.direction + 1], Char.row);
            if (IsAdvancableId(tile, Cell.modifier) || tile == 11) {
                ++gaps;
                if (gaps == 2)
                    found = 1;
            } else {
                scanning = 0;
            }
            if (found)
                break;
        }
        if (!scanning) {
            distance = GetDist1(x) + (gaps << 5);
            distance -= sequence == 4 ? 31 : 35;
            if (distance < -20 || distance >= 10) {
                if (distance < 0 && sequence == 4)
                    return 0;
                distance = -reach;
            }
            Char.x = AddCharX(distance + reach);
        }
        JumpSeq(sequence);
        Dir.up = ClrAll();
        result = 1;
    } else {
        result = 0;
    }
    return result;
}

/* 25da:1870 */
void DoRetreat(void)
{
    int frame = Char.frame;
    if (Char.character_kind != 7 && Char.character_kind != 8 && Char.character_kind != 11 &&
        (frame == 158 || frame == 170 || frame == 171)) {
        int sequence;
        if (Char.character_kind == 10)
            sequence = 104;
        else
            sequence = 57;
        JumpSeq(sequence);
        Dir.back = ClrAll();
    }
}

/* 25da:18ba */
void DoAdvance(void)
{
    int frame = Char.frame;
    if (Char.character_kind != 7 && Char.character_kind != 8 && Char.character_kind != 11 &&
        (frame == 158 || frame == 170 || frame == 171)) {
        int sequence;
        if (Char.character_kind == 0 || Char.character_kind == 1)
            sequence = 56;
        else if (Char.character_kind == 10)
            sequence = 108;
        else
            sequence = 86;
        JumpSeq(sequence);
        Dir.forward = ClrAll();
    }
}


/* 25da:1918 */
void DoEngarde(void)
{
    int sequence;

    Dir.forward = ClrAll();
    Dir.action = 2;
    Char.stance = 1;
    if (Char.character_kind == 0 || Char.character_kind == 1)
        sequence = DoKidEngarde();
    else if (Char.character_kind == 7 || Char.character_kind == 8)
        sequence = -1;
    else if (Char.character_kind != 1)
        sequence = 90;
    if (sequence != -1)
        JumpSeq(sequence);
}

/* 25da:1978 */
int DoKidEngarde(void)
{
    sbyte tile, slot;
    int sequence, anchor, adjustment, distance;

    if ((Char.frame >= 246 && Char.frame <= 261) ||
        ((Char.room == 1 || Char.room == 2) && Level.level_number == 14)) {
        sequence = -1;
    } else if (Char.character_kind == 1 && (Char.room == 7 || Char.room == 8) && Level.environment_kind == 6) {
        sequence = DoThrowFireball();
        slot = 0;
        while (RoomOpp[slot].character_kind != 0 && slot < 5)
            ++slot;
        Char.stance = RoomOpp[slot].stance;
    } else {
        sequence = 55;
        adjustment = -1;
        anchor = GetSlideRockX(Char.room, Char.row);
        if (anchor) {
            tile = GetCharCol(anchor);
            if (abs(tile - Char.column) > 1)
                anchor = 0;
        }
        tile = Char.direction == 0 ? GetUnderFt() : 0;
        if (tile == 4 && GateBarr()) {
            adjustment = -32;
        } else if (anchor && ((Char.direction == -1 && Char.x >= anchor) ||
                             (Char.direction == 0 && Char.x <= anchor))) {
            distance = GetDist();
            adjustment = abs(anchor - Char.x) - distance;
            if (adjustment == -1) adjustment = -2;
            adjustment -= 4;
        } else {
            tile = GetCellsInFront(1);
            if (!IsFloor(tile) || tile == 11 || (tile == 4 && GateBarr()))
                adjustment = 0;
        }
        if (adjustment != -1) {
            tile = GetBehind();
            if (IsFloor(tile) && tile != 11 && (tile != 4 || !GateBarr()) &&
                (!anchor || (Char.direction == -1 && Char.x >= anchor) ||
                 (Char.direction == 0 && anchor >= Char.x)) && Char.sequence_group != 4) {
                distance = GetDist() + adjustment;
                if (Char.direction == 0 && distance < 21)
                    Char.x = AddCharX(distance - 21);
                else if (Char.direction == -1 && distance < 15)
                    Char.x = AddCharX(distance - 15);
            } else {
                Char.stance = 0;
                sequence = -1;
            }
        }
    }
    if (sequence != -1 && sequence != 242)
        AddSound(19);
    else if (Char.stance != -1)
        Char.stance = 0;
    return sequence;
}


/* 25da:1b92 */
void FightCtrl(void)
{
    sbyte target;
    int gap, react;

    if (Char.slot == 10) {
        Kid = Char;
        target = -1;
        if (Char.opponent_index != -1 && Char.opponent_index < Level.room_objects[Char.room - 1].opponent_count) {
            LoadKidwOpp(Char.opponent_index);
            if (Char.life_points > 0)
                target = Char.opponent_index;
        }
        if (target == -1) {
            target = GetOppIdxClosestToKid(1);
            if (target == -1)
                target = 0;
            else
                Char.opponent_index = target;
        }
        LoadKidwOpp(target);
    }
    if (Char.action < 2 && abs(Char.life_delta) < Char.life_points) {
        sbyte tile = GetUnderFt();
        react = 1;
        if (tile == 11 || tile == 15 || Opp.alert_delay >= 2) {
            gap = GetOppDist();
            if (gap >= -10 && gap < 208) {
                OnAlert();
                react = 0;
            } else if (gap < 0) {
                if (gap >= -4 ||
                    (gap >= -30 && (Opp.character_kind == 7 || Opp.character_kind == 8))) {
                    OnAlert();
                } else if (Char.character_kind != 11) {
                    if ((Char.slot == 10 && !Player) || Opp.character_kind == 6 ||
                        (Char.row != 0 && Level.level_number == 5 &&
                         (Char.room == 10 || Char.room == 7 || Char.room == 12) &&
                         BrgSkelMustAdvance()))
                        OnAlert();
                    else
                        DoTurnEngarde();
                }
                react = 0;
            }
        }
        if (Char.slot == 10 && Dir.back < 0 &&
            (Dir.action == -1 || Dir.action == 1) &&
            (Level.level_number != 5 || Char.room != 10 || Bridge.broken <= 0)) {
            DoTurnEngarde();
            Dir.action = 2;
            Dir.back = ClrAll();
            Player = !Player;
            Char.opponent_index = GetOppIdxClosestToKid(~Char.direction);
            return;
        }
        if (react) {
            if (Char.slot == 10 && Dir.down < 0) {
                DoSheathe();
                return;
            }
            OnAlert();
        }
    }
}
#pragma pack(1)


int OppToFight(void)
{
    CHARREC saved_actor;
    sbyte index;
    int ready;
    OPPONENT *opponent;

    ready = 0;
    saved_actor = Char;
    LoadKid();
    if (Char.sequence_group != 13 && Char.character_kind != 1 &&
        Char.action != 3 && Char.action != 4 && !IsDeadPos(Char.frame)) {
        index = 0;
        do {
            if (index >= Level.room_objects[Kid.room - 1].opponent_count)
                break;
            LoadShad(index);
            if (Char.alive < 0 && Char.alert_delay > 0 && Char.character_kind != 11) {
                if (Char.character_kind == 7 || Char.character_kind == 8) {
                    opponent = GetOppInfoIdx(Char.room, Char.slot);
                    if (opponent->u.head.mode != 1 && opponent->u.head.mode != 3)
                        ready = 0;
                    else
                        ready = 1;
                } else
                    ready = 1;
            }
            index++;
        } while (!ready);
    }
    Char = saved_actor;
    return ready;
}


/* 25da:1e08 */
void OnAlert(void)
{
    int acted, frame, kind;

    if (Char.character_kind != 7 && Char.character_kind != 8) {
        acted = 0;
        frame = Char.frame;
        kind = Char.character_kind;
        if (frame == 161 && Dir.action >= 0) {
            JumpSeq(57);
            acted = 1;
        } else if (Dir.action == -2) {
            if (kind == 0 || kind == 1)
                GuardTimer = 15;
            DoStrike();
            acted = 1;
        }
        if (!acted) {
            if (Dir.down < 0) {
                if (frame == 158 || frame == 170 || frame == 171) {
                    DoSheathe();
                    return;
                }
            } else if (Dir.up < 0) {
                DoBlock();
                return;
            } else if (Dir.forward < 0) {
                DoAdvance();
                return;
            } else if (Dir.back < 0 && !Dir.action) {
                DoRetreat();
                return;
            }
        }
    } else {
        HeadOnAlert();
    }
}

/* 25da:1ebc */
void DoSheathe(void)
{
    Char.stance = 0;
    Dir.down = ClrAll();
    if (Char.character_kind == 0 || Char.character_kind == 1) {
        Refract = 9;
        if (OppToFight())
            JumpSeq(93);
        else
            JumpSeq(92);
    } else if (Char.character_kind == 10) {
        JumpSeq(90);
    } else {
        JumpSeq(77);
    }
}

/* 25da:1f08 */
void DoTurnEngarde(void)
{
    if (Char.character_kind == 7 || Char.character_kind == 8) {
        if (Char.sequence != 156 && Char.sequence != 166 &&
            !IsHeadLockedOn(Char.room, Char.slot))
            JumpSeq(156);
    } else if (Char.character_kind == 0 || Char.character_kind == 1) {
        DoKidTurnEngarde();
        return;
    } else if (Char.character_kind != 10 || Char.frame < 213 || Char.frame > 217) {
        JumpSeq(60);
    }
}

/* 25da:1f72 */
void DoKidTurnEngarde(void)
{
    sbyte tile;
    int offset, edge;

    offset = -1;
    tile = GetCellsInFront(1);
    if (IsFloor(tile) && (tile != 4 || !GateBarr())) {
        tile = GetCellsInFront(2);
        if (!IsFloor(tile) || (tile == 4 && GateBarr()))
            offset = 32;
    } else {
        offset = 0;
    }
    if (Char.sequence == 94)
        offset += 18;
    if (offset != -1) {
        edge = GetDist() + offset;
        if (edge < 56)
            Char.x = AddCharX(edge - 56);
    }
    JumpSeq(127);
}

/* 25da:2004 */
void DoStrike(void)
{
    int frame, sequence;
    if (Char.character_kind != 7 && Char.character_kind != 8) {
        frame = Char.frame;
        if (frame == 157 || frame == 158 || frame == 170 || frame == 171 || frame == 165) {
            switch (Char.character_kind) {
            case 1:
            case 0:
                sequence = Opp.character_kind == 11 ? 96 : 75;
                break;
            case 10:
                sequence = 109;
                break;
            default:
                sequence = 58;
                break;
            }
        } else if (frame == 150 || frame == 161) {
            sequence = 66;
        } else {
            sequence = -1;
        }
        if (sequence != -1) {
            JumpSeq(sequence);
            Dir.action = 2;
            ClrAll();
        }
    }
}

/* 25da:2090 */
void DoBlock(void)
{
    int threat_frame, kind, sequence, advance, frame;

    if (Char.character_kind != 7 && Char.character_kind != 8) {
        threat_frame = Opp.frame;
        kind = Char.character_kind;
        sequence = 62;
        advance = 0;
        frame = Char.frame;
        if (Char.frame == 158 || Char.frame == 170 || Char.frame == 171 || Char.frame == 168 || Char.frame == 165) {
            if (kind != 0 && kind != 1) {
                if (GetOppDist() >= 51) {
                    DoRetreat();
                    sequence = -1;
                } else if (threat_frame != 152)
                    sequence = -1;
            } else {
                if (threat_frame == 168)
                    sequence = -1;
                else if (threat_frame != 151 && threat_frame != 152 &&
                         threat_frame != 162 && threat_frame == 153)
                    advance = 1;
            }
        } else if (frame == 167) {
            sequence = 61;
        } else {
            sequence = -1;
        }
        if (sequence != -1) {
            Dir.up = 1;
            JumpSeq(sequence);
            if (advance)
                AnimChar();
        }
    }
}

/* 25da:2152 */
void CrushChar(void)
{
    sbyte state = Char.action;
    int frame = Char.frame;

    if ((frame < 5 || frame >= 15) && frame != 109 &&
        (state < 2 || state == 7) && Char.character_kind != 1) {
        ResetCharY();
        if (DecLifePts(Char.character_kind == 0 ? 1 : 100)) {
            if (Char.character_kind == 11)
                JumpSeq(169);
            else if (Char.character_kind == 7 || Char.character_kind == 8)
                JumpSeq(146);
            else if (Char.character_kind == 4) {
                BonesFall();
                return;
            } else {
                JumpSeq(22);
            }
        } else {
            if (frame == 109)
                return;
            if (IsSpace(GetBehind()))
                Char.x = AddCharX(-4);
            Char.sequence_group = 8;
            AddSound(10);
            JumpSeq(52);
        }
    }
}

#pragma pack(1)


/* 25da:2220 */
void CrushOppInfo(OPPONENT *opponent, byte room)
{
    if (Level.preferred_opponent != 2)
        opponent->life = 0;
    if (Level.preferred_opponent == 5) {
        if (opponent->kind == 8) {
            opponent->action = 169;
            opponent->saved_sequence = 0;
        } else {
            RemoveOppFromRoom(room, opponent->index);
        }
    } else if (Level.preferred_opponent == 2) {
        if (opponent->u.skeleton.shake_delay == 1)
            opponent->u.skeleton.shake_delay = 0;
    } else {
        opponent->action = 22;
        opponent->saved_sequence = 0;
    }
}

/* 25da:227c */
void SquishChar(void)
{
    JumpSeq(118);
    DecLifePts(100);
}
