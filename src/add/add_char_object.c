#pragma pack(1)

typedef unsigned char Byte;

typedef struct {
    int top;
    int left;
    int bottom;
    int right;
} Rect;

typedef struct {
    int x;
    int y;
    Byte set;
    int image;
    int object_id;
    Byte flags;
    Byte kind;
    Rect clip;
    Byte status;
    int depth;
    Byte object_type;
} CharacterObject;

extern CharacterObject character_objects[];
extern int character_object_count;
extern int actor_x;
extern int actor_y;
extern int actor_image;
extern Byte actor_set;
extern Byte actor_flags;
extern Rect actor_clip;
extern int actor_depth;
extern int actor_id;
extern Byte actor_type;

extern void ConvertX(int x, int *destination);
extern void update_char_object(Byte index);

void far _fastcall add_char_object(Byte kind)
{
    int index;
    CharacterObject *object;

    if (character_object_count < 30) {
        index = character_object_count++;
        object = &character_objects[index];
        object->kind = kind;
        ConvertX(actor_x, &object->x);
        object->y = actor_y;
        object->set = actor_set;
        object->image = actor_image;
        object->flags = actor_flags;
        object->clip = actor_clip;
        object->depth = actor_depth;
        update_char_object((Byte)index);
        object->object_id = actor_id;
        object->object_type = actor_type;
    }
}

