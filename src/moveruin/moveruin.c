/* MOVERUIN.C, DOS overlay 8 code frame 2a4e. */
typedef signed char CellCoord;
typedef unsigned char Byte;

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
extern CHARREC Char;
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
extern CHARREC Kid;
extern int previous_actor_x;
extern int current_actor_x;
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
#pragma pack(1)
typedef struct {
    int x, y;                                         /* 00,02 */
    unsigned char room;                               /* 04 */
    int velocity, auxiliary;                          /* 05,07 */
    signed char kind, row;                             /* 09-0a */
    unsigned int flags;                               /* 0b */
} MOBREC;
#pragma pack()

extern MOBREC Mob;
#pragma pack(1)
typedef struct {
    unsigned char number;                             /* 00 */
    unsigned char left, right, above, below;           /* 01-04 */
    unsigned char above_left, above_right;             /* 05-06 */
    unsigned char below_left, below_right;             /* 07-08 */
    unsigned char custom;                             /* 09 */
} ROOMREC;
#pragma pack()

extern ROOMREC Room;
extern Byte plateau_intro;
extern CellCoord CutRoom;
extern Byte *ruins_start_actor;
extern int ColEdgeX[20];
extern int plateau_room_loaded;
extern int ruins_start_loaded;

extern CellCoord get_col(int);
extern Byte ReadCell(CellCoord, CellCoord, CellCoord);
extern int IsWall(Byte);
extern int is_six_kind(Byte);
extern void add_mob(void);
extern void make_space(CellCoord, CellCoord);
extern void mark_mob_redraw(Byte);
extern void JumpSeq(int);
extern void reset_char_y(void);
extern void AnimChar(void);
extern void open_start_gate(void);
extern Byte ClrAll(void);
extern int load_palette_bank(unsigned int, int, Byte, Byte);
extern void AddSong(int);

void clip_ruins_actor(void)
{
    int clip_lower;
    CellCoord column, left_column;
    int previous_cell_remaining, current_cell_offset;

    clip_lower = Char.character_kind == 7 || Char.character_kind == 8;
    column = get_col(current_actor_x);
    if (IsWall(ReadCell(Char.room, column, Char.row))) {
        left_column = ReadCell(Char.room, column, ((CellCoord)Cell.top_row));
        if (IsWall(left_column) && left_column != 7 && Char.room == ((CellCoord)Cell.room))
            FChar.clip_right = Cell.column * 32;
    }
    if (clip_lower && (int)Char.sequence != 137 && (int)Char.sequence != 138 &&
        (int)Char.sequence != 143 && (int)Char.sequence > 144 &&
        (int)Char.sequence != 143 && (int)Char.sequence != 144) {
        left_column = get_col(previous_actor_x);
        if (get_col(current_actor_x) != left_column) {
            previous_cell_remaining = 32 - (previous_actor_x - 130) % 32;
            current_cell_offset = (current_actor_x - 32) % 32;
            if (previous_cell_remaining > current_cell_offset)
                --column;
        }
        if (!is_six_kind(ReadCell(Char.room, column, ((CellCoord)Cell.top_row))))
            FChar.clip_top = 63 * ((CellCoord)Cell.bottom_row) + 3;
    }
}

void fall_spikes(void)
{
    int saved_x;

    Mob.x = (Cell.index % 10) * 32;
    Mob.y = 63 * Cell.row + 66;
    Mob.room = ((CellCoord)Cell.room);
    Mob.velocity = Mob.auxiliary = 0;
    (*(volatile Byte *)&Mob.kind) = 3;
    (*(volatile CellCoord *)&Mob.row) = Cell.row;
    (*(volatile int *)&Mob.flags) = 0;
    add_mob();
    make_space(((CellCoord)Cell.room), Cell.index);
    saved_x = Mob.x;
    if (Room.left == Mob.room)
        Mob.x -= 320;
    mark_mob_redraw(0);
    Mob.x = saved_x;
}

void cut_kid_inside_level6(void)
{
    CellCoord start_room;

    Char.direction = 0;
    JumpSeq(5);
    start_room = 3;
    Char.room = start_room;
    CutRoom = start_room;
    Char.column = 4;
    Char.x = ColEdgeX[9] + 14;
    Char.row = 2;
    reset_char_y();
    AnimChar();
    Kid = Char;
    open_start_gate();
    ClrAll();
    plateau_intro = 0;
}


void load_ruins_start_palette(void)
{
    load_palette_bank(3500, 64, 160, 1);
}

void unload_ruins_start_palette(void)
{
    if (Kid.alive < 0 && ruins_start_loaded)
        load_palette_bank(3500, 64, 160, 0);
}

void LoadRuinsMoveCode(void)
{
}

void load_plateau_room(void)
{
    if (Kid.column >= 9) {
        if (Room.number == 16 && ((Byte)Level.level_number) == 9) {
            AddSong(92);
            return;
        }
        if (Room.number == 2 && ((Byte)Level.level_number) == 9 && !plateau_room_loaded) {
            AddSong(91);
            plateau_room_loaded = 1;
        }
    }
}
