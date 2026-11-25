#include "types.h"
#include "resc.h"
#include "snd.h"
#include "level.h"
#include "heap.h"


Handle hGetSnd(word id)
{
    Handle sound = hGetRsrc(0x00534e44UL, id);
    if (sound && !convert_sound_handle(sound)) {
        hFree(sound);
        sound = 0;
    }
    return sound;
}
