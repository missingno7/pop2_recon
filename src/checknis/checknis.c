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
extern int intro_music, default_music, special_music;
extern char *find_option(const char far *option);
extern int cdecl atoi(const char *value);
extern void start_music(int sound);
extern void prepare_music(int sound);
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
                start_music(intro_music);
                report_option(10034);
            } else if (sequence == 5 || sequence == 11) {
                prepare_music(special_music);
                start_music(special_music);
            } else {
                start_music(default_music);
            }
        } else if ((options.arg = find_option("TREE")) != 0) {
            quote_index = atoi(options.arg + 4);
            sequence = quote_index + 19;
        }
    }
    return sequence;
}

extern int playing_nis, pending_sound;
extern unsigned char current_room;
extern char screen_rect[];
extern void set_draw_mode(int mode);
extern void clear_dialog(void);
extern void stop_sound(int sound);
extern void set_palette(int first, int count, const unsigned char far *colors, int sync);
extern void erase_screen(const char far *rect);
extern void play_intro(int mode);
extern void play_quote_vector(int quote);
extern int play_story_vector(int sequence);
extern int play_other_vector(int sequence);
extern void finish_drawing(void);
extern void redraw_room(unsigned char room);

int play_nis(int sequence)
{
    int result = 1;
    unsigned char room;

    playing_nis = result;
    room = current_room;
    if (sequence != 100) {
        set_draw_mode(0);
        if (sequence < 20)
            clear_dialog();
        if (pending_sound != 0) {
            stop_sound(pending_sound);
            pending_sound = 0;
        }
    }
    if (EraseScreenBeforeNIS(sequence)) {
        set_palette(0, 256, (const unsigned char far *)0, 1);
        erase_screen(screen_rect);
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
        erase_screen(screen_rect);
    finish_drawing();
    redraw_room(room);
    playing_nis = 0;
    return result;
}

extern signed char room_to_draw;
extern unsigned char room_tile, level_redraw_flag;
extern int room_to_draw_word, nis_sound;
extern int room_music[];
extern void begin_room_drawing(void);
extern void select_room(signed char room);
extern void set_room_visibility(int visible);
extern void update_room(void);
extern void draw_room_tiles(unsigned char room, unsigned char tile);
extern void place_room_objects(unsigned char room);
extern void refresh_character(int mode);
extern void finish_room_drawing(void);
extern void draw_room_pass(int pass);
extern void present_room(void);
extern void restore_room_vector(void);

void draw_room_from_nis(signed char room, signed char level)
{
    unsigned char old_room;
    int saved_sound;

    begin_room_drawing();
    old_room = current_room;
    room_to_draw = room;
    room_to_draw_word = room;
    saved_sound = pending_sound;
    pending_sound = nis_sound;
    select_room(room);
    start_music(room_music[current_room]);
    set_room_visibility(!IsLevelStartWithNISScreen());
    {
        unsigned char room_delta = current_room ^ old_room;
        if (room_delta)
            update_room();
    }
    draw_room_tiles(current_room, room_tile);
    place_room_objects(current_room);
    level_redraw_flag = 0;
    level_code = level;
    refresh_character(1);
    finish_room_drawing();
    draw_room_pass(4);
    draw_room_pass(1);
    present_room();
    prepare_music(room_music[current_room]);
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
extern image_header far *lock_image(int image);
extern void flip_image(unsigned char far *pixels, int first, int second, int third);
extern void draw_image(image_header far *image, int x, int y, int mode, int plane);
extern void release_image(int image);

void draw_dead_kid(void)
{
    unsigned int shapes;
    int frame, x;
    image_header far *image;

    shapes = hGetSL((int)dead_kid_asset, 2);
    frame = load_nis_image(shapes, 80, 2);
    if (frame) {
        x = kid_image_x - 137;
        image = lock_image(frame);
        if (kid_facing == 0) {
            flip_image(image->pixels, image->first, image->second, image->third);
            x -= 19;
        }
        draw_image(image, x, kid_image_y - 10, 10, 2);
        release_image(frame);
        release_image(shapes);
    }
}
