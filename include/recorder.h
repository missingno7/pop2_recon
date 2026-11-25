/* Recorded games. */
#pragma pack(1)
typedef struct {
    sbyte level;
    int end_tick, data_size;
    sbyte data[1];
} Recording;

typedef struct {
    int tick;
    sbyte count;
} RecordedFrame;

typedef struct {                        /* Directions + 1, packed */
    word x : 2;
    word y : 2;
    word horizontal : 2;
    word vertical : 2;
    word action : 5;
} PackedDirection;
#pragma pack()

int CheckForEndOfRecording(void);
void CheckRecordedDirForChar(void);
void CheckRecordedGamePlayer(void);
void ConvertDirToRecDir(PackedDirection far *direction);
void ConvertRecDirToDir(PackedDirection far *direction);
RecordedFrame far *IncRecordedGameToNextFrame(Recording far *recording);
Handle LoadRecordedGameFromDisk(void);
void StartPlayingRecordedGame(void);
int ToggleRecorder(void);
void TurnRecordedGamePlayerOff(void);
void TurnRecordedGamePlayerOn(void);
