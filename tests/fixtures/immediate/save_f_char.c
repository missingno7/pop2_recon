struct Record { char bytes[20]; };
extern struct Record saved, current;
void SaveFChar(void) { saved = current; }
