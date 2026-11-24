typedef unsigned Handle;

typedef struct ShapeHeader ShapeHeader;

#pragma pack(1)

typedef struct {
    int first_id;
    int count;
    Handle mapping;
    unsigned char mapping_id;
} ShapeListResource;
#pragma pack()

typedef int (far pascal *InputPoll)(void);

typedef struct {
    unsigned char reserved[0x12];
    InputPoll release_discardable;
} HeapState;

extern HeapState far *heap_state;
extern int SAVE_HANDLE_FLAGS(Handle handle);
extern void far *FETCH_LOCKED_SHAPE_LIST(Handle handle);
extern Handle HGETRSRC(unsigned long type, int id);
extern Handle convert_shape(ShapeHeader far *shape, const unsigned far *banks, int mode);
extern void RESTORE_HANDLE_FLAGS(Handle handle, int flags);
extern void RELOCATE_HANDLE_PAYLOAD(Handle handle);

Handle hNewShpItem(Handle list, int item)
{
    Handle result = 0, shape;
    int list_flags, shape_flags;
    ShapeListResource far *resource;
    InputPoll release_discardable = heap_state->release_discardable;

    heap_state->release_discardable = 0;
    for (;;) {
        list_flags = SAVE_HANDLE_FLAGS(list);
        resource = FETCH_LOCKED_SHAPE_LIST(list);
        if (!resource)
            break;
        shape = HGETRSRC(0x53484150UL, resource->first_id + item - 1);
        if (!shape)
            break;
        shape_flags = SAVE_HANDLE_FLAGS(shape);
        result = convert_shape(FETCH_LOCKED_SHAPE_LIST(shape), &resource->mapping, 1);
        if (result) {
            RESTORE_HANDLE_FLAGS(shape, shape_flags);
            break;
        }
        RESTORE_HANDLE_FLAGS(list, list_flags);
        RESTORE_HANDLE_FLAGS(shape, shape_flags);
        RELOCATE_HANDLE_PAYLOAD(list);
        RELOCATE_HANDLE_PAYLOAD(shape);
        heap_state->release_discardable = release_discardable;
    }
    RESTORE_HANDLE_FLAGS(list, list_flags);
    heap_state->release_discardable = release_discardable;
    return result;
}

Handle hNewShpItem1(Handle list, int item, int mode)
{
    Handle result = 0, shape;
    int list_flags, shape_flags;
    ShapeListResource far *resource;
    InputPoll release_discardable = heap_state->release_discardable;

    heap_state->release_discardable = 0;
    for (;;) {
        list_flags = SAVE_HANDLE_FLAGS(list);
        resource = FETCH_LOCKED_SHAPE_LIST(list);
        if (!resource)
            break;
        shape = HGETRSRC(0x53484150UL, resource->first_id + item - 1);
        if (!shape)
            break;
        shape_flags = SAVE_HANDLE_FLAGS(shape);
        result = convert_shape(FETCH_LOCKED_SHAPE_LIST(shape), &resource->mapping, mode);
        if (result) {
            RESTORE_HANDLE_FLAGS(shape, shape_flags);
            break;
        }
        RESTORE_HANDLE_FLAGS(list, list_flags);
        RESTORE_HANDLE_FLAGS(shape, shape_flags);
        RELOCATE_HANDLE_PAYLOAD(list);
        RELOCATE_HANDLE_PAYLOAD(shape);
        heap_state->release_discardable = release_discardable;
    }
    RESTORE_HANDLE_FLAGS(list, list_flags);
    heap_state->release_discardable = release_discardable;
    return result;
}
