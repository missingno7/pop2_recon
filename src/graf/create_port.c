#include <malloc.h>
#include "types.h"
#include "graf.h"


PORT *BackPort(PORT *port, const Rect far *rect)
{
    int left_adjustment;
    int rows;
    PORT *previous;

    InitPort(port, rect);
    rows = rect->bottom - rect->top;
    port->map.row_offsets = _nmalloc(rows * sizeof(int));
    if (port->map.row_offsets) {
        OffsetRect(&port->map.bounds, rect, -rect->left, -rect->top);
        left_adjustment = (rect->left - Graf->map.bounds.left) & (pixels_per_byte - 1);
        port->map.bounds.left -= left_adjustment;
        port->map.row_bytes = (pixels_per_byte + (rect->right - rect->left) + left_adjustment - 1) / pixels_per_byte;
        port->map.pixels = allocate_buffer((long)port->map.row_bytes * rows);
        if (!port->map.pixels) {
            _nfree(port->map.row_offsets);
        } else {
            while (rows-- != 0) {
                int row_start = port->map.row_bytes * rows;
                port->map.row_offsets[rows] = row_start;
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
