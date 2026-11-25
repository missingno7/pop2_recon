#include "types.h"
#include "resc.h"
#include "graf.h"
#include "shape.h"
#include "level.h"
#include "heap.h"
#include "fade.h"
#include "anim.h"
#include "xlatgraf.h"


void hFreeShpList(Handle list)
{
    register int remaining;

    remaining = ((SHAPELISTRSRC far *)h2p(list))->count;
    while (remaining--)
        release_tagged_resource(0x53484150UL,
                                ((SHAPELISTRSRC far *)h2p(list))->first_id + remaining);
    hFree(unlink_shape_list(list));
}

Handle hGetShpList(int id)
{
    return hNoPurge(hGetRsrc(0x5348504cUL, id));
}


Handle hMapShpList(Handle list)
{
    SHAPELISTRSRC far *shape;
    word saved;
    word banks;

    saved = hGetState(list);
    shape = hLock(list);
    if (shape) {
        if (shape->palette.banks == 0 && (banks = AllocBanks(shape->palette.colors)) != 0) {
            shape->palette.banks = banks;
            BankStatus |= banks;
            MapPalInfo(&shape->palette);
            hNoPurge(hSetState(list, saved));
        } else
            hSetState(list, saved);
    }
    return list;
}

