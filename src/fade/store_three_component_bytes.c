void far pascal STORE_THREE_COMPONENT_BYTES(
    unsigned char far *p,
    unsigned char a,
    unsigned char b,
    unsigned char c)
{
    p[0] = a;
    p[1] = b;
    p[2] = c;
}
