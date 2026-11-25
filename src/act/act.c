#include "types.h"
#include "level.h"
#include "char.h"
#include "game.h"
extern void far pascal DoBlock(void);
extern void far pascal RemoveOppFromRoom(unsigned char room, int slot);
extern int Pause;
extern CHARREC Kid, Char;

extern int far pascal HGETRSRC(unsigned long type, int id);
extern int far pascal AddCharX(int column);
extern void far pascal RereadCells(void);
extern void far pascal CheckShadow(void);

void far pascal reset_actor_alert(void)
{
    Pause = 0;
}

void far pascal JumpSeq(int sequence)
{
    if (Char.character_kind == 1)
        CheckShadow();
    if (HGETRSRC(0x53455153UL, sequence)) {
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

extern int Screamed;

extern int Row2PixelY[5];
extern int CueSnd;

extern void far _fastcall AddKidScream(unsigned char room);
extern void far pascal AddSound(int effect);
extern void far pascal FallOn(void);
extern void far pascal KillFallingChar(void);
extern signed char far pascal GetRow(int y);
extern signed char far pascal GetUnderFt(void);
extern int far pascal IsWall(signed char kind);
extern signed char far pascal InsideCell(void);
extern int far pascal IsSpace(signed char kind);
extern void far pascal BreakLoose(signed char speed);
extern void far pascal BreakTotalLoose(void);
extern void far pascal ShakeTrapDoor(void);
extern int far pascal DecLifePts(signed char amount);
extern void far pascal ResetSnd(int delay);
void far pascal HitFloor(void);

void far pascal Falling(void)
{
    signed char tile;

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
    if ((signed char)63 * Char.row + 56 > Char.y) {
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
            Char.vertical_step /= (signed char)2;
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

extern LEVELREC Level;
extern int AlertGuard;
extern void far pascal ResetCharY(void);
extern int far pascal GetDist(void);
extern signed char far pascal GetCellsInFront(signed char direction);
extern int far pascal IsImpassable(signed char);
extern void far pascal DoLavaBoil(void);
extern void far pascal BonesFall(void);
extern void far pascal SetKidDeathMethod(unsigned char);
void far pascal PutDeadBodyOnSolidGround(void);
extern void far pascal AnimChar(void);

/* The original calls to and from this draft are far returns through the ACT frame. */
void far pascal HitFloor(void)
{
    signed char tile;
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

extern signed char far pascal GetBehind(void);
extern int far pascal IsMobFloor(signed char);

/* Same-file helper at 25da:03d4. */
void far pascal PutDeadBodyOnSolidGround(void)
{
    signed char tile;
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

extern int far pascal IsDeadPos(unsigned int);
extern signed char far pascal GetOppIdxClosestToKid(signed char);
extern void far pascal CutToKidRoom(void);
extern void far pascal FinlCheckDeadOpp(void);
extern CHARREC RoomOpp[5];
extern int ColEdgeX[20];
extern void far pascal GuardCtrl(void);
extern void far pascal Sitting(void);
extern void far pascal ThrowingFireball(void);
extern unsigned char far pascal ClrAll(void);

extern void far pascal FightCtrl(void);
extern void far pascal Turning(void);
extern void far pascal Starting(void);
extern void far pascal StJumpUp(void);
extern void far pascal Running(void);
extern void far pascal Hanging(void);
extern void far pascal Drop(void);
extern void far pascal Crouching(void);
extern void far pascal Crawling(void);
extern void far pascal StairClimbing(void);
extern void far pascal Dead(void);
extern void far pascal Standing(void);

/* 25da:047e */
void far pascal GenCtrl(void)
{
    unsigned int frame = Char.frame;
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

extern CELLREC Cell;
extern unsigned char far pascal ClrAll(void);
extern signed char far pascal ReadCell(signed char, signed char, signed char);
extern signed char far pascal GetCellCol(void);
extern void far pascal RuinCheckCrevices(void);
extern void far pascal TriggerCharOnCarpet(void);
extern int far pascal GateBarrToStandUp(void);
extern int far pascal ScytheBarrToStandUp(void);
extern int far pascal TryPickUp(void);

/* 25da:0650 */

extern Directions Dir;
void far pascal Crawling(void)
{
    signed char tile;
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
void far pascal Crouching(void)
{
    signed char tile, column;
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

extern int far pascal GetShadowToKidDist(void);
extern void far pascal KidRiseFromShadowman(void);
extern void far pascal SaveShad(void);
extern void far pascal LoadKid(void);
extern void far pascal LoadShad(signed char);

/* 25da:0974 */
void far pascal Dead(void)
{
    signed char slot;

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

extern int far pascal GetOppDist(void);
extern signed char ledge_input, actor_health_slot;
extern CHARREC Opp;
extern ROOMREC Room;
extern void far pascal CheckTurnSeqs(void);
extern void far pascal DoEngarde(void);
extern void far pascal DoTurn(void);
extern void far pascal Up(void);
extern void far pascal Down(void);
extern void far pascal DoStartRun(void);
extern void far pascal DoStepFwd(int);
extern void far pascal DoStandJump(void);
extern void far pascal DoJumpUp(void);
extern int far pascal IsExitOpen(void);
extern void far pascal Stairs(void);
extern void far pascal DoCrouch(void);

/* 25da:0a68 */
void far pascal Standing(void)
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
void far pascal Up(void)
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

extern int far pascal IsLedge(signed char behind, signed char under, unsigned int behind_mod, unsigned int under_mod);

/* 25da:0c6c */
void far pascal Down(void)
{
    signed char behind, under;
    int crouch, edge;
    unsigned int modifier;

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
void far pascal Stairs(void)
{
    Char.x = ColEdgeX[Cell.column + 4] + 30;
    Char.direction = -1;
    JumpSeq(70);
}

extern TROBREC Trob;
extern signed char far pascal IndexCell(signed char column, signed char row);
extern void far pascal AddLeaveLevelSong(void);
extern void far pascal RedrawExit(void);

/* 25da:0d62 */
void far pascal StairClimbing(void)
{
    signed char cell = IndexCell(Char.column, Char.row);

    Trob.room = Char.room;
    Trob.cell = Level.cell_kinds[Trob.room - 1][cell] == 17 ? cell : cell + 1;
    if (Char.frame == 224)
        AddLeaveLevelSong();
    RedrawExit();
}

/* 25da:0dba */
void far pascal Turning(void)
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

extern int far pascal CheckShadowmanBackIntoKid(void);

/* 25da:0e0e */
void far pascal DoCrouch(void)
{
    int sequence = Char.character_kind == 1 ? CheckShadowmanBackIntoKid() : -1;
    if (sequence == -1)
        sequence = 50;
    JumpSeq(sequence);
    Dir.down = ClrAll();
}

extern int far pascal GateBarr(void);
extern void far pascal CheckShadowmanCreate(void);

/* 25da:0e3a */
void far pascal DoTurn(void)
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

extern signed char StepTo;
extern int far pascal GetFwdDist(void);

/* 25da:0eba */
void far pascal DoStartRun(void)
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

extern int far pascal DoRunJump(int sequence);

/* 25da:0ef0 */
void far pascal Running(void)
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

extern int InDoStepFwd;

/* 25da:0f66 */
void far pascal DoStepFwd(int distance)
{
    int sequence, remainder;
    signed char edge_marker;

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

extern int far pascal RoofHangingFromShip(void);
extern int far pascal SlideRockInCell(int direction);
extern int far pascal PickItUp(void);
extern void far pascal RemoveObj(int portion);
extern void far pascal AddSong(int song);
extern void far pascal LoadSwordSet(unsigned char type);
extern struct {
    signed char argument, entrance_mode, sword_kind, death_countdown;
    int field_4, shadow_timer;
} KidInfo;
extern GAMESTATE GameState;

int far pascal TryPickUp(void)
{
    CELLREC saved_tile;
    int here = 0;
    int result;
    signed char tile = GetUnderFt();

    if (tile == 10 || tile == 22) {
        saved_tile = Cell;
        here = 1;
        Cell = saved_tile;
    }
    if (here || GetCellsInFront(1) == 10 || GetCellsInFront(1) == 22)
        here = PickItUp();
    return here;
}

int far pascal PickItUp(void)
{
    signed char item;
    int picked;
    signed char foot;
    int edge;
    unsigned char type;
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
void far pascal Starting(void)
{
    if (Dir.up < 0 && Dir.forward < 0) {
        unsigned char zero = ClrAll();
        Dir.forward = zero;
        Dir.up = zero;
        DoStandJump();
    }
}

/* 25da:11fa */
void far pascal StJumpUp(void)
{
    if (Dir.forward)
        DoStandJump();
}

/* 25da:1206 */
void far pascal DoStandJump(void)
{
    signed char held = 1;
    Dir.forward = held;
    Dir.up = held;
    JumpSeq(3);
}

extern int far pascal FaceDx(int x);
extern int far pascal GetBaseX(void);
extern signed char far pascal GetCol(int x);
extern signed char far pascal GetAbove(void);
extern signed char far pascal GetAboveInFront(void);
extern signed char far pascal GetAboveBehind(void);
extern int far _fastcall GetSlideRockX(signed char room, signed char row);
extern int abs(int);
#pragma intrinsic(abs)
extern void far pascal DoJumpHang(int back);
extern void far pascal DoJumpBack(void);
extern void far pascal DoJumpEdge(void);
extern void far pascal DoJumpHigh(void);

/* 25da:1218 */
void far pascal DoJumpUp(void)
{
    signed char above, front;
    unsigned int modifier;
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
void far pascal DoJumpBack(void)
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
void far pascal DoJumpEdge(void)
{
    GetAbove();
    Char.x = AddCharX(GetDist() - 24);
    JumpSeq(16);
}

/* 25da:134e */
void far pascal DoJumpHigh(void)
{
    signed char column;
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

extern int Stunned;
extern signed char far pascal GetAbove(void);
extern void far pascal ClimbUp(void);
extern int far pascal IsCharSwingingFwd(void);
extern int far pascal IsLandOnable(signed char tile);
extern int far pascal IsFloor(signed char tile);

/* 25da:141e */
void far pascal Hanging(void)
{
    signed char tile;
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
void far pascal ClimbUp(void)
{
    int sequence;
    unsigned char zero;
    signed char above;

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
void far pascal Drop(void)
{
    signed char under, behind;
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

extern signed char Plus1[2];

int far pascal IsCharSwingingFwd(void)
{
    int old_repeat = Char.sequence;
    int old_position = Char.sequence_position;
    int old_sequence = Char.sequence_id;
    unsigned int old_frame = Char.frame;

    AnimChar();
    Char.sequence_id = old_sequence;
    Char.sequence_position = old_position;
    Char.sequence = old_repeat;
    if (Char.frame <= old_frame)
        return 1;
    return 0;
}

/* 25da:16d2 */
void far pascal DoJumpHang(int back)
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

extern signed char far pascal GetCharCol(int x);
extern int far pascal GetDist1(int x);
extern int far pascal IsAdvancableId(signed char tile, unsigned int modifier);

/* 25da:1738 */
#define RUNNING_JUMP_REACH 9
#define OPPONENT_JUMP_REACH 9

int far pascal DoRunJump(int sequence)
{
    signed char tile, column;
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
void far pascal DoRetreat(void)
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
void far pascal DoAdvance(void)
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

extern int far pascal DoKidEngarde(void);
extern int far pascal DoThrowFireball(void);

/* 25da:1918 */
void far pascal DoEngarde(void)
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
int far pascal DoKidEngarde(void)
{
    signed char tile, slot;
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

extern unsigned int Player;
typedef struct {
    int wait, broken;
    unsigned char floors[8];
} BRIDGEREC;
extern BRIDGEREC Bridge;
extern int far pascal BrgSkelMustAdvance(void);

extern void far pascal LoadKidwOpp(signed char slot);
extern void far pascal OnAlert(void);
extern void far pascal DoTurnEngarde(void);
extern void far pascal DoSheathe(void);

/* 25da:1b92 */
void far pascal FightCtrl(void)
{
    signed char target;
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
        signed char tile = GetUnderFt();
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
struct opponent {
    signed char reserved[17];
    signed char attitude;
    signed char pad[5];
};

extern void far pascal LoadKid(void);
extern int far pascal IsDeadPos(unsigned int frame);
extern void far pascal LoadShad(signed char index);
extern struct opponent near * far pascal GetOppInfoIdx(int room, int index);

int far pascal OppToFight(void)
{
    CHARREC saved_actor;
    signed char index;
    int ready;
    struct opponent near *opponent;

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
                    if (opponent->attitude != 1 && opponent->attitude != 3)
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

extern int GuardTimer, Refract;
extern void far pascal HeadOnAlert(void);
extern int far pascal IsHeadLockedOn(signed char room, int slot);
extern void far pascal DoKidTurnEngarde(void);
extern void far pascal DoStrike(void);

/* 25da:1e08 */
void far pascal OnAlert(void)
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
void far pascal DoSheathe(void)
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
void far pascal DoTurnEngarde(void)
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
void far pascal DoKidTurnEngarde(void)
{
    signed char tile;
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
void far pascal DoStrike(void)
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
void far pascal DoBlock(void)
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
void far pascal CrushChar(void)
{
    signed char state = Char.action;
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
struct crush_record {
    signed char bytes_00[5];
    int sequence, position;
    signed char byte_09;
    signed char index;
    signed char byte_0b;
    signed char enabled;
    signed char bytes_0d[2];
    signed char kind;
    signed char byte_10;
    int pending;
};

/* 25da:2220 */
void far pascal CrushOppInfo(struct crush_record near *opponent, unsigned char room)
{
    if (Level.preferred_opponent != 2)
        opponent->enabled = 0;
    if (Level.preferred_opponent == 5) {
        if (opponent->kind == 8) {
            opponent->sequence = 169;
            opponent->position = 0;
        } else {
            RemoveOppFromRoom(room, opponent->index);
        }
    } else if (Level.preferred_opponent == 2) {
        if (opponent->pending == 1)
            opponent->pending = 0;
    } else {
        opponent->sequence = 22;
        opponent->position = 0;
    }
}

/* 25da:227c */
void far pascal SquishChar(void)
{
    JumpSeq(118);
    DecLifePts(100);
}
