typedef struct {
    unsigned state;
    unsigned extra;
} RoomObject;

extern RoomObject * volatile room_objects;
extern unsigned char *room_kinds;
extern unsigned char current_level;

void make_space(unsigned char room, signed char cell)
{
    int index = cell;
    unsigned state;

    room_kinds[index] = 0;
    if (current_level == 5 && room == 3)
        state = 0xc000;
    else
        state = (room_objects[*(volatile int *)&index].state & 0x80) + 3;
    room_objects[*(volatile int *)&index].state = state;
}
