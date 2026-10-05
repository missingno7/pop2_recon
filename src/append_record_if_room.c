#pragma pack(1)

struct item {
    int active;
    char state;
    char type;
    char far *data;
    int tail;
};

struct list {
    char prefix;
    struct item entries[10];
};

struct input {
    char prefix;
    char value;
    char far *data;
    int tail;
};

void far pascal APPEND_RECORD_IF_ROOM(struct list *destination,
                                      struct input *source)
{
    register int index;
    struct item *entry;

    index = 0;
    entry = destination->entries;
    do {
        if (entry->active == 0)
            break;
        ++entry;
        ++index;
    } while (index < 10);

    if (index < 10) {
        destination->entries[index].active = 1;
        destination->entries[index].state = 0;
        destination->entries[index].type = source->value;
        destination->entries[index].data = source->data;
        destination->entries[index].tail = source->tail;
    }
}
