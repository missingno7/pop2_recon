/* MISC */

extern Handle hOppFrames;

sbyte CellToCol(byte room, int cell);
sbyte CellToRow(byte room, int cell);
void ComputeClipRect(Rect far *rect, int col, int row);
Rect *ComputeOffsetCellRect(Rect *rect, Rect *base, int col, int row);
void GetAboveAry(void);
void GetBelowAry(void);
byte GetObject(byte room, sbyte col, sbyte row, byte *tile, LEVELCELL *info, byte missing);
byte PieceAboveId(sbyte col, sbyte row);
byte PieceAboveRightId(sbyte col, sbyte row);
byte PieceBelowId(sbyte col, sbyte row);
byte PieceLeftId(sbyte col, sbyte row);
byte PieceRightId(sbyte col, sbyte row);
int release_discardable_images(void);
int report_fatal_memory(void);
void select_room_arrays(byte room);
void SetLocVars(void);
void SetSideRoomInfo(void);
