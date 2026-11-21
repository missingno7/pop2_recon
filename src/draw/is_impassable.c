int far pascal IsImpassable(char kind)
{
    if (kind == 11 || kind == 15 || kind == 26 || kind == 12 ||
        kind == 13 || kind == 23 || kind == 24)
        return 1;
    return 0;
}
