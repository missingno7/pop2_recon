/* ALTAR.C, overlay 15. DOS entry order. */
#pragma pack(1)
typedef unsigned char Byte;
typedef unsigned int Handle;
typedef struct { int top, left, bottom, right; } Rect;
typedef struct {
    signed char type;
    int x, y;
    Byte pass, palette;
    int resource;
    Handle shape;
    Rect rect;
    Byte flags, peel, unknown[4];
} RoomPiece;
typedef struct {
    Byte count, type;
    int palette;
    char unknown[24];
    RoomPiece pieces[1];
} RoomData;
typedef struct {
    signed char position;
    int x;
    signed char direction;
    Byte flag, unknown[4], mode, id, slot, enabled, flag2;
} Spawn;
typedef struct { int size; char unknown[2]; unsigned int flags; char bytes[97]; } Shape;
typedef struct { char data[4]; Handle handle; } ShapeRef;
typedef struct { Byte type; char skip[3]; unsigned int modifier; } TileAnim;
typedef struct { Byte frame, unknown; signed char col, dy; } FlameStep;
#pragma pack()

#pragma pack(1)
struct char_bounds { int top, left, bottom, right; };
typedef struct {
    signed char slot;                  /* 00 */
    signed char direction;             /* 01 */
    int x, y;                          /* 02, 04 */
    signed char character_kind;        /* 06 */
    unsigned int frame;                /* 07 */
    signed char column, row;           /* 09, 0a */
    signed char action;                /* 0b */
    signed char horizontal_step;       /* 0c */
    signed char vertical_step;         /* 0d */
    signed char room;                  /* 0e */
    signed char field_0f;              /* 0f */
    signed char stance;                /* 10 */
    signed char alive;                 /* 11 */
    signed char life_points;           /* 12 */
    signed char life_initial;          /* 13 */
    signed char life_delta;            /* 14 */
    int sequence_position;             /* 15 */
    int sequence_id;                   /* 17 */
    unsigned int sequence;             /* 19 */
    struct char_bounds bounds;                    /* 1b: top, left, bottom, right */
    signed char alert_delay;           /* 23 */
    int sequence_group;                /* 24 */
    int previous_x, previous_y;        /* 26, 28 */
    unsigned int previous_frame;       /* 2a */
    struct char_bounds previous_bounds;           /* 2c: top, left, bottom, right */
    int field_34;                      /* 34 */
    int previous_mode;                 /* 36 */
    unsigned char behavior;            /* 38 */
    signed char opponent_index;        /* 39 */
    unsigned char field_3a;            /* 3a */
    unsigned char unknown_3b[5];        /* 3b..3f */
} CHARREC;
#pragma pack()

extern CHARREC Kid;
extern CHARREC Char;
extern CHARREC RoomOpp[5];
extern Byte scene_flag;                       /* DS:6290 */
extern int special_sequence;                        /* DS:0a16 */
extern RoomData *custom_room;
#pragma pack(1)
typedef struct {
    unsigned int modifier, unknown_02;
} LEVELCELL;

typedef struct {
    unsigned char left, right, above, below;
} ROOMLINKS;

typedef struct {
    signed char opponent_count;
    unsigned char opponent_data[5][23];
} ROOMOBJECTS;

typedef struct {
    signed char room, cell;
} REBIRTHPOINT;

typedef struct {
    signed char count;
    unsigned char spawn_flags;
    unsigned char header[2];
    unsigned char points[3][10];
} ROOMGENERATORS;

typedef struct {
    unsigned char cell_kinds[32][30];                  /* 0000 */
    LEVELCELL cell_info[32][30];                       /* 03c0 */
    unsigned char unknown_12c0[0x500];                 /* 12c0 */
    ROOMLINKS room_links[32];                          /* 17c0 */
    unsigned char room_count;                         /* 1840 */
    unsigned char unknown_1841[4];
    unsigned char environment_kind;                  /* 1845 */
    unsigned char unknown_1846;
    signed char level_number;                         /* 1847 */
    int background_overlays[8];                       /* 1848 */
    unsigned char unknown_1858[4];
    int actor_phase;                                  /* 185c */
    unsigned char unknown_185e[2];
    signed char entry_room, entry_cell, entry_facing;    /* 1860-1862 */
    unsigned char unknown_1863[3];
    unsigned char preferred_opponent;                /* 1866 */
    ROOMOBJECTS room_objects[32];                      /* 1867 */
    REBIRTHPOINT rebirth_points[2];                    /* 26e7 */
    unsigned char unknown_26eb[0x0e];
    ROOMGENERATORS generators[32];                     /* 26f9 */
    unsigned char unknown_2b39[0x3c0];                 /* 2b39-2ef8 */
} LEVELREC;
#pragma pack()
extern LEVELREC Level;
extern unsigned int BlueSpec;            /* DS:5e48 */
extern signed char tile_animation;            /* DS:67d0 */
Byte flame_image_map[6] = {0x24, 0x24, 0x25, 0x26, 0x26, 0x24};
extern Rect clip_rect, GamePlayRect;
extern Handle *kid_shapes;
extern Handle flame_steps_handle;             /* DS:6268 */
#pragma pack(1)
typedef struct {
    int image, equipment_image;                       /* 00,02 */
    signed char x, y;                                 /* 04-05 */
    unsigned char flags;                              /* 06 */
} FRAMEREC;
#pragma pack()
extern FRAMEREC Frame;
#pragma pack(1)
typedef struct {
    signed char index, flipped;                       /* 00-01 */
    int x, y, image;                                   /* 02,04,06 */
    unsigned char image_set;                          /* 08 */
    int clip_top, clip_left, clip_bottom, clip_right;    /* 09,0b,0d,0f */
    int depth;                                        /* 11 */
    unsigned char object_type;                        /* 13 */
} FCHARREC;
#pragma pack()
extern FCHARREC FChar;

extern void save_actor_slot(void), copy_kid_to_char(void), load_actor_slot(signed char), remove_char(void);
extern void clear_life_bar(void), DrawKidMeter(int, int), AddSong(int), AddSound(int);
extern void JumpSeq(int), step_forward(int), run_jump(int), FallOn(void);
extern int IsCharSwingingFwd(void);
extern void climb_up(void);
extern int AddCharX(int), FirstInRow(signed char), AddFCharX(int);
extern void decrement_life_points(Byte), SetKidDeathMethod(Byte);
extern int query_sound(int);
extern Spawn *GetOppInfoIdx(unsigned int, int);
extern void reset_char_y(void), ClearCharVel(void), AnimChar(void);
extern void set_palette(int, int, Byte, Byte);
extern void far _fastcall draw_custom_room_image(Byte, Byte);
typedef void far pascal DrawCallback(void);
extern DrawCallback mark_move;
extern void mark_bufs_for_rect(DrawCallback *, void near *, signed char);
extern Handle allocate_payload_handle(long);
extern void far *RESOLVE_HANDLE_TO_FAR_PAYLOAD(Handle);
extern void dispose_handle(Handle);
extern void PrinceNewShpItem(Handle, int, ShapeRef far *, int);
extern void PrinceFreeShp(int);
extern void MAKESHAPERECT(Rect far *, int, int, void far *);
extern void SaveFChar(void), RestoreFChar(void);

void altar_kid_rises_from_death(void);
void create_shadowman(void);

void altar_kid_rises_from_death(void)
{
    signed char id = Char.slot;
    save_actor_slot();
    copy_kid_to_char();
    JumpSeq(231);
    Char.alive = -1;
    Char.life_points = Char.life_initial;
    clear_life_bar();
    DrawKidMeter(Char.life_points, Char.life_initial);
    Char.sequence_group = 13;
    Char.stance = 0;
    Kid = Char;
    load_actor_slot(id);
    remove_char();
    AddSong(60);
}

extern void get_char_edges(void);
extern DrawCallback mark_floor;

void altar_shadow_progress(void)
{
    int sequence = -1;
    int distance;
    if (Char.frame == 15) {
        if (Char.direction == -1) {
            if (Char.column >= 6) {
                if (Char.sequence_group == 13)
                    sequence = 71;
                else if (Char.column >= 8)
                    sequence = 1;
                else {
                    step_forward(0);
                    if (Char.sequence == 44)
                        sequence = 3;
                }
            } else if (Char.sequence_group == 13 || Char.sequence == 232) {
                sequence = 5;
                AddSong(59);
            } else {
                sequence = 232;
            }
        } else if (Char.sequence_group != 13) {
            sequence = 5;
        } else if (Char.column < 6) {
            step_forward(0);
            if (Char.sequence == 44)
                sequence = 3;
        } else {
            distance = Kid.x - Char.x;
            if (Kid.direction == Char.direction)
                distance += 6;
            if (distance)
                step_forward(distance);
            else
                sequence = Kid.direction == Char.direction ? 71 : 5;
        }
    } else if (((Char.frame >= 1 && Char.frame <= 14) ||
                (Char.frame >= 49 && Char.frame <= 56)) &&
               Char.column >= 6) {
        run_jump(4);
    } else if (Char.frame >= 103 && Char.frame <= 106) {
        scene_flag = 255;
        FallOn();
    } else if (Char.frame == 91 && !IsCharSwingingFwd()) {
        climb_up();
        if (Char.sequence_group == 13)
            Char.y -= 2;
        if (Char.direction == 0) {
            get_char_edges();
            mark_bufs_for_rect(mark_floor, &Char.bounds, Char.slot);
        }
    } else if (Char.frame == 154) {
        Level.cell_info[3][11].modifier = 133;
    } else if (Char.frame == 153 && Char.sequence_group == 13) {
        set_palette(custom_room->palette, 48, 16, 0);
    } else if (Char.sequence_group == 13 && Char.frame == 185) {
        altar_kid_rises_from_death();
    } else if (Char.frame == 44) {
        sequence = 13;
        Char.x = AddCharX(-8);
    }
    if (sequence != -1)
        JumpSeq(sequence);
}

void altar_watch_kid(void)
{
    copy_kid_to_char();
    if (Char.row == 1 && Char.sequence != 230) {
        if (Char.x <= 218) {
            JumpSeq(230);
            Char.x = 210;
            decrement_life_points(100);
            AddSound(57);
            SetKidDeathMethod(16);
            Level.cell_info[3][11].modifier = 133;
        } else if (Char.x <= 236 && !query_sound(10061)) {
            AddSong(61);
        } else if (Char.frame == 185 && Char.row == 1 &&
                   Char.sequence_group != 12 && special_sequence == 1) {
            create_shadowman();
            special_sequence = -1;
        }
    }
    Kid = Char;
}

void create_shadowman(void)
{
    Spawn *entry;
    signed char id, zero, clear;
    int room;
    if (Char.room == 4 && ((Byte)Level.level_number) == 13) {
        Kid = Char;
        room = (Byte)Char.room;
        id = Level.room_objects[room - 1].opponent_count++;
        entry = GetOppInfoIdx(room, id);
        entry->position = FirstInRow(Char.row) + Char.column;
        entry->x = Char.x;
        entry->direction = Char.direction;
        clear = 0;
        entry->flag = clear;
        entry->mode = clear;
        entry->id = id;
        zero = 0;
        entry->slot = zero;
        entry->flag2 = zero;
        entry->enabled = 1;
        Char.slot = id;
        Char.x = entry->x;
        Char.life_points = Char.character_kind = 1;
        Char.alive = -1;
        Char.field_34 = 8;
        reset_char_y();
        --Char.y;
        ClearCharVel();
        Char.sequence_group = 0;
        Char.alert_delay = Char.stance = 0;
        JumpSeq(231);
        AnimChar();
        AddSong(63);
        save_actor_slot();
        copy_kid_to_char();
    }
}

int altar_shadow_active(void)
{
    int result = 0;
    signed char i, count;
    i = 0;
    count = Level.room_objects[3].opponent_count;
    if (count > 0) {
        do {
            if (result)
                break;
            if (RoomOpp[i].character_kind == 1)
                result = 1;
        } while (++i < count);
    }
    return result;
}

void animate_sacred_flame(void)
{
    unsigned int phase;
    Byte image;
    if (custom_room && custom_room->type == 32) {
        phase = BlueSpec;
        if (phase & 0x80) {
            if (!(phase & 15)) {
                phase = 0;
                image = 11;
            } else {
                --phase;
                image = flame_image_map[phase & 15];
            }
        } else {
            if (phase < 11)
                ++phase;
            else
                phase = 0;
            image = phase + 11;
        }
        mark_bufs_for_rect(mark_move, &custom_room->pieces[image].rect, -1);
        BlueSpec = phase;
    } else {
        tile_animation = -1;
    }
}

void draw_temp_sacred_flame(TileAnim *tile)
{
    struct { Rect saved; Byte align, image; unsigned int modifier; } draw;
    RoomPiece *piece;
    if (tile->type == 11) {
        draw.saved = clip_rect;
        clip_rect = GamePlayRect;
        draw.modifier = tile->modifier;
        if (draw.modifier & 0x80) {
            draw.image = flame_image_map[draw.modifier & 0xff7f];
            piece = &custom_room->pieces[draw.image];
            piece->pass = 1;
            draw_custom_room_image(draw.image, 1);
            piece->pass = 11;
        } else {
            draw_custom_room_image((Byte)draw.modifier + 11, 11);
        }
        clip_rect = draw.saved;
    }
}

void load_altar_room(void)
{
    int i, index;
    Handle handle;
    RoomPiece *piece;
    Shape far *source, far *copy;
    ShapeRef ref;

    handle = allocate_payload_handle(sizeof(Shape));
    source = RESOLVE_HANDLE_TO_FAR_PAYLOAD(*kid_shapes);
    copy = RESOLVE_HANDLE_TO_FAR_PAYLOAD(handle);
    *copy = *source;
    copy->flags = 0x4000;
    copy->size = custom_room->palette;
    for (i = 11; i <= 38; i++) {
        index = i;
        piece = &custom_room->pieces[index];
        dispose_handle(piece->shape);
        ref.handle = 0;
        PrinceNewShpItem(handle, piece->type + 1, &ref, 1);
        piece->shape = ref.handle;
        PrinceFreeShp(piece->type + custom_room->palette);
        i = index;
    }
    dispose_handle(handle);
    set_palette(custom_room->palette, 224, 16, 0);
    set_palette(2000, 48, 16, 0);
}

void setup_flame(void)
{
    Rect saved;
    FlameStep far *steps;
    Byte image;
    RoomPiece *piece;
    if (Char.room == 4 && ((Byte)Level.level_number) == 13) {
        SaveFChar();
        saved = clip_rect;
        steps = RESOLVE_HANDLE_TO_FAR_PAYLOAD(flame_steps_handle);
        image = steps[Frame.equipment_image].frame + 23;
        piece = &custom_room->pieces[image];
        piece->y = AddFCharX(steps[Frame.equipment_image].col);
        piece->x = steps[Frame.equipment_image].dy + FChar.y;
        MAKESHAPERECT(&piece->rect, piece->x, piece->y, RESOLVE_HANDLE_TO_FAR_PAYLOAD(piece->shape));
        clip_rect = piece->rect;
        piece->pass = 0;
        draw_custom_room_image(image, 0);
        piece->pass = 11;
        clip_rect = saved;
        RestoreFChar();
    }
}

void unload_altar_room(void)
{
    set_palette(3000, 224, 16, 0);
}
