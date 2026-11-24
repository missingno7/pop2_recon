#define KIND_SAME_ROW (Room.number)
#define KIND_ROW_ABOVE (Room.left)
#define KIND_ROW_BELOW (Room.right)

struct collision_row {
    unsigned char cell[10];
};

extern unsigned char current_rooms[10], prev_rooms[10], below_rooms[10], above_rooms[10];
extern unsigned char prev_edges[10], current_edges[10], below_edges[10], above_edges[10];
extern signed char current_row, first_col, last_col;
extern signed char previous_row;
extern signed char left_barrier_col;
extern signed char right_barrier_col;
#pragma pack(1)
typedef struct {
    unsigned char number;                             /* 00 */
    unsigned char left, right, above, below;           /* 01-04 */
    unsigned char above_left, above_right;             /* 05-06 */
    unsigned char below_left, below_right;             /* 07-08 */
    unsigned char custom;                             /* 09 */
} ROOMREC;
#pragma pack()
extern ROOMREC Room;
#pragma pack(1)
extern struct {
    signed char kind;                                 /* 00 */
    unsigned int modifier;                            /* 01 */
    signed char index;                                /* 03 */
    unsigned char room;                               /* 04 */
    signed char column, row;                          /* 05-06 */
    signed char first_col, last_col;                   /* 07-08 */
    unsigned char top_row, bottom_row;                 /* 09-0a */
} Cell;
#pragma pack()
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



#pragma pack(1)
typedef struct {
    unsigned int modifier, unknown_02;
} LEVELCELL;

typedef struct {
    unsigned char left, right, above, below;
} ROOMLINKS;

typedef struct {
    signed char opponent_count;
    unsigned char opponent_data[5][23];
} ROOMOBJECTS;

typedef struct {
    signed char room, cell;
} REBIRTHPOINT;

typedef struct {
    signed char count;
    unsigned char spawn_flags;
    unsigned char header[2];
    unsigned char points[3][10];
} ROOMGENERATORS;

typedef struct {
    unsigned char cell_kinds[32][30];                  /* 0000 */
    LEVELCELL cell_info[32][30];                       /* 03c0 */
    unsigned char unknown_12c0[0x500];                 /* 12c0 */
    ROOMLINKS room_links[32];                          /* 17c0 */
    unsigned char room_count;                         /* 1840 */
    unsigned char unknown_1841[4];
    unsigned char environment_kind;                  /* 1845 */
    unsigned char unknown_1846;
    signed char level_number;                         /* 1847 */
    int background_overlays[8];                       /* 1848 */
    unsigned char unknown_1858[4];
    int actor_phase;                                  /* 185c */
    unsigned char unknown_185e[2];
    signed char entry_room, entry_cell, entry_facing;    /* 1860-1862 */
    unsigned char unknown_1863[3];
    unsigned char preferred_opponent;                /* 1866 */
    ROOMOBJECTS room_objects[32];                      /* 1867 */
    REBIRTHPOINT rebirth_points[2];                    /* 26e7 */
    unsigned char unknown_26eb[0x0e];
    ROOMGENERATORS generators[32];                     /* 26f9 */
    unsigned char unknown_2b39[0x3c0];                 /* 2b39-2ef8 */
} LEVELREC;
#pragma pack()
extern LEVELREC Level;
extern int ledge_check, right_edge, left_edge, ImHeight;
extern int bump_pending;

extern int far _fastcall slide_rock_on_row(unsigned char room, signed char row);
extern CHARREC Char;
extern void slide_rock_collision_row(signed char row, unsigned char *rooms, unsigned char *edges);
extern signed char get_char_col(int x);
extern unsigned char ReadCell(unsigned char room, signed char col, signed char row);
extern int IS_SIX_KIND(unsigned char cell);
extern int AddCharX(int distance);
extern void JumpSeq(int sequence);

void CheckWeightlessBarr(void);
void InitColDetBufs(void);
void GetColDetData(signed char row, unsigned char *rooms, unsigned char *edges);
int GetBarrCode(unsigned char cell);
int GetLeftBarr(unsigned char room, signed char col, signed char row);
int GetRightBarr(unsigned char room, signed char col, signed char row);
extern int collision_x;
extern int ColEdgeX[20];
extern signed char BarL[], BarR[];


#define cell_flags (*(unsigned char *)&Cell.modifier)

extern int slide_rock_barrier(signed char direction, unsigned char room, signed char row);
extern int probe_chain_floor(int cell);
extern int IsWall(unsigned char cell);
extern void AnimChar(void);
extern void reread_cells(void);
extern void reset_char_y(void);
extern void AddSound(int);
extern int far _fastcall probe_jump_anchor(unsigned char room, signed char row);
extern int is_striking_position(unsigned char action, unsigned int frame);

void right_collision(void);
void left_collision(void);
int check_collision_col(signed char col);
int check_collision_tile(unsigned char cell);
int AdjustRoom(unsigned char kind, int value);
void apply_collision(int displacement, signed char direction);
void apply_air_bump(void);
void apply_ground_bump(signed char direction);
void BumpSound(void);
int CreviceBarr(void);
int gate_blocks_passage(void);
int scythe_blocks_passage(void);
extern int step_in_progress;

void *memset(void *, int, unsigned int);
#pragma intrinsic(memset)
#pragma pack(1)
typedef struct {
    signed char index, flipped;                       /* 00-01 */
    int x, y, image;                                   /* 02,04,06 */
    unsigned char image_set;                          /* 08 */
    int clip_top, clip_left, clip_bottom, clip_right;    /* 09,0b,0d,0f */
    int depth;                                        /* 11 */
    unsigned char object_type;                        /* 13 */
} FCHARREC;
#pragma pack()
extern FCHARREC FChar;
#pragma pack(1)
typedef struct {
    int image, equipment_image;                       /* 00,02 */
    signed char x, y;                                 /* 04-05 */
    unsigned char flags;                              /* 06 */
} FRAMEREC;
#pragma pack()
extern FRAMEREC Frame;


void CheckBarr(void)
{
    signed char col;

    right_barrier_col = left_barrier_col = -1;
    if ((unsigned char)Char.action == 7 || Char.sequence == 78 || Char.sequence == 70)
        goto weightless;
    if (Level.background_overlays[5] && slide_rock_on_row(Char.room, Char.row) && Char.sequence_group == 4)
        goto weightless;

    current_row = Char.row;
    InitColDetBufs();
    previous_row = current_row;
    if (get_char_col(right_edge) + 4 <= 11)
        last_col = get_char_col(right_edge) + 4;
    else
        last_col = 11;
    first_col = get_char_col(left_edge) - 2;
    GetColDetData(current_row, current_rooms, current_edges);
    GetColDetData(current_row + 1, below_rooms, below_edges);
    GetColDetData(current_row - 1, above_rooms, above_edges);
    if (Level.background_overlays[5]) {
        slide_rock_collision_row(current_row, current_rooms, current_edges);
        slide_rock_collision_row(current_row + 1, below_rooms, below_edges);
        slide_rock_collision_row(current_row - 1, above_rooms, above_edges);
    }
    col = 9;
    do {
        if (current_rooms[col] != 255 && current_rooms[col] == prev_rooms[col]) {
            if (!(prev_edges[col] & 0x0f) && (current_edges[col] & 0x0f))
                right_barrier_col = col;
            if (!(prev_edges[col] & 0xf0) && (current_edges[col] & 0xf0))
                left_barrier_col = col;
        }
    } while (--col >= 0);

weightless:
    if (Char.character_kind == 0 && right_barrier_col == -1 && left_barrier_col == -1 &&
        ledge_check && (unsigned char)Char.action == 4)
        CheckWeightlessBarr();
}

void CheckWeightlessBarr(void)
{
    if ((int)((signed char)63 * Char.row) - 4 > Char.y - ImHeight &&
        !IS_SIX_KIND(ReadCell(Char.room, Char.column, Char.row - 1))) {
        Char.x = AddCharX(-4);
        Char.horizontal_step = 0;
        JumpSeq(229);
    }
}

void InitColDetBufs(void)
{
    struct collision_row *rooms, *edges;

    if (current_row == previous_row || (current_row - previous_row) == -3 || (current_row - previous_row) == 3) {
        rooms = (struct collision_row *)current_rooms;
        edges = (struct collision_row *)current_edges;
    } else if ((current_row - previous_row) == -1 || (current_row - previous_row) == 2) {
        rooms = (struct collision_row *)above_rooms;
        edges = (struct collision_row *)above_edges;
    } else {
        rooms = (struct collision_row *)below_rooms;
        edges = (struct collision_row *)below_edges;
    }
    *(struct collision_row *)prev_rooms = *rooms;
    *(struct collision_row *)prev_edges = *edges;
    memset(current_rooms, 255, sizeof(current_rooms));
    memset(above_rooms, 255, sizeof(above_rooms));
    memset(below_rooms, 255, sizeof(below_rooms));
}

int GetBarrCode(unsigned char cell)
{
    int code;

    switch (cell) {
    case 2:
        if (cell_flags < 5 && step_in_progress) {
            code = 3;
            break;
        }
        goto solid;
    case 4:
        if (Level.environment_kind == 3)
            code = 5;
        else if (KIND_SAME_ROW == 9 && ((unsigned char)Level.level_number) == 8)
            code = 8;
        else
            code = 1;
        break;
    case 7:
        if (ImHeight <= 18) {
            code = 2;
            break;
        }
        goto solid;
    case 12:
        code = 6;
        break;
    case 20:
    case 25:
solid:
        code = 4;
        break;
    default:
        code = 0;
        break;
    }
    return code;
}

void GetColDetData(signed char row, unsigned char *rooms, unsigned char *edges)
{
    signed char col;
    int left, right;
    unsigned char flags;

    collision_x = ColEdgeX[first_col + 5] + 14;
    col = first_col;
    if (col <= last_col) do {
        left = GetLeftBarr(Char.room, col, row);
        right = GetRightBarr(Char.room, col, row);
        flags = right_edge > left ? 0x0f : 0;
        if (right > left_edge)
            flags |= 0xf0;
        edges[Cell.column] = flags;
        rooms[Cell.column] = Cell.room;
        collision_x += 32;
    } while (++col <= last_col);
}

int GetLeftBarr(unsigned char room, signed char col, signed char row)
{
    int code = GetBarrCode(ReadCell(room, col, row));
    int edge;

    if (code)
        edge = BarL[code] + collision_x;
    else
        edge = ColEdgeX[19];
    return edge;
}

int GetRightBarr(unsigned char room, signed char col, signed char row)
{
    int code = GetBarrCode(ReadCell(room, col, row));
    int edge;

    if (code)
        edge = collision_x - BarR[code] + 31;
    else
        edge = 0;
    return edge;
}

void collide_with_barriers(void)
{
    if ((unsigned char)Char.action != 2 && (unsigned char)Char.action != 6 && (Char.frame < 135 || Char.frame >= 145)) {
        if (right_barrier_col >= 0)
            left_collision();
        else if (left_barrier_col >= 0)
            right_collision();
    }
}

void right_collision(void)
{
    int barrier;

    if ((Char.stance == 1 || Char.direction != 0) && check_collision_col(left_barrier_col)) {
        barrier = GetRightBarr(Cell.room, Cell.column, ((unsigned char)Cell.row));
        if (barrier <= 0 && Level.background_overlays[5])
            barrier = slide_rock_barrier(0, Cell.room, ((unsigned char)Cell.row));
        barrier -= left_edge;
        apply_collision(barrier, 0);
    }
}

void left_collision(void)
{
    int barrier;

    if ((Char.stance == 1 || Char.direction != -1) && check_collision_col(right_barrier_col)) {
        barrier = GetLeftBarr(Cell.room, Cell.column, ((unsigned char)Cell.row));
        if (ColEdgeX[19] <= barrier && Level.background_overlays[5])
            barrier = slide_rock_barrier(-1, Cell.room, ((unsigned char)Cell.row));
        barrier -= right_edge;
        apply_collision(barrier, -1);
    }
}

int check_collision_col(signed char col)
{
    signed char row = Char.row;

    if (row < 0)
        row += 3;
    if (row >= 3)
        row -= 3;
    return check_collision_tile(ReadCell(current_rooms[col], col, row));
}

int check_collision_tile(unsigned char cell)
{
    int blocked;

    switch (cell) {
    case 4:
        blocked = gate_blocks_passage();
        break;
    case 7:
        blocked = CreviceBarr();
        break;
    case 10:
        blocked = 0;
        break;
    case 12:
        blocked = scythe_blocks_passage();
        break;
    default:
        blocked = 1;
        break;
    }
    if (blocked && Char.character_kind == 11)
        blocked = 0;
    if (blocked)
        collision_x = AdjustRoom(Cell.room, ColEdgeX[Cell.column + 5]) + 14;
    return blocked;
}


int far pascal AdjustRoom(unsigned char kind, int value)
{
    /* The first matching kind leaves the signed offset unchanged. */
    if (KIND_SAME_ROW == kind) {
        /* unchanged */
    } else if (KIND_ROW_ABOVE == kind || Room.below_left == kind) {
        value -= 320;
    } else if (KIND_ROW_BELOW == kind || Room.below_right == kind) {
        value += 320;
    }
    return value;
}

void apply_collision(int displacement, signed char direction)
{
    unsigned char cell;

    if (Char.alive >= 0 || Char.frame == 242 || Char.frame == 243)
        return;
    Char.x += displacement;
    cell = ((unsigned char)Cell.kind);
    if (direction == -1) {
        if (IsWall(cell))
            cell = ReadCell(Cell.room, --Cell.column, ((unsigned char)Cell.row));
    } else if (IsWall(cell)) {
        ++Cell.column;
        if (Cell.room == 0 && Cell.column == 10) {
            Cell.room = Char.room;
            Cell.column = 0;
        }
        cell = ReadCell(Cell.room, Cell.column, ((unsigned char)Cell.row));
    }
    if (IS_SIX_KIND(cell) && (Level.environment_kind != 2 || !probe_chain_floor(8)))
        apply_air_bump();
    else
        apply_ground_bump(direction);
}

void apply_air_bump(void)
{
    Char.x = AddCharX(-8);
    if ((unsigned char)Char.action == 4)
        Char.horizontal_step = 0;
    else {
        JumpSeq(45);
        AnimChar();
    }
    BumpSound();
    reread_cells();
}


void apply_ground_bump(signed char direction)
{
    int sequence;
    int boundary;
    unsigned int frame;

    if (Char.stance != 1 && (signed char)63 * Char.row - Char.y + 56 >= 15) {
        apply_air_bump();
        return;
    }
    reset_char_y();
    if (Char.vertical_step >= 22) {
        Char.x = AddCharX(-10), reread_cells();
        return;
    }
    Char.vertical_step = 0;
    if (Char.alive == 0)
        return;
    if (Char.stance == 1) {
        boundary = probe_jump_anchor(Char.room, Char.row);
        if (direction == Char.direction) {
            if (Char.sequence_id == 55 && Char.frame == 207)
                Char.x = AddCharX(10);
            Char.x = AddCharX(2), sequence = 65;
        } else {
            sequence = 64;
            if (is_striking_position(Char.character_kind, Char.frame)) {
                if (((unsigned char)Cell.kind) == 4 && cell_flags > 64)
                    sequence = -1;
                else
                    Char.x = AddCharX(16);
            }
        }
        if (boundary != 0 && boundary > Char.x) {
            sequence = 93, Char.stance = 0;
        }
    } else {
        frame = Char.frame;
        if (frame >= 246 && frame <= 261) {
            Char.x = AddCharX(-3), sequence = 121, Char.field_0f = 0;
        } else if (frame == 24 || frame == 25 ||
                   (frame >= 40 && frame < 43) || (frame >= 102 && frame < 107)) {
            sequence = 46;
        } else {
            sequence = 47;
        }
    }
    if (sequence != -1) {
        JumpSeq(sequence);
        AnimChar();
        reread_cells();
    }
    if (sequence != 64)
        BumpSound();
    else if (++sequence != 0 && Char.sequence != 56)
        AddSound(12);
}

void BumpSound(void)
{
    if (Char.character_kind != 1)
        bump_pending = 1;
}

void InitCollisionDetection(void)
{
    memset(prev_rooms, 255, sizeof(prev_rooms));
    memset(current_rooms, 255, sizeof(current_rooms));
    memset(below_rooms, 255, sizeof(below_rooms));
    memset(above_rooms, 255, sizeof(above_rooms));
    previous_row = -1;
}


int CreviceBarr(void)
{
    if (Char.character_kind == 7 || Char.character_kind == 8 || ImHeight > 18 || (cell_flags & 3) == 3)
        return 1;
    return 0;
}


int gate_blocks_passage(void)
{
    int height, clearance;
    int result;

    if (Level.environment_kind == 1) {
        if ((signed char)(Cell.modifier & 31) < 19)
            result = 1;
        else
            result = 0;
    } else if (Char.character_kind == 1) {
        result = 0;
    } else {
        height = Char.bounds.bottom - Char.bounds.top;
        if (Char.character_kind == 7)
            height += 26;
        else if (Char.character_kind == 8)
            height += 19;
        clearance = ((Cell.modifier & 252) >> 2) + 6;
        result = height + 7 <= clearance ? 0 : 1;
    }
    return result;
}

extern unsigned char ReadCell(unsigned char room, signed char col, signed char row);
extern unsigned char get_cells_ahead(signed char direction);
extern unsigned char get_behind(void);
extern int AddCharX(int distance);

int gate_blocks_standing(void)
{
    int position = 0;
    unsigned int opening;

    if ((unsigned char)Char.room == 9 && ((unsigned char)Level.level_number) == 8)
        goto done;
    if (ReadCell(Char.room, Char.column, Char.row) == 4 ||
        get_cells_ahead(1) == 4 || get_behind() == 4) {
        opening = Cell.modifier & 255;
        position = AddCharX(-6) - 14;
        if (opening != 0 && opening < 200 && ColEdgeX[Cell.column + 5] + 12 <= position &&
            ColEdgeX[Cell.column + 1 + 5] + 4 >= position)
            position = 1;
        else
            position = 0;
    }
done:
    return position;
}

extern int ScytheBladeExtended(unsigned int flags);

int scythe_blocks_passage(void)
{
    int blocked;

    if (Char.character_kind != 1 && Char.character_kind != 7 && Char.character_kind != 8 && Char.character_kind != 11 &&
        ImHeight > 25 && ScytheBladeExtended(Cell.modifier)) {
        blocked = 1;
    } else {
        blocked = 0;
    }
    return blocked;
}

extern int sword_table_id;
struct sprite_size {
    int height, width;
};
struct sword_frame {
    int offset;
    signed char width;
    unsigned char flags;
};

extern struct char_bounds NullRect;
extern int sprite_tables[], sprite_width, LeftEdge, sprite_right, sprite_top;
extern void far *get_set_image_size(int table, int index, struct sprite_size far *size);
extern int get_sword_width(void);
extern signed char get_col(int x);
extern signed char GetRow(int y);
extern void get_frame_info(void);
extern struct sword_frame far *RESOLVE_HANDLE_TO_FAR_PAYLOAD(int id);

void get_char_edges(void)
{
    struct char_bounds previous, bounds;
    struct sprite_size size;
    int sword_width;

    previous = Char.bounds;
    Char.bounds = NullRect;
    if (FChar.image == -1 || !get_set_image_size(sprite_tables[FChar.image_set], FChar.image, &size))
        return;
    sprite_width = size.width + 1;
    ImHeight = size.height;
    sword_width = get_sword_width();
    LeftEdge = FChar.x + 130;
    LeftEdge -= Char.direction == 0 ? sprite_width : sword_width;
    left_edge = LeftEdge;
    sprite_width += sword_width;
    right_edge = sprite_right = LeftEdge + sprite_width;
    sprite_top = FChar.y - ImHeight + 1;
    if (sprite_top >= 192)
        sprite_top = 0;
    bounds.top = sprite_top;
    bounds.bottom = FChar.y + 1;
    if (Char.direction == -1) {
        bounds.left = FChar.x;
        bounds.right = FChar.x + size.width;
    } else {
        bounds.left = FChar.x - size.width;
        bounds.right = FChar.x;
    }
    Char.bounds = bounds;
    Cell.top_row = GetRow(sprite_top);
    Cell.bottom_row = GetRow(FChar.y);
    if (Cell.bottom_row == 255)
        Cell.bottom_row = 3;
    if (get_col(LeftEdge) < 0)
        (*(unsigned char *)&Cell.first_col) = 0;
    else
        (*(unsigned char *)&Cell.first_col) = get_col(LeftEdge);
    if (get_col(sprite_right) <= 9)
        (*(unsigned char *)&Cell.last_col) = get_col(sprite_right);
    else
        (*(unsigned char *)&Cell.last_col) = 9;
    if (Frame.flags & 32) {
        left_edge += 9;
        right_edge -= 9;
    }
}

int get_sword_width(void)
{
    int width = 0;
    struct sword_frame far *frames;

    if (Char.stance == 1 && Char.character_kind != 7 && Char.character_kind != 8) {
        get_frame_info();
        if (Frame.equipment_image != width) {
            frames = RESOLVE_HANDLE_TO_FAR_PAYLOAD(sword_table_id);
            width = frames[Frame.equipment_image].width;
            width = width - 2 < 0 ? 0 : width - 2;
        }
    }
    return width;
}

extern int barrier_distance_from_face(unsigned char cell, unsigned char room, signed char col);
extern int barrier_distance_from_back(unsigned char cell, unsigned char room, signed char col);
extern unsigned char get_under_foot(void);
extern void setup_char(void);
extern void reread_base_cell(void);
extern signed char facing_col_step[];
extern unsigned char forward_barrier_kind;
extern int get_edge_distance(void);
extern int IsImpassable(unsigned char cell);
extern int abs(int);
#pragma intrinsic(abs)

int get_forward_distance(void)
{
    unsigned char cell;
    int distance;

    reread_base_cell();
    setup_char();
    get_char_edges();
    cell = get_under_foot();
    if (GetBarrCode(cell)) {
        distance = barrier_distance_from_face(cell, Char.room, Char.column);
        if (distance >= 0)
            goto barrier;
    }
    if (cell == 24)
        goto facing_left;
    cell = get_cells_ahead(1);
    if (GetBarrCode(cell)) {
        distance = barrier_distance_from_face(cell, Char.room, Char.column + facing_col_step[Char.direction]);
        if (distance >= 0)
            goto barrier;
    }
    if ((IS_SIX_KIND(cell) || IsImpassable(cell) || cell == 30) && cell != 24) {
        forward_barrier_kind = 0;
        distance = get_edge_distance();
        goto done;
    }
    if (cell == 6 || cell == 22 || cell == 2 || cell == 34)
        goto partial;
    if (cell != 24)
        goto fallback;
    goto facing_left;
partial:
    if ((distance = get_edge_distance()) == 0)
        goto fallback;
    --distance;
    forward_barrier_kind = 0;
    goto done;
facing_left:
    if (Char.direction == -1) {
        distance = get_edge_distance();
        if (Char.column == Cell.column)
            distance -= 15;
        else
            distance += 17;
        goto done;
    }
    goto fallback;
barrier:
    if (distance < 32) {
        forward_barrier_kind = 1;
        goto done;
    }
fallback:
    forward_barrier_kind = 2;
    distance = 28;
done:
    return distance;
}

extern void push_away_from_gate(void);
extern void SquishChar(void);

void check_gate_for_opponent(void)
{
    int left, right;

    unsigned char cell = get_under_foot();

    if (Char.alive >= 0)
        return;
    if (cell != 4 && get_behind() != 4 && get_cells_ahead(1) != 4)
        return;
    if ((Cell.room == 9 && ((unsigned char)Level.level_number) == 8) || Char.character_kind == 12 || !gate_blocks_passage())
        return;
    collision_x = ColEdgeX[Cell.column + 5] + 14;
    left = GetLeftBarr(Cell.room, Cell.column, ((unsigned char)Cell.row));
    right = GetRightBarr(Cell.room, Cell.column, ((unsigned char)Cell.row));
    if (Room.number != Cell.room) {
        if (Room.left == Cell.room) {
            left -= 320;
            right -= 320;
        } else if (Room.right == Cell.room) {
            left += 320;
            right += 320;
        } else {
            left = 999;
            right = 0;
        }
    }
    if (left < right_edge && right > left_edge)
        push_away_from_gate();
}

void check_gate_for_player(void)
{
    unsigned char cell = get_under_foot();

    if (Char.alive >= 0)
        return;
    if (Char.sequence == 118 || (cell != 4 && get_behind() != 4 && get_cells_ahead(1) != 4))
        return;
    if (Cell.room == 9 && ((unsigned char)Level.level_number) == 8)
        return;
    if ((current_edges[Cell.column] & prev_edges[Cell.column]) != 255 || !gate_blocks_passage())
        return;
    if (Char.frame != 109 && Cell.modifier != 0 && Char.stance != 1) {
        if (Char.sequence != 50)
            JumpSeq(50);
    } else if (Level.environment_kind == 3 && Cell.modifier != 0 && Char.stance != 1) {
        SquishChar();
    } else {
        BumpSound();
        push_away_from_gate();
    }
}

void push_away_from_gate(void)
{
    int face, back;

    if (barrier_distance_from_face(4, Cell.room, Cell.column) >= 0)
        face = barrier_distance_from_face(4, Cell.room, Cell.column);
    else
        face = -barrier_distance_from_face(4, Cell.room, Cell.column);
    if (barrier_distance_from_back(4, Cell.room, Cell.column) >= 0)
        back = barrier_distance_from_back(4, Cell.room, Cell.column);
    else
        back = -barrier_distance_from_back(4, Cell.room, Cell.column);
    Char.x = AddCharX(back > face ? -face : back);
}

extern int head_enemy_collide(int distance);

void collide_enemy_with_barrier(void)
{
    int back;
    int airborne, distance, front, sequence;
    unsigned char cell;

    airborne = Char.character_kind == 7 || Char.character_kind == 8;
    if ((signed char)(unsigned char)Char.action != ((Char.character_kind == 7 || Char.character_kind == 8) ? 0 : 1) &&
        ((Char.character_kind != 7 && Char.character_kind != 8) || Char.sequence == 164))
        return;
    if ((Char.alive >= 0 && !airborne) || Char.stance != 1 || Char.sequence == 70)
        return;
    cell = get_under_foot();
    if (!IsWall(cell) && (cell != 4 || !gate_blocks_passage())) {
        cell = get_cells_ahead(1);
        if (!IsWall(cell) && (cell != 4 || !gate_blocks_passage()))
            cell = 0;
    }
    if (!cell)
        return;
    setup_char();
    get_char_edges();
    if (!check_collision_tile(cell))
        return;
    sequence = -1;
    back = barrier_distance_from_back(((unsigned char)Cell.kind), Cell.room, Cell.column);
    front = barrier_distance_from_face(((unsigned char)Cell.kind), Cell.room, Cell.column);
    distance = abs(front) < abs(back) ? front : back;
    if (airborne)
        sequence = head_enemy_collide(front);
    if (sequence == -1 && distance < 5 && -sprite_width <= distance) {
        if (cell == 4 && distance > 0)
            distance = -distance;
        Char.x = AddCharX(-distance);
        if (distance >= 0 && cell == 4 && !airborne)
            sequence = 64;
        else if (!airborne || (Char.sequence != 145 && Char.sequence != 164 && Char.sequence_group != 1))
            sequence = (Char.character_kind == 7 || Char.character_kind == 8) ? 158 : 65;
    }
    if (sequence != -1) {
        JumpSeq(sequence);
        AnimChar();
        reread_cells();
    }
}

int barrier_distance_from_face(unsigned char cell, unsigned char room, signed char col)
{
    int code, distance;

    if (cell == 4 && !gate_blocks_passage())
        distance = -1;
    else {
        collision_x = AdjustRoom(room, ColEdgeX[col + 5] + 14);
        code = GetBarrCode(cell);
        if (!code)
            distance = -1;
        else if (Char.direction != 0)
            distance = BarR[code] - collision_x + left_edge - 31;
        else
            distance = BarL[code] - right_edge + collision_x;
    }
    return distance;
}

int barrier_distance_from_back(unsigned char cell, unsigned char room, signed char col)
{
    int code, distance;

    collision_x = AdjustRoom(room, ColEdgeX[col + 5] + 14);
    code = GetBarrCode(cell);
    if (!code)
        distance = 99;
    else if (Char.direction >= 0)
        distance = BarR[code] - collision_x + left_edge - 31;
    else
        distance = BarL[code] - right_edge + collision_x;
    return distance;
}

extern int Pause;

void reset_barrier_state(void)
{
    Pause = 0;
}
