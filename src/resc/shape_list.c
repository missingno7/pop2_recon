typedef unsigned Handle;

typedef struct ShapeList {
    unsigned first_id;
    int count;
} ShapeList;

extern ShapeList far *far pascal h2p(Handle list);
extern void far pascal release_tagged_resource(unsigned long tag, unsigned id);
extern Handle far pascal unlink_shape_list(Handle list);
extern void far pascal hFree(Handle handle);
extern Handle far pascal hGetRsrc(unsigned long type, unsigned id);
extern Handle far pascal hNoPurge(Handle handle);

void far pascal hFreeShpList(Handle list)
{
    register int remaining;

    remaining = h2p(list)->count;
    while (remaining--)
        release_tagged_resource(0x53484150UL,
                                h2p(list)->first_id + remaining);
    hFree(unlink_shape_list(list));
}

Handle far pascal hGetShpList(unsigned id)
{
    return hNoPurge(hGetRsrc(0x5348504cUL, id));
}


typedef struct ShapeListBanks {
    unsigned first_id;
    int count;
    unsigned bank_bits;
    unsigned char requested_units;
} ShapeListBanks;

extern unsigned int far pascal AllocBanks(unsigned char units);
extern unsigned int far pascal MapPalInfo(unsigned far *bank_bits);
extern unsigned int far pascal hGetState(Handle handle);
extern ShapeListBanks far *far pascal hLock(Handle handle);
extern Handle far pascal hSetState(Handle handle, unsigned int flags);
extern unsigned int BankStatus;

Handle far pascal hMapShpList(Handle list)
{
    ShapeListBanks far *shape;
    unsigned int saved;
    unsigned int banks;

    saved = hGetState(list);
    shape = hLock(list);
    if (shape) {
        if (shape->bank_bits == 0 && (banks = AllocBanks(shape->requested_units)) != 0) {
            shape->bank_bits = banks;
            BankStatus |= banks;
            MapPalInfo(&shape->bank_bits);
            hNoPurge(hSetState(list, saved));
        } else
            hSetState(list, saved);
    }
    return list;
}

