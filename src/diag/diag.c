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

typedef union {
    unsigned long value;
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

extern int position_value, reference_x, left_edge, right_edge;
extern signed char facing, page_number;
extern long tick_count;
extern drawing_context screen_context, *drawing_port;
extern int ruler_bottoms[];
extern heap_block far *far *heap_start;
extern char screen_name[];
static null_area null_snapshot;

static const char position_format[] = "Pos=%d";
static int dump_number = 0;
static char yes_text[] = "Yes";
static char no_text[] = "No ";
static char movable_text[] = "Relo";
static char fixed_text[] = "Fix ";
static char free_text[] = "Free";

int frame_diagnostics = 0;
static int diagnostic_default_options[3] = { 1, 0, 0 };
static int log_ticks = 0;
int show_reference_position = 0;
static int diagnostic_check_flags[2] = { 0, 0 };
int show_actor_position = 0;
int debug_show_door_links = 0;
int debug_show_meters = 0;
int show_ruler = 0;
int pause_on_frame = 0;
int debug_kid_y = 63;
int debug_kid_x = 140;
static int diagnostic_position_defaults[4] = { 20, 79, 34, 109 };

extern void copy_kid_to_char(void);
extern void get_frame_info(void);
extern void draw_message(char *);
extern void wait_key(void);
extern int get_base_x(void);
extern void MOVETO(int, int);
extern void draw_axis_segment(int, int);
extern void set_pen_color(int);
extern int hSetFont(int);
extern void DRAW_NUL_TERMINATED_TEXT(const char far *);
extern void far *RESOLVE_HANDLE_TO_FAR_PAYLOAD(int);
extern void STROKE_PORT_RECT(const char far *);
extern void paint_plain(const void far *, int, int, int);
extern void PAINT_INDEXED_SHAPE(const void far *, int, int, int, unsigned);
extern void paint_rle(const void far *, int, int, int);
extern int start_sound_payload(const unsigned char far *, void far *);
extern void wait_sound(int);
extern void dispatch_sound(int);
extern const char *find_option(const char far *);
extern void reclaim_heap_blocks(unsigned long);
extern void compact_heap(unsigned long);
extern unsigned block_capacity(const heap_block far *);
extern resource_metadata far *lookup_shape_metadata(resource_metadata far *, int);
extern heap_block far *next_heap_block(heap_block far *);
extern unsigned long largest_far_block(void);
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
        copy_kid_to_char();
        draw_frame_diagnostics();
        if (pause_on_frame) {
            if (show_actor_position) {
                copy_kid_to_char();
                get_frame_info();
                sprintf(message, position_format, position_value);
                draw_message(message);
            }
            wait_key();
        }
    }
}

void report_ticks(const char *label)
{
    char message[100];

    if (log_ticks) {
        sprintf(message, "%s. Ticks = %ld", label, tick_count);
        dump_heap(0, message, 1);
    }
}

void plot_edge_offset(register int distance)
{
    register int x;

    if (facing == 0)
        x = right_edge + distance;
    else
        x = left_edge - distance;
    distance = x - 130;
    plot_position(distance, 14);
}

void plot_actor_offset(int distance)
{
    int x = get_base_x() - 130;
    if (facing == 0)
        x += distance;
    else
        x -= distance;
    plot_position(x, 10);
}

void plot_position(int x, int color)
{
    register int y;
    register drawing_context *saved_port = drawing_port;
    volatile int saved_color;
    drawing_port = &screen_context;
    saved_color = screen_context.pen_color;
    y = ((volatile int *)ruler_bottoms)[page_number] - 5;
    screen_context.pen_color = color;
    MOVETO(x, y);
    draw_axis_segment(x, y + 2);
    drawing_port->pen_color = saved_color;
    drawing_port = saved_port;
}

void draw_ruler_marks(void)
{
    drawing_context *saved_port = drawing_port;
    int saved_color, saved_attribute, y, x;
    char label[10];

    drawing_port = &screen_context;
    saved_color = screen_context.pen_color;
    saved_attribute = screen_context.pen_attribute;
    set_pen_color(100);
    {
        int bottom = ruler_bottoms[page_number];
        y = bottom - 5;
    }
    x = 0;
    do {
        drawing_port->pen_color = (x % 2) ? 12 : 9;
        MOVETO(x, y);
        if (x % 10)
            draw_axis_segment(x, y);
        else {
            draw_axis_segment(x, y + 1);
            if (x % 20 == 0) {
                MOVETO(x, y + 15);
                itoa(x / 20, label, 10);
                DRAW_NUL_TERMINATED_TEXT(label);
            }
        }
        ++x;
    } while (x < 320);
    drawing_port->pen_color = saved_color;
    hSetFont(saved_attribute);
    drawing_port = saved_port;
}

static int preview_x = 0;

void preview_resource(int handle)
{
    image_header far *image = RESOLVE_HANDLE_TO_FAR_PAYLOAD(handle);

    if (preview_x == 0)
        STROKE_PORT_RECT(screen_name);
    if (image->width < 200 && image->height < 320 && image->width && image->height) {
        drawing_port = &screen_context;
        paint_plain(image, 0, preview_x, 0);
        PAINT_INDEXED_SHAPE(image, 100, preview_x, 0, 0x8000);
        paint_rle(image, 200, preview_x, 0);
        preview_x += image->width;
        if (preview_x > 280) {
            MOVETO(0, 300);
            DRAW_NUL_TERMINATED_TEXT("Waiting");
            wait_key();
            preview_x = 0;
        }
    }

    image = RESOLVE_HANDLE_TO_FAR_PAYLOAD(handle);
    if (*(unsigned char far *)image <= 2) {
        start_sound_payload((const unsigned char far *)image, 0);
        wait_sound(0);
        dispatch_sound(0);
    }
}

void dump_heap(const char *name, const char far *heading, int summary_only)
{
    char filename[66];
    resource_metadata metadata;
    union { unsigned long value; char bytes[5]; } type;
    file_handle *out;
    heap_block far *block;
    int size, flags;
    unsigned descriptor;
    long used = 0;
    /* The per-block preview decision queries the option again. */
    volatile int preview = find_option("SPECIAL") ? -1 : 0;

    reclaim_heap_blocks(0x00ffffffL);
    compact_heap(0x00ffffffL);
    if (name)
        strcpy(filename, name);
    else {
        char number[6];
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

        block = *heap_start;
        type.bytes[4] = '\0';
        if ((block->size_and_flags & 0xc000) != 0x4000) {
            do {
                size = block_capacity(block);
                flags = block->size_and_flags & 0xc000;
                if (flags) {
                    used += (unsigned)size;
                    if (!summary_only) {
                        if (flags == 0xc000) {
                            const char *resource_label, *lock_label, *purge_label;
                            descriptor = *(unsigned near *)block->handle;
                            resource_label = (descriptor & 0x1000) ? yes_text : no_text;
                            purge_label = (descriptor & 0x8000) ? yes_text : no_text;
                            lock_label = (descriptor & 0x4000) ? yes_text : no_text;

                            if (lookup_shape_metadata(&metadata, block->handle)) {
                                char byte;
                                type.value = metadata.type.value;
                                byte = type.bytes[3];
                                type.bytes[3] = type.bytes[0];
                                type.bytes[0] = byte;
                                byte = type.bytes[2];
                                type.bytes[2] = type.bytes[1];
                                type.bytes[1] = byte;
                                fprintf(out, "%s\t%5.u\t%s\t%s\t%s\t%4s\t%4.d\n",
                                        movable_text, size, purge_label, lock_label,
                                        resource_label, type.bytes, metadata.id);
                            } else {
                                fprintf(out, "%s\t%5.u\t%s\t%s\t%s\n",
                                        movable_text, size, purge_label, lock_label,
                                        resource_label);
                            }
                        } else if (flags == 0x8000)
                            fprintf(out, "%s\t%5.u\n", fixed_text, size);
                    }
                } else if (!summary_only)
                    fprintf(out, "%s\t%5.u\n", free_text, size);

                if (!size && find_option("SPECIAL"))
                    preview_resource(block->handle);
                block = next_heap_block(block);
            } while ((block->size_and_flags & 0xc000) != 0x4000);
        }

        fprintf(out, "\n%Fs\n", heading);
        fprintf(out, "Total used block size = %lu\n", used);
        fprintf(out, "Largest contigous block of far memory = %lu\n", largest_far_block());
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
            strcat(message, " far heap trashed.");
        abort_with_message(0, message);
    }
    if (check_near_heap()) {
        strcpy(message, label);
        if (strlen(message) < 120)
            strcat(message, " Near heap trashed.");
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
        get_frame_info();
        plot_position(get_base_x() - 130, 12);
    }
    if (show_ruler)
        draw_ruler_marks();
    if (show_reference_position)
        plot_position(reference_x - 130, 9);
}
