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

extern int Graf;
extern volatile int DITHER_SEED;
extern volatile long clock_ticks;
extern char ScreenPort[];
static const char dissolve_option[] = "NODIS";
typedef struct {
    int mode;
    long area, time;
} DISSOLVEENV;
extern DISSOLVEENV DissolveEnv;
typedef union
{
    long value;
    struct { unsigned int low; int high; } words;
} LongWords;

extern int check_wait_function(void);
extern void MEASURE_TEXT(int source, int destination, const Rect far *source_rect,
                      const Rect far *destination_rect, int mode);
extern void release_port(void near *port);
extern int create_port(void near *port, const Rect far *rect);
extern int find_option(const char far *option);
extern void COPY_DITHERED_PORT_REGION(int source, int destination, const Rect far *source_rect,
                      const Rect far *destination_rect, int level);
extern void near *cdecl _nmalloc(unsigned int size);
extern void cdecl _nfree(void near *pointer);

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
            interrupted = check_wait_function();
            more = dissolve_step(record) == 0;
            MEASURE_TEXT((int)ScreenPort, record->destination,
                      &record->rect, &record->rect, 0);
        } while (!interrupted && more);
        dissolve_free(record, 0);
    }
    return interrupted;
}

void dissolve_free(Dissolve *record, int restore)
{
    if (restore)
        MEASURE_TEXT(record->destination, record->source, &record->rect, &record->rect, 0);
    if (record->mode == 2)
        release_port(&record->temporary);
    _nfree(record);
}

Dissolve *dissolve_new(int mode, int source, const Rect far *bounds, long duration)
{
    Dissolve *record;

    record = _nmalloc(sizeof(*record));
    if (record) {
        record->source = source;
        record->rect = *bounds;
        record->duration = duration;
        record->level = 0;
        record->destination = Graf;
        record->step = 0;
        record->step_argument = 0;
        record->mode = mode;
        record->pattern = 0x4583;
        DITHER_SEED = 0x4583;

        if (record->mode == 2) {
            if (create_port(&record->temporary, bounds))
                MEASURE_TEXT((int)&record->temporary, record->source, bounds, bounds, 0);
            else {
                _nfree(record);
                record = 0;
            }
        }
        if (record)
            dissolve_set_inc_amt(record);
        if (record->mode == 2)
            MEASURE_TEXT((int)&record->temporary, record->destination, bounds, bounds, 0);
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
    if (DissolveEnv.mode == record->mode && DissolveEnv.area == area) {
        elapsed = DissolveEnv.time;
    } else {
        start = clock_ticks;
        saved_destination = record->destination;
        record->destination = record->source;
        record->level = 255;
        record->increment = 0;
        for (steps = 2; steps; --steps) {
            dissolve_step(record);
            MEASURE_TEXT(record->source, record->source, &record->rect, &record->rect, 0);
        }
        record->destination = saved_destination;
        record->level = 0;
        elapsed = clock_ticks - start;
        if (!elapsed)
            elapsed = 1;
        DissolveEnv.mode = record->mode;
        DissolveEnv.area = area;
        DissolveEnv.time = elapsed;
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
        scratch.words.high = Graf;
        Graf = record->destination;
        if (record->mode == 1)
            DITHER_SEED = record->pattern;
        else if (record->mode == 2)
            MEASURE_TEXT(record->destination, (int)&record->temporary,
                      &record->rect, &record->rect, 0);
        if (!find_option(dissolve_option))
            COPY_DITHERED_PORT_REGION(record->destination, record->source, &record->rect, &record->rect, record->level);
        else
            MEASURE_TEXT(record->destination, record->source, &record->rect, &record->rect, 2);
        Graf = scratch.words.high;
    } else {
        MEASURE_TEXT(record->destination, record->source, &record->rect, &record->rect, 0);
    }
    return finished;
}
