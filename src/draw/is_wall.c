int far pascal IsWall(char kind)
{
    if (kind == 20 || kind == 2 || kind == 7 || kind == 25)
        return 1;
    return 0;
}
