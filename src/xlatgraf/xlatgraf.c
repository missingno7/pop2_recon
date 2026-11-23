extern volatile unsigned int ALLOCATED_UNIT_BITS;

unsigned int far pascal AllocBanks(unsigned char request)
{
    unsigned char units;
    register unsigned int selected;
    register unsigned int bit;

    selected = 0;
    bit = 1;
    units = (request + 15) >> 4;

    while (units--) {
        while (ALLOCATED_UNIT_BITS & bit) {
            bit <<= 1;
            if (bit == 0)
                return 0;
        }
        selected |= bit;
        bit <<= 1;
    }

    ALLOCATED_UNIT_BITS |= selected;
    return selected;
}

void far pascal ResetBanks(unsigned int banks)
{
    register unsigned int bit;

    ALLOCATED_UNIT_BITS = 0;
    bit = 1;
    while (banks--) {
        ALLOCATED_UNIT_BITS |= bit;
        bit <<= 1;
    }
}

typedef struct ShapeHeader {
    unsigned short height, width, format;
} ShapeHeader;

typedef union ShapeLayout {
    unsigned short word;
    unsigned char bytes[2];
} ShapeLayout;

/* Unverified draft: CalcRowBytes flags and width arithmetic. */
int far pascal CalcRowBytes(const ShapeHeader far *shape)
{
    ShapeLayout code;
    const volatile unsigned char *layout = code.bytes;
    int row_bytes;

    code.word = shape->format;
    if (!(layout[1] & 0x80))
        row_bytes = ((short)shape->width + 7) / 8;
    else
        row_bytes = (unsigned short)(((((unsigned short)layout[1] & 0x70U) >> 4) + 1U) * shape->width + 7U) >> 3;
    return row_bytes;
}

#pragma optimize("gl", on)
void far pascal MCGAToMCGA(const ShapeHeader far *shape,
                           unsigned char far *pixels, unsigned banks)
{
    unsigned char map[16];
    int mapped = 0;
    unsigned bit = 1;
    unsigned bank;
    unsigned char far *cursor;
    unsigned row;

    for (bank = 0; bank < 16; ++bank, bit <<= 1) {
        if (banks & bit)
            map[mapped++] = (unsigned char)(bank * 16);
    }

    cursor = pixels;
    for (row = 0; row < shape->height; ++row) {
        unsigned column;
        for (column = 0; column < shape->width; ++column) {
            if (*cursor)
                *cursor = (*cursor & 15) | map[*cursor >> 4];
            ++cursor;
        }
    }
}

#pragma optimize("gl", off)

int far pascal Get1stBank(unsigned banks)
{
    register int index = 0;
    register unsigned bit = 1;

    do {
        if (banks & bit) {
            return index;
        }
        bit <<= 1;
        ++index;
    } while (index < 16);
    return 0;
}

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

extern unsigned far pascal SAVE_HANDLE_FLAGS(Handle handle);
extern void far *far pascal FETCH_LOCKED_SHAPE_LIST(Handle handle);
extern void far pascal restore_handle_flags(Handle handle, unsigned flags);
extern void far *far pascal detach_handle_payload(Handle handle);
extern int far pascal attach_handle_payload(Handle handle, void far *payload);
extern void far pascal mark_handle_converted(Handle handle);
extern void far pascal clear_payload_header(void far *payload);
extern void far pascal dispose_handle(Handle handle);
extern Handle far pascal allocate_payload_handle(unsigned long size);
extern Handle far pascal mark_payload_owned(Handle handle);
extern void far *far cdecl far_copy(void far *destination, const void far *source, unsigned length);
extern void far pascal MaskToMCGA(Glyph far *glyph, unsigned char far *bits);

Handle far pascal XlatFont(FontHeader far *font);

Handle far pascal convert_font_handle(Handle handle)
{
    unsigned flags;
    FontHeader far *source;
    void far *replacement;
    Handle temporary;

    flags = SAVE_HANDLE_FLAGS(handle);
    if (!(flags & 0x2000)) {
        source = FETCH_LOCKED_SHAPE_LIST(handle);
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

/* Unverified draft: font translation. */
Handle far pascal XlatFont(FontHeader far *font)
{
    unsigned count;
    unsigned size;
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
        unsigned far *input = font->glyph_offset;
        unsigned left = count;
        do {
            Glyph far *cell = (Glyph far *)((unsigned long)font + *input);
            size += halve_for_mode_two(cell->width, 1) * cell->height;
            ++input;
        } while (--left);
    }

    handle = allocate_payload_handle(size);
    if (handle) {
        translated = FETCH_LOCKED_SHAPE_LIST(handle);
        far_copy(translated, font, 10);
        size = (count + 5) * 2;
        if (count) {
            source_offsets = font->glyph_offset;
            output_offsets = translated->glyph_offset;
            remaining = count;
            do {
                *output_offsets = size;
                glyph = (Glyph far *)((unsigned long)font + *source_offsets);
                converted_width = halve_for_mode_two(glyph->width, 1);
                glyph->row_bytes = (glyph->width + 7) / 8;
                bytes = glyph->row_bytes * glyph->height + 6;
                output = (Glyph far *)((unsigned long)translated + size);
                far_copy(output, glyph, bytes);
                MaskToMCGA(output, output->bits);
                output->row_bytes = converted_width;
                size += converted_width * glyph->height + 6;
                ++source_offsets;
                ++output_offsets;
            } while (--remaining);
        }
        mark_payload_owned(handle);
    }
    return handle;
}

extern volatile int video_mode;
extern unsigned chunk_byte_limit;
extern unsigned char far *far pascal NormalizePtr(unsigned char far *pointer);
extern void far pascal TgaToMCGA(ShapeHeader far *shape, unsigned char far *pixels, int bank);
extern int far pascal resize_payload(Handle handle, unsigned long bytes);

unsigned *far pascal copy_far_word(unsigned *out, const unsigned far *in);
Handle far pascal XlatShape(ShapeHeader far *shape, unsigned *banks, int mode);
Handle far pascal convert_banded_shape(ShapeHeader far *shape, unsigned *banks);
void far pascal remap_banded_pixels(ShapeHeader far *shape, unsigned char far *pixels, unsigned banks);

Handle far pascal convert_shape(ShapeHeader far *shape, const unsigned far *banks, int mode)
{
    unsigned local_banks[256];

    return XlatShape(shape, copy_far_word(local_banks, banks), mode);
}

/* Unverified draft: unbanded shape translation. */
Handle far pascal XlatShape(ShapeHeader far *shape, unsigned *banks, int mode)
{
    ShapeLayout layout;
    const unsigned char *format;
    int indexed, mask, wide;
    unsigned long input_allocation, output_allocation;
    unsigned long input_left, output_left;
    unsigned band_rows, rows_left, input_bytes, output_bytes;
    unsigned output_stride;
    Handle handle;
    ShapeHeader far *output;
    unsigned char far *input_bits;
    unsigned char far *output_bits;

    layout.word = shape->format;
    format = layout.bytes;
    if (*format & 0x7f)
        return convert_banded_shape(shape, banks);
    indexed = !(layout.word & 0x8000);
    mask = indexed && !(layout.bytes[1] & 0x70);
    wide = (video_mode == 6 || video_mode == 4) && indexed && (layout.bytes[1] & 0x70) == 0x30;
    shape->format = CalcRowBytes(shape);
    input_allocation = (long)(short)shape->format * (short)shape->height;
    if (wide)
        input_allocation <<= 2;
    input_left = input_allocation;
    input_allocation += sizeof(ShapeHeader);
    output_stride = halve_for_mode_two(shape->width, mode);
    output_allocation = (1L * (1 + 3 * (!mask * (video_mode == 4 || video_mode == 6)))) *
                        (short)shape->height * output_stride;
    output_left = output_allocation;
    chunk_byte_limit = (unsigned long)((long)(short)shape->height * (short)shape->width) > 64100UL
                       ? 58240U : 64100U;
    output_allocation += sizeof(ShapeHeader);
    handle = allocate_payload_handle(output_allocation < input_allocation ? input_allocation : output_allocation);
    if (handle) {
        output = FETCH_LOCKED_SHAPE_LIST(handle);
        *output = *shape;
        band_rows = chunk_byte_limit / shape->width < shape->height ? chunk_byte_limit / shape->width : shape->height;
        rows_left = shape->height;
        input_bytes = rows_left * shape->format;
        if (wide)
            input_bytes <<= 2;
        output_bytes = (1 + 3 * (!mask * (video_mode == 4 || video_mode == 6))) * output_stride * band_rows;
        input_bits = (unsigned char far *)(shape + 1);
        output_bits = (unsigned char far *)(output + 1);
        if (rows_left) {
            do {
                output_bytes = output_left < output_bytes ? (unsigned)output_left : output_bytes;
                input_bits = NormalizePtr(input_bits);
                output_bits = NormalizePtr(output_bits);
                band_rows = rows_left < band_rows ? rows_left : band_rows;
                input_bits += unpack_pixels(output_bits, input_bits,
                                            input_bytes = input_bytes > input_left ? (unsigned)input_left : input_bytes, shape->format,
                                            (output->height = band_rows), layout.bytes[1] & 15, wide);
                if (!indexed && (layout.bytes[1] & 0x70) == 0x30) {
                    if (mode == 1)
                        TgaToMCGA(output, output_bits, Get1stBank(*banks));
                    else if (mode == 2) {
                        unsigned pixels = (short)output->width * (short)output->height;
                        far_copy(output + 1, output_bits, (pixels & 1) + (pixels >> 1));
                    }
                } else if (mask) {
                    MaskToMCGA((Glyph far *)output, output_bits);
                } else if (!indexed && (layout.bytes[1] & 0x70) == 0x70) {
                    MCGAToMCGA(output, output_bits, *banks);
                } else {
                    goto discard;
                }
                input_left -= input_bytes;
                output_left -= output_bytes;
                output_bits += output_bytes;
                rows_left -= band_rows;
            } while (rows_left);
        }
        goto finish;

    discard:
        dispose_handle(handle);
        handle = 0;
        goto restore_format;

    finish:
        output->height = shape->height;
        output->format = output_stride;
        resize_payload(mark_payload_owned(handle), output_allocation);
    }

restore_format:
    shape->format = layout.word;
    return handle;
}

/* Unverified draft: banded shape translation. */
Handle far pascal convert_banded_shape(ShapeHeader far *shape, unsigned *banks)
{
    ShapeLayout layout;
    const unsigned char *format;
    int indexed, mask, wide;
    unsigned output_stride, band_rows, rows_left, band_bytes;
    volatile unsigned input_bytes;
    unsigned long input_allocation, input_left, output_size, added;
    volatile unsigned long output_left;
    Handle handle;
    ShapeHeader far *output;
    unsigned char far *input_bits;
    unsigned char far *volatile output_bits;

    layout.word = shape->format;
    format = layout.bytes;
    indexed = !(layout.word & 0x8000);
    mask = indexed && !(format[1] & 0x70);
    wide = (video_mode == 6 || video_mode == 4) && indexed && (format[1] & 0x70) == 0x30;
    shape->format = CalcRowBytes(shape);
    input_allocation = *(unsigned far *)(shape + 1);
    if (wide)
        input_allocation <<= 2;
    input_left = input_allocation;
    input_allocation += sizeof(ShapeHeader);
    output_stride = halve_for_mode_two(shape->width, 1);
    output_size = output_left = 0;
    chunk_byte_limit = (unsigned long)((long)(short)shape->width * (short)shape->height) > 64100UL
                       ? 58240U : 64100U;
    handle = allocate_payload_handle(input_allocation <= (output_size = sizeof(ShapeHeader)) ? output_size : input_allocation);
    if (handle) {
        unsigned band_limit;
        volatile int planar_mode;

        output = FETCH_LOCKED_SHAPE_LIST(handle);
        *output = *shape;
        band_limit = 58238L / (long)(short)((shape->width + 1) * 2);
        band_rows = band_limit < (rows_left = shape->height) ? band_limit : rows_left;
        input_bytes = band_rows * shape->format;
        planar_mode = video_mode == 4 || video_mode == 6;
        input_bits = (unsigned char far *)(shape + 1);
        if (rows_left) {
            do {
                band_rows = rows_left < band_rows ? rows_left : band_rows;
                band_bytes = *(unsigned far *)input_bits;
                input_bytes = band_bytes;
                input_bits += 2;
                mark_payload_owned(handle);
                added = band_bytes;
                if (resize_payload(handle, added + output_size))
                    goto discard;
                output = FETCH_LOCKED_SHAPE_LIST(handle);
                output_bits = (unsigned char far *)output + (unsigned)output_size;
                output_size += added;
                input_bits = NormalizePtr(input_bits);
                output_bits = NormalizePtr(output_bits);
                output->height = band_rows;
                input_bits += unpack_pixels(output_bits, input_bits, input_bytes, shape->format,
                                            band_rows, format[1] & 15, wide);
                if (!indexed && (format[1] & 0x70) == 0x30)
                    TgaToMCGA(output, output_bits, Get1stBank(*banks));
                else if (mask)
                    MaskToMCGA((Glyph far *)output, output_bits);
                else if (video_mode == 5 && !indexed && (format[1] & 0x70) == 0x70)
                    remap_banded_pixels(output, output_bits, *banks);
                else if (!((video_mode == 6 || video_mode == 4) && indexed && (format[1] & 0x70) == 0x30))
                    goto discard;
                input_left -= input_bytes;
                output_left -= added;
                rows_left -= band_rows;
            } while (rows_left);
        }
        output->height = shape->height;
        output->format = output_stride;
        resize_payload(mark_payload_owned(handle), output_size);
    }
    goto restore_format;

discard:
    dispose_handle(handle);
    handle = 0;
restore_format:
    shape->format = layout.word;
    return handle;
}

#pragma optimize("gl", on)
void far pascal remap_banded_pixels(ShapeHeader far *shape, unsigned char far *pixels, unsigned banks)
{
    unsigned char map[16];
    int mapped = 0;
    unsigned bit = 1;
    unsigned bank;
    unsigned row;
    unsigned char far *cursor;

    for (bank = 0; bank < 16; ++bank, bit <<= 1) {
        if (banks & bit)
            map[mapped++] = (unsigned char)(bank * 16);
    }

    cursor = pixels;
    for (row = 0; row < shape->height; ++row) {
        unsigned column;
        unsigned char count;
        cursor += 2;
        for (column = 0; column < shape->width; column += count) {
            unsigned char run = *cursor++;
            count = (run & 0x7f) + 1;
            if ((signed char)run < 0) {
                if (*cursor)
                    *cursor = (*cursor & 15) | map[*cursor >> 4];
                ++cursor;
            } else {
                for (mapped = 0; mapped < count; ++mapped) {
                    if (*cursor)
                        *cursor = (*cursor & 15) | map[*cursor >> 4];
                    ++cursor;
                }
            }
        }
    }
}
#pragma optimize("gl", off)

unsigned *far pascal copy_far_word(unsigned *out, const unsigned far *in)
{
    *out = *in;
    return out;
}
