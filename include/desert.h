/* DESERT */

extern DRAWPROC DrawDsrtFunc[];

void AddDsrtAmbient(void);
void AddDsrtStoneUpPeels(CUSTOMPIECE *piece);
void AnimDsrtExit(void);
void AnimDsrtPlanks(void);
void AnimDsrtRocks(void);
void AnimDsrtStone(void);
void DrawDsrtExit(DRAWCELL *cell);
void DrawDsrtPlanks(DRAWCELL *cell);
void DrawDsrtRocks(DRAWCELL *cell);
void DrawDsrtStone(DRAWCELL *cell);
void DsrtCheckExit(void);
void DsrtClipChar(void);
void DsrtCycleColors(void);
void DsrtFrameFunc(void);
void DsrtWatchKid(void);
void InitDsrtGlobals(void);
word IsDsrtExitOpen(void);
int IsMarkStonePos(int code);
void LoadDsrtCode(void);
void RedrawStone(void);
void RemoveLastStonePeel(sbyte column);
void SetupSand(void);
int SinkDsrtStone(sbyte cell);
void TriggerAllStonesDown(void);
void TriggerDsrtExit(sbyte room, sbyte cell);
int TriggerStone(byte room, sbyte cell, sbyte action);
