struct RECORD_WITH_LONG_PAIR
{
    unsigned char prefix[72];
    unsigned long first;
    unsigned long second;
};

void far pascal DissolveSetStepFunc(
    struct RECORD_WITH_LONG_PAIR near *record,
    unsigned long first,
    unsigned long second)
{
    record->first = first;
    record->second = second;
}
