/* Packed data and changes of pixel format. */
byte far *NormalizePtr(byte far *pointer);
char far *resample_sound(char far *to, const char far *from, word count, word rate);
void unpack_delta_bits(char far *to, const char far *from);
int UnpackLZH(void far *to, const void far *from, int length);
int UnpackLZV(void far *to, const void far *from, int length, int row_bytes, int rows);
int UnpackRLEH(void far *to, const void far *from, int length);
int UnpackRLEV(void far *to, const void far *from, int length, int row_bytes, int rows);
char far *XlatTxt4(char far *to, const char far *from);
