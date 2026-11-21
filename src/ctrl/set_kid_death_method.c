extern volatile unsigned char STATE_KIND;
extern volatile unsigned char STATE_ARGUMENT;

void far pascal SetKidDeathMethod(unsigned char value)
{
    if (STATE_KIND == 0 || STATE_KIND == 1)
        STATE_ARGUMENT = value;
}
