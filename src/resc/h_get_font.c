typedef unsigned Handle;

extern Handle far pascal hGetRsrc(unsigned long type, unsigned id);
extern Handle far pascal convert_font_handle(Handle font);

Handle far pascal hGetFont(unsigned id)
{
    return convert_font_handle(hGetRsrc(0x464f4e54UL, id));
}
