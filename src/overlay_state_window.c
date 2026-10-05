extern unsigned char STATE_BYTE;
extern signed int POSITION_WORD;
extern unsigned char STATE_KIND;
extern int STATE_CODE;

int far pascal OVERLAY_STATE_WINDOW(void)
{
    int result;

    if (STATE_BYTE == 19 && POSITION_WORD < 208 &&
        (STATE_KIND == 2 || STATE_KIND == 6 || STATE_CODE == 80))
        result = 1;
    else
        result = 0;
    return result;
}
