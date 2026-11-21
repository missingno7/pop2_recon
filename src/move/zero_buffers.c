void * _cdecl memset(void *, int, unsigned int);
#pragma intrinsic(memset)

extern unsigned int RedrawBuf[30];
extern unsigned int MoveBuf[150];
extern unsigned int FRedrawBuf[150];
extern unsigned int FloorBuf[150];
extern unsigned int HalfBuf[30];
extern unsigned char ObjBuf[30];
extern unsigned int TopBuf[50];

void ZeroBuffers(void)
{
    memset(RedrawBuf, 0, sizeof(RedrawBuf));
    memset(MoveBuf, 0, sizeof(MoveBuf));
    memset(FRedrawBuf, 0, sizeof(FRedrawBuf));
    memset(FloorBuf, 0, sizeof(FloorBuf));
    memset(HalfBuf, 0, sizeof(HalfBuf));
    memset(ObjBuf, 0, sizeof(ObjBuf));
    memset(TopBuf, 0, sizeof(TopBuf));
}
