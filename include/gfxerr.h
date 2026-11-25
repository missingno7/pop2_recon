/* GFXERR */

Handle allocate_gold_handle(int id);
int confirm_action(const char far *message);
int confirm_quit(const char far *message);
void *create_dialog_port(int resource);
word read_queued_key(void);
void restore_dialog_port(void *port);
int run_dialog_preserving_directory(int (*show)(int, int, int), int first, int second, int third);
void run_window_item_effect(void *port, int id);
void set_mode_flag_value(void);
void set_near_record_word20(PORT *record, word value);
void set_window_item_text(void *port, int id, const void *text);
int show_message(const char far *message, int replace_port);
int show_resource_dialog(int resource, int item, const char far *message, int replace_port);
