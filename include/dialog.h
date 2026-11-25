/* Dialogs: credits, menus, saved games and the hall of fame. */
struct save_slot_table {
    int count;
    char initials[10][25];
};

#pragma pack(1)
struct hall_record {
    char name[27];
    int score;
};

struct hall_table {
    int count;
    struct hall_record records[5];
};
#pragma pack()

extern int notice_state;

int change_menu_option(int option);
Rect far * compute_save_slot_rect(Rect far *rect, int index);
int far * ComputeHallPlayerRect(int far *p, int index);
int * ComputeSymbolPt(int *p, int index);
void CopyProtectionDialog(void);
void DialogDrawNumber(int number, const POINT *point, const Rect far *rect, const POINT *range);
void DialogDrawStr(const char far *text, int font, int style, const POINT *point, const Rect far *rect, const POINT *bounds);
void DialogDrawStrId(int string_id, int font, int style, const POINT *point, const Rect far *rect, const POINT *range);
int DialogFadeIn(void);
int DialogFadeOut(void);
void DialogResetToGamePlay(int restart_input);
int DialogWaitFunc(void);
int DoCreditsDialog(void);
void draw_dialog_text_block(EDITFIELD far *block);
void draw_main_menu(void);
void draw_menu_icon(int index, int selected);
void draw_menu_label(int index, int selected);
void draw_save_slot(struct save_slot_table far *slots, int index, int selected);
void draw_save_slot_frame(Rect *rect, int selected);
void draw_save_slots(struct save_slot_table far *slots, int saving);
void DrawCopyProtectionDialog(word symbol, Handle asset, int attempt);
void DrawCreditCategory(int id);
void DrawCreditsPage(int first, int last);
void DrawDialogFrame(Handle asset);
void DrawHallOfFameDialog(struct hall_table far *table);
void DrawSaveGameMsg(int selection);
void DrawSymbolBox(int index, int selected);
int GetCopyProtectionSymbol(void);
void GetHallOfFameName(struct hall_table far *table, int row);
int GetSaveGameLocation(struct save_slot_table far *slots, int saving);
void HallOfFameDialog(int score);
struct hall_table far * HOFAddPlayer(struct hall_table far *table, int score, int *index);
void InitDialog(void);
struct hall_table far * InitHallOfFame(int score, int *index);
struct save_slot_table far * InitSavedGameStruct(void);
int IsValidTimeForDialog(void);
int IsValidTimeForSaveGame(void);
Handle load_frame_asset(void);
int main_menu_choice(void);
void main_menu_dialog(void);
int * make_index_rect(int *p, int index);
void RemoveRebirthPt(void);
int ResumeGame(int slot);
int ResumeGameDialog(void);
int SaveGame(struct save_slot_table far *slots, int slot);
int SaveGameDialog(void);
void SaveOptionsToDisk(void);
void SaveRebirthPtStatus(int point_id);
void SetOptionsFromDisk(void);
void SetRebirthPtInitials(void);
int SetRebirthPtKidStatus(void);
void SetRebirthPtLevelStatus(void);
int ShowHallOfFame(int hold);
int WriteHallOfFameFile(struct hall_table far *table);
