/* UTIL.C: rectangle offsets, text resources, memory limits, and RNG. */
typedef struct {
    int top, left, bottom, right;
} Rect;

static unsigned long random_state;
extern int random_seeded;

void far * far cdecl memcpy(void far *, const void far *, unsigned);
int far pascal find_resource(unsigned long, int);
unsigned far pascal load_text_image(int);
void far pascal set_memory_limit_a(unsigned long);
void far pascal set_memory_limit_b(unsigned long);
unsigned far cdecl _bios_timeofday(unsigned, long near *);
int far pascal is_action_pending(int);
void far pascal service_events(void);
void far pascal SetRndSeed(unsigned);

Rect far * far pascal AlterRect(register Rect far *destination,
                               const Rect far *source,
                               int left, int top, int right, int bottom)
{
    memcpy(destination, source, sizeof(*destination));
    destination->left += left;
    destination->top += top;
    destination->right += right;
    destination->bottom += bottom;
    return destination;
}

unsigned far pascal hGetTxt4(int id)
{
    return load_text_image(find_resource(0x54585434UL, id));
}

void far pascal expand_memory_limits(void)
{
    set_memory_limit_a(0x00ffffffUL);
    set_memory_limit_b(0x00ffffffUL);
}

unsigned far pascal Rnd(unsigned maximum)
{
    if (!random_seeded) {
        SetRndSeed(0);
        random_seeded = 1;
    }
    return ((random_state = random_state * 214013UL + 2531011UL) >> 16)
           % (unsigned long)(maximum + 1);
}

void far pascal SetRndSeed(unsigned seed)
{
    if (seed == 0) {
        _bios_timeofday(0, (long near *)&random_state);
        return;
    }
    random_state = seed;
}

void far pascal wait_for_action(int action)
{
    if (is_action_pending(action)) {
        do {
            service_events();
        } while (is_action_pending(action));
    }
}
