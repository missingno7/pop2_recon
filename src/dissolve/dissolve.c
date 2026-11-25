/* DISSOLVE.C */
#include <malloc.h>
#include "types.h"
#include "system.h"
#include "snd.h"
#include "graf.h"
#include "dissolve.h"
#include "init.h"
#include "util.h"
#include "intrface.h"


static const char dissolve_option[] = "NODIS";

DISSOLVEENV DissolveEnv;


int DissolveDoAll(int mode, PORT *source, const Rect far *bounds, long duration)
{
    int interrupted = 0;
    int more;
    DISSOLVE *record;

    record = DissolveNew(mode, source, bounds, duration);
    if (record) {
        do {
            interrupted = CheckWaitFunc();
            more = DissolveStep(record) == 0;
            CopyBits(&ScreenPort, record->destination,
                      &record->rect, &record->rect, 0);
        } while (!interrupted && more);
        DissolveFree(record, 0);
    }
    return interrupted;
}

void DissolveFree(DISSOLVE *record, int restore)
{
    if (restore)
        CopyBits(record->destination, record->source, &record->rect, &record->rect, 0);
    if (record->mode == 2)
        FreePort(&record->temporary);
    _nfree(record);
}

DISSOLVE *DissolveNew(int mode, PORT *source, const Rect far *bounds, long duration)
{
    DISSOLVE *record;

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
        dither_seed = 0x4583;

        if (record->mode == 2) {
            if (BackPort(&record->temporary, bounds))
                CopyBits(&record->temporary, record->source, bounds, bounds, 0);
            else {
                _nfree(record);
                record = 0;
            }
        }
        if (record)
            DissolveSetIncAmt(record);
        if (record->mode == 2)
            CopyBits(&record->temporary, record->destination, bounds, bounds, 0);
    }
    return record;
}

void DissolveSetIncAmt(DISSOLVE *record)
{
    long area;
    unsigned long elapsed;
    long start;
    PORT *saved_destination;
    int steps;

    area = (record->rect.bottom - record->rect.top) *
           (record->rect.right - record->rect.left);
    if (DissolveEnv.mode == record->mode && DissolveEnv.area == area) {
        elapsed = DissolveEnv.time;
    } else {
        start = TimeVal;
        saved_destination = record->destination;
        record->destination = record->source;
        record->level = 255;
        record->increment = 0;
        for (steps = 2; steps; --steps) {
            DissolveStep(record);
            CopyBits(record->source, record->source, &record->rect, &record->rect, 0);
        }
        record->destination = saved_destination;
        record->level = 0;
        elapsed = TimeVal - start;
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


void DissolveSetStepFunc(DISSOLVE *record, void (*step)(unsigned long, int), unsigned long argument)
{
    record->step = step;
    record->step_argument = argument;
}

int DissolveStep(DISSOLVE *record)
{
    int finished;

    finished = 0;
    if (record->level == finished) {
        record->start = TimeVal;
        record->level = record->increment;
    } else {
        long elapsed = TimeVal - record->start;

        if (record->duration - record->half_step_time / 2 >= elapsed) {
            record->level = (int)((unsigned long)(255L * elapsed) / record->duration);
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
        PORT *saved = Graf;

        Graf = record->destination;
        if (record->mode == 1)
            dither_seed = record->pattern;
        else if (record->mode == 2)
            CopyBits(record->destination, &record->temporary,
                      &record->rect, &record->rect, 0);
        if (!find_option(dissolve_option))
            copy_dithered_port_region(record->destination, record->source, &record->rect, &record->rect, record->level);
        else
            CopyBits(record->destination, record->source, &record->rect, &record->rect, 2);
        Graf = saved;
    } else {
        CopyBits(record->destination, record->source, &record->rect, &record->rect, 0);
    }
    return finished;
}
