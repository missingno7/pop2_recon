typedef unsigned Handle;

typedef struct FontHeader {
    unsigned char first;
    unsigned char last;
    unsigned char metrics[8];
    unsigned glyph_offset[1];
} FontHeader;

typedef struct Glyph {
    int height;
    int width;
    int row_bytes;
    unsigned char bits[1];
} Glyph;

extern unsigned far pascal read_handle_flags(Handle handle);
extern void far *far pascal get_locked_payload(Handle handle);
extern void far pascal restore_handle_flags(Handle handle, unsigned flags);
extern void far *far pascal detach_handle_payload(Handle handle);
extern int far pascal attach_handle_payload(Handle handle, void far *payload);
extern void far pascal mark_handle_converted(Handle handle);
extern void far pascal clear_payload_header(void far *payload);
extern void far pascal dispose_handle(Handle handle);
extern Handle far pascal allocate_payload_handle(unsigned long size);
extern void far pascal mark_payload_owned(Handle handle);
extern void far *far cdecl far_copy(void far *destination, const void far *source, unsigned length);
extern void far pascal MaskToMCGA(Glyph far *glyph, unsigned char far *bits);

Handle far pascal XlatFont(FontHeader far *font);

Handle far pascal convert_font_handle(Handle handle)
{
    unsigned flags;
    FontHeader far *source;
    void far *replacement;
    Handle temporary;

    flags = read_handle_flags(handle);
    if (!(flags & 0x2000)) {
        source = get_locked_payload(handle);
        if (source) {
            temporary = XlatFont(source);
            restore_handle_flags(handle, flags);
            replacement = detach_handle_payload(temporary);
            if (replacement && attach_handle_payload(handle, replacement) == 0) {
                mark_handle_converted(handle);
            } else {
                clear_payload_header(replacement);
                handle = 0;
            }
            dispose_handle(temporary);
        }
    }
    return handle;
}

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

int far pascal halve_for_mode_two(int value, int mode)
{
    if (mode == 2)
        return value % 2 + value / 2;
    return value;
}

Handle far pascal XlatFont(FontHeader far *font)
{
    unsigned count;
    unsigned size;
    unsigned glyph_position;
    unsigned far *source_offsets;
    unsigned far *output_offsets;
    Glyph far *glyph;
    Glyph far *output;
    FontHeader far *translated;
    Handle handle;
    unsigned remaining;
    int converted_width;
    unsigned bytes;

    count = (unsigned)font->last - font->first + 1;
    size = count * (2 + 6) + 10;
    if (count) {
        source_offsets = font->glyph_offset;
        remaining = count;
        do {
            glyph = (Glyph far *)((unsigned long)font + *source_offsets);
            size += halve_for_mode_two(glyph->width, 1) * glyph->height;
            ++source_offsets;
        } while (--remaining);
    }

    handle = allocate_payload_handle(size);
    if (handle) {
        translated = get_locked_payload(handle);
        far_copy(translated, font, 10);
        glyph_position = (count + 5) * 2;
        if (count) {
            source_offsets = font->glyph_offset;
            output_offsets = translated->glyph_offset;
            remaining = count;
            do {
                *output_offsets = glyph_position;
                glyph = (Glyph far *)((unsigned long)font + *source_offsets);
                converted_width = halve_for_mode_two(glyph->width, 1);
                glyph->row_bytes = (glyph->width + 7) / 8;
                bytes = glyph->row_bytes * glyph->height + 6;
                output = (Glyph far *)((unsigned long)translated + glyph_position);
                far_copy(output, glyph, bytes);
                MaskToMCGA(output, output->bits);
                output->row_bytes = converted_width;
                glyph_position += glyph->row_bytes * glyph->height + 6;
                ++source_offsets;
                ++output_offsets;
            } while (--remaining);
        }
        mark_payload_owned(handle);
    }
    return handle;
}
