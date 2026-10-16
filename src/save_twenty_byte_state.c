struct Record { char bytes[20]; };
extern struct Record saved, current;
void save_twenty_byte_state(void) { saved = current; }
