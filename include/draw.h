/* What is drawn in a room: the pieces of each cell kind, images and objects waiting to be drawn, saved backgrounds. */
#pragma pack(1)
typedef struct {                        /* where an image of a piece goes */
    int image, x, y;
} IMAGEPOS;

typedef struct {                        /* how a cell kind looks; one table for each environment */
    byte peel;                          /* 1: drawn over the saved background of a custom room */
    IMAGEPOS back, back_fore, fore;
} PIECE;

typedef struct {                        /* a neighbouring cell */
    sbyte kind;
    LEVELCELL info;
} TILE;

typedef struct {                        /* a cell to draw, as the drawing functions of an environment get it */
    byte pass;
    sbyte column, row, kind;
    LEVELCELL info;
} DRAWCELL;

typedef void (*DRAWPROC)(DRAWCELL *cell);

typedef struct {                        /* an image behind or in front of the objects */
    byte image_set, peel;
    IMAGEPOS pos;
    sbyte column, row;
    Rect clip;
    byte blit, mirrored;
} BFITEM;

typedef struct {                        /* an image between them */
    int x, y;
    sbyte image_set;
    int image;
    Rect clip;
    byte blit;
    int variant;
    byte mirrored, owner;
} MIDITEM;

typedef struct {                        /* a character or moving object */
    int x, y;
    byte image_set;
    int image, frame;
    byte direction, kind;
    Rect clip;
    sbyte cell;
    int variant;
    byte owner;
} OBJITEM;

typedef struct {                        /* a saved background */
    SAVEDRECT *saved;
    sbyte owner, kind;
    int keep;
} PEELITEM;

typedef struct {
    int count;
    PEELITEM items[40];
} PEELINFO;
typedef struct {                        /* a part of a cell to draw again */
    int dirty;
    Rect rect;
} DIRTYCELL;
#pragma pack()

typedef struct {                        /* by cell kind */
    DRAWPROC base, item;
} CELLDRAWER;

extern SHAPEINFO HorseShpInfo;
extern int Ct[5];
extern DRAWPROC *DrawFunc;
extern DRAWPROC DrawGateMoveFunc;
extern ROOMREC Room;
extern LEVELCELL BlueSpec;
extern sbyte ObjId;
extern Rect CellClipRect;
extern TILE CellLeft;
extern TILE CellRight;
extern TILE ScrRight[3];
extern TILE ScrLeft[3];
extern TILE Above[11];
extern TILE Below[11];
extern int NewRoom;
extern sbyte RowNo;
extern FCHARREC FChar;
extern SHAPELIST *Pics[5];
extern sbyte ColNo;
extern int XCo;
extern POINT horse_level14_origin;
extern DRAWPROC *LevelDrawFuncs[];
extern DRAWPROC LevelDrawGateMoveFuncs[];

void AddDRect(Rect *rect);
void CheckGateInFore(void);
void ClrLifeBar(void);
void ClrMessage(int reset_timer);
int CompareObj(int first, int second);
int ComputeHalfRect(Rect *area);
void CopyDRects(void);
void DoFast(void);
void DoSure(void);
void DrawAll(void);
void DrawBackImg(byte pass);
void DrawBFItem(byte pass, byte index);
void _fastcall DrawCustomRoomImg(sbyte index, sbyte pass);
void DrawCustomRoomImgs(sbyte pass);
void DrawFlask(DRAWCELL *args);
void DrawFloorBuf(void);
void DrawForeImg(void);
void DrawHalf(void);
void DrawHorse(word shape, const POINT *origin, sbyte pass);
void DrawIt(SHAPE far *image, SHAPE far *mask, int x, int y, int flip, int mode, word flags);
void DrawKidMeter(int life, int max_life);
void DrawMessage(const char *message);
void DrawMeters(void);
void DrawMidItem(int index);
void DrawObj(sbyte index);
void DrawObjLoose(byte type);
void DrawObjs(sbyte column);
void DrawOppMeter(sbyte guard, int life, int max_life);
void DrawRestartMessage(void);
void DrawSword(DRAWCELL *args);
void DumpTable(byte pass);
void Fast(void);
byte GetBackImgStartPass(void);
byte GetFlaskBubbleColor(word attributes);
const POINT * _fastcall GetHorsePtInPlane(byte room, sbyte pass, word *shape);
SHAPE far *_fastcall GetHorseShpInfo(int id, SHAPEINFO *ref);
byte _fastcall GetObjForePriority(byte type);
OBJITEM *GetObjTableType(sbyte set, byte type);
void GetRooms(void);
byte GetTotalPals(int resource);
int IsFloor(sbyte kind);
int IsImpassable(sbyte kind);
int IsLandOnable(sbyte kind);
int IsMobFloor(sbyte kind);
int IsSpace(sbyte kind);
int IsWall(sbyte kind);
byte LoadObject(sbyte index);
void RedrawCellFast(sbyte column);
void RedrawCellSure(void);
void RedrawTopFast(sbyte column);
void ResetStdPalette(void);
void RestoreGamePlayPalette(void);
void SaveGamePlayPalette(void);
void SetObjCell(sbyte index);
int SetPalette(word resource, int first, byte count, byte palette);
void SetPaletteToColorIdx(byte color);
void SortBackTable(void);
void SortForeTable(void);
void SortObjTable(void);
void Sure(void);
void TweakKidShpList(int variant);
int VisibleAbovePiece(byte room, byte piece);
