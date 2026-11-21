int * far pascal ComputeSymbolPt(int *p, int index)
{
    p[1] = (index % 5) * 42 + 62;
    p[0] = (index / 5) * 42 + 69;
    return p;
}
