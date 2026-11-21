/* This flag is set by the DOS sound-driver initialization code. */
extern int midi_driver_flag;

int far pascal get_midi_driver_flag(void)
{
    return midi_driver_flag;
}
