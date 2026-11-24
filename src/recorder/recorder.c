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

extern struct {
    int mode;
    int offset;
    int input_index;
    PackedDirection saved_directions[6];
    unsigned int handle;
} playback;

extern unsigned long FrameNum;
extern signed char current_character;
extern signed char LevelIdx;
extern signed char move_x, move_y, move_horizontal, move_vertical, move_action;
extern int recording_player_enabled, recording_player_started;
extern signed char recording_start_level;

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
        if (current_character == 10)
            character = 5;
        else
            character = current_character;
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
    direction->x = move_x + 1;
    direction->y = move_y + 1;
    direction->horizontal = move_horizontal + 1;
    direction->vertical = move_vertical + 1;
    direction->action = move_action + 2;
}

void far pascal ConvertRecDirToDir(PackedDirection far *direction)
{
    move_x = direction->x - 1;
    move_y = direction->y - 1;
    move_horizontal = direction->horizontal - 1;
    move_vertical = direction->vertical - 1;
    move_action = direction->action - 2;
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

extern unsigned int DEFAULT_HANDLE_TAG;
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
        if (recording_player_enabled)
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
    recording_player_enabled = 0;
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
    recording_player_enabled = 1;
    recording_player_started = 0;
    recording_start_level = recording->level;
    dispose_handle(handle);
}
