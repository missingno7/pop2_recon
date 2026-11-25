/* Resident PRINCE.C, DOS root [0000,1340). */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "gameport.h"
#include "types.h"
#include "snd.h"
#include "graf.h"
#include "shape.h"
#include "level.h"
#include "heap.h"
#include "char.h"
#include "game.h"
#include "draw.h"
#include "custom.h"
#include "movecave.h"
#include "act.h"
#include "add.h"
#include "altar.h"
#include "movespik.h"
#include "custcvca.h"
#include "autoguar.h"
#include "autoshad.h"
#include "bridge.h"
#include "checknis.h"
#include "ctrl.h"
#include "desert.h"
#include "diag.h"
#include "dlghelp.h"
#include "dialog.h"
#include "english.h"
#include "fade.h"
#include "system.h"
#include "final.h"
#include "auto.h"
#include "init.h"
#include "intrface.h"
#include "load.h"
#include "misc.h"
#include "move.h"
#include "anim.h"
#include "resc.h"
#include "nis.h"
#include "prince.h"
#include "recorder.h"
#include "sound.h"
#include "top.h"
#include "util.h"


/* The state of the game. RTLink lays these out first, in this order, at DGROUP 2b90. */
int DoBlackOut, BlackOut;
PORT *main_port;
int Pause;
AmbientState ambient_state;
int unused_state;                       /* never referenced */
GAMESTATE GameState;
LEVELREC Level;
REBIRTHREC far *hRebirthSaved;
CHARREC Char, Opp, Kid, RoomOpp[5];
int Resume;
KIDINFO KidInfo;
int LastResume;
PORT *BPort;
word Player;
FRAMEREC Frame;
word RedrawFlag;
static char title[] = "PRINCE OF PERSIA 2 v1.1";
/* FM Towns names these adjacent lookup tables OppCharId and CharOppId. */
byte OppCharId[11] = { 2, 10, 4, 6, 1, 7, 8, 2, 11, 12, 0 };
byte CharOppId[15] = {
    10, 4, 0, 0, 2, 0, 3, 5, 6, 0, 1, 8, 9, 0, 0
};


/* 0000 */ void far cdecl main(void)
{
    const char **file;
    char message[100];

    InitSystem();
    if (find_option("sbdiag")) {
        sprintf(message, "DType:%d MType:%d\rDDrv:%d MDrv: %d\rDInit:%d MInit:%d",
                configuration->digital_mode != 0, configuration->midi_mode != 0,
                digital_driver ? 1 : 0, midi_driver ? 1 : 0,
                (Midi.channels & 1) != 0, (Midi.channels & 2) != 0);
        show_init_message(message);
    }
    SetOptionsFromDisk();
    SetStartLevel();
    for (file = resource_files + 1; file < resource_files + 3; ++file) {
        OpenRsrcVerify(*file);
    }
    Rnd(1);
    Level.environment_kind = 0;
    ResetGame();
}

/* 0036 */ void ResetGame(void)
{
    sbyte level = StartLevel;

    for (;;) {
        if (frame_diagnostics) {
            if (find_option("WATCHMEM")) {
                const char * const restart_message = "At beginning of ResetGame() loop.";
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

/* 0116 */ void SetStartLevel(void)
{
    char *option;

    StartLevel = 0;
    if (frame_diagnostics) {
        option = find_option("LEVEL");
        if (option == 0)
            return;
        StartLevel = atoi(&option[5]);
        if (StartLevel < 1 || StartLevel > 14)
            StartLevel = 1;
    }
}

static int skip_intro = 1;

/* 014e */ char PlayOpeningSequence(void)
{
    int result;
    sbyte level;

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

/* 0210 */ char poll_and_clear_pending_state(void)
{
    sbyte level = StartLevel;

    if (ResumeGameDialog() > 0) {
        level = StartLevel = LevelIdx;
        RestartLevel = 0;
    }
    nis_skip_requested = 0;
    return level;
}


word PrincePoll(void)
{
    const char *message = 0; int restart = 0; word key = PollUser0();

    if (Joy.button || key) {
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

void CheckCheatKeys(word key)
{
    char text[80];
    char *message = 0;
    sbyte index, count;

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
        if (BlackOut) EraseRect(&GamePlayRect);
        else {
            DoBlackOut = 1; LoadCustomRoomData(Room.number);
        }
        index = 0; count = Level.room_objects[Room.number - 1].opponent_count;
        for (; index < count; ++index) {
            RoomOpp[index].previous_x = 0;
        }
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
        for (; index < count; ++index) {
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
        }
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


int toggle_sound(void)
{
    int enabled;

    if (Midi.volume == 0) {
        set_sound_level(15);
        enabled = 1;
    } else {
        set_sound_level(0);
        enabled = 0;
    }
    SaveOptionsToDisk();
    return enabled;
}


sbyte *joystick_status(void)
{
    char *message;
    if ((Midi.channels & 1) && configuration->midi_mode == 32) {
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


void AddRoomTrobs(void)
{
    int index;
    byte room;
    sbyte tile;
    byte kind;

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
                kind = ScrLeft[index - 30].kind;
            } else {
                tile = index - 33;
                kind = GetOS(tile);
            }
        }

        switch (kind) {
        default:
            if (room == 6 && Level.environment_kind == 6 && (((byte *)&BlueSpec.modifier)[1] & 16)) {
                TriggerFinlCrystal(room, tile);
            } else if ((room == 7 || room == 8) &&
                       Level.environment_kind == 6 && (((byte *)&BlueSpec.modifier)[1] & 16)) {
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


void KeepTime(void)
{
    char text[40];
    word seconds;

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


int PrepCut(int keep_screen)
{
    sbyte result = -2;
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


sbyte BoostMeter(void)
{
    int next = Char.life_initial + 1;
    Char.life_initial = (next > 12) ? 12 : next;
    return update_step_delta();
}

sbyte update_step_delta(void)
{
    Char.life_delta = Char.life_initial - Char.life_points;
    return Char.life_delta;
}

void CheckMeters(void)
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


void ChangeMeters(void)
{
    int hp;
    sbyte opponent, count;
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


int Input(void)
{
    int result;

    CutRoom = 0;
    memset(&Joy, 0, sizeof(Joy));
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
        result = Joy.button;
    return result;
}


#define keyboard_flags (*(byte far *)0x00400017UL)


/* 105a */ void read_controls(void)
{
    if (GameState.control_mode == 2)
        read_gameport_controls();
    else
        read_keyboard_controls();
}

void read_gameport_controls(void)
{
    int displacement;
    int threshold;

    poll_game_port();
    threshold = *(volatile int *)&joystick_state.center_x / 2;
    displacement = joystick_state.x - joystick_state.center_x;
    if (displacement > threshold)
        Joy.x = 1;
    else if (displacement < -threshold)
        Joy.x = -1;

    threshold = *(volatile int *)&joystick_state.center_y / 2;
    displacement = joystick_state.y - joystick_state.center_y;
    if (displacement > threshold)
        Joy.y = 1;
    else if (-threshold > displacement)
        Joy.y = -1;

    Joy.button = -joystick_state.button;
    if (Joy.button < -2)
        Joy.button = -1;
}

/* 10d8 */ void read_keyboard_controls(void)
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
            Joy.y = 1;
    } else {
        Joy.y = -1;
    }

    if (!(key_down[0x4b] || key_down[0x24] || key_down[0x1f] ||
          key_down[0x47] || key_down[0x16] || key_down[0x11] ||
          key_down[0x4f] || key_down[0x32] || key_down[0x2d])) {
        if (key_down[0x4d] || key_down[0x26] || key_down[0x21] ||
            key_down[0x49] || key_down[0x18] || key_down[0x13] ||
            key_down[0x51] || key_down[0x34] || key_down[0x2f])
            Joy.x = 1;
    } else {
        Joy.x = -1;
    }

    keyboard_flags &= 0x0f;
    if (!(keyboard_flags & 3) && !key_down[0x53]) {
        if ((keyboard_flags & 4) || key_down[0x52])
            Joy.button = -2;
    } else {
        Joy.button = -1;
    }
}


void ClearKeyboard(void)
{
    int key;

    if (GameState.control_mode == 0) {
        do {
            key = poll_dos_key();
            if (key)
                post_pointer_event(8, 0, key);
        } while (key);
    }
}


void PrinceCopy(Rect far *rect)
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

    CopyBits(&ScreenPort, BPort, copy_rect, copy_rect, 0);
}

void ToggleInvertState(void)
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


void GoWeightless(void)
{
    WeightlessTimer = 228;
    AddSong(105);
    ResetSnd(current_effect + 10000);
    CueSnd = -1;
}

/* 1316 */ void ResetData(void)
{
    ResetSnd(0);
    Graf = InitPort(&ScreenPort, &ScreenPortRect);
    ZeroPeels();
    LevelIdx = -1;
    LastResume = 0;
}

/* Window callback that ignores its arguments. */
void ignore_window_action(void *window, int action)
{
}
