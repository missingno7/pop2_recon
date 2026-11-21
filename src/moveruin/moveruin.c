/* MOVERUIN.C, DOS overlay 8 code frame 2a4e. */
typedef signed char CellCoord;
typedef unsigned char Byte;

typedef struct ActorState {
    Byte bytes[64];
} ActorState;

extern Byte actor_id, actor_action, level_number;
extern CellCoord actor_room, actor_column, actor_row;
extern CellCoord kid_direction, kid_column;
extern int actor_frame;
extern int previous_actor_x;
extern int current_actor_x;
extern int actor_edge_y, actor_floor_x, actor_x;
extern CellCoord tile_index, tile_room, tile_column, tile_row;
extern CellCoord left_cell_row, left_cell_column;
extern int spike_x, spike_y;
extern int spike_top, spike_left;
extern volatile int spike_sequence;
extern Byte spike_room;
extern volatile CellCoord spike_row;
extern volatile Byte spike_action;
extern Byte room_number, ruins_start_column, plateau_intro;
extern volatile Byte passed_plateau_room;
extern Byte *ruins_start_actor;
static int start_room_column_x[20] = {
    -30, 2, 34, 66, 98, 130, 162, 194, 226, 258,
    290, 322, 354, 386, 418, 450, 482, 514, 546, 578
};
extern int plateau_room_loaded;
extern int ruins_start_loaded;

extern CellCoord column_from_x(int);
extern Byte select_tile(CellCoord, CellCoord, CellCoord);
extern int IsWall(Byte);
extern int is_six_kind(Byte);
extern void add_spike_actor(void);
extern void redraw_spike(CellCoord, CellCoord);
extern void draw_spike(Byte);
extern void initialize_cutscene_actor(int);
extern void update_actor_frame(void);
extern void update_actor_coordinates(void);
extern void open_start_gate(void);
extern Byte ClrAll(void);
extern int load_palette_bank(unsigned int, int, Byte, Byte);
extern void queue_room_event(int);
extern ActorState Char, Kid;

void clip_ruins_actor(void)
{
    int clip_lower;
    CellCoord column, left_column;
    int previous_x, current_x;

    clip_lower = actor_id == 7 || actor_id == 8;
    column = column_from_x(current_actor_x);
    if (IsWall(select_tile(actor_room, column, actor_row))) {
        left_column = select_tile(actor_room, column, left_cell_row);
        if (IsWall(left_column) && left_column != 7 && actor_room == tile_room)
            actor_edge_y = tile_column * 32;
    }
    if (clip_lower && actor_frame != 137 && actor_frame != 138 &&
        actor_frame != 143 && actor_frame > 144 &&
        actor_frame != 143 && actor_frame != 144) {
        left_column = column_from_x(previous_actor_x);
        if (column_from_x(current_actor_x) != left_column) {
            previous_x = previous_actor_x - 130;
            current_x = current_actor_x - 32;
            if (current_x % 32 + previous_x % 32 < 32)
                --column;
        }
        if (!is_six_kind(select_tile(actor_room, column, left_cell_row)))
            actor_floor_x = 63 * left_cell_column + 3;
    }
}

void fall_spikes(void)
{
    int saved_x;

    spike_x = (tile_index % 10) * 32;
    spike_y = 63 * tile_row + 66;
    spike_room = tile_room;
    spike_top = spike_left = 0;
    spike_action = 3;
    spike_row = tile_row;
    spike_sequence = 0;
    add_spike_actor();
    redraw_spike(tile_room, tile_index);
    saved_x = spike_x;
    if (room_number == spike_room)
        spike_x -= 320;
    draw_spike(0);
    spike_x = saved_x;
}

void cut_kid_inside_level6(void)
{
    CellCoord start_room;

    actor_action = 0;
    initialize_cutscene_actor(5);
    start_room = 3;
    actor_room = start_room;
    passed_plateau_room = start_room;
    actor_column = 4;
    actor_x = start_room_column_x[9] + 14;
    actor_row = 2;
    update_actor_frame();
    update_actor_coordinates();
    Kid = Char;
    open_start_gate();
    ClrAll();
    plateau_intro = 0;
}

void load_ruins_start_palette(void)
{
    load_palette_bank(3500, 64, 160, 1);
}

void unload_ruins_start_palette(void)
{
    if (kid_direction < 0 && ruins_start_loaded)
        load_palette_bank(3500, 64, 160, 0);
}

void LoadRuinsMoveCode(void)
{
}

void load_plateau_room(void)
{
    if (kid_column >= 9) {
        if (ruins_start_column == 16 && level_number == 9) {
            queue_room_event(92);
            return;
        }
        if (ruins_start_column == 2 && level_number == 9 && !plateau_room_loaded) {
            queue_room_event(91);
            plateau_room_loaded = 1;
        }
    }
}






