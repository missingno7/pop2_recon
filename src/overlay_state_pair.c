extern signed char state_byte;
extern int state_word;

int far pascal overlay_state_pair(void)
{
    int result;
    if (state_byte == 19 && state_word == 59)
        result = 1;
    else
        result = 0;
    return result;
}
