#include <string.h>
#include "types.h"
#include "graf.h"
#include "level.h"
#include "heap.h"


int draw_text_handle(Handle handle)
{
    const char far *text = h2p(handle);
    if (text)
        return DrawString(text);
    return 0;
}

Handle create_string_handle(const char far *text)
{
    Handle handle = hNew((unsigned long)(_fstrlen(text) + 1));
    if (handle)
        _fstrcpy(h2p(handle), text);
    return handle;
}


Rect far *draw_text_handle_in_rect(Rect far *rect,
    int vertical_alignment, int horizontal_alignment, Handle handle)
{
    const char far *text = h2p(handle);
    if (text)
        return StringRect(rect, vertical_alignment, horizontal_alignment, text);
    return rect;
}

int measure_text_handle(Handle handle)
{
    const char far *text = h2p(handle);
    if (text)
        return measure_nul_terminated_text(text);
    return 0;
}
