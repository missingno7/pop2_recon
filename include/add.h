/* ADD */

extern BFITEM far BackTable[130];
extern BFITEM far ForeTable[100];
extern MIDITEM far MidTable[30];
extern OBJITEM ObjTable[30];
extern PEELINFO PeelInfo;

int AddBFImgToBTable(byte tile, int image, sbyte x, sbyte y, byte order, byte layer);
int AddBFImgToFTable(byte tile, int image, sbyte x, sbyte y, byte order, byte layer);
int AddBImgToBTable(byte tile, int image, sbyte x, sbyte y, byte order, byte layer);
void _fastcall AddCharObj(byte kind);
void AddChars(void);
int AddFImgToFTable(byte tile, int image, sbyte x, sbyte y, byte order, byte layer);
int _fastcall AddImgInfoToBFItem(byte set, IMAGEPOS *placement, int image, BFITEM far *item, sbyte x, sbyte y, byte order, byte layer);
int _fastcall AddMid(byte set, int image, int x, int y, byte order);
void _fastcall AddPeel(int left, int right, int top, int height, sbyte owner, sbyte kind);
void CheckBGSetImg(int index);
void CheckKidSetImg(int index);
void CheckOppSetImg(int index);
void ForceOppRedraw(void);
BFITEM far *_fastcall GetBackTableImgRow(int image, sbyte column, sbyte row);
int GetPeelIdxForCharIdx(sbyte owner, int occurrence);
SHAPESIZE far *GetSetImgSize(SHAPELIST *set, int index, SHAPESIZE far *size);
SHAPEINFO far *GetSetShapeItem(SHAPELIST *set, int image, SHAPEINFO far *item);
void Kid1(void);
void MarkPeelForeBufs(void);
void Opponent1(void);
void Peel(void);
void SavePeel(sbyte owner, sbyte occurrence);
void SetupChar(void);
void SetupComix(void);
void SetupFireball(void);
void SetupFlask(void);
void SetupKid(void);
int _fastcall SetupOpponent(void);
void _fastcall SetupSword(byte kind);
void ZeroPeels(void);
