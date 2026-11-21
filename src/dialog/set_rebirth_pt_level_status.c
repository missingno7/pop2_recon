#pragma pack(1)
struct rebirth_record {
    int point_id;
    int reserved2;
    int menu, disabled;
    signed char level;
    signed char health;
    signed char direction;
    char saved_flags;
    signed char facing;
    char reserved13[2];
    int level_flags;
    char room_state;
    char reserved18[5];
    char rooms[960];
    char room_links[3840];
    char room_objects[3712];
    char level_state[1088];
};


extern struct rebirth_record far *rebirth_data;
extern int level_flags;
extern char rooms[960], room_links[3840], room_objects[3712], level_state[1088];
extern void far * far cdecl _fmemcpy(void far *, const void far *, unsigned);

void far pascal SetRebirthPtLevelStatus(void)
{
    if (rebirth_data) {
        _fmemcpy(rooms, rebirth_data->rooms, sizeof(rooms));
        _fmemcpy(room_links, rebirth_data->room_links, sizeof(room_links));
        _fmemcpy(room_objects, rebirth_data->room_objects, sizeof(room_objects));
        _fmemcpy(level_state, rebirth_data->level_state, sizeof(level_state));
        level_flags = rebirth_data->level_flags;
    }
}

#pragma pack()
