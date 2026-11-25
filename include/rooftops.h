/* The rooftops. */
#pragma pack(1)
typedef struct {                        /* someone in the water */
    sbyte character, frame;
    int x;
    byte low;
} WATERSTATE;
#pragma pack()

extern WATERSTATE InWater;
extern DRAWPROC DrawRoofFunc[];

void AnimRoofGlass(void);
void AnimRoofPillar(void);
void AnimRoofShip(void);
int DrawPillarInBack(CUSTOMPIECE *piece);
void DrawRoofGlass(DRAWCELL *animation);
int _fastcall DrawRoofLedgeInBack(CUSTOMPIECE *piece);
void DrawRoofPillar(DRAWCELL *animation);
void DrawRoofShip(DRAWCELL *animation);
void LoadRoofCode(void);
void RoofCheckCharInWater(void);
void RoofCheckPalette(sbyte room);
void RoofCheckRoomSpecific(void);
int RoofClimbingIntoShip(void);
void RoofClipChar(void);
void RoofCycleColors(void);
void RoofFrameFunc(void);
int RoofGrabShip(void);
int RoofHangingFromShip(void);
void RoofKillKidOnWharf(void);
Rect *RoofMakeWaterTestRect(Rect *rect);
CUSTOMPIECE *RoofOffsetPillarWaterToFore(CUSTOMPIECE *piece);
void RoofTriggerCharInWater(void);
void RoofWatchKid(void);
void RoofWatchOpps(void);
void SetupSplash(void);
void UpdateShipPeel(sbyte slot, int bottom);
