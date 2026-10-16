struct Record {
    unsigned char prefix[32];
    unsigned int word20;
};

void far pascal SET_NEAR_RECORD_WORD20(struct Record *record,
                                      unsigned int value)
{
    record->word20 = value;
}
