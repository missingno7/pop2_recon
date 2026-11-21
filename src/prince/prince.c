/* PRINCE.C, resident DOS entries 12de through 133f. */
extern void far pascal update_display(int mode);
extern int far pascal InitPort(void near *port, void far *buffer);
extern void far pascal ZeroPeels(void);
extern unsigned char game_port[];
extern unsigned char game_buffer[];
extern int game_port_id;
extern char current_level;
extern int room_number;

extern volatile unsigned int MODE_TIMER;
extern volatile unsigned int MODE_CHANGED;
extern void far pascal REQUEST_IF_UNSET(unsigned int value);

void far pascal ToggleInvertState(void)
{
    if (MODE_TIMER != 0) {
        MODE_TIMER = 0;
        REQUEST_IF_UNSET(154);
    } else {
        MODE_TIMER = 1080;
        REQUEST_IF_UNSET(153);
    }
    MODE_CHANGED = 1;
}

/* Restrict the step value, then request mode 105 unless already pending. */
extern unsigned int RESTRICTED_STEP;
extern void far pascal REQUEST_IF_UNSET(unsigned int value);

void far pascal GoWeightless(void)
{
    RESTRICTED_STEP = 228;
    REQUEST_IF_UNSET(105);
}

/* 1316 */ void far pascal ResetData(void)
{
    update_display(0);
    game_port_id = InitPort(game_port, game_buffer);
    ZeroPeels();
    current_level = -1;
    room_number = 0;
}
