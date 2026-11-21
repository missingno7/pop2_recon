extern volatile unsigned int MODE_TIMER;
extern volatile unsigned int MODE_CHANGED;
extern void far pascal REQUEST_IF_UNSET(unsigned int value);

void far pascal ToggleInvertState(void)
{
    if (MODE_TIMER != 0) {
        MODE_TIMER = 0;
        REQUEST_IF_UNSET(154);
    } else {
        MODE_TIMER = 1080;
        REQUEST_IF_UNSET(153);
    }
    MODE_CHANGED = 1;
}
