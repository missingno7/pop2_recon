extern char state[5];
void *memset(void *, int, unsigned);
#pragma intrinsic(memset)
void InitInput(void) { memset(state, 0, sizeof(state)); }
