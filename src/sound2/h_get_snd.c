typedef unsigned Handle;

extern Handle far pascal hGetRsrc(unsigned long type, unsigned id);
extern Handle far pascal convert_sound_handle(Handle sound);
extern void far pascal hFree(Handle handle);

Handle far pascal hGetSnd(unsigned id)
{
    Handle sound = hGetRsrc(0x00534e44UL, id);
    if (sound && !convert_sound_handle(sound)) {
        hFree(sound);
        sound = 0;
    }
    return sound;
}
