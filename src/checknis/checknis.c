/* CHECKNIS.C reconstruction. External aliases describe the observed uses. */
#include "types.h"
#include "level.h"
#include "char.h"
#include "game.h"

signed char quote_index = -1;
extern int IsTimeForNIS(int, int);
extern int play_nis(int);

extern GAMESTATE GameState;

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
    GameState.nis_room_restored = result == 2 ? 1 : 0;
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
extern LEVELREC Level;
extern unsigned long hRebirthSaved;
extern unsigned int Resume;

extern CHARREC Kid;

extern unsigned char CutRoom;

int far pascal IsLevelStartWithNISScreen(void)
{
    int result = 0;
    if (hRebirthSaved == 0L && Resume == 0 && GameState.nis_room_restored == 0) {
        if ((Level.entry_room == 27 && Level.level_number == 6) ||
            (Level.entry_room == 22 && Level.level_number == 10) ||
            (Level.entry_room == 1 && Level.level_number == 14) ||
            (CutRoom == 9 && Level.level_number == 8))
            result = 1;
    } else if (hRebirthSaved != 0L && Resume == 0 && GameState.nis_room_restored == 0 &&
               Kid.room == 9 && Level.level_number == 8) {
        result = 1;
    }
    return result;
}

extern signed char StartLevel;
extern struct { int done; } notice_state;
extern unsigned int Minutes;
extern int frame_diagnostics;
extern int level_resource_files[];
extern char *find_option(const char far *);
extern int cdecl atoi(const char *);
extern void open_verified_resource(int);
extern void close_resource_file(int);
extern void play_sound(int sound);

int IsTimeForNIS(int level, int previous_level)
{
    int sequence = 0;
    struct {
        char *arg;
        signed char quote;
    } options;

    if (StartLevel > 2 && !notice_state.done) {
        sequence = 100;
    } else if (StartLevel) {
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
                options.quote = (1 - (int)Minutes) / 9 + 7;
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

extern int BPort;
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

    GameState.playing_nis = result;
    room = Level.environment_kind;
    if (sequence != 100) {
        unload_level(0);
        if (sequence < 20 && sequence != 6)
            RemoveRebirthPt();
        if (BPort != 0) {
            DeallocBackPort(BPort);
            BPort = 0;
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
    GameState.playing_nis = 0;
    return result;
}

extern signed char LevelIdx;
extern ROOMREC Room;
extern int NextLevel, Graf;
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
    NextLevel = room;
    saved_sound = BPort;
    BPort = Graf;
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
    BPort = saved_sound;
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
