/* Semantic names are hypotheses; their DS-relative operands are evidenced separately. */
extern volatile unsigned char signflag;
extern volatile int bias;

int far pascal signed_value(int value)
{
    if (signflag)
        value = -value;
    return value + bias;
}
