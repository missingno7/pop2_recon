extern unsigned char exit_height;

int IsExitOpen(void)
{
    if (exit_height >= 42)
        return 1;
    return 0;
}
