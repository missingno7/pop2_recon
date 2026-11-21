int EraseScreenAfterNIS(int x)
{
    int result;

    if (x == 1 || x == 100 || x == 2 || x == 3 || x == 6)
        result = 0;
    else
        result = 1;

    return result;
}
