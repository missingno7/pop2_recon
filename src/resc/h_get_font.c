#include "types.h"
#include "resc.h"
#include "graf.h"
#include "shape.h"
#include "level.h"
#include "fade.h"
#include "anim.h"
#include "xlatgraf.h"


Handle hGetFont(int id)
{
    return convert_font_handle(hGetRsrc(0x464f4e54UL, id));
}
