typedef unsigned char byte;
typedef signed char sbyte;

typedef struct {
    sbyte cell;
    byte room;
    sbyte state;
    byte kind;
} Trob;

extern Trob current_trob;
extern byte viewed_room, lower_room;

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

