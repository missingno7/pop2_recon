/* PRINCE */

extern byte key_down[];                /* by scan code */

extern int DoBlackOut;
extern int BlackOut;
extern PORT *main_port;
extern int Pause;
extern AmbientState ambient_state;
extern int unused_state;
extern GAMESTATE GameState;
extern LEVELREC Level;
extern REBIRTHREC far *hRebirthSaved;
extern CHARREC Char;
extern CHARREC Opp;
extern CHARREC Kid;
extern CHARREC RoomOpp[5];
extern int Resume;
extern KIDINFO KidInfo;
extern int LastResume;
extern PORT *BPort;
extern word Player;
extern FRAMEREC Frame;
extern word RedrawFlag;
extern byte OppCharId[11];
extern byte CharOppId[15];

void AddRoomTrobs(void);
sbyte BoostMeter(void);
void ChangeMeters(void);
void CheckCheatKeys(word key);
void CheckMeters(void);
void ClearKeyboard(void);
void GoWeightless(void);
void ignore_window_action(void *window, int action);
int Input(void);
sbyte *joystick_status(void);
void KeepTime(void);
char PlayOpeningSequence(void);
char poll_and_clear_pending_state(void);
int PrepCut(int keep_screen);
void PrinceCopy(Rect far *rect);
word PrincePoll(void);
void read_controls(void);
void read_gameport_controls(void);
void read_keyboard_controls(void);
void ResetData(void);
void ResetGame(void);
void SetStartLevel(void);
int toggle_sound(void);
void ToggleInvertState(void);
sbyte update_step_delta(void);
