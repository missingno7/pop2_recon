extern unsigned char STATE_BYTE;

void far pascal SetOppMaxLifePts(register unsigned char *p)
{
    if (p[16] == 0)
        STATE_BYTE = 3;
    else
        STATE_BYTE = p[16];
}
