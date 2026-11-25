/* INIT */

extern PORT ScreenPort;

unsigned long choose_memory_pool(int *memory_class);
void init_graphics(void);
void init_sound(void);
void InitSystem(void);
void save_initial_setting(void);
void show_init_message(const char *message);
void wait_for_ready(void);
