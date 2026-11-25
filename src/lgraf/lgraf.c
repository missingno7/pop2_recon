/* LGRAF.C (original module, FM Towns debug symbols): back ports, colour cycling and rectangles.
   CamelCase names are anchored (docs/source-layout.md); snake_case names are descriptive.
   EncloseRect: union(rect, other) == rect, between DeallocBackPort and EqualRect in both builds. */
#include <string.h>
#include <malloc.h>
#include "types.h"
#include "graf.h"
#include "level.h"
#include "heap.h"
#include "intrface.h"
#include "lgraf.h"


typedef struct {
    byte red, green, blue;
} COLOR;

static char allocation_message[] = "ERROR:  Couldn't allocate backport.";


PORT *AllocBackPort(Rect far *bounds)
{
    PORT *port = _nmalloc(sizeof(PORT));
    if (!BackPort(port, bounds))
        abort_with_message(1, allocation_message);
    return port;
}

void CycleColors(int first, int count, int setting)
{
    int bytes = count * 3;
    COLOR far *colors;
    /* MSC keeps the allocation ahead of the Pascal argument pushes through this alias. */
    COLOR far **palette_buffer = &colors;

    colors = allocate_pinned_payload((long)bytes);
    ReadDACBlock(first, count, *palette_buffer);
    {
        COLOR saved = colors[0];
        _fmemcpy(colors, colors + 1, bytes - 3);
        *(COLOR far *)((char far *)colors + bytes - 3) = saved;
    }
    SetDACBlock(first, count, colors, setting);
    clear_payload_header(colors);
}

void DeallocBackPort(PORT *port)
{
    if (port) {
        FreePort(port);
        _nfree(port);
    }
}

void DeallocMapSave(SAVEDRECT *saved)
{
    release_buffer(saved->map.pixels);
    _nfree(saved);
}

int EncloseRect(const Rect far *rect, const Rect far *other)
{
    Rect work;
    UnionRect(&work, rect, other);
    return _fmemcmp(&work, rect, sizeof(work)) == 0;
}

int EqualRect(const Rect far *a, const Rect far *b)
{
    return _fmemcmp(a, b, sizeof(*a)) == 0;
}

void RestoreRectKeep(SAVEDRECT *saved)
{
    CopyBits(Graf, saved, &saved->rect, &saved->rect, 0);
}

void InvertBitMap(const BITMAP far *map)
{
    flip_rows(map->pixels, map->bounds.bottom, map->row_bytes);
}
