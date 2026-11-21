/* init_fchar_rect (descriptive name; FM Towns inlines it in SetupCarpet, the Mac body is unlabelled):
   copy the play-area rectangle into the FChar record at byte offset 9.
   FChar (DS:6252) and GamePlayRect (DS:09fe) are FM Towns global names anchored by matching copies. */
#pragma pack(1)
typedef struct {
    int top, left, bottom, right;
} Rect;

typedef struct {
    char head[9];
    Rect rect;
    char tail[3];
} FCharRecord;

extern FCharRecord FChar;
extern Rect GamePlayRect;

void init_fchar_rect(void)
{
    FChar.rect = GamePlayRect;
}
