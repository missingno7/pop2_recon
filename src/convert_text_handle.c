typedef unsigned Handle;

extern unsigned far pascal hGetState(Handle handle);
extern unsigned long far pascal hGetSize(Handle handle);
extern unsigned far *far pascal MATERIALIZE_HANDLE(Handle handle);
extern int far pascal hSetSize(Handle handle, unsigned long bytes);
extern char far *far pascal h2p(Handle handle);
extern char far *far pascal MOVE_HUGE_BYTES(char far *destination, const char far *source, unsigned long bytes);
extern char far *far pascal XlatTxt4(char far *destination, const char far *source);
extern Handle far pascal Mark_Handle_Converted(Handle handle);

Handle hXlatTxt4(Handle handle)
{
    unsigned expansion;
    unsigned original_size;
    char far *payload;

    if (!(hGetState(handle) & 0x2000)) {
        expansion = (unsigned)hGetSize(handle);
        if (expansion && (payload = (char far *)MATERIALIZE_HANDLE(handle)) != 0 &&
            !hSetSize(handle, (unsigned long)((original_size = *(unsigned far *)payload) + expansion))) {
            payload = h2p(handle);
            XlatTxt4(payload, MOVE_HUGE_BYTES(payload + original_size, payload, expansion));
            hSetSize(handle, original_size);
            Mark_Handle_Converted(handle);
        } else
            handle = 0;
    }
    return handle;
}
