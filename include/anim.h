/* Animations: a table of moving images and a script of events, played into a port of their own. */
typedef union {
    word word;
    struct {
        word ink : 4;
        word move_rate : 4;
        word frame_rate : 4;
        word playback : 4;
    } bits;
} ELEMENTFLAGS;

typedef struct {
    int shape, first, last;
    ELEMENTFLAGS flags;
    sbyte move_ticks, frame_ticks;
    long x, y;
    long dx, dy;
    long ddx, ddy;
} ELEMENT;

typedef struct ANIMATION {
    PORT port;                          /* 00 */
    PORT *destination;                  /* 2c */
    SAVEDRECT background;               /* 2e */
    Rect dirty;                         /* 46 */
    const byte far *commands;           /* 4e */
    union {
        int repeats;
        byte wait_level;
        unsigned long deadline;
    } timing;                           /* 52 */
    int skip_draw;                      /* 56 */
    word frame_index, delay;            /* 58 */
    unsigned long deadline_base;        /* 5c */
    int (*callbacks[4])(struct ANIMATION *, int);
    int loops[4];
    ELEMENT elements[32];               /* 78 */
    word banks;                         /* 4b8 */
    int handle_count;                   /* 4ba */
    FADESTATE far *fade;                /* 4bc */
    Handle handles[1];                  /* 4c0 */
} ANIMATION;

typedef int (*ANIMATIONCALLBACK)(ANIMATION *, int);

typedef int (*ANIMATIONCALLBACK)(ANIMATION *, int);

extern int RLEShapeCutOff;
extern char *cp;
extern int TGAMode;
extern int ForceShapes;
extern int ForceFuncParam;

ANIMATION *near advance_animation_frame(ANIMATION *animation);
ANIMATION *AnimDraw(ANIMATION *animation);
void AnimFree(ANIMATION *animation);
int AnimIdle(ANIMATION *animation);
ANIMATION *AnimNew(const byte far *commands, Handle sheet, int first_shape, int last_shape, int save_background);
ANIMATION *DrawFrame(ANIMATION *animation);
int PlayAnim(Handle shapes, int scene, int scene_offset, int save_background, ANIMATIONCALLBACK callback, int restore, int flags, int interruptible);
void SwapBackGround(ANIMATION *animation);
