/* The running game: its state, moving objects and animated cells of the room. */
typedef struct {
    int control_mode, fade_active, playing_nis, recording_mode, skip_opening, field_0a;
    int plateau_loaded, throne_song_added, throne_nis_played, escher_room_eight_seen, nis_room_restored;
} GAMESTATE;

typedef struct {
    byte enabled;
    sbyte cue;
    int song, delay, reserved;
} AmbientState;

typedef struct {                        /* joystick or keyboard, screen directions */
    sbyte x, y, button;
} JoyState;

#pragma pack(1)
typedef struct {                        /* an object that moves: loose floor, rock, dart, spikes, bridge floor, ... */
    int x, y;                           /* 00, 02 */
    byte room;                 /* 04 */
    int velocity, auxiliary;            /* 05, 07 */
    byte kind;                 /* 09 */
    sbyte row;                    /* 0a */
    word flags;                 /* 0b */
} MOBREC;

typedef struct {                        /* a cell that changes: gate, plate, torch, potion, ... */
    sbyte cell;
    byte room;
    sbyte state;
    byte kind;
} TROBREC;
#pragma pack()

#pragma pack(1)
typedef struct {                        /* where the kid comes back to life, and the level as it was then */
    int point_id, unknown_02;
    int menu, disabled;
    sbyte level, life, direction, flags, sword_kind;
    byte unknown_0d[2];
    int invert;
    sbyte room_state;
    byte unknown_12[5];
    byte cell_kinds[32][30];
    LEVELCELL cell_info[32][30];
    ROOMOBJECTS room_objects[32];
    ROOMGENERATORS generators[32];
} REBIRTHREC;
#pragma pack()
