int switch_select(unsigned value)
{
    switch (value) {
    case 0: return 17;
    case 1: return 29;
    case 4: return 61;
    case 7: return 113;
    default: return -1;
    }
}
