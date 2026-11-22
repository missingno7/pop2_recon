/* NIS.C. Function and field declarations reflect the reviewed DOS ABI. */
typedef unsigned int Handle;
typedef struct {
    int first;
    int second;
    int shape_mode;
} ShapeList;

extern Handle get_resource(unsigned long type, int resource_id);
extern Handle make_fixed(Handle handle);
extern void far *lock_handle(Handle handle);
extern int cdecl sprintf(char *destination, const char *format, ...);
extern void fatal_error(int status, const char far *message);
static const char shape_list_error[] = "hGetSL(%d)";

Handle hGetSL(int resource_id, int mode)
{
    char message[40];
    Handle handle;

    handle = make_fixed(get_resource(0x5348504cUL, resource_id));
    if (handle) {
        ((ShapeList far *)lock_handle(handle))->shape_mode = mode;
        return handle;
    }

    sprintf(message, shape_list_error, resource_id);
    fatal_error(1, message);
}

unsigned int GetStrLCt(int resource_id)
{
    return *(unsigned char far *)lock_handle(get_resource(0x5354524cUL, resource_id));
}

extern unsigned char music_options;
extern unsigned long clock_ticks, music_deadline;
extern void play_sound(int sound);
extern int await_sound(int sound);
extern int (far *poll_input)(void);

void start_timed_music(int sound, unsigned int duration)
{
    if (music_options & 1) play_sound(sound);
    else
        music_deadline = clock_ticks + duration;
}

int await_timed_music(int sound);

int play_timed_music(int sound, unsigned int duration)
{
    start_timed_music(sound, duration);
    return await_timed_music(sound);
}

int await_timed_music(int sound)
{
    int interrupted = 0;

    if (music_options & 1) {
        interrupted = await_sound(sound);
    } else {
        do {
            if (poll_input()) interrupted = 1;
        } while (!interrupted && clock_ticks < music_deadline);
    }
    return interrupted;
}

extern int query_sound_payload(const void far *sound);
extern struct {
    unsigned char status;
    unsigned char play_time;
} midi;

int WaitMidiPlayTime(unsigned char time)
{
    while (query_sound_payload(0L)) {
        if (poll_input())
            return 1;
        if (time <= midi.play_time) break;
    }
    return 0;
}

extern Handle dialog_shape, dialog_font, title_font;
extern Handle load_font(int resource_id);
extern void dispose_handle(Handle handle);

void GetDialog(void)
{
    dialog_shape = hGetSL(25000, 1);
    dialog_font = load_font(10);
    title_font = load_font(11);
}

void FreeDialog(void)
{
    dispose_handle(title_font);
    dispose_handle(dialog_font);
    dispose_handle(dialog_shape);
}

typedef struct {
    int top, left, bottom, right;
} Rect;

typedef struct {
    char private_fields[40];
    int color;
} DrawPort;

typedef struct {
    char private_fields[16];
    Rect clip;
} ImagePort;

extern unsigned int extended_dialog;
extern unsigned int alternate_dialog;
extern Rect dialog_rect, extended_dialog_rect;
extern DrawPort *draw_port;
extern ImagePort *alternate_port, *extended_port;
extern int pending_decoration, decoration_shape, decoration_x, decoration_y;

extern const char far *lookup_string(const char far *list, unsigned char line);
extern char far *cdecl _fstrcpy(char far *to, const char far *from);
extern DrawPort *open_temporary_port(void *saved_state, const Rect far *bounds);
extern void copy_port(DrawPort *port, const void *source, const Rect far *src_rect,
                      const Rect far *dst_rect, int mode);
extern void PaintRLEShapeItem(Handle shapes, int shape, int x, int y, int mode);
extern void hSetFont(Handle font);
extern Rect far *copy_offset_rect(Rect far *dst, const Rect far *src,
                                  int left, int top, int right, int bottom);
extern Rect far *split_text_rect(Rect far *dst, const Rect far *src, int lines, int mode);
extern void draw_text(const Rect far *rect, int x, int y, const char far *text);
extern void close_temporary_port(void *saved_state);

void draw_nis_text(int resource_id, int line)
{
    char text[256], saved_state[44];
    Rect first_line, text_rect, bounds;
    int title_color, body_color;
    DrawPort *old_port;

    if (line == 0) text[0] = 0;
    else
        _fstrcpy(text, lookup_string(lock_handle(get_resource(0x5354524cUL, resource_id)), (unsigned char)line));

    if (extended_dialog)
        bounds = extended_dialog_rect;
    else
        bounds = dialog_rect;

    old_port = draw_port;
    draw_port = open_temporary_port(saved_state, &bounds);
    if (alternate_dialog) {
        body_color = 49;
        title_color = 34;
        copy_port(draw_port, alternate_port, &alternate_port->clip, &alternate_port->clip, 0);
    } else {
        body_color = 14;
        title_color = 15;
        if (extended_dialog)
            copy_port(draw_port, extended_port, &extended_port->clip, &extended_port->clip, 0);
        else
            PaintRLEShapeItem(dialog_shape, 2, 0, 157, 0);
    }

    if (pending_decoration) {
        PaintRLEShapeItem(dialog_shape, decoration_shape, decoration_x, decoration_y, 0);
        pending_decoration = 0;
    }

    hSetFont(dialog_font);
    draw_port->color = 0;
    copy_offset_rect(&text_rect, &bounds, 0, 3, 0, 0);
    draw_text(split_text_rect(&first_line, &text_rect, 1, 0), 0, 0, text);
    draw_port->color = body_color;
    draw_text(&text_rect, 0, 0, text);
    hSetFont(title_font);
    draw_port->color = title_color;
    draw_text(&text_rect, 0, 0, text);

    copy_port(old_port, saved_state, &bounds, &bounds, 0);
    draw_port = old_port;
    close_temporary_port(saved_state);
}

void paint_ruins_sky(Handle shapes)
{
    int x = 0;
    do {
        PaintRLEShapeItem(shapes, 7, x, 0, 0);
        x += 32;
    } while (x < 320);
}

typedef struct {
    int width, height, pitch;
    unsigned char pixels[1];
} ShapeImage;

extern void flip_horse_pixels(unsigned char far *pixels, int width, int height);
extern void flip_kid_pixels(unsigned char far *pixels, int width, int height);

typedef struct {
    unsigned first;
    unsigned left;
} ShapeRange;

void FlipHorses(const Handle *shapes)
{
    const Handle *entry;
    ShapeImage far *image;
    ShapeRange range;

    range.first = 1008;
    entry = shapes + range.first;
    range.left = 36;

    do {
        if (*entry) {
            image = lock_handle(*entry);
            flip_horse_pixels(image->pixels, image->width, image->height);
        }
        ++entry;
    } while (--range.left);
}

void FlipKids(const Handle *shapes)
{
    const Handle *entry;
    ShapeImage far *image;
    ShapeRange range;

    range.first = 857;
    entry = shapes + range.first;
    range.left = 14;

    do {
        if (*entry) {
            image = lock_handle(*entry);
            flip_kid_pixels(image->pixels, image->width, image->height);
        }
        ++entry;
    } while (--range.left);
}

void flip_kid(const Handle *shapes)
{
    ShapeImage far *image;

    image = lock_handle(shapes[623]);
    flip_kid_pixels(image->pixels, image->width, image->height);
}

extern DrawPort nis_draw_port;
extern Handle tree_palette;
extern int tree_scene;
enum {
    TREE_RESOURCE = 26000,
    TREE_PALETTE_BASE = 26001,
    TREE_MUSIC = 26002,
    LAST_TREE_PALETTE = 26004,
    TREE_VOICE_BASE = 26010
};

extern int await_palette(void *palette, unsigned int blocks, unsigned int delay);
extern int run_nis_dialog(int resource, int line, int voice, unsigned int duration);

int tree_callback(void *playback, int frame)
{
    int interrupted;
    DrawPort *old_port;

    midi.play_time = 0;
    play_sound(TREE_VOICE_BASE + tree_scene);
    interrupted = await_palette(tree_palette, -1, 0);
    if (!interrupted) {
        if (tree_scene == 1) {
            old_port = draw_port;
            draw_port = &nis_draw_port;
            interrupted = run_nis_dialog(TREE_RESOURCE, 1, 1, 69);
            if (!interrupted) start_timed_music(TREE_MUSIC, 120);
            draw_port = old_port;
        } else if (tree_scene == 9) {
            interrupted = WaitMidiPlayTime(97);
        }
    }
    return interrupted;
}

typedef int (far *AnimationCallback)(void *playback, int frame);

extern const char nis_file[];
extern const char *nis_data_files[6];
extern int (far *saved_poll_input)(void);
extern void open_nis_files(const char * const *files);
extern void install_nis_input(void);
extern void open_data_file(const char *name);
extern void close_data_file(const char *name);
extern void close_nis_files(const char * const *files);
extern void set_palette(int first, int count, const void far *colors, int sync);
extern int play_nis_animation(Handle shapes, int resource, int scene, int mode,
                              AnimationCallback callback, int x, int y, int options);
extern int pause_nis(int mode, unsigned int duration);
extern void finish_nis_animation(void);
extern void stop_nis_sound(const void far *sound);

void tree_check(int scene)
{
    register Handle shapes;
    int interrupted;
    int palette;

    open_nis_files(nis_data_files); install_nis_input();
    tree_scene = scene;
    open_data_file(nis_file);
    shapes = hGetSL(TREE_RESOURCE, -1); GetDialog();
    set_palette(0, 256, 0L, 1); PaintRLEShapeItem(dialog_shape, 1, 0, 0, 0);
    if (scene == 1) draw_nis_text(0, 0);
    PaintRLEShapeItem(shapes, 2, 40, 32, 0);

    palette = (tree_scene - 1) / 3;
    tree_palette = get_resource(0x50414c54UL, TREE_PALETTE_BASE + (palette > 2 ? 2 : palette));
    interrupted = play_nis_animation(shapes, TREE_RESOURCE, tree_scene, 1, tree_callback, 0, 0, 1);
    dispose_handle(tree_palette);
    if (!interrupted) {
        if (tree_scene == 1) {
            interrupted = await_timed_music(TREE_MUSIC);
            if (!interrupted) interrupted = run_nis_dialog(TREE_RESOURCE, 2, 3, 211);
            if (!interrupted) interrupted = run_nis_dialog(TREE_RESOURCE, 3, 4, 110);
            if (!interrupted) interrupted = run_nis_dialog(TREE_RESOURCE, 4, 5, 75);
        } else if (tree_scene == 9) {
            interrupted = WaitMidiPlayTime(98);
            if (!interrupted) {
                PaintRLEShapeItem(shapes, 15, 69, 56, 10);
                interrupted = WaitMidiPlayTime(99);
                if (!interrupted) {
                    tree_palette = get_resource(0x50414c54UL, LAST_TREE_PALETTE);
                    interrupted = await_palette(tree_palette, -1, 2); dispose_handle(tree_palette);
                }
            }
        }
    }
    if (!interrupted) {
        interrupted = await_sound(0);
        if (!interrupted) pause_nis(0, 60);
    }
    finish_nis_animation();
    if (interrupted) stop_nis_sound(0L);

    FreeDialog(); dispose_handle(shapes); dispose_handle(dialog_shape);
    close_data_file(nis_file);
    poll_input = saved_poll_input;
    close_nis_files(nis_data_files);
}

extern const char transition_file[], caverns_file[], ruins_file[];
extern Rect screen_bounds, viewport_bounds;
extern int transition_animation_state;
extern ImagePort *title_patch;
extern int suppress_title_frame;
enum { TRANSITION_RESOURCE = 25000, PRINCESS_MUSIC = 31030, PRINCESS_CUE = 31031 };
extern Handle transition_shapes, background_shapes, transition_palette;
extern Handle title_left, title_right, second_palette;
extern void fill_rect(const Rect far *bounds, int color);
extern int is_sound_playing(int sound);
extern Handle load_shape_item(Handle shapes, int item, int flags);
extern void close_dialog_port(ImagePort *port);
extern void draw_room_from_nis(signed char room, signed char level);
extern void PaintShapeItem(Handle shapes, int item, int x, int y, int mode, int plane);
extern void restore_screen_rect(const Rect far *bounds);
extern int title_callback(void *playback, int frame);
extern int transition_scene_one(void *playback, int frame);
extern int transition_scene_four(void *playback, int frame);
extern int transition_scene_five(void *playback, int frame);
extern int transition_scene_six(void *playback, int frame);
extern int transition_scene_seven(void *playback, int frame);
extern int transition_scene_eight(void *playback, int frame);
extern int transition_scene_nine(void *playback, int frame);
extern int transition_scene_ten(void *playback, int frame);
extern int transition_scene_eleven(void *playback, int frame);
extern int princess_room_callback(void *playback, int frame);

int trans_princess_room(Handle shapes);
int trans_throne_room(Handle shapes);
void draw_princess_room(Handle shapes);

#define TRANSITION_COLORS(i) get_resource(0x50414c54UL, TRANSITION_RESOURCE+(i))
#define PLAY_TRANSITION(script, mode, callback) \
    play_nis_animation(transition_shapes, TRANSITION_RESOURCE, script, mode, callback, 0, 0, 1)

#define NIS_ITEM(item, x, y, mode) \
    PaintRLEShapeItem(transition_shapes, item, x, y, mode)
#define BACKDROP_ITEM(item, x, y, mode) \
    PaintRLEShapeItem(background_shapes, item, x, y, mode)

int Transition(int scene)
{
    int interrupted;
    int default_result;

    open_nis_files(nis_data_files);
    transition_animation_state = 265;
    if (scene != 5)
        open_data_file(transition_file);
    if (scene != 6) {
        set_palette(0, 256, 0L, 1);
        fill_rect(&screen_bounds, 0);
    }
    install_nis_input();
    transition_shapes = hGetSL(TRANSITION_RESOURCE, -1);

    if (scene == 4) {
        if (!is_sound_playing(TRANSITION_RESOURCE + 11))
            play_sound(TRANSITION_RESOURCE + 16);
        fill_rect(&screen_bounds, 16);
        NIS_ITEM(320, 118, 0, 10);
        NIS_ITEM(321, 73, 60, 10);
        NIS_ITEM(322, -41, 32, 10);
        title_left = load_shape_item(transition_shapes, 312, 1);
        title_right = load_shape_item(transition_shapes, 313, 1);
        transition_palette = TRANSITION_COLORS(2);
        title_patch = 0;
        suppress_title_frame = 2;
        interrupted = PLAY_TRANSITION(12, 1, title_callback);
        suppress_title_frame = 0;
        if (title_patch)
            close_dialog_port(title_patch);
        dispose_handle(transition_palette);
        dispose_handle(title_right);
        dispose_handle(title_left);
    } else if (scene == 1) {
        open_data_file(caverns_file);
        open_data_file(ruins_file);
        fill_rect(&viewport_bounds, 48);
        NIS_ITEM(358, -127, -34, 0);
        NIS_ITEM(354, 0, 105, 0);
        NIS_ITEM(369, 185, 46, 10);
        transition_palette = TRANSITION_COLORS(1);
        interrupted = PLAY_TRANSITION(1, 1, transition_scene_one);
        dispose_handle(transition_palette);
        if (!interrupted) {
            set_palette(0, 256, 0L, 1);
            fill_rect(&screen_bounds, 0);
            fill_rect(&viewport_bounds, 16);
            NIS_ITEM(320, 117, 0, 10);
            NIS_ITEM(321, 73, 60, 10);
            NIS_ITEM(322, -41, 34, 10);
            transition_palette = TRANSITION_COLORS(2);
            interrupted = PLAY_TRANSITION(2, 1, transition_scene_one);
            dispose_handle(transition_palette);
        }
        if (!interrupted) {
            set_palette(0, 256, 0L, 1);
            fill_rect(&screen_bounds, 0);
            dispose_handle(transition_shapes);
            transition_shapes = hGetSL(TRANSITION_RESOURCE, -2);
            background_shapes = hGetSL(3500, -16);
            BACKDROP_ITEM(1100, 0, 0, 0);
            BACKDROP_ITEM(1101, 10, 100, 0);
            BACKDROP_ITEM(1102, 0, 100, 0);
            dispose_handle(background_shapes);
            transition_palette = TRANSITION_COLORS(11);
            interrupted = await_palette(transition_palette, -1, 0);
            dispose_handle(transition_palette);
        }
        if (!interrupted)
            interrupted = PLAY_TRANSITION(3, 1, 0);
        close_data_file(ruins_file);
        close_data_file(caverns_file);
    } else if (scene == 2) {
        open_data_file(ruins_file);
        background_shapes = hGetSL(3500, -8);
        paint_ruins_sky(background_shapes);
        BACKDROP_ITEM(763, 0, 39, 10);
        BACKDROP_ITEM(764, 0, 99, 0);
        NIS_ITEM(317, 75, 166, 10);
        transition_palette = TRANSITION_COLORS(3);
        second_palette = TRANSITION_COLORS(4);
        interrupted = PLAY_TRANSITION(4, 1, transition_scene_four);
        dispose_handle(second_palette);
        dispose_handle(transition_palette);
        if (!interrupted) {
            transition_palette = TRANSITION_COLORS(4);
            interrupted = PLAY_TRANSITION(5, 1, transition_scene_five);
            dispose_handle(transition_palette);
        }
        if (!interrupted) {
            set_palette(0, 256, 0L, 1);
            transition_palette = TRANSITION_COLORS(4);
            paint_ruins_sky(background_shapes);
            BACKDROP_ITEM(766, 0, 11, 10);
            BACKDROP_ITEM(767, 0, 101, 0);
            set_palette(0, 256, lock_handle(transition_palette), 1);
            dispose_handle(transition_palette);
            interrupted = PLAY_TRANSITION(6, 1, transition_scene_six);
        }
        if (!interrupted) {
            interrupted = await_palette(0, -1, 0);
            fill_rect(&screen_bounds, 0);
        }
        if (!interrupted) {
            NIS_ITEM(347, 0, 0, 10);
            transition_palette = TRANSITION_COLORS(5);
            interrupted = await_palette(transition_palette, -1, 0);
            dispose_handle(transition_palette);
        }
        if (!interrupted)
            interrupted = PLAY_TRANSITION(7, 1, transition_scene_seven);
        if (!interrupted) {
            interrupted = await_palette(0, -1, 0);
            fill_rect(&screen_bounds, 0);
        }
        if (!interrupted) {
            fill_rect(&viewport_bounds, 0);
            dispose_handle(transition_shapes);
            transition_shapes = hGetSL(TRANSITION_RESOURCE, -2);
            transition_palette = TRANSITION_COLORS(6);
            second_palette = TRANSITION_COLORS(7);
            draw_room_from_nis(10, 22);
            interrupted = PLAY_TRANSITION(8, 1, transition_scene_eight);
            dispose_handle(second_palette);
            dispose_handle(transition_palette);
        }
        dispose_handle(background_shapes);
    } else if (scene == 3) {
        fill_rect(&viewport_bounds, 48);
        NIS_ITEM(362, 0, 126, 0);
        NIS_ITEM(363, 0, 160, 10);
        NIS_ITEM(364, 0, 168, 10);
        NIS_ITEM(365, 183, 97, 10);
        NIS_ITEM(368, 11, 77, 10);
        NIS_ITEM(369, -12, 40, 10);
        NIS_ITEM(373, 28, 13, 10);
        NIS_ITEM(373, 154, 31, 10);
        NIS_ITEM(374, 126, 50, 10);
        NIS_ITEM(352, 0, 0, 10);
        interrupted = PLAY_TRANSITION(9, 1, transition_scene_nine);
        if (!interrupted) {
            await_palette(0, -1, 0);
            fill_rect(&screen_bounds, 0);
            fill_rect(&viewport_bounds, 48);
            NIS_ITEM(373, 153, 33, 0); NIS_ITEM(374, 129, 51, 0); NIS_ITEM(375, -8, 39, 0); NIS_ITEM(373, 2, 10, 0);
            NIS_ITEM(362, 0, 135, 0);
            NIS_ITEM(363, 0, 168, 0);
            NIS_ITEM(364, 0, 176, 10);
            transition_palette = TRANSITION_COLORS(8);
            interrupted = PLAY_TRANSITION(10, 1, transition_scene_ten);
            dispose_handle(transition_palette);
        }
        if (!interrupted) {
            set_palette(0, 256, 0L, 1);
            fill_rect(&screen_bounds, 0);
            dispose_handle(transition_shapes);
            transition_shapes = hGetSL(TRANSITION_RESOURCE, -2);
            draw_room_from_nis(14, 1);
            transition_palette = TRANSITION_COLORS(9);
            interrupted = await_palette(transition_palette, -1, 0);
            dispose_handle(transition_palette);
        }
        if (!interrupted) {
            second_palette = TRANSITION_COLORS(10);
            interrupted = PLAY_TRANSITION(11, 1, transition_scene_eleven);
            dispose_handle(second_palette);
        }
    } else if (scene == 5) {
        interrupted = trans_princess_room(transition_shapes);
    } else {
        /* The default branch reads the unused word in the DOS stack frame. */
        interrupted = default_result;
        if (scene == 6)
            interrupted = trans_throne_room(transition_shapes);
    }

    if (interrupted) {
        finish_nis_animation();
        stop_nis_sound(0L);
    }
    dispose_handle(transition_shapes);
    poll_input = saved_poll_input;
    if (scene != 5)
        close_data_file(transition_file);
    if (scene != 6)
        close_data_file(ruins_file);
    transition_animation_state = 0; close_nis_files(nis_data_files);
    return interrupted;
}

#undef BACKDROP_ITEM
#undef NIS_ITEM

int trans_princess_room(Handle shapes)
{
    Handle shadow;
    int interrupted;

    transition_animation_state = 265;
    draw_princess_room(shapes);
    shadow = hGetSL(TRANSITION_RESOURCE + 1, 2);
    PaintShapeItem(shadow, 249, 90, 137, 10, 1);
    dispose_handle(shadow);
    play_sound(PRINCESS_MUSIC);
    transition_palette = TRANSITION_COLORS(0);
    interrupted = await_palette(transition_palette, -1, 0);
    dispose_handle(transition_palette);
    if (!interrupted)
        interrupted = play_nis_animation(shapes, TRANSITION_RESOURCE, 0, 1, 0, 0, 0, 1);
    if (!interrupted)
        interrupted = pause_nis(0, 90);
    if (!interrupted) {
        ((ShapeList far *)lock_handle(shapes))->shape_mode = -2;
        interrupted = play_nis_animation(shapes, TRANSITION_RESOURCE, 1, 1, princess_room_callback, 0, 0, 1);
        restore_screen_rect(&screen_bounds);
    }
    transition_animation_state = 0;
    return interrupted;
}

void draw_princess_room(Handle shapes)
{
    PaintRLEShapeItem(transition_shapes, 344, 0, -7, 0);
    PaintRLEShapeItem(transition_shapes, 345, 0, 93, 0);
    PaintRLEShapeItem(transition_shapes, 348, 34, 79, 10);
    PaintRLEShapeItem(transition_shapes, 348, 223, 79, 10);
}

#undef PLAY_TRANSITION
#undef TRANSITION_COLORS

typedef struct {
    char header[0x2e];
    ImagePort port;
    char image_table[0x4bc - 0x46];
    void far *fade;
} AnimationState;

extern Rect small_throne_bounds;
extern void draw_dead_kid(void);
extern DrawPort *allocate_clipped_port(const Rect far *bounds);
extern DrawPort *make_transition_port(const Rect far *bounds);
extern void close_transition_port(DrawPort *port);
extern void close_dialog_port(ImagePort *port);
extern int animate_port(int mode, DrawPort *port, const Rect far *bounds, long duration);
extern int ruined_throne_callback(void *playback, int frame);
enum { THRONE_DIALOG_RESOURCE = 31000, THRONE_MUSIC = 31020, THRONE_NEXT_MUSIC = 31021 };
extern ImagePort *make_dialog_port(const Rect far *bounds);
extern void far *begin_palette_fade(Handle palette, int timing, int options);
extern void apply_palette_colors(int timing, const void far *colors);

void draw_ruined_throne_room(Handle shapes);

int trans_throne_room(Handle shapes)
{
    DrawPort *port;
    Handle background, palette, later_palette;
    int interrupted;

    extended_dialog = 1;
    extended_port = 0;
    dialog_font = load_font(10);
    title_font = load_font(11);
    port = make_transition_port(&viewport_bounds);
    draw_port = port;
    background = hGetSL(3500, -16);
    PaintRLEShapeItem(background, 708, 0, 0, 0);
    PaintRLEShapeItem(background, 709, 0, 100, 0);
    draw_dead_kid();
    palette = get_resource(0x50414c54UL, 4208);
    set_palette(0, 224, lock_handle(palette), 1);
    dispose_handle(palette);
    interrupted = await_sound(10254);
    if (!interrupted) {
        play_sound(THRONE_MUSIC);
        draw_port = &nis_draw_port;
        interrupted = animate_port(1, port, &viewport_bounds, 300L);
    }
    if (!interrupted) {
        palette = get_resource(0x50414c54UL, 4210);
        set_palette(32, 16, lock_handle(palette), 1);
        dispose_handle(palette);
        interrupted = WaitMidiPlayTime(97);
    }
    if (!interrupted) {
        ShapeList far *list;

        background = hGetSL(3500, 4);
        list = lock_handle(background);
        list->first = 4210;
        list->second = 4;
        interrupted = play_nis_animation(background, 4210, 0, 1, ruined_throne_callback, 0, 0, 1);
        dispose_handle(background);
        close_transition_port(port);
        port = 0;
    }
    if (!interrupted) {
        draw_port = &nis_draw_port;
        extended_port = make_dialog_port(&extended_dialog_rect);
    }
    if (!interrupted) interrupted = run_nis_dialog(THRONE_DIALOG_RESOURCE, 1, 1, 311);
    if (!interrupted) interrupted = run_nis_dialog(THRONE_DIALOG_RESOURCE, 2, 2, 278);
    if (!interrupted) interrupted = run_nis_dialog(THRONE_DIALOG_RESOURCE, 3, 3, 121);
    if (!interrupted)
        interrupted = run_nis_dialog(THRONE_DIALOG_RESOURCE, 4, 4, 326);
    if (!interrupted) {
        draw_nis_text(0, 0); interrupted = pause_nis(0, 60);
    }
    if (!interrupted)
        interrupted = run_nis_dialog(THRONE_DIALOG_RESOURCE, 5, 5, 222);
    if (!interrupted)
        interrupted = pause_nis(0, 60);
    if (!interrupted)
        interrupted = run_nis_dialog(THRONE_DIALOG_RESOURCE, 6, 6, 224);
    if (!interrupted) {
        interrupted = pause_nis(0, 60); draw_nis_text(0, 0);
    }
    if (extended_port)
        close_dialog_port(extended_port);
    if (!interrupted) {
        port = make_transition_port(&small_throne_bounds);
        draw_port = port;
        background = hGetSL(3500, -16);
        PaintRLEShapeItem(background, 709, 0, 100, 0);
        draw_port = &nis_draw_port;
        interrupted = animate_port(1, port, &small_throne_bounds, 180L);
        close_transition_port(port);
        port = 0;
    }
    if (!interrupted) {
        port = make_transition_port(&viewport_bounds);
        draw_port = port;
        background = hGetSL(3500, -16);
        draw_ruined_throne_room(background);
        interrupted = await_sound(0);
    }
    if (!interrupted) {
        palette = get_resource(0x50414c54UL, 4209);
        interrupted = await_palette(palette, -1, 1);
        dispose_handle(palette);
    }
    if (!interrupted) {
        copy_port(&nis_draw_port, port, &viewport_bounds, &viewport_bounds, 0);
        later_palette = get_resource(0x50414c54UL, 4211);
        interrupted = await_palette(later_palette, -1, 1);
        dispose_handle(later_palette);
        play_sound(THRONE_NEXT_MUSIC);
    }
    if (port)
        close_transition_port(port);
    draw_port = &nis_draw_port;
    dispose_handle(hGetSL(3500, -16));
    dispose_handle(get_resource(0x5354524cUL, 4210));
    dispose_handle(title_font);
    dispose_handle(dialog_font);
    return interrupted;
}

void draw_ruined_throne_room(Handle shapes)
{
    PaintRLEShapeItem(shapes, 700, 27, 0, 0);
    PaintRLEShapeItem(shapes, 701, 28, 100, 0);
    PaintRLEShapeItem(shapes, 702, 0, 0, 10);
    PaintRLEShapeItem(shapes, 703, 289, 90, 10);
    PaintRLEShapeItem(shapes, 704, 56, 181, 10);
    PaintRLEShapeItem(shapes, 705, 56, 185, 10);
    PaintRLEShapeItem(shapes, 707, 274, 90, 10);
    draw_dead_kid();
}

int ruined_throne_callback(void *playback, int frame)
{
    ImagePort *source = playback;
    DrawPort *port;
    int interrupted;

    port = make_transition_port(&small_throne_bounds);
    copy_port(port, source, &small_throne_bounds, &source->clip, 0);
    draw_port = &nis_draw_port;
    interrupted = animate_port(1, port, &small_throne_bounds, 180L);
    close_transition_port(port);
    if (!interrupted)
        interrupted = WaitMidiPlayTime(98);
    return interrupted;
}

void init_fade_to_primary(AnimationState *playback, int options)
{
    playback->fade = begin_palette_fade(transition_palette, -1, options);
}

void cut_to_primary(void)
{
    apply_palette_colors(-1, lock_handle(transition_palette));
}

void init_fade_to_black(AnimationState *playback, int options)
{
    playback->fade = begin_palette_fade(0, -1, options);
}

int transition_scene_one(void *playback, int frame)
{
    int interrupted = 0;

    if (frame == 0) {
        init_fade_to_primary(playback, 1);
        interrupted = WaitMidiPlayTime(97);
    } else {
        init_fade_to_black(playback, 1);
    }
    return interrupted;
}

int transition_scene_four(void *playback, int frame)
{
    if (frame == 0) {
        FlipHorses(playback);
        play_sound(TRANSITION_RESOURCE + 4);
    } else if (--frame == 0) {
        cut_to_primary();
    } else {
        ((AnimationState *)playback)->fade = begin_palette_fade(second_palette, 7, 1);
    }
    return 0;
}

int transition_scene_five(void *playback, int frame)
{
    DrawPort *saved;
    Rect lowered_bounds;

    if (frame == 0) {
        FlipHorses(playback);
        set_palette(0, 256, 0L, 1);
        saved = draw_port; draw_port = &nis_draw_port;
        fill_rect(&viewport_bounds, 0);
        paint_ruins_sky(background_shapes);
        lowered_bounds = viewport_bounds;
        lowered_bounds.top = 80;
        fill_rect(&lowered_bounds, 162);
        PaintRLEShapeItem(transition_shapes, 332, 0, 35, 10);
        draw_port = saved;
        copy_port((DrawPort *)&((AnimationState *)playback)->port, &nis_draw_port,
                  split_text_rect(&lowered_bounds, &screen_bounds,
                                  -((AnimationState *)playback)->port.clip.left,
                                  -((AnimationState *)playback)->port.clip.top),
                  &screen_bounds, 0);
    } else if (--frame == 0) {
        cut_to_primary();
    } else {
        set_palette(0, 256, 0L, 1);
    }
    return 0;
}

int transition_scene_six(void *playback, int frame)
{
    if (frame == 0)
        FlipHorses(playback);
    else
        play_sound(TRANSITION_RESOURCE + 6);
    return 0;
}

int transition_scene_seven(void *playback, int frame)
{
    FlipHorses(playback);
    return 0;
}

int transition_scene_eight(void *playback, int frame)
{
    if (frame == 0) {
        flip_kid(playback);
        cut_to_primary();
    } else if (--frame == 0) {
        ((AnimationState *)playback)->fade = begin_palette_fade(second_palette, 7, 1);
    }
    return 0;
}

int transition_scene_nine(void *playback, int frame)
{
    int interrupted;

    play_sound(TRANSITION_RESOURCE + 9);
    transition_palette = get_resource(0x50414c54UL, TRANSITION_RESOURCE + 8);
    interrupted = await_palette(transition_palette, -1, 0);
    dispose_handle(transition_palette);
    if (!interrupted)
        interrupted = WaitMidiPlayTime(97);
    return interrupted;
}

int transition_scene_ten(void *playback, int frame)
{
    if (frame == 0)
        init_fade_to_primary(playback, 1);
    else
        init_fade_to_black(playback, 1);
    return 0;
}

int transition_scene_eleven(void *playback, int frame)
{
    if (frame == 0)
        flip_kid(playback);
    else
        ((AnimationState *)playback)->fade = begin_palette_fade(second_palette, 7, 1);
    return 0;
}

void erase_static_jaffar(DrawPort *port);

int princess_room_callback(void *playback, int frame)
{
    Handle palette;

    if (frame != 0) {
        if (frame == 1)
            play_sound(PRINCESS_CUE);
        palette = get_resource(0x50414c54UL, TRANSITION_RESOURCE + frame);
        if (palette) {
            set_palette(0, 256, lock_handle(palette), 1);
            dispose_handle(palette);
        }
    } else {
        FlipKids(playback);
        erase_static_jaffar(playback);
    }
    return 0;
}

void erase_static_jaffar(DrawPort *port)
{
    DrawPort *saved = draw_port;
    ImagePort *image = (ImagePort *)port;

    draw_port = port;
    PaintRLEShapeItem(hGetSL(TRANSITION_RESOURCE, -1), 363, -6, -3, 0);
    copy_port((DrawPort *)((char *)port + 0x2e), port, &image->clip, &image->clip, 0);
    draw_port = saved;
}

extern void *cdecl memset(void *destination, int value, unsigned int length);
#pragma intrinsic(memset)

extern Rect nis_clip;
extern Handle nis_palette;
extern volatile int delay_count;
extern int wait_timer(int timer);
extern void finish_palette_effect(void);
extern void dispatch_resource_pointer(const void far *resource);
void clear_nis(DrawPort *port, int color);
void add_nis(DrawPort *port, Handle shapes, int item, int x, int y, int mode);
void copy_nis(const void *source);
void cut_nis(const void *source);
int fade_nis(const void *source, int duration);

enum { SHIP_SCENE = 28000, SHIP_MUSIC = 28100 };
#define SHIP_PALETTE(index) get_resource(0x50414c54UL, SHIP_SCENE + (index))

int aboard_ship(void)
{
    int interrupted;
    Handle shapes;
    union {
        DrawPort draw;
        char saved_state[44];
    } port;
    unsigned char white[240][3];

    play_sound(SHIP_MUSIC); memset(white, 63, sizeof white);
    open_temporary_port(&port, &nis_clip);
    dialog_shape = hGetSL(25000, 1);
    shapes = hGetSL(SHIP_SCENE, -1);
    GetDialog();
    set_palette(0, 256, 0L, 1);
    PaintRLEShapeItem(dialog_shape, 1, 0, 0, 0);
    PaintRLEShapeItem(shapes, 2, 40, 32, 0);
    draw_nis_text(0, 0);

    nis_palette = SHIP_PALETTE(1);
    interrupted = WaitMidiPlayTime(97);
    if (!interrupted) interrupted = await_palette(nis_palette, -1, 2);
    if (!interrupted) interrupted = WaitMidiPlayTime(98);
    if (!interrupted) {
        decoration_shape = 5;
        decoration_x = 88;
        decoration_y = 159;
        pending_decoration = 1;
        interrupted = run_nis_dialog(SHIP_SCENE, 1, 1, 186);
        if (!interrupted)
            interrupted = pause_nis(0, 20);
    }
    if (!interrupted) {
        dispose_handle(nis_palette);
        nis_palette = SHIP_PALETTE(2);
        clear_nis(&port.draw, 0);
        add_nis(&port.draw, shapes, 3, 38, 31, 0);
        add_nis(&port.draw, shapes, 4, 37, 102, 10);
        interrupted = WaitMidiPlayTime(99);
        draw_nis_text(0, 0);
        if (!interrupted)
            interrupted = fade_nis(&port.draw, 0);
        if (!interrupted)
            interrupted = pause_nis(0, 18);
    }
    if (!interrupted) interrupted = WaitMidiPlayTime(100);
    if (!interrupted) {
        interrupted = run_nis_dialog(SHIP_SCENE, 2, 2, 131);
        if (!interrupted) interrupted = pause_nis(0, 20);
    }
    if (!interrupted) interrupted = WaitMidiPlayTime(101);
    if (!interrupted) {
        interrupted = run_nis_dialog(SHIP_SCENE, 3, 3, 200);
        if (!interrupted) interrupted = pause_nis(0, 20);
    }
    if (!interrupted) {
        draw_nis_text(0, 0); interrupted = pause_nis(0, 72);
    }
    if (!interrupted) {
        interrupted = WaitMidiPlayTime(102);
        if (!interrupted) start_timed_music(SHIP_SCENE + 5, 238);
        if (!interrupted) interrupted = await_palette(0, -2, 5);
    }
    if (!interrupted) interrupted = await_timed_music(SHIP_SCENE + 5);
    if (!interrupted) {
        dispose_handle(nis_palette); nis_palette = SHIP_PALETTE(2); clear_nis(&port.draw, 0); add_nis(&port.draw, shapes, 5, 40, 32, 0);
        interrupted = WaitMidiPlayTime(103);
        if (!interrupted) interrupted = fade_nis(&port.draw, 3);
    }
    if (!interrupted)
        interrupted = WaitMidiPlayTime(104);
    if (!interrupted) {
        start_timed_music(10500, 121);
        draw_nis_text(SHIP_SCENE, 4);
        interrupted = await_timed_music(10500);
    }
    if (!interrupted)
        interrupted = WaitMidiPlayTime(105);
    if (!interrupted)
        play_sound(30500);
    if (!interrupted) {
        draw_nis_text(0, 0);
        add_nis(&port.draw, shapes, 6, 41, 32, 10);
        cut_nis(&port.draw);
        interrupted = pause_nis(0, 6);
    }
    if (!interrupted) {
        dispose_handle(nis_palette);
        nis_palette = SHIP_PALETTE(4);
        apply_palette_colors(-1, lock_handle(nis_palette));
        interrupted = pause_nis(0, 12);
    }
    if (!interrupted) {
        set_palette(16, 240, white, 1);
        interrupted = pause_nis(0, 12);
    }
    if (!interrupted) {
        dispose_handle(nis_palette);
        nis_palette = SHIP_PALETTE(5);
        clear_nis(&port.draw, 0);
        add_nis(&port.draw, shapes, 7, 40, 32, 0);
        play_sound(SHIP_MUSIC + 1);
        copy_nis(&port.draw);
        if (!interrupted)
            interrupted = pause_nis(0, 18);
    }
    if (!interrupted) {
        dispose_handle(nis_palette);
        nis_palette = SHIP_PALETTE(2);
        apply_palette_colors(-1, lock_handle(nis_palette));
        interrupted = pause_nis(0, 6);
    }
    if (!interrupted) {
        clear_nis(&port.draw, 0); add_nis(&port.draw, shapes, 3, 39, 32, 0); add_nis(&port.draw, shapes, 8, 38, 86, 10); cut_nis(&port.draw);
        if (!interrupted) interrupted = pause_nis(0, 114);
    }
    if (!interrupted) {
        dispose_handle(nis_palette); nis_palette = SHIP_PALETTE(6); clear_nis(&port.draw, 0); add_nis(&port.draw, shapes, 9, 38, 27, 0);
        interrupted = WaitMidiPlayTime(98);
        if (!interrupted) copy_nis(&port.draw);
    }
    if (!interrupted) {
        add_nis(&port.draw, shapes, 10, 40, 33, 10); cut_nis(&port.draw);
        if (!interrupted)
            interrupted = pause_nis(0, 6);
    }
    if (!interrupted) {
        if (music_options & 3)
            play_sound(30500);
        clear_nis(&port.draw, 0);
        add_nis(&port.draw, shapes, 9, 38, 27, 0);
        add_nis(&port.draw, shapes, 11, 40, 32, 10);
        cut_nis(&port.draw);
        if (!interrupted)
            interrupted = pause_nis(0, 6);
    }
    if (!interrupted) {
        clear_nis(&port.draw, 0);
        add_nis(&port.draw, shapes, 9, 38, 27, 0);
        add_nis(&port.draw, shapes, 12, 39, 32, 10);
        cut_nis(&port.draw);
        if (!interrupted)
            interrupted = pause_nis(0, 6);
    }
    if (!interrupted) {
        clear_nis(&port.draw, 0);
        add_nis(&port.draw, shapes, 13, 39, 31, 0);
        cut_nis(&port.draw);
        if (!interrupted)
            interrupted = pause_nis(0, 6);
    }
    if (!interrupted) {
        set_palette(16, 240, white, 1);
        delay_count = 12;
        interrupted = pause_nis(0, 12);
    }
    if (!interrupted) {
        clear_nis(&port.draw, 0);
        add_nis(&port.draw, shapes, 17, 40, 31, 0);
        interrupted = wait_timer(0);
        if (!interrupted) {
            apply_palette_colors(-2, 0L);
            interrupted = fade_nis(&port.draw, 0);
            if (!interrupted)
                interrupted = pause_nis(0, 18);
        }
    }
    if (!interrupted)
        interrupted = WaitMidiPlayTime(99);
    if (!interrupted)
        play_sound(10262);
    if (!interrupted) {
        add_nis(&port.draw, shapes, 14, 67, 45, 10);
        cut_nis(&port.draw);
        if (!interrupted)
            interrupted = pause_nis(0, 6);
    }
    if (!interrupted) {
        clear_nis(&port.draw, 0);
        add_nis(&port.draw, shapes, 17, 40, 31, 0);
        add_nis(&port.draw, shapes, 15, 78, 44, 10);
        cut_nis(&port.draw);
        if (!interrupted)
            interrupted = pause_nis(0, 6);
    }
    if (!interrupted) {
        clear_nis(&port.draw, 0);
        add_nis(&port.draw, shapes, 17, 40, 31, 0);
        add_nis(&port.draw, shapes, 16, 77, 46, 10);
        cut_nis(&port.draw);
        if (!interrupted)
            interrupted = pause_nis(0, 12);
    }
    if (!interrupted) {
        interrupted = WaitMidiPlayTime(100);
        if (!interrupted)
            interrupted = await_palette(0, -1, 4);
    }

    finish_palette_effect();
    if (interrupted)
        dispatch_resource_pointer(0L);
    dispose_handle(nis_palette);
    FreeDialog();
    dispose_handle(shapes);
    close_temporary_port(&port);
    return interrupted;
}

#undef SHIP_PALETTE

extern Handle prepare_sound(int cue);
extern void stop_current_sound(int sound);
extern Rect *get_shape_rect(Rect far *bounds, Handle shapes, int item, int x, int y);
extern void add_shape_nis(DrawPort *port, Handle shape, int x, int y, int mode);
int dissolve_nis(DrawPort *port, unsigned int duration, const Rect far *bounds);
extern Rect far *union_rect(Rect far *dst, const Rect far *a, const Rect far *b);
extern void paint_rle(const void far *shape, int x, int y, int mode);

typedef union {
    DrawPort draw;
    char saved_state[44];
} NisTemporaryPort;

enum { OPENING_SCENE = 27000 };

int opening_one(void)
{
    int interrupted;
    Handle shapes;
    Rect bounds;
    NisTemporaryPort port;
    midi.play_time = 0;
    play_sound(25010);
    open_temporary_port(&port, &nis_clip);
    dialog_shape = hGetSL(25000, 1);
    shapes = hGetSL(OPENING_SCENE, -1);
    GetDialog();
    set_palette(0, 256, 0L, 1);
    PaintRLEShapeItem(dialog_shape, 1, 0, 0, 0);
    fill_rect(&nis_clip, 212);
    draw_nis_text(0, 0);
    copy_port(&port.draw, draw_port, &nis_clip, &nis_clip, 0); nis_palette = get_resource(0x50414c54UL, OPENING_SCENE + 1);
    interrupted = await_palette(nis_palette, -1, 0);
    if (!interrupted) {
        clear_nis(&port.draw, 212); add_nis(&port.draw, shapes, 4, 98, 62, 10);
        get_shape_rect(&bounds, shapes, 4, 98, 62); interrupted = WaitMidiPlayTime(97);
        if (!interrupted) interrupted = dissolve_nis(&port.draw, 45, &bounds); if (!interrupted) interrupted = pause_nis(0, 120);
    }
    if (!interrupted) {
        clear_nis(&port.draw, 212); get_shape_rect(&bounds, shapes, 4, 98, 62);
        interrupted = dissolve_nis(&port.draw, 45, &bounds); if (!interrupted) interrupted = pause_nis(0, 48);
    }
    if (!interrupted) {
        add_nis(&port.draw, shapes, 5, 75, 68, 10); get_shape_rect(&bounds, shapes, 5, 75, 68);
        interrupted = WaitMidiPlayTime(98); if (!interrupted) interrupted = dissolve_nis(&port.draw, 45, &bounds);
        if (!interrupted) interrupted = pause_nis(0, 120);
    }
    if (interrupted)
        goto paint_next_scene;
    clear_nis(&port.draw, 212); get_shape_rect(&bounds, shapes, 5, 75, 68);
    interrupted = dissolve_nis(&port.draw, 45, &bounds); if (!interrupted) interrupted = pause_nis(0, 48);
paint_next_scene:
    if (!interrupted) {
        clear_nis(&port.draw, 0);
        add_nis(&port.draw, shapes, 6, 40, 32, 0);
        interrupted = await_palette(0, -2, 0);
        if (!interrupted) {
            copy_port(draw_port, &port.draw, &nis_clip, &nis_clip, 0);
            interrupted = WaitMidiPlayTime(99);
            if (!interrupted) interrupted = await_palette(nis_palette, -1, 0);
        }
    }
    if (!interrupted) {
        start_timed_music(OPENING_SCENE + 1, 257);
        decoration_shape = 4;
        decoration_x = 88;
        decoration_y = 167;
        draw_nis_text(OPENING_SCENE, pending_decoration = 1);
        interrupted = pause_nis(0, 90);
    }
    if (!interrupted) {
        draw_nis_text(OPENING_SCENE, 2);
        interrupted = await_timed_music(OPENING_SCENE + 1);
    }
    if (!interrupted) {
        delay_count = 45;
        prepare_sound(OPENING_SCENE + 2);
        interrupted = wait_timer(0);
        if (!interrupted) interrupted = run_nis_dialog(OPENING_SCENE, 3, 2, 184);
    }
    if (!interrupted) {
        delay_count = 85;
        prepare_sound(OPENING_SCENE + 3);
        wait_timer(0);
        start_timed_music(OPENING_SCENE + 3, 254);
        delay_count = 170;
        draw_nis_text(OPENING_SCENE, 4);
        dispose_handle(nis_palette);
        nis_palette = get_resource(0x50414c54UL, OPENING_SCENE + 2);
        clear_nis(&port.draw, 0);
        add_nis(&port.draw, shapes, 7, 39, 31, 0);
        interrupted = wait_timer(0);
        if (!interrupted) {
            draw_nis_text(OPENING_SCENE, 5);
            interrupted = await_palette(0, -2, 0);
            if (!interrupted) {
                copy_port(draw_port, &port.draw, &nis_clip, &nis_clip, 0);
                interrupted = WaitMidiPlayTime(100);
                draw_nis_text(0, 0);
                if (!interrupted) interrupted = await_palette(nis_palette, -1, 0);
            }
        }
    }
    if (!interrupted) {
        prepare_sound(OPENING_SCENE + 4);
        interrupted = WaitMidiPlayTime(101);
        if (!interrupted) interrupted = run_nis_dialog(OPENING_SCENE, 6, 4, 223);
        if (!interrupted) interrupted = pause_nis(0, 20);
    }
    if (!interrupted) interrupted = run_nis_dialog(OPENING_SCENE, 7, 5, 135);
    if (!interrupted) {
        interrupted = WaitMidiPlayTime(102);
        start_timed_music(OPENING_SCENE + 6, 197);
        draw_nis_text(OPENING_SCENE, 8); if (!interrupted) interrupted = pause_nis(0, 150);
    }
    if (interrupted)
        goto next_dialog;
    add_nis(&port.draw, shapes, 8, 40, 31, 10); get_shape_rect(&bounds, shapes, 8, 40, 31);
    interrupted = dissolve_nis(&port.draw, 45, &bounds); if (!interrupted) interrupted = await_timed_music(OPENING_SCENE + 6);
    if (!interrupted) interrupted = pause_nis(0, 30);
next_dialog:
    if (!interrupted) {
        interrupted = run_nis_dialog(OPENING_SCENE, 9, 7, 131); if (!interrupted) interrupted = pause_nis(0, 20);
    }
    if (!interrupted) {
        draw_nis_text(0, 0); clear_nis(&port.draw, 0);
        add_nis(&port.draw, shapes, 10, 39, 30, 0); interrupted = dissolve_nis(&port.draw, 60, &nis_clip);
        if (!interrupted) interrupted = pause_nis(0, 18);
    }
    if (!interrupted) {
        interrupted = run_nis_dialog(OPENING_SCENE, 10, 8, 250); if (!interrupted) interrupted = pause_nis(0, 20);
    }
    if (!interrupted) {
        start_timed_music(OPENING_SCENE + 9, 236); draw_nis_text(OPENING_SCENE, 11);
        add_nis(&port.draw, shapes, 9, 39, 30, 10); interrupted = dissolve_nis(&port.draw, 45, &nis_clip);
        if (!interrupted) interrupted = await_timed_music(OPENING_SCENE + 9);
    }
    if (!interrupted) {
        dispose_handle(nis_palette); nis_palette = get_resource(0x50414c54UL, OPENING_SCENE + 3);
        clear_nis(&port.draw, 0); add_nis(&port.draw, shapes, 12, 40, 32, 0);
        interrupted = await_sound(25010);
        if (!interrupted) {
            draw_nis_text(0, 0); interrupted = fade_nis(&port.draw, 0);
            midi.play_time = 0;
            if (!interrupted) {
                play_sound(25011);
            }
        }
    }
    if (!interrupted) {
        interrupted = run_nis_dialog(OPENING_SCENE, 12, 10, 124); if (!interrupted) interrupted = pause_nis(0, 20);
    }
    if (!interrupted) {
        interrupted = run_nis_dialog(OPENING_SCENE, 13, 11, 117); if (!interrupted) interrupted = pause_nis(0, 20);
    }
    if (!interrupted) interrupted = run_nis_dialog(OPENING_SCENE, 14, 12, 176);
    if (!interrupted) {
        draw_nis_text(0, 0); dispose_handle(nis_palette);
        nis_palette = get_resource(0x50414c54UL, OPENING_SCENE + 4); clear_nis(&port.draw, 0);
        add_nis(&port.draw, shapes, 11, 40, 32, 0); interrupted = WaitMidiPlayTime(97);
        if (!interrupted) interrupted = fade_nis(&port.draw, 0);
    }
    if (!interrupted) {
        prepare_sound(OPENING_SCENE + 13); interrupted = WaitMidiPlayTime(98);
        if (!interrupted) interrupted = run_nis_dialog(OPENING_SCENE, 15, 13, 208); if (!interrupted) interrupted = pause_nis(0, 20);
    }
    if (!interrupted) {
        prepare_sound(OPENING_SCENE + 14); interrupted = WaitMidiPlayTime(99);
        if (!interrupted) interrupted = run_nis_dialog(OPENING_SCENE, 16, 14, 145); if (!interrupted) interrupted = pause_nis(0, 90);
    }
    dispose_handle(nis_palette); finish_palette_effect();
    if (interrupted) {
        dispatch_resource_pointer(0L);
    }
    FreeDialog(); dispose_handle(shapes);
    close_temporary_port(&port); return interrupted;
}

int opening_two(void)
{
    struct {
        int step;
        Rect next_bounds;
        Handle shapes, foreground;
        NisTemporaryPort port;
        Rect bounds;
        Handle right_actor, left_actor, background;
        int interrupted;
    } scene;

    open_temporary_port(&scene.port, &nis_clip);
    dialog_shape = hGetSL(25000, 1);
    scene.shapes = hGetSL(OPENING_SCENE, -1);
    GetDialog(); set_palette(0, 256, 0L, 1);
    PaintRLEShapeItem(dialog_shape, 1, 0, 0, 0);
    draw_nis_text(0, 0);
    nis_palette = get_resource(0x50414c54UL, OPENING_SCENE + 5);
    scene.interrupted = WaitMidiPlayTime(106);
    if (!scene.interrupted) {
        scene.interrupted = await_palette(nis_palette, 1, 2);
        if (!scene.interrupted) {
            add_nis(&scene.port.draw, scene.shapes, 13, 9, 31, 0);
            add_nis(&scene.port.draw, scene.shapes, 14, 160, 32, 10);
                add_nis(&scene.port.draw, scene.shapes, 15, 193, 55, 10);
            get_shape_rect(&scene.bounds, scene.shapes, 15, 193, 55);
            copy_port(draw_port, &scene.port.draw, &nis_clip, &nis_clip, 0);
            prepare_sound(25012);
            scene.interrupted = await_sound(25011);
        }
        if (!scene.interrupted) {
            midi.play_time = 0; play_sound(25012);
            scene.interrupted = await_palette(nis_palette, -1, 0);
        }
    }
    if (!scene.interrupted) {
        decoration_shape = 3;
        decoration_x = 99; decoration_y = 167;
        pending_decoration = 1;
        prepare_sound(OPENING_SCENE + 15);
        scene.interrupted = WaitMidiPlayTime(97);
        if (!scene.interrupted) {
            start_timed_music(OPENING_SCENE + 15, 231);
            draw_nis_text(OPENING_SCENE, 17);
            scene.interrupted = pause_nis(0, 60);
            if (!scene.interrupted) draw_nis_text(OPENING_SCENE, 18);
        }
    }
    if (!scene.interrupted) {
        clear_nis(&scene.port.draw, 0);
        add_nis(&scene.port.draw, scene.shapes, 13, 9, 31, 0);
        add_nis(&scene.port.draw, scene.shapes, 14, 160, 32, 0);
        add_nis(&scene.port.draw, scene.shapes, 16, 192, 55, 10);
        get_shape_rect(&scene.next_bounds, scene.shapes, 16, 192, 55);
        prepare_sound(OPENING_SCENE + 16);
        await_timed_music(OPENING_SCENE + 15);
        scene.interrupted = WaitMidiPlayTime(98);
        if (!scene.interrupted) {
            start_timed_music(OPENING_SCENE + 16, 164);
            draw_nis_text(OPENING_SCENE, 19);
            scene.interrupted = dissolve_nis(&scene.port.draw, 300, union_rect(&scene.bounds, &scene.bounds, &scene.next_bounds));
        }
    }
    if (!scene.interrupted) await_sound(0);
    if (!scene.interrupted) {
        draw_nis_text(0, 0);
        scene.background = load_shape_item(scene.shapes, 17, 1);
        scene.left_actor = load_shape_item(scene.shapes, 13, 1);
        scene.right_actor = load_shape_item(scene.shapes, 14, 1);
        scene.foreground = load_shape_item(scene.shapes, 16, 1);
        play_sound(30503);
        for (scene.step = 1; scene.step < 17; ++scene.step) {
            if (!scene.interrupted) {
                delay_count = 14 - scene.step < 0 ? 0 : 14 - scene.step;
                draw_port = &scene.port.draw;
                paint_rle(lock_handle(scene.background), 39, 32, 0);
                paint_rle(lock_handle(scene.left_actor), 9 - (scene.step << 3), 31, 0);
                paint_rle(lock_handle(scene.right_actor), (scene.step + 20) << 3, 32, 0);
                paint_rle(lock_handle(scene.foreground), 192, 55, 10);
                draw_port = &nis_draw_port;
                cut_nis(&scene.port.draw);
                scene.interrupted = wait_timer(0);
            }
        }
        dispose_handle(scene.foreground);
        dispose_handle(scene.right_actor);
        dispose_handle(scene.left_actor);
        dispose_handle(scene.background);
        if (!scene.interrupted) scene.interrupted = await_sound(30503);
    }
    if (!scene.interrupted) {
        midi.play_time = 0; play_sound(25013);
        start_timed_music(OPENING_SCENE + 17, 240);
        draw_nis_text(OPENING_SCENE, 20);
    }
    if (!scene.interrupted) {
        clear_nis(&scene.port.draw, 0);
        add_nis(&scene.port.draw, scene.shapes, 18, 40, 32, 0);
        scene.interrupted = dissolve_nis(&scene.port.draw, 60, &nis_clip);
        if (!scene.interrupted) scene.interrupted = pause_nis(scene.interrupted, 42);
    }
    if (!scene.interrupted) {
        draw_nis_text(OPENING_SCENE, 21);
        clear_nis(&scene.port.draw, 0);
        add_nis(&scene.port.draw, scene.shapes, 19, 39, 32, 0);
        scene.interrupted = dissolve_nis(&scene.port.draw, 60, &nis_clip);
        if (!scene.interrupted) scene.interrupted = pause_nis(scene.interrupted, 42);
    }
    if (!scene.interrupted) scene.interrupted = await_timed_music(OPENING_SCENE + 17);
    if (!scene.interrupted) {
        clear_nis(&scene.port.draw, 0);
        add_nis(&scene.port.draw, scene.shapes, 20, 40, 32, 0);
        scene.interrupted = dissolve_nis(&scene.port.draw, 60, &nis_clip);
        if (!scene.interrupted) scene.interrupted = pause_nis(scene.interrupted, 42);
    }
    if (!scene.interrupted) {
        clear_nis(&scene.port.draw, 0);
        add_nis(&scene.port.draw, scene.shapes, 21, 40, 31, 0);
        add_nis(&scene.port.draw, scene.shapes, 24, 126, 44, 10);
        add_nis(&scene.port.draw, scene.shapes, 23, 65, 79, 10);
        add_nis(&scene.port.draw, scene.shapes, 25, 167, 50, 10);
        scene.interrupted = dissolve_nis(&scene.port.draw, 60, &nis_clip);
        if (!scene.interrupted) {
            draw_nis_text(scene.interrupted, scene.interrupted);
            scene.interrupted = pause_nis(0, 42);
        }
    }
    if (!scene.interrupted) {
        scene.interrupted = run_nis_dialog(OPENING_SCENE, 22, 18, 182);
        if (!scene.interrupted) scene.interrupted = pause_nis(scene.interrupted, 20);
    }
    if (!scene.interrupted) {
        dispose_handle(nis_palette);
        nis_palette = get_resource(0x50414c54UL, OPENING_SCENE + 6);
        draw_nis_text(0, 0);
        clear_nis(&scene.port.draw, 0);
        add_nis(&scene.port.draw, scene.shapes, 33, 39, 32, 0);
        add_nis(&scene.port.draw, scene.shapes, 34, 40, 32, 10);
        if (!scene.interrupted) scene.interrupted = WaitMidiPlayTime(97);
        copy_nis(&scene.port.draw);
        scene.interrupted = pause_nis(0, 48);
    }
    if (!scene.interrupted) scene.interrupted = WaitMidiPlayTime(98);
    if (!scene.interrupted) {
        scene.interrupted = run_nis_dialog(OPENING_SCENE, 23, 19, 274);
        if (!scene.interrupted) scene.interrupted = pause_nis(scene.interrupted, 20);
    }
    if (!scene.interrupted) {
        scene.background = load_shape_item(scene.shapes, 21, 1);
        scene.left_actor = load_shape_item(scene.shapes, 22, 1);
        scene.right_actor = load_shape_item(scene.shapes, 25, 1);
        dispose_handle(nis_palette);
        nis_palette = get_resource(0x50414c54UL, OPENING_SCENE + 5);
        draw_nis_text(0, 0);
        add_shape_nis(&scene.port.draw, scene.background, 40, 31, 0);
        add_nis(&scene.port.draw, scene.shapes, 26, 113, 51, 10);
        add_shape_nis(&scene.port.draw, scene.left_actor, 58, 78, 10);
        add_shape_nis(&scene.port.draw, scene.right_actor, 167, 50, 10);
        scene.interrupted = WaitMidiPlayTime(99);
        if (!scene.interrupted) {
            copy_nis(&scene.port.draw);
            scene.interrupted = WaitMidiPlayTime(100);
        }
    } else {
        scene.background = 0;
    }
    if (!scene.interrupted) {
        add_shape_nis(&scene.port.draw, scene.background, 40, 31, 0);
        add_nis(&scene.port.draw, scene.shapes, 27, 113, 50, 10);
        add_shape_nis(&scene.port.draw, scene.left_actor, 58, 78, 10);
        add_shape_nis(&scene.port.draw, scene.right_actor, 167, 50, 10);
        cut_nis(&scene.port.draw);
    }
    if (!scene.interrupted) {
        add_shape_nis(&scene.port.draw, scene.background, 40, 31, 0);
        add_nis(&scene.port.draw, scene.shapes, 28, 115, 50, 10);
        add_shape_nis(&scene.port.draw, scene.left_actor, 58, 78, 10);
        add_shape_nis(&scene.port.draw, scene.right_actor, 167, 50, 10);
        cut_nis(&scene.port.draw);
        scene.interrupted = pause_nis(0, 6);
    }
    if (!scene.interrupted) {
        add_shape_nis(&scene.port.draw, scene.background, 40, 31, 0);
        add_nis(&scene.port.draw, scene.shapes, 29, 113, 48, 10);
        add_shape_nis(&scene.port.draw, scene.left_actor, 58, 78, 10);
        add_shape_nis(&scene.port.draw, scene.right_actor, 167, 50, 10);
        cut_nis(&scene.port.draw);
        scene.interrupted = pause_nis(0, 6);
    }
    if (!scene.interrupted) {
        add_shape_nis(&scene.port.draw, scene.background, 40, 31, 0);
        add_nis(&scene.port.draw, scene.shapes, 30, 112, 48, 10);
        add_shape_nis(&scene.port.draw, scene.left_actor, 58, 78, 10);
        add_shape_nis(&scene.port.draw, scene.right_actor, 167, 50, 10);
        cut_nis(&scene.port.draw);
        scene.interrupted = pause_nis(0, 6);
    }
    if (!scene.interrupted) {
        add_shape_nis(&scene.port.draw, scene.background, 40, 31, 0);
        add_nis(&scene.port.draw, scene.shapes, 31, 112, 46, 10);
        add_shape_nis(&scene.port.draw, scene.left_actor, 58, 78, 10);
        add_shape_nis(&scene.port.draw, scene.right_actor, 167, 50, 10);
        cut_nis(&scene.port.draw);
        scene.interrupted = pause_nis(0, 6);
    }
    if (!scene.interrupted) {
        add_shape_nis(&scene.port.draw, scene.background, 40, 31, 0);
        add_nis(&scene.port.draw, scene.shapes, 32, 89, 47, 10);
        add_shape_nis(&scene.port.draw, scene.left_actor, 58, 78, 10);
        add_shape_nis(&scene.port.draw, scene.right_actor, 167, 50, 10);
        cut_nis(&scene.port.draw);
        scene.interrupted = pause_nis(0, 18);
    }
    if (scene.background) {
        dispose_handle(scene.right_actor);
        dispose_handle(scene.left_actor);
        dispose_handle(scene.background);
    }
    if (!scene.interrupted) {
        if (!scene.interrupted) scene.interrupted = run_nis_dialog(OPENING_SCENE, 24, 20, 129);
        if (!scene.interrupted) scene.interrupted = pause_nis(0, 20);
    }
    if (!scene.interrupted) {
        dispose_handle(nis_palette);
        nis_palette = get_resource(0x50414c54UL, OPENING_SCENE + 6);
        draw_nis_text(0, 0);
        clear_nis(&scene.port.draw, 0);
        add_nis(&scene.port.draw, scene.shapes, 35, 40, 32, 0);
        copy_nis(&scene.port.draw);
        midi.play_time = 0; play_sound(25014);
        scene.interrupted = run_nis_dialog(OPENING_SCENE, 25, 21, 174);
    }
    if (!scene.interrupted) {
        draw_nis_text(0, 0);
        clear_nis(&scene.port.draw, 0);
        add_nis(&scene.port.draw, scene.shapes, 33, 39, 32, 0);
        add_nis(&scene.port.draw, scene.shapes, 36, 72, 39, 10);
        copy_nis(&scene.port.draw);
    }
    if (!scene.interrupted) {
        delay_count = 12; clear_nis(&scene.port.draw, 0);
        add_nis(&scene.port.draw, scene.shapes, 33, 39, 32, 0);
        add_nis(&scene.port.draw, scene.shapes, 37, 46, 38, 10);
        scene.interrupted = wait_timer(0);
        if (!scene.interrupted) {
            start_timed_music(OPENING_SCENE + 22, 60);
            draw_nis_text(OPENING_SCENE, 26);
            cut_nis(&scene.port.draw);
            scene.interrupted = await_timed_music(OPENING_SCENE + 22);
            if (!scene.interrupted) scene.interrupted = pause_nis(scene.interrupted, 30);
        }
    }
    if (!scene.interrupted) {
        draw_nis_text(0, 0);
        clear_nis(&scene.port.draw, 0);
        add_nis(&scene.port.draw, scene.shapes, 38, 41, 32, 0);
        copy_nis(&scene.port.draw);
        if (!scene.interrupted) scene.interrupted = pause_nis(0, 18);
    }
    if (!scene.interrupted) {
        scene.interrupted = run_nis_dialog(OPENING_SCENE, 27, 23, 98);
        if (!scene.interrupted) scene.interrupted = pause_nis(scene.interrupted, 20);
    }
    if (!scene.interrupted) {
        start_timed_music(OPENING_SCENE + 24, 236);
        draw_nis_text(OPENING_SCENE, 28);
        clear_nis(&scene.port.draw, 0);
        add_nis(&scene.port.draw, scene.shapes, 33, 39, 32, 0);
        add_nis(&scene.port.draw, scene.shapes, 39, 40, 32, 10);
        copy_nis(&scene.port.draw);
    }
    if (!scene.interrupted) scene.interrupted = await_timed_music(OPENING_SCENE + 24);
    if (!scene.interrupted) {
        draw_nis_text(0, 0);
        clear_nis(&scene.port.draw, 0);
        add_nis(&scene.port.draw, scene.shapes, 43, 40, 32, 0);
        add_nis(&scene.port.draw, scene.shapes, 40, 128, 31, 10);
        get_shape_rect(&scene.bounds, scene.shapes, 40, 128, 31);
        copy_nis(&scene.port.draw);
        if (!scene.interrupted) scene.interrupted = pause_nis(0, 18);
    }
    if (!scene.interrupted) {
        scene.interrupted = run_nis_dialog(OPENING_SCENE, 29, 25, 97);
        if (!scene.interrupted) scene.interrupted = pause_nis(scene.interrupted, 18);
    }
    if (!scene.interrupted) {
        clear_nis(&scene.port.draw, 0);
        add_nis(&scene.port.draw, scene.shapes, 43, 40, 32, 0);
        add_nis(&scene.port.draw, scene.shapes, 41, 125, 28, 10);
        get_shape_rect(&scene.next_bounds, scene.shapes, 41, 125, 28);
        midi.play_time = 0; play_sound(25015);
        start_timed_music(OPENING_SCENE + 26, 93);
        draw_nis_text(OPENING_SCENE, 30);
        scene.interrupted = dissolve_nis(&scene.port.draw, 75, union_rect(&scene.bounds, &scene.bounds, &scene.next_bounds));
        if (!scene.interrupted) scene.interrupted = await_timed_music(OPENING_SCENE + 26);
        if (!scene.interrupted) scene.interrupted = pause_nis(scene.interrupted, 30);
    }
    if (!scene.interrupted) {
        clear_nis(&scene.port.draw, 0);
        add_nis(&scene.port.draw, scene.shapes, 43, 40, 32, 0);
        add_nis(&scene.port.draw, scene.shapes, 42, 125, 32, 10);
        get_shape_rect(&scene.bounds, scene.shapes, 42, 125, 32);
        scene.interrupted = dissolve_nis(&scene.port.draw, 75, union_rect(&scene.bounds, &scene.bounds, &scene.next_bounds));
        if (!scene.interrupted) scene.interrupted = pause_nis(scene.interrupted, 30);
    }
    if (!scene.interrupted) {
        dispose_handle(nis_palette);
        nis_palette = get_resource(0x50414c54UL, OPENING_SCENE + 7);
        draw_nis_text(0, 0);
        clear_nis(&scene.port.draw, 0);
        add_nis(&scene.port.draw, scene.shapes, 45, 40, 32, 0);
        copy_nis(&scene.port.draw);
        scene.interrupted = pause_nis(0, 48);
    }
    if (!scene.interrupted) {
        add_nis(&scene.port.draw, scene.shapes, 44, 82, 38, 10);
        get_shape_rect(&scene.bounds, scene.shapes, 44, 82, 38);
        prepare_sound(10049);
        stop_current_sound(0); play_sound(10049);
        scene.interrupted = dissolve_nis(&scene.port.draw, 90, &scene.bounds);
        if (!scene.interrupted) scene.interrupted = pause_nis(scene.interrupted, 30);
        if (!scene.interrupted) scene.interrupted = await_palette(scene.interrupted, -1, 2);
    }
    dispose_handle(nis_palette);
    finish_palette_effect();
    if (scene.interrupted) dispatch_resource_pointer(0L);
    FreeDialog(); dispose_handle(scene.shapes);
    close_temporary_port(&scene.port);
    return scene.interrupted;
}






extern Rect nis_clip;
extern Handle nis_palette;
extern void paint_rle(const void far *shape, int x, int y, int mode);

void clear_nis(DrawPort *port, int color)
{
    DrawPort *saved = draw_port;

    draw_port = port;
    fill_rect(&nis_clip, color);
    draw_port = saved;
}

void add_nis(DrawPort *port, Handle shapes, int item, int x, int y, int mode)
{
    DrawPort *saved = draw_port;

    draw_port = port;
    PaintRLEShapeItem(shapes, item, x, y, mode);
    draw_port = saved;
}

void add_shape_nis(DrawPort *port, Handle shape, int x, int y, int mode)
{
    DrawPort *saved = draw_port;

    draw_port = port;
    paint_rle(lock_handle(shape), x, y, mode);
    draw_port = saved;
}

void copy_nis(const void *source)
{
    apply_palette_colors(-2, 0L);
    copy_port(draw_port, source, &nis_clip, &nis_clip, 0);
    pause_nis(0, 2);
    apply_palette_colors(-1, lock_handle(nis_palette));
}

void cut_nis(const void *source)
{
    copy_port(draw_port, source, &nis_clip, &nis_clip, 0);
}

int fade_nis(const void *source, int duration)
{
    int interrupted = await_palette(0, -2, duration);

    if (!interrupted)
        copy_port(draw_port, source, &nis_clip, &nis_clip, 0);
    if (!interrupted)
        interrupted = await_palette(nis_palette, -1, duration);
    return interrupted;
}

int dissolve_nis(DrawPort *port, unsigned int duration, const Rect far *bounds)
{
    char saved_state[44];
    int interrupted;
    DrawPort *saved = draw_port;

    draw_port = open_temporary_port(saved_state, bounds);
    copy_port((DrawPort *)saved_state, &nis_draw_port, bounds, bounds, 0);
    interrupted = animate_port(1, port, bounds, duration);
    draw_port = saved;
    close_temporary_port(saved_state);
    return interrupted;
}

int ending(void);
int discovery(void);

int dispatch_nis(int sequence)
{
    int interrupted;
    int opened_nis_file;

    open_nis_files(nis_data_files);
    install_nis_input();
    if (sequence != 11) {
        opened_nis_file = 1;
        open_data_file(nis_file);
    } else {
        opened_nis_file = 0;
    }

    if (sequence == 7)
        interrupted = opening_one();
    else if (sequence == 8)
        interrupted = opening_two();
    else if (sequence == 9)
        interrupted = aboard_ship();
    else if (sequence == 10)
        interrupted = discovery();
    else if (sequence == 11)
        interrupted = ending();

    if (opened_nis_file)
        close_data_file(nis_file);
    poll_input = saved_poll_input;
    close_nis_files(nis_data_files);
    return interrupted;
}

extern Handle prepare_sound(int cue);

#pragma optimize("e", off)
int run_nis_dialog(int resource, int line, int voice, unsigned int duration)
{
    register int string_resource, cue;
    cue = *(volatile int *)&voice, string_resource = resource;
    prepare_sound(cue += string_resource); draw_nis_text(string_resource, line);
    return play_timed_music(cue, duration);
}
#pragma optimize("", on)

Rect *get_shape_rect(Rect far *bounds, Handle shapes, int item, int x, int y)
{
    const unsigned int far *list = lock_handle(shapes);
    const Rect far *shape = lock_handle(get_resource(0x53484150UL, list[0] + item - 1));

    bounds->left = x;
    bounds->top = y;
    bounds->right = x + shape->left;
    bounds->bottom = y + shape->top;
    return (Rect *)bounds;
}

extern void select_resource_file(const char *name);
extern void close_resource_file(const char *name);
extern void push_clip_rect(const Rect far *bounds);
extern void pop_clip_rect(void);
extern ImagePort *save_screen_patch(const Rect far *bounds);
extern void dispose_screen_patch(ImagePort *patch);
extern Handle duplicate_handle(Handle shape);
extern Rect ending_fill_bounds, ending_patch_bounds;
extern Handle ending_shapes, ending_sprite;
extern Handle ending_palette_one, ending_palette_two, ending_palette_three, ending_palette_four;
extern Handle discovery_shapes;

extern const char nis_music_file[];
int animate_ending(void);
int ending_callback(void *playback, int frame);
int discovery_callback(AnimationState *playback, int frame);

enum { ENDING_SCENE = 30000, DISCOVERY_SCENE = 29000 };

int ending(void)
{
    int interrupted;
    ImagePort *portrait_patch;
    Rect bounds;
    NisTemporaryPort port;

    animate_ending();
    open_data_file(nis_file);
    open_temporary_port(&port, &nis_clip);
    dialog_shape = hGetSL(25000, 1);
    ending_shapes = hGetSL(ENDING_SCENE, -1);
    GetDialog();
    set_palette(0, 256, 0L, 1);
    PaintRLEShapeItem(dialog_shape, 1, 0, 0, 0);
    push_clip_rect(&nis_clip);
    PaintRLEShapeItem(ending_shapes, 2, 40, 32, 0);
    PaintRLEShapeItem(ending_shapes, 5, 52, 90, 10);
    pop_clip_rect();
    draw_nis_text(0, 0);
    copy_port(&port.draw, draw_port, &nis_clip, &nis_clip, 0);
    nis_palette = get_resource(0x50414c54UL, ENDING_SCENE + 1);
    play_sound(ENDING_SCENE + 10);
    interrupted = ((int (far pascal *)(Handle, int, unsigned int))await_palette)(nis_palette, -1, 2U);
    if (!interrupted) {
        interrupted = pause_nis(0, 48);
    }
    if (!interrupted) {
        add_nis(&port.draw, ending_shapes, 3, 160, 45, 10);
        get_shape_rect(&bounds, ending_shapes, 3, 160, 45);
        interrupted = WaitMidiPlayTime(97);
        if (!interrupted) interrupted = dissolve_nis(&port.draw, 15, &bounds);
        if (!interrupted) interrupted = play_nis_animation(ending_shapes, ENDING_SCENE, 2, 1, ending_callback, 0, 0, 1);
        if (!interrupted) pause_nis(0, 40);
    }
    if (!interrupted) interrupted = WaitMidiPlayTime(98);
    if (!interrupted) {
        decoration_shape = 7;
        decoration_x = 77; decoration_y = 159;
        pending_decoration = 1;
        start_timed_music(ENDING_SCENE + 1, 181);
        draw_nis_text(ENDING_SCENE, 1);
        add_nis(&port.draw, ending_shapes, 7, 40, 32, 0);
        get_shape_rect(&bounds, ending_shapes, 44, 161, 64);
        portrait_patch = save_screen_patch(&bounds);
        prepare_sound(ENDING_SCENE + 2);
        PaintRLEShapeItem(ending_shapes, 44, 161, 64, 10);
        delay_count = 6; interrupted = wait_timer(0);
        dispose_screen_patch(portrait_patch);
        if (!interrupted) interrupted = pause_nis(0, 60);
        if (!interrupted) interrupted = fade_nis(&port.draw, 0);
        if (!interrupted) interrupted = await_timed_music(ENDING_SCENE + 1);
        if (!interrupted) interrupted = pause_nis(0, 40);
    }
    if (!interrupted) interrupted = WaitMidiPlayTime(99);
    if (!interrupted) {
        start_timed_music(ENDING_SCENE + 2, 278);
        draw_nis_text(ENDING_SCENE, 2);
        interrupted = pause_nis(0, 90);
    }
    if (!interrupted) {
        draw_nis_text(ENDING_SCENE, 3);
        interrupted = await_timed_music(ENDING_SCENE + 2);
        if (!interrupted) interrupted = pause_nis(0, 90);
    }
    if (!interrupted) interrupted = WaitMidiPlayTime(100);
    if (!interrupted) interrupted = await_palette(0, -1, 3);
    if (!interrupted) {
        fill_rect(&ending_fill_bounds, 48);
        PaintRLEShapeItem(ending_shapes, 8, 0, 6, 0);
        alternate_port = save_screen_patch(&dialog_rect);
        ending_palette_one = get_resource(0x50414c54UL, ENDING_SCENE + 2);
        ending_palette_two = get_resource(0x50414c54UL, ENDING_SCENE + 3);
        ending_palette_three = get_resource(0x50414c54UL, ENDING_SCENE + 4);
        ending_palette_four = get_resource(0x50414c54UL, ENDING_SCENE + 5);
        prepare_sound(ENDING_SCENE + 4);
        prepare_sound(ENDING_SCENE + 5);
        prepare_sound(ENDING_SCENE + 6);
        prepare_sound(ENDING_SCENE + 7);
        prepare_sound(ENDING_SCENE + 8);
        ending_sprite = duplicate_handle(load_shape_item(ending_shapes, 10, 1));
        interrupted = play_nis_animation(ending_shapes, ENDING_SCENE, 1, 1, ending_callback, 0, 0, 1);
        dispose_handle(ending_sprite);
        dispose_handle(ending_palette_four);
        dispose_handle(ending_palette_three);
        dispose_handle(ending_palette_two); dispose_handle(ending_palette_one);
        dispose_screen_patch(alternate_port);
    }
    finish_palette_effect();
    if (interrupted) dispatch_resource_pointer(0L);
    dispose_handle(nis_palette); FreeDialog();
    dispose_handle(ending_shapes); close_temporary_port(&port);
    close_data_file(nis_file);
    return interrupted;
}

int animate_ending(void)
{
    Handle foreground;
    int interrupted;

    set_palette(0, 256, 0L, 1);
    install_nis_input();
    transition_shapes = hGetSL(25000, -1);
    draw_princess_room(transition_shapes);
    foreground = hGetSL(25001, 1);
    PaintShapeItem(foreground, 15, 166, 139, 10, 0);
    dispose_handle(foreground);
    transition_palette = get_resource(0x50414c54UL, 25000);
    interrupted = play_nis_animation(transition_shapes, 25000, 2, 1, transition_scene_four, 0, 0, 1);
    dispose_handle(transition_palette);
    dispose_handle(transition_shapes);
    if (!interrupted)
        interrupted = pause_nis(0, 60);
    if (!interrupted)
        interrupted = await_palette(0, -1, 0);
    return interrupted;
}

extern void draw_happy_dialog(int resource, int line, int voice);
extern void toggle_playback_coordinates(AnimationState *playback);
extern void far *get_handle_contents(Handle handle);

int ending_callback(void *state, int frame)
{
    Rect bounds;
    int interrupted = 0;
    DrawPort *saved;
    ImagePort *patch;

    switch (frame) {
    case 0:
        interrupted = WaitMidiPlayTime(101);
        if (!interrupted) {
            ((AnimationState *)state)->fade = begin_palette_fade(ending_palette_one, -1, 1);
            draw_happy_dialog(ENDING_SCENE, 4, 4);
        }
        break;
    case 1:
        ((AnimationState *)state)->fade = begin_palette_fade(ending_palette_two, 7, frame);
        break;
    case 2:
        saved = draw_port;
        copy_port(draw_port = &nis_draw_port, alternate_port,
                  &alternate_port->clip, &alternate_port->clip, 0);
        draw_port = saved;
        ((AnimationState *)state)->fade = begin_palette_fade(ending_palette_three, -8, 2);
        break;
    case 3: {
        AnimationState *playback;

        if (!get_handle_contents(ending_sprite)) {
            dispose_handle(ending_sprite);
            ending_sprite = duplicate_handle(load_shape_item(ending_shapes, 10, 1));
        }
        playback = state;
        toggle_playback_coordinates(playback);
        paint_rle(lock_handle(ending_sprite), -playback->port.clip.left,
                  8 - playback->port.clip.top, 0);
        toggle_playback_coordinates(playback);

        saved = draw_port;
        draw_port = &nis_draw_port;
        fill_rect(&ending_fill_bounds, 0);
        paint_rle(lock_handle(ending_sprite), 0, 8, 10);
        draw_port = saved;
        play_sound(ENDING_SCENE + 20);
        playback->fade = begin_palette_fade(ending_palette_four, -1, 1);
    }
        break;
    case 4:
        init_fade_to_black(state, 1);
        break;
    case 5: case 6: case 7: case 8:
        draw_happy_dialog(ENDING_SCENE, frame, frame);
        break;
    case 98: {
        AnimationState *playback = state;

        toggle_playback_coordinates(playback);
        PaintRLEShapeItem(ending_shapes, 2, 40 - playback->port.clip.left,
                          32 - playback->port.clip.top, 0);
        toggle_playback_coordinates(playback);
    }
        break;
    case 99:
        saved = draw_port;
        draw_port = &nis_draw_port;
        get_shape_rect(&bounds, ending_shapes, 44, 161, 64);
        patch = save_screen_patch(&bounds);
        PaintRLEShapeItem(ending_shapes, 44, 161, 64, 10);
        delay_count = 6;
        while (delay_count)
            ;
        dispose_screen_patch(patch);
        draw_port = saved;
        break;
    }
    return interrupted;
}

void draw_happy_dialog(int resource, int line, int voice)
{
    DrawPort *saved = draw_port;

    draw_port = &nis_draw_port;
    play_sound(resource + voice);
    alternate_dialog = 1;
    draw_nis_text(resource, line);
    alternate_dialog = 0;
    draw_port = saved;
}

extern int read_nis_key(void);
extern void show_nis_status(void);
extern void sample_nis_input(void);
extern int nis_skip_requested, nis_button_latched;
extern unsigned char nis_button_held;

int check_nis_input(void)
{
    int key = read_nis_key();

    if (key == 0x1000 || key == 0x11)
        fatal_error(0, 0L);
    if (key == 0x1f00) {
        show_nis_status();
        key = 0;
    }
    if (key == 0x2600)
        return nis_skip_requested = 1;
    if (key == 27 || key == 32)
        return 1;

    sample_nis_input();
    if (nis_button_held) {
        if (!nis_button_latched)
            return 1;
    } else {
        nis_button_latched = 0;
    }
    return 0;
}

void install_nis_input(void)
{
    saved_poll_input = poll_input;
    poll_input = check_nis_input;
    sample_nis_input();
    nis_button_latched = nis_button_held != 0;
}

int discovery(void)
{
    int interrupted;
    Rect bounds, next_bounds;
    NisTemporaryPort port;
    unsigned char white[240][3];

    play_sound(29100);
    open_temporary_port(&port, &nis_clip);
    dialog_shape = hGetSL(25000, 1);
    discovery_shapes = hGetSL(DISCOVERY_SCENE, -1);
    GetDialog(); set_palette(0, 256, 0L, 1);
    PaintRLEShapeItem(dialog_shape, 1, 0, 0, 0);
    draw_nis_text(0, 0);
    PaintRLEShapeItem(discovery_shapes, 2, 40, 32, 0);
    nis_palette = get_resource(0x50414c54UL, DISCOVERY_SCENE + 1);
    interrupted = await_palette(nis_palette, -1, 0);
    if (!interrupted) interrupted = pause_nis(0, 240);
    if (!interrupted) {
        start_timed_music(DISCOVERY_SCENE + 1, 270);
        decoration_shape = 6;
        decoration_x = 96; decoration_y = 168;
        draw_nis_text(DISCOVERY_SCENE, pending_decoration = 1);
        interrupted = pause_nis(0, 60);
    }
    if (!interrupted) {
        draw_nis_text(DISCOVERY_SCENE, 2);
        interrupted = await_timed_music(DISCOVERY_SCENE + 1);
        if (!interrupted) interrupted = pause_nis(0, 90);
    }
    if (!interrupted) {
        interrupted = run_nis_dialog(DISCOVERY_SCENE, 3, 2, 157);
        if (!interrupted) interrupted = pause_nis(0, 40);
    }
    if (!interrupted) {
        interrupted = run_nis_dialog(DISCOVERY_SCENE, 4, 3, 187);
        if (!interrupted) interrupted = pause_nis(0, 20);
    }
    if (!interrupted) {
        draw_nis_text(0, 0); clear_nis(&port.draw, 0);
        add_nis(&port.draw, discovery_shapes, 3, 40, 32, 0);
        add_nis(&port.draw, discovery_shapes, 4, 51, 40, 10);
        get_shape_rect(&bounds, discovery_shapes, 4, 51, 40);
        interrupted = WaitMidiPlayTime(97);
        if (!interrupted) interrupted = fade_nis(&port.draw, 0);
        if (!interrupted) interrupted = pause_nis(0, 24);
    }
    if (!interrupted) {
        interrupted = run_nis_dialog(DISCOVERY_SCENE, 5, 4, 168);
        if (!interrupted) interrupted = pause_nis(0, 20);
    }
    if (!interrupted) {
        interrupted = run_nis_dialog(DISCOVERY_SCENE, 6, 5, 137);
        if (!interrupted) interrupted = pause_nis(0, 20);
    }
    if (!interrupted) {
        interrupted = run_nis_dialog(DISCOVERY_SCENE, 7, 6, 220);
        if (!interrupted) interrupted = pause_nis(0, 60);
    }
    if (!interrupted) interrupted = WaitMidiPlayTime(98);
    if (!interrupted) interrupted = play_timed_music(30501, 0);
    if (!interrupted) {
        draw_nis_text(0, 0); clear_nis(&port.draw, 0);
        add_nis(&port.draw, discovery_shapes, 3, 40, 32, 0);
        add_nis(&port.draw, discovery_shapes, 5, 50, 33, 10);
        get_shape_rect(&next_bounds, discovery_shapes, 5, 50, 33);
        if (!interrupted) play_sound(DISCOVERY_SCENE + 7);
        interrupted = dissolve_nis(&port.draw, 15, union_rect(&bounds, &bounds, &next_bounds));
        if (!interrupted) interrupted = pause_nis(0, 30);
    }
    if (!interrupted) {
        clear_nis(&port.draw, 0);
        add_nis(&port.draw, discovery_shapes, 8, 40, 32, 0);
        copy_nis(&port.draw);
        interrupted = pause_nis(0, 60);
    }
    if (!interrupted) {
        clear_nis(&port.draw, 0);
        add_nis(&port.draw, discovery_shapes, 3, 40, 32, 0);
        add_nis(&port.draw, discovery_shapes, 5, 50, 33, 10);
        copy_nis(&port.draw);
        interrupted = run_nis_dialog(DISCOVERY_SCENE, 8, 8, 66);
        if (!interrupted) interrupted = pause_nis(0, 20);
    }
    if (!interrupted) {
        play_sound(30500); clear_nis(&port.draw, 0);
        add_nis(&port.draw, discovery_shapes, 3, 40, 32, 0);
        add_nis(&port.draw, discovery_shapes, 6, 50, 33, 10);
        draw_nis_text(0, 0); cut_nis(&port.draw);
        interrupted = pause_nis(0, 6);
    }
    if (!interrupted) {
        clear_nis(&port.draw, 0);
        add_nis(&port.draw, discovery_shapes, 3, 40, 32, 0);
        add_nis(&port.draw, discovery_shapes, 7, 40, 32, 10);
        cut_nis(&port.draw);
        interrupted = pause_nis(0, 6);
    }
    if (!interrupted) {
        memset(white, 63, sizeof white);
        set_palette(16, 240, white, 1);
        play_sound(29101); interrupted = pause_nis(0, 6);
    }
    if (!interrupted) {
        clear_nis(&port.draw, 0);
        add_nis(&port.draw, discovery_shapes, 9, 39, 31, 0);
        copy_nis(&port.draw);
        interrupted = pause_nis(0, 6);
    }
    if (!interrupted) {
        clear_nis(&port.draw, 0);
        add_nis(&port.draw, discovery_shapes, 8, 40, 32, 0);
        cut_nis(&port.draw);
        interrupted = pause_nis(0, 144);
    }
    if (!interrupted) {
        add_nis(&port.draw, discovery_shapes, 10, 50, 31, 10);
        get_shape_rect(&bounds, discovery_shapes, 10, 50, 31);
        interrupted = dissolve_nis(&port.draw, 75, &bounds);
        if (!interrupted) interrupted = pause_nis(0, 156);
    }
    if (!interrupted) interrupted = await_palette(0, -2, 2);
    if (!interrupted) {
        clear_nis(&port.draw, 0);
        add_nis(&port.draw, discovery_shapes, 11, 40, 32, 0);
        add_nis(&port.draw, discovery_shapes, 13, 191, 89, 10);
        dispose_handle(nis_palette);
        nis_palette = get_resource(0x50414c54UL, DISCOVERY_SCENE + 2);
        if (!interrupted) interrupted = await_sound(0);
        if (!interrupted) play_sound(29102);
        if (!interrupted) interrupted = fade_nis(&port.draw, 2);
        if (!interrupted) interrupted = pause_nis(0, 60);
    }
    if (!interrupted) {
        interrupted = run_nis_dialog(DISCOVERY_SCENE, 9, 9, 286);
        if (!interrupted) {
            interrupted = pause_nis(0, 20);
        }
    }
    if (!interrupted) {
        interrupted = run_nis_dialog(DISCOVERY_SCENE, 10, 10, 286);
        if (!interrupted) {
            interrupted = pause_nis(0, 20);
        }
    }
    if (!interrupted) {
        interrupted = run_nis_dialog(DISCOVERY_SCENE, 11, 11, 200);
        if (!interrupted) {
            interrupted = pause_nis(0, 120);
        }
    }
    if (!interrupted) {
        interrupted = play_nis_animation(discovery_shapes, DISCOVERY_SCENE, 1, 1, discovery_callback, 0, 0, 1);
    }
    if (!interrupted) {
        interrupted = play_nis_animation(discovery_shapes, DISCOVERY_SCENE, 2, 1, discovery_callback, 0, 0, 1);
        if (!interrupted) {
            interrupted = await_palette(0, -1, 2);
        }
    }
    dispose_handle(nis_palette);
    finish_palette_effect();
    if (interrupted) {
        dispatch_resource_pointer(0L);
    }
    FreeDialog();
    dispose_handle(discovery_shapes);
    close_temporary_port(&port);
    return interrupted;
}


extern void toggle_playback_coordinates(AnimationState *playback);
extern Handle discovery_shapes;

int discovery_callback(AnimationState *playback, int frame)
{
    toggle_playback_coordinates(playback);
    PaintRLEShapeItem(discovery_shapes, 11, 40 - playback->port.clip.left,
                      32 - playback->port.clip.top, 0);
    toggle_playback_coordinates(playback);
    return 0;
}

typedef struct {
    char header[0x10];
    Rect saved_clip;
    char before_port[0x2e - 0x18];
    ImagePort port;
    Rect active_clip;
    char before_clock[0x5a - 0x4e];
    unsigned int frame_delay;
    unsigned long frame_clock;
} TitleAnimationState;

extern ImagePort *title_patch;
extern int suppress_title_frame;
extern void paint_animation_frame(AnimationState *playback);

int title_callback(void *state, int frame)
{
    struct {
        unsigned long deadline;
        Rect bounds;
        int interrupted;
        DrawPort *restore_port;
        int saved_suppression;
    } local;
    DrawPort *saved_port;

    local.interrupted = 0;
    saved_port = draw_port;

    if (frame == 0) {
        TitleAnimationState *playback = state;

        get_shape_rect(&local.bounds, transition_shapes, 312, 43, 53);
        local.bounds.bottom = playback->port.clip.top;
        draw_port = &nis_draw_port;
        title_patch = save_screen_patch(&local.bounds);
        draw_port = saved_port;
        init_fade_to_primary((AnimationState *)playback, 1);
        local.interrupted = WaitMidiPlayTime(100);
    } else if (frame == 2) {
        switch (midi.play_time) {
        case 101:
            draw_port = &nis_draw_port;
            paint_rle(lock_handle(title_left), 43, 68, 10);
            draw_port = saved_port;
            midi.play_time = 114;
        case 114: {
            TitleAnimationState *playback = state;
            paint_rle(lock_handle(title_left), 43 - playback->port.clip.left,
                      68 - playback->port.clip.top, 10);
            break;
        }
        case 103:
            draw_port = &nis_draw_port;
            paint_rle(lock_handle(title_right), 58, 53, 10);
            draw_port = saved_port;
            midi.play_time = 115;
        case 115: {
            TitleAnimationState *playback = state;
            paint_rle(lock_handle(title_right), 58 - playback->port.clip.left,
                      53 - playback->port.clip.top, 10);
            break;
        }
        case 105:
            init_fade_to_black(state, 1);
            midi.play_time = 98;
            break;
        case 102:
        case 104: {
            TitleAnimationState *playback;

            local.restore_port = saved_port;
            playback = state;
            local.deadline = playback->frame_clock + playback->frame_delay;

            while (local.deadline > *(volatile unsigned long *)&clock_ticks)
                ;
            copy_port(draw_port = &nis_draw_port, title_patch,
                      &title_patch->clip, &title_patch->clip, 0);
            draw_port = local.restore_port;
            playback->active_clip = playback->saved_clip;
            local.saved_suppression = suppress_title_frame;
            suppress_title_frame = 0;
            paint_animation_frame((AnimationState *)playback);
            suppress_title_frame = local.saved_suppression;
            midi.play_time = 120;
            break;
        }
        }
    }
    if (midi.play_time == 98 && !((AnimationState *)state)->fade)
        local.interrupted = 1;
    return local.interrupted;
}

void nis_noop(void)
{
}

int query_nis_sound(void)
{
    return query_sound_payload(0L);
}
