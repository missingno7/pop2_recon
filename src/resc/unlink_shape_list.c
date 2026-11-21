typedef unsigned Handle;

typedef struct ShapeListBanks {
    unsigned first_id;
    int count;
    unsigned bank_bits;
    unsigned char requested_units;
} ShapeListBanks;

extern unsigned int far pascal save_handle_flags(Handle handle);
extern ShapeListBanks far *far pascal fetch_locked_shape_list(Handle handle);
extern Handle far pascal restore_handle_flags(Handle handle, unsigned int flags);
extern unsigned int occupied_banks;

Handle far pascal unlink_shape_list(Handle list)
{
    unsigned int saved;
    ShapeListBanks far *shape;

    saved = save_handle_flags(list);
    shape = fetch_locked_shape_list(list);
    if (shape && shape->bank_bits != 0) {
        occupied_banks &= ~shape->bank_bits;
        shape->bank_bits = 0;
    }
    return restore_handle_flags(list, saved);
}
