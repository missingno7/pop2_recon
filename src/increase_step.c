/* Names and qualifiers are reconstruction hypotheses. */
extern signed char state_code;
extern signed char vertical_step;
extern int restricted_step;

void far pascal increase_step(void)
{
    if (state_code == 4 || state_code == 9) {
        if (restricted_step) {
            ++vertical_step;
            if (vertical_step > 4)
                vertical_step = 4;
        } else {
            vertical_step += 3;
            if (vertical_step > 33)
                vertical_step = 33;
        }
    }
}
