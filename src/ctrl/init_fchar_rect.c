/* init_fchar_rect (descriptive name; FM Towns inlines it in SetupCarpet, the Mac body is unlabelled):
   copy the play-area rectangle into the 8-byte rectangle stored at byte 9 of the FChar record.
   fchar_rect (DS:625b = FChar + 9) is a descriptive alias: the layout of FChar is not proven here.
   GamePlayRect (DS:09fe) is the FM Towns global name anchored by matching copies. */
typedef struct {
    int top, left, bottom, right;
} Rect;

extern Rect fchar_rect;
extern Rect GamePlayRect;

void init_fchar_rect(void)
{
    fchar_rect = GamePlayRect;
}
