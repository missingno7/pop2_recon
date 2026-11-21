/* Semantic names are hypotheses only; direct addresses are established separately. */
extern volatile unsigned char signflag;
extern volatile int accumulator;

int far pascal AddFCharX(int value)
{
    if (signflag)
        value = -value;
    accumulator = accumulator + value;
    return accumulator;
}
