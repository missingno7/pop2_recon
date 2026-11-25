typedef unsigned Handle;

typedef struct ShapeListBanks {
    unsigned first_id;
    int count;
    unsigned bank_bits;
    unsigned char requested_units;
} ShapeListBanks;

extern unsigned int far pascal hGetState(Handle handle);
extern ShapeListBanks far *far pascal hLock(Handle handle);
extern Handle far pascal hSetState(Handle handle, unsigned int flags);
extern unsigned int BankStatus;

Handle far pascal unlink_shape_list(Handle list)
{
    unsigned int saved;
    ShapeListBanks far *shape;

    saved = hGetState(list);
    shape = hLock(list);
    if (shape && shape->bank_bits != 0) {
        BankStatus &= ~shape->bank_bits;
        shape->bank_bits = 0;
    }
    return hSetState(list, saved);
}
