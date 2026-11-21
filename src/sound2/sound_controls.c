typedef unsigned Handle;

extern Handle far pascal hGetSnd(unsigned id);
extern void far *far pascal resolve_handle_to_far_payload(Handle handle);
extern void far pascal dispatch_resource_pointer(void far *payload);
extern int far pascal process_sound_handle(Handle handle);
extern int far pascal query_sound_payload(void far *payload);

void far pascal dispatch_sound(unsigned id)
{
    void far *payload = 0;

    if (id) {
        payload = resolve_handle_to_far_payload(hGetSnd(id));
        if (payload == 0)
            return;
    }
    dispatch_resource_pointer(payload);
}

int far pascal play_sound(unsigned id)
{
    return process_sound_handle(hGetSnd(id));
}

int far pascal query_sound(unsigned id)
{
    void far *payload = 0;

    if (id) {
        payload = resolve_handle_to_far_payload(hGetSnd(id));
        if (payload == 0)
            return 0;
    }
    return query_sound_payload(payload);
}
