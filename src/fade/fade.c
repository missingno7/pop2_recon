/* FADE.C: palette fades and DAC block transfers. */

typedef unsigned char component;
typedef component palette[768];
typedef struct fade_state fade_state;
typedef int (far pascal *fade_step)(fade_state far *);
typedef void (far pascal *fade_cleanup)(void far *);

typedef struct fade_header {
    unsigned words[4];
} fade_header;

struct fade_state {
    fade_header header;
    int delay;
    fade_step step;
    fade_cleanup cleanup;
    int elapsed;
    unsigned blocks;
    palette first;
    palette work;
    palette last;
};

extern fade_state far *far pascal ALLOCATE_PINNED_PAYLOAD(long size);
extern void far pascal CLEAR_PAYLOAD_HEADER(void far *block);
extern void far pascal READ_DAC_BLOCK(int first, int count, component far *colors);
extern void far pascal WRITE_DAC_BLOCK(int first, int count, component far *colors, int sync);
extern int far pascal check_wait_function(void);
extern void far pascal MEASURE_TEXT(int slot, int port, fade_state far *draw,
    fade_state far *update, int flags);
extern void far *far pascal RESOLVE_HANDLE_TO_FAR_PAYLOAD(void near *handle);
typedef struct { int top, left, bottom, right; } Rect;
extern void far pascal SET_RECORD_RANGE(const Rect far *bounds, int color);
extern void far *far cdecl _fmemset(void far *, int, unsigned);
extern void far *far cdecl _fmemcpy(void far *, const void far *, unsigned);
extern int far cdecl _fmemcmp(const void far *, const void far *, unsigned);

extern int fade_delay;
extern int Graf;
extern Rect screen_bounds;

fade_state far *far pascal init_fade_in(int port, fade_header far *header, unsigned blocks);
int far pascal fade_in_step(fade_state far *state);
void far pascal quit_fade_in(void far *block);
void far pascal read_dac_blocks(unsigned blocks, component far *colors);
void far pascal set_dac_blocks(unsigned, component far *);
fade_state far *far pascal init_fade_to(void near *handle, unsigned blocks, int delay);
void far pascal QuitFadeTo(void far *block);
int far pascal fade_to_step(fade_state far *state);
void far pascal blend_palette(component far *out, component far *first,
    component far *last, int first_factor, int last_factor);
void far pascal STORE_THREE_COMPONENT_BYTES();
void far pascal add_three_components(component far *, component far *);
void far pascal scale_three_component_bytes63(component far *, component far *, int);

int far pascal fade_in_port(fade_header near *port, unsigned blocks)
{
    int result = 0;
    fade_state far *state = init_fade_in((int)port,
        (fade_header near *)((char near *)port + 16), blocks);

    while (!fade_in_step(state))
        ;
    quit_fade_in(state);
    return result;
}

fade_state far *far pascal init_fade_in(int port, fade_header far *header, unsigned blocks)
{
    fade_state far *state = ALLOCATE_PINNED_PAYLOAD(sizeof(fade_state));

    state->header = *header;
    state->blocks = blocks; state->delay = state->elapsed = 0;
    state->cleanup = quit_fade_in; state->step = fade_in_step;
    read_dac_blocks(blocks, state->first);
    set_dac_blocks(blocks, 0);
    _fmemset(state->work, 0, sizeof(palette));
    MEASURE_TEXT(Graf, port, state, state, 0);
    fade_delay = 0;
    return state;
}
int far pascal fade_in_step(fade_state far *state)
{
    typedef struct { component red, green, blue; } RGB;
    int elapsed, bank, i;
    unsigned bit;
    RGB far *src;
    RGB far *dst;
    int done = 0;

    if (!fade_delay) {
        fade_delay = state->delay;
        elapsed = ++state->elapsed;
        bit = 1;
        for (bank = 0; bank < 16; ++bank, bit <<= 1) {
            if (state->blocks & bit) {
                src = (RGB far *)state->first + 16 * bank;
                dst = src + 256;
                for (i = 0; i < 16; ++i) {
                    dst[i].red = src[i].red * elapsed / 64;
                    dst[i].green = src[i].green * elapsed / 64;
                    dst[i].blue = src[i].blue * elapsed / 64;
                }
            }
        }
        set_dac_blocks(state->blocks, state->work);
        done = elapsed == 64;
    }
    return done;
}

void far pascal quit_fade_in(void far *block)
{
    fade_state far *state = block;

    set_dac_blocks(state->blocks, state->first);
    CLEAR_PAYLOAD_HEADER(block);
}

void far pascal read_dac_blocks(unsigned blocks, component far *colors)
{
    unsigned bit, offset;
    int first;

    if (blocks == 0xffff)
        READ_DAC_BLOCK(0, 256, colors);
    else {
        bit = 1;
        first = 0; offset = 0;
        do {
            if (blocks & bit)
                READ_DAC_BLOCK(first, 16, colors + offset);
            first += 16;
            bit <<= 1;
            offset += 48;
        } while (offset < sizeof(palette));
    }
}

void far pascal set_dac_blocks(unsigned blocks, component far *colors)
{
    unsigned bit, offset;
    unsigned first, changed;

    /* The fast paths reread the original far pointer after checking for null. */
    if (blocks == 0xffff)
        WRITE_DAC_BLOCK(0, 256, colors, 1);
    else if (blocks == 0xfffe) {
        if (*(component far *volatile *)&colors)
            WRITE_DAC_BLOCK(16, 240, colors + 48, 1);
        else
            WRITE_DAC_BLOCK(16, 240, (component far *)0L, 1);
    } else if (blocks == 0xfff8) {
        if (*(component far *volatile *)&colors)
            WRITE_DAC_BLOCK(48, 208, colors + 144, 1);
        else
            WRITE_DAC_BLOCK(48, 208, (component far *)0L, 1);
    } else {
        bit = 1;
        offset = first = changed = 0;
        do {
            if (blocks & bit) {
                if (colors)
                    WRITE_DAC_BLOCK(first, 16, colors + offset, (changed++ & 3) == 0);
                else
                    WRITE_DAC_BLOCK(first, 16, (component far *)0L, (changed++ & 3) == 0);
            }
            first += 16;
            bit <<= 1;
            offset += 48;
        } while (offset < sizeof(palette));
    }
}

int far pascal fade_to(void near *handle, unsigned blocks, int delay)
{
    int abort = 0;
    fade_state far *state = init_fade_to(handle, blocks, delay);

    if (_fmemcmp(state->last, state->first, sizeof(palette))) {
        while (!fade_to_step(state)) {
            abort = check_wait_function();
            if (abort)
                break;
        }
    }
    QuitFadeTo(state);
    return abort;
}

fade_state far *far pascal init_fade_to(void near *handle, unsigned blocks, int delay)
{
    fade_state far *state = ALLOCATE_PINNED_PAYLOAD(sizeof(fade_state));
    component far *last;

    state->blocks = blocks; state->elapsed = 0;
    state->delay = delay;
    state->cleanup = QuitFadeTo;
    state->step = fade_to_step;
    last = state->last;
    _fmemset(last, 0, sizeof(palette));
    read_dac_blocks(blocks, last);
    _fmemcpy(state->work, last, sizeof(palette));
    if (handle)
        _fmemcpy(state->first, RESOLVE_HANDLE_TO_FAR_PAYLOAD(handle), sizeof(palette));
    else
        _fmemset(state->first, 0, sizeof(palette));
    fade_delay = 0;
    return state;
}

/* Forward a far pointer to the separately reviewed block-flag helper. */
extern void far pascal CLEAR_PAYLOAD_HEADER(void far *block);

void far pascal QuitFadeTo(void far *block)
{
    CLEAR_PAYLOAD_HEADER(block);
}
int far pascal fade_to_step(fade_state far *state)
{
    typedef struct { component red, green, blue; } RGB;
    int elapsed, bank, i;
    unsigned bit;
    RGB far *src;
    RGB far *dst;
    RGB far *target;
    int done = 0;

    if (!fade_delay) {
        fade_delay = state->delay;
        elapsed = ++state->elapsed;
        bit = 1;
        for (bank = 0; bank < 16; ++bank, bit <<= 1) {
            if (state->blocks & bit) {
                src = (RGB far *)state->last + 16 * bank;
                target = src - 512;
                dst = src - 256;
                for (i = 0; i < 16; ++i) {
                    dst[i].red = src[i].red + (target[i].red - src[i].red) * elapsed / 64;
                    dst[i].green = src[i].green + (target[i].green - src[i].green) * elapsed / 64;
                    dst[i].blue = src[i].blue + (target[i].blue - src[i].blue) * elapsed / 64;
                }
            }
        }
        set_dac_blocks(state->blocks, state->work);
        done = elapsed == 64;
    }
    return done;
}

void far pascal fade_palette_up(component far *first, component far *last)
{
    palette colors;
    int step;

    step = 0;
    do {
        blend_palette(colors, first, last, 63 - step, step);
        WRITE_DAC_BLOCK(0, 128, colors, 1);
        WRITE_DAC_BLOCK(128, 128, colors + 384, 1);
    } while (++step < 64);
}

void far pascal fade_palette_down(component far *first, component far *last)
{
    palette colors;
    int step;

    step = 63;
    do {
        blend_palette(colors, first, last, 63 - step, step);
        WRITE_DAC_BLOCK(0, 128, colors, 1);
        WRITE_DAC_BLOCK(128, 128, colors + 384, 1);
    } while (--step >= 0);
}

void far pascal blend_palette(component far *out, component far *first,
    component far *last, int first_factor, int last_factor)
{
    int block, color, offset, index;
    component scaled[3];

    block = offset = 0;
    do {
        color = 0;
        index = block * 3;
        block += 16;
        do {
            STORE_THREE_COMPONENT_BYTES(&out[index], 0, 0, 0);
            scale_three_component_bytes63(scaled, &first[offset], first_factor);
            add_three_components(&out[index], scaled);
            scale_three_component_bytes63(scaled, &last[color], last_factor);
            add_three_components(&out[index], scaled);
            index += 3;
            color += 3;
        } while (color < 48);
        offset += 3;
    } while (offset < 48);
}

void far pascal STORE_THREE_COMPONENT_BYTES(
    unsigned char far *p,
    unsigned char a,
    unsigned char b,
    unsigned char c)
{
    p[0] = a;
    p[1] = b;
    p[2] = c;
}

void far pascal add_three_components(component far *out, component far *in)
{
    out[0] += in[0];
    out[1] += in[1];
    out[2] += in[2];
}

void far pascal scale_three_component_bytes63(component far *out, component far *in, int scale)
{
    out[0] = in[0] * scale / 63;
    out[1] = in[1] * scale / 63;
    out[2] = in[2] * scale / 63;
}

extern int (far *poll_input)(void);

void far pascal fade_out_and_restore_palette(void)
{
    int (far *saved)(void) = poll_input;

    poll_input = 0;
    fade_to(0, 0xffff, 0);
    SET_RECORD_RANGE(&screen_bounds, 0);
    poll_input = saved;
}
