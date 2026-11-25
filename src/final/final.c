#pragma pack(1)
typedef struct {
    unsigned char number;                             /* 00 */
    unsigned char left, right, above, below;           /* 01-04 */
    unsigned char above_left, above_right;             /* 05-06 */
    unsigned char below_left, below_right;             /* 07-08 */
    unsigned char custom;                             /* 09 */
} ROOMREC;

typedef struct {
    signed char index, flipped;                       /* 00-01 */
    int x, y, image;                                   /* 02,04,06 */
    unsigned char image_set;                          /* 08 */
    int clip_top, clip_left, clip_bottom, clip_right;    /* 09,0b,0d,0f */
    int depth;                                        /* 11 */
    unsigned char object_type;                        /* 13 */
} FCHARREC;

typedef struct {
    int x, y;                                         /* 00,02 */
    unsigned char room;                               /* 04 */
    int velocity, auxiliary;                          /* 05,07 */
    signed char kind, row;                             /* 09-0a */
    unsigned int flags;                               /* 0b */
} MOBREC;

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

/* FINAL.C, MSC 6.00A medium model. Claims and drafts are listed in notes.md. */

#pragma pack(1)

typedef signed char sbyte;
typedef unsigned char byte;

typedef struct {
    byte kind;
    byte attributes[3];
    unsigned int animation;
} FinalObject;

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

extern ROOMREC Room;
extern LEVELREC Level;
extern int Pause;
extern CHARREC Char;
extern CHARREC Opp;
extern CHARREC RoomOpp[5];
extern CHARREC Kid;
extern int TrobCt;

extern sbyte far pascal ReadCell(sbyte kind, sbyte room, sbyte column);
extern unsigned int far pascal Rnd(unsigned int max);
extern sbyte far pascal FirstInRow(sbyte row);
extern void far pascal select_room_arrays(sbyte kind);
extern void far pascal TriggerFlask(sbyte kind, sbyte cell);
extern void far pascal MarkRedraw(int cell);
extern void far pascal JumpSeq(int sequence);
extern void far pascal AddSound(int sound);
extern int far pascal IsDeadPos(int sequence);

void far pascal DrawFinlCrystal(FinalObject near *object);
void far pascal DrawFinlGargoyle(FinalObject near *object);
void far pascal FinlGenerateOpp(sbyte variant, sbyte room);

void far pascal DrawFinlObj(FinalObject near *object)
{
    if (Room.number == 6 && Level.environment_kind == 6 &&
        (object->animation & 0x1000))
        DrawFinlCrystal(object);

    if ((Room.number == 7 || Room.number == 8) &&
        Level.environment_kind == 6 && (object->animation & 0x1000))
        DrawFinlGargoyle(object);
}

void far pascal LoadFinlCode(void)
{
    Pause = 0;
}

void far pascal FinlCheckDeadOpp(void)
{
    if (Char.alive == 6) {
        sbyte cell;
        unsigned int frame;

        cell = ReadCell(Char.room, Char.column, Char.row);
        frame = Rnd(2);
        if (frame != 0 && cell == 1 && TrobCt < 4) {
            cell = FirstInRow(Char.row) + Char.column;
            if (cell < 22 || cell > 24) {
                Level.cell_kinds[2][cell] = 10;
                Level.cell_info[2][cell].modifier = (frame | 0x200) << 5;
                select_room_arrays(3);
                TriggerFlask(3, cell);
                MarkRedraw(cell);
            }
        }
        ++Char.alive;
    } else if (Char.alive >= 96 && Char.sequence != 239) {
        JumpSeq(239);
        AddSound(197);
        ++Char.alive;
    }
    if (Char.field_0f == 0 && Char.alive > 6)
        ++Char.alive;
}

void far pascal FinlCheckGenOpp(sbyte count)
{
    sbyte variant;
    sbyte room = -1;

    if (count <= 1) {
        if (count == 1) {
            if (RoomOpp[0].sequence != 238) {
                room = (RoomOpp[0].row == 1) ? 2 : 1;
                if (room == Kid.row)
                    room = -1;
            }
        } else {
            room = (Kid.row == 2) ? 1 : 2;
        }
    }
    if (room >= 0 && (count == 0 || Rnd(20) == 0)) {
        variant = (room == 1) ? 4 : 2;
        if (Rnd(1))
            variant += 4;
        FinlGenerateOpp(variant, room);
    }
}

typedef struct {
    byte cell;
    int x;
    sbyte direction;
    byte style;
    byte unused[4];
    byte bank;
    byte index;
    byte status;
    byte strength;
    byte delay;
    byte unused2[2];
    byte initial_strength;
    sbyte stance[4];
    union {
        sbyte row;
        int transition_state;
    } location;
} OpponentSlot;

extern int ColEdgeX[20];
extern signed char OppBank[2];
extern byte OppCharId[];

extern OpponentSlot near *far pascal GetOppInfoIdx(int bank, int serial);
extern void far _fastcall PresetOppBanks(byte kind);
extern void far pascal SetOppBanks(void);
extern void far pascal ResetCharY(void);
extern void far pascal ClearCharVel(void);
extern void far pascal AnimChar(void);
extern void far pascal SaveShad(void);

void far pascal FinlGenerateOpp(sbyte variant, sbyte room)
{
    sbyte index;
    OpponentSlot near *opponent;

    opponent = GetOppInfoIdx(3, index = Level.room_objects[2].opponent_count++);
    opponent->cell = FirstInRow(room) + variant;
    opponent->x = ColEdgeX[variant + 5];
    if (variant < 5)
        opponent->direction = 0;
    else
        opponent->direction = -1;
    opponent->style = 5;
    if (OppBank[0] == 0) {
        PresetOppBanks(1);
        SetOppBanks();
    }
    opponent->bank = OppBank[0];
    opponent->index = index;
    opponent->delay = opponent->status = 0;
    opponent->location.row = room;
    opponent->initial_strength = opponent->strength = Rnd(2) + 3;
    Char.slot = index;
    Char.direction = opponent->direction;
    Char.x = opponent->x;
    Char.character_kind = OppCharId[((sbyte)Level.preferred_opponent)];
    Char.column = variant;
    Char.row = room;
    Char.behavior = 0;
    Char.room = 3;
    Char.life_delta = Char.life_points = Char.life_initial = opponent->strength;
    Char.alive = -1;
    Char.field_34 = 4;
    ResetCharY();
    ClearCharVel();
    Char.sequence_group = 0;
    Char.field_0f = 1;
    Char.stance = 0;
    JumpSeq(238);
    Char.alert_delay = 0;
    AnimChar();
    AddSound(197);
    if ((byte)Kid.opponent_index == (byte)-1)
        Kid.opponent_index = Char.slot;
    SaveShad();
}

int far pascal FinlGetCutDirToJaffarBurning(void)
{
    int index = RoomOpp[0].sequence == 243 ? 0 : 1;
    int next;

    if (Room.number == 7 && RoomOpp[index].column >= 10)
        next = 1;
    else if (Room.number == 8 && RoomOpp[index].column < 0)
        next = 0;
    else
        next = -1;
    return next;
}

extern FCHARREC FChar;

void far pascal FinlClipChar(void)
{
    if (Char.room == 3 && Level.environment_kind == 6 &&
        Char.row == 1 && IsDeadPos((int)Char.frame))
        FChar.clip_bottom = 119;
}

extern sbyte CutRoom;
extern struct {
    signed char forward, back, up, down, action;
} Dir;
extern unsigned int current_song;
extern sbyte LevelIdx;
extern int NextLevel;
extern struct {
    unsigned char enabled;
    signed char cue;
    int song, delay, reserved[2];
} ambient_state;
extern struct {
    int control_mode, fade_active, playing_nis, recording_mode, skip_opening, field_0a;
    int plateau_loaded, throne_song_added, throne_nis_played, escher_room_eight_seen, nis_room_restored;
} GameState;
extern void far pascal ClrAll(void);
extern int far pascal SndOn(int);
extern void far pascal AddSong(int);
extern void far pascal PlayNIS(int);
extern void far pascal reset_level_overlays(void);
extern void far pascal LoadLevel(sbyte);
extern void far pascal SetPalette(int ticks, int speed, byte color, byte direction);
extern void far pascal SetKidPalette(sbyte room);

void far pascal FinlWatchKid(void);
void far pascal FinlWatchOpps(void);

void far pascal FinlCutKidToChessRoom(void)
{
    Char.field_34 = 2;
    Char.direction = 0;
    JumpSeq(55);
    Char.stance = 1;
    AddSound(19);
    CutRoom = Char.room = 4;
    Char.column = 5;
    Char.x = ColEdgeX[10] + 14;
    Char.row = 1;
    ResetCharY();
    Char.life_delta = Char.life_points;
    AnimChar();
    Kid = Char;
    ClrAll();
    Dir.action = 0;
}

void far pascal FinlFrameFunc(void)
{
    FinlWatchKid();
    FinlWatchOpps();

    if (ambient_state.enabled && !SndOn(current_song + 10000) &&
        Kid.alive < 0 && LevelIdx == NextLevel) {
        int tune = -1;

        switch (Room.number) {
        case 3:
            tune = 269;
            break;
        case 6:
            tune = (byte)Level.room_objects[5].opponent_count ? 270 : 268;
            break;
        case 8:
            GameState.escher_room_eight_seen = 1;
            /* Fall through. */
        case 7:
            tune = GameState.escher_room_eight_seen ? 263 : 268;
            break;
        }
        if (tune != -1 && !SndOn(tune + 10000))
            AddSong(tune);
    }
}

void far pascal FinlKidEnterPrincessRoom(void)
{
    PlayNIS(5);
    reset_level_overlays();
    LoadLevel(LevelIdx);
    SetPalette(1000, 240, 16, 0);
    SetKidPalette(6);
    FinlCutKidToChessRoom();
}

typedef struct {
    byte data[2];
    int shape;
    byte tail[16];
} ForegroundImage;

typedef struct {
    int xmin, ymin, xmax, ymax;
} Rect;

typedef struct {
    int dy, dx;
} ShapeOffset;

typedef struct {
    sbyte shape_delta;
    ShapeOffset shape_offset;
    byte layer;
    byte attributes[5];
    Rect bounds;
    byte tail[6];
} FinalImage;

typedef struct {
    byte room, tile;
    int first_shape;
    byte attributes[24];
    FinalImage images[1];
} RoomImageTable;

extern RoomImageTable near *CustomRoom;
extern ForegroundImage far ForeTable[];
extern int Ct[5];
extern void far cdecl _fmemmove(void far *dst, const void far *src, unsigned int size);

void far pascal FinlSortForeTable(void)
{
    int out, index;
    ForegroundImage saved;

    index = Ct[1] - 1;
    out = index;
    for (; index >= 0; --index) {
        int shape = CustomRoom->images[ForeTable[index].shape - CustomRoom->first_shape].shape_delta
                  + CustomRoom->first_shape;
        if ((shape >= 0x1021 && shape <= 0x1024) || (shape >= 0x1058 && shape <= 0x105c)) {
            if (index != out) {
                saved = ForeTable[index];
                _fmemmove(&ForeTable[index], &ForeTable[index + 1], (out - index) * sizeof(ForegroundImage));
                ForeTable[out] = saved;
            }
            --out;
        }
    }
}

extern int CDRightEdge;
extern byte screen_tile;
extern void far pascal LoadKid(void);
extern void far pascal LoadShad(sbyte slot);
extern sbyte far pascal GetKidIdxAsOpp(void);
extern sbyte far pascal RegisterCharInRoom(void);
extern void far pascal DecLifePts(sbyte frames);
extern void far pascal ChangeMeters(void);
extern void far pascal CheckMeters(void);

typedef struct {
    byte data[6];
    byte flags;
} AnimationFrame;

typedef AnimationFrame AnimationFrameTable[];
extern AnimationFrameTable far KidFrames;

void far pascal FinlWatchKid(void)
{
    LoadKid();
    if (Char.room == 2) {
        if (CDRightEdge <= 305 && Char.action != 4)
            FinlKidEnterPrincessRoom();
    } else if (Char.character_kind == 1 && Char.sequence != 71) {
        sbyte room;

        LoadShad(GetKidIdxAsOpp());
        room = RegisterCharInRoom();
        if ((room == 7 || room == 8) && Level.environment_kind == 6 &&
            Kid.room == 5 && Level.environment_kind == 6 && Kid.column <= 4 &&
            (KidFrames[(int)Kid.frame].flags & 0x40)) {
            LoadKid();
            DecLifePts(100);
            JumpSeq(71);
            Kid = Char;
            ChangeMeters();
            CheckMeters();
        }
        LoadKid();
    }
    Kid = Char;
}

void far pascal FinlWatchOpps(void)
{
    sbyte i;
    sbyte count = Level.room_objects[Room.number - 1].opponent_count;

    if (count) {
        i = 0;
        if (count > 0)
            do {
                LoadShad(i);
                SaveShad();
            } while (count > ++i);
    }
    if (Room.number == 3 && Level.environment_kind == 6)
        FinlCheckGenOpp(count);
}

void far pascal JaffarInCrystalRoomProg(void);
void far pascal JaffarInEscherRoomProg(void);

void far pascal JaffarProg(void)
{
    if (Char.room == 6 && Level.environment_kind == 6) {
        JaffarInCrystalRoomProg();
        return;
    }
    JaffarInEscherRoomProg();
}

void far pascal AllOtherJaffarsShimmerAway(void)
{
    CHARREC saved = Char;
    sbyte i = 0;
    sbyte count = Level.room_objects[(byte)Opp.room - 1].opponent_count;

    if (count > 0)
        do {
            LoadShad(i);
            if (Char.character_kind == 6 && Char.slot != Opp.slot && Char.frame == 15) {
                JumpSeq(240);
                SaveShad();
            }
        } while (count > ++i);
    Char = saved;
}

void far pascal DoJaffCastSpell(void)
{
    int sequence = -1;

    if (Char.frame == 15) {
        if ((Char.x > Opp.x && Char.direction != -1) ||
            (Char.x <= Opp.x && Char.direction != 0))
            sequence = 5;
        else
            sequence = 242;
    } else if (((Char.frame >= 1 && Char.frame <= 14) ||
                (Char.frame >= 49 && Char.frame <= 56)) &&
               Char.sequence != 13 && Char.sequence != 5) {
        sequence = 13;
    }
    if (sequence != -1)
        JumpSeq(sequence);
}

extern sbyte shadow_row;

void far pascal JaffarSetToNextPos(char near *p);
void far pascal JaffarSetToPrevPos(char near *record);
void far pascal JaffarGoToPos(sbyte near *stance);
int far pascal ShadowInRange(sbyte position, sbyte direction);
void far pascal JaffHitKidWithCast(void);
int far pascal JaffClearShotAtKid(void);

void far pascal JaffarAttackInEscherRooms(void)
{
    int move = 1;
    sbyte near *stance;
    OpponentSlot near *raw;

    raw = GetOppInfoIdx((byte)Char.room, Char.slot);
    stance = raw->stance;
    if (raw->stance[0] != 2)
        stance[0] = 2;
    if (Char.sequence == 242) {
        if (Char.frame == 281)
            AddSound(62);
        else if (Char.frame == 282 && JaffClearShotAtKid())
            JaffHitKidWithCast();
        move = 0;
    } else if (stance[1] == stance[2]) {
        if (Char.row == Opp.row && JaffClearShotAtKid()) {
            DoJaffCastSpell();
        } else if (Opp.sequence != 241) {
            if (ShadowInRange(stance[1], -1))
                JaffarSetToNextPos((char near *)stance);
            else
                JaffarSetToPrevPos((char near *)stance);
        }
    }
    if (move && stance[1] != stance[2] && Opp.life_points > 0) {
        if (Char.row == 1 && JaffClearShotAtKid()) {
            DoJaffCastSpell();
            return;
        }
        JaffarGoToPos(stance);
    }
}

void far pascal JaffarFleeInEscherRooms(void)
{
    sbyte near *stance;
    OpponentSlot near *raw;

    raw = GetOppInfoIdx((byte)Char.room, Char.slot);
    stance = raw->stance;
    if (raw->stance[0] != 1)
        stance[0] = 1;
    if (stance[1] == stance[2]) {
        if (ShadowInRange(stance[1], -1))
            JaffarSetToPrevPos((char near *)stance);
        else if (ShadowInRange(stance[1], 1))
            JaffarSetToNextPos((char near *)stance);
    }
    if (stance[1] != stance[2])
        JaffarGoToPos(stance);
}

int far pascal JaffClearShotAtKid(void)
{
    int clear = 0;

    if (Opp.life_points > 0 && (Opp.room == 7 || Opp.room == 8) &&
        Level.environment_kind == 6 && Opp.sequence != 241 && Char.row == Opp.row &&
        (KidFrames[(int)Char.frame].flags & 0x40) && Char.frame != 109) {
        sbyte position = Char.column;

        if (Char.room == 8)
            position += 10;
        if (Char.row == 0) {
            clear = position <= 6 || position >= 13;
        } else if (Char.row == 1) {
            sbyte shadow = Opp.column;

            if (Opp.room == 8)
                shadow += 10;
            if (position <= 12) {
                if (shadow <= 12)
                    clear = 1;
            } else if (shadow >= 14) {
                clear = 1;
            }
        } else if (Char.row == 2) {
            clear = 1;
        }
    }
    return clear;
}

void far pascal JaffarSetToNextPos(char near *p)
{
    if (p[1] == 3)
        p[1] = 0;
    else
        ++p[1];
}

void far pascal JaffarSetToPrevPos(char near *record)
{
    if (record[1] != 0) {
        --record[1];
        return;
    }

    record[1] = 3;
}

typedef struct {
    sbyte room_x;
    byte flags;
    sbyte direction;
    int min_x, max_x;
    sbyte shadow_column[2];
    sbyte shadow_min[2];
    sbyte shadow_max[2];
} PositionRule;

static PositionRule position_rules[4] = {
    {17, 0, -1, 388, 400, {1, 0}, {15, 1}, {18, 18}},
    {14, 2, -1, 280, 304, {2, 1}, {0, 14}, {18, 18}},
    { 5, 2,  0, 312, 336, {1, 2}, {2, 0}, {9, 18}},
    { 6, 0,  0, 360, 368, {0, 1}, {1, 6}, {18, 10}}
};
extern signed char Plus1[2];
static int jaffar_motion_disabled = 0;
static byte blink_frame_order[6] = {0, 1, 2, 2, 1, 0};
extern void far pascal DoStepFwd(int delta);
extern int far pascal JaffarGoToPos0(sbyte near *stance);
extern int far pascal JaffarGoToPos1(sbyte near *stance);
extern int far pascal JaffarGoToPos2(sbyte near *stance);
extern int far pascal JaffarGoToPos3(sbyte near *stance);

void far pascal JaffarGoToPos(sbyte near *stance)
{
    if (!jaffar_motion_disabled) {
        int move;
        switch (stance[1]) {
        case 0:
            move = JaffarGoToPos0(stance);
            break;
        case 1:
            move = JaffarGoToPos1(stance);
            break;
        case 2:
            move = JaffarGoToPos2(stance);
            break;
        case 3:
            move = JaffarGoToPos3(stance);
            break;
        }
        if (move != -1)
            JumpSeq(move);
    }
}

int far pascal JaffarHoneInOnExactPos(sbyte near *stance)
{
    int sequence = -1;

    if (Char.frame == 15) {
        if (stance[0] == 2 && JaffClearShotAtKid()) {
            DoJaffCastSpell();
        } else {
            const PositionRule near *rule = &position_rules[stance[1]];
            int x = Char.x;

            if (rule->direction != Char.direction)
                x += Plus1[rule->direction + 1] * 6;
            if (rule->room_x < 10) {
                if (Char.room == 8)
                    x += 320;
            } else if (Char.room == 7) {
                x -= 320;
            }
            if (x < rule->min_x) {
                if (Char.direction == 0)
                    DoStepFwd(rule->min_x - x + 2);
                else
                    sequence = 5;
            } else if (x > rule->max_x) {
                if (Char.direction == -1)
                    DoStepFwd(x - rule->min_x + 2);
                else
                    sequence = 5;
            } else if (rule->direction != Char.direction) {
                sequence = 5;
            } else {
                stance[2] = stance[1];
            }
        }
    }
    return sequence;
}

extern void far pascal DoStandJump(void);
extern void far pascal DoJumpUp(void);
extern void far pascal DoStartRun(void);
extern void far pascal ClimbUp(void);
extern void far pascal Down(void);
extern void far pascal Drop(void);
extern void far pascal DoRunJump(int mode);

int far pascal JaffarGoToPos0(sbyte near *stance)
{
    sbyte x = Char.column + ((Char.room == 8) ? 1 : 0) * 10;
    int sequence = -1;
    const PositionRule near *rule = &position_rules[stance[1]];

    if (stance[2] == 3) {
        if (Char.frame == 15) {
            if (x == 6)
                DoStandJump();
            else if (x > 7 && x < rule->room_x - 1)
                DoStartRun();
            else
                sequence = JaffarHoneInOnExactPos(stance);
        } else if (Char.frame == 7 || Char.frame == 11) {
            if ((x >= 5 && x <= 6) || (x >= 10 && x <= 11)) DoRunJump(4);
            else if (x >= rule->room_x - 1) sequence = 13;
        }
    } else if (Char.row == 2) {
        if (Char.frame == 15) {
            if (Char.direction != 0) goto turn;
            DoJumpUp();
        } else if (Char.frame == 91)
            ClimbUp();
    } else if (Char.row == 1) {
        if (Char.frame == 15) {
            if (Char.direction == 0) {
                if (x < 17) DoStartRun();
                else
                    goto turn;
            } else if (x >= 17)
                DoJumpUp();
            else {
turn:
                sequence = 5;
            }
        } else if (Char.frame == 7 && x >= 17)
            sequence = 13;
        else if (Char.frame == 91) ClimbUp();
        else if (Char.frame == 109) sequence = 49;
    } else {
settle:
        sequence = JaffarHoneInOnExactPos(stance);
    }
    return sequence;
}

int far pascal JaffarGoToPos1(sbyte near *stance)
{
    int sequence = -1; const PositionRule near *rule = &position_rules[stance[1]];

    if (stance[2] < stance[1]) {
        if (Char.row == 0) {
            if (Char.direction == 0 && Char.sequence != 5)
                sequence = 5;
            else if (Char.frame == 15)
                Down();
        } else if (Char.row == 1) {
            if (Char.frame >= 87 && Char.frame <= 91) {
                Drop();
            } else if (Char.frame == 15) {
                if (Char.direction != -1)
                    sequence = 5;
                else
                    DoStartRun();
            } else if (Char.frame == 109) {
                sequence = 49;
            }
        } else if (Char.row == 2) {
            if (Char.frame != 109)
                sequence = JaffarHoneInOnExactPos(stance);
            else
                sequence = 49;
        }
    } else {
        sbyte x = Char.column + ((Char.room == 8) ? 1 : 0) * 10;

        if (x == position_rules[2].room_x && Char.frame == 15)
            DoStartRun();
        else if (x >= 6 && x <= 7) {
            DoRunJump(4);
        } else if (x > 10) {
            if (Char.frame == 7 && x >= rule->room_x - 1)
                sequence = 13;
            else
                sequence = JaffarHoneInOnExactPos(stance);
        }
    }
    return sequence;
}

extern int far pascal AddCharX(int x);

int far pascal JaffarGoToPos2(sbyte near *stance)
{
    int sequence = -1;

    if (stance[2] < stance[1]) {
        sbyte x = Char.column + ((Char.room == 8) ? 1 : 0) * 10;

        if (x == position_rules[1].room_x && Char.frame == 15) {
            DoStartRun();
        } else if (x >= 11 && ColEdgeX[7] + 36 > Char.x &&
                   Char.sequence != 4 && (Char.frame == 7 || Char.frame == 11)) {
            Char.x = AddCharX(2);
            DoRunJump(4);
        } else if (x <= 6 && Char.frame == 7) {
            sequence = 13;
        } else {
            sequence = JaffarHoneInOnExactPos(stance);
        }
    } else if (Char.row == 0) {
        if (Char.frame == 15) {
            if (Char.direction == 0) sequence = 5;
            else
                Down();
        }
    } else if (Char.row == 1) {
        if (Char.frame >= 87 && Char.frame <= 91) {
            Drop();
        } else if (Char.frame == 15) {
            if (Char.direction == -1) DoStartRun();
            else
                sequence = 5;
        } else if (Char.frame == 109) {
            sequence = 49;
        }
    } else if (Char.row == 2) {
        if (Char.frame == 109) sequence = 49;
        else {
            sequence = JaffarHoneInOnExactPos(stance);
        }
    }
    return sequence;
}

int far pascal JaffarGoToPos3(sbyte near *stance)
{
    int sequence = -1;
    const PositionRule near *rule = &position_rules[stance[1]];
    sbyte x = Char.column + ((Char.room == 8) ? 1 : 0) * 10;

    if (stance[2] == 0) {
        if (Char.frame == 15) {
            if (x > 10) DoStartRun();
            else if (x <= rule->room_x)
                sequence = JaffarHoneInOnExactPos(stance);
        } else if (Char.frame == 7 || Char.frame == 11) {
            if (x <= rule->room_x) sequence = 13;
            else if ((x >= 14 && x <= 15) || (x >= 9 && x <= 10))
                DoRunJump(4);
        }
    } else if (Char.row == 2) {
        if (Char.frame == 15) {
            DoJumpUp();
        } else if (Char.frame == 91) {
            ClimbUp();
        }
    } else if (Char.row == 1) {
        if (Char.frame == 15) {
            if (Char.direction != 0) DoJumpUp();
            else if (x == 6) DoStepFwd(32);
            else
                sequence = 5;
        } else {
            if (Char.frame == 91) ClimbUp();
            else if (Char.frame == 109) sequence = 49;
        }
    } else {
        sequence = JaffarHoneInOnExactPos(stance);
    }
    return sequence;
}

extern void far pascal LoadShadwOpp(sbyte slot);
extern void far pascal SetKidDeathMethod(sbyte script);

void far pascal JaffHitKidWithCast(void)
{
    sbyte slot;

    JumpSeq(244);
    slot = Char.slot;
    SaveShad();
    LoadKid();
    JumpSeq(71);
    AddSound(14);
    DecLifePts(100);
    SetKidDeathMethod(17);
    Kid = Char;
    LoadShadwOpp(slot);
}

extern struct {
    signed char argument, entrance_mode, sword_kind, death_countdown;
    int field_4, shadow_timer;
} KidInfo;

extern int far pascal GetOppDist(void);
extern sbyte far pascal IndexCell(sbyte column, sbyte row);
extern int abs(int x);
#pragma intrinsic(abs)

void far pascal JaffarInCrystalRoomProg(void)
{
    if (Char.alive < 0) {
        sbyte count = Level.room_objects[(byte)Char.room - 1].opponent_count;

        if (Opp.character_kind == 1)
            --count;
        if (count == 1 && Char.sequence != 240 && Kid.alive < 0) {
            JumpSeq(240);
        } else if (Char.frame == 15) {
            if (Kid.alive < 0 && Kid.sequence != 44 && Kid.sequence != 6 &&
                Kid.character_kind == 0 && (KidFrames[(int)Kid.frame].flags & 0x40) &&
                Kid.action != 3 && Kid.action != 4) {
                int attack = 0;
                int range = GetOppDist();

                if (Char.direction == Opp.direction) {
                    range -= 15;
                    if (range > 18 && range < 48)
                        attack = 1;
                } else if (Char.direction != Opp.direction) {
                    if (range < 0) {
                        range = -15 - range;
                        if (range > 18 && range < 48) {
                            Char.direction = ~Char.direction;
                            attack = 1;
                        }
                    } else if (Kid.frame == 145) {
                        range -= 15;
                        if (range > 18 && range < 52)
                            attack = 1;
                    }
                }
                if (attack) {
                    Char.x = AddCharX(15);
                    Char.stance = 1;
                    JumpSeq(75);
                }
            }
        } else if (Char.frame == 171) {
            Char.stance = 0;
            JumpSeq(92);
        } else if (Char.frame == 52 && Kid.alive > 0) {
            JumpSeq(197);
            AddSound(262);
        } else if (Char.frame == 109) {
            JumpSeq(49);
        }
    }
}

void far pascal JaffarInEscherRoomProg(void)
{
    int flee;
    int attack;
    sbyte slot;
    OpponentSlot near *opponent;
    sbyte near *stance;

    opponent = GetOppInfoIdx((byte)Char.room, Char.slot);
    stance = opponent->stance;
    if (opponent->stance[0] == 1 && stance[2] != stance[1])
        flee = 1;
    else
        flee = 0;

    attack = 0;
    if (Opp.character_kind == 0) {
        if (Opp.stance == 1 &&
            !(Char.frame >= 207 && Char.frame <= 210) &&
            Char.frame != 158) {
            if (abs(GetOppDist()) < 62) {
                slot = Char.slot;
                SaveShad();
                LoadKid();
                JumpSeq(241);
                AddSong(265);
                Char.stance = -1;
                Kid = Char;
                LoadShad(slot);
            }
        } else if (Char.row == Opp.row && KidInfo.shadow_timer >= 4) {
            if (Opp.life_points <= 2 && Opp.sequence != 242)
                flee = 0;
            else
                flee = 1;
        } else {
            attack = Opp.stance == -1 ? 1 : 0;
        }
    } else {
        if (Opp.life_points <= 2 && Opp.sequence != 242)
            flee = 0;
        else
            flee = 1;
        if (Opp.life_points <= 2 && Opp.sequence != 242)
            attack = 1;
        else
            attack = 0;
    }

    if (Char.sequence == 243 && Char.frame == 351)
        ++NextLevel;
    if (flee) {
        JaffarFleeInEscherRooms();
        return;
    }
    if (attack)
        JaffarAttackInEscherRooms();
}

void far pascal JaffarUpdateOppInfo(OpponentSlot near *opponent)
{
    sbyte near *stance = opponent->stance;

    if (!((CutRoom == 7 || CutRoom == 8) && Level.environment_kind == 6) &&
        stance[2] != stance[1]) {
        const PositionRule near *rule;

        stance[2] = stance[1];
        rule = &position_rules[stance[1]];
        Char.row = rule->flags;
        Char.column = rule->room_x;
        Char.x = rule->min_x + 6;
        if (Char.room == 8) {
            Char.x -= 320;
            Char.column -= 10;
        }
        Char.direction = rule->direction;
        ResetCharY();
        JumpSeq(2);
        AnimChar();
        opponent->cell = IndexCell(Char.column, Char.row);
        opponent->x = Char.x;
        opponent->direction = Char.direction;
        *(int near *)((byte near *)opponent + 7) = Char.sequence_position;
        *(int near *)((byte near *)opponent + 5) = Char.sequence_id;
    }
    *(int near *)(stance + 4) = Char.y;
}

int far pascal ShadowInRange(sbyte position, sbyte direction)
{
    const PositionRule *rule;
    int inside;
    sbyte x;

    if (direction < 0)
        direction = 0;
    else
        direction = 1;
    rule = &position_rules[position];
    if (rule->shadow_column[direction] == Opp.row &&
        (x = Opp.column + (Opp.room == 8 ? 1 : 0) * 10) >= rule->shadow_min[direction] &&
        x <= rule->shadow_max[direction])
        inside = 1;
    else
        inside = 0;
    return inside;
}

extern unsigned int BPort;
extern unsigned int Graf;
extern byte GamePlayRect;
extern void far pascal UpdateOppInfo(void);
extern void far pascal FillRect(void far *work, int resource);
extern void far pascal EraseRect(void far *work);

void far pascal TransferJaffarToEscher2Room(void)
{
    Char.column = 17;
    Char.x = 712;
    Char.row = -3;
    Char.y = -133;
    Char.direction = position_rules[0].direction;
    GetOppInfoIdx((byte)Char.room, Char.slot)->location.transition_state = 0;
    JumpSeq(2);
    AnimChar();
    UpdateOppInfo();
}

void far pascal LoadTowerTop(void)
{
    if (!GameState.playing_nis)
        SetPalette(3500, 64, 192, 0);
    if (BPort)
        Graf = BPort;
    FillRect(&GamePlayRect, 139);
}

void far pascal LoadChess(void)
{
    SetPalette(3500, 64, 192, 1);
    SetPalette(1000, 240, 16, 0);
    if (Room.number == 4 && Level.environment_kind == 6) {
        unsigned int previous = Graf;

        Graf = BPort;
        EraseRect(&GamePlayRect);
        Graf = previous;
        AddSong(268);
    } else if (Room.number == 3 && Level.environment_kind == 6) {
        SetPalette(3000, 224, 16, 0);
        AddSong(269);
    }
}

typedef struct {
    int shape;
    sbyte x_offset, y_offset;
} SmokeFrame;

typedef struct {
    int image, equipment_image;
    sbyte x, y;
    byte flags;
} FRAMEREC;

extern FRAMEREC Frame;
extern unsigned int SwordFrames;
extern SmokeFrame far *far pascal h2p(unsigned int handle);
extern int far pascal AddFCharX(int x);
extern void far pascal MakeFCharClipRect(void);
extern void far _fastcall AddCharObj(byte style);
typedef void (far pascal *AnimationCallback)(sbyte cell, void near *clip);
extern void far pascal MarkFRedraw(sbyte cell, void near *clip);
extern void far pascal MarkBufsForRect(AnimationCallback callback, void near *state, sbyte slot);

void far pascal SetupGuardSmoke(void)
{
    SmokeFrame far *table;
    unsigned int index;

    if (Char.character_kind == 2 && Char.frame >= 229 && Char.frame <= 254) {
        index = (unsigned int)Frame.equipment_image;
        table = h2p(SwordFrames);
        if (table[index].shape != -1) {
            FChar.image = table[index].shape;
            AddFCharX(table[index].x_offset);
            FChar.y += table[index].y_offset;
            FChar.image_set = 3;
            MakeFCharClipRect();
            AddCharObj(9);
            MarkBufsForRect(MarkFRedraw, &FChar.clip_top, Char.slot);
        }
    }
}

/* Each frame selects a room image slot. Each shape pair packs two word offsets. */
static byte crystal_frame_slot[] = {
    0, 1, 2, 3, 2, 1, 0, 0
};
static ShapeOffset crystal_shapes[] = {
    {10, 310}, {72, 47}, {84, 138},
    {65, 206}, {84, 235}, {130, 14},
    {136, 52}, {164, 128}, {155, 267}
};
static byte crystal_layer[] = {
    1, 0, 0, 0, 0, 1, 1, 1, 1, 0
};
static byte gargoyle_frame_slot[] = {
    0, 1, 2, 4, 3, 4, 2, 1, 0, 0
};
static ShapeOffset gargoyle_shapes[] = {
    {67, 36}, {67, 63}, {67, 94},
    {67, 123}, {67, 154}, {131, 113},
    {68, -1}, {126, 25}, {131, 214}
};
static byte gargoyle_layer[] = {
    0, 0, 0, 0, 0, 1, 0, 1, 1, 0
};
extern void far pascal OFFSETRECT(Rect far *output, Rect far *input, int dx, int dy);
extern void far _fastcall DrawCustomRoomImg(byte index, byte layer);

extern unsigned int BlueSpec;
extern int NewRoom;
typedef struct {
    signed char tile;
    unsigned char room;
    signed char state;
    unsigned char field_3;
} TrobRecord;
extern TrobRecord Trob;
static byte crystal_serial;
static byte gargoyle_serial;
extern unsigned int (*near BlueS)[2];

extern void far pascal AddTrob(sbyte row, sbyte col, byte value, byte effect);
extern void far pascal MarkMove(sbyte cell, void near *clip);

#define ROOM_RECT(offset) ((Rect near *)((byte near *)CustomRoom + (offset)))

void far pascal AnimFinlJaffarBlink(void)
{
    unsigned int frame;

    ++BlueSpec;
    frame = BlueSpec & 15;
    if (frame >= 5 && CustomRoom->tile == 26) {
        frame -= 5;
        if (frame < 6) {
            MarkBufsForRect(MarkMove, ROOM_RECT(89), -1);
            return;
        }
        Trob.state = (byte)-1;
        BlueSpec = 0;
    }
}

void far pascal DrawFinlJaffarBlink(FinalObject near *object)
{
    unsigned int frame = object->animation & 15;

    if ((NewRoom || frame >= 5) && object->kind == 11) {
        if (!NewRoom)
            frame -= 5;
        if (frame < 6)
            DrawCustomRoomImg(blink_frame_order[frame] + 2, 11);
    }
}

void far pascal AnimFinlSpider(void)
{
    if (CustomRoom->tile == 25) {
        unsigned int state = BlueSpec;
        int ticks = (state & 0x7f8) >> 3;

        if (state & 0x800) {
            state = Rnd(5);
            if (--ticks > 0 && !NewRoom) {
                state |= 0x800;
            } else {
                ticks = Rnd(60) + 30;
            }
            MarkBufsForRect(MarkMove, ROOM_RECT(189), -1);
        } else if (--ticks <= 0) {
            if (!GameState.fade_active) {
                ticks = Rnd(2) + 1;
                state |= 0x800;
            } else {
                ticks = Rnd(60) + 30;
            }
        }
        state &= 0xf807;
        state |= ticks << 3;
        BlueSpec = state;
        return;
    }
    Trob.state = (byte)-1;
}

extern struct { int top, left, bottom, right; } CellClipRect;

void far pascal DrawFinlSpider(FinalObject near *object)
{
    if (object->kind == 11) {
        int previous = CellClipRect.left;

        CellClipRect.left -= 6;
        DrawCustomRoomImg((object->animation & 7) + 6, 11);
        CellClipRect.left = previous;
    }
}

void far pascal LoadCrystal(void)
{
    SetPalette(3500, 64, 192, 2);
    SetPalette(1000, 240, 16, 0);
    SetPalette(25001, 32, 16, 7);
    crystal_serial = 0;
    if (Room.number == 7) {
        SetPalette(3000, 224, 16, gargoyle_serial = 0);
        if (GameState.escher_room_eight_seen)
            AddSong(263);
    } else if (Room.number == 8) {
        gargoyle_serial = 6;
        SetPalette(3000, 224, 16, 0);
        if (!SndOn(263 + 10000) && Level.room_objects[7].opponent_count)
            AddSong(263);
    } else {
        AddSong(270);
    }
}

void far pascal AnimFinlCrystal(void)
{
    if (CustomRoom->tile == 28) {
        unsigned int state = BlueSpec; if (state & 0x800) {
            unsigned int frame = ((state & 0x7f0) >> 4) + 1;

            if (frame < 7) {
                Rect bounds;
                const ShapeOffset near *shape;

                state &= 0xf80f; state |= frame << 4;
                shape = &crystal_shapes[state & 15]; OFFSETRECT(&bounds, &CustomRoom->images[4].bounds, shape->dx, shape->dy);
                MarkBufsForRect(MarkMove, &bounds, -1);
            } else {
                state &= 0xf80f; state |= (Rnd(40) + 40) << 4; state &= ~0x800;
            }
        } else {
            int ticks = (state & 0x7f0) >> 4; state &= 0xf80f; if (--ticks <= 0) {
                if (GameState.fade_active && TrobCt)
                    state |= (Rnd(40) + 40) << 4;
                else
                    state |= 0x800;
            } else {
                state |= ticks << 4;
            }
        }
        BlueSpec = state | 0x1000;
        return;
    }
    Trob.state = (byte)-1;
}

void far pascal DrawFinlCrystal(FinalObject near *object)
{
    int frame;
    byte index, shape;
    FinalImage near *image;
    Rect bounds;

    if (object->kind != 1 || !(object->animation & 0x800))
        return;
    frame = (short)(object->animation & 0x7f0) >> 4;
    if (--frame < 0)
        return;
    index = crystal_frame_slot[frame] + 4;
    image = &CustomRoom->images[index];
    bounds = image->bounds;
    shape = object->animation & 15;
    image->shape_offset = crystal_shapes[shape];
    OFFSETRECT(&image->bounds, &image->bounds, image->shape_offset.dx, image->shape_offset.dy);
    image->layer = crystal_layer[shape];
    DrawCustomRoomImg(index, crystal_layer[shape]);
    image->layer = 11;
    image->bounds = bounds;
    image->shape_offset.dy = 0;
    image->shape_offset.dx = 0;
}

void far pascal TriggerFinlCrystal(sbyte row, sbyte col)
{
    unsigned int serial = crystal_serial++;
    unsigned int ticks = Rnd(40);

    BlueS[col][0] = ((ticks | 0x100) << 4) | serial;
    AddTrob(row, col, 1, 41);
}

void far pascal AnimFinlGargoyle(void)
{
    if (CustomRoom->tile == 29 || CustomRoom->tile == 30) {
        unsigned int state = BlueSpec;
        byte serial = (byte)(state & 15);

        if ((serial < 6 && Room.number == 7) || (serial >= 6 && Room.number == 8)) {
            if (state & 0x800) {
                unsigned int frame = ((state & 0x7f0) >> 4) + 1;

                if (frame < 9) {
                    Rect bounds;
                    const ShapeOffset near *shape;

                    state &= 0xf80f;
                    state |= frame << 4;
                    shape = &gargoyle_shapes[serial];
                    OFFSETRECT(&bounds, &CustomRoom->images[2].bounds, shape->dx, shape->dy);
                    MarkBufsForRect(MarkFRedraw, &bounds, -1);
                } else {
                    state &= 0xf80f;
                    state |= (Rnd(40) + 80) << 4;
                    state &= ~0x800;
                }
            } else {
                int ticks = (state & 0x7f0) >> 4;

                state &= 0xf80f;
                if (--ticks <= 0) {
                    if (GameState.fade_active && TrobCt)
                        state |= (Rnd(40) + 80) << 4;
                    else
                        state |= 0x800;
                } else {
                    state |= ticks << 4;
                }
            }
            BlueSpec = state | 0x1000;
            return;
        }
    }
    Trob.state = (byte)-1;
}

void far pascal DrawFinlGargoyle(FinalObject near *object)
{
    int frame;
    byte index, shape;
    FinalImage near *image;
    Rect bounds;

    if (object->kind != 1 || !(object->animation & 0x800))
        return;
    frame = (short)(object->animation & 0x7f0) >> 4;
    if (--frame < 0)
        return;
    index = gargoyle_frame_slot[frame] + 2;
    image = &CustomRoom->images[index];
    bounds = image->bounds;
    shape = object->animation & 15;
    image->shape_offset = gargoyle_shapes[shape];
    OFFSETRECT(&image->bounds, &image->bounds, image->shape_offset.dx, image->shape_offset.dy);
    image->layer = gargoyle_layer[shape];
    DrawCustomRoomImg(index, gargoyle_layer[shape]);
    image->layer = 11;
    image->bounds = bounds;
    image->shape_offset.dy = 0;
    image->shape_offset.dx = 0;
}

void far pascal TriggerFinlGargoyle(sbyte row, sbyte col)
{
    unsigned int serial = gargoyle_serial++;
    unsigned int ticks = Rnd(40);

    BlueS[col][0] = ((ticks | 0x100) << 4) | serial;
    AddTrob(row, col, 1, 42);
}

extern MOBREC Mob;
extern int MobHeight[13], MobWidth[13];
#define fireball_half_width MobWidth[12]
#define fireball_height MobHeight[12]
extern int CDLeftEdge;
extern unsigned int Pics[5];

extern byte far pascal GetRow(int y);
extern byte far pascal IndexCell1(sbyte col, byte row);
extern void far *far pascal GetSetImgSize(unsigned int sheet, unsigned int image, int far *size);
extern void far pascal AddMobObj(int y);
extern void far pascal MarkMobRedraw(byte changed);
extern int far pascal IsWall(sbyte tile);
extern void far pascal ResetSnd(unsigned int image);
extern void far pascal AddMob(void);
extern void far _fastcall AddMid(byte layer, int frame, int x, int y, byte flags);

typedef struct {
    int x;
    byte unknown0[2];
    byte room;
    byte unknown1[2];
    int velocity;
    byte unknown2;
    sbyte row;
    unsigned int flags;
} FireballSlot;

typedef struct {
    int x, y;
    sbyte type;
    int shape;
    Rect clip;
    byte attributes;
    int mode;
    byte palette, update_peel;
} MiddleEntry;

extern FireballSlot near *far pascal GetMobTypeItem(byte kind, int index);
extern MiddleEntry far MidTable[];
int far _fastcall GetFireballImg(unsigned int state);
void far pascal TriggerFireball(void);

void far pascal ATMFireball(void)
{
    int visible = 1;
    int y = Mob.y;

    if (Room.number == Mob.room) {
        if (Mob.x > 449 || Mob.x < 0)
            visible = 0;
    } else if (Mob.room == Room.left) {
        if (Mob.x + fireball_half_width > 320) {
            Mob.x -= 320;
            Mob.room = Room.number;
        } else {
            visible = 0;
        }
    } else if (Mob.room == Room.right) {
        if (Mob.x - fireball_half_width < 320) {
            Mob.x += 320;
            Mob.room = Room.number;
        } else {
            visible = 0;
        }
    } else {
        visible = 0;
    }

    if (visible) {
        sbyte col;
        unsigned int image;
        int size[2];

        FChar.index = IndexCell1(col = Mob.x / 32, GetRow(y));
        ++col;
        image = GetFireballImg(Mob.flags);
        if (GetSetImgSize(Pics[2], image, size)) {
            int saved_y;
            int x = Mob.x;
            Mob.x -= size[1] / 2;
            saved_y = y;
            Mob.y = (y += size[0] / 2);
            fireball_height = size[0];
            fireball_half_width = size[1];
            AddMobObj(Mob.y);
            MarkMobRedraw(1);
            Mob.y = saved_y;
            Mob.x = x;
        }
    }
}

void far pascal CheckFireballHitChar(void)
{
    if (Char.character_kind == 6 && Char.alive < 0 && Char.sequence != 243) {
        int index = 1;
        FireballSlot near *ball;

        do {
            int offset;

            ball = GetMobTypeItem(12, index);
            if (ball && ball->row == Char.row && !(ball->flags & 8)) {
                if (ball->room == (byte)Char.room)
                    offset = 0;
                else if (Char.room == 8 && Level.environment_kind == 6)
                    offset = -320;
                else
                    offset = 320;
                offset += ball->x + ball->velocity + 113;
                if (offset + 34 > CDLeftEdge && offset < CDRightEdge) {
                    ball->flags = 8;
                    ball->x = ball->velocity > 0 ? CDLeftEdge - 125 : CDRightEdge - 135;
                    DecLifePts(100);
                    JumpSeq(243);
                    AnimChar();
                    AddSound(62);
                    CutRoom = Char.room;
                }
            }
            ++index;
        } while (ball);
    }
}

void far pascal CheckFireballHitWall(void)
{
    int x = Mob.x + fireball_half_width - 14;
    sbyte col;

    if (Mob.auxiliary < 0)
        x -= 2 * fireball_half_width;
    col = x / 32;
    if (x < 0)
        --col;
    if (IsWall(ReadCell(Mob.room, col, Mob.row))) {
        Mob.x = col * 32 + 14;
        if (Mob.auxiliary < 0)
            Mob.x += 32;
        Mob.flags = 8;
        AddSound(259);
        ResetSnd(0x281a);
    }
}

int far pascal DoThrowFireball(void)
{
    int sequence;
    if (Char.life_points > 2) {
        sequence = 242;

        DecLifePts(2);
        AddSong(266);
    } else {
        sequence = -1;
    }
    return sequence;
}

void far pascal DrawObjFireball(FinalObject near *object)
{
    int image = GetFireballImg(FChar.image);

    AddMid(2, image + 1, FChar.x, FChar.y, 10);
    MidTable[Ct[3] - 1].mode = 8;
    MidTable[Ct[3] - 1].palette = 0;
}

int far _fastcall GetFireballImg(unsigned int state)
{
    unsigned int frame = state & 7;
    int number;

    if (state & 8) {
        number = frame;
        number += 308;
    } else {
        number = frame;
        number += 304;
    }
    return number;
}

void far pascal MobFireball(void)
{
    if (Mob.velocity >= 0) {
        unsigned int frame = Mob.flags & 7;

        if (Mob.flags & 8) {
            if (frame >= 5)
                Mob.velocity = -1;
            else
                ++frame;
        } else {
            if (Mob.auxiliary > 0) {
                if (Mob.auxiliary < 16)
                    Mob.auxiliary += 2;
            } else if (Mob.auxiliary > -16) {
                Mob.auxiliary -= 2;
            }
            Mob.x += Mob.auxiliary;
            if (Mob.room == 0) {
                Mob.velocity = -1;
                ResetSnd(0x2812);
            } else {
                CheckFireballHitWall();
                if (!(Mob.flags & 8)) {
                    if (frame >= 3)
                        frame = 0;
                    else
                        ++frame;
                    AddSound(258);
                }
            }
        }
        if (Mob.velocity == -1) {
            if (Kid.life_points <= 2) {
                SetPalette(2000, 48, 16, 0);
                return;
            }
        } else {
            Mob.flags &= ~7;
            Mob.flags |= frame;
        }
    }
}

void far pascal ThrowingFireball(void)
{
    if (Char.frame == 281)
        TriggerFireball();
    AddSound(258);
}

void far pascal TriggerFireball(void)
{
    int y;

    Mob.x = AddCharX(27) - 130;
    Mob.y = y = Char.y - 29;
    Mob.room = (byte)Char.room;
    Mob.velocity = 0;
    Mob.auxiliary = Plus1[Char.direction + 1] * 8;
    Mob.kind = 12;
    Mob.row = GetRow(y);
    Mob.flags = 2;
    CheckFireballHitWall();
    AddMob();
    AddSound(258);
}

extern void far pascal DrawFlask(FinalObject near *);

typedef void (far pascal *LevelTileProc)(FinalObject near *);
LevelTileProc DrawFinlFunc[] = {
    DrawFinlObj, DrawFinlObj, 0, 0, 0,
    0, 0, 0, 0, 0,
    DrawFlask, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    DrawFinlObj, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, DrawFinlJaffarBlink, 0, 0, 0,
    0, 0, 0, 0, 0,
    DrawFinlSpider, DrawFinlCrystal, DrawFinlGargoyle
};
