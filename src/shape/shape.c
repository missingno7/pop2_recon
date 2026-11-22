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

typedef struct ShapeListResource {
    int first_id;
    int count;
    Handle mapping;
    unsigned char mapping_id;
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

extern void dispose_handle(Handle handle);
extern Handle find_shape_list(int id);
typedef void *NearPointer;
typedef void far *FarPointer;
extern NearPointer cdecl memcpy(void *dest, const void *src, unsigned size);
#pragma intrinsic(memcpy)
extern NearPointer cdecl malloc(unsigned int size);
extern int cdecl sprintf(char *buffer, const char *format, ...);
extern void show_error(int fatal, const char far *text);
extern int get_handle_flags(Handle handle);
extern void far *get_handle_address(Handle handle);
extern Handle load_mapping(unsigned char id);
extern void prepare_mapping(unsigned long address);
extern void set_handle_flags(Handle handle, int flags);
extern void far *resolve_handle(Handle handle);
extern void lock_handle(Handle handle);
extern NearPointer cdecl memset(void *dst, int value, unsigned int size);
#pragma intrinsic(memset)
extern FarPointer cdecl _fmemset(void far *dst, int value, unsigned int size);
extern Handle load_shape_resource(unsigned long type, int id);
extern Handle mark_fixed(Handle handle);
extern void mark_purgeable(Handle handle);
extern void far *deref_handle(Handle handle);
extern Handle unpack_shape_data(const void far *packed, const void far *mapping, int mode);
extern void paint_four_bit(const void far *image, int x, int y, int mode, int page);
extern void paint_rle(const void far *image, int x, int y, int page);
extern void paint_mode(const void far *image, int x, int y, int page, int mode);
extern void paint_plain(const void far *image, int x, int y, int page);
extern void release_shape_list(Handle handle);
extern void cdecl free(void *pointer);
extern Handle new_shape_item(int group, int item, int mode);
extern void reverse_block_bytes(unsigned char far *pixels, int width, int format);

extern const unsigned char far builtin_mapping[];
extern unsigned int loaded_mapping_flags;

static struct ShapeErrors {
    char missing_list[41];
    char missing_item[65];
} shape_errors = {
    "ERROR: Couldn't find shape list.\012Id = %d",
    "PrinceNewShpItem().  BaseId = %d. item = %d.  usedExisting = %d."
};

ShapeInfo far *PrinceNewShpItem(Handle list, int index, ShapeInfo far *info, int mode);
void PrinceFreeShp(int id);
ShapeInfo far *get_list_item(ShapeList *list, int index, ShapeInfo far *result);

void FreeShpInfo(ShapeInfo far *info)
{
    if (info->mode && info->temporary) {
        dispose_handle(info->image);
        info->image = 0;
        info->mode = 0;
    }
}

Handle hGetNMapShpList(int id, Handle mapping, int prepare)
{
    Handle handle;
    int flags;
    ShapeListResource far *resource;

    handle = find_shape_list(id);
    if (!handle) {
        char *message = malloc(80);
        sprintf(message, shape_errors.missing_list, id);
        show_error(0, message);
    }
    flags = get_handle_flags(handle);
    resource = get_handle_address(handle);
    if (!resource->mapping) {
        if (!mapping)
            mapping = load_mapping(resource->mapping_id);
        resource->mapping = mapping;
        loaded_mapping_flags |= mapping;
        if (prepare) {
            Handle far *palette = &resource->mapping;
            unsigned long address = (unsigned long)palette;
            prepare_mapping(address);
        }
    }
    set_handle_flags(handle, flags);
    return handle;
}

int IsOverlayLoaded(ShapeList *list, register int index)
{
    int loaded = 0;
    int far *shape;

    if (list->count > index) {
        if (list->items[index].image) {
            shape = resolve_handle(list->items[index].image);
            if (shape && *shape)
                loaded = 1;
        }
    }
    return loaded;
}

ShapeList *load_shape_list(register int id, Handle mapping, signed char *mode, int prepare)
{
    Handle image;
    int index;
    ShapeInfo *entry;
    ShapeList *list;
    int flags;
    ShapeListResource far *resource;
    Handle handle;

    handle = hGetNMapShpList(id, mapping, prepare);
    flags = get_handle_flags(handle);
    lock_handle(handle);
    resource = get_handle_address(handle);
    list = malloc(sizeof(ShapeInfo) * (resource->count + 1));
    list->resource = handle;
    list->count = resource->count;
    list->id = id;
    set_handle_flags(handle, flags);
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
                image = mark_fixed(load_shape_resource(0x53484150UL, list->id + index + 1));
                if (image) {
                    if (*(int far *)resolve_handle(image))
                        entry->image = image;
                    else
                        dispose_handle(image);
                }
            }
            ++entry;
            ++index;
        } while (index < list->count);
    }
    return list;
}

void overlay_shape_list(ShapeList *list, int id, int first, int last, signed char *mode)
{
    int base_id = *(int far *)resolve_handle(find_shape_list(list->id));
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
                dispose_handle(list->items[index].image);
                list->items[index].image = 0;
            }
            PrinceNewShpItem(list->resource, id - base_id + index,
                             &list->items[index], item_mode);
            PrinceFreeShp(id + index - 1);
        } else {
            memset(&list->items[index], 0, sizeof(ShapeInfo));
            list->items[index].image = mark_fixed(load_shape_resource(0x53484150UL, id + index - 1));
        }
    }
}

void PrinceMarkImgsPurgeable(ShapeList *list, int first, int last)
{
    int index;
    if (list->count > last && last >= first) {
        for (index = first; index <= last; ++index) {
            if (list->items[index].image)
                mark_purgeable(list->items[index].image);
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
    const void far *mapping;
    int list_flags;
    int image_flags;
    ShapeEncoding *format_bytes;
    Handle handle;
    int reused;

    list_flags = get_handle_flags(list);
    resource = get_handle_address(list);

    if (info->image && deref_handle(info->image)) {
        handle = info->image;
        reused = 1;
    } else {
        if (info->image)
            dispose_handle(info->image);
        handle = load_shape_resource(0x53484150UL, resource->first_id + index - 1);
        reused = 0;
    }
    if (!handle) {
        sprintf(error, shape_errors.missing_item, resource->first_id, index, reused);
        show_error(1, error);
    }

    image_flags = get_handle_flags(handle);
    image = get_handle_address(handle);
    if (image->width) {
        format = image->format;
        format_bytes = (ShapeEncoding *)&format;
        if (!format_bytes->depth && !format_bytes->encoded)
            info->four_bit = 1;
        else
            info->four_bit = 0;
        mapping = info->four_bit ? builtin_mapping : (const unsigned char far *)resource + 4;
        info->image = unpack_shape_data(image, mapping, mode);
        info->mode = (signed char)mode;
        info->flags = info->temporary = 0;
        set_handle_flags(handle, image_flags);
    } else {
        set_handle_flags(handle, image_flags);
        dispose_handle(handle);
        info->image = 0;
        info->mode = 0;
        info->flags = 0;
    }
    set_handle_flags(list, list_flags);
    return info;
}

void PrincePaintShapeIdx(ShapeList *list, int index, int x, int y, int page, int mode)
{
    ShapeInfo info;
    ShapeImage far *image;

    get_list_item(list, index + 1, &info);
    if (info.mode) {
        image = resolve_handle(info.image);
        if (info.four_bit)
            paint_four_bit(image, x, y, 0, page);
        else {
            if (mode == 2)
                paint_rle(image, x, y, page);
            if (mode == 3)
                paint_mode(image, x, y, page, 0x2000);
            else
                paint_plain(image, x, y, page);
        }
    }
    FreeShpInfo(&info);
}

ShapeInfo far *get_list_item(ShapeList *list, int index, ShapeInfo far *result)
{
    ShapeInfo *entry = (ShapeInfo *)list + index;

    *result = *entry;
    if (!entry->mode) {
        PrinceNewShpItem(list->resource, index, result, 1);
        result->temporary = 1;
        result->flags = 0;
    }
    return result;
}

void far *PrinceGetnShapeItemSize(ShapeInfo *items, int index, unsigned long far *size)
{
    unsigned long far *image;

    if (items[index].image && (image = resolve_handle(items[index].image)) != 0)
        *size = *image;
    else
        size = 0;
    return size;
}

void PrinceFreeShp(int id)
{
    Handle handle = load_shape_resource(0x53484150UL, id);
    if (handle)
        dispose_handle(handle);
}

Handle PrinceUnpackShape(Handle handle, int mode, const void far *mapping)
{
    Handle result;

    if (handle) {
        lock_handle(handle);
        result = unpack_shape_data(get_handle_address(handle),
                                   (const unsigned char far *)mapping + 4, mode);
        dispose_handle(handle);
    } else {
        char message[60];
        memcpy(message, "Error in PrinceUnpackShape().", 30);
        show_error(1, message);
    }
    return result;
}

void unload_overlay(ShapeList *list, int first, int last)
{
    int index;
    int far *data;

    if (IsOverlayLoaded(list, first)) {
        for (index = first; index <= last; ++index) {
            if (list->items[index].image &&
                (data = resolve_handle(list->items[index].image)) && *data) {
                dispose_handle(list->items[index].image);
                memset(&list->items[index], 0, sizeof(ShapeInfo));
            }
        }
    }
}

void UnloadShapeList(ShapeList *list)
{
    int index = 0;
    Handle handle;
    Handle *image;

    if (index < list->count) {
        image = &list->items[0].image;
        do {
            if (*image) {
                dispose_handle(*image);
                *image = 0;
            }
            image += 3;
            ++index;
        } while (index < ((volatile ShapeList *)list)->count);
    }
    handle = find_shape_list(list->id);
    if (handle)
        release_shape_list(handle);
    free(list);
}

void PaintRLEShapeItem(int group, int item, int x, int y, int page)
{
    Handle handle = new_shape_item(group, item, 1);
    paint_rle(resolve_handle(handle), x, y, page);
    dispose_handle(handle);
}

void PaintShapeItem(int group, int item, int x, int y, int page, int reverse)
{
    Handle handle = new_shape_item(group, item, 1);
    ShapeImage far *image;

    if (handle) {
        image = resolve_handle(handle);
        if (reverse)
            reverse_block_bytes(image->pixels, image->width, image->format);
        paint_plain(image, x, y, page);
    }
    dispose_handle(handle);
}

void Paint4BitShapeItem(int group, int item, int x, int y, int page, int mode)
{
    Handle handle = new_shape_item(group, item, 2);
    paint_mode(resolve_handle(handle), x, y, page, mode);
    dispose_handle(handle);
}
