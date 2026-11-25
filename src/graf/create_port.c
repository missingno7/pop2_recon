#include "types.h"

typedef struct Port {
    void far *buffer;
    Rect bounds;
    int row_bytes;
    int *row_offsets;
} Port;

extern Port *Graf;
extern signed char PIXELS_PER_BYTE;

extern void far pascal InitPort(Port *port, const Rect far *rect);
extern void far pascal OFFSETRECT(Rect far *out, const Rect far *in, int x, int y);
extern void far *far pascal Allocate_Buffer(long bytes);
extern void far pascal SetOrigin(int x, int y);
extern void far pascal Finish_Port(Port *port);
extern void *far cdecl _nmalloc(unsigned bytes);
extern void far cdecl _nfree(void *memory);

Port *BackPort(Port *port, const Rect far *rect)
{
    int left_adjustment;
    int rows;
    Port *previous;

    InitPort(port, rect);
    rows = rect->bottom - rect->top;
    port->row_offsets = _nmalloc(rows * sizeof(int));
    if (port->row_offsets) {
        OFFSETRECT(&port->bounds, rect, -rect->left, -rect->top);
        left_adjustment = (rect->left - Graf->bounds.left) & (PIXELS_PER_BYTE - 1);
        port->bounds.left -= left_adjustment;
        port->row_bytes = (PIXELS_PER_BYTE + (rect->right - rect->left) + left_adjustment - 1) / PIXELS_PER_BYTE;
        port->buffer = Allocate_Buffer((long)port->row_bytes * rows);
        if (!port->buffer) {
            _nfree(port->row_offsets);
        } else {
            while (rows-- != 0) {
                int row_start = port->row_bytes * rows;
                port->row_offsets[rows] = row_start;
            }
            previous = Graf;
            Graf = port;
            SetOrigin(rect->left, rect->top);
            Finish_Port(port);
            Graf = previous;
            return port;
        }
    }
    return 0;
}
