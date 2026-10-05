/* Semantic name only; this external names the independently witnessed DS byte. */
extern unsigned char signflag;

int far pascal conditional_negative(int value)
{
    if (!signflag)
        value = -value;
    return value;
}
