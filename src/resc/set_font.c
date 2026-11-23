extern unsigned int far pascal hSetFont(unsigned int attribute);
extern unsigned int far pascal hGetFont(unsigned int attribute);

unsigned int far pascal SetFont(unsigned int attribute)
{
    return hSetFont(hGetFont(attribute));
}
