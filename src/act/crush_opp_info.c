#pragma pack(1)
struct crush_record {
    signed char bytes_00[5];
    int sequence, position;
    signed char byte_09;
    signed char index;
    signed char byte_0b;
    signed char enabled;
    signed char bytes_0d[2];
    signed char kind;
    signed char byte_10;
    int pending;
};
extern unsigned char crush_mode;
extern void far pascal RemoveOppFromRoom(unsigned char room, int slot);

/* 25da:2220 */
void far pascal CrushOppInfo(struct crush_record near *opponent, unsigned char room)
{
    if (crush_mode != 2)
        opponent->enabled = 0;
    if (crush_mode == 5) {
        if (opponent->kind == 8) {
            opponent->sequence = 169;
            opponent->position = 0;
        } else {
            RemoveOppFromRoom(room, opponent->index);
        }
    } else if (crush_mode == 2) {
        if (opponent->pending == 1)
            opponent->pending = 0;
    } else {
        opponent->sequence = 22;
        opponent->position = 0;
    }
}
