/* UTIL.C: rectangle offsets, text resources, memory limits, and RNG. */
#include <string.h>
#include <bios.h>
#include "types.h"
#include "resc.h"
#include "system.h"
#include "level.h"
#include "heap.h"
#include "intrface.h"
#include "util.h"

static long random_state;
static int random_seeded = 0;
Rect NullRect = { 0, 0, 0, 0 };


Rect far * AlterRect(register Rect far *destination,
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

Handle hGetTxt4(int id)
{
    return hXlatTxt4(hGetRsrc(0x54585434UL, id));
}

unsigned long expand_memory_limits(void)
{
    resize_dos_heap(0x00ffffffUL);
    HeapCompact(0x00ffffffUL);
}

word Rnd(word maximum)
{
    if (!random_seeded)
        SetRndSeed(0);
    return ((unsigned long)(random_state = random_state * 214013L + 2531011L) >> 16)
           % (unsigned long)(maximum + 1);
}

void SetRndSeed(word seed)
{
    if (seed == 0)
        _bios_timeofday(0, &random_state);
    else
        random_state = seed;
    random_seeded = 1;
}


void SetScriptRect(int resource, const Rect far *bounds)
{
    Handle handle = hGetRsrc(0x00534352UL, resource);
    Rect far *scene;

    if (handle) {
        scene = h2p(handle);
        *scene = *bounds;
    }
}

void wait_for_action(int action)
{
    while (mouse_button_unqueued(action)) {
        CheckWaitFunc();
    }
}
