typedef struct PaletteInfo {
    unsigned banks;
    unsigned char format;
    unsigned char colors[16][48];
} PaletteInfo;

extern void far pascal write_dac_block(int first_color, int color_count,
                                      const unsigned char far *colors, int flags);

void far pascal MapPalInfo(const PaletteInfo far *palette)
{
    register int first_color;
    register int bank;

    if (palette->banks != 0) {
        int stored_bank = 0;
        bank = stored_bank;
        first_color = bank;
        do {
            if (palette->banks & (unsigned short)(1U << (unsigned short)bank))
                write_dac_block(first_color, 16, palette->colors[stored_bank++], 1);
            ++bank;
            first_color += 16;
        } while (first_color < 256);
    }
}

extern void far *far cdecl far_copy(void far *destination, const void far *source, unsigned length);
extern int far pascal UnpackRLEH(void far *destination, const void far *source, int length);
extern int far pascal UnpackRLEV(void far *destination, const void far *source,
                                int length, int stride, int rows);
extern int far pascal UnpackLZH(void far *destination, const void far *source, int length);
extern int far pascal UnpackLZV(void far *destination, const void far *source,
                               int length, int stride, int rows);

int far pascal unpack_pixels(void far *destination, const void far *source,
                             int length, int stride, int rows, int codec, int wide)
{
    int result;

    if (wide)
        stride *= 4;
    switch (codec) {
    default:
        far_copy(destination, source, length);
        result = length;
        break;
    case 1:
        result = UnpackRLEH(destination, source, length);
        break;
    case 2:
        result = UnpackRLEV(destination, source, length, stride, rows);
        break;
    case 3:
        result = UnpackLZH(destination, source, length);
        break;
    case 4:
        result = UnpackLZV(destination, source, length, stride, rows);
        break;
    }
    return result;
}
