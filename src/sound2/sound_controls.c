#include "types.h"
#include "snd.h"
#include "level.h"
#include "heap.h"


void flag_sound_resource(word id)
{
    void far *payload = 0;
    if (id) {
        payload = h2p(hGetSnd(id));
        if (payload == 0)
            return;
    }
    flag_sound_payload(payload);
}

void ResetSnd(word id)
{
    void far *payload = 0;

    if (id) {
        payload = h2p(hGetSnd(id));
        if (payload == 0)
            return;
    }
    ResetSound(payload);
}

int SetSnd(word id)
{
    return process_handle(hGetSnd(id));
}

int SndOn(int id)
{
    void far *payload = 0;

    if (id) {
        payload = h2p(hGetSnd(id));
        if (payload == 0)
            return 0;
    }
    return query_sound_payload(payload);
}
