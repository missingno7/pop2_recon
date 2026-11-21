typedef struct {
    unsigned state;
    unsigned extra;
} RoomObject;

extern RoomObject *room_objects;
extern RoomObject room_object;
extern unsigned char *room_kinds;
extern unsigned char tile_kind;

void select_trob_cell(int cell)
{
    room_object = room_objects[cell];
    tile_kind = room_kinds[cell];
}
