/* AUTO.C, overlay-3:2344:0c6e through 0fae.
 * SHOULD_GENERATE and GET_GENERATOR retain unresolved binding obligations. */

#pragma pack(1)
typedef struct {
    unsigned char data[23];
} opponent_record;
typedef struct {
    signed char count;
    opponent_record opponents[5];
} room_opponents;
extern room_opponents opponent_rooms[];

#pragma pack(1)
typedef struct {
    unsigned char kind;
    unsigned char limit;
    signed char column;
    signed char row;
    unsigned char data[3];
    signed char alternate_column;
    unsigned char enabled;
    unsigned char flags;
} generator_point;
typedef struct {
    signed char count;
    unsigned char header[3];
    generator_point points[3];
} room_generators;

extern room_generators generators[];
extern volatile signed char kid_row;

int abs(int value);
#pragma intrinsic(abs)

generator_point *far pascal get_generator(unsigned char room, signed char index);
int far pascal is_active_generator(generator_point *point, signed char column);

generator_point *far pascal find_active_generator(unsigned char room, signed char column)
{
    signed char index = 0;
    generator_point *point = 0;
    signed char count = generators[room].count;

    for (; index < count; ++index) {
        point = get_generator(room, index);
        if (is_active_generator(point, column))
            break;
        point = 0;
    }
    return point;
}

int far pascal is_active_generator(generator_point *point, signed char column)
{
    int result;

    if ((point->column == column || point->alternate_column == column) &&
        abs(point->row - kid_row) > 2 && point->enabled != 0)
        result = 1;
    else
        result = 0;
    return result;
}

extern volatile signed char current_level;
extern int generator_level;
extern int game_tick;
extern volatile int kid_x;

#pragma pack(1)
typedef struct {
    unsigned char prefix[2];
    int x;
    unsigned char data[5];
    signed char row;
    unsigned char suffix[7];
    signed char active;
    unsigned char reserved[46];
} live_opponent;
extern live_opponent live_opponents[];
signed char far pascal find_room_row(unsigned char room, signed char row,
                                    signed char column, signed char direction);

int far pascal should_generate(generator_point *point, unsigned char room)
{
    signed char count = opponent_rooms[room].count;
    signed char index, room_row;
    unsigned char between;
    int allowed;

    if (count < 5 && current_level == generator_level && game_tick % 3 != 0) {
        allowed = 1;
        index = 0;
        between = 0;
        room_row = find_room_row(room, point->row, point->column,
                                 point->row >= 5 ? -1 : 1);
        if (count > 0) {
            do {
                if (!allowed)
                    break;
                if (live_opponents[index].active < 0 &&
                    ((point->row < kid_row && live_opponents[index].x < kid_x) ||
                     (point->row > kid_row && live_opponents[index].x > kid_x)) &&
                    ((point->row < room_row && kid_row < room_row) ||
                     (point->row > room_row && kid_row > room_row))) {
                    ++between;
                    if (point->limit <= between ||
                        (point->row < kid_row && live_opponents[index].row <= point->row) ||
                        (point->row > kid_row && live_opponents[index].row >= point->row))
                        allowed = 0;
                }
                ++index;
            } while (index < count);
        }
    } else
        allowed = 0;
    return allowed;
}

generator_point *far pascal get_generator(unsigned char room, signed char index)
{
    generator_point *point;

    if (room && generators[room].count > index)
        point = &generators[room].points[index];
    else
        point = 0;
    return point;
}

extern unsigned char current_room;
int far pascal generator_has_tumble(unsigned char room)
{
    int found = 0;
    signed char count = generators[current_room].count;
    signed char index = 0;
    generator_point *point;

    for (; index < count && !found; ++index) {
        point = get_generator(room, index);
        if (point && (point->flags & 128)) found = 1;
        else found = 0;
    }
    return found;
}

extern volatile unsigned char preferred_opponent;
extern volatile int bank_mask;
extern volatile unsigned char bank_opponents[2];
void far pascal load_opponent_bank(int resource, int start, unsigned char size,
                                   unsigned char kind);

void far _fastcall reserve_opponent_bank(unsigned char kind)
{
    int slot;

    if (preferred_opponent != 0 && preferred_opponent != 5 && preferred_opponent != 6) {
        bank_mask = preferred_opponent == 2 ? 8 : 4;
        return;
    }
    bank_mask = 0;
    if (kind == 0)
        kind = 1;

    for (slot = 0; slot < 2 && bank_mask == 0; ++slot) {
        if (bank_opponents[slot] == kind)
            bank_mask = 4 << slot;
    }
    if (bank_mask == 0) {
        for (slot = 0; slot < 2 && bank_mask == 0; ++slot) {
            if (bank_opponents[slot] == 0) {
                bank_opponents[slot] = kind;
                bank_mask = 4 << slot;
            }
        }
    }
    if (bank_mask == 0)
        bank_mask = 4;
}

void far pascal load_opponent_banks(void)
{
    int slot;
    int start;

    if (preferred_opponent != 0 && preferred_opponent != 5 && preferred_opponent != 6) {
        if (preferred_opponent == 2) {
            load_opponent_bank(750, 48, 16, 0);
            return;
        }
    } else {
        for (slot = 0, start = 32; start < 64; ++slot, start += 16) {
            if (bank_opponents[slot] != 0)
                load_opponent_bank(750, start, 16, bank_opponents[slot] - 1);
        }
    }
}
