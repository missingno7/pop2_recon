#include "types.h"
#include "unpack.h"
#include "system.h"
#include "snd.h"
#include "graf.h"
#include "level.h"
#include "heap.h"
#include "fade.h"
#include "anim.h"
typedef void (*SoundCallback)(void far *, word);

#pragma pack(1)
typedef struct {
    byte type;
    word rate;
    sbyte bits;
    word length;
    word loop_start, loop_end;
} SoundHeader;

typedef struct {
    SoundHeader header;
    word expanded_length;
} PackedSound;
#pragma pack()


int process_handle(Handle handle)
{
    int result;
    int flags;
    void far *payload;

    if (handle) {
        payload = h2p(handle);
        if (query_sound_payload(payload))
            ResetSound(payload);
        flags = hGetState(handle);
        result = start_sound_payload(hLock(handle), sound_payload_callback);
        if (!result)
            hSetState(handle, flags);
    } else {
        result = 0;
    }
    return result;
}

void sound_payload_callback(void far *payload, word status)
{
    hUnlock(p2h(payload));
}

Handle convert_sound_handle(Handle handle)
{
    SoundHeader far *sound;
    unsigned long length, output_length, capacity;
    word rate;
    long ratio;
    long header_size;
    char far *input, far *output;

    if (!(hGetState(handle) & 0x2000)) {
        sound = materialize_handle(handle);
        if (sound) {
            if (sound->type & 1) {
                header_size = sizeof(SoundHeader);
                length = hGetSize(handle) - header_size;
                switch (sound->bits) {
                case -1:
                    /* Move the packed input clear of the expanding output. */
                    output_length = ((PackedSound far *)sound)->expanded_length;
                    capacity = (output_length < length ? length : output_length) + 256;
                    if (!hSetSize(handle, capacity + header_size)) {
                        sound = h2p(handle);
                        input = (char far *)sound - length + capacity + header_size;
                        output = (char far *)(sound + 1);
                        move_huge_bytes(input, output, length);
                        unpack_delta_bits(output, input);
                        sound->bits = 8;
                        sound->length = output_length;
                        hSetSize(handle, output_length + header_size);
                        mark_handle_converted(handle);
                    } else {
                        return 0;
                    }
                }

                sound = h2p(handle);
                if (sound->rate < digital_rate)
                    rate = digital_rate;
                else if (sound->rate <= digital_type)
                    rate = 0;
                else
                    rate = digital_type;
                if (rate) {
                    length = sound->length;
                    ratio = divide_timer_interval(rate, sound->rate);
                    output_length = multiply_timer_interval(ratio, length);
                    capacity = (output_length < length ? length : output_length) + 256;
                    if (!hSetSize(handle, capacity + header_size)) {
                        sound = h2p(handle);
                        input = (char far *)sound - length + capacity + header_size;
                        output = (char far *)(sound + 1);
                        move_huge_bytes(input, output, length);
                        resample_sound(output, input, output_length, length);
                        sound->rate = rate;
                        sound->length = output_length;
                        sound->loop_start = multiply_timer_interval(ratio, sound->loop_start);
                        sound->loop_end = multiply_timer_interval(ratio, sound->loop_end);
                        hSetSize(handle, output_length + header_size);
                        mark_handle_converted(handle);
                    } else {
                        return 0;
                    }
                }
            }
        } else {
            return 0;
        }
    }
    return handle;
}
