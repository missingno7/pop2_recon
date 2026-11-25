#include "types.h"
#include "resc.h"
#include "graf.h"
#include "shape.h"
#include "level.h"
#include "heap.h"
#include "fade.h"
#include "anim.h"
#include "hnewshpi.h"
#include "xlatgraf.h"


Handle hNewShpItem(Handle list, int item)
{
    Handle result = 0, shape;
    int list_flags, shape_flags;
    SHAPELISTRSRC far *resource;
    int (*release_discardable)(void) = heap_state->release_discardable;

    heap_state->release_discardable = 0;
    for (;;) {
        list_flags = hGetState(list);
        resource = hLock(list);
        if (!resource)
            break;
        shape = hGetRsrc(0x53484150UL, resource->first_id + item - 1);
        if (!shape)
            break;
        shape_flags = hGetState(shape);
        result = convert_shape(hLock(shape), &resource->palette.banks, 1);
        if (result) {
            hSetState(shape, shape_flags);
            break;
        }
        hSetState(list, list_flags);
        hSetState(shape, shape_flags);
        relocate_handle_payload(list);
        relocate_handle_payload(shape);
        heap_state->release_discardable = release_discardable;
    }
    hSetState(list, list_flags);
    heap_state->release_discardable = release_discardable;
    return result;
}

Handle hNewShpItem1(Handle list, int item, int mode)
{
    Handle result = 0, shape;
    int list_flags, shape_flags;
    SHAPELISTRSRC far *resource;
    int (*release_discardable)(void) = heap_state->release_discardable;

    heap_state->release_discardable = 0;
    for (;;) {
        list_flags = hGetState(list);
        resource = hLock(list);
        if (!resource)
            break;
        shape = hGetRsrc(0x53484150UL, resource->first_id + item - 1);
        if (!shape)
            break;
        shape_flags = hGetState(shape);
        result = convert_shape(hLock(shape), &resource->palette.banks, mode);
        if (result) {
            hSetState(shape, shape_flags);
            break;
        }
        hSetState(list, list_flags);
        hSetState(shape, shape_flags);
        relocate_handle_payload(list);
        relocate_handle_payload(shape);
        heap_state->release_discardable = release_discardable;
    }
    hSetState(list, list_flags);
    heap_state->release_discardable = release_discardable;
    return result;
}
