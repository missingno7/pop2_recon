typedef struct {
    unsigned char room;
    signed char cell;
    unsigned char timer;
    unsigned char previous;
    unsigned char next;
} TileLink;

static TileLink links[256];

int get_link_next(int link)
{
    return links[link].next;
}

int GetLinkTimer(int link)
{
    return links[link].timer;
}

int ChangeLinkTimer(int link, unsigned char timer)
{
    return links[link].timer = timer;
}

int get_link_cell(int link)
{
    return links[link].cell;
}

int link_has_previous(int link)
{
    if (links[link].previous != 0xfd)
        return 1;
    return 0;
}

int link_has_next(int link)
{
    if (links[link].next != 0xfd)
        return 1;
    return 0;
}

unsigned char get_link_room(int link)
{
    return links[link].room;
}
