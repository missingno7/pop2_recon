extern volatile unsigned char STATE_LEVEL;

char far pascal ADJUST_BYTE_FOR_KIND(char kind, char value)
{
    if (STATE_LEVEL == 5 && kind == 7)
        value -= 10;
    else if (STATE_LEVEL == 5 && kind == 12)
        value += 10;
    return value;
}
