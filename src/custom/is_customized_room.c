typedef struct {
    unsigned int flags;
    unsigned int data;
} CustomTile;

extern CustomTile custom_tiles[][30];

int IsCustomizedRoom(unsigned char room)
{
    CustomTile *tiles = custom_tiles[room];
    unsigned int flags = 0;
    signed char tile = 0;

    do {
        if (flags)
            break;
        flags = tiles[tile].flags & 0xC000;
        ++tile;
    } while (tile < 30);
    return flags != 0;
}
