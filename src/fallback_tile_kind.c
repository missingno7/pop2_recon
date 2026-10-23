extern unsigned char STATE_MODE;
extern unsigned char STATE_LEVEL;
extern unsigned char STATE_KIND;
extern unsigned char STATE_FLAG;

char far pascal FALLBACK_TILE_KIND(signed char x, signed char y)
{
    char result;
    switch (STATE_MODE) {
    case 2:
        if ((STATE_KIND == 4 && STATE_LEVEL == 13) ||
            (STATE_LEVEL == 13 && STATE_KIND == 13))
            result = 0;
        else
            result = 20;
        break;
    case 4:
        if (!STATE_FLAG && y == -1) {
            result = 0;
            break;
        }
        if (STATE_KIND == 27 && STATE_LEVEL == 6) {
            result = 0;
            break;
        }
        result = 20;
        break;
    case 5:
        result = 0;
        break;
    case 6:
        if (((STATE_KIND == 7 || STATE_KIND == 8) && STATE_MODE == 6) ||
            (STATE_KIND == 3 && STATE_MODE == 6 && (x < 0 || x == 9) && y == 2))
            result = 20;
        else
            result = 0;
        break;
    default:
        result = 20;
        break;
    }
    return result;
}
