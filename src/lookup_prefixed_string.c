#include "types.h"
#include "resc.h"
#include "util.h"


sbyte far *lookup_prefixed_string(byte far *strings, byte index)
{
    if (strings && index && index <= *strings)
        return skip_nul_strings((const char far *)(strings + 1), index - 1);
    return 0;
}
