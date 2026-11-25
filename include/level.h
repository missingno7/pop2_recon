/* One level as stored on disk and kept in memory, and the room being shown. */
#pragma pack(1)
typedef struct {
    word modifier;                      /* state; low byte of doors and plates: link */
    word background;                    /* 001f: image, 0f00: piece */
} LEVELCELL;

typedef struct {                        /* what a pressure plate moves; a chain by prev and next (0xfd: none) */
    byte room;
    sbyte cell;
    byte timer, prev, next;
} LEVELLINK;

typedef struct {
    byte left, right, above, below;
} ROOMLINKS;

typedef struct {                        /* what a flying head is doing; of a snake only the mode */
    sbyte mode, column, row;
    union {
        struct {
            sbyte direction;
            byte flags;
        } attack;
        int flee_timer;
    } state;
    byte sound_played;
} HEADPLAN;

typedef struct {                        /* a guard, skeleton, head, snake, ... placed in a room */
    sbyte cell;                         /* 00 */
    int x;                              /* 01 */
    sbyte direction;                    /* 03 */
    byte skill;                         /* 04 */
    int action;                         /* 05 */
    int saved_sequence;                 /* 07 */
    byte palette_kind;                  /* 09 */
    sbyte index;                        /* 0a */
    sbyte combat_mode;                  /* 0b */
    sbyte life;                         /* 0c */
    sbyte behavior;                     /* 0d */
    sbyte behavior_type;                /* 0e */
    sbyte kind;                         /* 0f */
    byte max_life;                      /* 10 */
    union {                             /* 11 */
        HEADPLAN head;
        struct {
            int shake_delay, timer;
            sbyte row;                  /* the other row it may come from */
            byte shakes;
        } skeleton;
        struct {
            int y, unknown_13, entry_y;
        } place;
        struct {
            sbyte stance[4];
            int transition;
        } jaffar;
    } u;
} OPPONENT;

typedef struct {
    sbyte opponent_count;
    OPPONENT opponents[5];
} ROOMOBJECTS;

typedef struct {
    byte room;
    sbyte cell;
} REBIRTHPOINT;

typedef struct {                        /* where new opponents come from */
    byte kind;
    byte limit;
    sbyte row;
    sbyte column;
    byte delay;
    byte reset_delay;
    byte behavior;
    sbyte alternate_row;
    byte enabled;
    byte flags;
} GENERATOR;

typedef struct {
    sbyte count;
    byte spawn_flags;
    byte header[2];
    GENERATOR points[3];
} ROOMGENERATORS;

typedef struct {
    sbyte cell_kinds[32][30];                    /* 0000 */
    LEVELCELL cell_info[32][30];                       /* 03c0 */
    LEVELLINK links[256];                              /* 12c0 */
    ROOMLINKS room_links[32];                          /* 17c0 */
    byte room_count;                          /* 1840 */
    byte unknown_1841[4];
    byte environment_kind;                    /* 1845 */
    byte unknown_1846;
    sbyte level_number;                         /* 1847 */
    int background_overlays[8];                        /* 1848 */
    int sword_overlays[3];                             /* 1858 */
    int flask_flag;                                    /* 185e */
    byte entry_room;
    sbyte entry_cell, entry_facing;              /* 1860 */
    byte unknown_1863[3];
    sbyte preferred_opponent;                 /* 1866 */
    ROOMOBJECTS room_objects[32];                      /* 1867 */
    REBIRTHPOINT rebirth_points[2];                    /* 26e7 */
    byte unknown_26eb[0x0e];
    ROOMGENERATORS generators[32];                     /* 26f9 */
    sbyte ambient_cues[32][30];                  /* 2b39-2ef8 */
} LEVELREC;

typedef struct {
    byte number;                              /* 00 */
    byte left, right, above, below;           /* 01 */
    byte above_left, above_right;             /* 05 */
    byte below_left, below_right;             /* 07 */
    byte custom;                              /* 09 */
} ROOMREC;
#pragma pack()
