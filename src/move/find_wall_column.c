typedef signed char sbyte;
typedef unsigned char byte;

extern byte get_room_tile(byte room, sbyte column, sbyte row);
extern int is_wall_tile(byte tile);

sbyte find_wall_column(byte room, sbyte column, sbyte row, sbyte step)
{
    do {
        column += step;
        if (is_wall_tile(get_room_tile(room, column, row)))
            break;
    } while (column >= 0 && column <= 10);
    return column;
}
