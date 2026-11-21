/* FINAL.C, MSC 6.00 medium model. Claims and drafts are listed in notes.md. */

#pragma pack(1)

typedef signed char sbyte;
typedef unsigned char byte;

typedef struct {
    byte kind;
    byte attributes[3];
    unsigned int animation;
} FinalObject;

extern byte viewed_room;
extern byte environment_kind;
extern int final_counter;
extern sbyte opponent_ticks;
extern sbyte opponent_room;
extern sbyte opponent_column;
extern sbyte opponent_kind;
extern volatile byte opponent_alive;
extern int opponent_sequence;
extern sbyte nearby_opponent_room;
extern sbyte kid_room;
extern int kid_sequence;
extern int defeated_opponents;
extern byte tile_modifier[];
extern unsigned int tile_animation[][2];

extern sbyte far pascal tile_at(sbyte kind, sbyte room, sbyte column);
extern unsigned int far pascal bounded_random(unsigned int max);
extern sbyte far pascal FirstInRow(sbyte row);
extern void far pascal add_timer(sbyte kind);
extern void far pascal mark_cell(sbyte kind, sbyte cell);
extern void far pascal refresh_cell(int cell);
extern void far pascal set_sequence(int sequence);
extern void far pascal play_sound(int sound);
extern int far pascal sequence_finished(int sequence);

void far pascal draw_crystal(FinalObject near *object);
void far pascal draw_gargoyle(FinalObject near *object);
void far pascal generate_opponent(sbyte variant, sbyte room);

void far pascal draw_final_object(register FinalObject near *object)
{
    if (viewed_room == 6 && environment_kind == 6 &&
        (object->animation & 0x1000))
        draw_crystal(object);

    if ((viewed_room == 7 || viewed_room == 8) &&
        environment_kind == 6 && (object->animation & 0x1000))
        draw_gargoyle(object);
}

void far pascal load_final_code(void)
{
    final_counter = 0;
}

void far pascal check_dead_opponent(void)
{
    if (opponent_ticks == 6) {
        sbyte cell;
        unsigned int frame;

        cell = tile_at(opponent_kind, opponent_room, opponent_column);
        frame = bounded_random(2);
        if (frame != 0 && cell == 1 && defeated_opponents < 4) {
            cell = FirstInRow(opponent_column) + opponent_room;
            if (cell < 22 || cell > 24) {
                tile_modifier[cell] = 10;
                tile_animation[cell][0] = (frame | 0x200) << 5;
                add_timer(3);
                mark_cell(3, cell);
                refresh_cell(cell);
            }
        }
        ++opponent_ticks;
    } else if (opponent_ticks >= 96 && opponent_sequence != 239) {
        set_sequence(239);
        play_sound(197);
        ++opponent_ticks;
    }
    if (opponent_alive == 0 && opponent_ticks > 6)
        ++opponent_ticks;
}

void far pascal check_new_opponent(sbyte count)
{
    sbyte variant;
    sbyte room = -1;

    if (count <= 1) {
        if (count == 1) {
            if (kid_sequence != 238) {
                room = (nearby_opponent_room == 1) ? 2 : 1;
                if (room == kid_room)
                    room = -1;
            }
        } else {
            room = (kid_room == 2) ? 1 : 2;
        }
    }
    if (room >= 0 && (count == 0 || bounded_random(20) == 0)) {
        variant = (room == 1) ? 4 : 2;
        if (bounded_random(1))
            variant += 4;
        generate_opponent(variant, room);
    }
}

typedef struct {
    byte cell;
    int x;
    sbyte direction;
    byte style;
    byte unused[4];
    byte bank;
    byte index;
    byte status;
    byte strength;
    byte delay;
    byte unused2[2];
    byte initial_strength;
    sbyte stance[4];
    sbyte row;
} OpponentSlot;

extern sbyte opponent_serial;
extern const int tile_x[];
extern byte opponent_bank;
extern sbyte level_kind;
extern const byte opponent_style_for_level[];
extern sbyte char_slot;
extern sbyte char_direction;
extern int char_x;
extern byte char_style;
extern volatile byte char_local_state;
extern byte char_strength;
extern byte char_initial_strength;
extern byte char_max_strength;
extern int char_action;
extern volatile int char_floor;
extern volatile byte char_weapon;
extern byte char_delay;
extern byte stored_opponent_slot;

extern OpponentSlot near *far pascal reserve_opponent(int bank, int serial);
extern void far _fastcall select_opponent_bank(byte kind);
extern void far pascal load_opponent_shapes(void);
extern void far pascal position_opponent(void);
extern void far pascal init_opponent_sequence(void);
extern void far pascal update_opponent(void);
extern void far pascal save_opponent(void);

void far pascal generate_opponent(sbyte variant, sbyte room)
{
    sbyte index;
    OpponentSlot near *opponent;

    opponent = reserve_opponent(3, index = opponent_serial++);
    opponent->cell = FirstInRow(room) + variant;
    opponent->x = tile_x[variant];
    if (variant < 5)
        opponent->direction = 0;
    else
        *(byte near *)&opponent->direction = 0xff;
    opponent->style = 7;
    if (opponent_bank == 0) {
        select_opponent_bank(1);
        load_opponent_shapes();
    }
    opponent->bank = opponent_bank;
    opponent->index = index;
    opponent->delay = opponent->status = 0;
    opponent->row = room;
    opponent->initial_strength = opponent->strength = bounded_random(2) + 3;
    char_slot = index;
    char_direction = opponent->direction;
    char_x = opponent->x;
    char_style = opponent_style_for_level[level_kind];
    opponent_room = variant;
    opponent_column = room;
    char_local_state = 0;
    opponent_kind = 3;
    char_max_strength = char_strength = char_initial_strength = opponent->strength;
    opponent_ticks = -1;
    char_action = 4;
    position_opponent();
    init_opponent_sequence();
    char_floor = 0;
    opponent_alive = 1;
    char_weapon = 0;
    set_sequence(238);
    char_delay = 0;
    update_opponent();
    play_sound(197);
    if (stored_opponent_slot == (byte)-1)
        stored_opponent_slot = char_slot;
    save_opponent();
}

void far pascal final_noop(void)
{
}

extern int char_frame;
extern int clip_result;

void far pascal clip_final_character(void)
{
    if (opponent_kind == 3 && environment_kind == 6 &&
        opponent_column == 1 && sequence_finished(char_frame))
        clip_result = 119;
}

typedef struct {
    byte data[64];
} CharacterState;

typedef struct {
    byte opponent_count;
    byte data[115];
} FinalRoom;

extern CharacterState active_character;
extern CharacterState other_character;
extern int entry_x;
extern sbyte entry_room_copy;
extern byte scroll_pending;
extern FinalRoom room_state[];
extern byte crystal_available;
extern int sound_period;
extern byte saved_kid_room;
extern void far pascal reset_char_position(void);
extern int far pascal music_busy(int tune);
extern void far pascal request_music(int tune);
extern void far pascal prepare_scene(int mode);
extern void far pascal reset_scene(void);
extern void far pascal switch_room(sbyte room);
extern void far pascal fade_scene(int ticks, int speed, byte color, byte direction);
extern void far pascal redraw_room(sbyte room);

void far pascal watch_kid(void);
void far pascal watch_opponents(void);

void far pascal cut_kid_to_chess_room(void)
{
    char_action = 2;
    char_direction = 0;
    set_sequence(55);
    char_weapon = 1;
    play_sound(19);
    entry_room_copy = opponent_kind = 4;
    opponent_room = 5;
    char_x = entry_x + 14;
    opponent_column = 1;
    position_opponent();
    char_max_strength = char_strength;
    update_opponent();
    other_character = active_character;
    reset_char_position();
    scroll_pending = 0;
}

void far pascal run_final_frame(void)
{
    watch_kid();
    watch_opponents();

    if (!music_busy(sound_period + 10000)) {
        int tune = -1;

        switch (viewed_room) {
        case 3:
            tune = 269;
            break;
        case 6:
            if (crystal_available)
                tune = 270;
            break;
        case 7:
        case 8:
            if (room_state[viewed_room].opponent_count)
                tune = 263;
            break;
        }
        if (tune != -1)
            request_music(tune);
    }
}

void far pascal enter_princess_room(void)
{
    prepare_scene(5);
    reset_scene();
    switch_room(saved_kid_room);
    fade_scene(1000, 240, 16, 0);
    redraw_room(6);
    cut_kid_to_chess_room();
}

typedef struct {
    byte data[2];
    int shape;
    byte tail[16];
} ForegroundImage;

typedef struct {
    sbyte shape_delta;
    byte attributes[24];
} ShapeEntry;

typedef struct {
    byte room, tile;
    int first_shape;
    byte data[24];
    ShapeEntry entries[1];
} RoomShapeTable;

extern RoomShapeTable near *shapes_by_room;
extern ForegroundImage far foreground_image[];
extern _segment foreground_segment;
extern int foreground_count;
extern void far cdecl move_bytes(void far *dst, const void far *src, unsigned int size);

void far pascal sort_foreground(void)
{
    ForegroundImage _based(foreground_segment) *destination;
    int out, index;

    out = index = foreground_count - 1;
    if (index >= 0) {
        int _based(foreground_segment) *shape_ptr;
        ForegroundImage saved;

        destination = (ForegroundImage _based(foreground_segment) *)&foreground_image[out];
        shape_ptr = (int _based(foreground_segment) *)&foreground_image[index].shape;
        do {
            int shape;

            shape = shapes_by_room->entries[*shape_ptr - shapes_by_room->first_shape].shape_delta
                  + shapes_by_room->first_shape;
            if ((shape >= 0x1021 && shape <= 0x1024) ||
                (shape >= 0x1058 && shape <= 0x105c)) {
                if (index != out) {
                    ForegroundImage far *source;
                    char _based(void) *at;
                    _segment storage_segment = (_segment)foreground_image;

                    at = (char _based(void) *)shape_ptr;
                    source = storage_segment :> (ForegroundImage _based(void) *)(at - 2);
                    saved = *source;
                    move_bytes(source, storage_segment :> (void _based(void) *)(at + 18),
                               (out - index) * sizeof(ForegroundImage));
                    *destination = saved;
                }
                --destination;
                --out;
            }
            shape_ptr -= sizeof(ForegroundImage) / sizeof(*shape_ptr);
        } while (--index >= 0);
    }
}

extern int kid_x;
extern byte kid_action;
extern int opponent_x;
extern sbyte opponent_position;
extern int opponent_frame;
extern byte kid_style;
extern byte screen_tile;
extern void far pascal activate_kid(void);
extern void far pascal activate_opponent(sbyte slot);
extern sbyte far pascal selected_opponent(void);
extern sbyte far pascal room_opponent_kind(void);
extern void far pascal begin_final_attack(sbyte frames);
extern void far pascal before_princess_room(void);
extern void far pascal finish_princess_room(void);

typedef struct {
    byte data[6];
    byte flags;
} AnimationFrame;

extern _segment animation_frame_segment;
extern AnimationFrame _based(animation_frame_segment) animation_frames[];

void far pascal watch_kid(void)
{
    activate_kid();
    if (opponent_kind == 2) {
        if (kid_x <= 305 && kid_action != 4)
            enter_princess_room();
    } else if (char_style == 1 && opponent_sequence != 71) {
        sbyte room;

        activate_opponent(selected_opponent());
        room = room_opponent_kind();
        if ((room == 7 || room == 8) && environment_kind == 6 &&
            kid_style == 5 && environment_kind == 6 && opponent_position <= 4 &&
            (animation_frames[opponent_frame].flags & 0x40)) {
            activate_kid();
            begin_final_attack(100);
            set_sequence(71);
            other_character = active_character;
            before_princess_room();
            finish_princess_room();
        }
        activate_kid();
    }
    other_character = active_character;
}

void far pascal watch_opponents(void)
{
    sbyte i;
    sbyte count = room_state[viewed_room].opponent_count;

    if (count) {
        i = 0;
        if (count > 0)
            do {
                activate_opponent(i);
                save_opponent();
            } while (count > ++i);
    }
    if (viewed_room == 3 && environment_kind == 6)
        check_new_opponent(count);
}

void far pascal run_jaffar_in_crystal(void);
void far pascal run_jaffar_in_escher(void);

void far pascal run_jaffar(void)
{
    if (opponent_kind == 6 && environment_kind == 6) {
        run_jaffar_in_crystal();
        return;
    }
    run_jaffar_in_escher();
}

extern sbyte shadow_slot;
extern byte shadow_room;

void far pascal dismiss_other_jaffars(void)
{
    CharacterState saved = active_character;
    sbyte i = 0;
    sbyte count = room_state[shadow_room].opponent_count;

    if (count > 0)
        do {
            activate_opponent(i);
            if (char_style == 6 && shadow_slot != char_slot && char_frame == 15) {
                set_sequence(240);
                save_opponent();
            }
        } while (count > ++i);
    active_character = saved;
}

extern int shadow_x;

void far pascal cast_jaffar_spell(void)
{
    int sequence = -1;

    if ((unsigned int)char_frame == 15) {
        if ((shadow_x < char_x && char_direction != -1) ||
            (shadow_x >= char_x && char_direction != 0))
            sequence = 5;
        else
            sequence = 242;
    } else if ((((unsigned int)char_frame >= 1 && (unsigned int)char_frame <= 14) ||
                ((unsigned int)char_frame >= 49 && (unsigned int)char_frame <= 56)) &&
               opponent_sequence != 13 && opponent_sequence != 5) {
        sequence = 13;
    }
    if (sequence != -1)
        set_sequence(sequence);
}

extern sbyte shadow_column;
extern sbyte shadow_row;
extern sbyte shadow_life;
extern sbyte shadow_offset;

void far pascal JaffarSetToNextPos(char near *p);
void far pascal JaffarSetToPrevPos(char near *record);
void far pascal send_jaffar_to_position(sbyte near *stance);
int far pascal shadow_in_range(sbyte position, sbyte direction);
void far pascal finish_casting_at_kid(void);
int far pascal jaffar_has_clear_shot(void);

void far pascal jaffar_attack_in_escher(void)
{
    int move = 1;
    sbyte near *stance;
    OpponentSlot near *raw;

    raw = reserve_opponent((byte)opponent_kind, char_slot);
    stance = raw->stance;
    if (raw->stance[0] != 2)
        stance[0] = 2;
    if (opponent_sequence == 242) {
        if (char_frame == 281)
            play_sound(62);
        else if (char_frame == 282 && jaffar_has_clear_shot())
            finish_casting_at_kid();
        move = 0;
    } else if (stance[1] == stance[2]) {
        if (shadow_column == opponent_column && jaffar_has_clear_shot())
            cast_jaffar_spell();
        if (shadow_in_range(stance[1], -1))
            JaffarSetToNextPos((char near *)stance);
        else
            JaffarSetToPrevPos((char near *)stance);
    }
    if (move && stance[1] != stance[2] && shadow_life > 0) {
        if (opponent_column == 1 && jaffar_has_clear_shot()) {
            cast_jaffar_spell();
            return;
        }
        send_jaffar_to_position(stance);
    }
}

void far pascal jaffar_flee_in_escher(void)
{
    sbyte near *stance;
    OpponentSlot near *raw;

    raw = reserve_opponent((byte)opponent_kind, char_slot);
    stance = raw->stance;
    if (raw->stance[0] != 1)
        stance[0] = 1;
    if (stance[1] == stance[2]) {
        if (shadow_in_range(stance[1], -1))
            JaffarSetToPrevPos((char near *)stance);
        else if (shadow_in_range(stance[1], 1))
            JaffarSetToNextPos((char near *)stance);
    }
    if (stance[1] != stance[2])
        send_jaffar_to_position(stance);
}

int far pascal jaffar_has_clear_shot(void)
{
    int clear = 0;

    if (shadow_life > 0 && (shadow_room == 7 || shadow_room == 8) &&
        environment_kind == 6 && opponent_column == shadow_column &&
        (animation_frames[char_frame].flags & 0x40) && char_frame != 109) {
        sbyte position = opponent_room;

        if (opponent_kind == 8)
            position += 10;
        if (opponent_column == 0) {
            if (position > 6 && position < 13)
                return clear;
            clear = 1;
        } else if (opponent_column == 1) {
            sbyte shadow = shadow_offset;

            if (shadow_room == 8)
                shadow += 10;
            if (position <= 12) {
                if (shadow <= 12)
                    clear = 1;
            } else if (shadow >= 14) {
                clear = 1;
            }
        } else if (opponent_column == 2) {
            clear = 1;
        }
    }
    return clear;
}

void far pascal JaffarSetToNextPos(char near *p)
{
    if (p[1] == 3)
        p[1] = 0;
    else
        ++p[1];
}

void far pascal JaffarSetToPrevPos(char near *record)
{
    if (record[1] != 0) {
        --record[1];
        return;
    }

    record[1] = 3;
}

typedef struct {
    sbyte room_x;
    byte flags;
    sbyte direction;
    int min_x, max_x;
    sbyte shadow_column[2];
    sbyte shadow_min[2];
    sbyte shadow_max[2];
} PositionRule;

extern const PositionRule position_rules[];
extern const sbyte direction_step[];
extern int jaffar_motion_disabled;
extern void far pascal shift_character(int delta);
extern int far pascal jaffar_go_to_position_0(sbyte near *stance);
extern int far pascal jaffar_go_to_position_1(sbyte near *stance);
extern int far pascal jaffar_go_to_position_2(sbyte near *stance);
extern int far pascal jaffar_go_to_position_3(sbyte near *stance);

void far pascal send_jaffar_to_position(sbyte near *stance)
{
    if (!jaffar_motion_disabled) {
        int move;
        switch (stance[1]) {
        case 0:
            move = jaffar_go_to_position_0(stance);
            break;
        case 1:
            move = jaffar_go_to_position_1(stance);
            break;
        case 2:
            move = jaffar_go_to_position_2(stance);
            break;
        case 3:
            move = jaffar_go_to_position_3(stance);
            break;
        }
        if (move != -1)
            set_sequence(move);
    }
}

int far pascal settle_jaffar_at_position(sbyte near *stance)
{
    int sequence = -1;

    if (char_frame == 15) {
        if (stance[0] == 2 && jaffar_has_clear_shot()) {
            cast_jaffar_spell();
        } else {
            const PositionRule near *rule = &position_rules[stance[1]];
            int x = char_x;

            if (rule->direction != char_direction)
                x += direction_step[rule->direction] * 6;
            if (rule->room_x < 10) {
                if (opponent_kind == 8)
                    x += 320;
            } else if (opponent_kind == 7) {
                x -= 320;
            }
            if (x < rule->min_x) {
                if (char_direction == 0)
                    shift_character(rule->min_x - x + 2);
                else
                    sequence = 5;
            } else if (x > rule->max_x) {
                if (char_direction == -1)
                    shift_character(x - rule->min_x + 2);
                else
                    sequence = 5;
            } else if (rule->direction != char_direction) {
                sequence = 5;
            } else {
                stance[2] = stance[1];
            }
        }
    }
    return sequence;
}

/* The four position-specific routines precede this block in the original. */
extern void far pascal load_shadow_slot(sbyte slot);
extern void far pascal run_player_script(sbyte script);

void far pascal finish_casting_at_kid(void)
{
    sbyte slot;

    set_sequence(244);
    slot = char_slot;
    save_opponent();
    activate_kid();
    set_sequence(71);
    play_sound(14);
    begin_final_attack(100);
    run_player_script(17);
    other_character = active_character;
    load_shadow_slot(slot);
}

/* The two room-specific Jaffar programs precede this routine. */
int far pascal shadow_in_range(sbyte position, sbyte direction)
{
    const PositionRule near *rule = &position_rules[position];
    int inside;

    direction = direction < 0 ? 0 : 1;
    if (rule->shadow_column[direction] == shadow_column) {
        sbyte x = shadow_offset + ((shadow_room == 8) ? 1 : 0) * 10;

        inside = x >= rule->shadow_min[direction] && x <= rule->shadow_max[direction];
    } else {
        inside = 0;
    }
    return inside;
}

/* The intervening motion and animation routines still need reconstruction. */
typedef struct {
    int xmin, ymin, xmax, ymax;
} Rect;

typedef struct {
    byte type;
    unsigned long shape_list;
    byte layer;
    byte attributes[5];
    Rect bounds;
    byte tail[6];
} FinalImage;

typedef struct {
    byte attributes[28];
    FinalImage images[1];
} RoomImageTable;

extern RoomImageTable near *room_image_table;
extern byte crystal_frame_slot[];
extern byte gargoyle_frame_slot[];
extern unsigned long crystal_shapes[];
extern unsigned long gargoyle_shapes[];
extern byte crystal_layer[];
extern byte gargoyle_layer[];
extern void far pascal image_bounds(Rect far *output, Rect far *input, unsigned long shapes);
extern void far _fastcall update_final_image(byte index, byte layer);

void far pascal draw_crystal(FinalObject near *object)
{
    int frame;
    byte index, shape;
    FinalImage near *image;
    Rect bounds;

    if (object->kind != 1 || !(object->animation & 0x800))
        return;
    frame = (short)(object->animation & 0x7f0) >> 4;
    if (--frame < 0)
        return;
    index = crystal_frame_slot[frame] + 4;
    image = &room_image_table->images[index];
    bounds = image->bounds;
    shape = object->animation & 15;
    image->shape_list = crystal_shapes[shape];
    image_bounds(&image->bounds, &image->bounds, image->shape_list);
    image->layer = crystal_layer[shape];
    update_final_image(index, crystal_layer[shape]);
    image->layer = 11;
    image->bounds = bounds;
    image->shape_list = 0;
}

void far pascal draw_gargoyle(FinalObject near *object)
{
    int frame;
    byte index, shape;
    FinalImage near *image;
    Rect bounds;

    if (object->kind != 1 || !(object->animation & 0x800))
        return;
    frame = (short)(object->animation & 0x7f0) >> 4;
    if (--frame < 0)
        return;
    index = gargoyle_frame_slot[frame] + 2;
    image = &room_image_table->images[index];
    bounds = image->bounds;
    shape = object->animation & 15;
    image->shape_list = gargoyle_shapes[shape];
    image_bounds(&image->bounds, &image->bounds, image->shape_list);
    image->layer = gargoyle_layer[shape];
    update_final_image(index, gargoyle_layer[shape]);
    image->layer = 11;
    image->bounds = bounds;
    image->shape_list = 0;
}
