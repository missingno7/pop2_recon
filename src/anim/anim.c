/* ANIM.C. Exact contributions: anim_draw, draw_frame, paint_element,
 * accumulate_element_bounds and reset_element. Other routines are drafts. */
typedef struct
{
    int top, left, bottom, right;
} Rect;

typedef struct
{
    unsigned long pixels;
    Rect bounds;
    int row_bytes;
    int flags;
} Port;

typedef struct
{
    int shape, first, last;
    unsigned int flags;
    signed char move_ticks, frame_ticks;
    long x, y;
    long dx, dy;
    long ddx, ddy;
} Element;

typedef struct Animation Animation;
typedef int (far pascal *AnimationCallback)(Animation *, int);

struct Animation
{
    Port port;
    Rect source;
    unsigned char reserved_18[20];
    int destination;
    Port background;
    Rect bounds, dirty;
    const unsigned char far *commands;
    union {
        int repeats;
        unsigned char wait_level;
        unsigned long deadline;
    } timing;
    int skip_draw;
    unsigned int frame_index, delay;
    unsigned long deadline_base;
    AnimationCallback callbacks[4];
    int loops[4];
    Element elements[32];
    int sound_flag;
    volatile int handle_count;
    unsigned long fade;
    int handles[1];
};

typedef struct
{
    int height, width;
    int sound;
    unsigned char depth;
} ShapeHeader;

extern int current_port;
extern unsigned int active_animation_flags;
extern unsigned long clock_ticks;
extern void copy_port(int, int, const Rect far *, const Rect far *, int);
extern void release_fade(unsigned long);
extern int fade_finished(unsigned long);
extern void dispose_handle(int);
extern void release_port(void *);
extern void release_buffer(unsigned long);
extern void cdecl release_animation_heap(void *);

Animation *anim_draw(Animation *animation)
{
    int saved_port = current_port;

    current_port = animation->destination;
    copy_port(current_port, (int)animation, &animation->bounds, &animation->source, 0);
    current_port = saved_port;
    return animation;
}

void anim_free(Animation *animation)
{
    int i;
    int *handle;

    if (animation->fade)
        release_fade(animation->fade);
    i = 0;
    if (i < animation->handle_count) {
        for (handle = animation->handles;; ++handle) {
            if (*handle)
                dispose_handle(*handle);
            if (++i >= animation->handle_count)
                break;
        }
    }
    release_port(animation);
    release_buffer(animation->background.pixels);
    if (animation->sound_flag)
        active_animation_flags &= ~animation->sound_flag;
    release_animation_heap(animation);
}

Animation *draw_frame(Animation *animation);
Animation *near advance_frame(Animation *animation);
void near paint_element(Animation *animation, Element *element);
void near accumulate_element_bounds(Animation *animation, Element *element);
void near reset_element(Element *element);

extern void play_animation_sound(int);
extern void start_animation_fade(int);
extern int animation_fade_pending(int);
extern unsigned char animation_wait_level;

/* Read the command operands at their byte offsets in the animation resource. */
#define WORD_AT(p, n) (*(const int far *)((p) + (n)))
#define LONG_AT(p, n) (*(const long far *)((p) + (n)))

#pragma optimize("e", off)
int anim_idle(Animation *animation)
{
    int saved_port;
    int result = 0;
    const unsigned char far *command;
    int channel, old_flags;
    unsigned long saved_pixels;
    Element *element;

    saved_port = current_port;
    current_port = (int)animation;

next_command:
    command = animation->commands;
    switch (*command) {
    case 0:
        if (animation->deadline_base + animation->delay <= clock_ticks)
            result = 1;
        break;
    case 1:
        if (animation->deadline_base + animation->delay <= clock_ticks) {
            advance_frame(animation);
            if (!animation->timing.repeats) {
                animation->timing.repeats = WORD_AT(command, 1);
                animation->delay = WORD_AT(command, 3);
            }
            if (!--animation->timing.repeats)
                animation->commands += 5;
        }
        break;
    case 2:
        if (!animation->timing.deadline) {
            animation->timing.deadline = clock_ticks + WORD_AT(command, 1);
            if (animation->deadline_base)
                animation->deadline_base += WORD_AT(command, 1);
        } else if (animation->timing.deadline <= clock_ticks) {
            animation->timing.deadline = 0;
            animation->commands += 3;
        }
        break;
    case 3:
        if (animation->deadline_base + animation->delay <= clock_ticks) {
            play_animation_sound(WORD_AT(command, 1));
            animation->commands += 3;
        }
        break;
    case 4:
        start_animation_fade(WORD_AT(command, 1));
        animation->commands += 3;
        break;
    case 5:
        if (!animation_fade_pending(WORD_AT(command, 1)))
            animation->commands += 3;
        break;
    case 6:
        if (!animation->timing.wait_level)
            animation->timing.wait_level = command[1];
        if (!animation_wait_level || animation->timing.wait_level <= animation_wait_level) {
            animation->timing.wait_level = 0;
            animation->commands += 2;
        }
        break;
    case 7:
        draw_frame(animation);
        animation->skip_draw = 1;
        ++animation->commands;
        break;
    default:
        if (((channel = *command) & 0xfc) == 0x10) {
            channel &= 3;
            if (animation->callbacks[channel] &&
                animation->callbacks[channel](animation, WORD_AT(command, 1)))
                result = 2;
            animation->commands += 3;
            goto next_command;
        }
        if (((channel = *command) & 0xfc) == 0x14) {
            channel &= 3;
            if (animation->loops[channel]) {
                if (!--animation->loops[channel]) {
                    animation->commands += 5;
                    goto next_command;
                }
            } else {
                animation->loops[channel] = WORD_AT(command, 1);
            }
            animation->commands = command + WORD_AT(command, 3) + 3;
            goto next_command;
        }
        channel = *command;
        element = &animation->elements[channel & 31];
        switch (channel & 0xe0) {
        case 0x20:
            old_flags = element->flags;
            element->flags &= WORD_AT(command, 1);
            element->flags |= WORD_AT(command, 3);
            if ((element->flags ^ old_flags) & 0x00f0)
                element->move_ticks = 0;
            if ((element->flags ^ old_flags) & 0x0f00)
                element->frame_ticks = 0;
            break;
        case 0x40:
            accumulate_element_bounds(animation, element);
            element->x += (long)WORD_AT(command, 1) << 16;
            element->y += (long)WORD_AT(command, 3) << 16;
            accumulate_element_bounds(animation, element);
            break;
        case 0x60:
            accumulate_element_bounds(animation, element);
            element->x = (long)WORD_AT(command, 1) << 16;
            element->y = (long)WORD_AT(command, 3) << 16;
            accumulate_element_bounds(animation, element);
            break;
        case 0x80:
            accumulate_element_bounds(animation, element);
            element->last = WORD_AT(command, 3);
            element->shape = element->first = WORD_AT(command, 1);
            if (element->shape)
                accumulate_element_bounds(animation, element);
            else
                reset_element(element);
            break;
        case 0xa0:
            element->dx = LONG_AT(command, 1);
            element->dy = LONG_AT(command, 5);
            animation->commands += 9;
            goto next_command;
        case 0xc0:
            element->ddx = LONG_AT(command, 1);
            element->ddy = LONG_AT(command, 5);
            animation->commands += 9;
            goto next_command;
        case 0xe0:
            if ((signed char)command[1] > 0) {
                current_port = animation->destination;
                element->x += (long)animation->bounds.left << 16;
                element->y += (long)animation->bounds.top << 16;
                paint_element(animation, element);
                element->x -= (long)animation->bounds.left << 16;
                element->y -= (long)animation->bounds.top << 16;
                current_port = (int)animation;
            }
            if (!command[1]) {
                paint_element(animation, element);
            } else {
                saved_pixels = animation->port.pixels;
                animation->port.pixels = animation->background.pixels;
                if (animation->port.pixels)
                    paint_element(animation, element);
                animation->port.pixels = saved_pixels;
            }
            animation->commands += 2;
            goto next_command;
        default:
            result = 1;
            goto finish;
        }
        animation->commands += 5;
        goto next_command;
    }

finish:
    current_port = saved_port;
    return result;
}

#pragma optimize("e", on)
extern void near *cdecl malloc(unsigned int);
extern unsigned int cdecl allocation_size(void *);
extern void near *cdecl memset(void near *, int, unsigned int);
#pragma intrinsic(memset)
extern void far *cdecl _fmemcpy(void far *, const void far *, unsigned int);

extern int retain_sheet(int);
extern void restore_sheet(int, int);
extern ShapeHeader far *lock_sheet(int);
extern void prepare_sheet(int);
extern void unlock_sheet(int);
extern void prepare_transparency(int);
extern int load_sheet_shape(int, int, int);
extern int create_port(void *, const Rect far *);
extern void set_port_origin(int, int);
extern unsigned long allocate_pixels(unsigned long);
extern unsigned char animation_command_codes[];
extern unsigned char animation_operand_lengths[];
extern int load_all_animation_shapes;
extern int animation_color_mode;
extern int cdecl sprintf(char *, const char *, ...);
extern void fatal_error(int, const char far *);
extern char bad_animation_shape[];

Animation *anim_new(const unsigned char far *commands, int sheet, int first_shape,
                    int last_shape, int save_background)
{
    int old_sheet = retain_sheet(sheet);
    int count = last_shape - first_shape + 1;
    int i, element_index, saved_port;
    unsigned char referenced[512];
    Animation *animation;
    ShapeHeader far *header;
    const unsigned char far *scan;
    unsigned char opcode;
    char message[80];

    animation = malloc((0x260 + count) * 2);
    if (animation) {
        memset(animation, 0, allocation_size(animation));
        animation->destination = saved_port = current_port;
        for (element_index = 0; element_index < 32; ++element_index)
            reset_element(&animation->elements[element_index]);
        for (i = 0; i < sizeof(referenced); ++i)
            referenced[i] = (unsigned char)*(volatile int *)&load_all_animation_shapes;
        if (!load_all_animation_shapes) {
            scan = commands + 8;
            while ((opcode = *scan) != 0) {
                for (i = 0; i < 8; ++i)
                    if (opcode == animation_command_codes[i])
                        break;
                if (i == 8) {
                    if ((opcode & 0xfc) != animation_command_codes[i]) {
                        ++i;
                        if ((opcode & 0xfc) != animation_command_codes[i]) {
                            if ((opcode & 0xe0) == 0x80 && WORD_AT(scan, 1)) {
                                for (i = WORD_AT(scan, 1); i <= WORD_AT(scan, 3); ++i) {
                                    if (i <= last_shape)
                                        referenced[i - 1] = 1;
                                    else {
                                        sprintf(message, bad_animation_shape, i);
                                        fatal_error(1, message);
                                    }
                                }
                            }
                            for (i = 10; i < 17; ++i)
                                if ((opcode & 0xe0) == animation_command_codes[i])
                                    break;
                        }
                    }
                }
                if (i < 17)
                    scan += animation_operand_lengths[i] + 1;
            }
        }
        animation->handle_count = count;
        header = lock_sheet(sheet);
        if (header) {
            animation_color_mode = header->depth <= 16;
            if (!header->sound) {
                prepare_sheet(sheet);
                animation->sound_flag = header->sound;
            }
            for (count = 0; count < animation->handle_count; ++count) {
                if (referenced[count]) {
                    animation->handles[count] = load_sheet_shape(sheet, count + first_shape,
                                                             2 - (animation_color_mode < 1));
                    if (!animation->handles[count]) {
                        if (animation->sound_flag)
                            header->sound = 0;
                        goto fail;
                    }
                } else {
                    animation->handles[count] = 0;
                }
            }
            if (animation->sound_flag)
                header->sound = 0;
            unlock_sheet(sheet);
            prepare_transparency(sheet);
        }
        if (!create_port(animation, _fmemcpy(&animation->bounds, commands, sizeof(Rect))))
            goto fail;
        current_port = (int)animation;
        set_port_origin(0, 0);
        animation->dirty = animation->source;
        animation->commands = commands + 8;
        if (save_background) {
            animation->background = animation->port;
            animation->background.pixels = allocate_pixels(
                (long)(animation->port.bounds.bottom - animation->port.bounds.top) *
                animation->port.row_bytes);
            if (!animation->background.pixels)
                goto fail;
            copy_port((int)&animation->background, animation->destination,
                      &animation->source, &animation->bounds, 0);
        }
        draw_frame(animation);
        goto restore;
fail:
        anim_free(animation);
        animation = 0;
restore:
        current_port = saved_port;
    }
    restore_sheet(sheet, old_sheet);
    return animation;
}

extern void offset_rect(Rect far *, const Rect far *, int, int);
extern void clear_rect(Rect far *);
extern void fill_rect(Rect far *, int);
extern void finish_paint(void);
extern int animation_callback_argument;

Animation *near advance_frame(Animation *animation)
{
    Rect rect;
    Element *element;
    int count;
    signed char tick;

    ++animation->frame_index;
    animation->deadline_base = clock_ticks;
    if (!animation->skip_draw)
        draw_frame(animation);
    animation->skip_draw = 0;
    offset_rect(&rect, &animation->dirty, animation->bounds.left, animation->bounds.top);
    copy_port(animation->destination, (int)animation, &rect, &animation->dirty, 0);
    memset(&animation->dirty, 0, sizeof(Rect));
    for (element = animation->elements, count = 32; count; ++element, --count) {
        if (!element->shape)
            continue;
        if (element->first != element->last) {
            tick = element->frame_ticks++;
            if (((element->flags >> 8) & 15) <= tick) {
                element->frame_ticks = 0;
                accumulate_element_bounds(animation, element);
                element->shape += (element->flags & 0x1000) ? -1 : 1;
                if (element->flags & 0x2000) {
                    if (element->shape < element->first) {
                        element->shape = element->first + 1;
                        element->flags &= ~0x1000;
                    } else if (element->shape > element->last) {
                        element->shape = element->last - 1;
                        element->flags |= 0x1000;
                    }
                } else if (element->shape < element->first) {
                    element->shape = element->last;
                } else if (element->shape > element->last) {
                    element->shape = element->first;
                }
                accumulate_element_bounds(animation, element);
            }
        }
        tick = element->move_ticks++;
        if (((element->flags >> 4) & 15) <= tick) {
            element->move_ticks = 0;
            if (element->dx || element->dy) {
                accumulate_element_bounds(animation, element);
                element->x += element->dx;
                element->y += element->dy;
                accumulate_element_bounds(animation, element);
            }
            element->dx += element->ddx;
            element->dy += element->ddy;
        }
    }
    return animation;
}

Animation *draw_frame(Animation *animation)
{
    int i;
    Element *element;

    clear_rect(&animation->dirty);
    if (animation->background.pixels) {
        copy_port((int)animation, (int)&animation->background,
                  &animation->source, &animation->source, 0);
    } else {
        fill_rect(&animation->source, *(int *)((char *)animation->destination + 0x20));
    }
    i = 0;
    element = animation->elements;
    do {
        paint_element(animation, element);
        if (i && animation->frame_index > 1 && animation->fade && fade_finished(animation->fade)) {
            release_fade(animation->fade);
            animation->fade = 0;
        }
        ++element;
    } while (++i < 32);
    finish_paint();
    if (animation_callback_argument && animation->callbacks[0])
        animation->callbacks[0](animation, animation_callback_argument);
    return animation;
}

extern ShapeHeader far *lock_shape(int);
extern int high_color_shape_boundary;
extern void paint_low_color_shape(const ShapeHeader far *, int, int, int);
extern void paint_shape(const ShapeHeader far *, int, int, int);

void near paint_element(Animation *animation, Element *element)
{
    if (element->shape) {
        if (!animation_color_mode && high_color_shape_boundary < element->shape)
            paint_low_color_shape(lock_shape(animation->handles[element->shape - 1]),
                                  (int)(element->x >> 16), (int)(element->y >> 16),
                                  element->flags & 15);
        else
            paint_shape(lock_shape(animation->handles[element->shape - 1]),
                        (int)(element->x >> 16), (int)(element->y >> 16),
                        element->flags & 15);
    }
}

extern int clip_rect(Rect far *, const Rect far *, const Rect far *);
extern int rect_is_empty(const Rect far *);
extern void union_rect(Rect far *, const Rect far *, const Rect far *);

#pragma optimize("g", on)
void near accumulate_element_bounds(Animation *animation, Element *element)
{
    Rect rect;

    if (element->shape) {
        rect.left = (int)(element->x >> 16);
        rect.right = rect.left +
                     lock_shape(animation->handles[element->shape - 1])->width;
        rect.top = (int)(element->y >> 16);
        rect.bottom = rect.top +
                      lock_shape(animation->handles[element->shape - 1])->height;
        if (clip_rect(&rect, &rect, &animation->source)) {
            if (rect_is_empty(&animation->dirty))
                animation->dirty = rect;
            else
                union_rect(&animation->dirty, &animation->dirty, &rect);
        }
    }
}

#pragma optimize("g", off)
void near reset_element(Element *element)
{
    memset(element, 0, sizeof(*element));
    element->flags = (element->flags & 0xfff2) | 2;
    element->y = 0x80000000UL;
    element->x = 0x80000000UL;
}

extern int poll_events(void);
extern void reset_input(void);
extern void reset_animation_sound(int, int);
extern void set_animation_audio(int);
extern const unsigned char far *load_animation_commands(unsigned long, int);
extern int open_animation_sheet(int, int, int);
extern int find_animation_palette(unsigned long, int);
extern int load_animation_palette(int);
extern void release_animation_commands(const unsigned char far *);
extern int query_sound_payload(const void far *);
extern char animation_failure[];
extern char screen_port[];

int play_anim(int sheet_override, int scene, int scene_offset, int save_background,
              AnimationCallback callback, int restore, unsigned int flags, int interruptible)
{
    int scene_id = scene + scene_offset;
    int sheet, palette_list, count, i;
    const unsigned char far *commands = load_animation_commands(0x00534352UL, scene_id);
    Animation *animation;
    int palettes[16];
    int result = 0, aborted = 0;
    char message[80];

    if (sheet_override)
        sheet = sheet_override;
    else
        sheet = open_animation_sheet(scene, -1, 1);
    animation = anim_new(commands, sheet, 1, lock_sheet(sheet)->width, save_background);
    animation->callbacks[0] = callback;
    sprintf(message, animation_failure, scene);
    if (!animation)
        fatal_error(1, message);

    load_all_animation_shapes = 0;
    palette_list = find_animation_palette(0x0050534cUL, scene_id);
    count = *(int far *)lock_shape(palette_list);
    for (i = 0; i < count; ++i)
        palettes[i] = load_animation_palette(((int far *)lock_shape(palette_list))[i + 1]);
    dispose_handle(palette_list);
    do {
        result = anim_idle(animation);
        if (interruptible) {
            if (result == 2 || poll_events()) {
                reset_input();
                reset_animation_sound(0, 0);
                aborted = 1;
                break;
            }
            if (result == 2)
                result = 0;
        }
        if (animation->fade && fade_finished(animation->fade)) {
            release_fade(animation->fade);
            animation->fade = 0;
        }
    } while (!result);
    if (restore) {
        if (save_background) {
            Rect rect;
            offset_rect(&rect, &animation->bounds, -animation->bounds.left,
                        -animation->bounds.top);
            copy_port((int)screen_port, (int)&animation->background,
                      &animation->bounds, &rect, 0);
        } else {
            fill_rect((Rect far *)commands, 0);
        }
    }
    anim_free(animation);
    if (!sheet_override)
        dispose_handle(sheet);
    release_animation_commands(commands);
    if (flags & 2)
        set_animation_audio(0);
    if (flags & 1)
        reset_animation_sound(0, 0);
    for (i = 0; i < count; ++i)
        if (palettes[i] && !query_sound_payload(lock_shape(palettes[i])))
            dispose_handle(palettes[i]);
    return aborted;
}

void swap_record_pairs(Animation *animation)
{
    unsigned long pixels = animation->port.pixels;

    animation->port.pixels = animation->background.pixels;
    animation->background.pixels = pixels;
}
