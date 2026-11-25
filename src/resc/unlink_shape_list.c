#include "types.h"
#include "resc.h"
#include "graf.h"
#include "shape.h"
#include "level.h"
#include "heap.h"
#include "fade.h"
#include "anim.h"
#include "xlatgraf.h"


Handle unlink_shape_list(Handle list)
{
    word saved;
    SHAPELISTRSRC far *shape;

    saved = hGetState(list);
    shape = hLock(list);
    if (shape && shape->palette.banks != 0) {
        BankStatus &= ~shape->palette.banks;
        shape->palette.banks = 0;
    }
    return hSetState(list, saved);
}
