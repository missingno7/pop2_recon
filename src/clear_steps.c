/* Names, signedness and qualifiers are reconstruction hypotheses. */
extern signed char horizontal_step;
extern signed char vertical_step;

void far pascal clear_steps(void)
{
    register signed char zero = 0;
    horizontal_step = zero;
    vertical_step = zero;
}
