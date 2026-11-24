/* CTRL.C reconstruction. See notes.md for claims and unresolved bodies.
 * Functions follow original order. */
#include "types.h"
#include "level.h"
#include "char.h"
#include "game.h"

extern ROOMREC Room;

extern CELLREC Cell;
extern unsigned char *BlueT;
/* Each modifier entry occupies two words. ReadCell uses its first word. */
extern unsigned int (*BlueS)[2];

extern LEVELREC Level;

extern void far pascal select_room_arrays(unsigned char room);
char far pascal FirstInRow(signed char value);
char far pascal GetUndefineCellId(signed char x, signed char y);

int step_in_progress = 0;
unsigned int far pascal Handler(void)
{
    struct {
        unsigned char room;
        signed char fix_x;
    } local;

    local.fix_x = 0;
    local.room = Cell.room;

    if (Cell.column < 0) {
        local.fix_x = 10;
        if (local.room) {
            local.room = Level.room_links[local.room - 1].left;
            if (local.room) {
                Cell.column += 10;
                local.fix_x = 0;
                Cell.room = local.room;
                Handler();
            }
        }
    } else if (Cell.column >= 10) {
        local.fix_x = -10;
        if (Cell.room) {
            local.room = Level.room_links[Cell.room - 1].right;
            if (local.room) {
                Cell.column -= 10;
                local.fix_x = 0;
                Cell.room = local.room;
                Handler();
            }
        }
    }

    if (Cell.room) {
        if (Cell.row < 0) {
            Cell.row += 3;
            local.room = Level.room_links[Cell.room - 1].above;
            Cell.room = local.room;
            Handler();
        } else if (Cell.row >= 3) {
            Cell.row -= 3;
            local.room = Level.room_links[Cell.room - 1].below;
            Cell.room = local.room;
            Handler();
        }
    }

    if (!local.room && Cell.room)
        Cell.room = local.room;
    if (Cell.column < 0 || Cell.column >= 10)
        Cell.column += local.fix_x;
    return Cell.room;
}

char far pascal ReadCell(unsigned char room, signed char column, signed char row)
{
    Cell.room = room;
    Cell.column = column;
    Cell.row = row;
    Cell.room = Handler();
    Cell.index = FirstInRow(Cell.row) + Cell.column;
    if (Cell.room) {
        select_room_arrays(Cell.room);
        Cell.kind = BlueT[Cell.index];
        Cell.modifier = BlueS[Cell.index][0];
    } else {
        Cell.modifier = 0;
        Cell.kind = GetUndefineCellId(column, row);
    }
    return Cell.kind;
}

char far pascal GetUndefineCellId(signed char x, signed char y)
{
    char result;
    switch (Level.environment_kind) {
    case 2:
        if ((Room.number == 4 && (Level.level_number) == 13) ||
            ((Level.level_number) == 13 && Room.number == 13))
            result = 0;
        else
            result = 20;
        break;
    case 4:
        if (!Room.above && y == -1) {
            result = 0;
            break;
        }
        if ((Room.number == 27 && (Level.level_number) == 6) ||
            (Room.number == 16 && (Level.level_number) == 9)) {
            result = 0;
            break;
        }
        result = 20;
        break;
    case 5:
        result = 0;
        break;
    case 6:
        if (((Room.number == 7 || Room.number == 8) && Level.environment_kind == 6) ||
            (Room.number == 3 && Level.environment_kind == 6 && (x < 0 || x == 9) && y == 2))
            result = 20;
        else
            result = 0;
        break;
    default:
        result = 20;
        break;
    }
    return result;
}

signed char far pascal index_cell(signed char column, signed char row)
{
    signed char index;

    if (row < 3 && row >= -1 && column < 10 && column >= 0) {
        if (row == -1)
            index = -1 - column;
        else
            index = FirstInRow(row) + column;
    } else {
        index = 30;
    }
    return index;
}

signed char far pascal IndexCell1(signed char column, signed char row)
{
    signed char index = index_cell(column, row);
    signed char result;

    if (index >= 0)
        result = index;
    else
        result = 30;
    return result;
}

extern FRAMEREC Frame;
extern FRAMEREC far frame_table[];
extern CHARREC Kid;
extern CHARREC RoomOpp[5];
extern CHARREC Opp;
extern CHARREC Char;

extern unsigned int opponent_frames;
extern void far *far pascal RESOLVE_HANDLE_TO_FAR_PAYLOAD(unsigned int handle);

int far pascal get_base_x(void);
char far pascal get_char_col(int x);
char far pascal get_col(int x);
int far pascal AddCharX(int value);
void far pascal get_frame_info(void);
void far pascal reread_base_cell(void);

void far pascal reread_cells(void)
{
    get_frame_info();
    reread_base_cell();
}

void far pascal reread_base_cell(void)
{
    Char.column = get_char_col(get_base_x());
}

void far pascal get_frame_info(void)
{
    unsigned int adjust = 0;
    FRAMEREC far *frames = RESOLVE_HANDLE_TO_FAR_PAYLOAD(opponent_frames);

    switch (Char.character_kind) {
    case 0:
    case 1:
    case 6:
        Frame = frame_table[Char.frame];
        break;
    case 2:
    case 4:
    case 10:
    case 12:
        if (Char.frame >= 102 && Char.frame < 107)
            adjust = 70;
        Frame = frames[Char.frame + adjust - 149];
        break;
    case 8:
        if (Char.frame < 183)
            adjust = 43;
        /* Fall through. */
    case 7:
    case 11:
        Frame = frames[Char.frame + adjust - 149];
        break;
    }
}

#define char_direction ((unsigned char)Char.direction)
#define char_x (Char.x)
#define character_row (Char.row)
#define character_room (Char.room)

int far pascal AddCharX(int value)
{
    if (char_direction)
        value = -value;
    return value + char_x;
}

/* Semantic names are hypotheses only; direct addresses are established separately. */
extern FCHARREC FChar;

int far pascal AddFCharX(int value)
{
    if (FChar.flipped)
        value = -value;
    FChar.x = FChar.x + value;
    return FChar.x;
}

extern int ShakeFloor;
extern int WeightlessTimer;

extern KIDINFO KidInfo;
extern int NextLevel;
static int animation_portion;
static int sound_steps;
static FCHARREC saved_fchar;
extern int AlertGuard;

typedef int far * near * SequenceHandle;

int far _fastcall GetSeqOp(SequenceHandle sequence);
unsigned int far _fastcall find_char_sequence(unsigned int id);
void far pascal anim_char_tap(int sound);
int far pascal reset_char_y(void);
void far pascal SetKidDeathMethod(unsigned char method);
void far pascal PotionEffect(void);
void far pascal remove_char(void);

extern unsigned int far pascal SAVE_HANDLE_FLAGS(SequenceHandle);
extern void far pascal FETCH_LOCKED_SHAPE_LIST(SequenceHandle);
extern void far pascal RESTORE_HANDLE_FLAGS(SequenceHandle, unsigned int);
extern unsigned int far pascal HGETRSRC(unsigned long type, unsigned int id);
extern void far pascal fill_palette_with_color(int cue);
extern void far pascal restore_game_play_palette(void);
extern int far pascal Rnd(int condition);
extern void far pascal JumpSeq(int event);
extern void far pascal TriggerJinnee(void);
extern void far pascal AddSong(int sound);
extern void far pascal AddSound(int sound);
extern int far pascal IsExitChasingSkel(unsigned char room);

/* The animation dispatcher and tap are placement drafts. */
void far pascal AnimChar(void)
{
    int previous_position = Char.sequence_position;
    int reading = 1;
    SequenceHandle sequence = (SequenceHandle)find_char_sequence(Char.sequence_id);
    int opcode;
    int first, second;
    unsigned int status;

    if (!sequence)
        return;

    status = SAVE_HANDLE_FLAGS(sequence);
    FETCH_LOCKED_SHAPE_LIST(sequence);

    do {
        opcode = GetSeqOp(sequence);
        switch (opcode) {
        case -24:
            if (GetSeqOp(sequence))
                fill_palette_with_color(GetSeqOp(sequence));
            else
                restore_game_play_palette();
            break;
        case -23:
            Char.sequence_position = previous_position - 1 >= 0 ? previous_position - 1 : 0;
            break;
        case -22:
            opcode = GetSeqOp(sequence);
            first = GetSeqOp(sequence);
            second = GetSeqOp(sequence);
            if (Rnd(opcode))
                JumpSeq(first);
            else
                JumpSeq(second);
            break;
        case -21:
            Char.sequence_group = GetSeqOp(sequence);
            break;
        case -19:
            reset_char_y();
            break;
        case -18:
            Char.sequence = 0;
            break;
        case -17:
            remove_char();
            reading = 0;
            break;
        case -16:
            ++NextLevel;
            if ((Level.level_number) == 6)
                KidInfo.entrance_mode = Char.column >= 6 ? 2 : 1;
            break;
        case -15:
            anim_char_tap(GetSeqOp(sequence));
            break;
        case -14:
            opcode = GetSeqOp(sequence);
            if (opcode == 1 || (opcode == 2 && animation_portion == 6)) {
                PotionEffect();
                if (!Char.sequence_position) {
                    RESTORE_HANDLE_FLAGS(sequence, status);
                    sequence = (SequenceHandle)find_char_sequence(Char.sequence_id);
                    status = SAVE_HANDLE_FLAGS(sequence);
                    FETCH_LOCKED_SHAPE_LIST(sequence);
                    break;
                }
            } else if (opcode == 3) {
                TriggerJinnee();
            }
            break;
        case -13:
            ShakeFloor = -1;
            break;
        case -12:
            ShakeFloor = 1;
            break;
        case -11:
            SetKidDeathMethod(GetSeqOp(sequence));
            break;
        case -9:
            opcode = GetSeqOp(sequence);
            first = GetSeqOp(sequence);
            if (opcode && Char.horizontal_step < 16 &&
                (!Char.vertical_step || Char.vertical_step < 32)) {
                Char.horizontal_step = Char.horizontal_step + opcode > 16
                    ? 16 : Char.horizontal_step + opcode;
            }
            if (first && Char.vertical_step < 32) {
                Char.vertical_step = Char.vertical_step + first > 32
                    ? 32 : Char.vertical_step + first;
            }
            break;
        case -8:
            opcode = GetSeqOp(sequence);
            if (opcode != 10000)
                Char.horizontal_step = opcode;
            opcode = GetSeqOp(sequence);
            if (opcode != 10000)
                Char.vertical_step = opcode;
            break;
        case -7: Char.action = GetSeqOp(sequence); break;
        case -6: Char.y += GetSeqOp(sequence); break;
        case -5: char_x = AddCharX(GetSeqOp(sequence)); break;
        case -4: ++character_row; break;
        case -3: --character_row; break;
        case -2:
            if (Char.character_kind == 10)
                char_x = AddCharX(25);
            Char.direction = ~Char.direction;
            break;
        case -10:
            if (!WeightlessTimer) {
                GetSeqOp(sequence);
                break;
            }
            /* fall through */
        case -1:
            opcode = GetSeqOp(sequence);
            if (character_row != 0 && (Level.level_number) == 5 &&
                (character_room == 10 || character_room == 7 || character_room == 12) &&
                opcode == 57 && Char.sequence != 60 && Char.column == 9)
                opcode = 86;
            Char.sequence_id = opcode;
            Char.sequence_position = 0;
            RESTORE_HANDLE_FLAGS(sequence, status);
            sequence = (SequenceHandle)find_char_sequence(opcode);
            if (!sequence)
                break;
            status = SAVE_HANDLE_FLAGS(sequence);
            FETCH_LOCKED_SHAPE_LIST(sequence);
            break;
        default:
            Char.frame = opcode;
            reading = 0;
            break;
        }
    } while (reading);

    RESTORE_HANDLE_FLAGS(sequence, status);
}

int far _fastcall GetSeqOp(SequenceHandle sequence)
{
    int opcode;
    int far *entries;

    if (sequence) {
        entries = RESOLVE_HANDLE_TO_FAR_PAYLOAD(sequence);
        opcode = entries[Char.sequence_position]; ++Char.sequence_position;
    }
    return opcode;
}

unsigned int far _fastcall find_char_sequence(unsigned int id)
{
    return HGETRSRC(0x53455153UL, id);
}

void far pascal anim_char_tap(int sound)
{
    switch (sound) {
    case 1:
        ++sound_steps;
        if (Level.environment_kind != 1) {
            AddSound(sound_steps % 2 + 23);
            if (Char.character_kind == 4 && IsExitChasingSkel(character_room))
                char_x = AddCharX(Char.slot - 1);
        }
        /* Fall through. */
    case 0:
        if (!Char.character_kind)
            AlertGuard = 1;
        break;
    default:
        if (sound <= 273) {
            if (sound == 271)
                AddSong(sound);
            else
                AddSound(sound);
        }
        break;
    }
}

void far pascal SetKidDeathMethod(unsigned char value)
{
    if (Char.character_kind == 0 || Char.character_kind == 1)
        KidInfo.argument = value;
}

int far pascal reset_char_y(void)
{
    Char.y = (signed char)63 * character_row + 56;
    if (Char.character_kind == 7 || Char.character_kind == 8)
        Char.y -= 19;
    return Char.y;
}

char far FirstInRow(signed char value)
{
    char result;

    if (value >= 0)
        result = value * 10;
    else
        result = value * 10 + 9;

    return result;
}

void copy_kid_to_char(void);
void far pascal save_actor_slot(void);
void far pascal load_actor_slot(signed char slot);

void far pascal load_kid_with_opponent(signed char slot)
{
    if (slot >= 0 && slot < 5) {
        copy_kid_to_char();
        Opp = RoomOpp[slot];
    }
}

void far pascal save_kid_with_opponent(void)
{
    if (Opp.slot >= 0 && Opp.slot < 5) {
        Kid = Char;
        RoomOpp[Opp.slot] = Opp;
    }
}

void far pascal load_shadow_with_opponent(signed char slot)
{
    load_actor_slot(slot);
    Opp = Kid;
}

void far pascal save_shadow_with_opponent(void)
{
    if (Char.slot >= 0 && Char.slot < 5)
        save_actor_slot();
    Kid = Opp;
}

/* init_fchar_rect (descriptive name; FM Towns inlines it in SetupCarpet, the Mac body is unlabelled):
   copy the play-area rectangle into the 8-byte rectangle stored at byte 9 of the FChar record.
   GamePlayRect (DS:09fe) is the FM Towns global name anchored by matching copies. */

extern Rect GamePlayRect;

void init_fchar_rect(void)
{
    FChar.clip = GamePlayRect;
}

/* Semantic name hypothesis only; original symbol remains unrecovered. */
void far pascal ConvertX(int value, int near *value_out) {
    if (value < 0)
        --value;
    *value_out = value;
}

/* Names and qualifiers are reconstruction hypotheses. */

extern int WeightlessTimer;

void far pascal Gravity(void)
{
    if (Char.action == 4 || Char.action == 9) {
        if (WeightlessTimer) {
            ++Char.vertical_step;
            if (Char.vertical_step > 4)
                Char.vertical_step = 4;
        } else {
            Char.vertical_step += 3;
            if (Char.vertical_step > 33)
                Char.vertical_step = 33;
        }
    }
}

void far pascal add_char_velocity(void)
{
    if (Char.y < 1920) {
        Char.y += Char.vertical_step;
        if (Char.action == 4 || Char.character_kind == 7 || Char.character_kind == 8) {
            char_x = AddCharX(Char.horizontal_step);
            reread_cells();
        }
    }
}

/* Names, signedness and qualifiers are reconstruction hypotheses. */

void far pascal ClearCharVel(void)
{
    signed char zero = 0;
    Char.horizontal_step = zero;
    Char.vertical_step = zero;
}

int far pascal decrement_life_points(signed char amount)
{
    int lethal = 0;

    if (!(Char.character_kind == 4 && character_row != 0 && (Level.level_number) == 5 &&
          (character_room == 10 || character_room == 7 || character_room == 12))) {
        if (amount >= Char.life_points) {
            if (-Char.life_points < Char.life_delta)
                Char.life_delta = -Char.life_points;
            lethal = 1;
        } else if (-Char.life_delta < amount) {
            Char.life_delta = -amount;
        } else {
            lethal = Char.life_points + Char.life_delta == 0;
        }
    }
    return lethal;
}

char far pascal get_under_foot(void)
{
    return ReadCell(character_room, Char.column, character_row);
}

extern int WeightlessTimer;

extern signed char Plus1[2];
extern signed char Minus1[2];

struct CopyRecord {
    char bytes[5];
};

extern int DroppedOut;
extern unsigned int Stunned;
extern unsigned int Screamed;
extern void far pascal update_shadow_gameplay_palette(void);
extern int far pascal IsWall(char kind);
extern int far pascal IS_SIX_KIND(char kind);
int ImHeight, ImWidth, CDLeftEdge, CDRightEdge, LeftEdge, RightEdge, TopEdge, ExitEdge;
struct CopyRecord Dir, SelDir;
extern int far pascal tile_is_ledge(char kind);
extern int far pascal RoofClimbingIntoShip(void);
extern void far pascal trigger_all_stones_down(void);
extern int far pascal grab_roof_ship(void);
extern int far pascal probe_chain_floor(int index);
extern unsigned char far pascal pick_hidden_actor(unsigned char direction);
extern int far pascal query_sound(int mask);
extern void far pascal CutToKidRoom(void);

void far pascal start_fall(void);
char far pascal inside_cell(void);
int far pascal get_edge_distance(void);
int far pascal get_edge_distance_at(int x);
char far pascal get_cells_ahead(signed char distance);
char far pascal get_behind(void);
char far pascal get_above(void);
char far pascal get_above_front(void);
char far pascal get_above_behind(void);
int far pascal check_ledge(char above, char ahead, unsigned int above_modifier,
                         unsigned int ahead_modifier);
int far pascal GrabTest(void);

void far pascal on_ground(void)
{
    char kind;

    if ((Frame.flags & 0x40) && Char.character_kind != 10) {
        kind = get_under_foot();
        if (IsWall(kind)) {
            if (kind != 7 || ((Char.frame < 246 || Char.frame > 261) &&
                             Char.character_kind != 11))
                kind = inside_cell();
        }
        if (IS_SIX_KIND(kind) ||
            (kind == 30 && (Cell.modifier & 15) < 2)) {
            if (Level.environment_kind == 5 && RoofClimbingIntoShip())
                return;
            if (Level.environment_kind == 2 && probe_chain_floor(16))
                return;
            if (Char.character_kind == 11)
                char_x = AddCharX(get_edge_distance() - 42);
            else
                start_fall();
        }
    }
}

void far pascal start_fall(void)
{
    unsigned int old_frame = Char.frame;
    int adjust_clearance = 0;
    int special_sequence = -1;
    int sequence;
    int distance;
    struct { int y; } minimum;

    ++character_row;
    if (Char.character_kind == 1)
        update_shadow_gameplay_palette();
    if (Char.stance == 1)
        Char.stance = 0;
    if (Char.character_kind == 0 && (Level.level_number) == 5 && character_room == 10 &&
        ((char_direction == 0 && Dir.bytes[0]) ||
         (char_direction == 0xff && Dir.bytes[1]))) {
        special_sequence = 198;
        adjust_clearance = 1;
    }

    if (special_sequence != -1) {
        sequence = special_sequence;
    } else {
        switch (old_frame) {
        case 9:
            sequence = 7;
            break;
        case 13:
            sequence = 19;
            break;
        case 26:
            sequence = 18;
            adjust_clearance = 1;
            break;
        case 44:
            sequence = 21;
            adjust_clearance = 1;
            break;
        case 81:
            sequence = 7;
            Char.y += 12;
            break;
        default:
            adjust_clearance = 1;
            if (old_frame >= 81 && old_frame < 86) {
                sequence = 19;
            } else if (old_frame >= 150U && old_frame < 180U) {
                if (!Char.character_kind) {
                    DroppedOut = adjust_clearance;
                    if (old_frame == 153 || Char.sequence == 56)
                        sequence = 95;
                    else
                        sequence = 81;
                } else {
                    DroppedOut = 0;
                    if (Char.sequence == 86 || Char.sequence == 67 ||
                        Char.sequence == 108)
                        sequence = 83;
                    else
                        sequence = 82;
                }
            } else if (Char.character_kind == 2 && Char.sequence == 100) {
                sequence = 186;
            } else {
                sequence = 7;
            }
            break;
        }
    }

    if (adjust_clearance) {
        if (IsWall(get_cells_ahead(1)) ||
            IsWall(get_behind()) ||
            !IS_SIX_KIND(get_above()) || !IS_SIX_KIND(get_above_front()) ||
            !IS_SIX_KIND(get_above_behind())) {
            distance = get_edge_distance();
            if (distance <= 7)
                char_x = AddCharX(distance - 11);
            else if (distance >= 25)
                char_x = AddCharX(distance - 21);
            if (!IS_SIX_KIND(get_above()) || !IS_SIX_KIND(get_above_front())) {
                minimum.y = (signed char)63 * character_row + 1;
                if (minimum.y > Char.y)
                    Char.y = minimum.y;
            }
        }
    }

    if (Level.environment_kind == 1) {
        sequence = 27;
        trigger_all_stones_down();
        SetKidDeathMethod(8);
    }
    else if (Char.character_kind == 0 &&
             WeightlessTimer && Char.sequence != 228)
    {
        sequence = 228;
    }
    JumpSeq(sequence);
    AnimChar();
    reread_cells();

    if (IsWall(get_under_foot())) {
        inside_cell();
        return;
    }
    if (IsWall(get_cells_ahead(1))) {
        if (old_frame == 44 && get_edge_distance() < 14) {
            JumpSeq(114);
            AnimChar();
        } else {
            char_x = AddCharX(-2);
        }
        reread_cells();
    }
}

void far pascal finish_fall_in_undefined_room(void);

void far pascal FallingInUndefinedRoom(void)
{
    Gravity();
    add_char_velocity();
    if (Level.environment_kind == 5) {
        if (Char.y > 384) {
            if (Room.number != 19 && Room.number != 16) {
                AddSound(0);
                decrement_life_points(100);
                Char.frame = 185;
                SetKidDeathMethod(3);
            }
            Char.y = 384;
            Char.vertical_step = 0;
            Char.action = 1;
            if (Char.character_kind && Kid.opponent_index == Char.slot)
                Kid.opponent_index = pick_hidden_actor(char_direction);
        } else if (Char.character_kind == 2 && Char.frame != 185 &&
                   !query_sound(0x2729)) {
            AddSound(25);
        }
    } else if (Char.y > 384) {
        finish_fall_in_undefined_room();
    }
}

void far pascal finish_fall_in_undefined_room(void)
{
    decrement_life_points(100);
    Char.frame = 185;
    SetKidDeathMethod(3);
    Char.y = 384;
    Char.vertical_step = 0;
    Char.action = 1;
    if (Char.character_kind == 1)
        CutToKidRoom();
}

int far pascal is_undefined_room(void)
{
    if (character_room == 0 ||
        (((character_room == 3 && Level.environment_kind == 6) ||
          (character_room == 5 && Level.environment_kind == 6)) &&
         character_row >= 11))
        return 1;
    return 0;
}

void far pascal FallOn(void)
{
    int position;
    if (Dir.bytes[4] && Char.vertical_step < 32 && Char.alive < 0 &&
        (signed char)63 * character_row + 56 <= Char.y + 25) {
        position = char_x;
        char_x = AddCharX(-16);
        reread_cells();
        if (!GrabTest()) {
            char_x = position;
            return;
        }
        position = get_edge_distance();
        position += 4;
        if (char_direction == 0xff)
            position -= 7;
        else
            position -= 9;
        char_x = AddCharX(position);
        reset_char_y();
        Char.vertical_step = 0;
        JumpSeq(15);
        AnimChar();
        AddSound(11);
        Stunned = 12;
        Screamed = 0;
    }
}

int far pascal GrabTest(void)
{
    char above = get_above();
    unsigned int modifier = Cell.modifier;
    char ahead = get_above_front();
    int result = check_ledge(above, ahead, modifier, Cell.modifier);

    if (Level.environment_kind == 5 && !result)
        return grab_roof_ship();
    return result;
}

char far pascal inside_cell(void)
{
    int distance;
    char ahead;

    if (Char.sequence_group != 4 && Char.character_kind != 11 &&
        !(character_room == 3 && Level.environment_kind == 6 && Char.column < 0)) {
        distance = get_edge_distance();
        ahead = get_cells_ahead(1);
        if (distance < 11 && !IsWall(ahead))
            distance += 11;
        else
            distance -= 32;
        char_x = AddCharX(distance);
        reread_cells();
    }
    return get_under_foot();
}

char far pascal get_cells_ahead(signed char distance)
{
    return ReadCell(character_room,
                    Char.column + Plus1[Char.direction + 1] * distance,
                    character_row);
}

char far pascal get_behind(void)
{
    return ReadCell(character_room, Char.column + Minus1[Char.direction + 1],
                    character_row);
}

extern signed char Offset;

int far pascal get_base_x(void)
{
    return AddCharX((signed char)((Frame.x) - (Frame.flags & 31)));
}

int far pascal get_edge_distance(void)
{
    return get_edge_distance_at(get_base_x());
}

int far pascal get_edge_distance_at(int x)
{
    int distance;
    get_char_col(x);
    if (!char_direction)
        distance = 31 - Offset;
    else
        distance = Offset;
    return distance;
}

char far pascal get_char_col(int x)
{
    return get_col(x - 14);
}

char far pascal get_col(int x)
{
    int shifted = x - 4;
    char col;

    Offset = shifted % 32;
    col = shifted / 32 - 4;
    if (x < 0)
        --col;
    return col;
}

char far pascal GetRow(int value)
{
    int shifted = value - 3;
    char result = shifted / 63;

    if (shifted <= 0)
        --result;

    return result;
}

char far pascal GetCellCol(void)
{
    signed char col = Cell.column;

    if (Room.number != Cell.room && Room.below != Cell.room && Room.above != Cell.room) {
        if (Room.left == Cell.room || Room.below_left == Cell.room ||
            Room.above_left == Cell.room)
            col -= 10;
        else if (Room.right == Cell.room || Room.below_right == Cell.room ||
                 Room.above_right == Cell.room)
            col += 10;
    }

    return col;
}

extern int far pascal PlayerCtrl(void);
extern void LoadSelect(void);
extern void SaveSelect(void);
extern void far pascal ClrJoystick(void);
extern void far pascal UserCtrl(void);
extern void far pascal FaceJoystick(void);
extern void far pascal InvertJoystick(void);
extern void far pascal dispatch_sound(int actor);
extern void far pascal finish_start_kid(void);
extern void far pascal AddKidDeathSong(unsigned char method);
extern void far pascal lose_game(void);
extern void far pascal draw_restart_message(void);
extern int far pascal is_dead_pos(unsigned int frame);
extern int far pascal Input(void);
extern int far pascal CheckForEndOfRecording(void);
extern void far pascal add_opponent_death_song(void);
extern void far pascal update_automatic_actor(void);
extern void far pascal CheckRecordedDirForChar(void);
extern void far pascal apply_input(void);
extern void far pascal play_throne_room_nis(void);
extern unsigned int BackToLife;
extern unsigned int current_effect, current_song;
extern unsigned int TimeRequest, MsgTimer, MsgTimerStart;

extern GAMESTATE GameState;
extern unsigned int Minutes, Invert;
extern signed char LevelIdx;

JoyState Joy;

int far pascal ctrl_player(void)
{
    int result;
    result = PlayerCtrl();
    if (result != -2 || Char.alive < 0 || !is_dead_pos(Char.frame))
        return result;

    if (BackToLife) {
        dispatch_sound(0);
        Kid.life_delta = Kid.life_initial;
        copy_kid_to_char();
        JumpSeq(2);
        char_x += 16;
        AnimChar();
        reread_cells();
        finish_start_kid();
    }

    if (Char.character_kind != 0 && Char.character_kind != 1)
        return result;
    if (query_sound(current_effect + 10000) &&
        current_effect != 4 && current_effect != 7 &&
        current_effect != 54)
        return result;

    TimeRequest = 0;
    if (Char.alive < 0 || Char.alive >= 6) {
        if (Char.alive == 6) {
            AddKidDeathSong(KidInfo.argument);
        } else {
            if (Char.alive != 7 || query_sound(current_effect + 10000))
                return result;
            if (query_sound(current_song + 10000))
                return result;
            if (!Minutes) {
                lose_game();
                result = -1;
            } else if (MsgTimerStart != 600) {
                MsgTimer = MsgTimerStart = 600;
                draw_restart_message();
            }
        }
    }
    ++Char.alive;
    return result;
}

int far pascal PlayerCtrl(void)
{
    int result;
    if (Char.alive < 0 && !Char.life_points)
        Char.alive = 0;
    if (Stunned)
        --Stunned;
    LoadSelect();
    Input();
    if (LevelIdx != NextLevel || !character_room || Char.sequence == 59)
        Joy.y = Joy.x = 0;
    ClrJoystick();
    UserCtrl();
    result = CheckForEndOfRecording();
    SaveSelect();
    return result;
}

void far pascal shad_ctrl(void)
{
    if (Char.alive >= 0) {
        if (Char.alive >= 6) {
            add_opponent_death_song();
            if (Kid.opponent_index == Char.slot)
                Kid.opponent_index = -1;
        } else if (Char.repeat) {
            if (Char.alive < 20)
                ++Char.alive;
            else
                Char.repeat = 0;
        }
    } else if (!Char.life_points) {
        Char.alive = 0;
    }
    update_automatic_actor();
    if (GameState.recording_mode)
        CheckRecordedDirForChar();
    if (Char.character_kind != 1 && Char.character_kind != 12)
        apply_input();
}

void far pascal UserCtrl(void)
{
    unsigned char direction = char_direction;
    if (!direction)
        FaceJoystick();
    if (Invert)
        InvertJoystick();
    if (GameState.recording_mode)
        CheckRecordedDirForChar();
    apply_input();
    if (!direction)
        FaceJoystick();
    if (Invert)
        InvertJoystick();
    if (Char.sequence == 237 && Char.frame == 185 && character_room == 9 &&
        Level.level_number == 8)
        play_throne_room_nis();
}

void far pascal FaceJoystick(void)
{
    char temporary;
    Joy.x = -Joy.x;
    temporary = Dir.bytes[0];
    Dir.bytes[0] = Dir.bytes[1];
    Dir.bytes[1] = temporary;
}

void far pascal InvertJoystick(void)
{
    char temporary;
    Joy.y = -Joy.y;
    temporary = Dir.bytes[2];
    Dir.bytes[2] = Dir.bytes[3];
    Dir.bytes[3] = temporary;
}

/* Neutral DS-relative byte aliases; original names and qualifiers are unknown. */

unsigned char far pascal ClrAll(void)
{
    char zero = 0;
    Dir.bytes[3] = zero;
    Dir.bytes[2] = zero;
    Dir.bytes[0] = zero;
    Dir.bytes[1] = zero;
    return 1;
}

/* SaveSelect: remember the current selection (FM Towns CTRL.C public; anchored by PlayerCtrl
   call sites in both builds). Dir/SelDir are the FM Towns globals at DOS DS:628c/DS:626a. */

void SaveSelect(void)
{
    SelDir = Dir;
}

/* LoadSelect: restore the remembered selection before input polling (FM Towns CTRL.C). */

void LoadSelect(void)
{
    Dir = SelDir;
}

void *memset(void *, int, unsigned);
#pragma intrinsic(memset)
void InitInput(void) { memset(SelDir.bytes, 0, sizeof(SelDir.bytes)); }

void far pascal ClrJoystick(void)
{
    if (Dir.bytes[0] >= 0) {
        if (Joy.x == -1) {
            if (!Dir.bytes[0])
                Dir.bytes[0] = -1;
        } else
            Dir.bytes[0] = 0;
    }
    if (Dir.bytes[1] >= 0) {
        if (Joy.x == 1) {
            if (!Dir.bytes[1])
                Dir.bytes[1] = -1;
        } else
            Dir.bytes[1] = 0;
    }
    if (Dir.bytes[2] >= 0) {
        if (Joy.y == -1) {
            if (!Dir.bytes[2])
                Dir.bytes[2] = -1;
        } else
            Dir.bytes[2] = 0;
    }
    if (Dir.bytes[3] >= 0) {
        if (Joy.y == 1) {
            if (!Dir.bytes[3])
                Dir.bytes[3] = -1;
        } else
            Dir.bytes[3] = 0;
    }
    if (Joy.button == -1) {
        if (Dir.bytes[4] != 1)
            Dir.bytes[4] = -1;
    } else if (Joy.button == -2) {
        if (Dir.bytes[4] != 2)
            Dir.bytes[4] = -2;
    } else
        Dir.bytes[4] = 0;
}

extern int far pascal is_open_tile(char kind);

int far pascal check_ledge(char above, char ahead, unsigned int above_modifier,
                           unsigned int ahead_modifier)
{
    int result;
    if (!IS_SIX_KIND(above) || (Level.environment_kind == 2 && (above_modifier & 0x80)) ||
        ((ahead == 11 || ahead == 15) && (ahead_modifier & 15)) ||
        !is_open_tile(ahead) ||
        (ahead == 4 && !(ahead_modifier & 255) && char_direction == 255) ||
        (Level.environment_kind == 5 && character_room == 15 && character_row != 0))
        result = 0;
    else
        result = 1;
    return result;
}

char far pascal get_above(void)
{
    return ReadCell(character_room, Char.column, character_row - 1);
}

char far pascal get_above_behind(void)
{
    return ReadCell(character_room, Char.column + Minus1[Char.direction + 1],
                    character_row - 1);
}

char far pascal get_above_front(void)
{
    return ReadCell(character_room, Char.column + Plus1[Char.direction + 1],
                    character_row - 1);
}

/* Semantic name only; this external names the independently witnessed DS byte. */
/* FaceDx uses the active actor's direction. */

int far pascal FaceDx(int value)
{
    if (!char_direction)
        value = -value;
    return value;
}

extern int custom_room;
extern void far pascal mark_cell_redraw(void);
void far pascal RemoveObj(int portion)
{
    unsigned int modifier;
    int index;
    animation_portion = portion;
    Dir.bytes[4] = 1;
    if (Level.environment_kind == 3)
        modifier = Cell.modifier & 0x3f00;
    else if (Level.environment_kind == 4)
        modifier = Cell.modifier & 0x0f00;
    else
        modifier = 0;
    if (custom_room)
        modifier |= 0xc000;
    index = (unsigned char)30 * Cell.room + Cell.index;
    (&Level.cell_kinds[0][0])[index - 30] = 1;
    (&Level.cell_info[0][0])[index - 30].modifier = modifier;
    mark_cell_redraw();
}

extern Rect NullRect;
extern Rect near *clip_char_rects[5];
extern int far pascal ComputeOffsetCellRect(int near *bounds, Rect near *table, int column, int row);
extern void far pascal clip_character_at_water(void);
extern void far pascal clip_desert_character(void);
extern void far pascal clip_ruins_actor(void);
extern void far pascal clip_final_character(void);
extern int far _fastcall IsRoundCaveBlock(unsigned int modifier);
void far pascal MakeFCharClipRect(void);
int far pascal is_char_non_viewable(void);

void far pascal ClipChar(void)
{
    int clearance = 1;
    unsigned int frame = Char.frame;
    int action = Char.action;
    signed char row = character_row;
    int flying;
    char tile;
    struct { int top; int bounds[4]; } workspace;

    init_fchar_rect();
    MakeFCharClipRect();
    if (is_char_non_viewable()) {
        FChar.clip = NullRect;
        return;
    }
    if (Char.character_kind == 0 &&
        Char.frame >= 217 && Char.frame <= 226 &&
        (signed char)63 * character_row + 41 > Char.y) {
        tile = ReadCell(character_room, Char.column, character_row);
        if (ComputeOffsetCellRect(workspace.bounds, clip_char_rects[Level.environment_kind], (tile == 16) + Char.column, character_row)) {
            FChar.clip.top = workspace.bounds[0];
            FChar.clip.right = 320;
        }
        return;
    }
    if (Char.action == 9) {
        if (Level.environment_kind == 5)
            clip_character_at_water();
        return;
    }

    flying = Char.character_kind == 7 || Char.character_kind == 8;
    tile = ReadCell(character_room, Cell.first_col, (Cell.top_row));
    if (IS_SIX_KIND(tile) ||
        (Level.environment_kind == 3 && tile == 20 && IsRoundCaveBlock(Cell.modifier)) ||
        (Cell.room == 1 && (Level.level_number) == 14) ||
        ((Cell.room == 7 || Cell.room == 8) && Level.environment_kind == 6 && Char.sequence != 14)) {
        clearance = 0;
    } else if (action != 0 || (frame != 79 && frame != 81)) {
        tile = ReadCell(character_room, Cell.last_col, Cell.top_row);
        if (IS_SIX_KIND(tile) || (tile == 20 && Level.environment_kind == 3 && IsRoundCaveBlock(Cell.modifier)))
            clearance = 0;
    }

    if (clearance && Level.environment_kind != 5 && Level.environment_kind != 1 &&
        !(character_room == 16 && (Level.level_number) == 9) &&
        !(character_room == 27 && (Level.level_number) == 6) &&
        !(character_room == 3 && Level.environment_kind == 6) &&
        !(character_room == 6 && Level.environment_kind == 6)) {
        if ((signed char)((tile = row + 1) - 1)) {
            int height = (signed char)63 * tile - 60;
            clearance = height >= FChar.y ? 0 : height - 15 >= TopEdge ? 0 : clearance;
        }
        if (clearance && !flying) {
            if ((workspace.top = (signed char)63 * tile - 60) > FChar.clip.top)
                FChar.clip.top = TopEdge = workspace.top;
        }
    }
    switch (Level.environment_kind) {
    case 1: clip_desert_character(); break;
    case 4: clip_ruins_actor(); break;
    case 5: clip_character_at_water(); break;
    case 6: clip_final_character(); break;
    }
}

extern unsigned int Pics[];
extern void far *far pascal get_set_image_size(unsigned int, int, int far *);

void far pascal MakeFCharClipRect(void)
{
    int size[2];
    if (FChar.image != -1 && get_set_image_size(Pics[FChar.image_set], FChar.image, size)) {
        FChar.clip.top = FChar.y - size[0];
        FChar.clip.bottom = FChar.y + 1;
        if (!FChar.flipped) {
            FChar.clip.right = FChar.x;
            FChar.clip.left = FChar.x - size[1];
        } else {
            FChar.clip.left = FChar.x;
            FChar.clip.right = FChar.x + size[1];
        }
        return;
    }
    FChar.clip = NullRect;
}

void far pascal uneven_floor(void)
{
    if (Char.character_kind != 11 && Char.character_kind != 7 && Char.character_kind != 8 && get_under_foot() == 6 &&
        (Frame.flags & 0x40))
        Char.y = (signed char)63 * character_row + 57;
}

void far pascal IndexChar(void)
{
    unsigned int frame = Char.frame;
    int action = Char.action;
    if (action == 1) {
        Cell.row = (Cell.bottom_row);
        Cell.column = Cell.first_col;
    } else {
        Cell.row = character_row;
        Cell.column = Char.column;
    }
    if ((frame >= 135 && frame < 149) || action == 2 || action == 3 || action == 4 || action == 6)
        Cell.column += Minus1[Char.direction + 1];
    FChar.index = IndexCell1(Cell.column, Cell.row);
}

extern void far pascal BoostMeter(void);
extern void far pascal GoWeightless(void);
extern void far pascal ToggleInvertState(void);
extern void far pascal transfer_jaffar_to_escher2(void);
void far pascal VanishChar(void);

void far pascal PotionEffect(void)
{
    if (!animation_portion)
        return;
    if (Char.character_kind != 0 &&
        (Char.character_kind != 1 || animation_portion != 1))
        return;
    if (animation_portion == -1) {
        Char.stance = 0;
    } else {
        switch (--animation_portion) {
        case 0:
            if (Char.life_initial != Char.life_points)
                Char.life_delta = 1;
            AddSong(101);
            break;
        case 1:
            BoostMeter();
            AddSong(101);
            break;
        case 2: GoWeightless(); break;
        case 3:
            ToggleInvertState();
            break;
        case 4:
            if (Invert) {
                ToggleInvertState();
                break;
            }
            if (decrement_life_points(1))
                JumpSeq(71);
            AddSound(14);
            break;
        case 5:
            AddSound(14);
            JumpSeq(111);
            break;
        }
    }
    animation_portion = 0;
}

int far pascal is_static_frame(void)
{
    int result;
    if (is_dead_pos(Char.frame) || Char.frame == 0 || Char.frame == 15 ||
        Char.frame == 109 || Char.frame == 166 || Char.frame == 171 || Char.frame == 229)
        result = 1;
    else
        result = 0;
    return result;
}

int far pascal is_char_non_viewable(void)
{
    int result;
    if (Char.sequence_group == 4 || Char.sequence_group == 5 || Char.sequence_group == 6 ||
        Char.sequence_group == 7 || Char.sequence_group == 10 || Char.sequence_group == 12 ||
        (Level.environment_kind == 5 && character_room != 19 && character_room != 16 &&
         character_room != 15 && is_dead_pos(Char.frame)))
        result = 1;
    else
        result = 0;
    return result;
}

int far pascal is_dead_pos(unsigned int frame)
{
    int dead;
    if (frame == 185)
        dead = 1;
    else {
        switch (Char.character_kind) {
        case 0:
            dead = frame == 242 || frame == 243 || frame == 185 || frame == 271 || frame == 266;
            if (!dead && (Level.level_number) == 5 && character_room == 3)
                dead = Char.alive >= 0 && Char.sequence_group == 10;
            break;
        case 2:
            if (!Level.preferred_opponent) {
                dead = frame == 228;
                break;
            }
            dead = frame == 245 || frame == 248 || frame == 251 || frame == 252 ||
                   frame == 253 || frame == 254 || frame == 255;
            break;
        case 4:
            dead = frame == 206 || frame == 207;
            break;
        case 10:
            dead = frame == 213;
            break;
        case 11:
            dead = frame == 273;
            break;
        default:
            dead = 0;
            break;
        }
    }
    return dead;
}

void far pascal remove_char(void)
{
    if (character_room == 6 && Level.environment_kind == 6 &&
        Char.sequence == 240 && Kid.alive < 0)
        transfer_jaffar_to_escher2();
    VanishChar();
    character_room = 0;
}

extern void far pascal DrawOppMeter(char slot, int life, int maximum);
void far pascal VanishChar(void)
{
    signed char zero = 0;
    DrawOppMeter(Char.slot, 0, Char.life_initial);
    char_direction = 86;
    Char.action = zero;
    Char.alive = zero;
    Char.life_points = zero;
    if (Char.slot != zero && Kid.opponent_index == Char.slot)
        Kid.opponent_index = pick_hidden_actor(1);
}

void SaveFChar(void) { saved_fchar = FChar; }
void RestoreFChar(void) { FChar = saved_fchar; }

extern int far pascal can_continue_run_jump(char cell, unsigned int modifier);
extern void far pascal engage_weapon(void);
void far pascal guard_ctrl(void)
{
    if (Char.frame == 166 && Dir.bytes[3] < 0) {
        if (Dir.bytes[0] < 0) {
            if (!can_continue_run_jump(get_behind(), Cell.modifier))
                char_x = AddCharX(5);
            engage_weapon();
        } else {
            Dir.bytes[3] = 1;
            JumpSeq(80);
        }
    }
}

#define opponent_direction (Opp.direction)
#define opponent_x (Opp.x)
#define opponent_row (Opp.row)
#define opponent_room (Opp.room)
#define opponent_life (Opp.life_points)
int far pascal get_opp_dist(void)
{
    int distance;
    if (opponent_life && opponent_room == character_room && opponent_row == character_row &&
        Opp.direction != 86) {
        distance = opponent_x - char_x;
        if (char_direction)
            distance = -distance;
        if (distance >= 0 && char_direction != (unsigned char)Opp.direction)
            distance += 13;
    } else {
        distance = 999;
    }
    return distance;
}

static struct slot_error_messages {
    char save[27];
    char load[27];
} slot_errors = {
    "Error in SaveShad(). Quit?",
    "Error in LoadShad(). Quit?"
};
extern int far pascal confirm_quit(char far *message);
extern void far pascal abort_with_message(int code, int first, int second);

void far pascal save_actor_slot(void)
{
    if (Char.slot >= 0 && Char.slot < 5) {
        RoomOpp[Char.slot] = Char;
        return;
    }
    if (confirm_quit((char far *)slot_errors.save))
        abort_with_message(1, 0, 0);
}

void far pascal load_actor_slot(signed char slot)
{
    if (slot >= 0 && slot < 5)
        Char = RoomOpp[slot];
    else if (confirm_quit((char far *)slot_errors.load))
        abort_with_message(1, 0, 0);
}

void copy_kid_to_char(void)
{
    Char = Kid;
}

/* The frame's final routines follow the Kid-to-Char copy. */
