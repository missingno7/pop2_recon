extern unsigned char prev_rooms[10], current_rooms[10], below_rooms[10], above_rooms[10];
extern signed char previous_row;
void *memset(void *, int, unsigned int);
#pragma intrinsic(memset)

extern int bump_pending;

void BumpSound(void)
{
    bump_pending = 1;
}

void InitCollisionDetection(void)
{
    memset(prev_rooms, 255, sizeof(prev_rooms));
    memset(current_rooms, 255, sizeof(current_rooms));
    memset(below_rooms, 255, sizeof(below_rooms));
    memset(above_rooms, 255, sizeof(above_rooms));
    previous_row = -1;
}

extern unsigned char char_action, cell_flags;
extern int floor_y;

int CreviceBarr(void)
{
    if (char_action == 7 || char_action == 8 || floor_y > 18 || (cell_flags & 3) == 3)
        return 1;
    return 0;
}
