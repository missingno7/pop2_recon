int far pascal IN_REMAINDER_WINDOW(int first, register int second)
{
    int result;
    first = (first - 144) % 32;
    result = first >= 16 - second && 16 + second >= first;
    return result;
}
