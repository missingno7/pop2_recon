/* Loading levels, characters and their images. */
typedef void (*LevelFrameProc)(void);

typedef void (*LevelFrameProc)(void);

extern byte OppBank[2];
extern Handle SwordFrames;
extern const char *resource_files[3];
extern const char *LevelRsrcFiles[7];
extern int BGSetMarkedPurgeable;
extern const int Level1stOverlay[7];
extern LevelFrameProc LevelFrameFunc[7];
extern sbyte LastOppTypeLoaded;
extern int *Opp1stRange[11];
extern int Need1stImg[8];
extern int NeedLastImg[8];

void CheckBPort(void);
void CheckLoadedOppSet(sbyte kind);
void _fastcall clear_special_sequence_flag(int i);
int _fastcall GetSwordFrameImg(word index);
int load_resource_error(Handle resource);
void LoadBGSet(int palette);
void LoadCodeOverlays(byte background_kind, byte opponent_kind);
void LoadFlaskSet(void);
void LoadLevel(sbyte number);
void LoadLevelMap(sbyte map);
void LoadLevelMinimum(sbyte number);
void LoadOppSet(sbyte kind);
void LoadPiecesData(void);
void LoadSwordSet(sbyte kind);
int NeedOppOverlay(sbyte kind, int index);
void OpenLevelRsrcFiles(byte current, byte previous);
void PreloadRoomData(byte room);
void report_resource_path(const char far *path);
void SetKidPalette(sbyte kind);
void SetPrinceLevelSpecPtrs(byte kind);
void UnloadFrom(int first);
void UnloadLevel(int first);
void UnloadPiecesData(void);
void UnloadSet(int set);
