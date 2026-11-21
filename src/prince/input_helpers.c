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
            joystick_y = 1;
    } else {
        joystick_y = -1;
    }

    if (!(key_down[0x4b] || key_down[0x24] || key_down[0x1f] ||
          key_down[0x47] || key_down[0x16] || key_down[0x11] ||
          key_down[0x4f] || key_down[0x32] || key_down[0x2d])) {
        if (key_down[0x4d] || key_down[0x26] || key_down[0x21] ||
            key_down[0x49] || key_down[0x18] || key_down[0x13] ||
            key_down[0x51] || key_down[0x34] || key_down[0x2f])
            joystick_x = 1;
    } else {
        joystick_x = -1;
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
