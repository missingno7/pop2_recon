#include <malloc.h>
#include "types.h"
#include "graf.h"


void FreePort(PORT *port)
{
    if (port->map.pixels != default_port.map.pixels)
        release_buffer(port->map.pixels);
    if (port->map.row_offsets != default_port.map.row_offsets)
        _nfree(port->map.row_offsets);
}
