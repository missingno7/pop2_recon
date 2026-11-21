
int far pascal Get1stBank(unsigned banks)
{
    register int index = 0;
    register unsigned bit = 1;

    do {
        if (banks & bit) {
            return index;
        }
        bit <<= 1;
        ++index;
    } while (index < 16);
    return 0;
}
