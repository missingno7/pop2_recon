/* A character (the Kid or an opponent), how it is drawn, and the cell under examination. */
#pragma pack(1)
typedef struct {
    sbyte slot;                  /* 00 */
    sbyte direction;             /* 01 */
    int x, y;                          /* 02, 04 */
    sbyte character_kind;        /* 06 */
    word frame;                /* 07 */
    sbyte column, row;           /* 09, 0a */
    sbyte action;                /* 0b */
    sbyte horizontal_step;       /* 0c */
    sbyte vertical_step;         /* 0d */
    byte room;                /* 0e */
    sbyte repeat;                /* 0f */
    sbyte stance;                /* 10 */
    sbyte alive;                 /* 11 */
    sbyte life_points;           /* 12 */
    sbyte life_initial;          /* 13 */
    sbyte life_delta;            /* 14 */
    int sequence_position;             /* 15 */
    int sequence_id;                   /* 17 */
    int sequence;                      /* 19 */
    Rect bounds;                       /* 1b */
    sbyte alert_delay;           /* 23 */
    int sequence_group;                /* 24 */
    int previous_x, previous_y;        /* 26, 28 */
    word previous_frame;       /* 2a */
    Rect previous_bounds;              /* 2c */
    int image_bank;                    /* 34 */
    int previous_mode;                 /* 36 */
    sbyte behavior;              /* 38 */
    sbyte opponent_index;        /* 39 */
    sbyte flame_offset;          /* 3a */
    byte unknown_3b[5];       /* 3b */
} CHARREC;

typedef struct {                       /* the character as an object to draw */
    sbyte cell, direction;        /* 00, 01 */
    int x, y, image;                   /* 02, 04, 06 */
    byte image_set;           /* 08 */
    Rect clip;                         /* 09 */
    int variant;                         /* 11 */
    byte owner;         /* 13 */
} FCHARREC;

typedef struct {                       /* what a frame adds to a character: sword, flame, smoke */
    int image;
    sbyte x, y;
} ATTACHFRAME;

typedef struct {                       /* one animation frame of a character */
    int image;
    word equipment_image;
    sbyte x, y;
    byte flags;
} FRAMEREC;

typedef struct {
    sbyte kind;                  /* 00 */
    word modifier;             /* 01 */
    sbyte index;                 /* 03 */
    byte room;                /* 04 */
    sbyte column, row;           /* 05, 06 */
    sbyte first_col, last_col;   /* 07, 08 */
    byte top_row, bottom_row; /* 09, 0a */
} CELLREC;

typedef struct {
    sbyte argument, entrance_mode, sword_kind, death_countdown;
    int field_4, shadow_timer;
} KIDINFO;

typedef struct {                       /* joystick and keyboard state, forward and back as seen by the character */
    sbyte forward, back, up, down, action;
} Directions;
#pragma pack()
