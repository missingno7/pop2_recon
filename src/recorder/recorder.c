#include <string.h>
#include "types.h"
#include "resc.h"
#include "graf.h"
#include "level.h"
#include "heap.h"
#include "char.h"
#include "game.h"
#include "ctrl.h"
#include "dlghelp.h"
#include "dialog.h"
#include "intrface.h"
#include "prince.h"
#include "recorder.h"
#include "top.h"
#include "util.h"
#pragma pack(1)


typedef struct {
    sbyte character;
    PackedDirection direction;
} RecordedInput;

static struct {
    int mode;
    int offset;
    int input_index;
    PackedDirection saved_directions[6];
    Handle handle;
} playback;


int CheckForEndOfRecording(void)
{
    int result = -2;
    unsigned long tick;
    Recording far *recording;

    if (playback.mode == 3 && playback.handle) {
        recording = h2p(playback.handle);
        tick = FrameNum;
        if (tick > recording->end_tick) {
            playback.mode = 0;
            if (playback.handle) {
                hFree(playback.handle);
                playback.handle = 0;
            }
            result = -1;
        }
    }
    return result;
}

void CheckRecordedDirForChar(void)
{
    Recording far *recording;
    RecordedFrame far *frame;
    RecordedInput far *input;
    PackedDirection far *direction;
    sbyte character;

    if (playback.mode == 3 && playback.handle) {
        if (Char.slot == 10)
            character = 5;
        else
            character = Char.slot;
        recording = h2p(playback.handle);
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

void CheckRecordedGamePlayer(void)
{
    if (playback.mode == 4)
        LevelIdx = -1;
}

void ConvertDirToRecDir(PackedDirection far *direction)
{
    direction->x = Dir.forward + 1;
    direction->y = Dir.back + 1;
    direction->horizontal = Dir.up + 1;
    direction->vertical = Dir.down + 1;
    direction->action = Dir.action + 2;
}

void ConvertRecDirToDir(PackedDirection far *direction)
{
    Dir.forward = direction->x - 1;
    Dir.back = direction->y - 1;
    Dir.up = direction->horizontal - 1;
    Dir.down = direction->vertical - 1;
    Dir.action = direction->action - 2;
}

RecordedFrame far *IncRecordedGameToNextFrame(Recording far *recording)
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

static word DEFAULT_HANDLE_TAG;


Handle LoadRecordedGameFromDisk(void)
{
    return hGetRsrc(0, DEFAULT_HANDLE_TAG);
}

void StartPlayingRecordedGame(void)
{
    if (playback.mode == 4 || playback.mode == 3) {
        playback.mode = 3;
        playback.offset = 0;
        playback.input_index = 0;
        memset(playback.saved_directions, 0, sizeof(playback.saved_directions));
        if (playback.handle)
            hFree(playback.handle);
        if (GameState.recording_mode)
            playback.handle = LoadRecordedGameFromDisk();
        FrameNum = 0;
        SetRndSeed(1);
        Rnd(17);
        RemoveRebirthPt();
    }
}

int ToggleRecorder(void)
{
    playback.mode = playback.mode != 4 ? 4 : 0;
    return playback.mode != 0;
}

void TurnRecordedGamePlayerOff(void)
{
    GameState.recording_mode = 0;
    playback.mode = 0;
    if (playback.handle) {
        hFree(playback.handle);
        playback.handle = 0;
    }
}

void TurnRecordedGamePlayerOn(void)
{
    Handle handle;
    Recording far *recording;

    playback.mode = 4;
    if (!DEFAULT_HANDLE_TAG || (int)++DEFAULT_HANDLE_TAG >= 28)
        DEFAULT_HANDLE_TAG = 25;
    handle = LoadRecordedGameFromDisk();
    recording = h2p(handle);
    GameState.recording_mode = 1;
    GameState.skip_opening = 0;
    StartLevel = recording->level;
    hFree(handle);
}
