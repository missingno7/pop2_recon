/* UTIL.C: rectangle offsets, text resources, memory limits, and RNG. */
typedef struct {
    int top, left, bottom, right;
} Rect;

unsigned long random_state;
static int random_seeded = 0;

void far * far cdecl memcpy(void far *, const void far *, unsigned);
int far pascal HGETRSRC(unsigned long, int);
unsigned far pascal convert_text_handle(int);
void far pascal RESIZE_DOS_HEAP(unsigned long);
void far pascal COMPACT_HEAP_BLOCKS(unsigned long);
unsigned far cdecl _bios_timeofday(unsigned, long near *);
int far pascal mouse_button_unqueued(int);
void far pascal check_wait_function(void);
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
    return convert_text_handle(HGETRSRC(0x54585434UL, id));
}

void far pascal expand_memory_limits(void)
{
    RESIZE_DOS_HEAP(0x00ffffffUL);
    COMPACT_HEAP_BLOCKS(0x00ffffffUL);
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
    if (mouse_button_unqueued(action)) {
        do {
            check_wait_function();
        } while (mouse_button_unqueued(action));
    }
}
