/* MOVERUIN.C, DOS overlay 8 code frame 2a4e. */
#include "types.h"
#include "level.h"
#include "char.h"
#include "game.h"

extern CELLREC Cell;
extern CHARREC Char;

extern LEVELREC Level;
extern CHARREC Kid;
extern int CDLeftEdge;
extern int CDRightEdge;

extern FCHARREC FChar;

extern ROOMREC Room;
extern MOBREC Mob;

extern Directions Dir;
extern byte CutRoom;
extern byte *ruins_start_actor;
extern int ColEdgeX[20];

extern GAMESTATE GameState;
extern int Minutes;

extern sbyte get_col(int);
extern byte ReadCell(sbyte, sbyte, sbyte);
extern int IsWall(byte);
extern int is_six_kind(byte);
extern void add_mob(void);
extern void make_space(sbyte, sbyte);
extern void mark_mob_redraw(byte);
extern void JumpSeq(int);
extern void reset_char_y(void);
extern void AnimChar(void);
extern void open_start_gate(void);
extern byte ClrAll(void);
extern int set_palette(unsigned int, int, byte, byte);
extern void AddSong(int);

void clip_ruins_actor(void)
{
    int clip_lower;
    sbyte column, left_column;
    int previous_cell_remaining, current_cell_offset;

    clip_lower = Char.character_kind == 7 || Char.character_kind == 8;
    column = get_col(CDRightEdge);
    if (IsWall(ReadCell(Char.room, column, Char.row))) {
        left_column = ReadCell(Char.room, column, (Cell.top_row));
        if (IsWall(left_column) && left_column != 7 && Char.room == Cell.room)
            FChar.clip.right = Cell.column * 32;
    }
    if (clip_lower && Char.sequence != 137 && Char.sequence != 138 &&
        Char.sequence != 143 && Char.sequence > 144 &&
        Char.sequence != 143 && Char.sequence != 144) {
        left_column = get_col(CDLeftEdge);
        if (get_col(CDRightEdge) != left_column) {
            previous_cell_remaining = 32 - (CDLeftEdge - 130) % 32;
            current_cell_offset = (CDRightEdge - 32) % 32;
            if (previous_cell_remaining > current_cell_offset)
                --column;
        }
        if (!is_six_kind(ReadCell(Char.room, column, (Cell.top_row))))
            FChar.clip.top = 63 * ((sbyte)Cell.bottom_row) + 3;
    }
}

void fall_spikes(void)
{
    int saved_x;

    Mob.x = (Cell.index % 10) * 32;
    Mob.y = 63 * Cell.row + 66;
    Mob.room = (Cell.room);
    Mob.velocity = Mob.auxiliary = 0;
    Mob.kind = 3;
    Mob.row = Cell.row;
    Mob.flags = 0;
    add_mob();
    make_space((Cell.room), Cell.index);
    saved_x = Mob.x;
    if (Room.left == Mob.room)
        Mob.x -= 320;
    mark_mob_redraw(0);
    Mob.x = saved_x;
}

void cut_kid_inside_level6(void)
{
    sbyte start_room;

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
    Dir.action = 0;
}

void load_ruins_start_palette(void)
{
    set_palette(3500, 64, 160, 1);
}

void unload_ruins_start_palette(void)
{
    if (Kid.alive < 0 && Minutes)
        set_palette(3500, 64, 160, 0);
}

void LoadRuinsMoveCode(void)
{
}

void load_plateau_room(void)
{
    if (Kid.column >= 9) {
        if (Room.number == 16 && (Level.level_number) == 9) {
            AddSong(92);
            return;
        }
    } else if (Room.number == 2 && (Level.level_number) == 9 && !GameState.plateau_loaded
               && Kid.column < 5) {
        AddSong(91);
        GameState.plateau_loaded = 1;
    }
}
