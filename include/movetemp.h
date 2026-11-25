/* MOVETEMP */

void ATMSlideBolt(void);
void ATMSlideRock(void);
void ATMTrapDoor(void);
int CanSlideRockStop(void);
void check_temple_event(void);
void CheckSlideRockCrush(void);
void CheckTrapDoor(void);
Rect *ComputeSlideRockRect(Rect *rect, byte room, sbyte row);
void DrawObjSlideRock(byte kind);
void DrawObjTrapDoor(byte modifier);
int GetSlideRockBarrDist(sbyte direction, byte room, sbyte row);
void GetSlideRockDetData(sbyte row, byte *rooms, byte *flags);
MOBREC *_fastcall GetSlideRockOnRoomRow(byte room, sbyte row);
void HitCharWithSlideRock(void);
int IsAllFloorsInDirToRock(sbyte row_delta, int step);
void LoadInscriptionRoomFunc(void);
void MobSlideRock(void);
void MobTrapDoor(void);
void ShakeTrapDoor(void);
int SlideRockInCell(int margin);
int SlideRockNearSpikes(void);
void TempFrameFunc(void);
int _fastcall TempGetSlideRockX(byte room, sbyte row);
int TempGetTorchFrame(int active);
void TriggerSlideRock(byte room, sbyte cell, byte action);
void TriggerTrapDoor(byte room, sbyte cell);
