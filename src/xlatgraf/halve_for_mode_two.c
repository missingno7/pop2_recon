int far pascal halve_for_mode_two(int value, int mode)
{
    if (mode == 2)
        return value % 2 + value / 2;
    return value;
}
