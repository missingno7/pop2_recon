/* Shapes (images), lists of them in memory, and their palettes. */
#pragma pack(1)
typedef struct {                        /* what precedes the pixels of a shape */
    int height, width;
    word format;
} SHAPEHEADER;

typedef struct {                        /* an image as stored in a SHAP resource */
    int height, width;
    word format;                        /* 8000: packed, 7000: planes - 1, 0fff: bytes in a row */
    byte pixels[1];
} SHAPE;

typedef struct {
    int height, width;
} SHAPESIZE;

typedef struct {                        /* an image in memory */
    sbyte mode;                         /* 0: as stored, else unpacked */
    byte temporary, four_bit, mirrored;
    Handle image;
} SHAPEINFO;

typedef struct {
    Handle resource;
    int id, count;
    SHAPEINFO items[1];
} SHAPELIST;

typedef struct {
    word banks;                         /* allocated palette banks, 0 before AllocBanks */
    byte colors;
    byte vga[16][3];
    byte cga[16];
    byte ega[32];
} PALINFO;

typedef struct {                        /* a SHPL resource */
    int first_id, count;
    PALINFO palette;
} SHAPELISTRSRC;
#pragma pack()

void FreeShpInfo(SHAPEINFO far *info);
Handle hGetNMapShpList(int id, word mapping, int prepare);
int IsOverlayLoaded(SHAPELIST *list, int index);
SHAPELIST *LoadShapeList(int id, word mapping, int mode, int prepare);
void OverlayShapeList(SHAPELIST *list, int id, int first, int last, int mode);
void Paint4BitShapeItem(Handle shapes, int item, int x, int y, int page, int mode);
void PaintRLEShapeItem(Handle shapes, int item, int x, int y, int page);
void PaintShapeItem(Handle shapes, int item, int x, int y, int page, int reverse);
void PrinceFreeShp(int id);
SHAPEINFO far *PrinceGetnShapeItem(SHAPELIST *list, int index, SHAPEINFO far *result);
SHAPESIZE far *PrinceGetnShapeItemSize(SHAPELIST *list, int index, SHAPESIZE far *size);
void PrinceMarkImgsPurgeable(SHAPELIST *list, int first, int last);
void PrinceMarkRangesPurgeable(SHAPELIST *list, const int *first, const int *last);
SHAPEINFO far *PrinceNewShpItem(Handle list, int index, SHAPEINFO far *info, int mode);
void PrincePaintShapeIdx(SHAPELIST *list, int index, int x, int y, int page, int mode);
Handle PrinceUnpackShape(Handle handle, int mode, const void far *mapping);
void UnloadOverlay(SHAPELIST *list, int first, int last);
void UnloadShapeList(SHAPELIST *list);
