#include <string.h>
#include "types.h"
#include "unpack.h"
#include "graf.h"
#include "shape.h"
#include "level.h"
#include "auto.h"
#include "heap.h"
#include "fade.h"
#include "anim.h"
#include "xlatgraf.h"
word BankStatus = 1;

word AllocBanks(byte request)
{
    byte units;
    word selected;
    word bit;

    selected = 0;
    bit = 1;
    units = (request + 15) >> 4;

    while (units--) {
        while (BankStatus & bit) {
            bit <<= 1;
            if (bit == 0)
                return 0;
        }
        selected |= bit;
        bit <<= 1;
    }

    BankStatus |= selected;
    return selected;
}

void ResetBanks(word banks)
{
    word bit;

    BankStatus = 0;
    bit = 1;
    while (banks--) {
        BankStatus |= bit;
        bit <<= 1;
    }
}


typedef union ShapeLayout {
    unsigned short word;
    byte bytes[2];
    struct {
        word bands:7;
        word reserved:1;
        word packing:4;
        word planes:3;
        word indexed:1;
    } bits;
} ShapeLayout;

int CalcRowBytes(const SHAPE far *shape)
{
    ShapeLayout code;
    const ShapeLayout *format;

    code = *(ShapeLayout far *)&shape->format;
    format = &code;

    if (!format->bits.indexed)
        return (shape->width + 7) / 8;
    return ((format->bits.planes + 1) * shape->width + 7) / 8;
}

void MCGAToMCGA(const SHAPE far *shape,
                           byte far *pixels, word banks)
{
    byte map[16];
    int mapped = 0;
    word bit = 1;
    word bank;
    byte far *cursor;
    word row;

    for (bank = 0; bank < 16; ++bank, bit <<= 1) {
        if (banks & bit)
            map[mapped++] = (byte)(bank * 16);
    }

    cursor = pixels;
    for (row = 0; row < shape->height; ++row) {
        word column;
        for (column = 0; column < shape->width; ++column) {
            if (*cursor)
                *cursor = (*cursor & 15) | map[*cursor >> 4];
            ++cursor;
        }
    }
}


int Get1stBank(word banks)
{
    int index = 0;
    word bit = 1;

    do {
        if (banks & bit) {
            return index;
        }
        bit <<= 1;
        ++index;
    } while (index < 16);
    return 0;
}


typedef struct Glyph {
    int height;
    int width;
    int row_bytes;
    byte bits[1];
} Glyph;


Handle convert_font_handle(Handle handle)
{
    word flags;
    FONT far *source;
    void far *replacement;
    Handle temporary;

    flags = hGetState(handle);
    if (!(flags & 0x2000)) {
        source = hLock(handle);
        if (source) {
            temporary = XlatFont(source);
            hSetState(handle, flags);
            replacement = detach_handle_payload(temporary);
            if (replacement && attach_handle_payload(handle, replacement) == 0) {
                mark_handle_converted(handle);
            } else {
                clear_payload_header(replacement);
                handle = 0;
            }
            hFree(temporary);
        }
    }
    return handle;
}


void MapPalInfo(const PALINFO far *palette)
{
    int first_color;
    int bank;

    if (palette->banks != 0) {
        int stored_bank = 0;
        bank = stored_bank;
        first_color = bank;
        do {
            if (palette->banks & (unsigned short)(1 << (unsigned short)bank))
                SetDACBlock(first_color, 16, palette->vga + 16 * stored_bank++, 1);
            ++bank;
            first_color += 16;
        } while (first_color < 256);
    }
}


int unpack_pixels(void far *destination, const void far *source,
                             int length, int stride, int rows, int codec, int wide)
{
    int result;

    if (wide)
        stride *= 4;
    switch (codec) {
    default:
        _fmemcpy(destination, source, length);
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

int halve_for_mode_two(int value, int mode)
{
    if (mode == 2)
        return value % 2 + value / 2;
    return value;
}

Handle XlatFont(FONT far *font)
{
    word count;
    word size, offset;
    Glyph far *glyph;
    Glyph far *output;
    FONT far *translated;
    Handle handle;
    int converted_width;
    word bytes;

    count = font->last - font->first + 1;
    size = count * (2 + 6) + 10;
    {
        word i;
        for (i = 0; i < count; ++i) {
            Glyph far *cell = (Glyph far *)((unsigned long)font + font->glyph_offset[i]);
            size += halve_for_mode_two(cell->width, 1) * cell->height;
        }
    }

    handle = hNew(size);
    if (handle) {
        translated = hLock(handle);
        _fmemcpy(translated, font, 10); offset = (count + 5) * 2;
        {
            word index;
            for (index = 0; index < count; ++index) {
                translated->glyph_offset[index] = offset; glyph = (Glyph far *)((unsigned long)font + font->glyph_offset[index]);
                converted_width = halve_for_mode_two(glyph->width, 1); glyph->row_bytes = (glyph->width + 7) / 8;
                bytes = glyph->row_bytes * glyph->height + 6;
                output = (Glyph far *)((unsigned long)translated + offset);
                _fmemcpy(output, glyph, bytes);
                MaskToMCGA(output, output->bits);
                output->row_bytes = converted_width;
                offset += converted_width * glyph->height + 6;
            }
        }
        hUnlock(handle);
    }
    return handle;
}

word MaxPackSize = 0;


Handle convert_shape(void far *shape, const void far *banks, int mode)
{
    word local_banks[256];

    return XlatShape(shape, copy_far_word(local_banks, banks), mode);
}

Handle XlatShape(SHAPE far *shape, word *banks, int mode)
{
    ShapeLayout layout;
    const ShapeLayout *format;
    int indexed, mask, wide;
    unsigned long input_allocation;
    unsigned long output_allocation;
    unsigned long input_left, output_left;
    word rows_left, band_rows, output_bytes, input_bytes;
    word output_stride;
    Handle handle;
    SHAPE far *output;
    byte far *input_bits;
    byte far *output_bits;

    layout = *(ShapeLayout far *)&shape->format;
    format = &layout;
    if (format->bits.bands)
        return convert_banded_shape(shape, banks);
    indexed = !format->bits.indexed;
    mask = indexed && !format->bits.planes;
    wide = (graphics_mode == 6 || graphics_mode == 4) && indexed && format->bits.planes == 3;
    shape->format = CalcRowBytes(shape);
    input_allocation = (long)(short)shape->format * (short)shape->height;
    if (wide)
        input_allocation <<= 2;
    input_left = input_allocation;
    input_allocation += sizeof(SHAPEHEADER);
    output_stride = halve_for_mode_two(shape->width, mode);
    output_allocation = (1L * (1 + 3 * (!mask * (graphics_mode == 4 || graphics_mode == 6)))) *
                        (short)shape->height * output_stride;
    output_left = output_allocation;
    MaxPackSize = (unsigned long)((long)(short)shape->height * (short)shape->width) > 64100UL
                       ? 58240 : 64100;
    output_allocation += sizeof(SHAPEHEADER);
    handle = hNew(output_allocation < input_allocation ? input_allocation : output_allocation);
    if (handle) {
        output = hLock(handle);
        *(SHAPEHEADER far *)output = *(SHAPEHEADER far *)shape;
        band_rows = MaxPackSize / shape->width < shape->height ? MaxPackSize / shape->width : shape->height;
        rows_left = shape->height;
        input_bytes = band_rows * shape->format;
        if (wide)
            input_bytes <<= 2;
        output_bytes = (1 + 3 * (!mask * (graphics_mode == 4 || graphics_mode == 6))) * output_stride * band_rows;
        input_bits = shape->pixels;
        output_bits = output->pixels;
        while (rows_left) {
            output_bytes = (word)(output_left < output_bytes ? output_left : output_bytes);
            input_bits = NormalizePtr(input_bits);
            output_bits = NormalizePtr(output_bits);
            band_rows = rows_left < band_rows ? rows_left : band_rows;
            output->height = band_rows;
            input_bits += unpack_pixels(output_bits, input_bits,
                                        input_bytes = (word)(input_bytes > input_left ? input_left : input_bytes), shape->format,
                                        band_rows, format->bits.packing, wide);
            if (!indexed && format->bits.planes == 3) {
                if (mode == 1)
                    TgaToMCGA(output, output_bits, Get1stBank(*banks));
                else if (mode == 2) {
                    word pixels = (short)output->width * (short)output->height;
                    _fmemcpy(output->pixels, output_bits, (pixels & 1) + (pixels >> 1));
                }
            } else if (mask) {
                MaskToMCGA((Glyph far *)output, output_bits);
            } else if (!indexed && format->bits.planes == 7) {
                MCGAToMCGA(output, output_bits, *banks);
            } else {
                goto discard;
            }
            input_left -= input_bytes;
            output_left -= output_bytes;
            output_bits += output_bytes;
            rows_left -= band_rows;
        }
        goto finish;

    discard:
        hFree(handle);
        handle = 0;
        goto restore_format;

    finish:
        output->height = shape->height;
        output->format = output_stride;
        hSetSize(hUnlock(handle), output_allocation);
    }

restore_format:
    *(ShapeLayout far *)&shape->format = layout;
    return handle;
}

Handle convert_banded_shape(SHAPE far *shape, word *banks)
{
    ShapeLayout layout;
    const ShapeLayout *format;
    int indexed, mask, wide;
    word output_stride, band_rows, rows_left;
    word input_bytes, output_bytes;
    unsigned long input_allocation, input_left, output_size, output_left;
    Handle handle;
    SHAPE far *output;
    byte far *input_bits;
    byte far *output_bits;

    layout = *(ShapeLayout far *)&shape->format;
    format = &layout;
    indexed = !format->bits.indexed;
    mask = indexed && !format->bits.planes;
    wide = (graphics_mode == 6 || graphics_mode == 4) && indexed && format->bits.planes == 3;
    shape->format = CalcRowBytes(shape);
    input_allocation = *(word far *)shape->pixels;
    if (wide)
        input_allocation <<= 2;
    input_left = input_allocation;
    input_allocation += sizeof(SHAPEHEADER);
    output_stride = halve_for_mode_two(shape->width, 1);
    output_left = output_size = 0;
    MaxPackSize = (unsigned long)((long)(short)shape->width * (short)shape->height) > 64100UL
                       ? 58240 : 64100;
    if (mask)
        output_size += 6;                  /* glyph header */
    else
        output_size += sizeof(SHAPEHEADER);
    handle = hNew(output_size < input_allocation ? input_allocation : output_size);
    if (handle) {
        output = hLock(handle);
        *(SHAPEHEADER far *)output = *(SHAPEHEADER far *)shape;
        band_rows = 58238L / (long)(short)((shape->width + 1) * 2);
        band_rows = band_rows < shape->height ? band_rows : shape->height;
        rows_left = shape->height;
        input_bytes = band_rows * shape->format;
        if (wide)
            input_bytes <<= 2;
        output_bytes = (1 + 3 * (!mask * (graphics_mode == 4 || graphics_mode == 6))) * output_stride * band_rows;
        input_bits = shape->pixels;
        while (rows_left) {
            band_rows = rows_left < band_rows ? rows_left : band_rows;
            input_bytes = output_bytes = *(word far *)input_bits;
            input_bits += 2;
            hUnlock(handle);
            if (hSetSize(handle, output_bytes + output_size))
                goto discard;
            output = hLock(handle);
            output_bits = (byte far *)output + output_size;
            output_size += output_bytes;
            input_bits = NormalizePtr(input_bits);
            output_bits = NormalizePtr(output_bits);
            output->height = band_rows;
            input_bits += unpack_pixels(output_bits, input_bits, input_bytes, shape->format,
                                        band_rows, format->bits.packing, wide);
            if (!indexed && format->bits.planes == 3)
                TgaToMCGA(output, output_bits, Get1stBank(*banks));
            else if (mask)
                MaskToMCGA((Glyph far *)output, output_bits);
            else if (graphics_mode == 5 && !indexed && format->bits.planes == 7)
                remap_banded_pixels(output, output_bits, *banks);
            else if (!((graphics_mode == 6 || graphics_mode == 4) && indexed && format->bits.planes == 3))
                goto discard;
            input_left -= input_bytes;
            output_left -= output_bytes;
            output_bits += output_bytes;
            rows_left -= band_rows;
        }
        output->height = shape->height;
        output->format = output_stride;
        hSetSize(hUnlock(handle), output_size);
    }
    goto restore_format;

discard:
    hFree(handle);
    handle = 0;
restore_format:
    *(ShapeLayout far *)&shape->format = layout;
    return handle;
}

void remap_banded_pixels(SHAPE far *shape, byte far *pixels, word banks)
{
    byte map[16];
    int mapped = 0;
    word bit = 1;
    word bank;
    word row;
    byte far *cursor;

    for (bank = 0; bank < 16; ++bank, bit <<= 1) {
        if (banks & bit)
            map[mapped++] = (byte)(bank * 16);
    }

    cursor = pixels;
    for (row = 0; row < shape->height; ++row) {
        word column;
        byte count;
        cursor += 2;
        for (column = 0; column < shape->width; column += count) {
            byte run = *cursor++;
            count = (run & 0x7f) + 1;
            if ((sbyte)run < 0) {
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

word *copy_far_word(word *out, const word far *in)
{
    *out = *in;
    return out;
}
