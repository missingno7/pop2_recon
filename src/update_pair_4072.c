extern signed char state_byte_5e31;
extern unsigned char state_byte_628e;
extern unsigned char state_byte_628f;

void far pascal update_pair_4072(void)
{
    char temporary;
    state_byte_5e31 = -state_byte_5e31;
    temporary = state_byte_628e;
    state_byte_628e = state_byte_628f;
    state_byte_628f = temporary;
}
