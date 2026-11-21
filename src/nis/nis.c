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
    if (music_options & 1)
        play_sound(sound);
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
            if (poll_input())
                interrupted = 1;
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
        if (time <= midi.play_time)
            break;
    }
    return 0;
}

extern char dialog_shape_resource[];
extern Handle dialog_shape, dialog_font, title_font;
extern Handle load_font(int resource_id);
extern void dispose_handle(Handle handle);

void GetDialog(void)
{
    dialog_shape = hGetSL((int)dialog_shape_resource, 1);
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

    if (line == 0)
        text[0] = 0;
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

extern int await_palette(Handle palette, int cue, int mode);
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
            if (!interrupted)
                start_timed_music(TREE_MUSIC, 120);
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
extern int pause_nis(int mode, int duration);
extern void finish_nis_animation(void);
extern void stop_nis_sound(const void far *sound);

void tree_check(int scene)
{
    register Handle shapes;
    int interrupted;
    int palette;

    open_nis_files(nis_data_files);
    install_nis_input();
    tree_scene = scene;
    open_data_file(nis_file);
    shapes = hGetSL(TREE_RESOURCE, -1);
    GetDialog();
    set_palette(0, 256, 0L, 1);
    PaintRLEShapeItem(dialog_shape, 1, 0, 0, 0);
    if (scene == 1)
        draw_nis_text(0, 0);
    PaintRLEShapeItem(shapes, 2, 40, 32, 0);

    palette = (tree_scene - 1) / 3;
    tree_palette = get_resource(0x50414c54UL, TREE_PALETTE_BASE + (palette > 2 ? 2 : palette));
    interrupted = play_nis_animation(shapes, TREE_RESOURCE, tree_scene, 1, tree_callback, 0, 0, 1);
    dispose_handle(tree_palette);
    if (!interrupted) {
        if (tree_scene == 1) {
            interrupted = await_timed_music(TREE_MUSIC);
            if (!interrupted)
                interrupted = run_nis_dialog(TREE_RESOURCE, 2, 3, 211);
            if (!interrupted)
                interrupted = run_nis_dialog(TREE_RESOURCE, 3, 4, 110);
            if (!interrupted)
                interrupted = run_nis_dialog(TREE_RESOURCE, 4, 5, 75);
        } else if (tree_scene == 9) {
            interrupted = WaitMidiPlayTime(98);
            if (!interrupted) {
                PaintRLEShapeItem(shapes, 15, 69, 56, 10);
                interrupted = WaitMidiPlayTime(99);
                if (!interrupted) {
                    tree_palette = get_resource(0x50414c54UL, LAST_TREE_PALETTE);
                    interrupted = await_palette(tree_palette, -1, 2);
                    dispose_handle(tree_palette);
                }
            }
        }
    }
    if (!interrupted) {
        interrupted = await_sound(0);
        if (!interrupted)
            pause_nis(0, 60);
    }
    finish_nis_animation();
    if (interrupted)
        stop_nis_sound(0L);

    FreeDialog();
    dispose_handle(shapes);
    dispose_handle(dialog_shape);
    close_data_file(nis_file);
    poll_input = saved_poll_input;
    close_nis_files(nis_data_files);
}

extern const char transition_file[], caverns_file[], ruins_file[];
extern Rect screen_bounds, viewport_bounds;
extern int transition_animation_state;
extern int title_lamp;
extern int title_visibility_delay;
enum { TRANSITION_RESOURCE = 25000, PRINCESS_MUSIC = 31030, PRINCESS_CUE = 31031 };
extern Handle transition_shapes, background_shapes, transition_palette;
extern Handle title_left, title_right, second_palette;
extern void fill_rect(const Rect far *bounds, int color);
extern int is_sound_playing(int sound);
extern Handle load_shape_item(Handle shapes, int item, int flags);
extern void stop_lamp(int lamp);
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
        PaintRLEShapeItem(transition_shapes, 320, 118, 0, 10);
        PaintRLEShapeItem(transition_shapes, 321, 73, 60, 10);
        PaintRLEShapeItem(transition_shapes, 322, -41, 32, 10);
        title_left = load_shape_item(transition_shapes, 312, 1);
        title_right = load_shape_item(transition_shapes, 313, 1);
        transition_palette = TRANSITION_COLORS(2);
        title_lamp = 0;
        title_visibility_delay = 2;
        interrupted = PLAY_TRANSITION(12, 1, title_callback);
        title_visibility_delay = 0;
        if (title_lamp)
            stop_lamp(title_lamp);
        dispose_handle(transition_palette);
        dispose_handle(title_right);
        dispose_handle(title_left);
    } else if (scene == 1) {
        open_data_file(caverns_file);
        open_data_file(ruins_file);
        fill_rect(&viewport_bounds, 48);
        PaintRLEShapeItem(transition_shapes, 358, -127, -34, 0);
        PaintRLEShapeItem(transition_shapes, 354, 0, 105, 0);
        PaintRLEShapeItem(transition_shapes, 369, 185, 46, 10);
        transition_palette = TRANSITION_COLORS(1);
        interrupted = PLAY_TRANSITION(scene, scene, transition_scene_one);
        dispose_handle(transition_palette);
        if (!interrupted) {
            set_palette(0, 256, 0L, (signed char)scene);
            fill_rect(&screen_bounds, 0);
            fill_rect(&viewport_bounds, 16);
            PaintRLEShapeItem(transition_shapes, 320, 117, 0, 10);
            PaintRLEShapeItem(transition_shapes, 321, 73, 60, 10);
            PaintRLEShapeItem(transition_shapes, 322, -41, 34, 10);
            transition_palette = TRANSITION_COLORS(2);
            interrupted = PLAY_TRANSITION(2, scene, transition_scene_one);
            dispose_handle(transition_palette);
        }
        if (!interrupted) {
            set_palette(0, 256, 0L, (signed char)scene);
            fill_rect(&screen_bounds, 0);
            dispose_handle(transition_shapes);
            transition_shapes = hGetSL(TRANSITION_RESOURCE, -2);
            background_shapes = hGetSL(3500, -16);
            PaintRLEShapeItem(background_shapes, 1100, 0, 0, 0);
            PaintRLEShapeItem(background_shapes, 1101, 10, 100, 0);
            PaintRLEShapeItem(background_shapes, 1102, 0, 100, 0);
            dispose_handle(background_shapes);
            transition_palette = TRANSITION_COLORS(11);
            interrupted = await_palette(transition_palette, -1, 0);
            dispose_handle(transition_palette);
            if (!interrupted)
                interrupted = PLAY_TRANSITION(3, scene, 0);
        }
        close_data_file(ruins_file);
        close_data_file(caverns_file);
    } else if (scene == 2) {
        open_data_file(ruins_file);
        background_shapes = hGetSL(3500, -8);
        paint_ruins_sky(background_shapes);
        PaintRLEShapeItem(background_shapes, 763, 0, 39, 10);
        PaintRLEShapeItem(background_shapes, 764, 0, 99, 0);
        PaintRLEShapeItem(transition_shapes, 317, 75, 166, 10);
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
            PaintRLEShapeItem(background_shapes, 766, 0, 11, 10);
            PaintRLEShapeItem(background_shapes, 767, 0, 101, 0);
            set_palette(0, 256, lock_handle(transition_palette), 1);
            dispose_handle(transition_palette);
            interrupted = PLAY_TRANSITION(6, 1, transition_scene_six);
        }
        if (!interrupted) {
            interrupted = await_palette(0, -1, 0);
            fill_rect(&screen_bounds, 0);
        }
        if (!interrupted) {
            PaintRLEShapeItem(transition_shapes, 347, 0, 0, 10);
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
        PaintRLEShapeItem(transition_shapes, 362, 0, 126, 0);
        PaintRLEShapeItem(transition_shapes, 363, 0, 160, 10);
        PaintRLEShapeItem(transition_shapes, 364, 0, 168, 10);
        PaintRLEShapeItem(transition_shapes, 365, 183, 97, 10);
        PaintRLEShapeItem(transition_shapes, 368, 11, 77, 10);
        PaintRLEShapeItem(transition_shapes, 369, -12, 40, 10);
        PaintRLEShapeItem(transition_shapes, 373, 28, 13, 10);
        PaintRLEShapeItem(transition_shapes, 373, 154, 31, 10);
        PaintRLEShapeItem(transition_shapes, 374, 126, 50, 10);
        PaintRLEShapeItem(transition_shapes, 352, 0, 0, 10);
        interrupted = PLAY_TRANSITION(9, 1, transition_scene_nine);
        if (!interrupted) {
            await_palette(0, -1, 0);
            fill_rect(&screen_bounds, 0);
            fill_rect(&viewport_bounds, 48);
            PaintRLEShapeItem(transition_shapes, 373, 153, 33, 0);
            PaintRLEShapeItem(transition_shapes, 374, 129, 51, 0);
            PaintRLEShapeItem(transition_shapes, 375, -8, 39, 0);
            PaintRLEShapeItem(transition_shapes, 373, 2, 10, 0);
            PaintRLEShapeItem(transition_shapes, 362, 0, 135, 0);
            PaintRLEShapeItem(transition_shapes, 363, 0, 168, 0);
            PaintRLEShapeItem(transition_shapes, 364, 0, 176, 10);
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
    transition_animation_state = 0;
    close_nis_files(nis_data_files);
    return interrupted;
}

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
    if (!interrupted)
        interrupted = run_nis_dialog(THRONE_DIALOG_RESOURCE, 1, 1, 311);
    if (!interrupted)
        interrupted = run_nis_dialog(THRONE_DIALOG_RESOURCE, 2, 2, 278);
    if (!interrupted)
        interrupted = run_nis_dialog(THRONE_DIALOG_RESOURCE, 3, 3, 121);
    if (!interrupted)
        interrupted = run_nis_dialog(THRONE_DIALOG_RESOURCE, 4, 4, 326);
    if (!interrupted) {
        draw_nis_text(0, 0);
        interrupted = pause_nis(0, 60);
    }
    if (!interrupted)
        interrupted = run_nis_dialog(THRONE_DIALOG_RESOURCE, 5, 5, 222);
    if (!interrupted)
        interrupted = pause_nis(0, 60);
    if (!interrupted)
        interrupted = run_nis_dialog(THRONE_DIALOG_RESOURCE, 6, 6, 224);
    if (!interrupted) {
        interrupted = pause_nis(0, 60);
        draw_nis_text(0, 0);
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
        saved = draw_port;
        draw_port = &nis_draw_port;
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

/* Later definitions in NIS.C. The intervening scene bodies are still pending. */
extern Handle prepare_sound(int cue);

int run_nis_dialog(int resource, int line, int voice, unsigned int duration)
{
    int cue = resource + voice;

    prepare_sound(cue);
    draw_nis_text(resource, line);
    return play_timed_music(cue, duration);
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
    nis_button_latched = nis_button_held >= 1;
}
