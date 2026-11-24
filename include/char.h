/* A character (the Kid or an opponent), how it is drawn, and the cell under examination. */
#pragma pack(1)
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
    unsigned char room;                /* 0e */
    signed char repeat;                /* 0f */
    signed char stance;                /* 10 */
    signed char alive;                 /* 11 */
    signed char life_points;           /* 12 */
    signed char life_initial;          /* 13 */
    signed char life_delta;            /* 14 */
    int sequence_position;             /* 15 */
    int sequence_id;                   /* 17 */
    int sequence;                      /* 19 */
    Rect bounds;                       /* 1b */
    signed char alert_delay;           /* 23 */
    int sequence_group;                /* 24 */
    int previous_x, previous_y;        /* 26, 28 */
    unsigned int previous_frame;       /* 2a */
    Rect previous_bounds;              /* 2c */
    int image_bank;                    /* 34 */
    int previous_mode;                 /* 36 */
    signed char behavior;              /* 38 */
    signed char opponent_index;        /* 39 */
    signed char flame_offset;          /* 3a */
    unsigned char unknown_3b[5];       /* 3b */
} CHARREC;

typedef struct {                       /* the character as an object to draw */
    signed char index, flipped;        /* 00, 01 */
    int x, y, image;                   /* 02, 04, 06 */
    unsigned char image_set;           /* 08 */
    Rect clip;                         /* 09 */
    int depth;                         /* 11 */
    unsigned char object_type;         /* 13 */
} FCHARREC;

typedef struct {                       /* one animation frame of a character */
    int image;
    unsigned int equipment_image;
    signed char x, y;
    unsigned char flags;
} FRAMEREC;

typedef struct {
    signed char kind;                  /* 00 */
    unsigned int modifier;             /* 01 */
    signed char index;                 /* 03 */
    unsigned char room;                /* 04 */
    signed char column, row;           /* 05, 06 */
    signed char first_col, last_col;   /* 07, 08 */
    unsigned char top_row, bottom_row; /* 09, 0a */
} CELLREC;

typedef struct {
    signed char argument, entrance_mode, sword_kind, death_countdown;
    int field_4, shadow_timer;
} KIDINFO;

typedef struct {                       /* joystick and keyboard state, forward and back as seen by the character */
    signed char forward, back, up, down, action;
} Directions;
#pragma pack()
