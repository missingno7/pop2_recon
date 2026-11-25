/* LGRAF */

PORT *AllocBackPort(Rect far *bounds);
void CycleColors(int first, int count, int setting);
void DeallocBackPort(PORT *port);
void DeallocMapSave(SAVEDRECT *saved);
int EncloseRect(const Rect far *rect, const Rect far *other);
int EqualRect(const Rect far *a, const Rect far *b);
void InvertBitMap(const BITMAP far *map);
void RestoreRectKeep(SAVEDRECT *saved);
