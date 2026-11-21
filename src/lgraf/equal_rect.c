extern int far cdecl far_memory_compare(void far *first, void far *second, int count);

int far pascal EqualRect(void far *first, void far *second)
{
    return far_memory_compare(first, second, 8) == 0;
}
