/* Fading the palette in and out. */
typedef struct FADESTATE {
    Rect rect;                          /* of the port shown while fading in */
    int delay;
    int (*step)(struct FADESTATE far *);
    void (*cleanup)(void far *);
    int elapsed;
    word blocks;                        /* a bit for each 16 colors */
    byte first[768], work[768], last[768];
} FADESTATE;

int FadeInPort(PORT *port, word blocks);
int FadeInStep(FADESTATE far *state);
int FadeTo(Handle colors, word blocks, word delay);
int FadeToStep(FADESTATE far *state);
void ForceFadeOut(void);
FADESTATE far *InitFadeIn(PORT *port, const Rect far *rect, word blocks);
FADESTATE far *InitFadeTo(Handle colors, int blocks, int delay);
void QuitFadeIn(void far *block);
void QuitFadeTo(void far *block);
void ReadDACBlocks(word blocks, byte far *colors);
void SetDACBlocks(word blocks, byte far *colors);
