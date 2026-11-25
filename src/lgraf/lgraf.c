/* LGRAF.C (original module, FM Towns debug symbols): back ports, colour cycling and rectangles.
   CamelCase names are anchored (docs/source-layout.md); snake_case names are descriptive.
   EncloseRect: union(rect, other) == rect, between DeallocBackPort and EqualRect in both builds. */
#include "types.h"
#pragma pack(1)
typedef struct {
    unsigned first, second;
    char bounds[12];
} GraphicsRecord;
typedef struct {
    void far *pixels;
    unsigned reserved[2];
    unsigned rows, unused, pitch;
} BitmapRecord;
typedef struct {
    unsigned char bytes[3];
} Color;

extern unsigned Graf;
static char allocation_message[] = "ERROR:  Couldn't allocate backport.";

void near * far cdecl _nmalloc(unsigned);
void far cdecl _nfree(void near *);
void far * far cdecl _fmemcpy(void far *, const void far *, unsigned);
int far cdecl _fmemcmp(const void far *, const void far *, unsigned);
int far pascal BackPort(void near *, const Rect far *);
void far pascal abort_with_message(int, const char far *);
Color far * far pascal ALLOCATE_PINNED_PAYLOAD(long);
void far pascal ReadDACBlock(int, int, Color far *);
void far pascal SetDACBlock(int, int, Color far *, int);
void far pascal CLEAR_PAYLOAD_HEADER(void far *);
void far pascal FreePort(GraphicsRecord near *);
void far pascal RELEASE_BUFFER(unsigned, unsigned);
Rect far * far pascal UNIONRECT(Rect far *, const Rect far *, const Rect far *);
void far pascal CopyBits(unsigned, GraphicsRecord near *, void far *, void far *, int);
void far pascal flip_rows(void far *, unsigned, unsigned);

void near * far pascal AllocBackPort(const Rect far *bounds)
{
    void near *record = _nmalloc(44);
    if (!BackPort(record, bounds))
        abort_with_message(1, allocation_message);
    return record;
}

void far pascal CycleColors(int first, int count, int setting)
{
    int bytes = count * 3;
    Color far *colors;
    /* MSC keeps the allocation ahead of the Pascal argument pushes through this alias. */
    Color far *near *palette_buffer = &colors;

    colors = ALLOCATE_PINNED_PAYLOAD((long)bytes);
    ReadDACBlock(first, count, *palette_buffer);
    {
        Color saved = colors[0];
        _fmemcpy(colors, colors + 1, bytes - 3);
        *(Color far *)((char far *)colors + bytes - 3) = saved;
    }
    SetDACBlock(first, count, colors, setting);
    CLEAR_PAYLOAD_HEADER(colors);
}

void far pascal DeallocBackPort(GraphicsRecord near *record)
{
    if (record) {
        FreePort(record);
        _nfree(record);
    }
}

void far pascal DeallocMapSave(GraphicsRecord near *record)
{
    RELEASE_BUFFER(record->second, record->first);
    _nfree(record);
}

int far pascal EncloseRect(const Rect far *rect, const Rect far *other)
{
    Rect work;
    UNIONRECT(&work, rect, other);
    return _fmemcmp(&work, rect, sizeof(work)) == 0;
}

int far pascal EqualRect(const Rect far *a, const Rect far *b)
{
    return _fmemcmp(a, b, sizeof(*a)) == 0;
}

void far pascal RestoreRectKeep(GraphicsRecord near *record)
{
    void far *bounds = (char near *)record + 16;
    CopyBits(Graf, record, bounds, bounds, 0);
}

void far pascal InvertBitMap(const BitmapRecord far *bitmap)
{
    flip_rows(bitmap->pixels, bitmap->rows, bitmap->pitch);
}
