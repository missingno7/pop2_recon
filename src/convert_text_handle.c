#include "types.h"
#include "unpack.h"
#include "level.h"
#include "heap.h"
#include "util.h"


Handle hXlatTxt4(Handle handle)
{
    word expansion;
    word original_size;
    char far *payload;

    if (!(hGetState(handle) & 0x2000)) {
        expansion = hGetSize(handle);
        if (expansion && (payload = (char far *)materialize_handle(handle)) != 0 &&
            !hSetSize(handle, (unsigned long)((original_size = *(word far *)payload) + expansion))) {
            payload = h2p(handle);
            XlatTxt4(payload, move_huge_bytes(payload + original_size, payload, expansion));
            hSetSize(handle, original_size);
            mark_handle_converted(handle);
        } else
            handle = 0;
    }
    return handle;
}
