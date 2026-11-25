/* FADE.C: palette fades and DAC block transfers. */
#include <string.h>
#include "types.h"
#include "system.h"
#include "english.h"
#include "graf.h"
#include "level.h"
#include "heap.h"
#include "char.h"
#include "fade.h"
#include "intrface.h"


int FadeInPort(PORT *port, word blocks)
{
    int result = 0;
    FADESTATE far *state = InitFadeIn(port,
        &port->visible, blocks);

    while (!FadeInStep(state))
        ;
    QuitFadeIn(state);
    return result;
}

FADESTATE far *InitFadeIn(PORT *port, const Rect far *rect, word blocks)
{
    FADESTATE far *state = allocate_pinned_payload(sizeof(FADESTATE));

    state->rect = *rect;
    state->blocks = blocks; state->delay = state->elapsed = 0;
    state->cleanup = QuitFadeIn; state->step = FadeInStep;
    ReadDACBlocks(blocks, state->first);
    SetDACBlocks(blocks, 0);
    _fmemset(state->work, 0, sizeof(state->work));
    CopyBits(Graf, port, &state->rect, &state->rect, 0);
    Timer[3] = 0;
    return state;
}
int FadeInStep(FADESTATE far *state)
{
    typedef struct { byte red, green, blue; } RGB;
    int elapsed, bank, i;
    word bit;
    RGB far *src;
    RGB far *dst;
    int done = 0;

    if (!Timer[3]) {
        Timer[3] = state->delay;
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
        SetDACBlocks(state->blocks, state->work);
        done = elapsed == 64;
    }
    return done;
}

void QuitFadeIn(void far *block)
{
    FADESTATE far *state = block;

    SetDACBlocks(state->blocks, state->first);
    clear_payload_header(block);
}

void ReadDACBlocks(word blocks, byte far *colors)
{
    word bit, offset;
    int first;

    if (blocks == 0xffff) ReadDACBlock(0, 256, colors);
    else {
        bit = 1;
        first = 0; offset = 0;
        do {
            if (blocks & bit)
                ReadDACBlock(first, 16, colors + offset);
            first += 16;
            bit <<= 1;
            offset += 48;
        } while (offset < 768);
    }
}

void SetDACBlocks(word blocks, byte far *colors)
{
    word bit, offset;
    word first, changed;

    /* The fast paths reread the original far pointer after checking for null. */
    if (blocks == 0xffff)
        SetDACBlock(0, 256, colors, 1);
    else if (blocks == 0xfffe) {
        if (*(byte far *volatile *)&colors)
            SetDACBlock(16, 240, colors + 48, 1);
        else
            SetDACBlock(16, 240, (byte far *)0L, 1);
    } else if (blocks == 0xfff8) {
        if (colors)
            SetDACBlock(48, 208, colors + 144, 1);
        else
            SetDACBlock(48, 208, (byte far *)0L, 1);
    } else {
        bit = 1;
        offset = first = changed = 0;
        do {
            if (blocks & bit) {
                if (colors)
                    SetDACBlock(first, 16, colors + offset, (changed++ & 3) == 0);
                else
                    SetDACBlock(first, 16, (byte far *)0L, (changed++ & 3) == 0);
            }
            first += 16;
            bit <<= 1;
            offset += 48;
        } while (offset < 768);
    }
}

int FadeTo(Handle colors, word blocks, word delay)
{
    int abort = 0;
    FADESTATE far *state = InitFadeTo(colors, blocks, delay);

    if (_fmemcmp(state->last, state->first, 768)) {
        while (!FadeToStep(state)) {
            abort = CheckWaitFunc();
            if (abort)
                break;
        }
    }
    QuitFadeTo(state);
    return abort;
}

FADESTATE far *InitFadeTo(Handle colors, int blocks, int delay)
{
    FADESTATE far *state = allocate_pinned_payload(sizeof(FADESTATE));
    byte far *last;

    state->blocks = blocks; state->elapsed = 0;
    state->delay = delay;
    state->cleanup = QuitFadeTo;
    state->step = FadeToStep;
    last = state->last;
    _fmemset(last, 0, 768);
    ReadDACBlocks(blocks, last);
    _fmemcpy(state->work, last, 768);
    if (colors)
        _fmemcpy(state->first, h2p(colors), 768);
    else
        _fmemset(state->first, 0, 768);
    Timer[3] = 0;
    return state;
}


void QuitFadeTo(void far *block)
{
    clear_payload_header(block);
}
int FadeToStep(FADESTATE far *state)
{
    typedef struct { byte red, green, blue; } RGB;
    int elapsed, bank, i;
    word bit;
    RGB far *src;
    RGB far *dst;
    RGB far *target;
    int done = 0;

    if (!Timer[3]) {
        Timer[3] = state->delay;
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
        SetDACBlocks(state->blocks, state->work);
        done = elapsed == 64;
    }
    return done;
}


void ForceFadeOut(void)
{
    int (*saved)(void) = WaitFunc;

    WaitFunc = 0;
    FadeTo(0, 0xffff, 0);
    FillRect(&ScreenPortRect, 0);
    WaitFunc = saved;
}
