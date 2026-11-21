struct slide_rock {
    char unused[4];
    unsigned char room;
    char middle[5];
    signed char row;
};

extern struct slide_rock *find_moving_record(unsigned char kind, int index);

struct slide_rock *far _fastcall slide_rock_on_row(unsigned char room, signed char row)
{
    struct slide_rock *rock;
    int finished = 0;
    int index = 1;

    do {
        rock = find_moving_record(6, index++);
        if (!rock || (rock->room == room && rock->row == row))
            finished = 1;
    } while (!finished);
    return rock;
}
