/* Dissolving one image into another. */
typedef struct {
    int mode;
    PORT *source, *destination;
    PORT temporary;                     /* 06 */
    int increment;                      /* 32 */
    int half_step_time;
    long duration, start;
    int level;                          /* 3e */
    Rect rect;                          /* 40 */
    void (*step)(unsigned long, int);  /* 48 */
    unsigned long step_argument;
    int pattern;
} DISSOLVE;

typedef struct {
    int mode;
    long area, time;
} DISSOLVEENV;

extern DISSOLVEENV DissolveEnv;

int DissolveDoAll(int mode, PORT *source, const Rect far *bounds, long duration);
void DissolveFree(DISSOLVE *record, int restore);
DISSOLVE *DissolveNew(int mode, PORT *source, const Rect far *bounds, long duration);
void DissolveSetIncAmt(DISSOLVE *record);
void DissolveSetStepFunc(DISSOLVE *record, void (*step)(unsigned long, int), unsigned long argument);
int DissolveStep(DISSOLVE *record);
