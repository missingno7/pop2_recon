/* CTRL.C worker draft. Functions follow original order. Omitted bodies
 * leave later publics short of their original code offsets. */

extern unsigned char cell_room;
extern signed char cell_column;
extern signed char cell_row;
extern signed char cell_index;
extern char cell_kind;
extern unsigned int cell_modifier;
extern unsigned char *room_kinds;
/* Each modifier entry occupies two words. ReadCell uses its first word. */
extern unsigned int (*room_modifiers)[2];

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
        cell_modifier = room_modifiers[cell_index][0];
    } else {
        cell_modifier = 0;
        cell_kind = GetUndefineCellId(column, row);
    }
    return cell_kind;
}

extern unsigned char STATE_MODE;
extern unsigned char STATE_LEVEL;
extern unsigned char STATE_KIND;
extern unsigned char STATE_FLAG;

char far pascal GetUndefineCellId(signed char x, signed char y)
{
    char result;
    switch (STATE_MODE) {
    case 2:
        if ((STATE_KIND == 4 && STATE_LEVEL == 13) ||
            (STATE_LEVEL == 13 && STATE_KIND == 13))
            result = 0;
        else
            result = 20;
        break;
    case 4:
        if (!STATE_FLAG && y == -1) {
            result = 0;
            break;
        }
        if (STATE_KIND == 27 && STATE_LEVEL == 6) {
            result = 0;
            break;
        }
        result = 20;
        break;
    case 5:
        result = 0;
        break;
    case 6:
        if (((STATE_KIND == 7 || STATE_KIND == 8) && STATE_MODE == 6) ||
            (STATE_KIND == 3 && STATE_MODE == 6 && (x < 0 || x == 9) && y == 2))
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

struct frame_record {
    unsigned char prefix;
    struct frame_info info;
    unsigned char suffix;
};

extern struct frame_info current_frame;
struct default_frame_record {
    struct frame_info info;
    unsigned char suffix[2];
};

extern struct default_frame_record far default_frames[];
extern signed char frame_set;
extern unsigned int frame_number;
extern unsigned int frame_resource;
extern void far *far pascal lock_frame_resource(unsigned int handle);

extern unsigned char base_cell;
int far pascal get_base_x(void);
char far pascal get_char_col(int x);
char far pascal get_col(int x);
int far pascal offset_char_x(int value);
void far pascal get_frame_info(void);
void far pascal reread_base_cell(void);

void far pascal reread_cells(void)
{
    get_frame_info();
    reread_base_cell();
}

void far pascal reread_base_cell(void)
{
    base_cell = get_char_col(get_base_x());
}

void far pascal get_frame_info(void)
{
    unsigned int adjust = 0;
    struct frame_record far *frames = lock_frame_resource(frame_resource);

    switch (frame_set) {
    case 0:
    case 1:
    case 6:
        current_frame = default_frames[frame_number].info;
        break;
    case 2:
    case 4:
    case 10:
    case 12:
        if (frame_number >= 102 && frame_number < 107)
            adjust = 70;
        current_frame = frames[frame_number + adjust - 116].info;
        break;
    case 8:
        if (frame_number < 183)
            adjust = 43;
        /* Fall through. */
    case 7:
    case 11:
        current_frame = frames[frame_number + adjust - 116].info;
        break;
    }
}

extern unsigned char char_direction;
extern int char_x;

int far pascal offset_char_x(int value)
{
    if (char_direction)
        value = -value;
    return char_x + value;
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

/* 0394..0786 contains the animation dispatcher and its helpers.
 * 07a2..07c6 contains the unresolved character Y reset. */
#define STATE_KIND character_kind
extern volatile unsigned char STATE_KIND;
extern volatile unsigned char STATE_ARGUMENT;

void far pascal SetKidDeathMethod(unsigned char value)
{
    if (STATE_KIND == 0 || STATE_KIND == 1)
        STATE_ARGUMENT = value;
}
#undef STATE_KIND

char far FirstInRow(signed char value)
{
    char result;

    if (value >= 0)
        result = value * 10;
    else
        result = value * 10 + 9;

    return result;
}

/* Unresolved entries between 07fc and 1074 are omitted.
 * The available accepted helpers are kept in original order. */
/* Semantic name hypothesis only; original symbol remains unrecovered. */
void far pascal ConvertX(register int value, int near *value_out) {
    if (value < 0)
        --value;
    *value_out = value;
}

/* Names and qualifiers are reconstruction hypotheses. */
extern signed char state_code;
extern signed char vertical_step;
extern int restricted_step;

void far pascal Gravity(void)
{
    if (state_code == 4 || state_code == 9) {
        if (restricted_step) {
            ++vertical_step;
            if (vertical_step > 4)
                vertical_step = 4;
        } else {
            vertical_step += 3;
            if (vertical_step > 33)
                vertical_step = 33;
        }
    }
}

/* Names, signedness and qualifiers are reconstruction hypotheses. */
extern signed char horizontal_step;
extern signed char vertical_step;

void far pascal ClearCharVel(void)
{
    register signed char zero = 0;
    horizontal_step = zero;
    vertical_step = zero;
}

extern unsigned char frame_x;
extern unsigned char frame_flags;
extern signed char column_remainder;

int far pascal get_base_x(void)
{
    return offset_char_x((signed char)(frame_x - (frame_flags & 31)));
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

/* Unresolved player-control entries between 112a and 1392. */
extern signed char state_byte_5e30;
extern unsigned char state_byte_628c;
extern unsigned char state_byte_628d;

void far pascal FaceJoystick(void)
{
    char temporary;
    state_byte_5e30 = -state_byte_5e30;
    temporary = state_byte_628c;
    state_byte_628c = state_byte_628d;
    state_byte_628d = temporary;
}

extern signed char state_byte_5e31;
extern unsigned char state_byte_628e;
extern unsigned char state_byte_628f;

void far pascal InvertJoystick(void)
{
    char temporary;
    state_byte_5e31 = -state_byte_5e31;
    temporary = state_byte_628e;
    state_byte_628e = state_byte_628f;
    state_byte_628f = temporary;
}

/* Neutral DS-relative byte aliases; original names and qualifiers are unknown. */
extern unsigned char state_byte_628f;
extern unsigned char state_byte_628e;
extern unsigned char state_byte_628c;
extern unsigned char state_byte_628d;

unsigned char far pascal ClrAll(void)
{
    char zero = 0;
    state_byte_628f = zero;
    state_byte_628e = zero;
    state_byte_628c = zero;
    state_byte_628d = zero;
    return 1;
}

#define CopyRecord SelectionRecord
/* SaveSelect: remember the current selection (FM Towns CTRL.C public; anchored by PlayerCtrl
   call sites in both builds). Dir/SelDir are the FM Towns globals at DOS DS:628c/DS:626a. */
struct CopyRecord {
    char bytes[5];
};

extern struct CopyRecord SelDir;
extern struct CopyRecord Dir;

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
#undef CopyRecord

extern char state[5];
void *memset(void *, int, unsigned);
#pragma intrinsic(memset)
void InitInput(void) { memset(state, 0, sizeof(state)); }

/* Unresolved helpers between 1414 and 158e. */
#define signflag char_facing
/* Semantic name only; this external names the independently witnessed DS byte. */
extern unsigned char signflag;

int far pascal FaceDx(int value)
{
    if (!signflag)
        value = -value;
    return value;
}
#undef signflag

/* Unresolved character and potion routines between 15a4 and 1c3e. */
struct Record { char bytes[20]; };
extern struct Record saved, current;
void SaveFChar(void) { saved = current; }

#define CopyRecord FCharRecord
/* RestoreFChar: restore the 20-byte FChar record (FM Towns CTRL.C; anchored by SetupCarpet).
   saved_fchar is descriptive: its original name is not anchored. */
struct CopyRecord {
    char bytes[20];
};

extern struct CopyRecord FChar;
extern struct CopyRecord saved_fchar;

void RestoreFChar(void)
{
    FChar = saved_fchar;
}
#undef CopyRecord

/* Unresolved guard and opponent routines between 1c62 and 1d94. */
#define CopyRecord CharacterRecord
/* copy_kid_to_char (descriptive name): Char = Kid, the inverse of the copies in SaveKidwOpp.
   Char/Kid are the FM Towns globals at DOS DS:5c00/DS:5c80. */
struct CopyRecord {
    char bytes[64];
};

extern struct CopyRecord Char;
extern struct CopyRecord Kid;

void copy_kid_to_char(void)
{
    Char = Kid;
}
#undef CopyRecord

/* The frame's final routines follow the Kid-to-Char copy. */
