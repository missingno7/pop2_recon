#include "types.h"
#include "char.h"
typedef struct {
    char private_fields[40];
    int pen_color, pen_attribute;
} drawing_context;

typedef struct {
    int width, height;
} image_header;

typedef struct heap_block {
    unsigned size_and_flags;
    struct heap_block near *resource;
    int handle;
} heap_block;

typedef struct {
    char bytes[4];
} resource_type;

typedef struct {
    int id;
    resource_type type;
} resource_metadata;

typedef struct file_handle file_handle;
typedef struct {
    char bytes[66];
} null_area;

extern CHARREC Kid;
extern CHARREC Char;
extern int LeftEdge, RightEdge;

extern long TimeVal;
extern drawing_context ScreenPort, *Graf;
extern int Row2PixelY[5];
extern heap_block far *far *heap_state;
extern char ScreenPortRect[];
static null_area null_snapshot;

static const char position_format[] = "Pos=%d";
static int dump_number = 0;
static char yes_text[] = "Yes";
static char no_text[] = "No ";
static char movable_text[] = "Relo";
static char fixed_text[] = "Fix ";
static char free_text[] = "Free";

int frame_diagnostics = 0;
/* The original link registers nine public debug variables here that no code refers to (docs/rtlink.md). */
int debug_option_1 = 1;
int debug_option_2 = 0;
int debug_option_3 = 0;
static int log_ticks = 0;
int show_reference_position = 0;
int debug_check_1 = 0;
int debug_check_2 = 0;
int show_actor_position = 0;
int debug_show_door_links = 0;
int debug_show_meters = 0;
int show_ruler = 0;
int pause_on_frame = 0;
struct { int y, x; } DebugKid = { 63, 140 };
int debug_position_1 = 20;
int debug_position_2 = 79;
int debug_position_3 = 34;
int debug_position_4 = 109;

extern void LoadKid(void);
extern void GetFrameInfo(void);
extern void DrawMessage(char *);
extern void WaitKey(void);
extern int GetBaseX(void);
extern void MOVETO(int, int);
extern void draw_axis_segment(int, int);
extern void SetFont(int);
extern int hSetFont(int);
extern void DrawString(const char far *);
extern void far *h2p(int);
extern void EraseRect(const char far *);
extern void PaintShape(const void far *, int, int, int);
extern void PAINT_INDEXED_SHAPE(const void far *, int, int, int, unsigned);
extern void PaintRLEShape_asm(const void far *, int, int, int);
extern int start_sound_payload(const unsigned char far *, void far *);
extern void WaitSnd(int);
extern void ResetSnd(int);
extern const char *find_option(const char far *);
extern void HeapPurge(unsigned long);
extern void HeapCompact(unsigned long);
extern unsigned banked_pointer_capacity(const heap_block far *);
extern resource_metadata far *lookup_shape_metadata(resource_metadata far *, int);
extern heap_block far *normalize_banked_pointer(heap_block far *);
extern unsigned long expand_memory_limits(void);
extern int check_heap_chain(void);
extern void abort_with_message(int, const char far *);

extern int _cdecl sprintf(char *, const char *, ...);
extern char *_cdecl itoa(int, char *, int);
extern file_handle *_cdecl fopen(const char *, const char *);
extern int _cdecl fprintf(file_handle *, const char *, ...);
extern int _cdecl fclose(file_handle *);
extern unsigned _cdecl _memmax(void);
extern int _cdecl _heapchk(void);
extern char *_cdecl strcpy(char *, const char *);
extern char *_cdecl strcat(char *, const char *);
extern unsigned _cdecl strlen(const char *);
extern void *_cdecl memcpy(void *, const void *, unsigned);
extern int _cdecl memcmp(const void *, const void *, unsigned);
#pragma intrinsic(strcpy, strcat, strlen, memcpy, memcmp)

void draw_frame_diagnostics(void);
void dump_heap(const char *, const char far *, int);
void plot_position(int, int);
int check_far_heap(void);
int check_near_heap(void);
int null_area_matches(void);

void limit_frame_rate(void)
{
    char message[80];

    if (frame_diagnostics) {
        LoadKid();
        draw_frame_diagnostics();
        if (pause_on_frame) {
            if (show_actor_position) {
                LoadKid();
                GetFrameInfo();
                sprintf(message, position_format, Kid.frame);
                DrawMessage(message);
            }
            WaitKey();
        }
    }
}

void report_ticks(const char *label)
{
    char message[100];

    if (log_ticks) {
        sprintf(message, "%s. Ticks = %ld", label, TimeVal);
        dump_heap(0, message, 1);
    }
}

void plot_edge_offset(int distance)
{
    int x;

    if (Char.direction == 0)
        x = RightEdge + distance - 130;
    else
        x = LeftEdge - distance - 130;
    plot_position(x, 14);
}

void plot_actor_offset(int distance)
{
    int x = GetBaseX() - 130;
    if (Char.direction == 0)
        x += distance;
    else
        x -= distance;
    plot_position(x, 10);
}

void plot_position(int x, int color)
{
    int y;
    drawing_context *saved_port = Graf;
    int saved_color;
    Graf = &ScreenPort;
    saved_color = ScreenPort.pen_color;
    y = Row2PixelY[Char.row + 1] - 5;
    Graf->pen_color = color;
    MOVETO(x, y);
    draw_axis_segment(x, y + 2);
    Graf->pen_color = saved_color;
    Graf = saved_port;
}

void draw_ruler_marks(void)
{
    drawing_context *saved_port = Graf;
    int saved_color, saved_attribute, y, x;
    char label[10];

    Graf = &ScreenPort;
    saved_color = ScreenPort.pen_color;
    saved_attribute = ScreenPort.pen_attribute;
    SetFont(100);
    {
        int bottom = Row2PixelY[Char.row + 1];
        y = bottom - 5;
    }
    x = 0;
    do {
        Graf->pen_color = (x % 2) ? 12 : 9;
        MOVETO(x, y);
        if (x % 10)
            draw_axis_segment(x, y);
        else {
            draw_axis_segment(x, y + 1);
            if (x % 20 == 0) {
                MOVETO(x, y + 15);
                itoa(x / 20, label, 10);
                DrawString(label);
            }
        }
        ++x;
    } while (x < 320);
    Graf->pen_color = saved_color;
    hSetFont(saved_attribute);
    Graf = saved_port;
}

static int preview_x = 0;

void preview_resource(int handle)
{
    image_header far *image = h2p(handle);

    if (preview_x == 0)
        EraseRect(ScreenPortRect);
    if (image->width < 200 && image->height < 320 && image->width && image->height) {
        Graf = &ScreenPort;
        PaintShape(image, 0, preview_x, 0);
        PAINT_INDEXED_SHAPE(image, 100, preview_x, 0, 0x8000);
        PaintRLEShape_asm(image, 200, preview_x, 0);
        preview_x += image->width;
        if (preview_x > 280) {
            MOVETO(0, 300);
            DrawString("Waiting");
            WaitKey();
            preview_x = 0;
        }
    }

    image = h2p(handle);
    if (*(unsigned char far *)image <= 2) {
        start_sound_payload((const unsigned char far *)image, 0);
        WaitSnd(0);
        ResetSnd(0);
    }
}

void dump_heap(const char *name, const char far *heading, int summary_only)
{
    char filename[66];
    resource_metadata metadata;
    struct { resource_type code; char end; } type;
    file_handle *out;
    heap_block far *block;
    int size;
    unsigned descriptor;
    long used = 0;
    /* Every released version selects zero with and without the option. */
    int preview_size;

    if (!find_option("SPECIAL"))
        preview_size = 0;
    else
        preview_size = 0;

    HeapPurge(0x00ffffffL);
    HeapCompact(0x00ffffffL);
    if (name)
        strcpy(filename, name);
    else {
        char number[5];
        strcpy(filename, "dump");
        itoa(dump_number++, number, 10);
        strcat(filename, number);
        strcat(filename, ".txt");
    }
    out = fopen(filename, "w+t");
    if (out) {
        fprintf(out, "%Fs\n", heading);
        if (!summary_only)
            fprintf(out, "BTYPE\tBYTES\tLOCK\tPURGE\tRSRC?\tRTYPE\tID\n");

        block = *heap_state;
        type.end = '\0';
        if ((block->size_and_flags & 0xc000) != 0x4000) {
            do {
                size = banked_pointer_capacity(block);
                if (block->size_and_flags & 0xc000) {
                    used += (unsigned)size;
                    if (!summary_only) {
                        if ((block->size_and_flags & 0xc000) == 0xc000) {
                            const char *resource_label, *lock_label, *purge_label;
                            descriptor = *(unsigned near *)block->handle;
                            resource_label = (descriptor & 0x1000) ? yes_text : no_text;
                            purge_label = (descriptor & 0x8000) ? yes_text : no_text;
                            lock_label = (descriptor & 0x4000) ? yes_text : no_text;

                            if (lookup_shape_metadata(&metadata, block->handle)) {
                                char byte;
                                type.code = metadata.type;
                                byte = type.code.bytes[3];
                                type.code.bytes[3] = type.code.bytes[0];
                                type.code.bytes[0] = byte;
                                byte = type.code.bytes[2];
                                type.code.bytes[2] = type.code.bytes[1];
                                type.code.bytes[1] = byte;
                                fprintf(out, "%s\t%5.u\t%s\t%s\t%s\t%4s\t%4.d\n",
                                        movable_text, size, purge_label, lock_label,
                                        resource_label, (const char *)&type, metadata.id);
                            } else {
                                fprintf(out, "%s\t%5.u\t%s\t%s\t%s\n",
                                        movable_text, size, purge_label, lock_label,
                                        resource_label);
                            }
                        } else if ((block->size_and_flags & 0xc000) == 0x8000)
                            fprintf(out, "%s\t%5.u\n", fixed_text, size);
                    }
                } else if (!summary_only)
                    fprintf(out, "%s\t%5.u\n", free_text, size);

                if (size == preview_size && find_option("SPECIAL"))
                    preview_resource(block->handle);
                block = normalize_banked_pointer(block);
            } while ((block->size_and_flags & 0xc000) != 0x4000);
        }

        fprintf(out, "\n%Fs\n", heading);
        fprintf(out, "Total used block size = %lu\n", used);
        fprintf(out, "Largest contigous block of far memory = %lu\n", expand_memory_limits());
        fprintf(out, "Largest contigous block of near memory = %u\n", _memmax());
        fclose(out);
    }
}

void check_heaps(const char *label)
{
    char message[150];

    if (check_far_heap()) {
        strcpy(message, label);
        if (strlen(message) < 120)
            strcat(message, ". Far heap trashed.");
        abort_with_message(0, message);
    }
    if (check_near_heap()) {
        strcpy(message, label);
        if (strlen(message) < 120)
            strcat(message, ". Near heap trashed.");
        abort_with_message(0, message);
    }
}

int check_near_heap(void)
{
    if (_heapchk() != -2)
        return 1;
    return 0;
}

int check_far_heap(void)
{
    return check_heap_chain();
}

void snapshot_null_area(void)
{
    memcpy(&null_snapshot, (const void near *)0, sizeof(null_snapshot));
}

int null_area_matches(void)
{
    int comparison = memcmp(&null_snapshot, (const void near *)0, sizeof(null_snapshot));
    return comparison == 0;
}

void check_null_area(const char *label)
{
    char message[150];

    if (!null_area_matches()) {
        strcpy(message, label);
        if (strlen(message) < 120)
            strcat(message, " NULL area trashed.");
        /* The DOS path passes the original label here. */
        abort_with_message(0, label);
    }
}

void draw_frame_diagnostics(void)
{
    if (show_actor_position) {
        GetFrameInfo();
        plot_position(GetBaseX() - 130, 12);
    }
    if (show_ruler)
        draw_ruler_marks();
    if (show_reference_position)
        plot_position(Char.x - 130, 9);
}

char volume_format[] = "Volume: '%.11s'\r%ld bytes free.";      /* unreferenced */
