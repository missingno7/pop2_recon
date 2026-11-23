/* MOVERUIN.C, DOS overlay 8 code frame 2a4e. */
typedef signed char CellCoord;
typedef unsigned char Byte;

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

extern CellCoord tile_room;
extern CHARREC Char;
extern Byte level_number;
extern CHARREC Kid;
extern int previous_actor_x;
extern int current_actor_x;
extern int actor_edge_y, actor_floor_x;
extern CellCoord tile_index, tile_column, tile_row;
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

void clip_ruins_actor(void)
{
    int clip_lower;
    CellCoord column, left_column;
    int previous_cell_remaining, current_cell_offset;

    clip_lower = Char.character_kind == 7 || Char.character_kind == 8;
    column = get_col(current_actor_x);
    if (IsWall(ReadCell(Char.room, column, Char.row))) {
        left_column = ReadCell(Char.room, column, left_cell_row);
        if (IsWall(left_column) && left_column != 7 && Char.room == tile_room)
            actor_edge_y = tile_column * 32;
    }
    if (clip_lower && (int)Char.sequence != 137 && (int)Char.sequence != 138 &&
        (int)Char.sequence != 143 && (int)Char.sequence > 144 &&
        (int)Char.sequence != 143 && (int)Char.sequence != 144) {
        left_column = get_col(previous_actor_x);
        if (get_col(current_actor_x) != left_column) {
            previous_cell_remaining = 32 - (previous_actor_x - 130) % 32;
            current_cell_offset = (current_actor_x - 32) % 32;
            if (previous_cell_remaining > current_cell_offset)
                --column;
        }
        if (!is_six_kind(ReadCell(Char.room, column, left_cell_row)))
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

    Char.direction = 0;
    JumpSeq(5);
    start_room = 3;
    Char.room = start_room;
    CutRoom = start_room;
    Char.column = 4;
    Char.x = ColEdgeX[9] + 14;
    Char.row = 2;
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
    if (Kid.alive < 0 && ruins_start_loaded)
        load_palette_bank(3500, 64, 160, 0);
}

void LoadRuinsMoveCode(void)
{
}

void load_plateau_room(void)
{
    if (Kid.column >= 9) {
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
