typedef unsigned Handle;

typedef struct ShapeList {
    unsigned first_id;
    int count;
} ShapeList;

extern ShapeList far *far pascal resolve_handle_to_far_payload(Handle list);
extern void far pascal release_tagged_resource(unsigned long tag, unsigned id);
extern Handle far pascal unlink_shape_list(Handle list);
extern void far pascal dispose_handle(Handle handle);
extern Handle far pascal hGetRsrc(unsigned long type, unsigned id);
extern Handle far pascal mark_fixed(Handle handle);

void far pascal release_shape_list(Handle list)
{
    register int remaining;

    remaining = resolve_handle_to_far_payload(list)->count;
    while (remaining--)
        release_tagged_resource(0x53484150UL,
                                resolve_handle_to_far_payload(list)->first_id + remaining);
    dispose_handle(unlink_shape_list(list));
}

Handle far pascal find_shape_list(unsigned id)
{
    return mark_fixed(hGetRsrc(0x5348504cUL, id));
}


typedef struct ShapeListBanks {
    unsigned first_id;
    int count;
    unsigned bank_bits;
    unsigned char requested_units;
} ShapeListBanks;

extern unsigned int far pascal AllocBanks(unsigned char units);
extern unsigned int far pascal MapPalInfo(unsigned far *bank_bits);
extern unsigned int far pascal save_handle_flags(Handle handle);
extern ShapeListBanks far *far pascal fetch_locked_shape_list(Handle handle);
extern Handle far pascal restore_handle_flags(Handle handle, unsigned int flags);
extern unsigned int occupied_banks;

Handle far pascal load_shape_list_banks(Handle list)
{
    ShapeListBanks far *shape;
    unsigned int saved;
    unsigned int banks;

    saved = save_handle_flags(list);
    shape = fetch_locked_shape_list(list);
    if (shape) {
        if (shape->bank_bits == 0 && (banks = AllocBanks(shape->requested_units)) != 0) {
            shape->bank_bits = banks;
            occupied_banks |= banks;
            MapPalInfo(&shape->bank_bits);
            mark_fixed(restore_handle_flags(list, saved));
        } else
            restore_handle_flags(list, saved);
    }
    return list;
}

