#pragma pack(1)
typedef struct {
    signed char level;
    int end_tick;
    int data_size;
    signed char data[1];
} Recording;

typedef struct {
    int tick;
    signed char count;
} RecordedFrame;

typedef struct {
    unsigned int x : 2;
    unsigned int y : 2;
    unsigned int horizontal : 2;
    unsigned int vertical : 2;
    unsigned int action : 5;
} PackedDirection;

typedef struct {
    signed char character;
    PackedDirection direction;
} RecordedInput;

static struct {
    int mode;
    int offset;
    int input_index;
    PackedDirection saved_directions[6];
    unsigned int handle;
} playback;

extern unsigned long FrameNum;
extern signed char Char;
extern signed char LevelIdx;
typedef struct {
    signed char forward, back, up, down, action;
} Directions;
extern Directions Dir;
typedef struct {
    int control_mode, fade_active, playing_nis, recording_mode, skip_opening, field_0a;
    int plateau_loaded, throne_song_added, throne_nis_played, escher_room_eight_seen, nis_room_restored;
} GAMESTATE;
extern GAMESTATE GameState;
extern signed char StartLevel;

extern void far *far pascal RESOLVE_HANDLE_TO_FAR_PAYLOAD(unsigned int handle);
extern void far pascal dispose_handle(unsigned int handle);
extern void far pascal abort_with_message(int exit_status, char far *message);
extern void far pascal SetRndSeed(int enabled);
extern int far pascal Rnd(int value);
extern void far pascal RemoveRebirthPt(void);
extern void *cdecl memset(void *, int, unsigned int);
#pragma intrinsic(memset)

void far pascal ConvertRecDirToDir(PackedDirection far *direction);
RecordedFrame far *far pascal IncRecordedGameToNextFrame(Recording far *recording);

int far pascal CheckForEndOfRecording(void)
{
    int result = -2;
    unsigned long tick;
    Recording far *recording;

    if (playback.mode == 3 && playback.handle) {
        recording = RESOLVE_HANDLE_TO_FAR_PAYLOAD(playback.handle);
        tick = FrameNum;
        if (tick > recording->end_tick) {
            playback.mode = 0;
            if (playback.handle) {
                dispose_handle(playback.handle);
                playback.handle = 0;
            }
            result = -1;
        }
    }
    return result;
}

void far pascal CheckRecordedDirForChar(void)
{
    Recording far *recording;
    RecordedFrame far *frame;
    RecordedInput far *input;
    PackedDirection far *direction;
    signed char character;

    if (playback.mode == 3 && playback.handle) {
        if (Char == 10)
            character = 5;
        else
            character = Char;
        recording = RESOLVE_HANDLE_TO_FAR_PAYLOAD(playback.handle);
        if (playback.offset < recording->data_size) {
            frame = (RecordedFrame far *)(recording->data + playback.offset);
            if (FrameNum > frame->tick)
                frame = IncRecordedGameToNextFrame(recording);
            if (FrameNum == frame->tick && frame->count > playback.input_index) {
                input = (RecordedInput far *)(frame + 1) + playback.input_index;
                if (input->character == character) {
                    direction = &input->direction;
                    ConvertRecDirToDir(direction);
                    playback.saved_directions[character] = *direction;
                    playback.input_index++;
                }
            }
        }
        ConvertRecDirToDir(&playback.saved_directions[character]);
    }
}

void far pascal CheckRecordedGamePlayer(void)
{
    if (playback.mode == 4)
        LevelIdx = -1;
}

void far pascal ConvertDirToRecDir(PackedDirection far *direction)
{
    direction->x = Dir.forward + 1;
    direction->y = Dir.back + 1;
    direction->horizontal = Dir.up + 1;
    direction->vertical = Dir.down + 1;
    direction->action = Dir.action + 2;
}

void far pascal ConvertRecDirToDir(PackedDirection far *direction)
{
    Dir.forward = direction->x - 1;
    Dir.back = direction->y - 1;
    Dir.up = direction->horizontal - 1;
    Dir.down = direction->vertical - 1;
    Dir.action = direction->action - 2;
}

RecordedFrame far *far pascal IncRecordedGameToNextFrame(Recording far *recording)
{
    RecordedFrame far *frame;

    int count;

    frame = (RecordedFrame far *)(recording->data + playback.offset);
    count = frame->count + 1;
    playback.offset += count * sizeof(RecordedInput);
    playback.input_index = 0;
    frame = (RecordedFrame far *)(recording->data + playback.offset);
    if (playback.mode == 3 && recording->data_size < playback.offset)
        abort_with_message(0, "Error playing recorded game");
    return frame;
}

static unsigned int DEFAULT_HANDLE_TAG;
extern unsigned int far pascal HGETRSRC(void far *value, unsigned int tag);

unsigned int far pascal ALLOCATE_DEFAULT_HANDLE(void)
{
    return HGETRSRC((void far *)0, DEFAULT_HANDLE_TAG);
}

void far pascal reset_recorded_playback(void)
{
    if (playback.mode == 4 || playback.mode == 3) {
        playback.mode = 3;
        playback.offset = 0;
        playback.input_index = 0;
        memset(playback.saved_directions, 0, sizeof(playback.saved_directions));
        if (playback.handle)
            dispose_handle(playback.handle);
        if (GameState.recording_mode)
            playback.handle = ALLOCATE_DEFAULT_HANDLE();
        FrameNum = 0;
        SetRndSeed(1);
        Rnd(17);
        RemoveRebirthPt();
    }
}

int far pascal toggle_recording_player(void)
{
    playback.mode = playback.mode != 4 ? 4 : 0;
    return playback.mode != 0;
}

void far pascal TurnRecordedGamePlayerOff(void)
{
    GameState.recording_mode = 0;
    playback.mode = 0;
    if (playback.handle) {
        dispose_handle(playback.handle);
        playback.handle = 0;
    }
}

void far pascal TurnRecordedGamePlayerOn(void)
{
    unsigned int handle;
    Recording far *recording;

    playback.mode = 4;
    if (!DEFAULT_HANDLE_TAG || (int)++DEFAULT_HANDLE_TAG >= 28)
        DEFAULT_HANDLE_TAG = 25;
    handle = ALLOCATE_DEFAULT_HANDLE();
    recording = RESOLVE_HANDLE_TO_FAR_PAYLOAD(handle);
    GameState.recording_mode = 1;
    GameState.skip_opening = 0;
    StartLevel = recording->level;
    dispose_handle(handle);
}
