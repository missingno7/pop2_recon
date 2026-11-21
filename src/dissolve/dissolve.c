/* DISSOLVE.C */
typedef struct
{
    int top, left, bottom, right;
} Rect;

typedef void (far pascal *StepFunc)(unsigned long, int);

typedef struct
{
    int mode;
    int source;
    int destination;
    unsigned char temporary[44];
    int increment;
    int half_step_time;
    long duration;
    long start;
    int level;
    Rect rect;
    StepFunc step;
    unsigned long step_argument;
    int pattern;
} Dissolve;

extern int current_port;
extern volatile int drawing_pattern;
extern volatile long clock_ticks;
extern char screen_port[];
static const char dissolve_option[] = "NODIS";
extern int cached_mode;
typedef union
{
    long value;
    struct { unsigned int low; int high; } words;
} LongWords;
extern long cached_area;
extern unsigned long cached_time;

extern int poll_events(void);
extern void copy_port(int source, int destination, const Rect far *source_rect,
                      const Rect far *destination_rect, int mode);
extern void release_port(void near *port);
extern int create_port(void near *port, const Rect far *rect);
extern int find_option(const char far *option);
extern void mix_ports(int source, int destination, const Rect far *source_rect,
                      const Rect far *destination_rect, int level);
extern void near *cdecl malloc(unsigned int size);
extern void cdecl free(void near *pointer);

Dissolve *dissolve_new(int mode, int source, const Rect far *bounds, long duration);
void dissolve_free(Dissolve *record, int restore);
int dissolve_step(Dissolve *record);
void dissolve_set_inc_amt(Dissolve *record);

int dissolve_do_all(int mode, int source, const Rect far *bounds, long duration)
{
    int interrupted = 0;
    int more;
    Dissolve *record;

    record = dissolve_new(mode, source, bounds, duration);
    if (record) {
        do {
            interrupted = poll_events();
            more = dissolve_step(record) == 0;
            copy_port((int)screen_port, record->destination,
                      &record->rect, &record->rect, 0);
        } while (!interrupted && more);
        dissolve_free(record, 0);
    }
    return interrupted;
}

void dissolve_free(Dissolve *record, int restore)
{
    if (restore)
        copy_port(record->destination, record->source, &record->rect, &record->rect, 0);
    if (record->mode == 2)
        release_port(&record->temporary);
    free(record);
}

Dissolve *dissolve_new(int mode, int source, const Rect far *bounds, long duration)
{
    Dissolve *record;

    record = malloc(sizeof(*record));
    if (record) {
        record->source = source;
        record->rect = *bounds;
        record->duration = duration;
        record->level = 0;
        record->destination = current_port;
        record->step = 0;
        record->step_argument = 0;
        record->mode = mode;
        record->pattern = 0x4583;
        drawing_pattern = 0x4583;

        if (record->mode == 2) {
            if (create_port(&record->temporary, bounds))
                copy_port((int)&record->temporary, record->source, bounds, bounds, 0);
            else {
                free(record);
                record = 0;
            }
        }
        if (record)
            dissolve_set_inc_amt(record);
        if (record->mode == 2)
            copy_port((int)&record->temporary, record->destination, bounds, bounds, 0);
    }
    return record;
}

void dissolve_set_inc_amt(Dissolve *record)
{
    long area;
    unsigned long elapsed;
    long start;
    int saved_destination, steps;

    area = (record->rect.bottom - record->rect.top) *
           (record->rect.right - record->rect.left);
    if (cached_mode == record->mode && cached_area == area) {
        elapsed = cached_time;
    } else {
        start = clock_ticks;
        saved_destination = record->destination;
        record->destination = record->source;
        record->level = 255;
        record->increment = 0;
        for (steps = 2; steps; --steps) {
            dissolve_step(record);
            copy_port(record->source, record->source, &record->rect, &record->rect, 0);
        }
        record->destination = saved_destination;
        record->level = 0;
        elapsed = clock_ticks - start;
        if (!elapsed)
            elapsed = 1;
        cached_mode = record->mode;
        cached_area = area;
        cached_time = elapsed;
    }
    steps = (int)((record->duration << 1) / elapsed);
    if (steps <= 5)
        steps = 0;
    else if (steps > 255)
        steps = 254;
    record->increment = 255 / (steps + 1);
    record->half_step_time = (int)((unsigned long)record->duration / (255 / record->increment));
}

struct RECORD_WITH_LONG_PAIR
{
    unsigned char prefix[72];
    unsigned long first;
    unsigned long second;
};

void far pascal DissolveSetStepFunc(
    struct RECORD_WITH_LONG_PAIR near *record,
    unsigned long first,
    unsigned long second)
{
    record->first = first;
    record->second = second;
}

int dissolve_step(Dissolve *record)
{
    LongWords scratch;
    int finished;

    finished = 0;
    if (record->level == finished) {
        record->start = clock_ticks;
        record->level = record->increment;
    } else {
        scratch.value = clock_ticks - record->start;
        if (record->duration - record->half_step_time / 2 >= scratch.value) {
            record->level = (int)((unsigned long)(255L * scratch.value) / record->duration);
            if (!record->level)
                record->level = 1;
        } else {
            record->level = 255;
            finished = 1;
        }
    }
    if (record->step)
        record->step(record->step_argument, record->level);
    if (!finished) {
        scratch.words.high = current_port;
        current_port = record->destination;
        if (record->mode == 1)
            drawing_pattern = record->pattern;
        else if (record->mode == 2)
            copy_port(record->destination, (int)&record->temporary,
                      &record->rect, &record->rect, 0);
        if (!find_option(dissolve_option))
            mix_ports(record->destination, record->source, &record->rect, &record->rect, record->level);
        else
            copy_port(record->destination, record->source, &record->rect, &record->rect, 2);
        current_port = scratch.words.high;
    } else {
        copy_port(record->destination, record->source, &record->rect, &record->rect, 0);
    }
    return finished;
}
