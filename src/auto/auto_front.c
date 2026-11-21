/* AUTO.C front piece through 2344:0064. 0008 remains a draft. */

extern int opponent_update_state;
extern unsigned char STATE_BYTE;
extern volatile unsigned char actor_kind;
extern volatile int state_word;
extern unsigned char opponent_life;
extern unsigned char opponent_life_display;

void far pascal SetOppMaxLifePts(register unsigned char *p);

void far pascal reset_overlay_state(void)
{
    opponent_update_state = 0;
}

void far pascal set_opponent_life(unsigned char *record)
{
    SetOppMaxLifePts(record);
    if ((actor_kind == 4 && state_word == 119) ||
        (actor_kind == 10 && state_word == 105)) {
        opponent_life = 0;
        opponent_life_display = 0;
    } else {
        opponent_life = STATE_BYTE;
        opponent_life_display = opponent_life;
    }
}

void far pascal SetOppMaxLifePts(register unsigned char *p)
{
    if (p[16] == 0)
        STATE_BYTE = 3;
    else
        STATE_BYTE = p[16];
}
