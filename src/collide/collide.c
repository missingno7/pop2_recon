extern unsigned char resolved_room;
struct adjacent_rooms {
    unsigned char home, left, right;
};
#define KIND_SAME_ROW (collision_room_links.home)
#define KIND_ROW_ABOVE (collision_room_links.left)
#define KIND_ROW_BELOW (collision_room_links.right)

extern struct adjacent_rooms collision_room_links;

struct collision_row {
    unsigned char cell[10];
};

extern unsigned char current_rooms[10], prev_rooms[10], below_rooms[10], above_rooms[10];
extern unsigned char prev_edges[10], current_edges[10], below_edges[10], above_edges[10];
extern signed char current_row, first_col, last_col;
extern volatile signed char previous_row;
extern signed char left_barrier_col, right_barrier_col;
extern unsigned char char_kind, char_room, char_action, char_floor;
extern signed char char_row, char_col;
extern int char_sequence, char_motion, char_y, char_x;
extern int slide_rocks_active, ledge_check, right_edge, left_edge, floor_y;
extern int bump_pending;

extern int far _fastcall slide_rock_on_row(unsigned char room, signed char row);
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
extern int collision_x, empty_left_edge;
extern int col_positions[];
extern signed char left_offsets[], right_offsets[], resolved_col;
extern unsigned char level, stage;
extern signed char char_direction, char_alive;
extern unsigned char char_sword, resolved_row, cell_type;
extern unsigned int char_frame;
extern signed char char_fall;
extern unsigned char char_sword_frame;
extern unsigned int gate_opening;
#define cell_flags (*(unsigned char *)&gate_opening)

extern int slide_rock_barrier(signed char direction, unsigned char room, signed char row);
extern int probe_chain_floor(int cell);
extern int IsWall(unsigned char cell);
extern void AnimChar(void);
extern void reread_cells(void);
extern void update_floor_y(void);
extern void cue_sound(int);
extern int far _fastcall sword_room_boundary(unsigned char room, signed char row);
extern int is_sword_motion(unsigned char action, unsigned int frame);

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
extern int gate_closed;

void *memset(void *, int, unsigned int);
#pragma intrinsic(memset)
extern int sprite_index, sprite_x, sprite_y, sword_index, sword_table_id;


void CheckBarr(void)
{
    signed char col;

    right_barrier_col = left_barrier_col = -1;
    if (char_kind == 7 || char_sequence == 78)
        goto weightless;
    if (slide_rocks_active && slide_rock_on_row(char_room, char_row) && char_motion == 4)
        goto weightless;

    current_row = char_row;
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
    if (slide_rocks_active) {
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
    if (char_action == 0 && right_barrier_col == -1 && left_barrier_col == -1 &&
        ledge_check && char_kind == 4)
        CheckWeightlessBarr();
}

void CheckWeightlessBarr(void)
{
    if ((int)((signed char)63 * char_row) - 4 > char_y - floor_y &&
        !IS_SIX_KIND(ReadCell(char_room, char_col, char_row - 1))) {
        char_x = AddCharX(-4);
        char_floor = 0;
        JumpSeq(229);
    }
}

void InitColDetBufs(void)
{
    struct collision_row *rooms, *edges;
    volatile int delta;

    if (current_row == previous_row || (delta = current_row - previous_row) == -3 || delta == 3) {
        rooms = (struct collision_row *)current_rooms;
        edges = (struct collision_row *)current_edges;
    } else if (delta == -1 || delta == 2) {
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
        if (cell_flags < 5 && gate_closed) {
            code = 3;
            break;
        }
        goto solid;
    case 4:
        if (level == 3)
            code = 5;
        else if (KIND_SAME_ROW == 9 && stage == 8)
            code = 8;
        else
            code = 1;
        break;
    case 7:
        if (floor_y <= 18) {
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

    collision_x = col_positions[first_col] + 14;
    col = first_col;
    if (col <= last_col) do {
        left = GetLeftBarr(char_room, col, row);
        right = GetRightBarr(char_room, col, row);
        edges[resolved_col] = ((right_edge > left) * 15) | (-((left_edge < right) << 4));
        rooms[resolved_col] = resolved_room;
        collision_x += 32;
    } while (++col <= last_col);
}

int GetLeftBarr(unsigned char room, signed char col, signed char row)
{
    int code = GetBarrCode(ReadCell(room, col, row));
    int edge;

    if (code)
        edge = left_offsets[code] + collision_x;
    else
        edge = empty_left_edge;
    return edge;
}

int GetRightBarr(unsigned char room, signed char col, signed char row)
{
    int code = GetBarrCode(ReadCell(room, col, row));
    int edge;

    if (code)
        edge = collision_x - right_offsets[code] + 31;
    else
        edge = 0;
    return edge;
}

void collide_with_barriers(void)
{
    if (char_kind != 2 && char_kind != 6 && (char_frame < 135 || char_frame >= 145)) {
        if (right_barrier_col >= 0)
            left_collision();
        else if (left_barrier_col >= 0)
            right_collision();
    }
}

void right_collision(void)
{
    int barrier;

    if ((char_sword == 1 || char_direction != 0) && check_collision_col(left_barrier_col)) {
        barrier = GetRightBarr(resolved_room, resolved_col, resolved_row);
        if (barrier <= 0 && slide_rocks_active)
            barrier = slide_rock_barrier(0, resolved_room, resolved_row);
        barrier -= left_edge;
        apply_collision(barrier, 0);
    }
}

void left_collision(void)
{
    int barrier;

    if ((char_sword == 1 || char_direction != -1) && check_collision_col(right_barrier_col)) {
        barrier = GetLeftBarr(resolved_room, resolved_col, resolved_row);
        if (empty_left_edge <= barrier && slide_rocks_active)
            barrier = slide_rock_barrier(-1, resolved_room, resolved_row);
        barrier -= right_edge;
        apply_collision(barrier, -1);
    }
}

int check_collision_col(signed char col)
{
    signed char row = char_row;

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
    if (blocked && char_action == 11)
        blocked = 0;
    if (blocked)
        collision_x = AdjustRoom(resolved_room, col_positions[resolved_col]) + 14;
    return blocked;
}

extern volatile unsigned char KIND_ROW_ABOVE_ALT;
extern volatile unsigned char KIND_ROW_BELOW_ALT;

int far pascal AdjustRoom(unsigned char kind, int value)
{
    /* The first matching kind leaves the signed offset unchanged. */
    if (KIND_SAME_ROW == kind) {
        /* unchanged */
    } else if (KIND_ROW_ABOVE == kind || KIND_ROW_ABOVE_ALT == kind) {
        value -= 320;
    } else if (KIND_ROW_BELOW == kind || KIND_ROW_BELOW_ALT == kind) {
        value += 320;
    }
    return value;
}

void apply_collision(int displacement, signed char direction)
{
    unsigned char cell;

    if (char_alive >= 0 || char_frame == 242 || char_frame == 243)
        return;
    char_x += displacement;
    cell = cell_type;
    if (direction == -1) {
        if (IsWall(cell))
            cell = ReadCell(resolved_room, --resolved_col, resolved_row);
    } else if (IsWall(cell)) {
        ++resolved_col;
        if (resolved_room == 0 && resolved_col == 10) {
            resolved_room = char_room;
            resolved_col = 0;
        }
        cell = ReadCell(resolved_room, resolved_col, resolved_row);
    }
    if (IS_SIX_KIND(cell) && (level != 2 || !probe_chain_floor(8)))
        apply_air_bump();
    else
        apply_ground_bump(direction);
}

void apply_air_bump(void)
{
    char_x = AddCharX(-8);
    if (char_kind == 4)
        char_floor = 0;
    else {
        JumpSeq(45);
        AnimChar();
    }
    BumpSound();
}

extern int sequence_id;

void apply_ground_bump(signed char direction)
{
    int sequence;
    int boundary;
    unsigned int frame;

    if (char_sword != 1 && (signed char)63 * char_row - char_y + 56 >= 15) {
        apply_air_bump();
        return;
    }
    update_floor_y();
    if (char_fall >= 22) {
        char_x = AddCharX(-10), reread_cells();
        return;
    }
    char_fall = 0;
    if (char_alive == 0)
        return;
    if (char_sword == 1) {
        boundary = sword_room_boundary(char_room, char_row);
        if (!(char_direction ^ direction)) {
            if (sequence_id == 55 && char_frame == 207)
                char_x = AddCharX(10);
            char_x = AddCharX(2), sequence = 65;
        } else {
            sequence = 64;
            if (is_sword_motion(char_action, char_frame)) {
                if (cell_type == 4 && cell_flags > 64)
                    sequence = -1;
                else
                    char_x = AddCharX(16);
            }
        }
        if (boundary != 0 && boundary > char_x) {
            sequence = 93, char_sword = 0;
        }
    } else {
        frame = char_frame;
        if (frame >= 246 && frame <= 261) {
            char_x = AddCharX(-3), sequence = 121, char_sword_frame = 0;
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
    else if (++sequence != 0 && char_sequence != 56)
        cue_sound(12);
}

void BumpSound(void)
{
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
    if (char_action == 7 || char_action == 8 || floor_y > 18 || (cell_flags & 3) == 3)
        return 1;
    return 0;
}

extern int char_top, char_bottom;

int gate_blocks_passage(void)
{
    int height, clearance, result;

    if (level != 1)
        goto general;
    if ((signed char)(cell_flags & 31) >= 19)
        goto passable;
blocked:
    result = 1;
    goto finished;
general:
    if (char_action == 1)
        goto passable;
    height = char_bottom - char_top;
    if (char_action == 7)
        height += 26;
    else if (char_action == 8)
        height += 19;
    clearance = ((gate_opening & 252) >> 2) + 6;
    if (clearance < height + 7)
        goto blocked;
passable:
    result = 0;
finished:
    return result;
}

extern unsigned char char_room, stage;
extern signed char char_col, char_row, resolved_col;
extern int col_positions[];
extern unsigned char ReadCell(unsigned char room, signed char col, signed char row);
extern unsigned char get_cells_ahead(signed char direction);
extern unsigned char get_behind(void);
extern int AddCharX(int distance);

int gate_blocks_standing(void)
{
    int position = 0;
    unsigned int opening;

    if (char_room == 9 && stage == 8)
        goto done;
    if (ReadCell(char_room, char_col, char_row) == 4 ||
        get_cells_ahead(1) == 4 || get_behind() == 4) {
        opening = gate_opening & 255;
        position = AddCharX(-6) - 14;
        if (opening != 0 && opening < 200 && col_positions[resolved_col] + 12 <= position &&
            col_positions[resolved_col + 1] + 4 >= position)
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

    if (char_action != 1 && char_action != 7 && char_action != 8 && char_action != 11 &&
        floor_y > 25 && ScytheBladeExtended(gate_opening)) {
        blocked = 1;
    } else {
        blocked = 0;
    }
    return blocked;
}

struct collision_box {
    int top, left, bottom, right;
};
struct sprite_size {
    int height, width;
};
struct sword_frame {
    int offset;
    signed char width;
    unsigned char flags;
};

extern struct collision_box char_box, empty_box;
extern unsigned char sprite_bank, top_collision_row, bottom_collision_row;
extern unsigned char collision_first_col, collision_last_col, collision_flags;
extern int sprite_tables[], sprite_width, sprite_left, sprite_right, sprite_top;
extern void far *get_set_image_size(int table, int index, struct sprite_size far *size);
extern int get_sword_width(void);
extern signed char get_col(int x);
extern signed char GetRow(int y);
extern void get_frame_info(void);
extern struct sword_frame far *RESOLVE_HANDLE_TO_FAR_PAYLOAD(int id);

void get_char_edges(void)
{
    struct collision_box previous, bounds;
    struct sprite_size size;
    int sword_width;

    previous = char_box;
    char_box = empty_box;
    if (sprite_index == -1 || !get_set_image_size(sprite_tables[sprite_bank], sprite_index, &size))
        return;
    sprite_width = size.width + 1;
    floor_y = size.height;
    sword_width = get_sword_width();
    sprite_left = sprite_x + 130;
    sprite_left -= char_direction == 0 ? sprite_width : sword_width;
    left_edge = sprite_left;
    sprite_width += sword_width;
    right_edge = sprite_right = sprite_left + sprite_width;
    sprite_top = sprite_y - floor_y + 1;
    if (sprite_top >= 192)
        sprite_top = 0;
    bounds.top = sprite_top;
    bounds.bottom = sprite_y + 1;
    if (char_direction == -1) {
        bounds.left = sprite_x;
        bounds.right = sprite_x + size.width;
    } else {
        bounds.left = sprite_x - size.width;
        bounds.right = sprite_x;
    }
    char_box = bounds;
    top_collision_row = GetRow(sprite_top);
    bottom_collision_row = GetRow(sprite_y);
    if (bottom_collision_row == 255)
        bottom_collision_row = 3;
    if (get_col(sprite_left) < 0)
        collision_first_col = 0;
    else
        collision_first_col = get_col(sprite_left);
    if (get_col(sprite_right) <= 9)
        collision_last_col = get_col(sprite_right);
    else
        collision_last_col = 9;
    if (collision_flags & 32) {
        left_edge += 9;
        right_edge -= 9;
    }
}

int get_sword_width(void)
{
    int width = 0;
    struct sword_frame far *frames;

    if (char_sword == 1 && char_action != 7 && char_action != 8) {
        get_frame_info();
        if (sword_index != width) {
            frames = RESOLVE_HANDLE_TO_FAR_PAYLOAD(sword_table_id);
            width = frames[sword_index].width;
            width = width - 2 < 0 ? 0 : width - 2;
        }
    }
    return width;
}

extern int barrier_distance_from_face(unsigned char cell, unsigned char room, signed char col);
extern int barrier_distance_from_back(unsigned char cell, unsigned char room, signed char col);
extern unsigned char get_under_foot(void);
extern void select_character_frame(void);
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
    select_character_frame();
    get_char_edges();
    cell = get_under_foot();
    if (GetBarrCode(cell)) {
        distance = barrier_distance_from_face(cell, char_room, char_col);
        if (distance >= 0)
            goto barrier;
    }
    if (cell == 24)
        goto facing_left;
    cell = get_cells_ahead(1);
    if (GetBarrCode(cell)) {
        distance = barrier_distance_from_face(cell, char_room, char_col + facing_col_step[char_direction]);
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
    if (char_direction == -1) {
        distance = get_edge_distance();
        if (char_col == resolved_col)
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

    if (get_under_foot() != 4 && get_behind() != 4 && get_cells_ahead(1) != 4)
        return;
    if ((resolved_room == 9 && stage == 8) || char_action == 12 || !gate_blocks_passage())
        return;
    collision_x = col_positions[resolved_col] + 14;
    left = GetLeftBarr(resolved_room, resolved_col, resolved_row);
    right = GetRightBarr(resolved_room, resolved_col, resolved_row);
    if (collision_room_links.home != resolved_room) {
        if (collision_room_links.left == resolved_room) {
            left -= 320;
            right -= 320;
        } else if (collision_room_links.right == resolved_room) {
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

    if (char_sequence == 118 || (cell != 4 && get_behind() != 4 && get_cells_ahead(1) != 4))
        return;
    if (resolved_room == 9 && stage == 8)
        return;
    if ((current_edges[resolved_col] & prev_edges[resolved_col]) != 255 || !gate_blocks_passage())
        return;
    if (char_frame != 109 && gate_opening != 0 && char_sword != 1) {
        if (char_sequence != 50)
            JumpSeq(50);
    } else if (level == 3 && gate_opening != 0 && char_sword != 1) {
        SquishChar();
    } else {
        BumpSound();
        push_away_from_gate();
    }
}

void push_away_from_gate(void)
{
    int face, back;

    if (barrier_distance_from_face(4, resolved_room, resolved_col) >= 0)
        face = barrier_distance_from_face(4, resolved_room, resolved_col);
    else
        face = -barrier_distance_from_face(4, resolved_room, resolved_col);
    if (barrier_distance_from_back(4, resolved_room, resolved_col) >= 0)
        back = barrier_distance_from_back(4, resolved_room, resolved_col);
    else
        back = -barrier_distance_from_back(4, resolved_room, resolved_col);
    char_x = AddCharX(back > face ? -face : back);
}

extern int head_enemy_collide(int distance);

void collide_enemy_with_barrier(void)
{
    int back;
    int airborne, distance, front, sequence;
    unsigned char cell;

    airborne = char_action == 7 || char_action == 8;
    if ((signed char)char_kind != ((char_action == 7 || char_action == 8) ? 0 : 1) &&
        ((char_action != 7 && char_action != 8) || char_sequence == 164))
        return;
    if ((char_alive >= 0 && !airborne) || char_sword != 1)
        return;
    cell = get_under_foot();
    if (!IsWall(cell) && (cell != 4 || !gate_blocks_passage())) {
        cell = get_cells_ahead(1);
        if (!IsWall(cell) && (cell != 4 || !gate_blocks_passage()))
            cell = 0;
    }
    if (!cell)
        return;
    select_character_frame();
    get_char_edges();
    if (!check_collision_tile(cell))
        return;
    sequence = -1;
    back = barrier_distance_from_back(cell_type, resolved_room, resolved_col);
    front = barrier_distance_from_face(cell_type, resolved_room, resolved_col);
    distance = abs(front) < abs(back) ? front : back;
    if (airborne)
        sequence = head_enemy_collide(front);
    if (sequence == -1 && distance < 5 && -sprite_width <= distance) {
        if (cell == 4 && distance > 0)
            distance = -distance;
        char_x = AddCharX(-distance);
        if (distance >= 0 && cell == 4 && !airborne)
            sequence = 64;
        else if (!airborne || (char_sequence != 145 && char_sequence != 164 && char_motion != 1))
            sequence = (char_action == 7 || char_action == 8) ? 158 : 65;
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
        collision_x = AdjustRoom(room, col_positions[col] + 14);
        code = GetBarrCode(cell);
        if (!code)
            distance = -1;
        else if (char_direction != 0)
            distance = right_offsets[code] - collision_x + left_edge - 31;
        else
            distance = left_offsets[code] - right_edge + collision_x;
    }
    return distance;
}

int barrier_distance_from_back(unsigned char cell, unsigned char room, signed char col)
{
    int code, distance;

    collision_x = AdjustRoom(room, col_positions[col] + 14);
    code = GetBarrCode(cell);
    if (!code)
        distance = 99;
    else if (char_direction >= 0)
        distance = right_offsets[code] - collision_x + left_edge - 31;
    else
        distance = left_offsets[code] - right_edge + collision_x;
    return distance;
}

extern int barrier_state;

void reset_barrier_state(void)
{
    barrier_state = 0;
}
