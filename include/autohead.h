/* AUTOHEAD */

extern Handle hHeadChompFrame;

void AlignHeadOnKidNeck(void);
int CharCanSeeOpp(void);
int ChompAgain(HEADPLAN *plan);
OPPONENT *HeadAddOpponent(OPPONENT *opponent);
void HeadAttackProg(HEADPLAN *plan);
int HeadBlocked(void);
int HeadClearInDir(int direction);
void HeadDoneAttackingKid(HEADPLAN *plan);
int HeadEnemyCollide(int minimum_gap);
void HeadFaceKid(void);
void HeadFleeProg(HEADPLAN *plan);
void HeadFocusOnNeck(HEADPLAN *plan);
void HeadGetAngry(HEADPLAN *plan);
void HeadGetPeaceful(HEADPLAN *plan);
int HeadGoToDest(HEADPLAN *plan);
void HeadLockedProg(HEADPLAN *plan);
void HeadOnAlert(void);
void HeadPeacefulProg(void);
void HeadProg(void);
int HeadSetToFleeMode(void);
int IsAbleToFleeInRoom(byte room);
int IsHeadLockedOn(byte room, int slot);
int JustStartVerticalSeq(void);
void SnakeAttack(void);
void SnakeComeOutOfHole(OPPONENT *record);
void SnakeGoIntoHole(OPPONENT *record);
void SnakeProg(void);
int SnakeSlitheringTowardOpp(void);
void SnakeUpdateOppInfo(OPPONENT *record);
void unused_head_entry(void);
int XWithinMiddleOfCell(int first, int second);
int YWithinMiddleOfCell(int value, int half_width);
