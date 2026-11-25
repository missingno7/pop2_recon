/* UTIL */

extern Rect NullRect;

Rect far * AlterRect(register Rect far *destination, const Rect far *source, int left, int top, int right, int bottom);
unsigned long expand_memory_limits(void);
Handle hGetTxt4(int id);
Handle hXlatTxt4(Handle handle);
sbyte far *lookup_prefixed_string(byte far *strings, byte index);
word Rnd(word maximum);
void SetRndSeed(word seed);
void SetScriptRect(int resource, const Rect far *bounds);
void wait_for_action(int action);
