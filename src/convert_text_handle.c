typedef unsigned Handle;

extern unsigned far pascal SAVE_HANDLE_FLAGS(Handle handle);
extern unsigned long far pascal QUERY_HANDLE_SIZE(Handle handle);
extern unsigned far *far pascal MATERIALIZE_HANDLE(Handle handle);
extern int far pascal Resize_Handle(Handle handle, unsigned long bytes);
extern char far *far pascal RESOLVE_HANDLE_TO_FAR_PAYLOAD(Handle handle);
extern char far *far pascal MOVE_HUGE_BYTES(char far *destination, const char far *source, unsigned long bytes);
extern char far *far pascal XlatTxt4(char far *destination, const char far *source);
extern Handle far pascal Mark_Handle_Converted(Handle handle);

Handle convert_text_handle(Handle handle)
{
    unsigned expansion;
    unsigned original_size;
    char far *payload;

    if (!(SAVE_HANDLE_FLAGS(handle) & 0x2000)) {
        expansion = (unsigned)QUERY_HANDLE_SIZE(handle);
        if (!expansion)
            goto fail;
        payload = (char far *)MATERIALIZE_HANDLE(handle);
        if (!payload)
            goto fail;
        original_size = *(unsigned far *)payload;
        if (Resize_Handle(handle, (unsigned long)(original_size + expansion)))
            goto fail;
        payload = RESOLVE_HANDLE_TO_FAR_PAYLOAD(handle);
        XlatTxt4(payload, MOVE_HUGE_BYTES(payload + original_size, payload, expansion));
        Resize_Handle(handle, original_size);
        Mark_Handle_Converted(handle);
    }
    goto done;

fail:
    handle = 0;
done:
    return handle;
}
