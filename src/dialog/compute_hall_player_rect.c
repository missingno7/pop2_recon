int far * far pascal ComputeHallPlayerRect(int far *p, int index)
{
    p[1] = 25;
    p[3] = 155;
    p[0] = index * 24 + 59;
    p[2] = p[0] + 20;
    return p;
}
