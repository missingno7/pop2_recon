/* Increment the signed state byte up to 12, then refresh the step delta. */
extern unsigned char STATE_BYTE;
extern unsigned char STATE_BASE;
extern unsigned char STATE_DELTA;
char far pascal UPDATE_STEP_DELTA(void);

char far pascal INCREMENT_STATE_AND_UPDATE(void)
{
    int next = (signed char)STATE_BYTE + 1;
    STATE_BYTE = (next > 12) ? 12 : next;
    return UPDATE_STEP_DELTA();
}

char far pascal UPDATE_STEP_DELTA(void)
{
    STATE_DELTA = STATE_BYTE - STATE_BASE;
    return STATE_DELTA;
}
