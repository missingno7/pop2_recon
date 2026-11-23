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
extern Byte room_number, viewed_room, plateau_intro;
extern volatile Byte CutRoom;
extern Byte *ruins_start_actor;
extern int ColEdgeX[20];
extern int plateau_room_loaded;
extern int ruins_start_loaded;

extern CellCoord get_col(int);
extern Byte ReadCell(CellCoord, CellCoord, CellCoord);
extern int IsWall(Byte);
extern int is_six_kind(Byte);
extern void add_mob(void);
extern void make_space(CellCoord, CellCoord);
extern void mark_mob_redraw(Byte);
extern void JumpSeq(int);
extern void reset_char_y(void);
extern void AnimChar(void);
extern void open_start_gate(void);
extern Byte ClrAll(void);
extern int load_palette_bank(unsigned int, int, Byte, Byte);
extern void AddSong(int);
extern ActorState Char, Kid;

void clip_ruins_actor(void)
{
    int clip_lower;
    CellCoord column, left_column;
    int previous_cell_remaining, current_cell_offset;

    clip_lower = actor_id == 7 || actor_id == 8;
    column = get_col(current_actor_x);
    if (IsWall(ReadCell(actor_room, column, actor_row))) {
        left_column = ReadCell(actor_room, column, left_cell_row);
        if (IsWall(left_column) && left_column != 7 && actor_room == tile_room)
            actor_edge_y = tile_column * 32;
    }
    if (clip_lower && actor_frame != 137 && actor_frame != 138 &&
        actor_frame != 143 && actor_frame > 144 &&
        actor_frame != 143 && actor_frame != 144) {
        left_column = get_col(previous_actor_x);
        if (get_col(current_actor_x) != left_column) {
            previous_cell_remaining = 32 - (previous_actor_x - 130) % 32;
            current_cell_offset = (current_actor_x - 32) % 32;
            if (previous_cell_remaining > current_cell_offset)
                --column;
        }
        if (!is_six_kind(ReadCell(actor_room, column, left_cell_row)))
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
    add_mob();
    make_space(tile_room, tile_index);
    saved_x = spike_x;
    if (room_number == spike_room)
        spike_x -= 320;
    mark_mob_redraw(0);
    spike_x = saved_x;
}

#pragma optimize("e", off)
void cut_kid_inside_level6(void)
{
    CellCoord start_room;

    actor_action = 0;
    JumpSeq(5);
    start_room = 3;
    actor_room = start_room;
    CutRoom = start_room;
    actor_column = 4;
    actor_x = ColEdgeX[9] + 14;
    actor_row = 2;
    reset_char_y();
    AnimChar();
    Kid = Char;
    open_start_gate();
    ClrAll();
    plateau_intro = 0;
}

#pragma optimize("e", on)

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
        if (viewed_room == 16 && level_number == 9) {
            AddSong(92);
            return;
        }
        if (viewed_room == 2 && level_number == 9 && !plateau_room_loaded) {
            AddSong(91);
            plateau_room_loaded = 1;
        }
    }
}
