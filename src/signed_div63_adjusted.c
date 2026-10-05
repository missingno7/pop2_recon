char far pascal SIGNED_DIV63_ADJUSTED(int value)
{
    int shifted = value - 3;
    char result = shifted / 63;

    if (shifted <= 0)
        --result;

    return result;
}
