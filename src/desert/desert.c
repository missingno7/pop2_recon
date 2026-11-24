/* DOS DESERT.C, overlay 5. Definitions follow DOS code order. */
typedef signed char sbyte;
typedef unsigned char ubyte;
typedef unsigned int uword;

#pragma pack(1)
typedef struct {
    int left, top, right, bottom;
} Rect;

typedef struct {
    sbyte shape;
    int x, y;
    sbyte pass;
    ubyte reserved[3];
    uword handle;
    Rect rect;
    ubyte tail[6];
} Piece;

/* Animated pieces use the high byte of y and the pass byte as a state word. */
typedef struct {
    ubyte reserved[4];
    uword flags;
} PackedPieceState;

#define piece_state(piece) (((PackedPieceState *)(piece))->flags)

typedef struct {
    sbyte count;
    ubyte template_id;
    int shape_base;
    ubyte header[24];
    Piece pieces[1];
} DesertRoom;

typedef struct {
    ubyte reserved[16];
    Rect bounds;
} Peel;

typedef struct {
    Peel *peel;
    int reserved;
    int active;
} PeelRecord;
#pragma pack()

extern ubyte active_sound_channels;
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

static sbyte current_column, previous_column;
extern CHARREC Char;
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
extern uword BlueSpec;
typedef struct {
    int control_mode, fade_active, playing_nis, recording_mode, skip_opening, field_0a;
    int plateau_loaded, throne_song_added, throne_nis_played, escher_room_eight_seen, nis_room_restored;
} GAMESTATE;
extern GAMESTATE GameState;
extern unsigned long FrameNum;
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
extern struct {
    unsigned char enabled;
    signed char cue;
    int song, delay, reserved[2];
} ambient_state;
extern CHARREC Kid;
extern int NextLevel, pending_effect;
typedef struct {
    signed char forward, back, up, down, action;
} Directions;
extern Directions Dir;
static Rect stone_bounds = {49, 11, 70, 54};
static sbyte desert_variant = -1;
extern DesertRoom *custom_room;
extern uword *Pics[5];
extern Rect CellClipRect, GamePlayRect;
extern int ImWidth;
typedef struct {
    signed char tile;
    unsigned char room;
    signed char state;
    unsigned char field_3;
} TrobRecord;
extern TrobRecord Trob;
extern PeelRecord Peels[];

extern void AddSong(int), AddSound(int);
extern void CycleColors(int, int, int);
extern int query_sound(int);
extern void dispatch_sound(int);
extern int get_trob_by_id(sbyte, sbyte);
extern void copy_kid_to_char(void), reset_char_y(void);
extern void JumpSeq(int);
extern int Rnd(int);
extern void far *FETCH_LOCKED_SHAPE_LIST(uword);
extern void CLEAR_HANDLE_OWNED(uword), DISPOSE_HANDLE(uword);
extern uword HGETRSRC(unsigned long, int);
extern uword PrinceUnpackShape(uword, int, void far *);
extern void far *RESOLVE_HANDLE_TO_FAR_PAYLOAD(uword);
extern Rect far *MAKESHAPERECT(Rect far *, int, int, void far *);
extern void mark_move(sbyte, Rect *);
extern void mark_bufs_for_rect(void (far *)(sbyte, Rect *), Rect *, sbyte);
extern void far _fastcall draw_custom_room_image(sbyte, sbyte);
extern int get_trob_room_cell(sbyte, int);
extern void add_trob(sbyte, sbyte, sbyte, sbyte);
extern int check_trob_visibility(void);
extern sbyte RestoreKeepCustomPeel(ubyte);
extern void DrawCustomRoomImgAtInfo(ubyte, Piece *);
extern void OFFSETRECT(Rect far *, Rect far *, int, int);
extern void far _fastcall add_peel(int, int, int, int, ubyte, ubyte);
extern int compute_trob_move_rect(Rect *, Rect *);
extern sbyte check_trob_right(void), check_trob_below(void);
extern int find_character_peel(ubyte, int);

void check_desert_exit(void);
void cycle_desert_colors(void);
void watch_desert_kid(void);
int is_selected_code(int);
uword is_desert_exit_open(void);
void trigger_desert_exit(sbyte, sbyte);
void animate_desert_stone(void);
void redraw_stone(void);
void remove_last_stone_peel(sbyte);
int sink_desert_stone(sbyte);
int trigger_stone_at(ubyte, sbyte, sbyte);

void add_desert_ambient(void)
{
    if (active_sound_channels & 2)
        AddSong(251);
}

void clip_desert_character(void)
{
    FChar.clip_bottom = FChar.clip_bottom > 126 ? 126 : FChar.clip_bottom;
    if (FChar.clip_bottom < FChar.clip_top)
        FChar.clip_top = FChar.clip_bottom;
}

void check_desert_exit(void)
{
    uword flags = Level.cell_info[0][10].modifier;
    uword frame;
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
                desert_variant = (sbyte)Rnd(2);
                current_column = (sbyte)Rnd(4) + 1;
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
    ubyte index;
    Piece *piece;
    Rect *bounds;

    if (Char.sequence == 27) {
        saved_clip = CellClipRect;
        index = (ubyte)(FrameNum & 1) + 3;
        piece = &custom_room->pieces[index];
        piece->x = 119;
        piece->y = FChar.x;
        if (!((ubyte)FChar.flipped))
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
    uword frame;
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
    piece->rect.top -= 2;
    bounds = &piece->rect;
    mark_move(Trob.tile, bounds);
    mark_move(Trob.tile + 1, bounds);
    mark_move(Trob.tile - 10, bounds);
    mark_move(Trob.tile - 9, bounds);
    RestoreKeepCustomPeel(piece->shape);
}

void draw_desert_exit(Piece *piece)
{
    int frame;

    if (piece->shape == 11) {
        frame = piece_state(piece) & 31;
        custom_room->pieces[2].y = (4 - frame) * 2;
        custom_room->pieces[15].y = 27 - frame * 2;
        custom_room->pieces[15].rect.top = 0;
        draw_custom_room_image(2, 11);
        draw_custom_room_image(15, 11);
    }
}

uword is_desert_exit_open(void)
{
    return (ubyte)Level.cell_info[0][10].modifier & 31;
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
    uword frame;

    if (check_trob_visibility()) {
        frame = (ubyte)BlueSpec & 15;
        if (frame == 7)
            frame = 0;
        else
            ++frame;
        BlueSpec &= ~15;
        BlueSpec |= frame;
        mark_move(Trob.tile, &custom_room->pieces[1].rect);
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
    uword frame;

    if (check_trob_visibility()) {
        frame = (ubyte)BlueSpec & 15;
        if (frame == 7)
            frame = 0;
        else
            ++frame;
        BlueSpec &= ~15;
        BlueSpec |= frame;
        mark_move(Trob.tile, &custom_room->pieces[1].rect);
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
    ubyte id;

    id = piece->shape + 100;
    y = 126;
    x = 64;
    count = 6;
    do {
        OFFSETRECT(&bounds, &piece->rect, x, y);
        add_peel(bounds.top, bounds.bottom, bounds.left,
                 bounds.right - bounds.left, id, 3);
        x += 32;
    } while (--count);
}

void animate_desert_stone(void)
{
    uword delay = (BlueSpec & 0x1f0) >> 4;
    uword frame;

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
    uword frame;

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
        mark_move(Trob.tile, &bounds);
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
            bounds = &Peels[slot].peel->bounds;
            if (bounds->top == x || bounds->bottom == x) {
                Peels[slot].active = 0;
                if (++matches == 2)
                    slot = -1;
            } else if (bounds->top < x && bounds->bottom > x &&
                       bounds->left == 114 && bounds->right == 127) {
                Peels[slot].active = 0;
                slot = -1;
            }
        }
        ++occurrence;
    } while (slot + 1);
}

int sink_desert_stone(sbyte cell)
{
    int started;
    uword flags;

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


int trigger_stone_at(ubyte room, sbyte cell, sbyte action)
{
    uword original = Level.cell_info[room - 1][cell].modifier;
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
