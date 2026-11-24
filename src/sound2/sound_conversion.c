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

extern void far *RESOLVE_HANDLE_TO_FAR_PAYLOAD(Handle);
extern int QUERY_SOUND_PAYLOAD(const void far *);
extern void DISPATCH_RESOURCE_POINTER(void far *);
extern int SAVE_HANDLE_FLAGS(Handle);
extern void far *FETCH_LOCKED_SHAPE_LIST(Handle);
extern int START_SOUND_PAYLOAD(void far *, SoundCallback);
extern void RESTORE_HANDLE_FLAGS(Handle, int);
extern Handle owner_of_payload(const void far *);
extern void clear_handle_owned(Handle);

extern void far *MATERIALIZE_HANDLE(Handle);
extern unsigned long QUERY_HANDLE_SIZE(Handle);
extern long DIVIDE_TIMER_INTERVAL(long, long);
extern long MULTIPLY_TIMER_INTERVAL(long, long);
extern int resize_handle(Handle, unsigned long);
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
        payload = RESOLVE_HANDLE_TO_FAR_PAYLOAD(handle);
        if (QUERY_SOUND_PAYLOAD(payload))
            DISPATCH_RESOURCE_POINTER(payload);
        flags = SAVE_HANDLE_FLAGS(handle);
        result = START_SOUND_PAYLOAD(FETCH_LOCKED_SHAPE_LIST(handle), sound_payload_callback);
        if (!result)
            RESTORE_HANDLE_FLAGS(handle, flags);
    } else {
        result = 0;
    }
    return result;
}

void sound_payload_callback(void far *payload, unsigned status)
{
    clear_handle_owned(owner_of_payload(payload));
}

Handle convert_sound_handle(Handle handle)
{
    SoundHeader far *sound;
    unsigned long length, output_length, capacity;
    unsigned rate;
    long ratio;
    long header_size;
    char far *input, far *output;

    if (!(SAVE_HANDLE_FLAGS(handle) & 0x2000)) {
        sound = MATERIALIZE_HANDLE(handle);
        if (sound) {
            if (sound->type & 1) {
                header_size = sizeof(SoundHeader);
                length = QUERY_HANDLE_SIZE(handle) - header_size;
                switch (sound->bits) {
                case -1:
                    /* Move the packed input clear of the expanding output. */
                    output_length = ((PackedSound far *)sound)->expanded_length;
                    capacity = (output_length < length ? length : output_length) + 256;
                    if (!resize_handle(handle, capacity + header_size)) {
                        sound = RESOLVE_HANDLE_TO_FAR_PAYLOAD(handle);
                        input = (char far *)sound - (unsigned)length + (unsigned)capacity + header_size;
                        output = (char far *)(sound + 1);
                        MOVE_HUGE_BYTES(input, output, length);
                        UNPACK_DELTA_BITS(output, input);
                        sound->bits = 8;
                        sound->length = (unsigned)output_length;
                        resize_handle(handle, output_length + header_size);
                        mark_handle_converted(handle);
                    } else {
                        return 0;
                    }
                }

                sound = RESOLVE_HANDLE_TO_FAR_PAYLOAD(handle);
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
                    if (!resize_handle(handle, capacity + header_size)) {
                        sound = RESOLVE_HANDLE_TO_FAR_PAYLOAD(handle);
                        input = (char far *)sound - (unsigned)length + (unsigned)capacity + header_size;
                        output = (char far *)(sound + 1);
                        MOVE_HUGE_BYTES(input, output, length);
                        RESAMPLE_SOUND(output, input, (unsigned)output_length, (unsigned)length);
                        sound->rate = rate;
                        sound->length = (unsigned)output_length;
                        sound->loop_start = (unsigned)MULTIPLY_TIMER_INTERVAL(ratio, sound->loop_start);
                        sound->loop_end = (unsigned)MULTIPLY_TIMER_INTERVAL(ratio, sound->loop_end);
                        resize_handle(handle, output_length + header_size);
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
