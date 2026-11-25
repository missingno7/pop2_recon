/* COLLIDE */

extern byte ColDetLastFrame[10];
extern byte ColDetThisFrame[10];
extern sbyte StepTo;

int AdjustRoom(byte kind, int value);
void AirBump(void);
void BumpSound(void);
void CheckBarr(void);
int CheckCollide(byte cell);
int CheckCollide1(sbyte col);
void CheckGateForKid(void);
void CheckGateForOpp(void);
void CheckWeightlessBarr(void);
void Collide(int displacement, sbyte direction);
void Collisions(void);
int CreviceBarr(void);
void EnemyCollide(void);
int GateBarr(void);
int GateBarrToStandUp(void);
int GetBarrCode(byte cell);
int GetBarrDistFromBack(byte cell, byte room, sbyte col);
int GetBarrDistFromFace(byte cell, byte room, sbyte col);
void GetCharEdges(void);
void GetColDetData(sbyte row, byte *rooms, byte *edges);
int GetFwdDist(void);
int GetLeftBarr(byte room, sbyte col, sbyte row);
int GetRightBarr(byte room, sbyte col, sbyte row);
int GetSwordWidth(void);
void GroundBump(sbyte direction);
void InitColDetBufs(void);
void InitCollisionDetection(void);
void LeftCollide(void);
void PushCharAwayFromGate(void);
void reset_barrier_state(void);
void RightCollide(void);
int Scythe1Barr(void);
