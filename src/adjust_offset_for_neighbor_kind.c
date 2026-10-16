extern volatile unsigned char KIND_SAME_ROW;
extern volatile unsigned char KIND_ROW_ABOVE;
extern volatile unsigned char KIND_ROW_ABOVE_ALT;
extern volatile unsigned char KIND_ROW_BELOW;
extern volatile unsigned char KIND_ROW_BELOW_ALT;

int far pascal ADJUST_OFFSET_FOR_NEIGHBOR_KIND(unsigned char kind, int value)
{
    /* The first matching kind leaves the signed offset unchanged. */
    if (KIND_SAME_ROW == kind) {
        /* unchanged */
    } else if (KIND_ROW_ABOVE == kind || KIND_ROW_ABOVE_ALT == kind) {
        value -= 320;
    } else if (KIND_ROW_BELOW == kind || KIND_ROW_BELOW_ALT == kind) {
        value += 320;
    }
    return value;
}