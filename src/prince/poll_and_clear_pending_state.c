extern char selected_level;
extern char current_level;
extern int demo_changed;
extern int demo_pending;
extern int far pascal poll_demo_state(void);

char far pascal poll_and_clear_pending_state(void)
{
    char level = selected_level;

    if (poll_demo_state() > 0) {
        level = selected_level = current_level;
        demo_changed = 0;
    }
    demo_pending = 0;
    return level;
}
