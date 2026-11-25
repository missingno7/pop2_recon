#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include "types.h"
#include "resc.h"
#include "system.h"
#include "snd.h"
#include "graf.h"
#include "shape.h"
#include "level.h"
#include "heap.h"
#include "char.h"
#include "game.h"
#include "draw.h"
#include "ctrl.h"
#include "diag.h"
#include "fade.h"
#include "init.h"
#include "intrface.h"
#include "prince.h"
#include "tables.h"
#include "util.h"


typedef struct {
    char bytes[66];
} null_area;


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
POINT DebugKid = { 63, 140 };
int debug_position_1 = 20;
int debug_position_2 = 79;
int debug_position_3 = 34;
int debug_position_4 = 109;


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
    PORT *saved_port = Graf;
    int saved_color;
    Graf = &ScreenPort;
    saved_color = ScreenPort.foreground;
    y = Row2PixelY[Char.row + 1] - 5;
    Graf->foreground = color;
    MoveTo(x, y);
    draw_axis_segment(x, y + 2);
    Graf->foreground = saved_color;
    Graf = saved_port;
}

void draw_ruler_marks(void)
{
    PORT *saved_port = Graf;
    int saved_color;
    Handle saved_font;
    int y, x;
    char label[10];

    Graf = &ScreenPort;
    saved_color = ScreenPort.foreground;
    saved_font = ScreenPort.font;
    SetFont(100);
    {
        int bottom = Row2PixelY[Char.row + 1];
        y = bottom - 5;
    }
    for (x = 0; x < 320; ++x) {
        Graf->foreground = (x % 2) ? 12 : 9;
        MoveTo(x, y);
        if (x % 10)
            draw_axis_segment(x, y);
        else {
            draw_axis_segment(x, y + 1);
            if (x % 20 == 0) {
                MoveTo(x, y + 15);
                itoa(x / 20, label, 10);
                DrawString(label);
            }
        }
    }
    Graf->foreground = saved_color;
    hSetFont(saved_font);
    Graf = saved_port;
}

static int preview_x = 0;

void preview_resource(Handle handle)
{
    SHAPE far *image = h2p(handle);

    if (preview_x == 0)
        EraseRect(&ScreenPortRect);
    if (image->height < 200 && image->width < 320 && image->height && image->width) {
        Graf = &ScreenPort;
        PaintShape(image, 0, preview_x, 0);
        paint_indexed_shape(image, 100, preview_x, 0, 0x8000);
        PaintRLEShape_asm(image, 200, preview_x, 0);
        preview_x += image->height;
        if (preview_x > 280) {
            MoveTo(0, 300);
            DrawString("Waiting");
            WaitKey();
            preview_x = 0;
        }
    }

    image = h2p(handle);
    if (*(byte far *)image <= 2) {
        start_sound_payload((const byte far *)image, 0);
        WaitSnd(0);
        ResetSnd(0);
    }
}

void dump_heap(const char *name, const char far *heading, int summary_only)
{
    char filename[66];
    RSRCINFO metadata;
    struct { RSRCTYPE code; char end; } type;
    FILE *out;
    HEAPBLOCK far *block;
    int size;
    word descriptor;
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

        block = heap_state->first;
        type.end = '\0';
        while ((block->size & 0xc000) != 0x4000) {
            size = banked_pointer_capacity(block);
            if (block->size & 0xc000) {
                used += (word)size;
                if (!summary_only) {
                    if ((block->size & 0xc000) == 0xc000) {
                        const char *resource_label, *lock_label, *purge_label;
                        descriptor = *(word *)block->handle;
                        resource_label = (descriptor & 0x1000) ? yes_text : no_text;
                        purge_label = (descriptor & 0x8000) ? yes_text : no_text;
                        lock_label = (descriptor & 0x4000) ? yes_text : no_text;

                        if (lookup_shape_metadata(&metadata, block->handle)) {
                            char byte;
                            type.code = metadata.type;
                            byte = type.code.letters[3];
                            type.code.letters[3] = type.code.letters[0];
                            type.code.letters[0] = byte;
                            byte = type.code.letters[2];
                            type.code.letters[2] = type.code.letters[1];
                            type.code.letters[1] = byte;
                            fprintf(out, "%s\t%5.u\t%s\t%s\t%s\t%4s\t%4.d\n",
                                    movable_text, size, purge_label, lock_label,
                                    resource_label, (const char *)&type, metadata.id);
                        } else {
                            fprintf(out, "%s\t%5.u\t%s\t%s\t%s\n",
                                    movable_text, size, purge_label, lock_label,
                                    resource_label);
                        }
                    } else if ((block->size & 0xc000) == 0x8000)
                        fprintf(out, "%s\t%5.u\n", fixed_text, size);
                }
            } else if (!summary_only)
                fprintf(out, "%s\t%5.u\n", free_text, size);

            if (size == preview_size && find_option("SPECIAL"))
                preview_resource(block->handle);
            block = normalize_banked_pointer(block);
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
    memcpy(&null_snapshot, (const void *)0, sizeof(null_snapshot));
}

int null_area_matches(void)
{
    int comparison = memcmp(&null_snapshot, (const void *)0, sizeof(null_snapshot));
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
