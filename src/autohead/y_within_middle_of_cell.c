int far pascal YWithinMiddleOfCell(int value, register int half_width)
{
    int result;
    value = (value - 3) % 63;
    result = value >= 31 - half_width && 31 + half_width >= value;
    return result;
}