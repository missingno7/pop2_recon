extern unsigned char char_room, stage, char_action;
extern signed char char_col, char_row, resolved_col;
extern int col_positions[], floor_y;
extern unsigned int gate_opening;
extern unsigned char read_cell(unsigned char room, signed char col, signed char row);
extern unsigned char read_adjacent_cell(signed char direction);
extern unsigned char read_floor_cell(void);
extern int shift_x(int distance);
extern int scythe_is_active(unsigned int flags);

int gate_blocks_standing(void)
{
    int position = 0;
    unsigned int opening;

    if (char_room == 9 && stage == 8)
        goto done;
    if (read_cell(char_room, char_col, char_row) == 4 ||
        read_adjacent_cell(1) == 4 || read_floor_cell() == 4) {
        opening = gate_opening & 255;
        position = shift_x(-6) - 14;
        if (opening != 0 && opening < 200 && col_positions[resolved_col] + 12 <= position &&
            col_positions[resolved_col + 1] + 4 >= position)
            position = 1;
        else
            position = 0;
    }
done:
    return position;
}

int scythe_blocks_passage(void)
{
    int blocked;

    if (char_action != 1 && char_action != 7 && char_action != 8 && char_action != 11 &&
        floor_y > 25 && scythe_is_active(gate_opening)) {
        blocked = 1;
    } else {
        blocked = 0;
    }
    return blocked;
}
