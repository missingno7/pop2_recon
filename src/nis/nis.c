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

void flip_horses(const Handle *shapes)
{
    const Handle *entry = shapes + 1008;
    int count = 36;
    ShapeImage far *image;

    do {
        if (*entry) {
            image = lock_handle(*entry);
            flip_horse_pixels(image->pixels, image->width, image->height);
        }
        ++entry;
    } while (--count);
}

void flip_rider_shapes(const Handle *shapes)
{
    const Handle *entry = shapes + 857;
    int count = 14;
    ShapeImage far *image;

    do {
        if (*entry) {
            image = lock_handle(*entry);
            flip_kid_pixels(image->pixels, image->width, image->height);
        }
        ++entry;
    } while (--count);
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
extern int tree_resource, tree_music, tree_voice_base;

extern int await_palette(Handle palette, int cue, int mode);
extern int run_nis_dialog(int resource, int line, int voice, unsigned int duration);

int tree_callback(void *playback, int frame)
{
    int interrupted;
    DrawPort *old_port;

    midi.play_time = 0;
    play_sound((int)&tree_voice_base + tree_scene);
    interrupted = await_palette(tree_palette, -1, 0);
    if (!interrupted) {
        if (tree_scene == 1) {
            old_port = draw_port;
            draw_port = &nis_draw_port;
            interrupted = run_nis_dialog((int)&tree_resource, 1, 1, 69);
            if (!interrupted)
                start_timed_music((int)&tree_music, 120);
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
extern int tree_palette_base, last_tree_palette;
extern int (far *saved_poll_input)(void);
extern void open_nis_files(const char * const *files);
extern void install_nis_input(void);
extern void open_data_file(const char *name);
extern void close_data_file(const char *name);
extern void close_nis_files(const char * const *files);
extern void set_palette(int first, int count, const void far *colors, int sync);
extern int play_nis_animation(Handle shapes, int resource, int scene, int mode,
                              AnimationCallback callback, int x, int y, int options);
extern void pause_nis(int mode, int duration);
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
    shapes = hGetSL((int)&tree_resource, -1);
    GetDialog();
    set_palette(0, 256, 0L, 1);
    PaintRLEShapeItem(dialog_shape, 1, 0, 0, 0);
    if (scene == 1)
        draw_nis_text(0, 0);
    PaintRLEShapeItem(shapes, 2, 40, 32, 0);

    palette = (tree_scene - 1) / 3;
    tree_palette = get_resource(0x50414c54UL, (int)&tree_palette_base + (palette > 2 ? 2 : palette));
    interrupted = play_nis_animation(shapes, (int)&tree_resource, tree_scene, 1, tree_callback, 0, 0, 1);
    dispose_handle(tree_palette);
    if (!interrupted) {
        if (tree_scene == 1) {
            interrupted = await_timed_music((int)&tree_music);
            if (!interrupted)
                interrupted = run_nis_dialog((int)&tree_resource, 2, 3, 211);
            if (!interrupted)
                interrupted = run_nis_dialog((int)&tree_resource, 3, 4, 110);
            if (!interrupted)
                interrupted = run_nis_dialog((int)&tree_resource, 4, 5, 75);
        } else if (tree_scene == 9) {
            interrupted = WaitMidiPlayTime(98);
            if (!interrupted) {
                PaintRLEShapeItem(shapes, 15, 69, 56, 10);
                interrupted = WaitMidiPlayTime(99);
                if (!interrupted) {
                    tree_palette = get_resource(0x50414c54UL, (int)&last_tree_palette);
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
