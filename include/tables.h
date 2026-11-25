/* Tables the whole game uses. */
#pragma pack(1)
typedef struct {                        /* how a sound behaves among others */
    byte mode, priority, attribute;
} SOUNDRULE;
#pragma pack()

extern sbyte Plus1[2];       /* by direction + 1 */
extern sbyte Minus1[2];
extern int ColEdgeX[20];
extern int Col2PixelX[13];
extern int Row2PixelY[5];
extern sbyte BarL[9];          /* where a barrier begins and ends in its cell */
extern sbyte BarR[9];
extern SOUNDRULE Snd[];
extern PIECE *Pieces;                   /* of the environment shown */
extern byte LooseSounds[12];
extern FRAMEREC far KidFrames[];        /* by Char.frame */
