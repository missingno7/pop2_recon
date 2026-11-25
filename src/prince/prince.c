/* Resident PRINCE.C, DOS root [0000,1340). */
#include "types.h"
#include "level.h"
#include "char.h"
#include "game.h"

extern const char *resource_files[3];
extern char StartLevel;
extern char LevelIdx;
extern int frame_diagnostics;

extern int demo_level;
extern int RestartLevel;
extern int nis_skip_requested;
extern int Minutes;
/* The state of the game. RTLink lays these out first, in this order, at DGROUP 2b90. */
int DoBlackOut, BlackOut, main_port, Pause;
struct {
    unsigned char enabled;
    signed char cue;
    int song, delay, reserved[2];
} ambient_state;
GAMESTATE GameState;
LEVELREC Level;
int far *hRebirthSaved;
CHARREC Char, Opp, Kid, RoomOpp[5];
int Resume;
KIDINFO KidInfo;
int LastResume;
unsigned int BPort, Player;
struct {
    int image, equipment_image;                       /* 00,02 */
    signed char x, y;                                 /* 04-05 */
    unsigned char flags;                              /* 06 */
} Frame;
unsigned int RedrawFlag;
static char title[] = "PRINCE OF PERSIA 2 v1.1";
/* FM Towns names these adjacent lookup tables OppCharId and CharOppId. */
unsigned char OppCharId[11] = { 2, 10, 4, 6, 1, 7, 8, 2, 11, 12, 0 };
unsigned char CharOppId[15] = {
    10, 4, 0, 0, 2, 0, 3, 5, 6, 0, 1, 8, 9, 0, 0
};

typedef struct {
    int unused;
    int control_mode;
    int memory;
    unsigned int digital_type, midi_type;
} GameOptions;

extern GameOptions *configuration;
extern struct { unsigned char channels, volume; } Midi;
extern void far *digital_driver, far *midi_driver;
extern int far cdecl sprintf(char *buffer, const char *format, ...);
extern void far pascal show_init_message(const char *);
extern void far pascal InitSystem(void);
extern void far pascal SetOptionsFromDisk(void);
extern void far pascal OpenRsrcVerify(const char *);
extern int far pascal Rnd(int);
extern char near *far pascal find_option(const char far *);
extern int far cdecl atoi(const char near *);
extern void far pascal check_heaps(const char near *);
extern void far pascal check_null_area(const char near *);
extern void far pascal dump_heap(const char near *, const char far *, int);
extern char far pascal Start(char level);
extern int far pascal PlayNIS(int sequence);
extern void far pascal HallOfFameDialog(int);
extern void far pascal CheckPlayNIS(int status, int flags);
extern void far pascal HeapPurge(long);
extern void far pascal HeapCompact(long);
extern int far pascal DoCreditsDialog(void);
extern int far pascal ShowHallOfFame(int);
extern void far pascal TurnRecordedGamePlayerOff(void);
extern void far pascal TurnRecordedGamePlayerOn(void);
extern void far pascal reset_level_overlays(void);
extern int far pascal ResumeGameDialog(void);
extern void far pascal ResetSnd(int);
extern int far pascal InitPort(void near *, void far *);
extern void far pascal ZeroPeels(void);

extern unsigned char ScreenPort[];
extern unsigned char ScreenPortRect[];
extern int Graf;

void far pascal ResetGame(void);
void far pascal SetStartLevel(void);
char far pascal PlayOpeningSequence(void);
char far pascal poll_and_clear_pending_state(void);
void far pascal ResetData(void);

/* 0000 */ void far cdecl main(void)
{
    const char **file;
    char message[100];

    InitSystem();
    if (find_option("sbdiag")) {
        sprintf(message, "DType:%d MType:%d\rDDrv:%d MDrv: %d\rDInit:%d MInit:%d",
                configuration->digital_type != 0, configuration->midi_type != 0,
                digital_driver ? 1 : 0, midi_driver ? 1 : 0,
                (Midi.channels & 1) != 0, (Midi.channels & 2) != 0);
        show_init_message(message);
    }
    SetOptionsFromDisk();
    SetStartLevel();
    file = resource_files + 1;
    do {
        OpenRsrcVerify(*file);
        ++file;
    } while (file < resource_files + 3);
    Rnd(1);
    Level.environment_kind = 0;
    ResetGame();
}

/* 0036 */ void far pascal ResetGame(void)
{
    char level = StartLevel;

    for (;;) {
        if (frame_diagnostics) {
            if (find_option("WATCHMEM")) {
                const char near * const restart_message = "At beginning of ResetGame() loop.";
                check_heaps(restart_message);
                check_null_area(restart_message);
                dump_heap(0, restart_message, 0);
            }
            if (find_option("NIS") || find_option("TREE"))
                break;
        }
        ResetData();
        if (StartLevel == 0) {
            level = PlayOpeningSequence();
        } else {
            level = Start(level);
            if (GameState.recording_mode)
                Level.level_number = level = 0;
            if (Minutes == 0) {
                StartLevel = level = 0;
            } else if (level > 14) {
                PlayNIS(11);
                HallOfFameDialog(Minutes);
                StartLevel = level = 0;
            } else if (StartLevel != 0 && level == 0) {
                StartLevel = level = Level.level_number;
            }
        }
    }
    for (;;)
        CheckPlayNIS(0, 0);
}

/* 0116 */ void far pascal SetStartLevel(void)
{
    char near *option;

    StartLevel = 0;
    if (frame_diagnostics) {
        option = find_option("LEVEL");
        if (option == 0)
            return;
        StartLevel = (char)atoi(&option[5]);
        if (StartLevel < 1 || StartLevel > 14)
            StartLevel = 1;
    }
}

static int skip_intro = 1;

/* 014e */ char far pascal PlayOpeningSequence(void)
{
    int result;
    char level;

    HeapPurge(0x00ffffffL); HeapCompact(0x00ffffffL);
    result = 1;
    if (!skip_intro) {
        if (GameState.skip_opening)
            result = 2;
        else if (DoCreditsDialog() || ShowHallOfFame(0))
            result = 2;
    } else {
        skip_intro = 0;
    }
    if (result != 2)
        result = PlayNIS(7);
next_opening_clip:
    if (result != 2) result = PlayNIS(4);
    if (result != 2) result = PlayNIS(8);
    if (result == 2) {
        StartLevel = 1;
        skip_intro = 1;
        TurnRecordedGamePlayerOff();
    } else {
        TurnRecordedGamePlayerOn();
    }
    level = StartLevel;
    reset_level_overlays();
    if (nis_skip_requested)
        level = poll_and_clear_pending_state();
    return level;
}

/* 0210 */ char far pascal poll_and_clear_pending_state(void)
{
    char level = StartLevel;

    if (ResumeGameDialog() > 0) {
        level = StartLevel = LevelIdx;
        RestartLevel = 0;
    }
    nis_skip_requested = 0;
    return level;
}

extern ROOMREC Room;
extern signed char Joy[3];
#define joystick_y (Joy[0])
#define joystick_x (Joy[1])
#define joystick_button (Joy[2])
extern signed char quote_index;
extern int Minutes, TimeRequest, BackToLife;
extern int NextLevel, MsgTimer, MsgTimerStart;
extern unsigned int SixtiethsCtr;
extern char sound_on_message[], sound_off_message[], joystick_unavailable_message[], joystick_mode_message[], joystick_missing_message[], keyboard_mode_message[];

extern unsigned int far pascal PollUser0(void);
extern int far pascal IsValidTimeForDialog(void);
extern int far pascal IsValidTimeForSaveGame(void);
extern void far pascal SaveGameDialog(void);
extern void far pascal main_menu_dialog(void);
extern void far pascal LoadKid(void);
extern int far pascal toggle_sound(void);
extern int far pascal ShowHallOfFame(int mode);
extern char *far pascal joystick_status(void);
extern int far pascal ResumeGameDialog(void);
extern const char *far pascal toggle_ambient(void);
extern int far pascal IsLeaveLevelMusicOn(void);
extern void far pascal DrawMessage(const char *message);
extern void far pascal ResetSnd(int mode);
extern int far pascal IsAltarShadowActive(void);
extern void far pascal ClearKeyboard(void);
extern void far pascal CheckCheatKeys(unsigned int key);

unsigned int far pascal PrincePoll(void)
{
    const char *message = 0; int restart = 0; unsigned int key = PollUser0();

    if (joystick_button || key) {
        if ((Minutes && Kid.alive > 6) || GameState.recording_mode) { restart = 1;
            if (GameState.recording_mode) GameState.skip_opening = 1;
        }
    }

    if (key) {
        ClearKeyboard(); switch (key) {
        case 0x1b: Pause = 1; break;
        case 0x20: if (quote_index >= 0) TimeRequest = 1; break;
        case 0x1300:
            StartLevel = 0; quote_index = -1;
        case 0x1e00:
            if (!IsLeaveLevelMusicOn()) restart = 1; break;
        case 0x1800: if (IsValidTimeForDialog() && !GameState.recording_mode) {
                main_menu_dialog();
                LoadKid();
            } break;
        case 0x1f00: message = toggle_sound() ? sound_on_message : sound_off_message; break;
        case 0x2200:
            if (!IsValidTimeForDialog()) break;
            if (!IsValidTimeForSaveGame()) break;
            SaveGameDialog(); LoadKid(); break;
        case 0x2300:
        /* This case starts a separate CodeView record. */
        shortcut_dialog:
            if (IsValidTimeForDialog()) ShowHallOfFame(1); break;
        case 0x2400: message = joystick_status(); break;
        case 0x2500:
            /* The keyboard label follows the audio and Joy strings. */
            message = keyboard_mode_message; GameState.control_mode = 0; break;
        case 0x2600:
            if (!GameState.recording_mode) {
                if (IsValidTimeForDialog()) ResumeGameDialog();
                LoadKid();
            } else {
                nis_skip_requested = 1;
            }
            break;
        case 0x2f00: message = title; break;
        case 0x3100:
            if (LevelIdx <= 3 || frame_diagnostics) { if (LevelIdx == 14 && frame_diagnostics) {
                    NextLevel = 1;
                } else {
                    NextLevel = LevelIdx + 1;
                    if (!frame_diagnostics && Minutes > 15) {
                        Minutes = 15; SixtiethsCtr = 719; }
                }
                ResetSnd(0);
            } break;
        case 0x3200: message = toggle_ambient(); break;
        }

        if (frame_diagnostics) CheckCheatKeys(key);
        if (message) { DrawMessage(message); MsgTimer = MsgTimerStart = 24;
        }
    }
    if (restart && BackToLife == 0 &&
        (Room.number != 4 || Level.level_number != 13 || !IsAltarShadowActive())) {
        RestartLevel = 1; ResetSnd(0);
    }
    return key;
}

extern int Minutes, MsgTimer, MsgTimerStart, TimeRequest;
extern int pause_on_frame, show_actor_position, show_reference_position;
extern int show_ruler, debug_show_door_links, debug_show_meters;
extern struct { int y, x; } DebugKid;

extern int far cdecl sprintf(char *buffer, const char *format, ...);
extern void far pascal DrawMessage(const char *);
extern void far pascal LoadCustomRoomData(unsigned char room);
extern void far pascal EraseRect(const char far *asset);
extern void far pascal ToggleInvertState(void);
extern void far pascal AddSong(int value);
extern void far pascal AddSound(int value);
extern unsigned char CutRoom;
extern void far pascal ChangeMeters(void);
extern void far pascal CheckMeters(void);
extern void far pascal LoadKid(void);
extern char far pascal BoostMeter(void);
extern void far pascal GoWeightless(void);
extern void far pascal LoadShad(signed char opponent);
extern void far pascal SaveShad(void);
extern void far pascal JumpSeq(int sequence);
extern void far pascal BonesFall(void);
extern void far pascal SetSkelTimer(signed char opponent, int value);
extern void far pascal CheckShadowmanCreate(void);
extern void far pascal SetPalette(int duration, int range, unsigned char color, unsigned char mode);
extern void far pascal DrawOppMeter(signed char id, int value, int flags);
extern void far pascal ClrMessage(int restore);
extern int far pascal ToggleRecorder(void);
extern int far pascal AddCharX(int amount);

extern char GamePlayRect[];
extern void *cdecl memcpy(void *destination, const void *source, unsigned int size);
#pragma intrinsic(memcpy)

static struct {
    char room_number_format[8];
    char debug_cheat_message[30];
    char recording_on_message[10];
    char recording_off_message[11];
    char say_cheese_option[10];
    char kid_x_saved_message[12];
    char kid_x_restored_message[15];
    char kid_y_saved_message[12];
    char kid_y_restored_message[15];
} debug_messages = {
    "Room %d",
    "Called via program cheat key.",
    "PLAYER ON",
    "PLAYER OFF",
    "SAYCHEESE",
    "Kid.X saved",
    "Kid.X restored",
    "Kid.Y saved",
    "Kid.Y restored",
};

void far pascal CheckCheatKeys(unsigned int key)
{
    char text[80];
    char *message = 0;
    signed char index, count;

    switch (key) {
    case '+':
        ++Minutes; MsgTimer = MsgTimerStart = 0;
        TimeRequest = 1; break;
    case '-':
        if (Minutes > 1) --Minutes;
        MsgTimer = MsgTimerStart = 0;
        TimeRequest = 1; break;
    case 'B':
        BlackOut = !BlackOut;
        if (BlackOut) EraseRect(GamePlayRect);
        else {
            DoBlackOut = 1; LoadCustomRoomData(Room.number);
        }
        index = 0; count = Level.room_objects[Room.number - 1].opponent_count;
        if (index < count) do {
            RoomOpp[index].previous_x = 0;
        } while (++index < count);
        break;
    case 'I': ToggleInvertState(); break;
    case 'K':
        Char.life_delta = -1; AddSound(31);
        Kid = Char; ChangeMeters();
        CheckMeters(); LoadKid();
        if (Char.life_points == 0) JumpSeq(71);
        Char.life_delta = 0; break;
    case 'R':
        sprintf(text, debug_messages.room_number_format, Room.number); DrawMessage(text);
        MsgTimer = MsgTimerStart = 24; break;
    case 'S':
        if (Level.environment_kind == 2 || Level.environment_kind == 6) {
            KidInfo.shadow_timer = 7; Char.sequence = 5;
            CheckShadowmanCreate();
            if (Level.environment_kind == 6) SetPalette(2000, 48, 16, 1);
            else
                SetPalette(2000, 48, 16, 0);
        }
        break;
    case 'T':
        AddSong(101); BoostMeter(); Kid = Char; break;
    case 'W': GoWeightless(); break;
    case 'g':
        Kid = Char;
        if (Char.opponent_index != -1) { LoadShad(Char.opponent_index); ++Char.life_initial;
            Char.life_delta = 1; SaveShad();
            ChangeMeters(); CheckMeters();
        }
        LoadKid(); break;
    case 'k':
        Char.opponent_index = -1; Kid = Char;
        index = 0; count = Level.room_objects[Room.number - 1].opponent_count;
        if (index < count) do {
            LoadShad(index); Char.alive = 0;
            Char.life_delta = -Char.life_points;
            if (Level.preferred_opponent == 2) { BonesFall();
            } else if (Char.character_kind == 10) { SetSkelTimer(index, 480); JumpSeq(107);
                Char.repeat = 1;
            } else if (Level.preferred_opponent == 5 || Level.preferred_opponent == 6) {
                Char.direction = ~Kid.direction; Char.life_points = 0;
                JumpSeq(154);
            } else {
                JumpSeq(85);
            }
            SaveShad();
        } while (++index < count);
        LoadKid(); break;
    case 'r':
        if (Kid.alive > 0) {
            BackToLife = 20;
            Kid.alive = -1;
            ClrMessage(1);
        }
        break;
    case 'v':
        text[0] = 0;
        break;
    case 0x2000:
        dump_heap(0, debug_messages.debug_cheat_message, 0);
        break;
    case 0x3b00:
        pause_on_frame = !pause_on_frame;
        break;
    case 0x3c00:
        show_actor_position = !show_actor_position;
        break;
    case 0x3d00:
        message = (char *)(ToggleRecorder() ? debug_messages.recording_on_message : debug_messages.recording_off_message);
        break;
    case 0x3f00:
        show_reference_position = !show_reference_position;
        break;
    case 0x4000:
        show_ruler = !show_ruler;
        break;
    case 0x4100:
        debug_show_door_links = !debug_show_door_links;
        break;
    case 0x4200:
        debug_show_meters = !debug_show_meters;
        break;
    }

    if (find_option(debug_messages.say_cheese_option)) {
        switch (key) {
        case 0x4800: CutRoom = Room.above; break;
        case 0x4b00: CutRoom = Room.left; break;
        case 0x4d00: CutRoom = Room.right; break;
        case 0x5000: CutRoom = Room.below; break;
        }
        if (CutRoom)
            DrawOppMeter(-1, 0, 8);
    }

    if (GameState.control_mode) {
        switch (key) {
        case '1':
            LoadKid();
            DebugKid.x = Char.x;
            Kid = Char;
            memcpy(text, debug_messages.kid_x_saved_message, sizeof(debug_messages.kid_x_saved_message));
            message = text;
            break;
        case '2':
            LoadKid();
            Char.x = DebugKid.x;
            Kid = Char;
            memcpy(text, debug_messages.kid_x_restored_message, sizeof(debug_messages.kid_x_restored_message));
            message = text;
            break;
        case '3':
            LoadKid();
            DebugKid.y = Char.y;
            Kid = Char;
            memcpy(text, debug_messages.kid_y_saved_message, sizeof(debug_messages.kid_y_saved_message));
            message = text;
            break;
        case '4':
            LoadKid();
            Char.y = DebugKid.y;
            Kid = Char;
            memcpy(text, debug_messages.kid_y_restored_message, sizeof(debug_messages.kid_y_restored_message));
            message = text;
            break;
        case 0x4700:
            LoadKid();
            if (Char.direction == -1)
                Char.x = AddCharX(32);
            else
                Char.x = AddCharX(-32);
            Kid = Char;
            break;
        case 0x4900:
            LoadKid();
            if (Char.direction == 0)
                Char.x = AddCharX(32);
            else
                Char.x = AddCharX(-32);
            Kid = Char;
            break;
        case 0x4b00:
            LoadKid();
            if (Char.direction == -1)
                Char.x = AddCharX(1);
            else
                Char.x = AddCharX(-1);
            Kid = Char;
            break;
        case 0x4d00:
            LoadKid();
            if (Char.direction == 0)
                Char.x = AddCharX(1);
            else
                Char.x = AddCharX(-1);
            Kid = Char;
            break;
        case 0x4f00:
            LoadKid();
            if (Char.direction == -1)
                Char.x = AddCharX(5);
            else
                Char.x = AddCharX(-5);
            Kid = Char;
            break;
        case 0x5100:
            LoadKid();
            if (Char.direction == 0)
                Char.x = AddCharX(5);
            else
                Char.x = AddCharX(-5);
            Kid = Char;
            break;
        }
    }
    if (message) {
        DrawMessage(message);
        MsgTimer = MsgTimerStart = 24;
    }
}

#undef debug_char
#undef debug_kid

extern char one_second_left[], seconds_left[], minutes_left[], time_expired[];

extern void far pascal SET_SOUND_LEVEL(signed char level);
extern void far pascal SaveOptionsToDisk(void);

int far pascal toggle_sound(void)
{
    int enabled;

    if (Midi.volume == 0) {
        SET_SOUND_LEVEL(15);
        enabled = 1;
    } else {
        SET_SOUND_LEVEL(0);
        enabled = 0;
    }
    SaveOptionsToDisk();
    return enabled;
}

#pragma pack(1)
typedef struct {
    int x, y;
    int center_x, center_y;
    signed char button;
} GameportState;
#pragma pack()
extern GameportState joystick_state;
extern int far pascal check_game_port(void);
extern void *cdecl memset(void *destination, int value, unsigned int size);
#pragma intrinsic(memset)

char *far pascal joystick_status(void)
{
    char *message;
    if ((Midi.channels & 1) && configuration->midi_type == 32) {
        message = joystick_unavailable_message;
    } else {
        configuration->control_mode = 2;
        if (check_game_port()) {
            message = joystick_mode_message;
            configuration->control_mode = GameState.control_mode = 2;
        } else {
            message = joystick_missing_message;
            configuration->control_mode = GameState.control_mode = 0;
            memset(&joystick_state, 0, sizeof(joystick_state));
        }
    }
    return message;
}

extern void far pascal SetSideRoomInfo(void);
extern void far pascal select_room_arrays(unsigned char room);
extern unsigned char far pascal FirstInRow(int tile);
extern unsigned char far pascal GetOS(int tile);
extern void far pascal CheckSpikesJammed(unsigned char room, int tile);
extern void far pascal TriggerFlask(unsigned char room, signed char tile);
extern void far pascal TriggerTorch(unsigned char room, signed char tile);
extern void far pascal TriggerLava(unsigned char room, signed char tile);
extern void far pascal AddTrob(unsigned char room, signed char tile,
                                      signed char active, unsigned char type);
extern int far pascal TriggerStone(unsigned char room, signed char tile, signed char delay);
extern void far pascal TriggerBrgFloor(unsigned char room, signed char tile);
extern void far pascal TriggerFinlCrystal(unsigned char room, signed char tile);
extern void far pascal TriggerFinlGargoyle(unsigned char room, signed char tile);
#pragma pack(1)
typedef struct {
    unsigned char bytes[4];
} TileInfo;

typedef struct {
    unsigned char tile;
    TileInfo info;
} Tile;
#pragma pack()
extern TileInfo BlueSpec;
extern Tile ScrLeft[];

void far pascal AddRoomTrobs(void)
{
    int index;
    unsigned char room;
    signed char tile;
    unsigned char kind;

    SetSideRoomInfo();
    room = Room.number;
    index = 0;
    tile = 0;
    do {
        if (index < 30) {
            kind = GetOS(index);
        } else {
            if (index == 30) {
                if (Room.left) {
                    room = Room.left;
                    select_room_arrays(room);
                } else {
                    index += 3;
                }
            }
            if (index == 33) {
                if (!Room.below)
                    break;
                room = Room.below;
                select_room_arrays(room);
            }
            if (index < 33) {
                tile = FirstInRow(index - 30) + 9;
                kind = ScrLeft[index - 30].tile;
            } else {
                tile = index - 33;
                kind = GetOS(tile);
            }
        }

        switch (kind) {
        default:
            if (room == 6 && Level.environment_kind == 6 && (BlueSpec.bytes[1] & 16)) {
                TriggerFinlCrystal(room, tile);
            } else if ((room == 7 || room == 8) &&
                       Level.environment_kind == 6 && (BlueSpec.bytes[1] & 16)) {
                TriggerFinlGargoyle(room, tile);
            }
            break;
        case 2:
            CheckSpikesJammed(room, tile);
            break;
        case 10:
            if (index < 33)
                TriggerFlask(room, tile);
            break;
        case 19:
        case 32:
            TriggerTorch(room, tile);
            break;
        case 23:
            if (index < 33)
                TriggerLava(room, tile);
            break;
        case 28:
        case 29:
        case 31:
        case 37:
        case 38:
        case 39:
        case 40:
        case 43:
            AddTrob(room, tile, 1, kind);
            break;
        case 30:
            TriggerStone(room, tile, 3);
            break;
        case 44:
            TriggerBrgFloor(room, tile);
            break;
        }
        ++tile;
    } while (++index < 43);
    select_room_arrays(Room.number);
}

extern signed char quote_index;
extern int Minutes, TimeRequest;
extern unsigned int SixtiethsCtr;
extern int MsgTimer, MsgTimerStart;
extern void far pascal DrawMessage(const char *message);
extern int far cdecl sprintf(char *buffer, const char *format, ...);
extern char *cdecl strcpy(char *destination, const char *source);
#pragma intrinsic(strcpy)

void far pascal KeepTime(void)
{
    char text[40];
    unsigned int seconds;

    if (quote_index >= 0 && Kid.alive < 0 && Minutes != 0) {
        if (--SixtiethsCtr == 0) {
            SixtiethsCtr = 719;
            if (--Minutes != 0 &&
                (Minutes < 5 || Minutes % 5 == 0))
                TimeRequest = 1;
        } else if (Minutes == 1 && SixtiethsCtr % 12 == 0) {
            TimeRequest = 1;
            MsgTimer = MsgTimerStart = 0;
        }
    }

    if (MsgTimer)
        return;
    if (Resume) {
        LastResume = Resume;
        Resume = 0;
        return;
    }
    if (TimeRequest) {
        if (Minutes > 0) {
            if (Minutes == 1) {
                seconds = (SixtiethsCtr + 1) / 12;
                if (seconds == 1)
                    strcpy(text, one_second_left);
                else
                    sprintf(text, seconds_left, seconds);
            } else {
                sprintf(text, minutes_left, Minutes);
            }
            DrawMessage(text);
        } else {
            DrawMessage(time_expired);
        }
        MsgTimer = MsgTimerStart = 24;
        TimeRequest = 0;
    }
}

extern int CutPlan;
extern unsigned char OppBank[2];

extern void far pascal GetRooms(void);
extern int far pascal IsLevelStartWithNISScreen(void);
extern void far pascal SaveGamePlayPalette(void);
extern void far pascal LoadCustomRoomData(unsigned char room);
extern void far pascal PreloadRoomData(unsigned char room);
extern void far pascal AddOpponent(void);
extern signed char far pascal GetOppIdxClosestToKid(signed char mode);
extern void far pascal LoadKid(void);
extern void *cdecl memset(void *destination, int value, unsigned int size);
#pragma intrinsic(memset)

int far pascal PrepCut(int keep_screen)
{
    signed char result = -2;
    if (CutRoom && Room.number != CutRoom) {
        Room.number = CutRoom;
        GetRooms();
        if (!keep_screen || !IsLevelStartWithNISScreen())
            SaveGamePlayPalette();
        LoadCustomRoomData(Room.number);
        CutPlan = 1;
        LoadKid();
        AddRoomTrobs();
        memset(OppBank, 0, sizeof(OppBank));
        PreloadRoomData(Room.number);
        AddOpponent();
        if (!Level.room_objects[Room.number - 1].opponent_count)
            DrawOppMeter(-1, 0, 8);
        else
            Kid.opponent_index = GetOppIdxClosestToKid(1);
    }
    return result;
}

extern void far pascal LoadKid(void);
extern void far pascal DrawKidMeter(int previous, int current);
extern unsigned long FrameNum;
extern unsigned int SixtiethsCtr;

/* Increment the signed state byte up to 12, then refresh the step delta. */
char far pascal UPDATE_STEP_DELTA(void);

char far pascal BoostMeter(void)
{
    int next = Char.life_initial + 1;
    Char.life_initial = (next > 12) ? 12 : next;
    return UPDATE_STEP_DELTA();
}

char far pascal UPDATE_STEP_DELTA(void)
{
    Char.life_delta = Char.life_initial - Char.life_points;
    return Char.life_delta;
}

void far pascal CheckMeters(void)
{
    LoadKid();
    if (Char.life_delta)
        DrawKidMeter(Char.life_points, Char.life_initial);
    if (Char.life_points == 1) {
        if (FrameNum & 1)
            DrawKidMeter(1, 0);
        else
            DrawKidMeter(0, 1);
    }
    Kid = Char;
    if (Kid.opponent_index != -1) {
        LoadShad(Kid.opponent_index);
        if (Char.life_delta && !((Char.character_kind == 7 || Char.character_kind == 8) && Char.sequence_group == 2))
            DrawOppMeter(Char.slot, Char.life_points, Char.life_initial);
        if (Char.life_points == 1 && !((Char.character_kind == 7 || Char.character_kind == 8) && Char.sequence_group == 2)) {
            if (SixtiethsCtr & 1)
                DrawOppMeter(Char.slot, 1, Char.life_initial);
            else
                DrawOppMeter(Char.slot, 0, Char.life_initial);
        }
    }
}

extern void far pascal CheckShadowPaletteChange(void);

void far pascal ChangeMeters(void)
{
    int hp;
    signed char opponent, count;
    CHARREC *meter;

    if (Kid.life_delta) {
        if (Kid.character_kind == 1 && Level.environment_kind == 6)
            CheckShadowPaletteChange();
        hp = Kid.life_points + Kid.life_delta;
        hp = hp < 0 ? 0 : hp;
        hp = hp > Kid.life_initial ? Kid.life_initial : hp;
        Kid.life_points = hp;
    }
    opponent = 0;
    count = Level.room_objects[Room.number - 1].opponent_count;
    if (count > 0) {
        do {
            meter = &RoomOpp[opponent];
            if (meter->life_delta) {
                hp = meter->life_points + meter->life_delta;
                hp = hp < 0 ? 0 : hp;
                hp = hp > meter->life_initial ? meter->life_initial : hp;
                meter->life_points = hp;
            }
        } while (++opponent < count);
    }
}

extern signed char Joy[3];
extern char paused_message[];

extern void far pascal read_controls(void);
extern unsigned int far pascal PrincePoll(void);
extern void far pascal DrawMessage(const char *message);
extern void far pascal ResetSnd(int mode);
extern void far pascal WaitKey(void);
extern void far pascal ClrMessage(int restore);
extern void *cdecl memset(void *buffer, int value, unsigned int size);
#pragma intrinsic(memset)

int far pascal Input(void)
{
    int result;

    CutRoom = 0;
    memset(Joy, 0, sizeof(Joy));
    read_controls();
    result = PrincePoll();
    if (Pause) {
        Pause = 0;
        DrawMessage(paused_message);
        ResetSnd(0);
        WaitKey();
        ClrMessage(1);
    }
    if (result == 0)
        result = Joy[2];
    return result;
}

extern unsigned char key_down[];
extern void far pascal POLL_GAME_PORT(void);

#define keyboard_flags (*(unsigned char far *)0x00400017UL)

void far pascal read_gameport_controls(void);
void far pascal read_keyboard_controls(void);

/* 105a */ void far pascal read_controls(void)
{
    if (GameState.control_mode == 2)
        read_gameport_controls();
    else
        read_keyboard_controls();
}

void far pascal read_gameport_controls(void)
{
    int displacement;
    int threshold;

    POLL_GAME_PORT();
    threshold = *(volatile int *)&joystick_state.center_y / 2;
    displacement = joystick_state.y - joystick_state.center_y;
    if (displacement > threshold)
        joystick_y = 1;
    else if (displacement < -threshold)
        joystick_y = -1;

    threshold = *(volatile int *)&joystick_state.center_x / 2;
    displacement = joystick_state.x - joystick_state.center_x;
    if (displacement > threshold)
        joystick_x = 1;
    else if (-threshold > displacement)
        joystick_x = -1;

    joystick_button = -joystick_state.button;
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

extern int far pascal POLL_DOS_KEY(void);
extern void far pascal post_pointer_event(int type, int data, int key);

void far pascal ClearKeyboard(void)
{
    int key;

    if (GameState.control_mode == 0) {
        do {
            key = POLL_DOS_KEY();
            if (key)
                post_pointer_event(8, 0, key);
        } while (key);
    }
}

/* PRINCE.C, resident DOS entries 12de through 133f. */
extern void far pascal ResetSnd(int mode);
extern int far pascal InitPort(void near *port, void far *buffer);
extern void far pascal ZeroPeels(void);
extern unsigned char ScreenPort[];
extern unsigned char ScreenPortRect[];
extern int Graf;
extern char LevelIdx;

extern unsigned int Invert;

extern unsigned char ScreenPort[];
extern void far pascal CopyBits(void near *port, unsigned int source,
                                     const Rect far *source_rect,
                                     const Rect far *dest_rect, int flags);

void far pascal PrinceCopy(const Rect far *rect)
{
    Rect flipped;
    Rect far *copy_rect;

    if (Invert != 0) {
        copy_rect = &flipped;
        *copy_rect = *rect;
        copy_rect->top = 192 - rect->bottom;
        flipped.bottom = 192 - rect->top;
    } else {
        copy_rect = (Rect far *)rect;
    }

    CopyBits(ScreenPort, BPort, copy_rect, copy_rect, 0);
}

void far pascal ToggleInvertState(void)
{
    if (Invert != 0) {
        Invert = 0;
        AddSong(154);
    } else {
        Invert = 1080;
        AddSong(153);
    }
    RedrawFlag = 1;
}

/* Restrict the step value, then request mode 105 unless already pending. */
extern unsigned int WeightlessTimer;
extern unsigned int current_effect;
extern int CueSnd;

void far pascal GoWeightless(void)
{
    WeightlessTimer = 228;
    AddSong(105);
    ResetSnd(current_effect + 10000);
    CueSnd = -1;
}

/* 1316 */ void far pascal ResetData(void)
{
    ResetSnd(0);
    Graf = InitPort(ScreenPort, ScreenPortRect);
    ZeroPeels();
    LevelIdx = -1;
    LastResume = 0;
}

/* Window callback that ignores its arguments. */
void far pascal ignore_window_action(void *window, int action)
{
}
