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
extern int shadow_x;
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
extern byte tile_kind[][30];
extern unsigned int custom_tile_fields[][30][2];

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
                tile_kind[3][cell] = 10;
                custom_tile_fields[3][cell][0] = (frame | 0x200) << 5;
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
extern int start_room_column_x[20];
extern byte opponent_bank;
extern sbyte level_kind;
extern const byte opponent_style_for_level[];
typedef struct {
    sbyte slot;
    byte other_fields[63];
} CharacterState;

extern CharacterState active_character;
#define char_slot active_character.slot
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
    opponent->x = start_room_column_x[variant + 5];
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
    byte opponent_count;
    byte data[115];
} FinalRoom;

extern CharacterState other_character;
extern sbyte entry_room_copy;
extern byte scroll_pending;
extern FinalRoom room_state[];
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
    char_x = start_room_column_x[10] + 14;
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
            if (room_state[6].opponent_count)
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

static PositionRule position_rules[4] = {
    {17, 0, -1, 388, 400, {1, 0}, {15, 1}, {18, 18}},
    {14, 2, -1, 280, 304, {2, 1}, {0, 14}, {18, 18}},
    { 5, 2,  0, 312, 336, {1, 2}, {2, 0}, {9, 18}},
    { 6, 0,  0, 360, 368, {0, 1}, {1, 6}, {18, 10}}
};
extern const sbyte direction_step[];
static volatile int jaffar_motion_disabled = 0;
static byte blink_frame_order[6] = {0, 1, 2, 2, 1, 0};
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

extern void far pascal step_room_left(void);
extern void far pascal step_room_right(void);
extern void far pascal run_room_left(void);
extern void far pascal climb_room_right(void);
extern void far pascal cross_room_upper(void);
extern void far pascal cross_room_lower(void);
extern void far pascal start_room_climb(int mode);

int far pascal jaffar_go_to_position_0(sbyte near *stance)
{
    sbyte x = opponent_room + ((opponent_kind == 8) ? 1 : 0) * 10;
    int sequence = -1;
    const PositionRule near *rule = &position_rules[stance[1]];

    if (stance[2] == 3) {
        if (char_frame == 15) {
            if (x == 6) {
                step_room_left();
                goto done;
            }
            if (x > 7 && x < rule->room_x - 1)
                goto run_left;
            goto settle;
        }
        if (char_frame == 7 || char_frame == 11) {
            if ((x >= 5 && x <= 6) || (x >= 10 && x <= 11)) {
                start_room_climb(4);
                goto done;
            }
            if (x >= rule->room_x - 1)
                goto halt_run;
        }
    } else if (opponent_column == 2) {
        if (char_frame == 15) {
            if (char_direction == 0)
                goto step_right;
            goto turn;
        }
        if (char_frame == 91)
            goto climb_right;
    } else if (opponent_column == 1) {
        if (char_frame == 15) {
            if (char_direction == 0) {
                if (x < 17)
                    goto run_left;
            } else if (x >= 17) {
                goto step_right;
            }
turn:
            sequence = 5;
        } else {
            if (char_frame == 7 && x >= 17) {
                sequence = 13;
                goto done;
            }
            if (char_frame == 91)
                goto climb_right;
            if (char_frame == 109)
                sequence = 49;
        }
    } else {
settle:
        sequence = settle_jaffar_at_position(stance);
    }
    goto done;

run_left:
    run_room_left(); goto done;
halt_run:
    sequence = 13; goto done;
step_right:
    step_room_right(); goto done;
climb_right:
    climb_room_right();
done:
    return sequence;
}

int far pascal jaffar_go_to_position_1(sbyte near *stance)
{
    int sequence = -1; const PositionRule near *rule = &position_rules[stance[1]];

    if (stance[2] < stance[1]) {
        if (opponent_column == 0) {
            if (char_direction == 0 && opponent_sequence != 5) goto turn;
            if (char_frame == 15) cross_room_upper();
        } else if (opponent_column == 1) {
            if ((unsigned int)char_frame >= 87 && (unsigned int)char_frame <= 91) {
                cross_room_lower();
            } else if (char_frame == 15) {
                if (char_direction != -1) goto turn;
                goto run_left;
            } else if (char_frame == 109) {
                sequence = 49;
            }
        } else if (opponent_column == 2) {
            if (char_frame == 109) sequence = 49;
            else
                goto settle;
        }
    } else {
        sbyte x = opponent_room + ((opponent_kind == 8) ? 1 : 0) * 10;

        if (x == position_rules[2].room_x && char_frame == 15) goto run_left;
        if (x >= 6 && x <= 7) {
            start_room_climb(4);
        } else if (x > 10) {
            if (char_frame == 7 && x >= rule->room_x - 1) sequence = 13;
            else
                goto settle;
        }
    }
    goto done;

turn:
    sequence = 5; goto done;
run_left:
    run_room_left(); goto done;
settle:
    sequence = settle_jaffar_at_position(stance);
done:
    return sequence;
}

int far pascal jaffar_go_to_position_2(sbyte near *stance)
{
    int sequence = -1;

    if (stance[2] < stance[1]) {
        sbyte x = opponent_room + ((opponent_kind == 8) ? 1 : 0) * 10;

        if (x == position_rules[1].room_x && char_frame == 15) {
run_left:
            run_room_left();
        } else if (x >= 12 && x <= 13 && char_frame == 11) {
            start_room_climb(4);
        } else if (x <= 6 && char_frame == 7) {
            sequence = 13;
        } else {
            goto settle;
        }
    } else if (opponent_column == 0) {
        if (char_frame == 15) {
            if (char_direction == 0) {
turn:
                sequence = 5; goto done;
            }
            cross_room_upper();
        }
    } else if (opponent_column == 1) {
        if ((unsigned int)char_frame >= 87 && (unsigned int)char_frame <= 91) {
            cross_room_lower();
        } else if (char_frame == 15) {
            if (char_direction != -1) goto turn;
            goto run_left;
        } else if (char_frame == 109) {
jump_down:
            sequence = 49;
        }
    } else if (opponent_column == 2) {
        if (char_frame != 109) goto settle;
        sequence = 49;
    }
    goto done;
settle:
    sequence = settle_jaffar_at_position(stance);
done:
    return sequence;
}

int far pascal jaffar_go_to_position_3(sbyte near *stance)
{
    int sequence = -1; const PositionRule near *rule = &position_rules[stance[1]];
    sbyte x = opponent_room + ((opponent_kind == 8) ? 1 : 0) * 10;

    if (stance[2] == 0) {
        if (char_frame == 15) {
            if (x > 10) run_room_left();
            else if (x <= rule->room_x)
                goto settle;
        } else if (char_frame == 7 || char_frame == 11) {
            if (x <= rule->room_x) sequence = 13;
            else if ((x >= 14 && x <= 15) || (x >= 9 && x <= 10))
                start_room_climb(4);
        }
    } else if (opponent_column == 2) {
        if (char_frame == 15) {
step_right:
            step_room_right();
        } else if (char_frame == 91) {
climb_right:
            climb_room_right();
        }
    } else if (opponent_column == 1) {
        if (char_frame == 15) {
            if (char_direction != 0) goto step_right;
            if (x == 6) shift_character(32);
            else
                sequence = 5;
        } else {
            if (char_frame == 91) goto climb_right;
            if (char_frame == 109) sequence = 49;
        }
    } else {
settle:
        sequence = settle_jaffar_at_position(stance);
    }
    return sequence;
}

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

extern signed char shadow_range_status;
extern sbyte shadow_direction;
extern sbyte shadow_draw_phase;
extern sbyte shadow_stance;
extern unsigned int shadow_sequence;
extern int battle_progress;
extern int escape_counter;
extern int char_y;
extern int char_flail_x;
extern int char_flail_y;

extern int far pascal shadow_distance(void);
extern int far pascal align_character_x(int x);
extern sbyte far pascal cell_from_column(sbyte column, sbyte row);
extern int abs(int x);
#pragma intrinsic(abs)

void far pascal run_jaffar_in_crystal(void)
{
    if (opponent_ticks < 0) {
        sbyte count = room_state[(byte)opponent_kind].opponent_count;

        if (shadow_draw_phase == 1)
            --count;
        if (count == 1 && opponent_sequence != 240 && shadow_range_status < 0) {
            set_sequence(240);
            return;
        }
        if (shadow_direction == char_direction) {
            if (char_weapon == 0 && shadow_range_status < 0) {
                int range = shadow_distance() - 15;

                if (range > 18 && range < 48) {
                    char_x = align_character_x(15);
                    char_weapon = 1;
                    set_sequence(75);
                    return;
                }
            } else if (char_frame == 171) {
                char_weapon = 0;
                set_sequence(92);
                return;
            } else if (char_frame == 52 && shadow_range_status > 0) {
                set_sequence(197);
                play_sound(262);
            }
        }
    }
}

void far pascal run_jaffar_in_escher(void)
{
    int flee;
    int attack;
    sbyte slot;
    OpponentSlot near *opponent;
    sbyte near *stance;

    opponent = reserve_opponent((byte)opponent_kind, char_slot);
    stance = opponent->stance;
    if (opponent->stance[0] == 1 && stance[2] != stance[1])
        flee = 1;
    else
        flee = 0;

    attack = 0;
    if (shadow_draw_phase == 0) {
        if (shadow_stance == 1 &&
            !((unsigned int)char_frame >= 207 && (unsigned int)char_frame <= 210) &&
            char_frame != 158) {
            if (abs(shadow_distance()) < 62) {
                slot = char_slot;
                save_opponent();
                activate_kid();
                set_sequence(241);
                request_music(265);
                char_weapon = (byte)-1;
                other_character = active_character;
                activate_opponent(slot);
            }
        } else if (shadow_column == opponent_column && battle_progress >= 4) {
            if (shadow_life <= 2 && shadow_sequence != 242)
                flee = 0;
            else
                flee = 1;
        } else if (shadow_life <= 2 && shadow_stance == -1 && shadow_sequence != 242) {
            attack = 1;
        } else {
            attack = 0;
        }
    } else {
        if (shadow_life <= 2 && shadow_sequence != 242)
            flee = 0;
        else
            flee = 1;
        if (shadow_life <= 2 && shadow_sequence != 242)
            attack = 1;
        else
            attack = 0;
    }

    if (opponent_sequence == 243 && char_frame == 351)
        ++escape_counter;
    if (flee) {
        jaffar_flee_in_escher();
        return;
    }
    if (attack)
        jaffar_attack_in_escher();
}

void far pascal update_jaffar_slot(OpponentSlot near *opponent)
{
    sbyte near *stance = opponent->stance;

    if (!((entry_room_copy == 7 || entry_room_copy == 8) && environment_kind == 6) &&
        stance[2] != stance[1]) {
        const PositionRule near *rule;

        stance[2] = stance[1];
        rule = &position_rules[stance[1]];
        opponent_column = rule->flags;
        opponent_room = rule->room_x;
        char_x = rule->min_x + 6;
        if (opponent_kind == 8) {
            char_x -= 320;
            opponent_room -= 10;
        }
        char_direction = rule->direction;
        position_opponent();
        set_sequence(2);
        update_opponent();
        opponent->cell = cell_from_column(opponent_room, opponent_column);
        opponent->x = char_x;
        opponent->direction = char_direction;
        *(int near *)((byte near *)opponent + 7) = char_flail_x;
        *(int near *)((byte near *)opponent + 5) = char_flail_y;
    }
    *(int near *)(stance + 4) = char_y;
}

int far pascal shadow_in_range(sbyte position, sbyte direction)
{
    const PositionRule near *rule = &position_rules[position];
    int side;
    int inside;
    sbyte x;
    const sbyte near *side_fields;

    if (direction < 0)
        direction = 0;
    else
        direction = 1;
    side = direction;
    side_fields = (const sbyte near *)rule + side;
    if (side_fields[7] == shadow_column &&
        (x = shadow_offset + ((shadow_room == 8) ? 1 : 0) * 10) >= side_fields[9] &&
        x <= side_fields[11])
        inside = 1;
    else
        inside = 0;
    return inside;
}

extern int playing_scene;
extern unsigned int current_sound_bank;
extern unsigned int active_sound_bank;
extern byte scene_background;
extern void far pascal finish_room_transfer(void);
extern void far pascal load_scene_background(void far *work, int resource);
extern void far pascal draw_scene_background(void far *work);

void far pascal transfer_jaffar_to_escher2(void)
{
    opponent_room = 17;
    char_x = 712;
    opponent_column = -3;
    char_y = -133;
    char_direction = position_rules[0].direction;
    set_sequence(2);
    update_opponent();
    finish_room_transfer();
}

void far pascal load_tower_top(void)
{
    if (!playing_scene)
        fade_scene(3500, 64, 192, 0);
    if (current_sound_bank)
        active_sound_bank = current_sound_bank;
    load_scene_background(&scene_background, 139);
}

void far pascal load_chess(void)
{
    fade_scene(3500, 64, 192, 1);
    fade_scene(1000, 240, 16, 0);
    if (viewed_room == 4 && environment_kind == 6) {
        unsigned int previous = active_sound_bank;

        active_sound_bank = current_sound_bank;
        draw_scene_background(&scene_background);
        active_sound_bank = previous;
        request_music(268);
    } else if (viewed_room == 3 && environment_kind == 6) {
        fade_scene(3000, 224, 16, 0);
        request_music(269);
    }
}

typedef struct {
    int shape;
    sbyte x_offset, y_offset;
} SmokeFrame;

extern unsigned int smoke_frame_index;
extern unsigned int smoke_resource;
extern int smoke_shape;
extern int smoke_y;
extern byte smoke_mode;
extern byte smoke_shape_state;
extern SmokeFrame far *far pascal find_smoke_frames(unsigned int resource);
extern void far pascal change_smoke_x(int offset);
extern void far pascal finish_smoke_setup(void);
extern void far _fastcall select_smoke_layer(byte layer);
typedef void (far pascal *AnimationCallback)(void);
extern void far pascal update_frame_callback(void);
extern void far pascal spawn_temporary(AnimationCallback callback, void near *state, sbyte slot);

void far pascal setup_guard_smoke(void)
{
    if (char_style == 2 && (unsigned int)char_frame >= 229 && (unsigned int)char_frame <= 254) {
        unsigned int index = smoke_frame_index;
        SmokeFrame far *frames = find_smoke_frames(smoke_resource);
        SmokeFrame far *frame = &frames[index];

        if (frame->shape != -1) {
            smoke_shape = frame->shape;
            change_smoke_x(frame->x_offset);
            smoke_y += frame->y_offset;
            smoke_mode = 3;
            finish_smoke_setup();
            select_smoke_layer(9);
            spawn_temporary(update_frame_callback, &smoke_shape_state, char_slot);
        }
    }
}

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
/* Each frame selects a room image slot. Each shape pair packs two word offsets. */
static byte crystal_frame_slot[] = {
    0, 1, 2, 3, 2, 1, 0, 0
};
static unsigned long crystal_shapes[] = {
    0x0136000aUL, 0x002f0048UL, 0x008a0054UL,
    0x00ce0041UL, 0x00eb0054UL, 0x000e0082UL,
    0x00340088UL, 0x008000a4UL, 0x010b009bUL
};
static byte crystal_layer[] = {
    1, 0, 0, 0, 0, 1, 1, 1, 1, 0
};
static byte gargoyle_frame_slot[] = {
    0, 1, 2, 4, 3, 4, 2, 1, 0, 0
};
static unsigned long gargoyle_shapes[] = {
    0x00240043UL, 0x003f0043UL, 0x005e0043UL,
    0x007b0043UL, 0x009a0043UL, 0x00710083UL,
    0xffff0044UL, 0x0019007eUL, 0x00d60083UL
};
static byte gargoyle_layer[] = {
    0, 0, 0, 0, 0, 1, 0, 1, 1, 0
};
extern void far pascal image_bounds(Rect far *output, Rect far *input, unsigned long shapes);
extern void far _fastcall update_final_image(byte index, byte layer);

extern unsigned int final_animation;
extern int blink_always;
extern byte final_object_visibility;
extern int fade_active;
extern byte crystal_serial;
static byte gargoyle_serial;
extern unsigned int (*near tile_effects)[2];

extern void far pascal append_tile_effect(sbyte row, sbyte col, byte value, byte effect);
extern void far pascal finish_blink_frame(void);

#define ROOM_RECT(offset) ((Rect near *)((byte near *)room_image_table + (offset)))

void far pascal animate_jaffar_blink(void)
{
    unsigned int frame;

    ++final_animation;
    frame = final_animation & 15;
    if (frame >= 5 && room_image_table->attributes[1] == 26) {
        frame -= 5;
        if (frame < 6) {
            spawn_temporary(finish_blink_frame, ROOM_RECT(89), -1);
            return;
        }
        final_object_visibility = (byte)-1;
        final_animation = 0;
    }
}

void far pascal draw_jaffar_blink(FinalObject near *object)
{
    unsigned int frame = object->animation & 15;

    if ((blink_always || frame >= 5) && object->kind == 11) {
        if (!blink_always)
            frame -= 5;
        if (frame < 6)
            update_final_image(blink_frame_order[frame] + 2, 11);
    }
}

void far pascal animate_spider(void)
{
    if (room_image_table->attributes[1] == 25) {
        unsigned int state = final_animation;
        int ticks = (state & 0x7f8) >> 3;

        if (state & 0x800) {
            state = bounded_random(5);
            if (--ticks > 0 && !blink_always) {
                state |= 0x800;
            } else {
                ticks = bounded_random(60) + 30;
            }
            spawn_temporary(finish_blink_frame, ROOM_RECT(189), -1);
        } else if (--ticks <= 0) {
            if (!fade_active) {
                ticks = bounded_random(2) + 1;
                state |= 0x800;
            } else {
                ticks = bounded_random(60) + 30;
            }
        }
        state &= 0xf807;
        final_animation = (ticks << 3) | state;
        return;
    }
    final_object_visibility = (byte)-1;
}

extern int sprite_vertical_origin;

void far pascal draw_spider(FinalObject near *object)
{
    if (object->kind == 11) {
        int previous = sprite_vertical_origin;

        sprite_vertical_origin -= 6;
        update_final_image((object->animation & 7) + 6, 11);
        sprite_vertical_origin = previous;
    }
}

void far pascal setup_crystal_room(void)
{
    fade_scene(3500, 64, 192, 2);
    fade_scene(1000, 240, 16, 0);
    fade_scene(25001, 32, 16, 7);
    crystal_serial = 0;
    if (viewed_room == 7) {
        fade_scene(3000, 224, 16, gargoyle_serial = 0);
        request_music(263);
    } else if (viewed_room == 8) {
        gargoyle_serial = 6;
        request_music(263);
    } else {
        request_music(270);
    }
}

void far pascal animate_crystal(void)
{
    if (room_image_table->attributes[1] == 28) {
        unsigned int state = final_animation;

        if (state & 0x800) {
            unsigned int frame = ((state & 0x7f0) >> 4) + 1;

            state &= 0xf80f;
            if (frame < 7) {
                Rect bounds;
                const unsigned long near *shape = &crystal_shapes[state & 15];

                state |= frame << 4;
                image_bounds(&bounds, ROOM_RECT(139), *shape);
                spawn_temporary(finish_blink_frame, &bounds, -1);
            } else {
                state |= (bounded_random(40) + 40) << 4;
                state &= ~0x800;
            }
        } else {
            int ticks = (state & 0x7f0) >> 4;

            state &= 0xf80f;
            if (--ticks <= 0) {
                if (fade_active && defeated_opponents)
                    state |= (bounded_random(40) + 40) << 4;
                else
                    state |= 0x800;
            } else {
                state |= ticks << 4;
            }
        }
        final_animation = state | 0x1000;
        return;
    }
    final_object_visibility = (byte)-1;
}

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
    image_bounds(&image->bounds, &image->bounds, *(volatile unsigned long near *)&image->shape_list);
    image->layer = crystal_layer[shape];
    update_final_image(index, crystal_layer[shape]);
    image->layer = 11;
    image->bounds = bounds;
    ((unsigned int near *)&image->shape_list)[0] = 0;
    ((unsigned int near *)&image->shape_list)[1] = 0;
}

void far pascal trigger_crystal(sbyte row, sbyte col)
{
    unsigned int serial = crystal_serial++;
    unsigned int ticks = bounded_random(40);

    tile_effects[col][0] = ((ticks | 0x100) << 4) | serial;
    append_tile_effect(row, col, 1, 41);
}

void far pascal animate_gargoyle(void)
{
    if (room_image_table->attributes[1] == 29 || room_image_table->attributes[1] == 30) {
        unsigned int state = final_animation;
        byte serial = (byte)(state & 15);

        if ((serial < 6 && viewed_room == 7) || (serial >= 6 && viewed_room == 8)) {
            if (state & 0x800) {
                unsigned int frame = ((state & 0x7f0) >> 4) + 1;

                                if (frame < 9) {
                    Rect bounds;

                    state = (state & 0xf80f) | (frame << 4);
                    image_bounds(&bounds, ROOM_RECT(89), gargoyle_shapes[serial]);
                    spawn_temporary(update_frame_callback, &bounds, -1);
                } else {
                    state &= 0xf80f;
                    state |= (bounded_random(40) + 30) << 4;
                    state &= ~0x800;
                }
            } else {
                int ticks = (state & 0x7f0) >> 4;

                state &= 0xf80f;
                if (--ticks > 0)
                    state |= ticks << 4;
                else if (fade_active && defeated_opponents)
                    state |= (bounded_random(40) + 30) << 4;
                else
                    state |= 0x800;
            }
            final_animation = state | 0x1000;
            return;
        }
    }
    final_object_visibility = (byte)-1;
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
    image_bounds(&image->bounds, &image->bounds, *(volatile unsigned long near *)&image->shape_list);
    image->layer = gargoyle_layer[shape];
    update_final_image(index, gargoyle_layer[shape]);
    image->layer = 11;
    image->bounds = bounds;
    ((unsigned int near *)&image->shape_list)[0] = 0;
    ((unsigned int near *)&image->shape_list)[1] = 0;
}


void far pascal trigger_gargoyle(sbyte row, sbyte col)
{
    unsigned int serial = gargoyle_serial++;
    unsigned int ticks = bounded_random(40);

    tile_effects[col][0] = ((ticks | 0x100) << 4) | serial;
    append_tile_effect(row, col, 1, 42);
}

extern int fireball_x, fireball_y;
extern byte fireball_room;
extern int fireball_active;
extern volatile int fireball_velocity;
extern unsigned int fireball_phase;
extern byte fireball_kind, fireball_row;
extern byte left_room, right_room;
extern int fireball_half_width, fireball_height;
extern int player_left, player_right;
extern byte object_bucket;
extern unsigned int fireball_sheet;
extern int fireball_draw_x, fireball_draw_y;
extern int object_counter;
extern sbyte other_strength;

extern byte far pascal find_row_for_y(int y);
extern byte far pascal shape_position(sbyte col, byte row);
extern void far *far pascal check_shape_dimensions(unsigned int sheet, unsigned int image, int far *size);
extern void far pascal submit_fireball_y(int y);
extern void far pascal update_fireball_shape(byte changed);
extern int far pascal tile_blocks_fireball(sbyte tile);
extern void far pascal remove_fireball_image(unsigned int image);
extern void far pascal activate_fireball(void);
extern void far _fastcall queue_fireball(byte layer, int frame, int x, int y, byte flags);

typedef struct {
    int x;
    byte unknown0[2];
    byte room;
    byte unknown1[2];
    int velocity;
    byte unknown2;
    sbyte row;
    unsigned int flags;
} FireballSlot;

typedef struct {
    byte coordinates[10];
    unsigned int flags;
    byte mode;
    byte reserved[7];
} FireballQueueItem;

extern FireballSlot near *far pascal find_fireball_slot(byte kind, int index);
extern _segment fireball_queue_segment;
extern FireballQueueItem _based(fireball_queue_segment) fireball_queue[];
int far _fastcall get_fireball_image(unsigned int state);
void far pascal trigger_fireball(void);

void far pascal update_fireball_view(void)
{
    int visible = 1;
    volatile int y = fireball_y;

    if (fireball_room == viewed_room) {
        if (fireball_x > 449 || fireball_x < 0)
            visible = 0;
    } else if (left_room == fireball_room) {
        if (fireball_x + fireball_half_width > 320) {
            fireball_x -= 320;
            fireball_room = viewed_room;
        } else {
            visible = 0;
        }
    } else if (right_room == fireball_room) {
        if (fireball_x - fireball_half_width < 320) {
            fireball_x += 320;
            fireball_room = viewed_room;
        } else {
            visible = 0;
        }
    } else {
        visible = 0;
    }

    if (visible) {
        sbyte col = fireball_x / 32;
        unsigned int image;
        int size[2];

        object_bucket = shape_position(col, find_row_for_y(y));
        image = get_fireball_image(fireball_phase);
        if (check_shape_dimensions(fireball_sheet, image, size)) {
            register int saved_y;
            int x = fireball_x;
            saved_y = y;
            fireball_x -= size[1] / 2;
            fireball_y = saved_y + size[0] / 2;
            fireball_height = size[0];
            fireball_half_width = size[1];
            submit_fireball_y(fireball_y);
            update_fireball_shape(1);
            fireball_y = saved_y;
            fireball_x = x;
        }
    }
}

void far pascal check_fireball_hit_character(void)
{
    if (char_style == 6 && opponent_ticks < 0 && opponent_sequence != 243) {
        int index = 1;
        FireballSlot near *ball;

        do {
            int offset;

            ball = find_fireball_slot(12, index);
            if (ball && ball->row == opponent_column && !(ball->flags & 8)) {
                if (ball->room == (byte)opponent_kind)
                    offset = 0;
                else if (opponent_kind == 8 && environment_kind == 6)
                    offset = -320;
                else
                    offset = 320;
                offset += ball->x + ball->velocity + 113;
                if (offset + 34 > player_left && offset < player_right) {
                    ball->flags = 8;
                    ball->x = ball->velocity > 0 ? player_left - 125 : player_right - 135;
                    play_sound(62);
                    begin_final_attack(100);
                    set_sequence(243);
                    update_opponent();
                    request_music(267);
                }
            }
            ++index;
        } while (ball);
    }
}

void far pascal check_fireball_hit_wall(void)
{
    int x = fireball_x + fireball_half_width - 14;
    sbyte col;

    if (fireball_velocity < 0)
        x -= 2 * fireball_half_width;
    col = x / 32;
    if (x < 0)
        --col;
    if (tile_blocks_fireball(tile_at(fireball_room, col, fireball_row))) {
        fireball_x = col * 32 + 14;
        if (fireball_velocity < 0)
            fireball_x += 32;
        fireball_phase = 8;
        play_sound(259);
        remove_fireball_image(0x281a);
    }
}

int far pascal do_throw_fireball(void)
{
    register int sequence;
    if ((sbyte)char_strength > 2) {
        sequence = 242;

        begin_final_attack(2);
        request_music(266);
    } else {
        sequence = -1;
    }
    return sequence;
}

void far pascal draw_fireball_object(FinalObject near *object)
{
    int image = get_fireball_image(smoke_shape);

    queue_fireball(2, image + 1, fireball_draw_x, fireball_draw_y, 10);
    fireball_queue[object_counter].flags = 8;
    fireball_queue[object_counter].mode = 0;
}

int far _fastcall get_fireball_image(unsigned int state)
{
    unsigned int frame = state & 7;
    int number;

    if (state & 8) {
        number = frame;
        number += 308;
    } else {
        number = frame;
        number += 304;
    }
    return number;
}

void far pascal move_fireball(void)
{
    if (fireball_active >= 0) {
        unsigned int frame = fireball_phase & 7;

        if (fireball_phase & 8) {
            if (frame >= 5)
                fireball_active = -1;
            else
                ++frame;
        } else {
            if (fireball_velocity > 0) {
                if (fireball_velocity < 16)
                    fireball_velocity += 2;
            } else if (fireball_velocity > -16) {
                fireball_velocity -= 2;
            }
            fireball_x += fireball_velocity;
            if (fireball_room == 0) {
                fireball_active = -1;
                remove_fireball_image(0x2812);
            } else {
                check_fireball_hit_wall();
                if (!(fireball_phase & 8)) {
                    if (frame >= 3)
                        frame = 0;
                    else
                        ++frame;
                    play_sound(258);
                }
            }
        }
        if (fireball_active == -1) {
            if (other_strength <= 2) {
                fade_scene(2000, 48, 16, 0);
                return;
            }
        } else {
            fireball_phase &= ~7;
            fireball_phase |= frame;
        }
    }
}

void far pascal throwing_fireball(void)
{
    if (char_frame == 281)
        trigger_fireball();
    play_sound(258);
}

void far pascal trigger_fireball(void)
{
    int y;

    fireball_x = align_character_x(27) - 130;
    fireball_y = y = char_y - 29;
    fireball_room = (byte)opponent_kind;
    fireball_active = 0;
    fireball_velocity = direction_step[char_direction] * 8;
    fireball_kind = 12;
    fireball_row = find_row_for_y(y);
    fireball_phase = 2;
    check_fireball_hit_wall();
    activate_fireball();
    play_sound(258);
}
