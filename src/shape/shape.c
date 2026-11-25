#pragma pack(1)

typedef unsigned int Handle;

typedef struct ShapeInfo {
    signed char mode;
    unsigned char temporary;
    unsigned char four_bit;
    unsigned char flags;
    Handle image;
} ShapeInfo;

typedef struct ShapeList {
    Handle resource;
    int id;
    int count;
    ShapeInfo items[1];
} ShapeList;

typedef struct PalInfo {
    unsigned int banks;                 /* allocated palette banks, 0 before AllocBanks */
    unsigned char colors;
    unsigned char vga[16][3];
    unsigned char cga[16];
    unsigned char ega[32];
} PalInfo;

typedef struct ShapeListResource {
    int first_id;
    int count;
    PalInfo palette;
} ShapeListResource;

typedef struct ShapeEncoding {
    unsigned int other : 12;
    signed int depth : 3;
    unsigned int encoded : 1;
} ShapeEncoding;

typedef struct ShapeImage {
    int width;
    int height;
    unsigned int format;
    unsigned char pixels[1];
} ShapeImage;

extern void hFree(Handle handle);
extern Handle hGetShpList(int id);
typedef void *NearPointer;
typedef void far *FarPointer;
extern NearPointer cdecl memcpy(void *dest, const void *src, unsigned size);
#pragma intrinsic(memcpy)
extern NearPointer cdecl _nmalloc(unsigned int size);
extern int cdecl sprintf(char *buffer, const char *format, ...);
extern void abort_with_message(int fatal, const char far *text);
extern int hGetState(Handle handle);
extern void far *hLock(Handle handle);
extern Handle AllocBanks(unsigned char id);
extern void MapPalInfo(unsigned long address);
extern void hSetState(Handle handle, int flags);
extern void far *h2p(Handle handle);
extern void RELOCATE_HANDLE_PAYLOAD(Handle handle);
extern NearPointer cdecl memset(void *dst, int value, unsigned int size);
#pragma intrinsic(memset)
extern FarPointer cdecl _fmemset(void far *dst, int value, unsigned int size);
extern Handle HGETRSRC(unsigned long type, int id);
extern Handle hNoPurge(Handle handle);
extern void hPurge(Handle handle);
extern void far *MATERIALIZE_HANDLE(Handle handle);
extern Handle convert_shape(const void far *packed, const void far *mapping, int mode);
extern void Paint4BitShape(const void far *image, int x, int y, int mode, int page);
extern void PaintRLEShape_asm(const void far *image, int x, int y, int page);
extern void PAINT_INDEXED_SHAPE(const void far *image, int x, int y, int page, int mode);
extern void PaintShape(const void far *image, int x, int y, int page);
extern void hFreeShpList(Handle handle);
extern void cdecl _nfree(void *pointer);
extern Handle hNewShpItem1(int group, int item, int mode);
extern void MirrorMCGABlock(unsigned char far *, int, int);

extern unsigned int BankStatus;

static PalInfo far default_palette = {
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

ShapeInfo far *PrinceNewShpItem(Handle, int, ShapeInfo far *, int);
void PrinceFreeShp(int);
ShapeInfo far *PrinceGetnShapeItem(ShapeList *, int, ShapeInfo far *);

void FreeShpInfo(ShapeInfo far *info)
{
    if (info->mode && info->temporary) {
        hFree(info->image);
        info->image = 0;
        info->mode = 0;
    }
}

Handle hGetNMapShpList(int id, Handle mapping, int prepare)
{
    Handle handle;
    int flags;
    ShapeListResource far *resource;

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
            Handle far *palette = &resource->palette.banks;
            unsigned long address = (unsigned long)palette;
            MapPalInfo(address);
        }
    }
    hSetState(handle, flags);
    return handle;
}

int IsOverlayLoaded(ShapeList *list, int index)
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

ShapeList *LoadShapeList(int id, Handle mapping, signed char *mode, int prepare)
{
    int index;
    Handle image;
    ShapeInfo *entry;
    ShapeList *list;
    int flags;
    ShapeListResource far *resource;
    Handle handle;

    handle = hGetNMapShpList(id, mapping, prepare);
    flags = hGetState(handle);
    RELOCATE_HANDLE_PAYLOAD(handle);
    resource = hLock(handle);
    list = _nmalloc(sizeof(ShapeInfo) * (resource->count + 1));
    list->resource = handle;
    list->count = resource->count;
    list->id = id;
    hSetState(handle, flags);
    _fmemset(list->items, 0, list->count * sizeof(ShapeInfo));

    index = 0;
    if (index < list->count) {
        entry = list->items;
        do {
            int item_mode;
            if (!((unsigned)mode & 0xff00U) &&
                ((unsigned char)(unsigned)mode == 1 || (unsigned char)(unsigned)mode == 2))
                item_mode = (int)mode;
            else
                item_mode = mode && mode[index] ? mode[index] : 0;
            if (item_mode) {
                PrinceNewShpItem(list->resource, index + 1, entry, item_mode);
                PrinceFreeShp(id + index + 1);
            } else {
                image = hNoPurge(HGETRSRC(0x53484150UL, list->id + index + 1));
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

void OverlayShapeList(ShapeList *list, int id, int first, int last, signed char *mode)
{
    int base_id = *(int far *)h2p(list->resource = hGetShpList(list->id));
    int index;

    id += 2;
    for (index = first; index <= last; ++index) {
        int item_mode;
        if (!((unsigned)mode & 0xff00U) &&
            ((unsigned char)(unsigned)mode == 1 || (unsigned char)(unsigned)mode == 2))
            item_mode = (int)mode;
        else
            item_mode = mode && mode[index] ? mode[index] : 0;
        if (item_mode) {
            if (list->items[index].image) {
                hFree(list->items[index].image);
                list->items[index].image = 0;
            }
            PrinceNewShpItem(list->resource, index - base_id + id,
                             &list->items[index], item_mode);
            PrinceFreeShp(index + id - 1);
        } else {
            memset(&list->items[index], 0, sizeof(ShapeInfo));
            list->items[index].image = hNoPurge(HGETRSRC(0x53484150UL, index + id - 1));
        }
    }
}

void PrinceMarkImgsPurgeable(ShapeList *list, int first, int last)
{
    int index;
    if (list->count > last && last >= first) {
        for (index = first; index <= last; ++index) {
            if (list->items[index].image)
                hPurge(list->items[index].image);
        }
    }
}

void PrinceMarkRangesPurgeable(ShapeList *list, int *first, int *last)
{
    int *begin, *end;
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

ShapeInfo far *PrinceNewShpItem(Handle list, int index, ShapeInfo far *info, int mode)
{
    char error[80];
    ShapeListResource far *resource;
    ShapeImage far *image;
    unsigned int format;
    const PalInfo far *palette;
    int list_flags;
    int image_flags;
    ShapeEncoding *format_bytes;
    Handle handle;
    int reused;

    list_flags = hGetState(list);
    resource = hLock(list);

    if (info->image && MATERIALIZE_HANDLE(info->image)) {
        handle = info->image;
        reused = 1;
    } else {
        if (info->image) hFree(info->image);
        handle = HGETRSRC(0x53484150UL, resource->first_id + index - 1);
        reused = 0;
    }
    if (!handle) {
        sprintf(error, shape_errors.missing_item, resource->first_id, index, reused);
        abort_with_message(1, error);
    }

    image_flags = hGetState(handle);
    image = hLock(handle);
    if (image->width) {
        format = image->format;
        format_bytes = (ShapeEncoding *)&format;
        if (!format_bytes->depth && !format_bytes->encoded)
            info->four_bit = 1;
        else
            info->four_bit = 0;
        palette = info->four_bit ? &default_palette : &resource->palette;
        info->image = convert_shape(image, palette, mode);
        info->mode = (signed char)mode;
        info->flags = info->temporary = 0;
        hSetState(handle, image_flags);
    } else {
        hSetState(handle, image_flags);
        hFree(handle);
        info->image = 0;
        info->mode = 0;
        info->flags = 0;
    }
    hSetState(list, list_flags);
    return info;
}

void PrincePaintShapeIdx(ShapeList *list, int index, int x, int y, int page, int mode)
{
    ShapeInfo info;
    ShapeImage far *image;

    PrinceGetnShapeItem(list, index + 1, &info);
    if (info.mode) {
        image = h2p(info.image);
        if (info.four_bit)
            Paint4BitShape(image, x, y, 0, page);
        else {
            if (mode == 2)
                PaintRLEShape_asm(image, x, y, page);
            if (mode == 3)
                PAINT_INDEXED_SHAPE(image, x, y, page, 0x2000);
            else
                PaintShape(image, x, y, page);
        }
    }
    FreeShpInfo(&info);
}

ShapeInfo far *PrinceGetnShapeItem(ShapeList *list, int index, ShapeInfo far *result)
{
    ShapeInfo *entry = (ShapeInfo *)list + index;

    *result = *entry;
    if (!entry->mode) {
        if (!list->resource || !MATERIALIZE_HANDLE(list->resource)) {
            if (list->resource)
                hFree(list->resource);
            list->resource = hGetShpList(list->id);
        }
        PrinceNewShpItem(list->resource, index, result, 1);
        result->temporary = 1;
        result->flags = 0;
    }
    return result;
}

void far *PrinceGetnShapeItemSize(ShapeInfo *items, int index, unsigned long far *size)
{
    unsigned long far *image;

    if (items[index].image && (image = h2p(items[index].image)) != 0)
        *size = *image;
    else
        size = 0;
    return size;
}

void PrinceFreeShp(int id)
{
    Handle handle = HGETRSRC(0x53484150UL, id);
    if (handle)
        hFree(handle);
}

Handle PrinceUnpackShape(Handle handle, int mode, const void far *mapping)
{
    Handle result;

    if (handle) {
        RELOCATE_HANDLE_PAYLOAD(handle);
        result = convert_shape(hLock(handle),
                                   (const unsigned char far *)mapping + 4, mode);
        hFree(handle);
    } else {
        char message[60];
        memcpy(message, "Error in PrinceUnpackShape().", 30);
        abort_with_message(1, message);
    }
    return result;
}

void UnloadOverlay(ShapeList *list, int first, int last)
{
    int index;
    int far *data;

    if (IsOverlayLoaded(list, first)) {
        for (index = first; index <= last; ++index) {
            if (list->items[index].image &&
                (data = h2p(list->items[index].image)) && *data) {
                hFree(list->items[index].image);
                memset(&list->items[index], 0, sizeof(ShapeInfo));
            }
        }
    }
}

void UnloadShapeList(ShapeList *list)
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

void PaintRLEShapeItem(int group, int item, int x, int y, int page)
{
    Handle handle = hNewShpItem1(group, item, 1);
    PaintRLEShape_asm(h2p(handle), x, y, page);
    hFree(handle);
}

void PaintShapeItem(int group, int item, int x, int y, int page, int reverse)
{
    Handle handle = hNewShpItem1(group, item, 1);
    ShapeImage far *image;

    if (handle) {
        image = h2p(handle);
        if (reverse)
            MirrorMCGABlock(image->pixels, image->width, image->format);
        PaintShape(image, x, y, page);
    }
    hFree(handle);
}

void Paint4BitShapeItem(int group, int item, int x, int y, int page, int mode)
{
    Handle handle = hNewShpItem1(group, item, 2);
    PAINT_INDEXED_SHAPE(h2p(handle), x, y, page, mode);
    hFree(handle);
}
