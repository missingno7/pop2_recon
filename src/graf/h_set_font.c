#include "types.h"
#include "graf.h"


Handle hSetFont(Handle font)
{
    Handle old = Graf->font;
    Graf->font = font;
    return old;
}
