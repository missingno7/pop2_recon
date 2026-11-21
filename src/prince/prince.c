extern int control_mode;
extern int joystick_x_raw, joystick_y_raw;
extern int joystick_x_center, joystick_y_center;
extern signed char joystick_button_raw;
extern signed char joystick_x, joystick_y, joystick_button;
extern unsigned char key_down[];
extern void far pascal sample_gameport(void);
extern void far pascal flush_bios_keyboard(void);

#define keyboard_flags (*(volatile unsigned char far *)0x00400017UL)

void far pascal read_gameport_controls(void);
void far pascal read_keyboard_controls(void);

/* 105a */ void far pascal read_controls(void)
{
    if (control_mode == 2)
        read_gameport_controls();
    else
        read_keyboard_controls();
}

/* 106c */ void far pascal read_gameport_controls(void)
{
    register int displacement;
    int threshold;

    sample_gameport();
    threshold = *(volatile int *)&joystick_y_center / 2;
    displacement = joystick_y_raw - joystick_y_center;
    if (displacement > threshold)
        joystick_y = 1;
    else if (-threshold > displacement)
        joystick_y = -1;

    threshold = *(volatile int *)&joystick_x_center / 2;
    displacement = joystick_x_raw - joystick_x_center;
    if (displacement > threshold)
        joystick_x = 1;
    else if (-threshold > displacement)
        joystick_x = -1;

    joystick_button = -joystick_button_raw;
    if (joystick_button < -2)
        joystick_button = -1;
}

/* 10d8 */ void far pascal read_keyboard_controls(void)
{
    if (keyboard_flags & 8)
        return;

    if (!(key_down[0x48] || key_down[0x17] || key_down[0x12] ||
          key_down[0x47] || key_down[0x16] || key_down[0x11] ||
          key_down[0x49] || key_down[0x18] || key_down[0x13])) {
        if (key_down[0x50] || key_down[0x25] || key_down[0x20] ||
            key_down[0x4c] || key_down[0x33] || key_down[0x2e] ||
            key_down[0x4f] || key_down[0x32] || key_down[0x2d] ||
            key_down[0x51] || key_down[0x34] || key_down[0x2f])
            joystick_x = 1;
    } else {
        joystick_x = -1;
    }

    if (!(key_down[0x4b] || key_down[0x24] || key_down[0x1f] ||
          key_down[0x47] || key_down[0x16] || key_down[0x11] ||
          key_down[0x4f] || key_down[0x32] || key_down[0x2d])) {
        if (key_down[0x4d] || key_down[0x26] || key_down[0x21] ||
            key_down[0x49] || key_down[0x18] || key_down[0x13] ||
            key_down[0x51] || key_down[0x34] || key_down[0x2f])
            joystick_y = 1;
    } else {
        joystick_y = -1;
    }

    keyboard_flags &= 0x0f;
    if (!(keyboard_flags & 3) && !key_down[0x53]) {
        if ((keyboard_flags & 4) || key_down[0x52])
            joystick_button = -2;
    } else {
        joystick_button = -1;
    }
}

/* 125a */ void far pascal clear_keyboard(void)
{
    if (control_mode == 0)
        flush_bios_keyboard();
}

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

typedef struct {
    int top, left, bottom, right;
} Rectangle;


extern unsigned char game_port[];
extern unsigned int source_port;
extern void far pascal copy_port_rect(void near *port, unsigned int source,
                                     const Rectangle far *source_rect,
                                     const Rectangle far *dest_rect, int flags);

void far pascal PrinceCopy(const Rectangle far *rect)
{
    Rectangle flipped;
    Rectangle far *copy_rect;

    if (MODE_TIMER != 0) {
        copy_rect = &flipped;
        *copy_rect = *rect;
        copy_rect->top = 192 - rect->bottom;
        flipped.bottom = 192 - rect->top;
    } else {
        copy_rect = (Rectangle far *)rect;
    }

    copy_port_rect(game_port, source_port, copy_rect, copy_rect, 0);
}

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
