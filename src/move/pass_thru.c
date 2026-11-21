#pragma pack(1)
typedef struct {
    int x, y;
    unsigned char room;
    int speed, auxiliary;
    unsigned char kind;
    signed char row;
    int flags;
} Mob;
#pragma pack()

typedef struct {
    unsigned char left, right, upper, lower;
} RoomLinks;

extern Mob current_mob;
extern RoomLinks room_links[];

void PassThru(void)
{
    ++current_mob.row;
    if (current_mob.row >= 3) {
        current_mob.y -= 192;
        current_mob.row = 0;
        current_mob.room = room_links[current_mob.room].lower;
    }
}
