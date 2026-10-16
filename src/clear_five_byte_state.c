extern char state[5];
void *memset(void *, int, unsigned);
#pragma intrinsic(memset)
void clear_five_byte_state(void) { memset(state, 0, sizeof(state)); }
