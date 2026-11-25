/* AUTOHEAD.C. Routines in original DOS order. */

#pragma pack(1)
typedef struct {
    int x, y;
    unsigned char kind;
    int frame;
    int step;
    unsigned char reserved[6];
    unsigned char opponent_kind, padding;
    signed char opponent_mode;
} HeadSprite;

typedef struct {
    int height, width;
} SpriteBounds;

typedef struct {
    signed char x, y;
} NeckPoint;
#pragma pack()

#pragma pack(1)
struct char_bounds { int top, left, bottom, right; };
typedef struct {
    signed char slot;                  /* 00 */
    signed char direction;             /* 01 */
    int x, y;                          /* 02, 04 */
    signed char character_kind;        /* 06 */
    unsigned int frame;                /* 07 */
    signed char column, row;           /* 09, 0a */
    signed char action;                /* 0b */
    signed char horizontal_step;       /* 0c */
    signed char vertical_step;         /* 0d */
    signed char room;                  /* 0e */
    signed char field_0f;              /* 0f */
    signed char stance;                /* 10 */
    signed char alive;                 /* 11 */
    signed char life_points;           /* 12 */
    signed char life_initial;          /* 13 */
    signed char life_delta;            /* 14 */
    int sequence_position;             /* 15 */
    int sequence_id;                   /* 17 */
    unsigned int sequence;             /* 19 */
    struct char_bounds bounds;                    /* 1b: top, left, bottom, right */
    signed char alert_delay;           /* 23 */
    int sequence_group;                /* 24 */
    int previous_x, previous_y;        /* 26, 28 */
    unsigned int previous_frame;       /* 2a */
    struct char_bounds previous_bounds;           /* 2c: top, left, bottom, right */
    int field_34;                      /* 34 */
    int previous_mode;                 /* 36 */
    unsigned char behavior;            /* 38 */
    signed char opponent_index;        /* 39 */
    unsigned char field_3a;            /* 3a */
    unsigned char unknown_3b[5];        /* 3b..3f */
} CHARREC;
#pragma pack()


#pragma pack(1)
typedef struct {
    signed char index, flipped;                       /* 00-01 */
    int x, y, image;                                   /* 02,04,06 */
    unsigned char image_set;                          /* 08 */
    int clip_top, clip_left, clip_bottom, clip_right;    /* 09,0b,0d,0f */
    int depth;                                        /* 11 */
    unsigned char object_type;                        /* 13 */
} FCHARREC;
#pragma pack()
extern FCHARREC FChar;
extern CHARREC Kid;
extern CHARREC Opp;
extern CHARREC Char;
extern void near *Pics[5];
extern unsigned int hHeadChompFrame;

extern HeadSprite near *far pascal GetObjTableType(char kind, unsigned char index);
extern NeckPoint far *far pascal hLock(unsigned int handle);
extern int far *far pascal GetSetImgSize(void near *sheet, int frame, SpriteBounds far *bounds);
extern void far pascal hUnlock(unsigned int handle);

typedef struct HeadPlan HeadPlan;

#pragma pack(1)
typedef struct {
    unsigned char unknown_00;
    int spawn_x;
    unsigned char spawn_direction, skill;
    unsigned char unknown_05[10], kind, reserved;
    signed char mode, column, row;
    int timer;
    unsigned char sound_played;
} OpponentRecord;
#pragma pack()



#pragma pack(1)
typedef struct {
    signed char kind;                                 /* 00 */
    unsigned int modifier;                            /* 01 */
    signed char index;                                /* 03 */
    unsigned char room;                               /* 04 */
    signed char column, row;                          /* 05-06 */
    signed char first_col, last_col;                   /* 07-08 */
    unsigned char top_row, bottom_row;                 /* 09-0a */
} CELLREC;
#pragma pack()
extern CELLREC Cell;
extern unsigned char OppCharId[];


#pragma pack(1)
typedef struct {
    int image, equipment_image;                       /* 00,02 */
    signed char x, y;                                 /* 04-05 */
    unsigned char flags;                              /* 06 */
} FRAMEREC;
#pragma pack()
extern FRAMEREC Frame;
extern int ImWidth;
#pragma pack(1)
typedef struct {
    unsigned int modifier, unknown_02;
} LEVELCELL;

typedef struct {
    unsigned char left, right, above, below;
} ROOMLINKS;

typedef struct {
    signed char opponent_count;
    unsigned char opponent_data[5][23];
} ROOMOBJECTS;

typedef struct {
    signed char room, cell;
} REBIRTHPOINT;

typedef struct {
    signed char count;
    unsigned char spawn_flags;
    unsigned char header[2];
    unsigned char points[3][10];
} ROOMGENERATORS;

typedef struct {
    unsigned char cell_kinds[32][30];                  /* 0000 */
    LEVELCELL cell_info[32][30];                       /* 03c0 */
    unsigned char unknown_12c0[0x500];                 /* 12c0 */
    ROOMLINKS room_links[32];                          /* 17c0 */
    unsigned char room_count;                         /* 1840 */
    unsigned char unknown_1841[4];
    unsigned char environment_kind;                  /* 1845 */
    unsigned char unknown_1846;
    signed char level_number;                         /* 1847 */
    int background_overlays[8];                       /* 1848 */
    unsigned char unknown_1858[4];
    int actor_phase;                                  /* 185c */
    unsigned char unknown_185e[2];
    signed char entry_room, entry_cell, entry_facing;    /* 1860-1862 */
    unsigned char unknown_1863[3];
    unsigned char preferred_opponent;                /* 1866 */
    ROOMOBJECTS room_objects[32];                      /* 1867 */
    REBIRTHPOINT rebirth_points[2];                    /* 26e7 */
    unsigned char unknown_26eb[0x0e];
    ROOMGENERATORS generators[32];                     /* 26f9 */
    unsigned char unknown_2b39[0x3c0];                 /* 2b39-2ef8 */
} LEVELREC;
#pragma pack()
extern LEVELREC Level;
static const signed char first_chomp[12] = {
    1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 99
};
static const signed char last_chomp[12] = {
    1, 1, 2, 2, 2, 3, 3, 3, 3, 4, 4, 99
};
static const int flee_length[12] = {
    70, 65, 60, 55, 50, 45, 40, 35, 30, 25, 20, 15
};
typedef int COL_EDGE_X[20];                           /* DS:0d7c */
extern COL_EDGE_X ColEdgeX;
extern signed char Plus1[2];
extern signed char Minus1[2];

extern signed char far pascal ReadCell(unsigned char room, signed char column, signed char row);
extern int far pascal IsWall(signed char tile);
extern int far pascal IsSpace(signed char tile);
extern int far pascal GetBarrCode(unsigned char tile);
extern int far pascal IsCharClosestToKid(void);
extern int far pascal GateBarr(void);
extern int far pascal Rnd(int maximum);
extern void far pascal ResetCharY(void);
extern void far pascal ClearCharVel(void);
extern void far pascal AddSound(int sound);
extern void far pascal AddSong(int sound);
extern int far pascal SndOn(int sound);
extern int far pascal DecLifePts(signed char amount);
extern int far pascal AddCharX(int amount);
extern void far pascal JumpSeq(int sequence);
extern int far pascal GetBarrDistFromFace(unsigned char room, unsigned char column, unsigned char row);
extern signed char far pascal GetCellCol(void);
extern signed char far pascal GetCharCol(int room);
extern int far pascal GetBaseX(void);
extern int far pascal GetOppDist(void);
extern void far pascal SaveShad(void);
extern void far pascal DrawOppMeter(signed char id, int x, int y);
extern void far pascal LoadShad(signed char id);
extern OpponentRecord near *far pascal GetOppInfoIdx(int room, int id);
extern void far pascal LoadShadwOpp(signed char id);
extern void far pascal SaveShadwOpp(void);
extern void far pascal GetFrameInfo(void);
extern void far pascal GetCharEdges(void);
extern int far pascal IsFloor(signed char tile);
extern signed char far pascal Get1stWallColInDir(unsigned char room, signed char column,
                                                signed char row, signed char direction);
extern signed char far pascal GetUnderFt(void);
extern int far pascal GetDist(void);

int far pascal HeadClearInDir(int direction);
int far pascal HeadGoToDest(HeadPlan near *plan);
void far pascal HeadFaceKid(void);
void far pascal HeadGetAngry(HeadPlan near *plan);
void far pascal HeadGetPeaceful(HeadPlan near *plan);
void far pascal HeadFocusOnNeck(HeadPlan near *plan);
int far pascal HeadSetToFleeMode(void);
int far pascal IsAbleToFleeInRoom(unsigned char room);
int far pascal JustStartVerticalSeq(void);
int far pascal XWithinMiddleOfCell(int first, int second);
int far pascal YWithinMiddleOfCell(int value, int half_width);
int far pascal SnakeSlitheringTowardOpp(void);
void far pascal SnakeAttack(void);
void far pascal SnakeComeOutOfHole(OpponentRecord near *record);
void far pascal SnakeGoIntoHole(OpponentRecord near *record);

int abs(int value);
#pragma intrinsic(abs)

void far pascal unused_head_entry(void)
{
}

void far pascal AlignHeadOnKidNeck(void)
{
    signed char neck_x;
    SpriteBounds bounds;
    HeadSprite near *body;
    NeckPoint far *neck;

    body = GetObjTableType(2, 0);
    if (body == 0)
        return;

    neck = hLock(hHeadChompFrame);
    if (Kid.direction != Char.direction)
        neck_x = neck[body->frame].y;
    else {
        GetSetImgSize(Pics[3], FChar.image, &bounds);
        neck_x = -((Char.character_kind == 7 ? 10 : 16) + neck[body->frame].y + (signed char)bounds.width);
    }
    if (Char.character_kind == 8)
        neck_x += 6;
    if (Char.direction == 0)
        neck_x = -neck_x;
    FChar.x = body->x + neck_x;
    FChar.y = body->y + ((signed char far *)neck)[body->frame * 2];
    hUnlock(hHeadChompFrame);
    Char.x = FChar.x + 130;
}

/* FM Towns CharCanSeeOpp: the same statements and the same order of locals. */
#define ABS(x) ((x) < 0 ? -(x) : (x))

int far pascal CharCanSeeOpp(void)
{
    signed char end_column, end_row, column, row, columns, rows, step_column, step_row, straight_error, diagonal, error, tile;
    int clear = 1;
    int result;

    if (Opp.column == Char.column && Opp.row == Char.row) {
        result = 4;
    } else {
        row = Char.row * 2;
        column = Char.column * 2;
        if ((Char.x + Frame.x - 122) % 32 > 16)
            ++column;
        end_row = Opp.row * 2 + 1;
        end_column = Opp.column * 2;
        if ((Opp.x - 130) % 32 > 16)
            ++end_column;

        columns = ABS(end_column - column);
        rows = ABS(end_row - row);
        step_column = end_column > column ? 1 : -1;
        step_row = end_row > row ? 1 : -1;

        if (columns > rows) {
            error = rows * 2 - columns;
            straight_error = rows * 2;
            diagonal = (rows - columns) * 2;
            do {
                column += step_column;
                if ((step_column > 0 && column % 2 == 0) ||
                    (step_column < 0 && column % 2 != 0)) {
                    tile = ReadCell(Char.room, column / 2, row / 2);
                    clear = !IsWall(tile);
                }
                if (clear) {
                    if (error < 0)
                        error += straight_error;
                    else {
                        if ((step_row > 0 && row % 2 != 0) ||
                            (step_row < 0 && row % 2 == 0)) {
                            if (step_row < 0)
                                tile = ReadCell(Char.room, column / 2, (row - 1) / 2);
                            else
                                tile = ReadCell(Char.room, column / 2, row / 2);
                            clear = IsSpace(tile);
                        }
                        row += step_row;
                        error += diagonal;
                    }
                }
            } while (--columns && clear);
        } else {
            error = columns * 2 - rows;
            straight_error = columns * 2;
            diagonal = (columns - rows) * 2;
            do {
                if ((step_row > 0 && row % 2 != 0) ||
                    (step_row < 0 && row % 2 == 0)) {
                    if (step_row < 0)
                        tile = ReadCell(Char.room, column / 2, (row - 1) / 2);
                    else
                        tile = ReadCell(Char.room, column / 2, row / 2);
                    clear = IsSpace(tile);
                }
                if (clear) {
                    row += step_row;
                    if (error < 0)
                        error += straight_error;
                    else {
                        column += step_column;
                        if ((step_column > 0 && column % 2 == 0) ||
                            (step_column < 0 && column % 2 != 0)) {
                            tile = ReadCell(Char.room, column / 2, row / 2);
                            clear = !IsWall(tile);
                        }
                        error += diagonal;
                    }
                }
            } while (--rows && clear);
        }
        if (clear) {
            if (!IsCharClosestToKid())
                result = 4;
            else if (Opp.row != Char.row)
                result = step_row > 0 ? 2 : 3;
            else
                result = step_column > 0 ? 1 : 0;
        } else
            result = -1;
    }
    return result;
}

#pragma pack(1)
struct HeadPlan {
    signed char mode, column, row;
    union {
        struct {
            signed char direction;
            unsigned char flags;
        } attack;
        int flee_timer;
    } state;
    unsigned char sound_played;
};
#pragma pack()

int far pascal ChompAgain(HeadPlan near *plan)
{
    unsigned char room_skill = Level.room_objects[(unsigned char)Char.room - 1].opponent_data[Char.slot][4];
    int result;
    signed char column;

    if (plan->column < first_chomp[room_skill])
        result = 1;
    else if (last_chomp[room_skill] <= (column = plan->column))
        result = 0;
    else
        result = Rnd(1);
    return result;
}

HeadSprite near *far pascal HeadAddOpponent(HeadSprite near *sprite)
{
    if (sprite->opponent_kind == 0) {
        sprite->opponent_kind = 5;
        Char.character_kind = OppCharId[5];
        ResetCharY();
    }
    if (Char.character_kind == 11) {
        if (Char.alive < 0) {
            if (sprite->opponent_mode <= 0) {
                sprite->frame = 170;
                sprite->step = 0;
                sprite->opponent_mode = -1;
            } else {
                sprite->frame = 175;
                sprite->step = 0;
                Char.sequence_group = 6;
            }
            Char.stance = 1;
            AddSound(71);
        } else
            Char.stance = 0;
    } else {
        Char.x = ColEdgeX[Char.column + 5] + 30;
        Char.direction = -1;
        sprite->frame = 130;
        sprite->step = 0;
        sprite->opponent_mode = Char.stance = 0;
    }
    return sprite;
}

void far pascal HeadAttackProg(HeadPlan near *plan)
{
    unsigned char dx, dy;
    int direction, vertical_sequence;

    if (Char.sequence == 140 || Char.sequence == 152 ||
        Char.sequence == 156 || Char.sequence == 162)
        return;
    if ((int)Char.sequence >= 147 && (int)Char.sequence <= 151) {
        HeadFocusOnNeck(plan);
        return;
    }
    if (Char.frame != 150 && Char.frame != 165 && Char.frame != 166 &&
        Char.frame != 158 && Char.frame != 173)
        return;
    direction = CharCanSeeOpp();
    if (direction != -1) {
        plan->state.attack.direction = 1;
        {
            int delta = Char.column - Opp.column;
            dx = delta < 0 ? -delta : delta;
        }
        {
            int delta = Char.row - Opp.row;
            dy = delta < 0 ? -delta : delta;
        }
        if (dy == 0 && dx < 2) {
            vertical_sequence = Char.x < Opp.x ? 1 : 0;
            if (dx != 0 && !HeadClearInDir(vertical_sequence)) {
                if (Char.sequence != 166 && YWithinMiddleOfCell(Char.y, 15))
                    JumpSeq(166);
                return;
            }
            if (Char.sequence != 147)
                JumpSeq(147);
        } else if (dx == 0 && dy == 1 && HeadClearInDir(direction)) {
            if (Opp.row < Char.row)
                vertical_sequence = 143;
            else
                vertical_sequence = 144;
            if (vertical_sequence != Char.sequence)
                JumpSeq(vertical_sequence);
        } else if (direction == 4) {
            if (dx || dy) {
                if (Char.sequence != 166 && YWithinMiddleOfCell(Char.y, 15))
                    JumpSeq(166);
            }
        } else {
            plan->row = Opp.row;
            plan->column = Opp.column;
            if ((direction == 3 || direction == 2) && !HeadClearInDir(direction))
                direction = Char.x < Opp.x ? 1 : 0;
            if (HeadClearInDir(direction)) {
                if (direction == 3)
                    direction = 143;
                else if (direction == 2)
                    direction = 144;
                else {
                    direction = 142;
                    HeadFaceKid();
                    ResetCharY();
                }
            } else {
                direction = 166;
                ClearCharVel();
            }
            if (Char.sequence != direction)
                JumpSeq(direction);
        }
    } else {
        if (plan->state.attack.direction != 0) {
            plan->state.attack.direction = 0;
            plan->column = Opp.column;
            plan->row = Opp.row;
        }
        HeadGoToDest(plan);
    }
}

int far pascal HeadBlocked(void)
{
    int sequence;
    if (DecLifePts(1))
        sequence = 154;
    else {
        AddSound(87);
        HeadSetToFleeMode();
        sequence = 153;
    }
    return sequence;
}

int far pascal HeadClearInDir(int direction)
{
    signed char tile;
    int clear = 0;
    switch (direction) {
    case 0:
        if (Char.column < -1 || JustStartVerticalSeq())
            break;
        tile = ReadCell(Char.room, Char.column - 1, Char.row);
        if (IsWall(tile))
            break;
        if (tile == 4 && GateBarr())
            break;
        clear = 1;
        break;
    case 1:
        if (Char.column > 10 || JustStartVerticalSeq())
            break;
        tile = ReadCell(Char.room, Char.column + 1, Char.row);
        if (IsWall(tile))
            break;
        if (tile == 4 && GateBarr())
            break;
        if (ReadCell(Char.room, Char.column, Char.row) != 4 || !GateBarr())
            clear = 1;
        break;
    case 2:
        if (Char.row != 2 && XWithinMiddleOfCell(Char.x, 8))
            clear = IsSpace(ReadCell(Char.room, Char.column, Char.row));
        break;
    case 3:
        if (Char.row != 0 && XWithinMiddleOfCell(Char.x, 8))
            clear = IsSpace(ReadCell(Char.room, Char.column, Char.row - 1));
        break;
    }
    return clear;
}

void far pascal HeadDoneAttackingKid(HeadPlan near *plan)
{
    HeadGetPeaceful(plan);
    Char.sequence_group = 0;
}

int far pascal HeadEnemyCollide(int minimum_gap)
{
    int sequence = -1;
    int gap;

    if (Char.sequence_group != 1) {
        gap = GetBarrDistFromFace((unsigned char)Cell.kind, Cell.room, (unsigned char)Cell.column);
        if (gap < 5 && minimum_gap <= gap && gap < 5 && -ImWidth <= gap) {
            Char.x = AddCharX(-(gap + 14));
            if (Char.alive < 0 && Char.sequence != 153)
                sequence = 157;
            else if (Char.sequence != 164) {
                if (Char.frame <= 191) {
                    sequence = 164;
                    Char.x = ColEdgeX[GetCellCol() + 5] + 14;
                    if (Char.direction == -1)
                        Char.x += 32;
                } else
                    JumpSeq(145);
                DecLifePts(100);
            }
        }
    }
    return sequence;
}

void far pascal HeadFaceKid(void)
{
    signed char column = Char.column;
    if (column < Opp.column) {
        if (Char.direction != 0) {
            Char.x = AddCharX(-14);
            Char.direction = 0;
        }
    } else if (Char.direction != -1) {
        Char.x = AddCharX(-14);
        Char.direction = -1;
    }
}

void far pascal HeadFleeProg(HeadPlan near *plan)
{
    if (Char.sequence != 153) {
        if (Char.sequence == 159) {
            if (plan->column == Char.column && XWithinMiddleOfCell(Char.x, 2)) {
                if (Char.row == Opp.row && abs(Char.column - Opp.column) < 3)
                    HeadGetAngry(plan);
                else {
                    JumpSeq(160);
                    plan->state.flee_timer = flee_length[Level.room_objects[(unsigned char)Char.room - 1].opponent_data[Char.slot][4]];
                }
            }
        } else if (Char.row == Opp.row && abs(Char.column - Opp.column) < 3 &&
                   Opp.life_points && (Opp.sequence != 70 || (Opp.frame != 0 && Opp.frame < 223)))
            HeadGetAngry(plan);
        else if (--plan->state.flee_timer == 0) {
            if (Opp.life_points && (Opp.sequence != 70 || (Opp.frame != 0 && Opp.frame < 223)) &&
                CharCanSeeOpp() != -1) {
                HeadGetAngry(plan);
                JumpSeq(162);
            } else {
                HeadGetPeaceful(plan);
                JumpSeq(163);
            }
        }
        if (!plan->sound_played && !SndOn(10087)) {
            plan->sound_played = 1;
            AddSong(146);
        }
    } else if (Char.sequence_group == 2) {
        Char.alert_delay = 0;
        JumpSeq(159);
    }
}

void far pascal HeadFocusOnNeck(HeadPlan near *plan)
{
    int low, high, distance;
    if (Char.sequence == 147 || Char.sequence == 144 || Char.sequence == 143) {
        if (Opp.stance == 1) {
            low = -25;
            high = 0;
        } else {
            low = -3;
            high = 30;
        }
        distance = GetOppDist();
        if (Char.row == Opp.row && low <= distance && high >= distance) {
            ClearCharVel();
            JumpSeq(Rnd(1) + 150);
            plan->mode = 3;
            plan->column = 1;
        } else {
            int close_limit = Opp.stance == 1 ? -35 : -13;
            if ((Char.row != Opp.row || close_limit > distance) && Char.sequence == 147) {
                ClearCharVel();
                JumpSeq(156);
            }
            if (distance - 64 <= high && Char.row == Opp.row && Char.sequence != 144)
                AddSound(90);
        }
    }
}

void far pascal HeadGetAngry(HeadPlan near *plan)
{
    plan->mode = 1;
    plan->row = Opp.row;
    plan->column = Opp.column;
    plan->state.attack.direction = 1;
    JumpSeq(140);
    ClearCharVel();
    HeadFaceKid();
    Char.alert_delay = 3;
    Char.stance = 1;
    if (Opp.opponent_index == -1) {
        SaveShad();
        DrawOppMeter(Char.slot, Char.life_points, Char.life_initial);
        LoadShad(Char.slot);
        Opp.opponent_index = Char.slot;
    }
    AddSound(87);
}

void far pascal HeadGetPeaceful(HeadPlan near *plan)
{
    signed char zero = 0;
    plan->mode = zero;
    Char.stance = zero;
    ClearCharVel();
    JumpSeq(139);
    Char.column = GetCharCol(GetBaseX());
    Char.x = ColEdgeX[Char.column + 5] + 30;
    ResetCharY();
    Char.alert_delay = 0;
}

int far pascal HeadGoToDest(HeadPlan near *plan)
{
    int clear = 0;
    int sequence;
    int direction;
    if (Char.column != plan->column || plan->row != Char.row) {
        if (plan->row != Char.row) {
            if (plan->row > Char.row) {
                direction = 2;
                sequence = 144;
            } else {
                direction = 3;
                sequence = 143;
            }
            clear = HeadClearInDir(direction);
            if (clear && Char.sequence != sequence)
                JumpSeq(sequence);
        }
        if (!clear && Char.column != plan->column) {
            if (plan->column > Char.column)
                sequence = 1;
            else
                sequence = 0;
            clear = HeadClearInDir(sequence);
            if (clear && Char.sequence != 142)
                JumpSeq(142);
        }
    }
    if (!clear)
        HeadGetPeaceful(plan);
    return clear;
}

void far pascal HeadLockedProg(HeadPlan near *plan)
{
    int lost = 0;
    OpponentRecord near *record;
    if ((Char.sequence == 150 && Char.frame == 167 && Char.sequence_group != 1) ||
        (Char.sequence == 151 && Char.frame == 166)) {
        if (ChompAgain(plan)) {
            if (Opp.alive < 0) {
                ++plan->column;
                JumpSeq(150 + (Char.sequence == 150));
            } else {
                record = GetOppInfoIdx((unsigned char)Char.room, Char.slot);
                if (record && record->skill >= 11)
                    JumpSeq(165);
            }
        } else {
            JumpSeq(152);
            Char.x = AddCharX(-8);
            lost = 1;
        }
    }
    if (lost || Char.sequence == 158 || Char.sequence == 157) {
        plan->mode = 1;
        plan->column = Opp.column;
        plan->row = Opp.row;
    }
}

void far pascal HeadOnAlert(void)
{
}

void far pascal HeadPeacefulProg(void)
{
    int direction, sequence;
    if (Char.frame != 150)
        return;
    direction = Rnd(3);
    if (direction == 3) {
        if ((int)Char.sequence >= 131 && (int)Char.sequence <= 133)
            direction = 0;
        else if ((int)Char.sequence >= 134 && (int)Char.sequence <= 136)
            direction = 1;
        else
            direction = -1;
    } else if (direction == 2)
        direction += Rnd(1);
    if (direction != -1 && HeadClearInDir(direction)) {
        if (direction == 3)
            sequence = 137;
        else if (direction == 2)
            sequence = 138;
        else if (direction == 0)
            sequence = Rnd(2) + 131;
        else
            sequence = Rnd(2) + 134;
        JumpSeq(sequence);
    }
}

void far pascal HeadProg(void)
{
    OpponentRecord near *record;
    HeadPlan near *plan;
    if (Char.alive >= 0)
        return;
    record = GetOppInfoIdx((unsigned char)Char.room, Char.slot);
    if (!record)
        return;
    plan = (HeadPlan near *)&record->mode;
    LoadShadwOpp(Char.slot);
    GetFrameInfo();
    if (Frame.image != -1) {
        FChar.image = Frame.image;
        GetCharEdges();
    }
    switch (plan->mode) {
    case 0:
        if (Opp.life_points && (Opp.sequence != 70 || (Opp.frame != 0 && Opp.frame < 223)) &&
            (Char.frame == 150 || Char.frame == 165 || Char.frame == 166 ||
             Char.frame == 158 || Char.frame == 173) && CharCanSeeOpp() != -1)
            HeadGetAngry(plan);
        else
            HeadPeacefulProg();
        break;
    case 1:
        if (Opp.life_points && (Opp.sequence != 70 || (Opp.frame != 0 && Opp.frame < 223)))
            HeadAttackProg(plan);
        else
            HeadDoneAttackingKid(plan);
        break;
    case 2:
        HeadFleeProg(plan);
        break;
    case 3:
        if (Opp.life_points && (Opp.sequence != 70 || (Opp.frame != 0 && Opp.frame < 223)))
            HeadLockedProg(plan);
        else
            HeadDoneAttackingKid(plan);
        break;
    }
    SaveShadwOpp();
}

int far pascal HeadSetToFleeMode(void)
{
    unsigned char skill;
    signed char column;
    int clear, edge, initial_column;
    OpponentRecord near *record;

    skill = Level.room_objects[(unsigned char)Char.room - 1].opponent_data[Char.slot][4];
    if (IsAbleToFleeInRoom(Char.room) && Rnd(skill / 3 + 1) == 0 &&
        ((Char.direction == 0 && Char.column >= 3) ||
         (Char.direction == -1 && Char.column + 3 <= 10))) {
        clear = 1;
        initial_column = *(volatile signed char *)&column = Char.column;
        edge = Char.direction == 0 ? -1 : 10;
        if (initial_column != edge)
            do {
                if (!clear)
                    break;
                skill = ReadCell(Char.room, column, Char.row);
                if (GetBarrCode(skill) && (skill != 4 || GateBarr()))
                    clear = 0;
                column += Minus1[Char.direction + 1];
            } while (column != edge);
        if (!clear) {
            column += Plus1[Char.direction + 1] * 2;
            if (abs(column - Char.column) >= 3)
                clear = 1;
        } else
            column += Plus1[Char.direction + 1];
        if (clear) {
            Char.stance = 0;
            Char.alert_delay = 1;
            record = GetOppInfoIdx((unsigned char)Char.room, Char.slot);
            record->mode = 2;
            record->column = column;
            record->row = Char.row;
            record->timer = -1;
            record->sound_played = 0;
            SaveShad();
            DrawOppMeter(Char.slot, 0, Char.life_initial);
            LoadShad(Char.slot);
            if (Char.slot == Opp.opponent_index)
                Opp.opponent_index = -1;
        }
    } else
        clear = 0;
    return clear;
}

int far pascal IsAbleToFleeInRoom(unsigned char room)
{
    signed char slot, count;
    int room_number = room;
    int available;

    count = (unsigned char)Level.room_objects[room_number - 1].opponent_count;
    if (count > 1) {
        available = 1;
        slot = 0;
        do {
            if (!(slot < count))
                break;
            {
                int can_flee = GetOppInfoIdx(room, slot)->mode != 2;
                available = can_flee;
                ++slot;
                if (!can_flee)
                    break;
            }
        } while (1);
    } else
        available = 0;
    return available;
}

extern signed char far pascal GetCellsInFront(signed char direction);

int far pascal IsHeadLockedOn(unsigned char room, int slot)
{
    OpponentRecord near *record;
    int result;
    record = GetOppInfoIdx(room, slot);
    result = record != 0 && record->mode == 3;
    return result;
}

int far pascal JustStartVerticalSeq(void)
{
    int baseline = Char.row * 63 + 37;
    int result;
    if (Char.sequence != 144) {
        if (Char.sequence == 143)
            result = Char.y < baseline;
        else
            result = 0;
        return result;
    } else if (Char.y > baseline)
        result = 1;
    else
        result = 0;
    return result;
}

int far pascal XWithinMiddleOfCell(int first, int second)
{
    int result;
    first = (first - 144) % 32;
    result = first >= 16 - second && 16 + second >= first;
    return result;
}

int far pascal YWithinMiddleOfCell(int value, int half_width)
{
    int result;
    value = (value - 3) % 63;
    result = value >= 31 - half_width && 31 + half_width >= value;
    return result;
}

void far pascal SnakeProg(void)
{
    OpponentRecord near *record;
    if (Char.alive < 0) {
        record = GetOppInfoIdx((unsigned char)Char.room, Char.slot);
        if (record) {
            LoadShadwOpp(Char.slot);
            if (record->mode < 0) {
                if (SnakeSlitheringTowardOpp())
                    SnakeAttack();
                else
                    SnakeGoIntoHole(record);
            } else
                SnakeComeOutOfHole(record);
            SaveShadwOpp();
        }
    }
}

void far pascal SnakeAttack(void)
{
    int hit = 0;
    int distance = GetOppDist();
    signed char tile;
    if (distance >= 13 && distance <= 45) {
        if (Char.frame == 278 || Char.frame == 279 ||
            (Char.sequence == 176 &&
             ((Opp.frame >= 1 && Opp.frame <= 14) || (Opp.frame >= 49 && Opp.frame <= 56))))
            hit = 1;
        else if (Char.sequence != 176 && Char.sequence != 173) {
            JumpSeq(176);
            AddSound(71);
        }
    } else if (Char.sequence == 176 &&
               ((Opp.frame >= 1 && Opp.frame <= 14) || (Opp.frame >= 49 && Opp.frame <= 56)) &&
               distance > 13 && distance < 50)
        hit = 1;
    else if (Char.sequence == 176 && Opp.frame >= 135 && Opp.frame <= 149 &&
             distance > 13 && distance < 45)
        hit = 1;
    else if ((Char.frame == 278 || Char.frame == 279) &&
             (distance < 13 || distance > 49)) {
        tile = GetCellsInFront(1);
        if (IsFloor(tile) || (tile == 7 && (Cell.modifier & 128)))
            JumpSeq(177);
    }
    if (hit && Char.sequence != 173) {
        Char.x = AddCharX(distance - 32);
        JumpSeq(173);
    }
}

void far pascal SnakeComeOutOfHole(OpponentRecord near *record)
{
    signed char tile;
    signed char left, right;
    if (--record->mode > 0)
        return;
    record->mode = -1;
    left = Get1stWallColInDir(Char.room, Char.column, Char.row, -1);
    right = Get1stWallColInDir(Char.room, Char.column, Char.row, 1);
    tile = GetUnderFt();
    if ((unsigned char)Opp.room == (unsigned char)Char.room && Char.row == Opp.row &&
        Char.column > left && Char.column < right) {
        if (Char.column <= Opp.column &&
            (!IsWall(tile) || IsFloor(tile) || (tile == 7 && (Cell.modifier & 128))))
            Char.direction = 0;
        else
            Char.direction = -1;
    } else
        Char.direction = ~Char.direction;
    if (IsWall(tile) && !IsFloor(tile) && (tile != 7 || !(Cell.modifier & 128))) {
        JumpSeq(170);
        Char.x = AddCharX(38);
    } else {
        JumpSeq(172);
    }
    Char.sequence_group = 0;
}

void far pascal SnakeGoIntoHole(OpponentRecord near *record)
{
    signed char tile;
    int distance;
    if (Opp.alive >= 0) {
        if (Char.sequence != 176 && Char.frame != 278 && Char.frame != 279)
            JumpSeq(176);
        return;
    }
    if (Char.sequence != 171 && Char.sequence != 172 && Char.sequence_group != 9) {
        GetUnderFt();
        if ((((unsigned char *)&Cell.modifier)[1] & 15) == 12) {
            distance = (Char.x - 144) % 32;
            if (distance >= 14 && distance <= 18) {
                distance = 16 - distance;
                if (Char.direction == -1)
                    distance = -distance;
                else
                    distance += 8;
                Char.x = AddCharX(distance);
                JumpSeq(171);
            }
        } else if (Char.sequence != 176 && Char.sequence != 168 &&
                   Char.frame != 278 && Char.frame != 279) {
            tile = GetCellsInFront(1);
            if (IsFloor(tile) || (tile == 7 && (Cell.modifier & 128)))
                return;
            if (!IsWall(tile))
                JumpSeq(176);
            else {
                if (tile != 7) {
                    if (Char.direction == 0) {
                        Char.sequence_group = 9;
                        return;
                    }
                    if (Char.frame != 254)
                        return;
                }
                JumpSeq(168);
            }
        } else {
            tile = GetCellsInFront(1);
            if (IsFloor(tile) || (tile == 7 && (Cell.modifier & 128)))
                JumpSeq(177);
        }
    } else {
        if (Char.sequence_group == 9) {
            tile = GetUnderFt();
            distance = GetDist();
            if (IsWall(tile) && distance < 8) {
                Char.frame = 264;
                JumpSeq(168);
            }
        }
        if (Char.frame == 264) {
            Char.sequence_group = 6;
            record->mode = Char.alert_delay == 2 ? 6 : Rnd(60) + 36;
        }
    }
}

int far pascal SnakeSlitheringTowardOpp(void)
{
    int result = 0;
    if (Char.sequence != 171 && Char.alert_delay >= 2) {
        if ((Char.direction == 0 && Char.column <= Opp.column) ||
            (Char.direction == -1 && Char.column >= Opp.column))
            result = 1;
    }
    return result;
}

void far pascal SnakeUpdateOppInfo(OpponentRecord near *record)
{
    signed char tile;
    if (Char.sequence == 171 || Char.sequence_group == 9) {
        Char.sequence_group = 6;
        record->mode = Rnd(60) + 36;
    } else {
        tile = GetCellsInFront(1);
        if (!IsFloor(tile) && (tile != 7 || !(Cell.modifier & 128)) &&
            (Char.sequence == 176 || Char.sequence == 168 || Char.sequence == 170)) {
            record->spawn_direction = ~Char.direction;
            record->spawn_x = AddCharX(-32);
            return;
        }
    }
}

unsigned int hHeadChompFrame = 0;
