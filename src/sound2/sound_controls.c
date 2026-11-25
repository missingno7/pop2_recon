typedef unsigned Handle;

extern Handle far pascal hGetSnd(unsigned id);
extern void far *far pascal h2p(Handle handle);
extern void far pascal ResetSound(void far *payload);
extern int far pascal process_handle(Handle handle);
extern int far pascal query_sound_payload(void far *payload);

extern void far pascal flag_sound_payload(void far *payload);

void far pascal flag_sound_resource(unsigned id)
{
    void far *payload = 0;
    if (id) {
        payload = h2p(hGetSnd(id));
        if (payload == 0)
            return;
    }
    flag_sound_payload(payload);
}

void far pascal ResetSnd(unsigned id)
{
    void far *payload = 0;

    if (id) {
        payload = h2p(hGetSnd(id));
        if (payload == 0)
            return;
    }
    ResetSound(payload);
}

int far pascal SetSnd(unsigned id)
{
    return process_handle(hGetSnd(id));
}

int far pascal SndOn(unsigned id)
{
    void far *payload = 0;

    if (id) {
        payload = h2p(hGetSnd(id));
        if (payload == 0)
            return 0;
    }
    return query_sound_payload(payload);
}
