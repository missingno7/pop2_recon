#pragma pack(1)
typedef struct {
    signed char cell;
    int x;
    unsigned char fields_before_action[12];
    unsigned char action;
} OpponentPosition;
#pragma pack()

typedef struct {
    int x, y;
} MobPosition;

extern MobPosition current_mob;
extern unsigned char stage_mode;

int opp_under_mob(OpponentPosition *opp)
{
    int result = 0;
    int height = (signed char)(opp->cell / 10) * 63 + 56;

    if (stage_mode != 5 || opp->action != 8)
        height -= 30;
    if (height < current_mob.y) {
        if (current_mob.x / 32 == (opp->x - 144) / 32)
            result = 1;
        else
            result = 0;
    }
    return result;
}
