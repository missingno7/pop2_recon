typedef unsigned Handle;

extern unsigned far pascal Read_Handle_Flags(Handle handle);
extern unsigned long far pascal Text_Expansion_Size(Handle handle);
extern unsigned far *far pascal Deref_Handle(Handle handle);
extern int far pascal Resize_Handle(Handle handle, unsigned long bytes);
extern char far *far pascal Handle_Payload(Handle handle);
extern char far *far pascal Move_Far_Bytes(char far *destination, const char far *source, unsigned long bytes);
extern char far *far pascal XlatTxt4(char far *destination, const char far *source);
extern Handle far pascal Mark_Handle_Converted(Handle handle);

Handle convert_text_handle(Handle handle)
{
    unsigned expansion;
    unsigned original_size;
    char far *payload;

    if (!(Read_Handle_Flags(handle) & 0x2000)) {
        expansion = (unsigned)Text_Expansion_Size(handle);
        if (!expansion)
            goto fail;
        payload = (char far *)Deref_Handle(handle);
        if (!payload)
            goto fail;
        original_size = *(unsigned far *)payload;
        if (Resize_Handle(handle, (unsigned long)(original_size + expansion)))
            goto fail;
        payload = Handle_Payload(handle);
        XlatTxt4(payload, Move_Far_Bytes(payload + original_size, payload, expansion));
        Resize_Handle(handle, original_size);
        Mark_Handle_Converted(handle);
    }
    goto done;

fail:
    handle = 0;
done:
    return handle;
}
