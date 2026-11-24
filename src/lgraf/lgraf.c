/* LGRAF.C (original module, FM Towns debug symbols): back ports, colour cycling and rectangles.
   CamelCase names are anchored (docs/source-layout.md); snake_case names are descriptive.
   EncloseRect: union(rect, other) == rect, between DeallocBackPort and EqualRect in both builds. */
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
typedef struct {
    int top, left, bottom, right;
} Rect;

extern unsigned Graf;
static char allocation_message[] = "ERROR:  Couldn't allocate backport.";

void near * far cdecl _nmalloc(unsigned);
void far cdecl _nfree(void near *);
void far * far cdecl _fmemcpy(void far *, const void far *, unsigned);
int far cdecl _fmemcmp(const void far *, const void far *, unsigned);
int far pascal create_port(void near *, const Rect far *);
void far pascal abort_with_message(int, const char far *);
Color far * far pascal ALLOCATE_PINNED_PAYLOAD(long);
void far pascal READ_DAC_BLOCK(int, int, Color far *);
void far pascal WRITE_DAC_BLOCK(int, int, Color far *, int);
void far pascal CLEAR_PAYLOAD_HEADER(void far *);
void far pascal release_port(GraphicsRecord near *);
void far pascal RELEASE_BUFFER(unsigned, unsigned);
Rect far * far pascal UNIONRECT(Rect far *, const Rect far *, const Rect far *);
void far pascal MEASURE_TEXT(unsigned, GraphicsRecord near *, void far *, void far *, int);
void far pascal flip_rows(void far *, unsigned, unsigned);

void near * far pascal AllocBackPort(const Rect far *bounds)
{
    void near *record = _nmalloc(44);
    if (!create_port(record, bounds))
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
    READ_DAC_BLOCK(first, count, *palette_buffer);
    {
        Color saved = colors[0];
        _fmemcpy(colors, colors + 1, bytes - 3);
        *(Color far *)((char far *)colors + bytes - 3) = saved;
    }
    WRITE_DAC_BLOCK(first, count, colors, setting);
    CLEAR_PAYLOAD_HEADER(colors);
}

void far pascal DeallocBackPort(GraphicsRecord near *record)
{
    if (record) {
        release_port(record);
        _nfree(record);
    }
}

void far pascal free_record_pair(GraphicsRecord near *record)
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

void far pascal draw_record_bounds(GraphicsRecord near *record)
{
    void far *bounds = (char near *)record + 16;
    MEASURE_TEXT(Graf, record, bounds, bounds, 0);
}

void far pascal InvertBitMap(const BitmapRecord far *bitmap)
{
    flip_rows(bitmap->pixels, bitmap->rows, bitmap->pitch);
}
