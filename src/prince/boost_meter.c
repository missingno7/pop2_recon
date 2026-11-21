extern void far pascal CtrlPlayer(void);
extern void far pascal select_character(char index);
extern void far pascal DrawKidMeter(int previous, int current);
extern void far pascal DrawOppMeter(char room, int previous, int current);
extern int video_ticks;
extern unsigned int ticks_remaining;

typedef struct {
    char room;
    char unknown1[5];
    char kind;
    char unknown7[29];
    int sequence;
    char rest[26];
} CharacterState;

extern CharacterState Char;
extern CharacterState Kid;
extern signed char opponent_index;

/* Increment the signed state byte up to 12, then refresh the step delta. */
extern unsigned char STATE_BYTE;
extern unsigned char STATE_BASE;
extern unsigned char STATE_DELTA;
char far pascal UPDATE_STEP_DELTA(void);

char far pascal BoostMeter(void)
{
    int next = (signed char)STATE_BYTE + 1;
    STATE_BYTE = (next > 12) ? 12 : next;
    return UPDATE_STEP_DELTA();
}

char far pascal UPDATE_STEP_DELTA(void)
{
    STATE_DELTA = STATE_BYTE - STATE_BASE;
    return STATE_DELTA;
}

void far pascal CheckMeters(void)
{
    CtrlPlayer();
    if (STATE_DELTA)
        DrawKidMeter((signed char)STATE_BASE, (signed char)STATE_BYTE);
    if (STATE_BASE == 1) {
        if (video_ticks % 2)
            DrawKidMeter(1, 0);
        else
            DrawKidMeter(0, 1);
    }
    Kid = Char;
    if (opponent_index != -1) {
        select_character(opponent_index);
        if (STATE_DELTA && !((Char.kind == 7 || Char.kind == 8) && Char.sequence == 2))
            DrawOppMeter(Char.room, (signed char)STATE_BASE, (signed char)STATE_BYTE);
        if (STATE_BASE == 1 && !((Char.kind == 7 || Char.kind == 8) && Char.sequence == 2)) {
            if (ticks_remaining & 1)
                DrawOppMeter(Char.room, 1, (signed char)STATE_BYTE);
            else
                DrawOppMeter(Char.room, 0, (signed char)STATE_BYTE);
        }
    }
}
