/* DRAWCAVE */

extern DRAWPROC DrawCaveFunc[];

void DrawCaveBlock(DRAWCELL *cell);
void DrawCaveBotEdge(DRAWCELL *cell);
void DrawCaveDart(DRAWCELL *cell);
void DrawCaveExit1(DRAWCELL *cell);
void DrawCaveExit2(DRAWCELL *cell);
void DrawCaveExit3(DRAWCELL *cell);
void DrawCaveExit4(DRAWCELL *cell);
void DrawCaveExitMovable(DRAWCELL *cell);
void DrawCaveFloor(DRAWCELL *cell);
void DrawCaveGate(DRAWCELL *cell);
void DrawCaveGateMovable(DRAWCELL *cell);
void DrawCaveGateSquishing(DRAWCELL *cell);
void DrawCaveLava1(DRAWCELL *cell);
void DrawCaveLava2(DRAWCELL *cell);
void DrawCaveLavaBubbles(DRAWCELL *cell, int right);
void DrawCaveLoose(DRAWCELL *cell);
void DrawCavePillarBot(DRAWCELL *cell);
void DrawCavePillarTop(DRAWCELL *cell);
void DrawCavePosts(DRAWCELL *cell);
void DrawCaveRevPlate(DRAWCELL *cell);
void DrawCaveRubble(DRAWCELL *cell);
void DrawCaveRubblePieces(DRAWCELL *cell);
void DrawCaveRubbleTorch(DRAWCELL *cell);
void DrawCaveSpace(DRAWCELL *cell);
void DrawCaveSpikes(DRAWCELL *cell);
void DrawCaveTopEdge(DRAWCELL *cell);
void DrawCaveTorch(DRAWCELL *cell);
int _fastcall IsRoundCaveBlock(word modifier);
void load_cave_drawing(void);
