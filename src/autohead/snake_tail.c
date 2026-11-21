/* AUTOHEAD.C. Routines in original DOS order. */

#pragma pack(1)
typedef struct {
    int x, y;
    unsigned char kind;
    int frame;
    int step;
    unsigned char reserved[6];
    unsigned char opponent_kind, padding;
    signed char opponent_mode;
} HeadSprite;

typedef struct {
    signed char width, height, top, left;
} SpriteBounds;

typedef struct {
    signed char x, y;
} NeckPoint;
#pragma pack()

extern signed char actor_facing, body_facing;
extern unsigned char actor_kind;
extern int actor_x, sprite_x, sprite_y, kid_sheet, kid_frame;
extern void near *kid_shapes;

extern HeadSprite near *far pascal find_sprite(unsigned char kind, unsigned char index);
extern NeckPoint far *far pascal lock_shape_data(void near *handle);
extern void far pascal sprite_dimensions(int sheet, int frame, SpriteBounds far *bounds);
extern void far pascal unlock_shape_data(void near *handle);

typedef struct {
    signed char mode, column, row, direction;
    unsigned char flags, sound_played;
} HeadPlan;

typedef struct {
    unsigned char prefix[15], kind, reserved;
    signed char mode, column, row;
    int timer;
    unsigned char sound_played;
} OpponentRecord;

extern signed char target_column, target_row, target_life, target_facing;
extern signed char actor_slot, actor_column, actor_row, actor_life;
extern signed char actor_velocity_x, actor_velocity_y, camera_actor, actor_alert, head_active;
extern unsigned char actor_room, level_head_kind, target_visible, tile_flags, target_room;
extern unsigned char tile_modifier;
extern unsigned int actor_frame, target_frame, target_progress;
extern int actor_y, target_x, actor_sequence, actor_timer;
extern signed char tile_test_x;
extern int current_sprite_frame, snake_distance;
extern unsigned char collision_room, collision_column, collision_row;
extern unsigned char room_opponents[][116];
extern signed char first_chomp[], last_chomp[];
extern int flee_length[], cell_x[];
extern signed char move_with_facing[], move_against_facing[];

extern signed char far pascal read_tile(unsigned char room, signed char column, signed char row);
extern int far pascal tile_blocks_head(signed char tile);
extern int far pascal tile_allows_vertical(signed char tile);
extern int far pascal tile_has_floor(signed char tile);
extern int far pascal head_has_target(void);
extern int far pascal gate_blocks_head(void);
extern int far pascal bounded_random(int maximum);
extern void far pascal reset_char_y(void);
extern void far pascal clear_char_velocity(void);
extern void far pascal play_sound(int sound);
extern void far pascal play_loop_sound(int sound);
extern int far pascal sound_is_playing(int sound);
extern int far pascal lose_life(signed char amount);
extern int far pascal offset_char_x(int amount);
extern void far pascal select_animation_sequence(int sequence);
extern int far pascal target_wall_gap(unsigned char room, unsigned char column, unsigned char row);
extern signed char far pascal actor_x_to_column(void);
extern signed char far pascal random_open_column(int room);
extern int far pascal get_move_room(void);
extern int far pascal target_distance(void);
extern void far pascal clear_actor_motion(void);
extern void far pascal set_actor_speed(signed char id, int x, int y);
extern void far pascal refresh_actor_movement(signed char id);
extern OpponentRecord near *far pascal get_opponent_info(int room, int id);
extern void far pascal load_actor_slot(signed char id);
extern void far pascal store_actor_slot(void);
extern void far pascal advance_actor_frame(void);
extern void far pascal update_actor_sprite(void);
extern signed char far pascal get_cells_ahead(signed char direction);
extern int far pascal tile_blocks_snake(signed char tile);
extern signed char far pascal last_clear_column(unsigned char room, signed char column,
                                                signed char row, signed char direction);
extern signed char far pascal tile_under_actor(void);
extern int far pascal distance_into_cell(void);

int far pascal head_clear_in_dir(int direction);
int far pascal head_go_to_dest(HeadPlan near *plan);
void far pascal head_face_kid(void);
void far pascal head_get_angry(HeadPlan near *plan);
void far pascal head_get_peaceful(HeadPlan near *plan);
void far pascal head_focus_on_neck(HeadPlan near *plan);
int far pascal head_set_to_flee_mode(void);
int far pascal is_able_to_flee_in_room(unsigned char room);
int far pascal just_start_vertical_seq(void);
int far pascal XWithinMiddleOfCell(int first, register int second);
int far pascal YWithinMiddleOfCell(int value, register int half_width);
int far pascal snake_slithering_toward_opp(void);
void far pascal snake_attack(OpponentRecord near *record);
void far pascal snake_come_out_of_hole(OpponentRecord near *record);
void far pascal snake_go_into_hole(OpponentRecord near *record);

int abs(int value);
#pragma intrinsic(abs)

void far pascal snake_go_into_hole(OpponentRecord near *record)
{
    signed char tile;
    int distance;
    if (target_life >= 0) {
        if (actor_sequence != 176 && actor_frame != 278 && actor_frame != 279)
            select_animation_sequence(176);
        return;
    }
    if (actor_sequence != 171 && actor_sequence != 172 && actor_timer != 9) {
        tile_under_actor();
        if ((tile_modifier & 15) == 12) {
            distance = (actor_x - 144) % 32;
            if (distance >= 14 && distance <= 18) {
                distance = 16 - distance;
                if (actor_facing == -1)
                    distance = -distance;
                else
                    distance += 8;
                actor_x = offset_char_x(distance);
                select_animation_sequence(171);
            }
        } else if (actor_sequence != 176 && actor_sequence != 168 &&
                   actor_frame != 278 && actor_frame != 279) {
            tile = get_cells_ahead(1);
            if (tile_blocks_snake(tile) || (tile == 7 && (tile_flags & 128)))
                return;
            if (!tile_blocks_head(tile))
                select_animation_sequence(176);
            else {
                if (tile != 7) {
                    if (actor_facing == 0) {
                        actor_timer = 9;
                        return;
                    }
                    if (actor_frame != 254)
                        return;
                }
                select_animation_sequence(168);
            }
        } else {
            tile = get_cells_ahead(1);
            if (tile_blocks_snake(tile) || (tile == 7 && (tile_flags & 128)))
                select_animation_sequence(177);
        }
    } else {
        if (actor_timer == 9) {
            tile = tile_under_actor();
            distance = distance_into_cell();
            if (tile_blocks_head(tile) && distance < 8) {
                actor_frame = 264;
                select_animation_sequence(168);
            }
        }
        if (actor_frame == 264) {
            actor_timer = 6;
            record->mode = actor_alert == 2 ? 6 : bounded_random(60) + 36;
        }
    }
}

int far pascal snake_slithering_toward_opp(void)
{
    int result = 0;
    if (actor_sequence != 171 && actor_alert >= 2) {
        if (actor_facing == 0) {
            signed char column = actor_column;
            if (target_column >= column)
                goto toward;
        }
        if (actor_facing == -1) {
            signed char column = actor_column;
            if (target_column <= column)
toward:
                result = 1;
        }
    }
    return result;
}

void far pascal snake_update_opp_info(OpponentRecord near *record)
{
    signed char tile;
    if (actor_sequence == 171 || actor_timer == 9) {
        actor_timer = 6;
        record->mode = bounded_random(60) + 36;
    } else {
        tile = get_cells_ahead(1);
        if (!tile_blocks_snake(tile) && (tile != 7 || !(tile_flags & 128)) &&
            (actor_sequence == 176 || actor_sequence == 168 || actor_sequence == 170)) {
            record->prefix[3] = ~actor_facing;
            *(int near *)&record->prefix[1] = offset_char_x(-32);
            return;
        }
    }
}
