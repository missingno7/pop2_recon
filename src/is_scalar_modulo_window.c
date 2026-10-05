int far pascal IS_SCALAR_MODULO_WINDOW(unsigned int value, int index)
{
    int i;
    int result;

    if (index >= 4 && value > 45U && value < 52U) {
        i = index;
        i -= 4;
        value -= 46U;
        if (i < 2) {
            result = (value % (3 - i) == 0);
        } else {
            result = (value + 1U) % 3U;
        }
    } else {
        result = 0;
    }
    return result;
}
