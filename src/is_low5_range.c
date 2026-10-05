int far pascal IS_LOW5_RANGE(unsigned char value)
{
    unsigned int low5 = (unsigned int)value & 31U;
    return low5 >= 3U && low5 <= 15U;
}