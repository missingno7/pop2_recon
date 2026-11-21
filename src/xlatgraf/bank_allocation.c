extern volatile unsigned int ALLOCATED_UNIT_BITS;

unsigned int far pascal AllocBanks(unsigned char request)
{
    unsigned char units;
    register unsigned int selected;
    register unsigned int bit;

    selected = 0;
    bit = 1;
    units = (request + 15) >> 4;

    while (units--) {
        while (ALLOCATED_UNIT_BITS & bit) {
            bit <<= 1;
            if (bit == 0)
                return 0;
        }
        selected |= bit;
        bit <<= 1;
    }

    ALLOCATED_UNIT_BITS |= selected;
    return selected;
}

void far pascal ResetBanks(unsigned int banks)
{
    register unsigned int bit;

    ALLOCATED_UNIT_BITS = 0;
    bit = 1;
    while (banks--) {
        ALLOCATED_UNIT_BITS |= bit;
        bit <<= 1;
    }
}
