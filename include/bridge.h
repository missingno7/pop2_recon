/* The bridge of the caverns. */
#pragma pack(1)
typedef struct {
    int wait, broken;
    byte floors[8];
} BRIDGEREC;
#pragma pack()

extern BRIDGEREC Bridge;
extern Rect BridgeRect;

void AnimBrgFloor(void);
void ATMBrgFloor(void);
int BrgSkelMustAdvance(void);
int BrgSkelProg(void);
int BrgSkelToPlate(void);
void BridgeCheckBreak(int kid_on_bridge, int opponent_on_bridge);
void BridgeFrameFunc(void);
sbyte BridgeGetCharCol(sbyte kind, sbyte value);
int BridgeWatchKid(void);
int BridgeWatchOpp(void);
void DrawBrgFloor(DRAWCELL *tile);
void DrawObjBrgFloor(byte unused);
int IsBrgGateOpen(void);
int IsCharOnBridge(void);
int IsOKForBrgSkelToAdvance(void);
void LoadBridgeCode(void);
void MarkBrgFloorsForChars(int kid_on_bridge, int opponent_on_bridge);
void MarkBridgeForCol(sbyte col);
void MobBrgFloor(void);
int _fastcall OffsetBrgFloorY(byte image, sbyte col);
void RedrawBrgFloor(int phase, sbyte cell);
void _fastcall RestoreBrgFloorY(byte image, int old_y);
void TriggerBrgFloor(sbyte room, sbyte cell);
void TriggerBrgFloorMob(byte room, sbyte cell);
void TweakCharYOnBridge(void);
void UnloadBridgeCode(void);
