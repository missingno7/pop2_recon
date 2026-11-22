/* MOVESPIK.C, resident DOS frame 101f.
 * External names describe DOS uses. Structure members outside the accessed
 * fields are placeholders for the observed offsets and strides.
 */
typedef signed char SByte;
typedef unsigned char Byte;
typedef unsigned int Word;

#pragma pack(1)
typedef union SpikeState {
    Word word;
    int signed_word; /* Signed view of the same packed state word. */
    struct {
        Byte phase;
        Byte flags;
    } byte;
} SpikeState;

typedef struct TileState {
    SpikeState spikes;
    Word other;
} TileState;

typedef struct Mobile {
    Byte prefix[4];
    Byte room;
    Byte middle[5];
    SByte row;
    SpikeState spikes;
} Mobile;
#pragma pack()

#pragma pack(1)
typedef struct MobileState {
    int x;
    int y;
    Byte room;
    int slot;
    int delay;
    Byte kind;
    SByte row;
    Word spikes_state;
} MobileState;
#pragma pack()

/* The mobile scratch record is copied as a packed 13-byte contribution. */
extern MobileState mobile_state;
#define mob_x mobile_state.x
#define mob_y mobile_state.y
#define mob_room mobile_state.room
#define mob_slot mobile_state.slot
#define mob_delay mobile_state.delay
#define mob_kind mobile_state.kind
#define mob_row mobile_state.row
#define mob_spikes_state mobile_state.spikes_state
struct RoomNeighbors {
    Byte current;
    Byte right;
};
extern struct RoomNeighbors rooms;
#define current_room rooms.current
#define right_room rooms.right

extern SByte actor_direction, actor_column, actor_row;
extern int actor_x, actor_y;
extern Byte actor_action, actor_room, actor_alive, actor_facing, actor_stance;
extern Word actor_frame, actor_sequence;
extern Byte level;

extern SByte tile_column, tile_row;
extern Byte tile_room;
extern SpikeState tile_state;
#define tile_flags tile_state.byte.flags
#define tile_object tile_state.byte.phase
#define tile_state_word tile_state.word
#pragma pack(1)
extern struct {
    Byte column;
    Byte flags;
    int y, x;
    SpikeState spikes;
    Byte tail[12];
} render_cell;
#pragma pack()
extern int render_context;
extern int screen_edge;
extern int side_offset;
extern const volatile int column_x[];
static const int spike_images_a[] = {0, 116, 117, 118, 119, 120, 117, 116, 120};
static const int spike_images_b[] = {0, 121, 122, 123, 124, 125, 122, 121, 126};
extern TileState room_state[];
extern TileState *current_room_state;

extern Byte row_to_render_column(SByte row);
extern void add_mobile_sprite(int y);
extern int immune_frame(Word frame);
extern Byte get_tile(Byte room, SByte column, SByte row);
extern int impale_override(void);
extern SByte tile_to_actor_column(void);
extern int actor_x_adjustment(void);
extern void add_mobile(void);
extern Mobile *get_mobile(Byte kind, int index);
extern void impale_other_actor(void);
extern void select_sequence(Word sequence);
extern void play_sound(int sound);
extern void set_actor_health(Byte amount);
extern void advance_actor(void);
extern void snap_actor_to_floor(void);
extern void set_room_state(Byte room);
/* The sprite entry consumes kind, image and context in AX, DX and BX. */
extern void _fastcall draw_spike_sprite(Byte kind, int image, int context, int x, Byte height);

int GetSpikes(void);
Mobile *GetSpikeMobRoomRow(Byte room, SByte row);
void DoImpale(void);
void TriggerSpikes(Byte room, int index);

/* ATMSpikes */
void ATMSpikes(void)
{
    SByte column;

    column = mob_x / 32;
    if (mob_room != current_room) {
        if (mob_room == right_room && column == 10) {
            mob_room = current_room;
            column = 0;
            mob_x -= 320;
        } else {
            mob_slot = -1;
        }
    }
    if (mob_slot != -1) {
        render_cell.column = row_to_render_column(mob_row) + column;
        add_mobile_sprite(mob_y);
        mob_kind = 5;
        mob_x -= 4;
        mob_y += 7;
        add_mobile_sprite(mob_y);
        mob_kind = 4;
        mob_x += 4;
        mob_y -= 7;
    }
}

/* CheckImpale */
void CheckImpale(void)
{
    SByte column;
    volatile Mobile *mob;
    int x;
    SByte last_column;

    if (actor_sequence == 0x55 || actor_action == 4 || actor_action == 1
        || immune_frame(actor_frame))
        return;
    column = actor_column - 1;
    if (column <= actor_column) {
        do {
            if (get_tile(actor_room, column, actor_row) == 2 && GetSpikes() == 2) {
                x = column_x[column] + 14;
                if (actor_stance == 0)
                    x += 14;
                else
                    x += 10;
                if ((x > screen_edge
                        && actor_row * 63 + 26 <= actor_y
                        && (actor_frame < 0x79 || actor_frame > 0x84)
                        && actor_frame != 0x0f)
                    || (level == 2 && impale_override())) {
                    DoImpale();
                    mob = GetSpikeMobRoomRow(actor_room, actor_row);
                    if (mob) {
                        mob->spikes.byte.phase = 0;
                        mob->spikes.byte.phase |= 0xf0;
                        room_state[tile_room * 30 + tile_column].spikes.word = mob->spikes.word;
                    }
                }
            }
            last_column = actor_column;
            ++column;
        } while (column <= last_column);
    }
}

/* CheckSpikes */
void CheckSpikes(void)
{
    int x;

    if (actor_column >= 0 && actor_action != 4
        && get_tile(actor_room, actor_column - 1, actor_row) == 2
        && actor_row * 63 + 18 <= actor_y) {
        x = column_x[tile_to_actor_column()] + 14;
        x += actor_stance == 0 ? 16 : 2;
        if (actor_direction == 0) {
            x += side_offset;
            x -= actor_x_adjustment();
        }
        if (x >= actor_x)
            TriggerSpikes(tile_room, tile_column);
    }
}

/* CheckSpikesJammed */
void CheckSpikesJammed(Byte room, int index)
{
    union { SByte value; Byte raw; Word storage; } row;
    Word state;
    int zero;

    if (GetSpikeMobRoomRow(room, row.value = index / 10) == 0) {
        state = room_state[room * 30 + index].spikes.word;
        if ((Byte)state == 0xf0) {
            mob_x = (index % 10) * 32 + 46;
            /* Narrow the stored byte before multiplying. */
            mob_y = (row.value = (SByte)row.raw) * 63 + 47;
            mob_row = row.value;
            mob_room = room;
            zero = 0;
            mob_delay = zero;
            mob_slot = zero;
            mob_kind = 4;
            mob_spikes_state = state;
            add_mobile();
        }
    }
}

/* DoImpale */
void DoImpale(void)
{
    volatile SpikeState *state;
    Mobile *mob;
    Word sequence;

    state = &room_state[tile_room * 30 + tile_column].spikes;
    if (actor_action != 4) {
        Byte flags = tile_flags & 8;
        state->word = 0xf0 + ((Word)flags << 8);
    } else {
        state->signed_word = 0x8f;
    }

    mob = get_mobile(4, 1);
    if (mob && mob->room == tile_room && mob->row == tile_row)
        mob->spikes.word = state->word;

    snap_actor_to_floor();
    actor_alive = 0;
    if (actor_action == 0 || actor_action == 2) {
        actor_facing = 1;
        actor_x = column_x[tile_to_actor_column()] + 27;
        if (actor_direction == 0) {
            sequence = actor_action == 0 ? 0x36 : 0xbd;
            actor_direction = -1;
        } else {
            sequence = actor_action == 0 ? 0x33 : 0xbc;
        }
        select_sequence(sequence);
        play_sound(77);
        set_actor_health(100);
    } else if (actor_action != 4) {
        return;
    } else {
        impale_other_actor();
    }
    advance_actor();
}

/* DrawObjSpikes */
void DrawObjSpikes(Byte object)
{
    int image;

    if (render_cell.spikes.word == 0xf0)
        render_cell.spikes.word = 8;
    else if (render_cell.spikes.byte.phase & 0x80)
        render_cell.spikes.word = 5;
    else
        render_cell.spikes.byte.flags = 0;

    if (object == 0x84)
        image = spike_images_a[render_cell.spikes.word];
    else if (object == 0x85)
        image = spike_images_b[render_cell.spikes.word];
    else
        return;

    if (image)
        draw_spike_sprite(4, image, render_context, render_cell.x, 10);
}

/* GetSpikeIdx */
int GetSpikeIdx(Word state)
{
    int index;
    if (state & 0x80) {
        index = 5;
    } else {
        state &= 7;
        index = state;
    }
    return index;
}

/* GetSpikeMobRoomRow */
Mobile *GetSpikeMobRoomRow(Byte room, SByte row)
{
    int found = 0;
    int index = 1;
    Mobile *mob;

    do {
        mob = get_mobile(4, index++);
        if (!mob || (mob->room == room && mob->row == row))
            found = 1;
    } while (!found);
    return mob;
}

/* GetSpikes */
int GetSpikes(void)
{
    int state;
    int result;

    state = tile_object;
    if (state && state != 0xf0) {
        if (state & 0x80)
            result = 1;
        else if (state < 5)
            result = 2;
        else
            result = 0;
    } else {
        result = 0;
    }
    return result;
}

/* MobSpikes */
void MobSpikes(void)
{
    Word state;
    TileState *room;

    get_tile(mob_room, mob_x / 32 - 1, mob_row);
    state = tile_object;
    if (state != 0xf0) {
        if (state & 0x80) {
            --*(Byte *)&state; /* Decrement the low byte without changing the flags. */
            if (!(state & 0x7f)) {
                state = 6;
                play_sound(79);
            }
        } else {
            ++state;
            if (state == 5)
                state = 0x8f;
            else if (state == 8) {
                state = 0;
                mob_slot = -1;
            }
        }
        set_room_state(tile_room);
        room = current_room_state;
        room[tile_column].spikes.word = (state += (tile_flags << 8) & 0x800);
        mob_spikes_state = state;
    }
}

/* TriggerSpikes */
void TriggerSpikes(Byte room, int index)
{
    register Word next;
    SpikeState *spikes;
    Word state;
    SByte row;
    Mobile *mob;
    int zero;

    spikes = &current_room_state[index].spikes;
    /* Taking the addresses of the pointer locals affects MSC allocation. */
    state = (*(&spikes))->byte.phase;
    if (state == 0 || (state & 0x80)) {
        if (state == 0) {
            mob_x = (index % 10) * 32 + 46;
            row = index / 10;
            mob_y = row * 63 + 47;
            mob_row = row;
            mob_room = room;
            zero = 0;
            mob_delay = zero;
            mob_slot = zero;
            mob_kind = 4;
            next = spikes->word + 1;
            spikes->word = next;
            mob_spikes_state = next;
            add_mobile();
            play_sound(78);
        } else if (state != 0xf0) {
            next = (*(volatile Word *)&tile_state_word & 0x800) + 0x8f;
            spikes->word = next;
            mob = GetSpikeMobRoomRow(actor_room, actor_row);
            if (mob)
                (*(&mob))->spikes.word = next;
        }
    }
}
