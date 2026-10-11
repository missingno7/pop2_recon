int far pascal IS_SIX_KIND(char kind)
{
    if (kind == 0 || kind == 9 || kind == 33 || kind == 35 || kind == 27 || kind == 37)
        return 1;
    return 0;
}
