/* DRAWRUIN */

extern DRAWPROC DrawRuinFunc[];

void DrawRuinBackWall(DRAWCELL *cell);
void DrawRuinBlock(DRAWCELL *cell);
void DrawRuinBones(DRAWCELL *cell);
void DrawRuinCrevice(DRAWCELL *cell);
void DrawRuinDownPlate(DRAWCELL *cell);
void DrawRuinExit1(DRAWCELL *cell);
void DrawRuinExit2(DRAWCELL *cell);
void DrawRuinExitMovable(DRAWCELL *cell);
void DrawRuinFlaw(DRAWCELL *cell);
void DrawRuinFloor(DRAWCELL *cell);
void DrawRuinGate(DRAWCELL *cell);
void DrawRuinGateMovable(DRAWCELL *cell);
void DrawRuinLoose(DRAWCELL *cell);
void DrawRuinPillarBot(DRAWCELL *cell);
void DrawRuinPillarTop(DRAWCELL *cell);
void DrawRuinPosts(DRAWCELL *cell);
void DrawRuinRubble(DRAWCELL *cell);
void DrawRuinRubblePieces(DRAWCELL *cell);
void DrawRuinSpace(DRAWCELL *cell);
void DrawRuinUpPlate(DRAWCELL *cell);
void LoadRuinsDrawCode(void);
void RuinChangeCrevices(int reveal, sbyte room, sbyte column, sbyte row);
void RuinCheckCrevices(void);
void RuinSetCreviceRevealed(int reveal);
