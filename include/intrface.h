/* INTRFACE */

extern int dos_vectors_installed;
extern void *interface_buffer;
extern int backing_driver_active;
extern Rect ScreenPortRect;
extern int (*WaitFunc)(void);
extern void (*resource_error)(const char far *);

void abort_program(void);
void abort_with_message(int status, const char far *message);
int CheckWaitFunc(void);
void close_level_resources(const char **names);
void CloseRsrcFile(const char *name);
int file_exists(const char *name);
Handle GetRsrcMapHnd(const char *name);
void open_level_resources(const char **names);
word OpenRsrcVerify(const char *name);
word PollUser0(void);
void report_system_error(void);
void ResetAll(void);
int StdRsrcError(const char far *name);
void StdRsrcVerify(const char far *name);
int Wait(int timer, word ticks);
int WaitKey(void);
int WaitSnd(int sound);
int WaitTimer(int timer);
void WaitTimerNoPoll(int timer);
