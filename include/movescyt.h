/* MOVESCYT */

extern int ScytheImg[20];

void AnimScythe(void);
void CheckScythe(void);
void CheckScytheHit(void);
int ComputeBladeHitRect(Rect *bounds);
int GetScytheHitType(word value);
void RedrawScythe(int bloody);
int ScytheBarrToStandUp(void);
int ScytheBladeExtended(word value);
void TriggerScythe(byte room, sbyte index);
