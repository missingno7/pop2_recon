/* CHECKNIS.C reconstruction. External aliases describe the observed uses. */

signed char quote_index = -1;
extern int IsTimeForNIS(int level, int previous_level);
extern int play_nis(int sequence);
extern int nis_room_restored;

int CheckPlayNIS(int level, int previous_level)
{
    int sequence;
    int result;

    sequence = IsTimeForNIS(level, previous_level);
    if (sequence) {
        result = play_nis(sequence);
    } else {
        result = 0;
    }
    nis_room_restored = result == 2 ? 1 : 0;
    return result;
}

int EraseScreenAfterNIS(int x)
{
    int result;

    if (x == 1 || x == 100 || x == 2 || x == 3 || x == 6)
        result = 0;
    else
        result = 1;

    return result;
}

int EraseScreenBeforeNIS(int sequence)
{
    int pre_screen_sequence;
    int result;

    /* DOS tests an uninitialized stack word for the case-4 exception. */
    if (sequence == 1 || sequence == 100 || pre_screen_sequence == 4 || sequence == 6)
        result = 0;
    else
        result = 1;
    return result;
}

/* Field aliases, original long type, signedness, and purpose are hypotheses. */
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
extern unsigned long hRebirthSaved;
extern unsigned int resume_mode;
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

extern CHARREC Kid;

extern signed char CutRoom;

int far pascal IsLevelStartWithNISScreen(void)
{
    int result = 0;
    if (hRebirthSaved == 0L && resume_mode == 0 && nis_room_restored == 0) {
        if ((Level.entry_room == 27 && Level.level_number == 6) ||
            (Level.entry_room == 22 && Level.level_number == 10) ||
            (Level.entry_room == 1 && Level.level_number == 14) ||
            (CutRoom == 9 && Level.level_number == 8))
            result = 1;
    } else if (hRebirthSaved != 0L && resume_mode == 0 && nis_room_restored == 0 &&
               Kid.room == 9 && Level.level_number == 8) {
        result = 1;
    }
    return result;
}

extern signed char game_request;
extern int intro_played;
extern unsigned int remaining_minutes;
extern int frame_diagnostics;
extern int level_resource_files[];
extern char *find_option(const char far *option);
extern int cdecl atoi(const char *value);
extern void open_verified_resource(int sound);
extern void close_resource_file(int sound);
extern void play_sound(int sound);

int IsTimeForNIS(int level, int previous_level)
{
    int sequence = 0;
    struct {
        char *arg;
        signed char quote;
    } options;

    if (game_request > 2 && !intro_played) {
        sequence = 100;
    } else if (game_request) {
        if (level == 0 || (previous_level != level && previous_level != -1)) {
            switch (level) {
            case 1:
                sequence = 9;
                break;
            case 2: sequence = 100; break;
            case 3: sequence = 10; break;
            case 5: sequence = 1; break;
            case 9: sequence = 2; break;
            case 13: sequence = 3; break;
            }
        }
        if (sequence == 0 && level >= 4) {
            if (quote_index == -1 && level >= 4) {
                sequence = 20;
                quote_index = 0;
            } else {
                options.quote = (1 - (int)remaining_minutes) / 9 + 7;
                if (options.quote > quote_index) {
                    sequence = options.quote + 20;
                    quote_index = options.quote;
                }
            }
        }
    }

    if (frame_diagnostics) {
        if ((options.arg = find_option("NIS")) != 0) {
            sequence = atoi(options.arg + 3);
            if (sequence == 1) {
                open_verified_resource(level_resource_files[3]);
                play_sound(10034);
            } else if (sequence == 5 || sequence == 11) {
                close_resource_file(level_resource_files[6]);
                open_verified_resource(level_resource_files[6]);
            } else {
                open_verified_resource(level_resource_files[4]);
            }
        } else if ((options.arg = find_option("TREE")) != 0) {
            quote_index = atoi(options.arg + 4);
            sequence = quote_index + 19;
        }
    }
    return sequence;
}

extern int playing_nis, pending_sound;
extern char ScreenPortRect[];
extern void unload_level(int mode);
extern void RemoveRebirthPt(void);
extern void DeallocBackPort(int sound);
extern void WRITE_DAC_BLOCK(int first, int count, const unsigned char far *colors, int sync);
extern void STROKE_PORT_RECT(const char far *rect);
extern void show_dialog_notice(void);
extern void Tree_Check(int quote);
extern int Transition(int sequence);
extern int Dispatch_NIS(int sequence);
extern void set_default_palette(void);
extern void set_kid_palette(unsigned char room);


int play_nis(int sequence)
{
    int result = 1;
    unsigned char room;

    playing_nis = result;
    room = Level.environment_kind;
    if (sequence != 100) {
        unload_level(0);
        if (sequence < 20 && sequence != 6)
            RemoveRebirthPt();
        if (pending_sound != 0) {
            DeallocBackPort(pending_sound);
            pending_sound = 0;
        }
    }
    if (EraseScreenBeforeNIS(sequence)) {
        WRITE_DAC_BLOCK(0, 256, (const unsigned char far *)0, 1);
        STROKE_PORT_RECT(ScreenPortRect);
    }
    if (sequence == 100) {
        show_dialog_notice();
    } else if (sequence >= 20) {
        Tree_Check(sequence - 19);
    } else if (sequence >= 1 && sequence <= 6) {
        if (Transition(sequence))
            result = 2;
        switch (sequence) {
        case 1: room = 4; break;
        case 2: room = 2; break;
        case 3: room = 6; break;
        }
    } else {
        if (Dispatch_NIS(sequence))
            result = 2;
    }
    if (EraseScreenAfterNIS(sequence))
        STROKE_PORT_RECT(ScreenPortRect);
    set_default_palette();
    set_kid_palette(room);
    playing_nis = 0;
    return result;
}

extern signed char LevelIdx;
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
extern int room_to_draw_word, Graf;
extern void reset_level_overlays(void);
extern void load_level_map(signed char room);
extern void load_background_set(int visible);
extern void load_pieces_data(void);
extern void load_code_overlays(unsigned char room, unsigned char tile);
extern void set_level_spec_ptrs(unsigned char room);
extern void PrepCut(int mode);
extern void clean_cut(void);
extern void unload_set(int pass);
extern void unload_pieces_data(void);
extern void nis_noop(void);

void draw_room_from_nis(signed char room, signed char level)
{
    unsigned char old_room;
    int saved_sound;

    reset_level_overlays();
    old_room = Level.environment_kind;
    LevelIdx = room;
    room_to_draw_word = room;
    saved_sound = pending_sound;
    pending_sound = Graf;
    load_level_map(room);
    open_verified_resource(level_resource_files[Level.environment_kind]);
    load_background_set(!IsLevelStartWithNISScreen());
    if (Level.environment_kind != old_room)
        load_pieces_data();
    load_code_overlays(Level.environment_kind, Level.preferred_opponent);
    set_level_spec_ptrs(Level.environment_kind);
    Room.number = 0;
    CutRoom = level;
    PrepCut(1);
    clean_cut();
    unload_set(4);
    unload_set(1);
    unload_pieces_data();
    close_resource_file(level_resource_files[Level.environment_kind]);
    Level.environment_kind = 0;
    pending_sound = saved_sound;
    nis_noop();
}

typedef struct {
    int first;
    int second;
    int third;
    unsigned char pixels[1];
} image_header;


extern unsigned int hGetSL(int resource_id, int mode);
extern int hNewShpItem1(int asset, int frame, int type);
extern image_header far *RESOLVE_HANDLE_TO_FAR_PAYLOAD(int image);
extern void MIRROR4BITBLOCK(unsigned char far *pixels, int first, int second, int third);
extern void PAINT_INDEXED_SHAPE(image_header far *image, int x, int y, int mode, int plane);
extern void DISPOSE_HANDLE(int image);

extern void set_palette(int id, unsigned int size, unsigned char type, unsigned char bank);

void draw_dead_kid(int install_palette)
{
    unsigned int shapes;
    int frame, x;
    image_header far *image;

    shapes = hGetSL(25001, 2);
    frame = hNewShpItem1(shapes, 80, 2);
    if (frame) {
        x = Kid.x - 137;
        image = RESOLVE_HANDLE_TO_FAR_PAYLOAD(frame);
        if (Kid.direction == 0) {
            MIRROR4BITBLOCK(image->pixels, image->first, image->second, image->third);
            x -= 19;
        }
        PAINT_INDEXED_SHAPE(image, x, Kid.y - 10, 10, 2);
        DISPOSE_HANDLE(frame);
        DISPOSE_HANDLE(shapes);
    }

    shapes = hGetSL(1000, 0x8000);
    frame = hNewShpItem1(shapes, 11, 2);
    if (frame) {
        PAINT_INDEXED_SHAPE(RESOLVE_HANDLE_TO_FAR_PAYLOAD(frame), 170, 181, 10, 0x8000);
        DISPOSE_HANDLE(frame);
        DISPOSE_HANDLE(shapes);
    }
    if (install_palette)
        set_palette(1000, 240, 16, 0);
}

int step_in_progress = 0;
