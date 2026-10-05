/* Field aliases, original long type, signedness, and purpose are hypotheses. */
extern unsigned long guard_pair;
extern unsigned int guard_other;
extern signed char mode_code, state_code, level_code;

int far pascal is_special_state(void)
{
    int result = 0;
    if (guard_pair == 0L && guard_other == 0) {
        if ((mode_code == 27 && state_code == 6) ||
            (mode_code == 22 && state_code == 10) ||
            (mode_code == 1 && state_code == 14) ||
            (level_code == 9 && state_code == 8))
            result = 1;
    }
    return result;
}
