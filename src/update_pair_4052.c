extern signed char state_byte_5e30;
extern unsigned char state_byte_628c;
extern unsigned char state_byte_628d;

void far pascal update_pair_4052(void)
{
    char temporary;
    state_byte_5e30 = -state_byte_5e30;
    temporary = state_byte_628c;
    state_byte_628c = state_byte_628d;
    state_byte_628d = temporary;
}
