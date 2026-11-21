int * far pascal MAKE_INDEX_RECT(int *p, int index)
{
    if (index >= 3) {
        p[1] = 198;
        index -= 3;
    } else {
        p[1] = 38;
    }
    p[3] = (p[1] + 140 > 310) ? 310 : p[1] + 140;
    p[0] = index * 22 + 40;
    p[2] = p[0] + 20;
    return p;
}
