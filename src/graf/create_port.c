typedef struct Rect {
    int top, left, bottom, right;
} Rect;

typedef struct Port {
    void far *buffer;
    Rect bounds;
    int row_bytes;
    int *row_offsets;
} Port;

extern Port *Current_Port;
extern signed char Byte_Alignment;

extern void far pascal Init_Port(Port *port, const Rect far *rect);
extern void far pascal OFFSETRECT(Rect far *out, const Rect far *in, int x, int y);
extern void far *far pascal Allocate_Buffer(long bytes);
extern void far pascal Set_Origin(int x, int y);
extern void far pascal Finish_Port(Port *port);
extern void *far cdecl _nmalloc(unsigned bytes);
extern void far cdecl _nfree(void *memory);

Port *create_port(Port *port, const Rect far *rect)
{
    int left_adjustment;
    int rows;
    Port *previous;

    Init_Port(port, rect);
    rows = rect->bottom - rect->top;
    port->row_offsets = _nmalloc(rows * sizeof(int));
    if (!port->row_offsets)
        goto fail;
    OFFSETRECT(&port->bounds, rect, -rect->left, -rect->top);
    left_adjustment = (rect->left - Current_Port->bounds.left) & (Byte_Alignment - 1);
    port->bounds.left -= left_adjustment;
    port->row_bytes = (Byte_Alignment + (rect->right - rect->left) + left_adjustment - 1) / Byte_Alignment;
    port->buffer = Allocate_Buffer((long)port->row_bytes * rows);
    if (!port->buffer) {
        _nfree(port->row_offsets);
        goto fail;
    }
    while (rows-- != 0) {
        int row_start = port->row_bytes * rows;
        port->row_offsets[rows] = row_start;
    }
    previous = Current_Port;
    Current_Port = port;
    Set_Origin(rect->left, rect->top);
    Finish_Port(port);
    Current_Port = previous;
    return port;

fail:
    return 0;
}
