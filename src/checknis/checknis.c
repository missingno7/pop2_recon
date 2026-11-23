/* CHECKNIS.C reconstruction. External aliases describe the observed uses. */
extern int IsTimeForNIS(int level, int previous_level);
extern int play_nis(int sequence);

int CheckPlayNIS(int level, int previous_level)
{
    int sequence;
    int result;

    sequence = IsTimeForNIS(level, previous_level);
    if (sequence) {
        int played = play_nis(sequence);
        result = played;
    } else {
        result = 0;
    }
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
extern unsigned long guard_pair;
extern unsigned int guard_other;
extern signed char mode_code, state_code, level_code;

int far pascal IsLevelStartWithNISScreen(void)
{
    int result = 0;
    if (guard_pair == 0L && guard_other == 0) {
        if ((mode_code == 27 && state_code == 6) ||
            (mode_code == 22 && state_code == 10) ||
            (mode_code == 1 && state_code == 14) ||
            (level_code == 9 && state_code == 8))
            result = 1;
    }
    return result;
}

extern signed char cinematics_enabled, quote_index;
extern int remaining_time, debug_options;
extern int level_resource_files[];
extern char *find_option(const char far *option);
extern int cdecl atoi(const char *value);
extern void open_verified_resource(int sound);
extern void close_resource_file(int sound);
extern void report_option(int message);

int IsTimeForNIS(int level, int previous_level)
{
    int sequence = 0;
    struct {
        char *arg;
        signed char quote;
    } options;

    if (cinematics_enabled) {
        if (level == 0 || (previous_level != level && previous_level != -1)) {
            switch (level) {
            case 1: sequence = 9; break;
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
                options.quote = (1 - remaining_time) / 9 + 7;
                if (options.quote > quote_index) {
                    sequence = options.quote + 20;
                    quote_index = options.quote;
                }
            }
        }
    }

    if (debug_options) {
        if ((options.arg = find_option("NIS")) != 0) {
            sequence = atoi(options.arg + 3);
            if (sequence == 1) {
                open_verified_resource(level_resource_files[3]);
                report_option(10034);
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
extern char screen_rect[];
extern void unload_level(int mode);
extern void RemoveRebirthPt(void);
extern void DeallocBackPort(int sound);
extern void WRITE_DAC_BLOCK(int first, int count, const unsigned char far *colors, int sync);
extern void STROKE_PORT_RECT(const char far *rect);
extern void play_intro(int mode);
extern void play_quote_vector(int quote);
extern int play_story_vector(int sequence);
extern int play_other_vector(int sequence);
extern void set_default_palette(void);
extern void set_kid_palette(unsigned char room);

extern unsigned char current_room;

int play_nis(int sequence)
{
    int result = 1;
    unsigned char room;

    playing_nis = result;
    room = current_room;
    if (sequence != 100) {
        unload_level(0);
        if (sequence < 20)
            RemoveRebirthPt();
        if (pending_sound != 0) {
            DeallocBackPort(pending_sound);
            pending_sound = 0;
        }
    }
    if (EraseScreenBeforeNIS(sequence)) {
        WRITE_DAC_BLOCK(0, 256, (const unsigned char far *)0, 1);
        STROKE_PORT_RECT(screen_rect);
    }
    if (sequence == 100) {
        play_intro(2);
    } else if (sequence >= 20) {
        play_quote_vector(sequence - 19);
    } else if (sequence >= 1 && sequence <= 6) {
        if (play_story_vector(sequence))
            result = 2;
        switch (sequence) {
        case 1: room = 4; break;
        case 2: room = 2; break;
        case 3: room = 6; break;
        }
    } else {
        if (play_other_vector(sequence))
            result = 2;
    }
    if (EraseScreenAfterNIS(sequence))
        STROKE_PORT_RECT(screen_rect);
    set_default_palette();
    set_kid_palette(room);
    playing_nis = 0;
    return result;
}

extern signed char room_to_draw;
extern unsigned char room_tile, level_redraw_flag;
extern int room_to_draw_word, nis_sound;
extern void reset_level_overlays(void);
extern void load_level_map(signed char room);
extern void load_background_set(int visible);
extern void load_pieces_data(void);
extern void load_code_overlays(unsigned char room, unsigned char tile);
extern void place_room_objects(unsigned char room);
extern void refresh_character(int mode);
extern void clean_cut(void);
extern void unload_set(int pass);
extern void unload_pieces_data(void);
extern void restore_room_vector(void);

void draw_room_from_nis(signed char room, signed char level)
{
    unsigned char old_room;
    int saved_sound;

    reset_level_overlays();
    old_room = current_room;
    room_to_draw = room;
    room_to_draw_word = room;
    saved_sound = pending_sound;
    pending_sound = nis_sound;
    load_level_map(room);
    open_verified_resource(level_resource_files[current_room]);
    load_background_set(!IsLevelStartWithNISScreen());
    if (old_room != current_room)
        load_pieces_data();
    load_code_overlays(current_room, room_tile);
    place_room_objects(current_room);
    level_redraw_flag = 0;
    level_code = level;
    refresh_character(1);
    clean_cut();
    unload_set(4);
    unload_set(1);
    unload_pieces_data();
    close_resource_file(level_resource_files[current_room]);
    current_room = 0;
    pending_sound = saved_sound;
    restore_room_vector();
}

typedef struct {
    int first;
    int second;
    int third;
    unsigned char pixels[1];
} image_header;

extern char dead_kid_asset[];
extern int kid_image_x, kid_image_y;
extern unsigned char kid_facing;
extern unsigned int hGetSL(int resource_id, int mode);
extern int load_nis_image(int asset, int frame, int type);
extern image_header far *RESOLVE_HANDLE_TO_FAR_PAYLOAD(int image);
extern void flip_image(unsigned char far *pixels, int first, int second, int third);
extern void PAINT_INDEXED_SHAPE(image_header far *image, int x, int y, int mode, int plane);
extern void DISPOSE_HANDLE(int image);

void draw_dead_kid(void)
{
    unsigned int shapes;
    int frame, x;
    image_header far *image;

    shapes = hGetSL((int)dead_kid_asset, 2);
    frame = load_nis_image(shapes, 80, 2);
    if (frame) {
        x = kid_image_x - 137;
        image = RESOLVE_HANDLE_TO_FAR_PAYLOAD(frame);
        if (kid_facing == 0) {
            flip_image(image->pixels, image->first, image->second, image->third);
            x -= 19;
        }
        PAINT_INDEXED_SHAPE(image, x, kid_image_y - 10, 10, 2);
        DISPOSE_HANDLE(frame);
        DISPOSE_HANDLE(shapes);
    }
}
