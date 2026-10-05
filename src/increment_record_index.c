void far pascal increment_record_index(char near *p)
{
    if (p[1] == 3)
        p[1] = 0;
    else
        ++p[1];
}
