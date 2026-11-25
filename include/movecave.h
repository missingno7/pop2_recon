/* Lava pools of the caverns and their bubbles. */
#pragma pack(1)
typedef struct {
    int active;
    sbyte frame, kind;
    POINT pos;
    int layer;
} LAVABUBBLE;

typedef struct {
    sbyte frame;                        /* 3f: frame, 40: the kid fell in, 80: boiling */
    LAVABUBBLE bubbles[10];
} LAVAINFO;
typedef struct {                        /* a bubble that starts at this frame of a pool */
    sbyte frame, kind;
    POINT pos;
    int layer;
} LAVASEED;
#pragma pack()

extern sbyte *BubbleScript[6];

void AnimDart(void);
void AnimLava(void);
void ATMDart(void);
void cave_overlay_entry(void);
void CaveFrameFunc(void);
word CaveGetTorchFrame(word previous);
int CaveMoveSquishingGate(int sequence);
void CheckDartHit(void);
void CheckLavaBoil(void);
void DoLavaBoil(void);
byte GetFreeLavaInfoIdx(void);
LAVAINFO *GetLavaInfoPtr(sbyte room, sbyte col, sbyte row);
TROBREC *GetLavaTrob(sbyte room, sbyte col, sbyte row);
void HitCharWithDart(void);
void InitBridgeGlobals(void);
void InitLavaGlobals(void);
void MobDart(void);
void RedrawDart(void);
void RedrawLava(void);
void ResetLavaInfo(LAVAINFO *pool, int type);
void TriggerDart(sbyte room, sbyte cell);
int TriggerGateWithRevPlate(sbyte room, sbyte cell);
void TriggerLava(sbyte room, sbyte cell);
void TriggerLavaBubble(LAVAINFO *pool, LAVASEED *seed);
