void far pascal DECREMENT_RECORD_INDEX(char near *record)
{
    if (record[1] != 0) {
        --record[1];
        return;
    }

    record[1] = 3;
}
