typedef unsigned char byte;
typedef signed char sbyte;

typedef struct {
    sbyte cell;
    byte room;
    sbyte state;
    byte kind;
} Trob;

extern Trob current_trob;
extern byte viewed_room, left_room, right_room, upper_room, lower_room;
extern byte upper_right_room, lower_right_room;
int is_visible_room_cell(byte room, sbyte cell);

sbyte check_trob(void)
{
    sbyte cell = current_trob.cell;

    if (lower_room == current_trob.room) {
        if (cell >= 20 && cell < 30)
            cell = 19 - cell;
        else
            cell = 30;
    } else if (viewed_room != current_trob.room)
        cell = 30;
    return cell;
}

int check_trob_visibility(void)
{
    int visible = is_visible_room_cell(current_trob.room, current_trob.cell);
    if (!visible)
        current_trob.state = -1;
    return visible;
}

sbyte check_trob_left(void)
{
    sbyte cell = current_trob.cell;

    if (viewed_room == current_trob.room) {
        if (cell % 10 == 0)
            cell = 30;
        else
            --cell;
    } else if (left_room == current_trob.room && cell % 10 == 0)
        cell += 9;
    else
        cell = 30;
    return cell;
}

sbyte check_trob_right(void)
{
    sbyte cell = current_trob.cell;

    if (viewed_room == current_trob.room) {
        if (cell % 10 == 9)
            cell = 30;
        else
            ++cell;
    } else if (right_room == current_trob.room) {
        if (cell % 10 == 9)
            cell -= 9;
        else
            cell = 30;
    } else if (lower_room == current_trob.room) {
        if (cell >= 20 && cell < 29)
            cell = 18 - cell;
        else
            cell = 30;
    } else
        cell = lower_right_room == current_trob.room && cell == 29 ? -1 : 30;
    return cell;
}

sbyte check_trob_above(void)
{
    sbyte cell = current_trob.cell;

    if (viewed_room == current_trob.room) {
        if (cell < 10)
            cell = -1 - cell;
        else
            cell -= 10;
    } else if (upper_room == current_trob.room && cell < 10)
        cell += 20;
    else
        cell = 30;
    return cell;
}

sbyte check_trob_above_right(void)
{
    sbyte cell = current_trob.cell;

    if (viewed_room == current_trob.room) {
        if (cell % 10 == 9)
            cell = 30;
        else if (cell < 10)
            cell = -2 - cell;
        else
            cell -= 9;
    } else if (right_room == current_trob.room) {
        if (cell == 9)
            cell = -1;
        else if (cell % 10 == 9)
            cell -= 19;
        else
            cell = 30;
    } else if (upper_room == current_trob.room) {
        if (cell < 9)
            cell += 21;
        else
            cell = 30;
    } else
        cell = upper_right_room == current_trob.room && cell == 9 ? 20 : 30;
    return cell;
}

sbyte check_trob_below(void)
{
    sbyte cell = current_trob.cell;

    if (viewed_room == current_trob.room) {
        if (cell < 20)
            cell += 10;
        else
            cell = 30;
    } else if (lower_room == current_trob.room && cell >= 20)
        cell %= 10;
    else
        cell = 30;
    return cell;
}

int is_visible_room_cell(byte room, sbyte cell)
{
    int visible;

    if (room == viewed_room || (room == right_room && cell % 10 == 9)
            || (room == upper_room && cell < 10)
            || (room == upper_right_room && cell == 9))
        visible = 1;
    else
        visible = 0;
    return visible;
}
