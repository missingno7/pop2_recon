int far pascal IsMobFloor(char kind)
{
    if (kind == 11 || kind == 26 || kind == 15)
        return 1;
    return 0;
}
