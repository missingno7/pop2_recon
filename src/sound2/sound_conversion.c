typedef unsigned Handle;
typedef void (far pascal *SoundCallback)(void far *, unsigned);

#pragma pack(1)
typedef struct {
    unsigned char type;
    unsigned rate;
    signed char bits;
    unsigned length;
    unsigned loop_start, loop_end;
} SoundHeader;

typedef struct {
    SoundHeader header;
    unsigned expanded_length;
} PackedSound;
#pragma pack()

extern void far *h2p(Handle);
extern int QUERY_SOUND_PAYLOAD(const void far *);
extern void ResetSound(void far *);
extern int hGetState(Handle);
extern void far *hLock(Handle);
extern int START_SOUND_PAYLOAD(void far *, SoundCallback);
extern void hSetState(Handle, int);
extern Handle p2h(const void far *);
extern void hUnlock(Handle);

extern void far *MATERIALIZE_HANDLE(Handle);
extern unsigned long hGetSize(Handle);
extern long DIVIDE_TIMER_INTERVAL(long, long);
extern long MULTIPLY_TIMER_INTERVAL(long, long);
extern int hSetSize(Handle, unsigned long);
extern char far *MOVE_HUGE_BYTES(char far *, const char far *, unsigned long);
extern char far *RESAMPLE_SOUND(char far *, const char far *, unsigned, unsigned);
extern void UNPACK_DELTA_BITS(char far *, const char far *);
extern Handle mark_handle_converted(Handle);
extern unsigned digital_rate, digital_type;

void sound_payload_callback(void far *payload, unsigned status);

int process_handle(Handle handle)
{
    int result;
    int flags;
    void far *payload;

    if (handle) {
        payload = h2p(handle);
        if (QUERY_SOUND_PAYLOAD(payload))
            ResetSound(payload);
        flags = hGetState(handle);
        result = START_SOUND_PAYLOAD(hLock(handle), sound_payload_callback);
        if (!result)
            hSetState(handle, flags);
    } else {
        result = 0;
    }
    return result;
}

void sound_payload_callback(void far *payload, unsigned status)
{
    hUnlock(p2h(payload));
}

Handle convert_sound_handle(Handle handle)
{
    SoundHeader far *sound;
    unsigned long length, output_length, capacity;
    unsigned rate;
    long ratio;
    long header_size;
    char far *input, far *output;

    if (!(hGetState(handle) & 0x2000)) {
        sound = MATERIALIZE_HANDLE(handle);
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
                        input = (char far *)sound - (unsigned)length + (unsigned)capacity + header_size;
                        output = (char far *)(sound + 1);
                        MOVE_HUGE_BYTES(input, output, length);
                        UNPACK_DELTA_BITS(output, input);
                        sound->bits = 8;
                        sound->length = (unsigned)output_length;
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
                    ratio = DIVIDE_TIMER_INTERVAL(rate, sound->rate);
                    output_length = MULTIPLY_TIMER_INTERVAL(ratio, length);
                    capacity = (output_length < length ? length : output_length) + 256;
                    if (!hSetSize(handle, capacity + header_size)) {
                        sound = h2p(handle);
                        input = (char far *)sound - (unsigned)length + (unsigned)capacity + header_size;
                        output = (char far *)(sound + 1);
                        MOVE_HUGE_BYTES(input, output, length);
                        RESAMPLE_SOUND(output, input, (unsigned)output_length, (unsigned)length);
                        sound->rate = rate;
                        sound->length = (unsigned)output_length;
                        sound->loop_start = (unsigned)MULTIPLY_TIMER_INTERVAL(ratio, sound->loop_start);
                        sound->loop_end = (unsigned)MULTIPLY_TIMER_INTERVAL(ratio, sound->loop_end);
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
