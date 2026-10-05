/* Neutral DS-relative byte aliases; original names and qualifiers are unknown. */
extern unsigned char state_byte_628f;
extern unsigned char state_byte_628e;
extern unsigned char state_byte_628c;
extern unsigned char state_byte_628d;

unsigned char far pascal clear_state_bytes(void)
{
    char zero = 0;
    state_byte_628f = zero;
    state_byte_628e = zero;
    state_byte_628c = zero;
    state_byte_628d = zero;
    return 1;
}
