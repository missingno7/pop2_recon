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

extern unsigned graphics_port;
static char allocation_message[] = "ERROR:  Couldn't allocate backport.";

void near * far cdecl malloc(unsigned);
void far cdecl free(void near *);
void far * far cdecl memcpy(void far *, const void far *, unsigned);
int far cdecl memcmp(const void far *, const void far *, unsigned);
int far pascal initialize_record(void near *, const Rect far *);
void far pascal report_error(int, const char far *);
Color far * far pascal allocate_colors(long);
void far pascal read_colors(int, int, Color far *);
void far pascal install_colors(int, int, Color far *, int);
void far pascal release_far_block(void far *);
void far pascal dispose_record(GraphicsRecord near *);
void far pascal dispose_record_pair(unsigned, unsigned);
Rect far * far pascal union_rect(Rect far *, const Rect far *, const Rect far *);
void far pascal draw_record(unsigned, GraphicsRecord near *, void far *, void far *, int);
void far pascal flip_rows(void far *, unsigned, unsigned);

void near * far pascal AllocBackPort(const Rect far *bounds)
{
    void near *record = malloc(44);
    if (!initialize_record(record, bounds))
        report_error(1, allocation_message);
    return record;
}

void far pascal CycleColors(int first, int count, int setting)
{
    int bytes = count * 3;
    Color far *colors;
    /* MSC keeps the allocation ahead of the Pascal argument pushes through this alias. */
    Color far *near *palette_buffer = &colors;

    colors = allocate_colors((long)bytes);
    read_colors(first, count, *palette_buffer);
    {
        Color saved = colors[0];
        memcpy(colors, colors + 1, bytes - 3);
        *(Color far *)((char far *)colors + bytes - 3) = saved;
    }
    install_colors(first, count, colors, setting);
    release_far_block(colors);
}

void far pascal DeallocBackPort(GraphicsRecord near *record)
{
    if (record) {
        dispose_record(record);
        free(record);
    }
}

void far pascal free_record_pair(GraphicsRecord near *record)
{
    dispose_record_pair(record->second, record->first);
    free(record);
}

int far pascal EncloseRect(const Rect far *rect, const Rect far *other)
{
    Rect work;
    union_rect(&work, rect, other);
    return memcmp(&work, rect, sizeof(work)) == 0;
}

int far pascal EqualRect(const Rect far *a, const Rect far *b)
{
    return memcmp(a, b, sizeof(*a)) == 0;
}

void far pascal draw_record_bounds(GraphicsRecord near *record)
{
    void far *bounds = (char near *)record + 16;
    draw_record(graphics_port, record, bounds, bounds, 0);
}

void far pascal InvertBitMap(const BitmapRecord far *bitmap)
{
    flip_rows(bitmap->pixels, bitmap->rows, bitmap->pitch);
}
