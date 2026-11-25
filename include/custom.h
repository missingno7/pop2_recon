/* Rooms with images of their own. */
#pragma pack(1)
typedef struct {
    sbyte shape;                        /* number in the room's shapes */
    POINT pos;
    sbyte pass, blit;
    int image;                          /* -1: not drawn */
    Handle handle;                      /* the image, unpacked */
    Rect clip;
    byte mirrored, peel;
    byte unknown_15[4];
} CUSTOMPIECE;

typedef struct {
    sbyte count;
    byte kind;                          /* which template */
    int first_shape, last_shape, unknown_06;
    int first_sound, last_sound;
    int floor_image, background_shape;
    byte unknown_10[12];
    CUSTOMPIECE pieces[1];
} CUSTOMROOM;
typedef struct {                        /* what a kind of room does when it is entered and left */
    union {
        void (*call)(void);
        unsigned long address;
    } load, unload;
    byte unused[20];
} ROOMTEMPLATE;
#pragma pack()

extern CUSTOMROOM *CustomRoom;
extern ROOMTEMPLATE CVCarpetPlate;
extern ROOMTEMPLATE CVBridgePlate;
extern ROOMTEMPLATE DSExitPlate;
extern ROOMTEMPLATE RNThronePlate;
extern ROOMTEMPLATE RNStart6Plate;
extern ROOMTEMPLATE RNPlateauPlate;
extern ROOMTEMPLATE TMAltarPlate;
extern ROOMTEMPLATE TMInscriptionPlate;
extern ROOMTEMPLATE FNChessPlate;
extern ROOMTEMPLATE FNCrystalPlate;
extern ROOMTEMPLATE FNTowerTopPlate;
extern ROOMTEMPLATE NullPlate;
extern ROOMTEMPLATE *CustomTemplate;

void AddCustomPiecePeel(CUSTOMPIECE *piece, Rect far *rect);
void CheckRoomSpecific(void);
CUSTOMPIECE *CustomPieceOffsetToColRow(CUSTOMPIECE *piece, sbyte column, sbyte row);
void DrawCustomRoomImgAtInfo(byte index, DRAWCELL *cell);
void GetCustomAboveAry(void);
void far *GetCustomPieceShape(CUSTOMPIECE *piece);
SHAPEINFO *_fastcall GetCustomPieceShpInfo(int index, SHAPEINFO *info);
SHAPESIZE far *GetCustomPieceSize(byte index, SHAPESIZE far *size);
int IsCustomizedRoom(byte room);
void LoadCustomRoomData(byte room_number_arg);
void LoadCustomRoomShapes(CUSTOMROOM *room);
int NeedDrawRoomBelowCustom(void);
int RestoreKeepCustomPeel(byte id);
void SaveCustomRoomPeels(void);
void SetCustomRoomTemplate(byte type);
void UnloadCustomRoomData(void);
void UnloadCustomRoomShapes(int all);
