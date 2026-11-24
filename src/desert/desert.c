/* DOS DESERT.C, overlay 5. Definitions follow DOS code order. */
#include "types.h"
#include "level.h"
#include "char.h"
#include "game.h"

#pragma pack(1)

typedef struct {
    sbyte shape;
    int x, y;
    sbyte pass;
    byte reserved[3];
    word handle;
    Rect rect;
    byte tail[6];
} Piece;

/* Animated pieces use the high byte of y and the pass byte as a state word. */
typedef struct {
    byte reserved[4];
    word flags;
} PackedPieceState;

#define piece_state(piece) (((PackedPieceState *)(piece))->flags)

typedef struct {
    sbyte count;
    byte template_id;
    int shape_base;
    byte header[24];
    Piece pieces[1];
} DesertRoom;

typedef struct {
    byte reserved[16];
    Rect bounds;
} Peel;

typedef struct {
    Peel *peel;
    int reserved;
    int active;
} PeelRecord;
#pragma pack()

extern struct { unsigned char channels, volume; } Midi;
extern FCHARREC FChar;

static sbyte current_column, previous_column;
extern CHARREC Char;
extern LEVELREC Level;
extern word BlueSpec;

extern GAMESTATE GameState;
extern unsigned long FrameNum;
extern ROOMREC Room;
extern AmbientState ambient_state;
extern CHARREC Kid;
extern int NextLevel, pending_effect;

extern Directions Dir;
static Rect stone_bounds = {49, 11, 70, 54};
static sbyte desert_variant = -1;
extern DesertRoom *custom_room;
extern word *Pics[5];
extern Rect CellClipRect, GamePlayRect;
extern int ImWidth;
extern TROBREC Trob;
extern struct {
    int count;
    PeelRecord entries[40];
} PeelInfo;

extern void AddSong(int), AddSound(int);
extern void CycleColors(int, int, int);
extern int query_sound(int);
extern void dispatch_sound(int);
extern int get_trob_by_id(sbyte, sbyte);
extern void copy_kid_to_char(void), reset_char_y(void);
extern void JumpSeq(int);
extern int Rnd(int);
extern void far *FETCH_LOCKED_SHAPE_LIST(word);
extern void CLEAR_HANDLE_OWNED(word), DISPOSE_HANDLE(word);
extern word HGETRSRC(unsigned long, int);
extern word PrinceUnpackShape(word, int, void far *);
extern void far *RESOLVE_HANDLE_TO_FAR_PAYLOAD(word);
extern Rect far *MAKESHAPERECT(Rect far *, int, int, void far *);
extern void mark_move(sbyte, Rect *);
extern void mark_bufs_for_rect(void (far *)(sbyte, Rect *), Rect *, sbyte);
extern void far _fastcall draw_custom_room_image(sbyte, sbyte);
extern int get_trob_room_cell(sbyte, int);
extern void add_trob(sbyte, sbyte, sbyte, sbyte);
extern int check_trob_visibility(void);
extern sbyte RestoreKeepCustomPeel(byte);
extern void DrawCustomRoomImgAtInfo(byte, Piece *);
extern void OFFSETRECT(Rect far *, Rect far *, int, int);
extern void far _fastcall add_peel(int, int, int, int, byte, byte);
extern int compute_trob_move_rect(Rect *, Rect *);
extern sbyte check_trob_right(void), check_trob_below(void);
extern int find_character_peel(byte, int);

void check_desert_exit(void);
void cycle_desert_colors(void);
void watch_desert_kid(void);
int is_selected_code(int);
word is_desert_exit_open(void);
void trigger_desert_exit(sbyte, sbyte);
void animate_desert_stone(void);
void redraw_stone(void);
void remove_last_stone_peel(sbyte);
int sink_desert_stone(sbyte);
int trigger_stone_at(byte, sbyte, sbyte);

void add_desert_ambient(void)
{
    if (Midi.channels & 2)
        AddSong(251);
}

void clip_desert_character(void)
{
    FChar.clip.bottom = FChar.clip.bottom > 126 ? 126 : FChar.clip.bottom;
    if (FChar.clip.bottom < FChar.clip.top)
        FChar.clip.top = FChar.clip.bottom;
}

void check_desert_exit(void)
{
    word flags = Level.cell_info[0][10].modifier;
    word frame;
    int keep_open;
    sbyte cell;

    if ((flags & 31) != 0)
        return;
    frame = (flags >> 8) & 31;
    keep_open = current_column - previous_column != -2; cell = 0;
    if (keep_open) {
        while (cell < 6) {
            int state = Level.cell_info[0][cell + 12].modifier & 15;
            if ((state && current_column != cell) ||
                (!state && current_column == cell && frame < 10))
                keep_open = 0;
            ++cell;
            if (!keep_open)
                break;
        }
    }

    if (keep_open) {
        if (frame >= 20) {
            trigger_desert_exit(1, 10);
        } else {
            if (frame == 6)
                AddSong(37);
            else if (frame == 9)
                AddSound(45);
            else if (frame == 10)
                sink_desert_stone(current_column + 12);
            ++frame;
            flags &= 0xe0ff;
            frame <<= 8;
            flags |= frame;
        }
    } else {
        flags &= 0xe0ff;
    }
    Level.cell_info[0][10].modifier = flags;
}

void cycle_desert_colors(void)
{
    if (!GameState.fade_active && Room.number != 1 && FrameNum % 3 == 0) {
        CycleColors(176, 13, 0);
        CycleColors(196, 4, 0);
        CycleColors(191, 5, 0);
    }
}

void desert_frame(void)
{
    cycle_desert_colors();
    watch_desert_kid();
    check_desert_exit();
    if (Room.number != 1) {
        if (!query_sound(10038))
            AddSound(38);
        if (query_sound(10251))
            dispatch_sound(10251);
    } else if (query_sound(10251)) {
        if (query_sound(10038))
            dispatch_sound(10038);
    } else if (Kid.sequence_group != 7 && Kid.alive < 0 &&
               !query_sound(ambient_state.song + 10000) &&
               !get_trob_by_id(30, 1) && !is_desert_exit_open()) {
        add_desert_ambient();
    }
}

void watch_desert_kid(void)
{
    copy_kid_to_char();
    if (Char.room == 1) {
        if (Char.column != previous_column) {
            if (is_selected_code(Char.frame)) {
                if (previous_column >= 2) {
                    if (previous_column <= 7) {
                        if (sink_desert_stone(previous_column + 10) &&
                            current_column - previous_column == -2)
                            AddSong(41);
                    }
                }
            }
        }
        if (is_selected_code(Char.frame))
            previous_column = Char.column;
        if (Char.x < 130) {
            if (Level.level_number == NextLevel) {
                ++NextLevel;
                JumpSeq(2);
            } else {
                JumpSeq(2);
                pending_effect = -1;
            }
        }
    } else if (Char.room == 3) {
        if (Char.x >= 489) {
            if (Dir.up || Dir.down || (!Char.direction && Dir.forward)) {
                JumpSeq(2);
                reset_char_y();
            }
            if (!query_sound(10046) && (Dir.action < 0 || !Rnd(20)))
                AddSound(46);
        }
    }
    Kid = Char;
}

void reset_desert_variant(void)
{
    desert_variant = -1;
}

int is_selected_code(int code)
{
    int selected;

    if (code == 15 || code == 109 || code == 7 || code == 46 ||
        (code >= 11 && code <= 14) || code == 38 || code == 44 || code == 26 ||
        code == 158)
        selected = 1;
    else if (code == 170 || code == 171)
        selected = 1;
    else
        selected = 0;
    return selected;
}

void LoadDsrtCode(void)
{
    Piece *piece;
    void far *archive;
    int resource;

    previous_column = -1;
    if (custom_room != 0) {
        if (custom_room->template_id == 6) {
            if (desert_variant == -1) {
                desert_variant = Rnd(2);
                current_column = Rnd(4) + 1;
            }
            if (desert_variant != 2) {
                archive = FETCH_LOCKED_SHAPE_LIST(*Pics[4]);
                piece = &custom_room->pieces[15];
                DISPOSE_HANDLE(piece->handle);
                resource = desert_variant + custom_room->shape_base + 15;
                piece->handle = HGETRSRC(0x53484150UL, resource);
                piece->handle = PrinceUnpackShape(piece->handle, 2, archive);
                piece = &custom_room->pieces[16];
                DISPOSE_HANDLE(piece->handle);
                piece->handle = HGETRSRC(0x53484150UL, resource + 3);
                piece->handle = PrinceUnpackShape(piece->handle, 2, archive);
                CLEAR_HANDLE_OWNED(*Pics[4]);
            }
            }
    }
}

void setup_sand(void)
{
    Rect saved_clip;
    byte index;
    Piece *piece;
    Rect *bounds;

    if (Char.sequence == 27) {
        saved_clip = CellClipRect;
        index = (byte)(FrameNum & 1) + 3;
        piece = &custom_room->pieces[index];
        piece->x = 119;
        piece->y = FChar.x;
        if (!(FChar.flipped))
            piece->y -= ImWidth;
        bounds = &piece->rect;
        MAKESHAPERECT(bounds, piece->x, piece->y, RESOLVE_HANDLE_TO_FAR_PAYLOAD(piece->handle));
        mark_bufs_for_rect(mark_move, bounds, -1);
        CellClipRect = *bounds;
        draw_custom_room_image(index, piece->pass = 1);
        piece->pass = 11;
        CellClipRect = saved_clip;
    }
}

void animate_desert_exit(void)
{
    word frame;
    Piece *piece;
    Rect *bounds;

    frame = BlueSpec & 31;
    if (frame == 20) {
        Trob.state = -1;
        dispatch_sound(10007);
    } else {
        ++frame;
        if (!query_sound(10045))
            AddSound(7);
    }
    BlueSpec &= ~31;
    BlueSpec |= frame;
    piece = &custom_room->pieces[2];
    piece->rect.left -= 2;
    bounds = &piece->rect;
    mark_move(Trob.cell, bounds);
    mark_move(Trob.cell + 1, bounds);
    mark_move(Trob.cell - 10, bounds);
    mark_move(Trob.cell - 9, bounds);
    RestoreKeepCustomPeel(piece->shape);
}

void draw_desert_exit(Piece *piece)
{
    int frame;

    if (piece->shape == 11) {
        frame = piece_state(piece) & 31;
        custom_room->pieces[2].y = (4 - frame) * 2;
        custom_room->pieces[15].y = 27 - frame * 2;
        custom_room->pieces[15].rect.left = 0;
        draw_custom_room_image(2, 11);
        draw_custom_room_image(15, 11);
    }
}

word is_desert_exit_open(void)
{
    return Level.cell_info[0][10].modifier & 31;
}

void trigger_desert_exit(sbyte room, sbyte cell)
{
    if (!get_trob_room_cell(room, cell)) {
        add_trob(room, cell, 1, 4);
        AddSong(39);
    }
}

void animate_desert_planks(void)
{
    word frame;

    if (check_trob_visibility()) {
        frame = BlueSpec & 15;
        if (frame == 7)
            frame = 0;
        else
            ++frame;
        BlueSpec &= ~15;
        BlueSpec |= frame;
        mark_move(Trob.cell, &custom_room->pieces[1].rect);
    }
}

void draw_desert_planks(Piece *piece)
{
    Rect saved_clip;
    sbyte index;

    if (piece->shape == 11) {
        index = ((piece_state(piece) >> 1) & 7) + 1;
        saved_clip = CellClipRect;
        CellClipRect = GamePlayRect;
        draw_custom_room_image(index, 11);
        CellClipRect = saved_clip;
    }
}

void animate_desert_rocks(void)
{
    word frame;

    if (check_trob_visibility()) {
        frame = BlueSpec & 15;
        if (frame == 7)
            frame = 0;
        else
            ++frame;
        BlueSpec &= ~15;
        BlueSpec |= frame;
        mark_move(Trob.cell, &custom_room->pieces[1].rect);
    }
}

void draw_desert_rocks(Piece *piece)
{
    Rect saved_clip;
    sbyte index;

    if (piece->shape == 11) {
        index = ((piece_state(piece) >> 1) & 7) + 1;
        saved_clip = CellClipRect;
        CellClipRect = GamePlayRect;
        draw_custom_room_image(index, 11);
        CellClipRect = saved_clip;
    }
}

void AddDsrtStoneUpPeels(Piece *piece)
{
    Rect bounds;
    int x, y, count;
    byte id;

    id = piece->shape + 100;
    y = 126;
    x = 64;
    count = 6;
    do {
        OFFSETRECT(&bounds, &piece->rect, x, y);
        add_peel(bounds.left, bounds.right, bounds.top,
                 bounds.bottom - bounds.top, id, 3);
        x += 32;
    } while (--count);
}

void animate_desert_stone(void)
{
    word delay = (BlueSpec & 0x1f0) >> 4;
    word frame;

    if (delay == 0) {
        frame = BlueSpec & 15;
        if (Trob.state == 3 && frame == 0)
            AddSound(42);

        if ((frame == 10 && Trob.state == 3) ||
            (frame == 0 && Trob.state == 1)) {
            if (Trob.state == 3 && !get_trob_by_id(30, 2))
                add_desert_ambient();
            Trob.state = -1;
        } else {
            BlueSpec &= ~15;
            BlueSpec |= frame + Trob.state - 2;
        }

        if (Trob.state > 0)
            redraw_stone();
    } else if (Kid.sequence != 27 && Kid.alive < 0) {
        BlueSpec &= ~0x1f0;
        BlueSpec |= --delay << 4;
    }
}

void draw_desert_stone(Piece *piece)
{
    word frame;

    if (piece->shape == 11 || piece->shape == 2) {
        frame = piece_state(piece) & 15;
        if (frame) {
            DrawCustomRoomImgAtInfo(frame + 4, piece);
            if (frame > 7 && (sbyte)piece->x - current_column == 2)
                DrawCustomRoomImgAtInfo(16, piece);
        }
    }
}

void redraw_stone(void)
{
    Rect bounds;

    if (compute_trob_move_rect(&bounds, &stone_bounds)) {
        mark_move(Trob.cell, &bounds);
        mark_move(check_trob_right(), &bounds);
        mark_move(check_trob_below(), &bounds);
    }
}

void remove_last_stone_peel(sbyte column)
{
    int x = (column + 1) << 5;
    sbyte matches = 0;
    int occurrence = 1;
    int slot;
    Rect far *bounds;

    do {
        slot = find_character_peel(114, occurrence);
        if (slot != -1) {
            bounds = &PeelInfo.entries[slot].peel->bounds;
            if (bounds->left == x || bounds->right == x) {
                PeelInfo.entries[slot].active = 0;
                if (++matches == 2)
                    slot = -1;
            } else if (bounds->left < x && bounds->right > x &&
                       bounds->top == 114 && bounds->bottom == 127) {
                PeelInfo.entries[slot].active = 0;
                slot = -1;
            }
        }
        ++occurrence;
    } while (slot + 1);
}

int sink_desert_stone(sbyte cell)
{
    int started;
    word flags;

    started = trigger_stone_at(1, cell, 1);
    if (started) {
        flags = BlueSpec;
        BlueSpec = Level.cell_info[0][cell].modifier;
        animate_desert_stone();
        Level.cell_info[0][cell].modifier = BlueSpec;
        BlueSpec = flags;
    }
    return started;
}

void trigger_all_stones_down(void)
{
    if (previous_column != -1 && previous_column >= 2 && previous_column <= 7)
        sink_desert_stone(previous_column + 10);
    sink_desert_stone(Char.column + 10);
}

int trigger_stone_at(byte room, sbyte cell, sbyte action)
{
    word original = Level.cell_info[room - 1][cell].modifier;
    int frame = original & 15;
    int started = 1;

    if (action == 3 && frame == 0) {
        original &= ~0x1f0;
        original |= (Rnd(10) + 21) << 4;
        Level.cell_info[room - 1][cell].modifier = original;
    } else if (action == 1) {
        if (frame) {
            AddSound(43);
        } else {
            started = get_trob_room_cell(room, cell) != 0;
            if (original & 0x1f0) {
                original &= ~0x1f0;
                Level.cell_info[room - 1][cell].modifier = original;
            }
        }
    } else {
        started = 0;
        if (action == 3 && (sbyte)(cell % 10) == 7 && !get_trob_by_id(30, 1))
            add_desert_ambient();
    }

    if (started) {
        add_trob(room, cell, action, 30);
        if (action == 1)
            remove_last_stone_peel((sbyte)(cell % 10));
    }
    return started;
}

typedef void (far pascal *LevelTileProc)(Piece near *);
LevelTileProc desert_level_spec[] = {
    0, 0, 0, 0, draw_desert_exit,
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, draw_desert_rocks, draw_desert_planks,
    draw_desert_stone
};
