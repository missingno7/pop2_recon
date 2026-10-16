/* Restrict the step value, then request mode 105 unless already pending. */
extern unsigned int RESTRICTED_STEP;
extern void far pascal REQUEST_IF_UNSET(unsigned int value);

void far pascal SET_TIMER_AND_REQUEST(void)
{
    RESTRICTED_STEP = 228;
    REQUEST_IF_UNSET(105);
}
