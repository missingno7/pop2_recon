/* CUSTCVCA */

extern int carpet_x;
extern int carpet_y;

void AnimCaveBars(void);
Handle CheckCarpetImg(int index);
void DrawCarpet(DRAWCELL *cell);
void DrawCaveBars(DRAWCELL *tile);
void DrawObjCarpet(byte unused);
int GetCarpetFrame(void);
SHAPE far * _fastcall GetCarpetShpInfo(SHAPEINFO *ref, int index);
int IsCaveBarsOpen(void);
void LoadCarpetCode(void);
void LoadCarpetData(void);
void MapCarpetColors(void);
void RedrawCaveBars(void);
void SaveUnderCarpet(void);
void SetupCarpet(void);
void Sitting(void);
sbyte TriggerCaveBars(byte room, sbyte cell);
void TriggerCharOnCarpet(void);
void UnloadCarpetCode(void);
void UnloadCarpetData(void);
