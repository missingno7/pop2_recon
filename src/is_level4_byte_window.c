extern volatile unsigned char STATE_LEVEL;

int far pascal IS_LEVEL4_BYTE_WINDOW(unsigned char value)
{
    int result = 0;

    if (STATE_LEVEL == 4 && value >= 22 && value <= 28 && value != 23) {
        result = 1;
    }
    return result;
}