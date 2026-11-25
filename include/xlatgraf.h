/* Change of pixel format when a shape or font is loaded. */
typedef struct {
    byte first, last;                   /* characters */
    byte metrics[8];
    word glyph_offset[1];
} FONT;

void MaskToMCGA(void far *glyph, byte far *bits);
void TgaToMCGA(SHAPE far *shape, byte far *pixels, int bank);

extern word BankStatus;
extern word MaxPackSize;

word AllocBanks(byte request);
int CalcRowBytes(const SHAPE far *shape);
Handle convert_banded_shape(SHAPE far *shape, word *banks);
Handle convert_font_handle(Handle handle);
Handle convert_shape(void far *shape, const void far *banks, int mode);
word *copy_far_word(word *out, const word far *in);
int Get1stBank(word banks);
int halve_for_mode_two(int value, int mode);
void MapPalInfo(const PALINFO far *palette);
void MCGAToMCGA(const SHAPE far *shape, byte far *pixels, word banks);
void remap_banded_pixels(SHAPE far *shape, byte far *pixels, word banks);
void ResetBanks(word banks);
int unpack_pixels(void far *destination, const void far *source, int length, int stride, int rows, int codec, int wide);
Handle XlatFont(FONT far *font);
Handle XlatShape(SHAPE far *shape, word *banks, int mode);
