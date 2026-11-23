/* CTRL.C reconstruction. See notes.md for claims and unresolved bodies.
 * Functions follow original order. */

extern unsigned char cell_room;
extern signed char cell_column;
extern signed char cell_row;
extern signed char cell_index;
extern unsigned int cell_modifier;
/* The kind byte immediately precedes the modifier word in the DOS cell result. */
#define cell_kind (((char near *)&cell_modifier)[-1])
extern unsigned char *room_kinds;
/* Each modifier entry occupies two words. ReadCell uses its first word. */
extern unsigned int (*BlueS)[2];

struct room_links {
    unsigned char left;
    unsigned char right;
    unsigned char above;
    unsigned char below;
};
extern struct room_links room_neighbors[];

extern void far pascal select_cell_tables(unsigned char room);
char far pascal FirstInRow(signed char value);
char far pascal GetUndefineCellId(signed char x, signed char y);

unsigned int far pascal Handler(void)
{
    struct {
        unsigned char room;
        signed char fix_x;
    } local;

    local.fix_x = 0;
    local.room = cell_room;

    if (cell_column < 0) {
        local.fix_x = 10;
        if (!local.room)
            goto check_row;
        local.room = room_neighbors[local.room].left;
        if (!local.room)
            goto check_row;
        cell_column += 10;
    } else if (cell_column >= 10) {
        local.fix_x = -10;
        if (!cell_room)
            goto check_row;
        local.room = room_neighbors[cell_room].right;
        if (!local.room)
            goto check_row;
        cell_column -= 10;
    } else {
        goto check_row;
    }

    local.fix_x = 0;
    cell_room = local.room;
    Handler();

check_row:
    if (cell_room) {
        if (cell_row < 0) {
            cell_row += 3;
            local.room = room_neighbors[cell_room].above;
        } else if (cell_row >= 3) {
            cell_row -= 3;
            local.room = room_neighbors[cell_room].below;
        } else {
            goto finish;
        }
        cell_room = local.room;
        Handler();
    }

finish:
    if (!local.room && cell_room)
        cell_room = local.room;
    if (cell_column < 0 || cell_column >= 10)
        cell_column += local.fix_x;
    return cell_room;
}

char far pascal ReadCell(unsigned char room, signed char column, signed char row)
{
    cell_room = room;
    cell_column = column;
    cell_row = row;
    cell_room = Handler();
    cell_index = FirstInRow(cell_row) + cell_column;
    if (cell_room) {
        select_cell_tables(cell_room);
        cell_kind = room_kinds[cell_index];
        cell_modifier = BlueS[cell_index][0];
    } else {
        cell_modifier = 0;
        cell_kind = GetUndefineCellId(column, row);
    }
    return cell_kind;
}

extern unsigned char STATE_MODE;
extern unsigned char STATE_LEVEL;
extern unsigned char viewed_room;
extern unsigned char STATE_FLAG;

char far pascal GetUndefineCellId(signed char x, signed char y)
{
    char result;
    switch (STATE_MODE) {
    case 2:
        if ((viewed_room == 4 && STATE_LEVEL == 13) ||
            (STATE_LEVEL == 13 && viewed_room == 13))
            result = 0;
        else
            result = 20;
        break;
    case 4:
        if (!STATE_FLAG && y == -1) {
            result = 0;
            break;
        }
        if (viewed_room == 27 && STATE_LEVEL == 6) {
            result = 0;
            break;
        }
        result = 20;
        break;
    case 5:
        result = 0;
        break;
    case 6:
        if (((viewed_room == 7 || viewed_room == 8) && STATE_MODE == 6) ||
            (viewed_room == 3 && STATE_MODE == 6 && (x < 0 || x == 9) && y == 2))
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
    /* Preserve the distinct byte stores for the exceptional and normal rows. */
    union {
        volatile signed char value;
        unsigned char alternate;
    } index;

    if (row < 3 && row >= -1 && column < 10 && column >= 0) {
        if (row == -1)
            index.value = -1 - column;
        else
            index.alternate = FirstInRow(row) + column;
    } else {
        index.value = 30;
    }
    return index.value;
}

signed char far pascal IndexCell1(signed char column, signed char row)
{
    signed char index = index_cell(column, row);
    /* The fallback has its own byte store in the original. */
    union {
        signed char value;
        volatile unsigned char fallback;
    } result;

    if (index >= 0)
        result.value = index;
    else
        result.fallback = 30;
    return result.value;
}


struct frame_info {
    unsigned char fields[7];
};

extern struct frame_info current_frame;
extern struct frame_info far default_frames[];
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
extern CHARREC Char;
extern CHARREC Opp;
extern CHARREC Kid;

extern unsigned int frame_resource;
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
    (*(unsigned char near *)&Char.column) = get_char_col(get_base_x());
}

void far pascal get_frame_info(void)
{
    unsigned int adjust = 0;
    struct frame_info far *frames = RESOLVE_HANDLE_TO_FAR_PAYLOAD(frame_resource);

    switch ((signed char)(*(volatile unsigned char near *)&Char.character_kind)) {
    case 0:
    case 1:
    case 6:
        current_frame = default_frames[Char.frame];
        break;
    case 2:
    case 4:
    case 10:
    case 12:
        if (Char.frame >= 102 && Char.frame < 107)
            adjust = 70;
        current_frame = frames[Char.frame + adjust - 149];
        break;
    case 8:
        if (Char.frame < 183)
            adjust = 43;
        /* Fall through. */
    case 7:
    case 11:
        current_frame = frames[Char.frame + adjust - 149];
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
extern volatile unsigned char signflag;
extern volatile int accumulator;

int far pascal AddFCharX(int value)
{
    if (signflag)
        value = -value;
    accumulator = accumulator + value;
    return accumulator;
}


extern int animation_advance;
extern int restricted_step;
extern unsigned char STATE_MODE;
extern unsigned char STATE_LEVEL;

extern unsigned char animation_variant;
extern int animation_items;
extern int animation_portion;
extern int sound_steps;
extern unsigned char active_sound_channels;
extern int kid_step;


typedef int far * near * SequenceHandle;

int far _fastcall GetSeqOp(SequenceHandle sequence);
unsigned int far _fastcall find_char_sequence(unsigned int id);
void far pascal anim_char_tap(int sound);
int far pascal reset_char_y(void);
void far pascal SetKidDeathMethod(unsigned char method);
void far pascal PotionEffect(void);
void far pascal remove_char(void);

extern unsigned int far pascal SAVE_HANDLE_FLAGS(SequenceHandle handle);
extern void far pascal FETCH_LOCKED_SHAPE_LIST(SequenceHandle handle);
extern void far pascal RESTORE_HANDLE_FLAGS(SequenceHandle handle, unsigned int status);
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
    SequenceHandle sequence = (SequenceHandle)find_char_sequence((*(unsigned int near *)&Char.sequence_id));
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
            (*(int near *)&Char.sequence) = 0;
            break;
        case -17:
            remove_char();
            reading = 0;
            break;
        case -16:
            ++animation_items;
            if (STATE_LEVEL == 6)
                animation_variant = (signed char)(*(unsigned char near *)&Char.column) >= 6 ? 2 : 1;
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
                    sequence = (SequenceHandle)find_char_sequence((*(unsigned int near *)&Char.sequence_id));
                    status = SAVE_HANDLE_FLAGS(sequence);
                    FETCH_LOCKED_SHAPE_LIST(sequence);
                    break;
                }
            } else if (opcode == 3) {
                TriggerJinnee();
            }
            break;
        case -13:
            animation_advance = -1;
            break;
        case -12:
            animation_advance = 1;
            break;
        case -11:
            SetKidDeathMethod(GetSeqOp(sequence));
            break;
        case -10:
            if (restricted_step)
                goto change_sequence;
            GetSeqOp(sequence);
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
            if ((*(volatile unsigned char near *)&Char.character_kind) == 10)
                char_x = AddCharX(25);
            Char.direction ^= -1;
            break;
        case -1:
        change_sequence:
            opcode = GetSeqOp(sequence);
            if (STATE_LEVEL == 5 && character_row == 1 &&
                (character_room == 10 || character_room == 7 || character_room == 12) &&
                opcode == 57 && (*(int near *)&Char.sequence) != 60 && (*(unsigned char near *)&Char.column) == 9)
                opcode = 86;
            (*(unsigned int near *)&Char.sequence_id) = opcode;
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
        entries = RESOLVE_HANDLE_TO_FAR_PAYLOAD((unsigned int)sequence);
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
        if (STATE_MODE != 1 && !(active_sound_channels & 3)) {
            AddSound(sound_steps % 2 + 23);
            if ((*(volatile unsigned char near *)&Char.character_kind) == 4 && IsExitChasingSkel(character_room))
                char_x = AddCharX(Char.slot - 1);
        }
        /* Fall through. */
    case 0:
        if (!(*(volatile unsigned char near *)&Char.character_kind))
            kid_step = 1;
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


extern volatile unsigned char STATE_ARGUMENT;

void far pascal SetKidDeathMethod(unsigned char value)
{
    if ((*(volatile unsigned char near *)&Char.character_kind) == 0 || (*(volatile unsigned char near *)&Char.character_kind) == 1)
        STATE_ARGUMENT = value;
}

int far pascal reset_char_y(void)
{
    Char.y = (signed char)63 * character_row + 56;
    if ((*(volatile unsigned char near *)&Char.character_kind) == 7 || (*(volatile unsigned char near *)&Char.character_kind) == 8)
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

extern CHARREC RoomOpp[5];

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
   fchar_rect (DS:625b = FChar + 9) is a descriptive alias: the layout of FChar is not proven here.
   GamePlayRect (DS:09fe) is the FM Towns global name anchored by matching copies. */
typedef struct {
    int top, left, bottom, right;
} Rect;

extern Rect fchar_rect;
extern Rect GamePlayRect;

void init_fchar_rect(void)
{
    fchar_rect = GamePlayRect;
}

/* Semantic name hypothesis only; original symbol remains unrecovered. */
void far pascal ConvertX(register int value, int near *value_out) {
    if (value < 0)
        --value;
    *value_out = value;
}

/* Names and qualifiers are reconstruction hypotheses. */


extern int restricted_step;

void far pascal Gravity(void)
{
    if (Char.action == 4 || Char.action == 9) {
        if (restricted_step) {
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
    if (Char.y < 2000) {
        Char.y += Char.vertical_step;
        if (Char.action == 4 || (*(volatile unsigned char near *)&Char.character_kind) == 7 || (*(volatile unsigned char near *)&Char.character_kind) == 8) {
            char_x = AddCharX(Char.horizontal_step);
            reread_cells();
        }
    }
}

/* Names, signedness and qualifiers are reconstruction hypotheses. */


void far pascal ClearCharVel(void)
{
    register signed char zero = 0;
    Char.horizontal_step = zero;
    Char.vertical_step = zero;
}


int far pascal decrement_life_points(signed char amount)
{
    int lethal = 0;

    if ((*(volatile unsigned char near *)&Char.character_kind) == 4 && STATE_LEVEL == 5 && character_row == 1 &&
          (character_room == 10 || character_room == 7 || character_room == 12))
        goto done;
    if (amount >= Char.life_points) {
        if (-Char.life_points < Char.life_delta)
            Char.life_delta = -Char.life_points;
        lethal = 1;
    } else if (-Char.life_delta < amount) {
        Char.life_delta = -amount;
    } else {
        lethal = Char.life_points + Char.life_delta == 0;
    }
done:
    return lethal;
}

char far pascal get_under_foot(void)
{
    return ReadCell(character_room, (*(unsigned char near *)&Char.column), character_row);
}

extern unsigned char frame_flags;
extern int restricted_step;

extern signed char front_offsets[];
extern signed char behind_offsets[];

extern unsigned char grab_enabled;
struct CopyRecord {
    char bytes[5];
};

extern struct CopyRecord SelDir;
extern struct CopyRecord Dir;
extern unsigned int crouch_fall;
extern unsigned int grab_delay;
extern unsigned int ledge_hold;
extern char fall_check_before[], fall_check_after[];
extern int far pascal check_fall_state(char far *name);
extern int far pascal IsWall(char kind);
extern int far pascal IS_SIX_KIND(char kind);
extern int far pascal tile_is_ledge(char kind);
extern int far pascal RoofClimbingIntoShip(void);
extern void far pascal prepare_palace_fall(void);
extern int far pascal palace_grab_test(void);
extern int far pascal probe_chain_floor(int index);
extern unsigned char far pascal turn_hidden_actor(unsigned char direction);
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

    if ((frame_flags & 0x40) && (*(volatile unsigned char near *)&Char.character_kind) != 10) {
        kind = get_under_foot();
        if (IsWall(kind)) {
            if (kind != 7 || ((Char.frame < 246 || Char.frame > 261) &&
                             (*(volatile unsigned char near *)&Char.character_kind) != 11))
                kind = inside_cell();
        }
        if (IS_SIX_KIND(kind) ||
            (kind == 30 && ((unsigned char)cell_modifier & 15) < 2)) {
            if (STATE_MODE == 5 && RoofClimbingIntoShip())
                return;
            if (STATE_MODE == 2 && probe_chain_floor(16))
                return;
            if ((*(volatile unsigned char near *)&Char.character_kind) == 11)
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
    int sequence = -1;
    int distance;
    int min_y;

    ++character_row;
    if (Char.stance == 1)
        Char.stance = 0;
    if ((*(volatile unsigned char near *)&Char.character_kind) == 0 && STATE_LEVEL == 5 && character_room == 10 &&
        ((char_direction == 0 && Dir.bytes[0]) ||
         (char_direction == 0xff && Dir.bytes[1]))) {
        sequence = 198;
        adjust_clearance = 1;
    }

    if (sequence == -1) {
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
            } else if (old_frame - 150U < 30U) {
                if (!(*(volatile unsigned char near *)&Char.character_kind)) {
                    crouch_fall = adjust_clearance;
                    if (old_frame == 153 || (*(int near *)&Char.sequence) == 56)
                        sequence = 95;
                    else
                        sequence = 81;
                } else {
                    crouch_fall = 0;
                    if ((*(int near *)&Char.sequence) == 86 || (*(int near *)&Char.sequence) == 67 ||
                        (*(int near *)&Char.sequence) == 108)
                        sequence = 83;
                    else
                        sequence = 82;
                }
            } else if ((*(volatile unsigned char near *)&Char.character_kind) == 2 && (*(int near *)&Char.sequence) == 100) {
                sequence = 186;
            } else {
                sequence = 7;
            }
            break;
        }
    }

    if (!check_fall_state((char far *)fall_check_before) && adjust_clearance) {
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
                min_y = (signed char)63 * character_row + 1;
                if (min_y > Char.y)
                    Char.y = min_y;
            }
        }
    }

    if (STATE_MODE == 1) {
        sequence = 27;
        prepare_palace_fall();
        SetKidDeathMethod(8);
    } else if ((*(volatile unsigned char near *)&Char.character_kind) == 0 && restricted_step && (*(int near *)&Char.sequence) != 228) {
        sequence = 228;
    }
    JumpSeq(sequence);
    AnimChar();
    reread_cells();

    if (check_fall_state((char far *)fall_check_after) && adjust_clearance) {
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
                min_y = (signed char)63 * character_row + 1;
                if (min_y > Char.y)
                    Char.y = min_y;
            }
        }
    }

    if (IsWall(get_under_foot())) {
        inside_cell();
    } else if (IsWall(get_cells_ahead(1))) {
        if (old_frame == 44 && get_edge_distance() < 14) {
            JumpSeq(114);
            AnimChar();
        } else {
            char_x = AddCharX(-2);
        }
        reread_cells();
    }
}

void far pascal FallingInUndefinedRoom(void)
{
    Gravity();
    add_char_velocity();
    if (STATE_MODE == 5) {
        if (Char.y > 384) {
            if (viewed_room != 19 && viewed_room != 16) {
                AddSound(0);
                decrement_life_points(100);
                Char.frame = 185;
                SetKidDeathMethod(3);
            }
            Char.y = 384;
            Char.vertical_step = 0;
            Char.action = 1;
            if ((*(volatile unsigned char near *)&Char.character_kind) && (*(unsigned char near *)&Kid.opponent_index) == (unsigned char)Char.slot)
                (*(unsigned char near *)&Kid.opponent_index) = turn_hidden_actor(char_direction);
        } else if ((*(volatile unsigned char near *)&Char.character_kind) == 2 && Char.frame != 185 &&
                   !query_sound(0x2729)) {
            AddSound(25);
        }
    } else if (Char.y > 384) {
        decrement_life_points(100);
        Char.frame = 185;
        SetKidDeathMethod(3);
        Char.y = 384;
        Char.vertical_step = 0;
        if ((*(volatile unsigned char near *)&Char.character_kind) == 0)
            Char.action = 1;
        else if ((*(volatile unsigned char near *)&Char.character_kind) == 1)
            CutToKidRoom();
    }
}

int far pascal is_undefined_room(void)
{
    if (character_room == 0 ||
        (((character_room == 3 && STATE_MODE == 6) ||
          (character_room == 5 && STATE_MODE == 6)) &&
         character_row >= 11))
        return 1;
    return 0;
}

void far pascal FallOn(void)
{
    int position;
    if (grab_enabled && Char.vertical_step < 32 && Char.alive < 0 &&
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
        grab_delay = 12;
        ledge_hold = 0;
    }
}

int far pascal GrabTest(void)
{
    char above = get_above();
    unsigned int modifier = cell_modifier;
    char ahead = get_above_front();
    int result = check_ledge(above, ahead, modifier, cell_modifier);

    if (STATE_MODE == 5 && !result)
        return palace_grab_test();
    return result;
}

char far pascal inside_cell(void)
{
    int distance;
    char ahead;

    if (Char.sequence_group != 4 && (*(volatile unsigned char near *)&Char.character_kind) != 11 &&
        !(character_room == 3 && STATE_MODE == 6 && (signed char)(*(unsigned char near *)&Char.column) < 0)) {
        distance = get_edge_distance();
        ahead = get_cells_ahead(1);
        if (distance < 11 && !IsWall(ahead))
            distance += 11;
        else
            distance = 15 - distance;
        char_x = AddCharX(distance);
        reread_cells();
    }
    return get_under_foot();
}

char far pascal get_cells_ahead(signed char distance)
{
    return ReadCell(character_room,
                    (*(unsigned char near *)&Char.column) + front_offsets[(signed char)char_direction] * distance,
                    character_row);
}

char far pascal get_behind(void)
{
    return ReadCell(character_room, (*(unsigned char near *)&Char.column) + behind_offsets[(signed char)char_direction],
                    character_row);
}

extern unsigned char frame_x;
extern unsigned char frame_flags;
extern signed char column_remainder;

int far pascal get_base_x(void)
{
    return AddCharX((signed char)(frame_x - (frame_flags & 31)));
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
        distance = 31 - column_remainder;
    else
        distance = column_remainder;
    return distance;
}

char far pascal get_char_col(int x)
{
    return get_col(x - 14);
}

char far pascal get_col(int x)
{
    x -= 4;
    column_remainder = x % 32;
    return x / 32 - 4;
}

char far pascal GetRow(int value)
{
    int shifted = value - 3;
    char result = shifted / 63;

    if (shifted <= 0)
        --result;

    return result;
}

extern unsigned char state_left_room;
extern unsigned char state_right_room;
extern unsigned char state_other_room;

char far pascal GetCellCol(void)
{
    signed char col = cell_column;

    if (cell_room != viewed_room && cell_room != state_other_room && cell_room != STATE_FLAG) {
        if (cell_room == state_left_room)
            col -= 10;
        else if (cell_room == state_right_room)
            col += 10;
    }

    return col;
}

void far pascal prepare_player_control(void)
{
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
extern void far pascal play_shadow_death(void);
extern void far pascal update_shadow_control(void);
extern void far pascal handle_replay_input(void);
extern void far pascal apply_input(void);
extern unsigned int prefer_kid_state;
extern unsigned int death_sequence_primary, death_sequence_alternate;
extern unsigned int pending_death_sound, death_timer, death_timer_limit;
extern unsigned int delayed_death, replay_input, Invert;
extern signed char LevelIdx;
extern signed char state_byte_5e30, state_byte_5e31, state_byte_5e32;

int far pascal ctrl_player(void)
{
    int result;
    prepare_player_control();
    result = PlayerCtrl();
    if (result != -2 || Char.alive < 0 || !is_dead_pos(Char.frame))
        goto done;

    if (prefer_kid_state) {
        dispatch_sound(0);
        Kid.life_delta = Kid.life_initial;
        copy_kid_to_char();
        JumpSeq(2);
        char_x += 16;
        AnimChar();
        reread_cells();
        finish_start_kid();
    }

    if ((*(volatile unsigned char near *)&Char.character_kind) != 0 && (*(volatile unsigned char near *)&Char.character_kind) != 1)
        goto done;
    if (query_sound(death_sequence_primary + 10000) &&
        death_sequence_primary != 4 && death_sequence_primary != 7 &&
        death_sequence_primary != 54)
        goto done;

    pending_death_sound = 0;
    if (Char.alive >= 0 && Char.alive < 6)
        goto advance;
    if (Char.alive == 6) {
        AddKidDeathSong(STATE_ARGUMENT);
        goto advance;
    }
    if (Char.alive != 7 || query_sound(death_sequence_primary + 10000))
        goto done;
    if (query_sound(death_sequence_alternate + 10000))
        goto done;
    if (!delayed_death) {
        lose_game();
        result = -1;
    } else if (death_timer_limit != 600) {
        death_timer = death_timer_limit = 600;
        draw_restart_message();
    }
advance:
    ++Char.alive;
done:
    return result;
}

int far pascal PlayerCtrl(void)
{
    int result;
    if (Char.alive < 0 && !Char.life_points)
        Char.alive = 0;
    if (grab_delay)
        --grab_delay;
    LoadSelect();
    Input();
    if (LevelIdx != animation_items || !character_room || (*(int near *)&Char.sequence) == 59)
        state_byte_5e31 = state_byte_5e30 = 0;
    ClrJoystick();
    UserCtrl();
    result = CheckForEndOfRecording();
    SaveSelect();
    return result;
}

void far pascal unused_shadow_hook(void)
{
}

void far pascal shad_ctrl(void)
{
    if (Char.alive >= 0) {
        if (Char.alive >= 6) {
            play_shadow_death();
            if ((*(unsigned char near *)&Kid.opponent_index) == (unsigned char)Char.slot)
                (*(unsigned char near *)&Kid.opponent_index) = -1;
        } else if (Char.field_0f) {
            ++Char.alive;
        }
    } else if (!Char.life_points) {
        Char.alive = 0;
    }
    update_shadow_control();
    if (replay_input)
        handle_replay_input();
    if ((*(volatile unsigned char near *)&Char.character_kind) != 1 && (*(volatile unsigned char near *)&Char.character_kind) != 12)
        apply_input();
}

void far pascal UserCtrl(void)
{
    unsigned char direction = char_direction;
    if (!direction)
        FaceJoystick();
    if (Invert)
        InvertJoystick();
    if (replay_input)
        handle_replay_input();
    apply_input();
    if (!direction)
        FaceJoystick();
    if (Invert)
        InvertJoystick();
}

extern signed char state_byte_5e30;

void far pascal FaceJoystick(void)
{
    char temporary;
    state_byte_5e30 = -state_byte_5e30;
    temporary = Dir.bytes[0];
    Dir.bytes[0] = Dir.bytes[1];
    Dir.bytes[1] = temporary;
}

extern signed char state_byte_5e31;

void far pascal InvertJoystick(void)
{
    char temporary;
    state_byte_5e31 = -state_byte_5e31;
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
extern struct CopyRecord Dir;
extern struct CopyRecord SelDir;

void LoadSelect(void)
{
    Dir = SelDir;
}

void *memset(void *, int, unsigned);
#pragma intrinsic(memset)
void InitInput(void) { memset(SelDir.bytes, 0, sizeof(SelDir.bytes)); }

void far pascal ClrJoystick(void)
{
    if ((signed char)Dir.bytes[0] >= 0) {
        if (state_byte_5e30 == -1) {
            if (!Dir.bytes[0])
                Dir.bytes[0] = -1;
        } else
            Dir.bytes[0] = 0;
    }
    if ((signed char)Dir.bytes[1] >= 0) {
        if (state_byte_5e30 == 1) {
            if (!Dir.bytes[1])
                Dir.bytes[1] = -1;
        } else
            Dir.bytes[1] = 0;
    }
    if ((signed char)Dir.bytes[2] >= 0) {
        if (state_byte_5e31 == -1) {
            if (!Dir.bytes[2])
                Dir.bytes[2] = -1;
        } else
            Dir.bytes[2] = 0;
    }
    if ((signed char)Dir.bytes[3] >= 0) {
        if (state_byte_5e31 == 1) {
            if (!Dir.bytes[3])
                Dir.bytes[3] = -1;
        } else
            Dir.bytes[3] = 0;
    }
    if (state_byte_5e32 == -1) {
        if (grab_enabled != 1)
            grab_enabled = -1;
    } else if (state_byte_5e32 == -2) {
        if (grab_enabled != 2)
            grab_enabled = -2;
    } else
        grab_enabled = 0;
}

extern int far pascal is_open_tile(char kind);

int far pascal check_ledge(char above, char ahead, unsigned int above_modifier,
                           unsigned int ahead_modifier)
{
    int result;
    if (!IS_SIX_KIND(above) || (STATE_MODE == 2 && (above_modifier & 0x80)) ||
        ((ahead == 11 || ahead == 15) && (ahead_modifier & 15)) ||
        !is_open_tile(ahead) ||
        (ahead == 4 && !(ahead_modifier & 255) && char_direction == 255) ||
        (STATE_MODE == 5 && character_room == 15 && character_row != 0))
        result = 0;
    else
        result = 1;
    return result;
}

char far pascal get_above(void)
{
    return ReadCell(character_room, (*(unsigned char near *)&Char.column), character_row - 1);
}

char far pascal get_above_behind(void)
{
    return ReadCell(character_room, (*(unsigned char near *)&Char.column) + behind_offsets[(signed char)char_direction],
                    character_row - 1);
}

char far pascal get_above_front(void)
{
    return ReadCell(character_room, (*(unsigned char near *)&Char.column) + front_offsets[(signed char)char_direction],
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

extern int special_object_flag;
extern unsigned char object_removed[];
extern struct object_modifier { unsigned int flags, reserved; } object_modifiers[];
extern void far pascal mark_cell_redraw(void);
void far pascal RemoveObj(int portion)
{
    unsigned int modifier;
    int index;
    animation_portion = portion;
    grab_enabled = 1;
    if (STATE_MODE == 3)
        modifier = cell_modifier & 0x3f00;
    else if (STATE_MODE == 4)
        modifier = cell_modifier & 0x0f00;
    else
        modifier = 0;
    if (special_object_flag)
        modifier |= 0xc000;
    index = (unsigned char)30 * cell_room + cell_index;
    object_removed[index] = 1;
    object_modifiers[index].flags = modifier;
    mark_cell_redraw();
}

extern Rect NullRect;
extern signed char above_col, front_col, above_row, contact_row;
extern unsigned int clip_tables[];
extern int far pascal clip_floor(int near *bounds, unsigned int table, int column, int row);
extern void far pascal clip_cave(void);
extern void far pascal clip_ruins(void);
extern void far pascal clip_temple(void);
extern void far pascal clip_rooftops(void);
extern int far _fastcall clip_desert_floor(unsigned int modifier);
extern int clip_top_cache;
extern int sprite_y;
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
        fchar_rect = NullRect;
        return;
    }
    if ((*(volatile unsigned char near *)&Char.character_kind) == 0 && Char.frame >= 217 && Char.frame <= 226) {
        tile = ReadCell(character_room, (*(unsigned char near *)&Char.column), character_row);
        if (clip_floor(workspace.bounds, clip_tables[STATE_MODE], (tile == 16) + (signed char)(*(unsigned char near *)&Char.column), character_row)) {
            fchar_rect.top = workspace.bounds[0];
            fchar_rect.right = workspace.bounds[3];
        }
        return;
    }
    if (Char.action == 9) {
        if (STATE_MODE == 5)
            clip_cave();
        return;
    }

    flying = (*(volatile unsigned char near *)&Char.character_kind) == 7 || (*(volatile unsigned char near *)&Char.character_kind) == 8;
    tile = ReadCell(character_room, above_col, above_row);
    if (IS_SIX_KIND(tile))
        goto solid;
    if (STATE_MODE == 3 && tile == 20 && clip_desert_floor(cell_modifier))
        goto solid;
    if (cell_room == 1 && STATE_LEVEL == 14)
        goto solid;
    if ((cell_room == 7 || cell_room == 8) && STATE_MODE == 6 && (*(int near *)&Char.sequence) != 14)
        goto solid;
    if (action == 0 && (frame == 79 || frame == 81))
        goto adjust;
    tile = ReadCell(character_room, front_col, above_row);
    if (IS_SIX_KIND(tile) || (tile == 20 && STATE_MODE == 3 && clip_desert_floor(cell_modifier))) {
    solid:
        clearance = 0;
    }

adjust:
    if (clearance && STATE_MODE != 5 && STATE_MODE != 1 &&
        !(character_room == 16 && STATE_LEVEL == 9) &&
        !(character_room == 27 && STATE_LEVEL == 6) &&
        !(character_room == 3 && STATE_MODE == 6) &&
        !(character_room == 6 && STATE_MODE == 6)) {
        if ((signed char)((tile = row + 1) - 1)) {
            workspace.top = (signed char)63 * tile - 60;
            clearance = workspace.top >= sprite_y ? 0 : workspace.top - 15 >= clip_top_cache ? 0 : clearance;
        }
        if (clearance && !flying) {
            if ((workspace.top = (signed char)63 * tile - 60) > fchar_rect.top)
                fchar_rect.top = clip_top_cache = workspace.top;
        }
    }
    switch (STATE_MODE) {
    case 1: clip_ruins(); break;
    case 4: clip_temple(); break;
    case 5: clip_cave(); break;
    case 6: clip_rooftops(); break;
    }
}

extern int sprite_id;
extern unsigned char sprite_table_index;
extern unsigned int sprite_tables[];
extern void far *far pascal get_set_image_size(unsigned int table, int id, int far *size);

void far pascal MakeFCharClipRect(void)
{
    int size[2];
    if (sprite_id != -1 && get_set_image_size(sprite_tables[sprite_table_index], sprite_id, size)) {
        fchar_rect.top = sprite_y - size[0];
        fchar_rect.bottom = sprite_y + 1;
        if (!signflag) {
            fchar_rect.right = (*(int *)&accumulator);
            fchar_rect.left = (*(int *)&accumulator) - size[1];
        } else {
            fchar_rect.left = (*(int *)&accumulator);
            fchar_rect.right = (*(int *)&accumulator) + size[1];
        }
        return;
    }
    fchar_rect = NullRect;
}

void far pascal uneven_floor(void)
{
    if ((*(volatile unsigned char near *)&Char.character_kind) != 11 && (*(volatile unsigned char near *)&Char.character_kind) != 7 && (*(volatile unsigned char near *)&Char.character_kind) != 8 && get_under_foot() == 6)
        Char.y = (signed char)63 * character_row + 57;
}

struct FCharRecord { char bytes[20]; };
extern struct FCharRecord FChar, saved_fchar;

void far pascal IndexChar(void)
{
    unsigned int frame = Char.frame;
    int action = Char.action;
    if (action == 1) {
        cell_row = contact_row;
        cell_column = above_col;
    } else {
        cell_row = character_row;
        cell_column = (*(unsigned char near *)&Char.column);
    }
    if ((frame >= 135 && frame < 149) || action == 2 || action == 3 || action == 4 || action == 6)
        cell_column += behind_offsets[(signed char)char_direction];
    FChar.bytes[0] = IndexCell1(cell_column, cell_row);
}


extern void far pascal BoostMeter(void);
extern void far pascal GoWeightless(void);
extern void far pascal ToggleInvertState(void);
extern void far pascal transfer_jaffar_to_escher2(void);
void far pascal VanishChar(void);

void far pascal PotionEffect(void)
{
    if ((*(volatile unsigned char near *)&Char.character_kind) || !animation_portion)
        return;
    if (animation_portion == -1) {
        (*(unsigned char near *)&Char.stance) = 0;
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
        case 2:
            GoWeightless();
            break;
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
        (STATE_MODE == 5 && character_room != 19 && character_room != 16 &&
         character_room != 15 && is_dead_pos(Char.frame)))
        result = 1;
    else
        result = 0;
    return result;
}


extern unsigned char actor_variant;

int far pascal is_dead_pos(unsigned int frame)
{
    int dead;
    if (frame == 185) {
        dead = 1;
        goto done;
    }
    switch ((signed char)(*(volatile unsigned char near *)&Char.character_kind)) {
    case 0:
        dead = frame == 242 || frame == 243 || frame == 185 || frame == 271 || frame == 266;
        if (!dead && STATE_LEVEL == 5 && character_room == 3)
            dead = Char.alive >= 0 && Char.sequence_group == 10;
        break;
    case 2:
        if (!actor_variant) {
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
done:
    return dead;
}


void far pascal remove_char(void)
{
    if (character_room == 6 && STATE_MODE == 6 &&
        (*(int near *)&Char.sequence) == 240 && Kid.alive < 0)
        transfer_jaffar_to_escher2();
    VanishChar();
    character_room = 0;
}

extern void far pascal change_actor_life(char slot, int life, int maximum);
void far pascal VanishChar(void)
{
    signed char zero = 0;
    change_actor_life(Char.slot, 0, Char.life_initial);
    char_direction = 86;
    Char.action = zero;
    Char.alive = zero;
    Char.life_points = zero;
    if (Char.slot != zero && (*(unsigned char near *)&Kid.opponent_index) == (unsigned char)Char.slot)
        (*(unsigned char near *)&Kid.opponent_index) = turn_hidden_actor(1);
}

void SaveFChar(void) { saved_fchar = FChar; }
void RestoreFChar(void) { FChar = saved_fchar; }

extern int far pascal can_continue_run_jump(char cell, unsigned int modifier);
extern void far pascal engage_weapon(void);
void far pascal guard_ctrl(void)
{
    if (Char.frame == 166 && (signed char)Dir.bytes[3] < 0) {
        if ((signed char)Dir.bytes[0] < 0) {
            if (!can_continue_run_jump(get_behind(), cell_modifier))
                char_x = AddCharX(5);
            engage_weapon();
        } else {
            Dir.bytes[3] = 1;
            JumpSeq(80);
        }
    }
}

#define opponent_direction ((unsigned char)Opp.direction)
#define opponent_x (Opp.x)
#define opponent_row (Opp.row)
#define opponent_room (Opp.room)
#define opponent_life (Opp.life_points)
int far pascal get_opp_dist(void)
{
    int distance;
    if (opponent_life && opponent_room == character_room && opponent_row == character_row &&
        (unsigned char)Opp.direction != 86) {
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

