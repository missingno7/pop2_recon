/* Keyboard, mouse, timer, interrupts and directories. */
extern volatile unsigned long TimeVal;  /* counts timer_rate times a second */
extern volatile int Timer[4];
extern word timer_rate;
extern int interrupt_vectors_installed;

void discard_queued_events(int mask);
long divide_timer_interval(long interval, long by);
int get_current_dos_directory(char far *path);
void install_default_timer(int rate);
void install_dos_vectors(void);
void install_interrupt_handlers(void);
int mouse_button_unqueued(int button);
long multiply_timer_interval(long interval, long by);
int poll_dos_key(void);
void poll_game_port(void);
void post_pointer_event(int type, int data, int key);
void reset_timer_hook(void);
void restore_dos_vectors(void);
void restore_interrupt_vectors(void);
int set_current_dos_directory(const char far *path);
int check_rom_signature(void);
