/* The running game: its state, moving objects and animated cells of the room. */
typedef struct {
    int control_mode, fade_active, playing_nis, recording_mode, skip_opening, field_0a;
    int plateau_loaded, throne_song_added, throne_nis_played, escher_room_eight_seen, nis_room_restored;
} GAMESTATE;

typedef struct {
    unsigned char enabled;
    signed char cue;
    int song, delay, reserved;
} AmbientState;

typedef struct {                        /* joystick or keyboard, screen directions */
    signed char x, y, button;
} JoyState;

#pragma pack(1)
typedef struct {                        /* an object that moves: loose floor, rock, dart, spikes, bridge floor, ... */
    int x, y;                           /* 00, 02 */
    unsigned char room;                 /* 04 */
    int velocity, auxiliary;            /* 05, 07 */
    unsigned char kind;                 /* 09 */
    signed char row;                    /* 0a */
    unsigned int flags;                 /* 0b */
} MOBREC;

typedef struct {                        /* a cell that changes: gate, plate, torch, potion, ... */
    signed char cell;
    unsigned char room;
    signed char state;
    unsigned char kind;
} TROBREC;
#pragma pack()
