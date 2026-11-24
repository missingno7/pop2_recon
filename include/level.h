/* One level as stored on disk and kept in memory, and the room being shown. */
#pragma pack(1)
typedef struct {
    unsigned int modifier, unknown_02;
} LEVELCELL;

typedef struct {                        /* what a pressure plate moves; a chain by prev and next (0xfd: none) */
    unsigned char room;
    signed char cell;
    unsigned char timer, prev, next;
} LEVELLINK;

typedef struct {
    unsigned char left, right, above, below;
} ROOMLINKS;

typedef struct {                        /* a guard, skeleton, head, snake, ... placed in a room */
    signed char cell;                   /* 00 */
    int x;                              /* 01 */
    signed char direction;              /* 03 */
    unsigned char skill;                /* 04 */
    int action;                         /* 05 */
    int saved_sequence;                 /* 07 */
    unsigned char palette_kind;         /* 09 */
    unsigned char index;                /* 0a */
    unsigned char combat_mode;          /* 0b */
    signed char life;                   /* 0c */
    unsigned char behavior;             /* 0d */
    unsigned char behavior_type;        /* 0e */
    signed char kind;                   /* 0f */
    unsigned char max_life;             /* 10 */
    unsigned char state[6];             /* 11: by kind */
} OPPONENT;

typedef struct {
    signed char opponent_count;
    unsigned char opponent_data[5][23];
} ROOMOBJECTS;

typedef struct {
    unsigned char room;
    signed char cell;
} REBIRTHPOINT;

typedef struct {                        /* where new opponents come from */
    unsigned char kind;
    unsigned char limit;
    signed char column;
    signed char row;
    unsigned char delay;
    unsigned char reset_delay;
    unsigned char behavior;
    signed char alternate_column;
    unsigned char enabled;
    unsigned char flags;
} GENERATOR;

typedef struct {
    signed char count;
    unsigned char spawn_flags;
    unsigned char header[2];
    GENERATOR points[3];
} ROOMGENERATORS;

typedef struct {
    signed char cell_kinds[32][30];                    /* 0000 */
    LEVELCELL cell_info[32][30];                       /* 03c0 */
    LEVELLINK links[256];                              /* 12c0 */
    ROOMLINKS room_links[32];                          /* 17c0 */
    unsigned char room_count;                          /* 1840 */
    unsigned char unknown_1841[4];
    unsigned char environment_kind;                    /* 1845 */
    unsigned char unknown_1846;
    signed char level_number;                         /* 1847 */
    int background_overlays[8];                        /* 1848 */
    int sword_overlays[3];                             /* 1858 */
    int flask_flag;                                    /* 185e */
    unsigned char entry_room;
    signed char entry_cell, entry_facing;              /* 1860 */
    unsigned char unknown_1863[3];
    unsigned char preferred_opponent;                  /* 1866 */
    ROOMOBJECTS room_objects[32];                      /* 1867 */
    REBIRTHPOINT rebirth_points[2];                    /* 26e7 */
    unsigned char unknown_26eb[0x0e];
    ROOMGENERATORS generators[32];                     /* 26f9 */
    signed char ambient_cues[32][30];                  /* 2b39-2ef8 */
} LEVELREC;

typedef struct {
    unsigned char number;                              /* 00 */
    unsigned char left, right, above, below;           /* 01 */
    unsigned char above_left, above_right;             /* 05 */
    unsigned char below_left, below_right;             /* 07 */
    unsigned char custom;                              /* 09 */
} ROOMREC;
#pragma pack()
