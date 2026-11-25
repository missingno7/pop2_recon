/* UTIL.C: rectangle offsets, text resources, memory limits, and RNG. */
#include "types.h"

static long random_state;
static int random_seeded = 0;
Rect NullRect = { 0, 0, 0, 0 };

void far * far cdecl _fmemcpy(void far *, const void far *, unsigned);
int far pascal HGETRSRC(unsigned long, int);
unsigned far pascal hXlatTxt4(int);
void far pascal RESIZE_DOS_HEAP(unsigned long);
void far pascal HeapCompact(unsigned long);
unsigned far cdecl _bios_timeofday(unsigned, long near *);
int far pascal mouse_button_unqueued(int);
void far pascal CheckWaitFunc(void);
void far pascal SetRndSeed(unsigned);

Rect far * far pascal AlterRect(register Rect far *destination,
                               const Rect far *source,
                               int left, int top, int right, int bottom)
{
    _fmemcpy(destination, source, sizeof(*destination));
    destination->left += left;
    destination->top += top;
    destination->right += right;
    destination->bottom += bottom;
    return destination;
}

unsigned far pascal hGetTxt4(int id)
{
    return hXlatTxt4(HGETRSRC(0x54585434UL, id));
}

void far pascal expand_memory_limits(void)
{
    RESIZE_DOS_HEAP(0x00ffffffUL);
    HeapCompact(0x00ffffffUL);
}

unsigned far pascal Rnd(unsigned maximum)
{
    if (!random_seeded)
        SetRndSeed(0);
    return ((unsigned long)(random_state = random_state * 214013L + 2531011L) >> 16)
           % (unsigned long)(maximum + 1);
}

void far pascal SetRndSeed(unsigned seed)
{
    if (seed == 0)
        _bios_timeofday(0, &random_state);
    else
        random_state = seed;
    random_seeded = 1;
}

void far *far pascal h2p(unsigned handle);

void far pascal SetScriptRect(int resource, const Rect far *bounds)
{
    unsigned handle = HGETRSRC(0x00534352UL, resource);
    Rect far *scene;

    if (handle) {
        scene = h2p(handle);
        *scene = *bounds;
    }
}

void far pascal wait_for_action(int action)
{
    if (mouse_button_unqueued(action)) {
        do {
            CheckWaitFunc();
        } while (mouse_button_unqueued(action));
    }
}
