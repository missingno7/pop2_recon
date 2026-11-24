#include "types.h"
typedef unsigned Handle;

extern unsigned far cdecl _fstrlen(const char far *text);
extern char far *far cdecl _fstrcpy(char far *destination, const char far *source);
extern Handle far pascal allocate_payload_handle(unsigned long size);
extern void far *far pascal resolve_handle_to_far_payload(Handle handle);

extern int far pascal draw_nul_terminated_text(const char far *text);

int far pascal draw_text_handle(Handle handle)
{
    const char far *text = resolve_handle_to_far_payload(handle);
    if (text)
        return draw_nul_terminated_text(text);
    return 0;
}

Handle far pascal create_string_handle(const char far *text)
{
    Handle handle = allocate_payload_handle((unsigned long)(_fstrlen(text) + 1));
    if (handle)
        _fstrcpy(resolve_handle_to_far_payload(handle), text);
    return handle;
}

typedef Rect Rect;

extern Rect far *far pascal draw_nul_text_in_rect(Rect far *rect,
    int vertical_alignment, int horizontal_alignment, const char far *text);
extern int far pascal measure_nul_terminated_text(const char far *text);

Rect far *far pascal draw_text_handle_in_rect(Rect far *rect,
    int vertical_alignment, int horizontal_alignment, Handle handle)
{
    const char far *text = resolve_handle_to_far_payload(handle);
    if (text)
        return draw_nul_text_in_rect(rect, vertical_alignment, horizontal_alignment, text);
    return rect;
}

int far pascal measure_text_handle(Handle handle)
{
    const char far *text = resolve_handle_to_far_payload(handle);
    if (text)
        return measure_nul_terminated_text(text);
    return 0;
}
