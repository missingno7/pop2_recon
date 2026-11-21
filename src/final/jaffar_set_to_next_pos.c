void far pascal JaffarSetToNextPos(char near *p)
{
    if (p[1] == 3)
        p[1] = 0;
    else
        ++p[1];
}
