extern unsigned char STATE_A;
extern unsigned char STATE_B;

void far pascal set_overlay_state_bytes(unsigned char value)
{
    STATE_A = value;
    STATE_B = value;
}
