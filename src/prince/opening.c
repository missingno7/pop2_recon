/* Resident PRINCE.C startup. Externs denote symbols in other contributions. */

extern const char *resource_files[2];
extern unsigned char restart_requested;
extern char selected_level;
extern char current_level;
extern char restart_level;
extern int command_line_enabled;
extern int skip_intro;
extern int skip_opening;
extern int restart_from_demo;
extern int restart_game_level;
extern int demo_level;
extern int demo_changed;
extern int demo_pending;
extern char watch_memory_option[];
extern char restart_message[];
/* These three adjacent option strings are used by the startup routines. */
static struct {
    char nis[4];
    char tree[5];
    char level[7];
} startup_options = { "NIS", "TREE", "LEVEL" };

extern void far pascal platform_setup(void);
extern void far pascal save_options(void);
extern void far pascal OpenRsrcVerify(const char *filename);
extern int far pascal Rnd(int seed);
extern char near *far pascal find_option(const char far *option);
extern int far cdecl atoi(const char near *text);
extern void far pascal log_message(const char near *message);
extern void far pascal write_message(const char near *message);
extern void far pascal show_dialog(const char near *title, const char far *message, int flags);
extern char far pascal Start(char level);
extern int far pascal PlayNIS(int sequence);
extern void far pascal finish_restart(int level);
extern void far pascal quit_game(int status, int flags);
extern void far pascal clear_picture(long color);
extern void far pascal clear_display(long color);
extern int far pascal opening_disabled(void);
extern int far pascal opening_input(int mode);
extern void far pascal TurnRecordedGamePlayerOff(void);
extern void far pascal TurnRecordedGamePlayerOn(void);
extern void far pascal finish_opening(void);
extern int far pascal poll_demo_state(void);
extern void far pascal update_display(int mode);
extern int far pascal InitPort(void near *port, void far *buffer);
extern void far pascal ZeroPeels(void);

extern unsigned char game_port[];
extern unsigned char game_buffer[];
extern int game_port_id;
extern int room_number;

void far pascal ResetGame(void);
void far pascal read_start_level(void);
char far pascal PlayOpeningSequence(void);
char far pascal poll_and_clear_pending_state(void);
void far pascal ResetData(void);

/* 0000 */ void far cdecl main(void)
{
    const char **file;

    platform_setup();
    save_options();
    read_start_level();
    file = resource_files;
    do {
        OpenRsrcVerify(*file);
        ++file;
    } while (file < resource_files + 2);
    Rnd(1);
    restart_requested = 0;
    ResetGame();
}

/* 0036 */ void far pascal ResetGame(void)
{
    char level = selected_level;

    for (;;) {
        if (command_line_enabled) {
            if (find_option(watch_memory_option)) {
                log_message(restart_message);
                write_message(restart_message);
                show_dialog(0, restart_message, 0);
            }
            if (find_option(startup_options.nis) || find_option(startup_options.tree))
                break;
        }
        ResetData();
        if (selected_level == 0) {
            level = PlayOpeningSequence();
        } else {
            level = Start(level);
            if (restart_from_demo)
                restart_level = level = 0;
            if (restart_game_level == 0) {
                selected_level = level = 0;
            } else if (level > 14) {
                PlayNIS(11);
                finish_restart(restart_game_level);
                selected_level = level = 0;
            } else if (selected_level != 0 && level == 0) {
                selected_level = level = restart_level;
            }
        }
    }
    for (;;)
        quit_game(0, 0);
}

#pragma optimize("e", off)
/* 0116 */ void far pascal read_start_level(void)
{
    const char *option;

    selected_level = 0;
    if (command_line_enabled) {
        option = find_option(startup_options.level);
        if (option == 0)
            return;
        selected_level = (char)atoi(option + 5);
        if (selected_level < 1 || selected_level > 14)
            selected_level = 1;
    }
}

#pragma optimize("e", on)
/* 014e */ char far pascal PlayOpeningSequence(void)
{
    int result;
    char level;

    clear_picture(0x00ffffffL); clear_display(0x00ffffffL);
    result = 1;
    if (!skip_intro) {
        if (skip_opening)
            result = 2;
        else if (opening_disabled() || opening_input(0))
            result = 2;
    } else {
        skip_intro = 0;
    }
    if (result != 2)
        result = PlayNIS(7);
next_opening_clip:
    if (result != 2) result = PlayNIS(4);
    if (result != 2)
        result = PlayNIS(8);
    if (result == 2) {
        selected_level = 1;
        skip_intro = 1;
        TurnRecordedGamePlayerOff();
    } else {
        TurnRecordedGamePlayerOn();
    }
    level = selected_level;
    finish_opening();
    if (demo_pending)
        level = poll_and_clear_pending_state();
    return level;
}

/* 0210 */ char far pascal poll_and_clear_pending_state(void)
{
    char level = selected_level;

    if (poll_demo_state() > 0) {
        level = selected_level = current_level;
        demo_changed = 0;
    }
    demo_pending = 0;
    return level;
}


/* ResetData remains a draft at this placement. */
/* 1316 */ void far pascal ResetData(void)
{
    update_display(0);
    game_port_id = InitPort(game_port, game_buffer);
    ZeroPeels();
    current_level = -1;
    room_number = 0;
}
