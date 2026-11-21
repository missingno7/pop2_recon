extern volatile int state_word;

int far pascal IsTheStrikePos(char kind, int value)
{
    int result;

    if (kind == 10) {
        if (value == 224)
            result = 1;
        else
            result = 0;
    } else if (kind == 11) {
        if (state_word == 173 && value == 280)
            result = 1;
        else
            result = 0;
    } else {
        result = value == 154;
        if (!result && kind == 0)
            result = value == 245;
    }
    return result;
}
