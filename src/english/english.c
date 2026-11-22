/* ENGLISH.C -- DOS countdown, control, and diagnostic messages. */

char one_second_left[] = "1 SECOND LEFT";
char seconds_left[] = "%d SECONDS LEFT";
char minutes_left[] = "%d MINUTES LEFT";
char time_expired[] = "TIME HAS EXPIRED!";

char restart_key_message2[] = "PRESS BUTTON TO CONTINUE";
char restart_key_message1[] = "PRESS KEY TO CONTINUE";

char sound_on_message[] = "Sound On";
char sound_off_message[] = "Sound Off";
char ambient_on_message[] = "Ambient Music On";
char ambient_off_message[] = "Ambient Music Off";
char music_unavailable_message[] = "Music Unavailable";
char joystick_unavailable_message[] = "Joystick Unavailable";
char joystick_mode_message[] = "Joystick Mode";
char joystick_missing_message[] = "Joystick Not Found";
char keyboard_mode_message[] = "Keyboard Mode";
char paused_message[] = "GAME PAUSED";

char video_error[] =
    "Prince of Persia II only runs in MCGA video mode\n."
    "The program is unable to detect an MCGA video mode.\n\n";

char saved_message[] = "GAME SAVED";
char nosave_message[] = "UNABLE TO SAVE GAME";

const char digital_sound_error[] =
    "The digital sound card selected did not initialize.  "
    "There will be no digital sounds.";
const char midi_sound_error[] =
    "The midi sound card selected did not initialize.  "
    "There will be no music.";

const char OutOfFarMem[] = "Insufficient far Memory";
const char system_error[] = "System error.\r\rPress any key to return to DOS.";
char resource_path_format[] = "Couldn't find resource file \"%s\"";
char file_error[] =
    "\nCould not find the system configuration file %s.\n"
    "Please run SETUP from within the Prince of Persia 2 directory.\n";
char config_filename[] = "CONFIG.DAT";
