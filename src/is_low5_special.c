int far pascal IS_LOW5_SPECIAL(unsigned char value)
{
    unsigned int low5 = (unsigned int)value & 31U;

    if (low5 == 2U || (low5 >= 16U && low5 <= 17U)) {
        low5 = 1U;
    } else {
        low5 = 0U;
    }

    return low5;
}
