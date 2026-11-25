#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include "types.h"
#include "resc.h"
#include "graf.h"
#include "shape.h"
#include "level.h"
#include "heap.h"
#include "fade.h"
#include "anim.h"
#include "hnewshpi.h"
#include "intrface.h"
#include "xlatgraf.h"
#pragma pack(1)


typedef struct ShapeEncoding {
    word other : 12;
    signed int depth : 3;
    word encoded : 1;
} ShapeEncoding;


static PALINFO far default_palette = {
    0, 16,
    {
        { 0, 0, 0 }, { 255, 255, 255 }, { 255, 255, 255 }, { 255, 255, 255 },
        { 255, 255, 255 }, { 255, 255, 255 }, { 255, 255, 255 }, { 255, 255, 255 },
        { 255, 255, 255 }, { 255, 255, 255 }, { 255, 255, 255 }, { 255, 255, 255 },
        { 255, 255, 255 }, { 255, 255, 255 }, { 255, 255, 255 }, { 255, 255, 255 }
    }
};

static struct ShapeErrors {
    char missing_list[41];
    char missing_item[65];
} shape_errors = {
    "ERROR: Couldn't find shape list.\012Id = %d",
    "PrinceNewShpItem().  BaseId = %d. item = %d.  usedExisting = %d."
};


void FreeShpInfo(SHAPEINFO far *info)
{
    if (info->mode && info->temporary) {
        hFree(info->image);
        info->image = 0;
        info->mode = 0;
    }
}

Handle hGetNMapShpList(int id, word mapping, int prepare)
{
    Handle handle;
    int flags;
    SHAPELISTRSRC far *resource;

    handle = hGetShpList(id);
    if (!handle) {
        char *message = _nmalloc(80);
        sprintf(message, shape_errors.missing_list, id);
        abort_with_message(0, message);
    }
    flags = hGetState(handle);
    resource = hLock(handle);
    if (!resource->palette.banks) {
        if (!mapping)
            mapping = AllocBanks(resource->palette.colors);
        resource->palette.banks = mapping;
        BankStatus |= mapping;
        if (prepare) {
            MapPalInfo(&resource->palette);
        }
    }
    hSetState(handle, flags);
    return handle;
}

int IsOverlayLoaded(SHAPELIST *list, int index)
{
    int loaded = 0;
    int far *shape;

    if (list->count > index) {
        if (list->items[index].image) {
            shape = h2p(list->items[index].image);
            if (shape && *shape)
                loaded = 1;
        }
    }
    return loaded;
}

SHAPELIST *LoadShapeList(int id, word mapping, int mode, int prepare)
{
    int index;
    Handle image;
    SHAPEINFO *entry;
    SHAPELIST *list;
    int flags;
    SHAPELISTRSRC far *resource;
    Handle handle;

    handle = hGetNMapShpList(id, mapping, prepare);
    flags = hGetState(handle);
    relocate_handle_payload(handle);
    resource = hLock(handle);
    list = _nmalloc(sizeof(SHAPEINFO) * (resource->count + 1));
    list->resource = handle;
    list->count = resource->count;
    list->id = id;
    hSetState(handle, flags);
    _fmemset(list->items, 0, list->count * sizeof(SHAPEINFO));

    index = 0;
    if (index < list->count) {
        entry = list->items;
        do {
            int item_mode;
            if (!(mode & 0xff00) &&
                ((byte)mode == 1 || (byte)mode == 2))
                item_mode = mode;
            else
                item_mode = mode && ((sbyte *)mode)[index] ? ((sbyte *)mode)[index] : 0;
            if (item_mode) {
                PrinceNewShpItem(list->resource, index + 1, entry, item_mode);
                PrinceFreeShp(id + index + 1);
            } else {
                image = hNoPurge(hGetRsrc(0x53484150UL, list->id + index + 1));
                if (image) {
                    if (*(int far *)h2p(image))
                        entry->image = image;
                    else
                        hFree(image);
                }
            }
            ++entry;
            ++index;
        } while (index < list->count);
    }
    return list;
}

void OverlayShapeList(SHAPELIST *list, int id, int first, int last, int mode)
{
    int base_id = *(int far *)h2p(list->resource = hGetShpList(list->id));
    int index;

    id += 2;
    for (index = first; index <= last; ++index) {
        int item_mode;
        if (!(mode & 0xff00) &&
            ((byte)mode == 1 || (byte)mode == 2))
            item_mode = mode;
        else
            item_mode = mode && ((sbyte *)mode)[index] ? ((sbyte *)mode)[index] : 0;
        if (item_mode) {
            if (list->items[index].image) {
                hFree(list->items[index].image);
                list->items[index].image = 0;
            }
            PrinceNewShpItem(list->resource, index - base_id + id,
                             &list->items[index], item_mode);
            PrinceFreeShp(index + id - 1);
        } else {
            memset(&list->items[index], 0, sizeof(SHAPEINFO));
            list->items[index].image = hNoPurge(hGetRsrc(0x53484150UL, index + id - 1));
        }
    }
}

void PrinceMarkImgsPurgeable(SHAPELIST *list, int first, int last)
{
    int index;
    if (list->count > last && last >= first) {
        for (index = first; index <= last; ++index) {
            if (list->items[index].image)
                hPurge(list->items[index].image);
        }
    }
}

void PrinceMarkRangesPurgeable(SHAPELIST *list, const int *first, const int *last)
{
    const int *begin, *end;
    int range = 0; /* Retained in the FM Towns build. */

    if (*first != -1) {
        end = last;
        begin = first;
        do {
            PrinceMarkImgsPurgeable(list, *begin, *end);
            ++end;
            ++begin;
            ++range;
        } while (*begin != -1);
    }
}

SHAPEINFO far *PrinceNewShpItem(Handle list, int index, SHAPEINFO far *info, int mode)
{
    char error[80];
    SHAPELISTRSRC far *resource;
    SHAPE far *image;
    word format;
    const PALINFO far *palette;
    int list_flags;
    int image_flags;
    ShapeEncoding *format_bytes;
    Handle handle;
    int reused;

    list_flags = hGetState(list);
    resource = hLock(list);

    if (info->image && materialize_handle(info->image)) {
        handle = info->image;
        reused = 1;
    } else {
        if (info->image) hFree(info->image);
        handle = hGetRsrc(0x53484150UL, resource->first_id + index - 1);
        reused = 0;
    }
    if (!handle) {
        sprintf(error, shape_errors.missing_item, resource->first_id, index, reused);
        abort_with_message(1, error);
    }

    image_flags = hGetState(handle);
    image = hLock(handle);
    if (image->height) {
        format = image->format;
        format_bytes = (ShapeEncoding *)&format;
        if (!format_bytes->depth && !format_bytes->encoded)
            info->four_bit = 1;
        else
            info->four_bit = 0;
        palette = info->four_bit ? &default_palette : &resource->palette;
        info->image = convert_shape(image, palette, mode);
        info->mode = mode;
        info->mirrored = info->temporary = 0;
        hSetState(handle, image_flags);
    } else {
        hSetState(handle, image_flags);
        hFree(handle);
        info->image = 0;
        info->mode = 0;
        info->mirrored = 0;
    }
    hSetState(list, list_flags);
    return info;
}

void PrincePaintShapeIdx(SHAPELIST *list, int index, int x, int y, int page, int mode)
{
    SHAPEINFO info;
    SHAPE far *image;

    PrinceGetnShapeItem(list, index + 1, &info);
    if (info.mode) {
        image = h2p(info.image);
        if (info.four_bit)
            Paint4BitShape(image, x, y, 0, page);
        else {
            if (mode == 2)
                PaintRLEShape_asm(image, x, y, page);
            if (mode == 3)
                paint_indexed_shape(image, x, y, page, 0x2000);
            else
                PaintShape(image, x, y, page);
        }
    }
    FreeShpInfo(&info);
}

SHAPEINFO far *PrinceGetnShapeItem(SHAPELIST *list, int index, SHAPEINFO far *result)
{
    SHAPEINFO *entry = &list->items[index - 1];

    *result = *entry;
    if (!entry->mode) {
        if (!list->resource || !materialize_handle(list->resource)) {
            if (list->resource)
                hFree(list->resource);
            list->resource = hGetShpList(list->id);
        }
        PrinceNewShpItem(list->resource, index, result, 1);
        result->temporary = 1;
        result->mirrored = 0;
    }
    return result;
}

SHAPESIZE far *PrinceGetnShapeItemSize(SHAPELIST *list, int index, SHAPESIZE far *size)
{
    SHAPESIZE far *image;

    if (list->items[index - 1].image && (image = h2p(list->items[index - 1].image)) != 0)
        *size = *image;
    else
        size = 0;
    return size;
}

void PrinceFreeShp(int id)
{
    Handle handle = hGetRsrc(0x53484150UL, id);
    if (handle)
        hFree(handle);
}

Handle PrinceUnpackShape(Handle handle, int mode, const void far *mapping)
{
    Handle result;

    if (handle) {
        relocate_handle_payload(handle);
        result = convert_shape(hLock(handle),
                                   (const byte far *)mapping + 4, mode);
        hFree(handle);
    } else {
        char message[60];
        memcpy(message, "Error in PrinceUnpackShape().", 30);
        abort_with_message(1, message);
    }
    return result;
}

void UnloadOverlay(SHAPELIST *list, int first, int last)
{
    int index;
    int far *data;

    if (IsOverlayLoaded(list, first)) {
        for (index = first; index <= last; ++index) {
            if (list->items[index].image &&
                (data = h2p(list->items[index].image)) && *data) {
                hFree(list->items[index].image);
                memset(&list->items[index], 0, sizeof(SHAPEINFO));
            }
        }
    }
}

void UnloadShapeList(SHAPELIST *list)
{
    int index = 0;
    Handle handle;
    for (; index < list->count; ++index) {
        if (list->items[index].image) {
            hFree(list->items[index].image);
            list->items[index].image = 0;
        }
    }
    handle = hGetShpList(list->id);
    if (handle)
        hFreeShpList(handle);
    _nfree(list);
}

void PaintRLEShapeItem(Handle shapes, int item, int x, int y, int page)
{
    Handle handle = hNewShpItem1(shapes, item, 1);
    PaintRLEShape_asm(h2p(handle), x, y, page);
    hFree(handle);
}

void PaintShapeItem(Handle shapes, int item, int x, int y, int page, int reverse)
{
    Handle handle = hNewShpItem1(shapes, item, 1);
    SHAPE far *image;

    if (handle) {
        image = h2p(handle);
        if (reverse)
            MirrorMCGABlock(image->pixels, image->height, image->format);
        PaintShape(image, x, y, page);
    }
    hFree(handle);
}

void Paint4BitShapeItem(Handle shapes, int item, int x, int y, int page, int mode)
{
    Handle handle = hNewShpItem1(shapes, item, 2);
    paint_indexed_shape(h2p(handle), x, y, page, mode);
    hFree(handle);
}
