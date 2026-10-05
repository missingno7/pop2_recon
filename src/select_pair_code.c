char far pascal SELECT_PAIR_CODE(char first, char second)
{
    char result = 0;
    if (first == 19)
        result = second == 5 ? 39 : 47;
    else if (first == 16)
        result = second == 1 ? 48 : second == 4 ? 61 : 69;
    return result;
}
