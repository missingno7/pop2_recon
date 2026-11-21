/* LOAD.C, resident resource loading and overlay selection.
 * Declarations outside the verified claims retain the working TU hypotheses.
 * See notes.md for the exact claims and the unresolved draft extents.
 */
typedef unsigned char Byte;
typedef signed char SByte;
typedef unsigned Handle;
typedef struct {
    int top, left, bottom, right;
} Rect;
typedef struct {
    Byte reserved[12];
    SByte sword_kind;
} KidRecord;
typedef struct {
    Byte x, y;
    int flags;
} LevelStart;

extern void near *back_port;
extern Rect back_port_bounds;
extern int level_resource_files[];
extern Byte background;
extern int background_overlay_enabled[];
extern int background_overlay_first[], background_overlay_last[];
extern int background_intro_group[], background_intro_number[];
extern int background_image_first[], background_image_last[];
extern int background_image_count, flask_enabled;
extern unsigned overlay_output;
extern Handle sprite_sets[];
extern Byte level_number;
extern SByte requested_map;
extern int active_map, resume_mode;
extern char level_effect[], map_option[], map_archive[], background_overlay_file[];
extern Byte opponent, start_room_count;
extern SByte loaded_opponent, loaded_sword;
extern int opponent_resource_files[];
extern int *opponent_overlay_first[], *opponent_overlay_last[];
extern int sword_overlay_first[], sword_overlay_last[];
extern int kid_overlay_first[], kid_overlay_last[];
extern int extra_kid_overlay, sword_overlay_enabled[];
extern SByte sword_kind;
extern Handle level_pieces, sword_frames, opponent_frames, opponent_effect_frames;
extern Byte play_state, room_feature, music_number;
extern KidRecord far * volatile saved_kid;
extern int *level_start_table[];
extern void far *level_sequence_table[];
extern int *level_starts;
extern void far *level_sequences;
extern int *sequence_flags;
extern Byte *sequence_opcodes;
extern LevelStart room_starts[];
extern Byte start_x, start_y;
extern Byte room_objects[];
extern unsigned long room_object_info[];
extern Byte room_trigger_data[];
extern int *room_init_ptr;
extern int room_init_data[];
extern Byte opponent_overlay_one, opponent_overlay_two;
extern int sound_available;
extern int saved_background_set;
extern char load_error_format[], resource_path_format[];

extern void near * far pascal AllocBackPort(const Rect far *);
extern void far pascal close_resource_file(int);
extern void far pascal select_resource_file(int);
extern Handle far pascal load_sprite_set(int, unsigned, unsigned, int);
extern void far pascal release_sprite_set(Handle);
extern void far pascal load_overlay_range(Handle, unsigned, int, int, int);
extern int far pascal has_set_overlay(Handle, int);
extern void far pascal release_overlay_range(Handle, int, int);
extern void far pascal preload_sprite_range(Handle, int, int);
extern void far pascal load_scene_overlay(int, int);
extern void far pascal unload_scene_overlay(int, int);
extern void far pascal load_opponent_sound(SByte);
extern void far pascal load_resource_palette(unsigned, int, Byte, Byte);
extern void * far cdecl malloc(unsigned);
extern void far cdecl free(void *);
extern void * far cdecl memset(void *, int, unsigned);
#pragma intrinsic(memset)
extern void far pascal reset_checkpoint(void);
extern int far pascal IsLevelStartWithNISScreen(void);
extern void far pascal start_named_effect(const char far *);
extern void far pascal initialize_level_overlay(void);
extern void * far pascal find_option(const char far *);
extern void far pascal load_map_resource(int, int, int, const char far *, long);
extern void far pascal initialize_level(void);
extern void far pascal reset_level_display(void);
extern void far pascal load_overlay_one(void);
extern void far pascal load_overlay_two(void);
extern void far pascal load_overlay_three(void);
extern void far pascal load_overlay_four(void);
extern void far pascal load_overlay_five(void);
extern void far pascal load_overlay_six(void);
extern void far pascal load_overlay_seven(void);
extern void far pascal load_overlay_eight(void);
extern void far pascal load_overlay_nine(void);
extern void far pascal load_overlay_ten(void);
extern void far pascal load_overlay_eleven(void);
extern Handle far pascal find_near_resource(unsigned long, unsigned);
extern void far pascal release_handle(Handle);
extern Handle far pascal allocate_tagged_handle(unsigned long, unsigned);
extern void far pascal release_tagged_handle(unsigned long, unsigned);
extern unsigned long far * far pascal resolve_handle_to_far_payload(Handle);
extern void far pascal enter_room(Byte);
extern void far pascal set_loading_progress(int, int);
extern void far _fastcall load_opponent_extra(Byte);
extern Handle far pascal load_typed_handle(unsigned);
extern void far pascal lock_handle(Handle);
extern const char far * far pascal get_handle_data(Handle);
extern char far * far cdecl copy_far_string(char far *, const char far *);
extern void far cdecl split_path(char *, char *, char *, char *, char *);
extern int far cdecl sprintf(char *, const char *, ...);
extern void far pascal report_error(int, const char far *);
extern void far pascal reset_sound_data(void);
extern void far pascal release_sound_data(void);

void far pascal unload_set(register int set);
void far pascal check_background_overlays(Byte kind);
void far pascal load_set(register int set, unsigned id, unsigned limit, int count, int mode);
void far pascal unload_level(int first);
void far pascal load_level_map(SByte map);
void far pascal load_kid_set(void);
void far pascal load_kid_set_overlays(void);
void far pascal load_opponent_set(SByte kind);
void far pascal set_kid_sword_kind(void);
void far pascal load_sword_set(SByte kind);
void far pascal load_pieces_data(void);
void far pascal switch_background_overlay(Byte current, Byte previous);
void far pascal load_code_overlays(Byte background_kind, Byte opponent_kind);
void far pascal set_kid_palette(Byte kind);
void far pascal set_level_spec_ptrs(Byte kind);
void far pascal set_initial_room_state(void);
void far pascal unload_pieces_data(void);
Handle far pascal load_sword_frames(Byte kind);
int far pascal need_sword_overlay(SByte index);
int far pascal need_kid_overlay(int index);
void far pascal unload_opponent_overlays(SByte previous, SByte current);
void far pascal load_opponent_overlays(SByte kind);
void far pascal report_resource_path(const char far *path);

void far pascal open_level_resource_files(Byte current, Byte previous)
{
    if (current != previous) {
        if (previous) {
            if (previous == 6)
                unload_set(2);
            close_resource_file(level_resource_files[previous]);
        }
        select_resource_file(level_resource_files[current]);
    }
}

void far pascal CheckBPort(void)
{
    if (back_port == 0)
        back_port = AllocBackPort(&back_port_bounds);
}

void far pascal load_background_set(int palette)
{
    unsigned limit = 0x3ff0;

    if (background == 5)
        limit = 0x7ff0;
    else if (background == 6)
        limit = 0xfff0;

    load_set(4, 3500, limit, 1, 0);
    if (palette)
        load_resource_palette(3500, 64, 160, 0);
    check_background_overlays(background);
    background_image_count = 0;
    if (background_image_first[background])
        preload_sprite_range(sprite_sets[4], background_image_first[background],
                             background_image_last[background]);
}

void far pascal check_background_overlays(Byte kind)
{
    int i;

    for (i = 0; i < 8; ++i) {
        if (background_overlay_enabled[i]) {
            if (background_overlay_first[i] != -1)
                load_overlay_range(sprite_sets[4], 3700, background_overlay_first[i] - 1,
                                   background_overlay_last[i] - 1, 1);
            if (background_intro_group[i] != -1)
                load_scene_overlay(background_intro_group[i], background_intro_number[i]);
        }
    }
}

void far pascal load_flask_set(void)
{
    if (flask_enabled) {
        Byte *entries = malloc(35);
        int i;

        memset(entries, 2, 35);
        for (i = 2; i <= 21; ++i)
            entries[i] = 1;
        load_set(1, 3000, 0x4000, (int)entries, 0);
        free(entries);
        load_resource_palette(3000, 224, 16, 0);
    } else {
        unload_set(1);
    }
}

void far pascal load_level(SByte number)
{
    Byte previous;

    unload_level(3);
    previous = background;
    if (number != -2 && number != -1) {
        if (*(volatile SByte *)&requested_map != number) {
            if (resume_mode == 0)
                reset_checkpoint();
            requested_map = number;
        }
    }
    active_map = requested_map;
    CheckBPort();
    load_level_map(requested_map);
    if (!IsLevelStartWithNISScreen())
        start_named_effect(level_effect);
    load_flask_set();
    open_level_resource_files(background, previous);
    load_background_set(!IsLevelStartWithNISScreen());
    load_kid_set();
    load_kid_set_overlays();
    load_opponent_set(opponent);
    set_kid_sword_kind();
    load_sword_set(sword_kind);
    load_scene_overlay(0, 26);
    load_pieces_data();
    if (background != previous) {
        switch_background_overlay(background, previous);
        set_kid_palette(background);
    }
    load_code_overlays(background, opponent);
    if (number != -2 && number != -1 && background == 1)
        initialize_level_overlay();
    set_level_spec_ptrs(background);
}

void far pascal load_level_map(SByte map)
{
    int id = map + 1999;
    if (find_option(map_option))
        id += 20;
    load_map_resource(0, 0, id, map_archive, 12025L);
    set_initial_room_state();
    initialize_level();
}

void far pascal load_minimum_level(SByte number)
{
    load_level_map(number);
    set_kid_sword_kind();
    load_sword_set(sword_kind);
    reset_level_display();
    room_init_ptr = room_init_data;
    start_named_effect(level_effect);
}

void far pascal switch_background_overlay(Byte current, Byte previous)
{
    if (previous != current && background_intro_group[previous] != -1)
        unload_scene_overlay(background_intro_group[previous], background_intro_number[previous]);
    if (background_intro_group[background] != -1)
        load_scene_overlay(background_intro_group[background], background_intro_number[background]);
}

void far pascal load_code_overlays(Byte background_kind, Byte opponent_kind)
{
    switch (background_kind) {
    case 1:
        load_overlay_one();
        break;
    case 2:
        load_overlay_two();
        load_overlay_three();
        break;
    case 3:
        load_overlay_four();
        load_overlay_five();
        break;
    case 4:
        load_overlay_six();
        load_overlay_seven();
        break;
    case 5:
        load_overlay_eight();
        break;
    case 6:
        load_overlay_nine();
        break;
    }
    if (opponent_kind == 5 || opponent_kind == 6 || opponent_kind == 8)
        load_overlay_ten();
    else
        load_overlay_eleven();
}

void far pascal load_pieces_data(void)
{
    unload_pieces_data();
    level_pieces = find_near_resource(0x50494543UL, 3500);
}

void far pascal load_sword_set(SByte kind)
{
    unsigned id;
    int mode;
    SByte i;

    if (loaded_sword != kind) {
        if (sword_frames) {
            release_handle(sword_frames);
            sword_frames = 0;
        }
        unload_set(0);
    }
    if (sword_frames == 0)
        sword_frames = load_sword_frames(kind);

    id = 1000;
    if (kind == 2)
        id = 1200;
    mode = play_state == 1 && level_number == 14 ? 0 : 1;
    load_set(0, id, 0x8000, 2, mode);
    if (kind == 1) {
        for (i = 0; i < 1; ++i) {
            if (need_sword_overlay(i)) {
                if (!has_set_overlay(sprite_sets[0], sword_overlay_first[i]))
                    load_overlay_range(sprite_sets[0], 1100, sword_overlay_first[i],
                                       sword_overlay_last[i], 2);
            } else {
                release_overlay_range(sprite_sets[0], sword_overlay_first[i], sword_overlay_last[i]);
            }
        }
    }
    loaded_sword = kind;
}

Handle far pascal load_sword_frames(Byte kind)
{
    unsigned id = 1000;
    if (kind == 2)
        id = 1200;
    return allocate_tagged_handle(0x4652414dUL, id);
}

int far _fastcall get_sword_frame_image(unsigned index)
{
    int image;

    if (index)
        image = (int)resolve_handle_to_far_payload(sword_frames)[index];
    else
        image = -1;
    return image;
}

void far pascal set_level_spec_ptrs(Byte kind)
{
    level_starts = level_start_table[kind];
    level_sequences = level_sequence_table[kind];
}

void far _fastcall clear_special_sequence_flag(int i)
{
    int *flags = &sequence_flags[2 * i];
    switch ((unsigned)sequence_opcodes[i]) {
    case 7:
        *flags &= ~0x80;
        break;
    }
}

void far pascal set_initial_room_state(void)
{
    Byte room;
    int index;
    room = 1;
    if (start_room_count >= 1) {
        Byte count;
        do {
            enter_room(room);
            start_x = room_starts[room].x;
            start_y = room_starts[room].y;
            for (index = 0; index < 30; ++index)
                clear_special_sequence_flag(index);
            count = start_room_count;
        } while (++room <= count);
    }
}

void far pascal load_set(register int set, unsigned id, unsigned limit, int count, int mode)
{
    if (sprite_sets[set] == 0)
        sprite_sets[set] = load_sprite_set(id, limit, count, mode);
}

void far pascal load_kid_set(void)
{
    Byte *entries;
    int i, j;

    if (background == 6)
        select_resource_file(level_resource_files[6]);
    entries = malloc(314);
    memset(entries, 0, 314);
    for (i = 131; i <= 132; ++i)
        entries[i] = 1;
    for (j = 216; j <= 218; ++j)
        entries[j] = 1;
    load_set(2, 25001U, 2, (int)entries, 0);
    free(entries);
    preload_sprite_range(sprite_sets[2], 1976, 1994);
}

void far pascal load_kid_set_overlays(void)
{
    int i;
    for (i = 0; i < 11; ++i) {
        if (need_kid_overlay(i)) {
            if (!has_set_overlay(sprite_sets[2], kid_overlay_first[i]))
                load_overlay_range(sprite_sets[2], 24601, kid_overlay_first[i],
                                   kid_overlay_last[i], 0);
        } else {
            release_overlay_range(sprite_sets[2], kid_overlay_first[i], kid_overlay_last[i]);
        }
    }
}

int far pascal need_kid_overlay(int index)
{
    int needed;
    switch (index) {
    default:
        needed = 0;
        break;
    case 0:
        needed = background_overlay_enabled[0];
        break;
    case 1:
        needed = background == 4 || background == 2;
        break;
    case 2:
        needed = background == 3 || opponent == 5 || opponent == 6;
        break;
    case 3:
        needed = background_overlay_enabled[3];
        break;
    case 5:
        needed = opponent == 5 || opponent == 6;
        break;
    case 6:
        needed = level_number == 5;
        break;
    case 8:
        needed = level_number == 13;
        break;
    case 9:
        needed = level_number == 13 || background == 6;
        break;
    case 4:
    case 7:
    case 10:
        needed = 0;
        break;
    }
    return needed;
}

void far pascal set_kid_palette(Byte kind)
{
    load_resource_palette(25001, 16, 16, (Byte)(kind - 1));
}

int far pascal need_opponent_overlay(SByte kind, int index)
{
    int needed = 0;
    if (kind != -1 && opponent_overlay_first[kind]) {
        int count = -1;
        if (opponent_overlay_first[kind][-1] != count) {
            int *first = ((int * volatile *)opponent_overlay_first)[*(volatile SByte *)&kind] - 1;
            do {
                ++count;
                ++first;
                } while (*first != -1);
        }
        if (count >= index) {
            if (index < 3)
                needed = sword_overlay_enabled[index];
            else if (kind == 7 || background == 6)
                needed = 1;
        }
    }
    return needed;
}

int far pascal need_sword_overlay(SByte index)
{
    int needed; /* The original sets this local only on the success path. */
    switch ((int)index) {
    case 0:
        if (sword_overlay_enabled[0])
            needed = 1;
        break;
    }
    return needed;
}

void far pascal load_opponent_set(SByte kind)
{
    set_loading_progress(50, 100);
    if (kind == -1 || kind != loaded_opponent || sprite_sets[3] == 0) {
        unload_set(3);
        if (opponent_frames) {
            release_handle(opponent_frames);
            opponent_frames = 0;
        }
        if (opponent_effect_frames) {
            release_handle(opponent_effect_frames);
            opponent_effect_frames = 0;
        }
        if (loaded_opponent != -1 && kind != loaded_opponent)
            close_resource_file(opponent_resource_files[loaded_opponent]);
    }
    if (loaded_opponent != -1 && opponent_overlay_first[loaded_opponent])
        unload_opponent_overlays(loaded_opponent, kind);

    if (kind != -1) {
        if (kind == loaded_opponent && sprite_sets[3]) {
            if (kind == loaded_opponent)
                load_opponent_overlays(kind);
        } else {
            select_resource_file(opponent_resource_files[kind]);
            load_set(3, 750, 4, 2, 0);
            load_resource_palette(750, 32, 16, 0);
            if (opponent_overlay_first[kind]) {
                load_opponent_overlays(kind);
                preload_sprite_range(sprite_sets[3], (int)opponent_overlay_first[kind],
                                     (int)opponent_overlay_last[kind]);
            }
            load_opponent_sound(kind);
            opponent_frames = allocate_tagged_handle(0x4652414dUL, 750);
            if ((kind == 5 || kind == 6) && opponent_effect_frames == 0)
                opponent_effect_frames = allocate_tagged_handle(0L, 755);
        }
    }
    loaded_opponent = kind;
}

void far pascal load_opponent_overlays(SByte kind)
{
    int *last_base = opponent_overlay_last[kind];
    int *first_base = opponent_overlay_first[kind];
    if (first_base && kind != 5 && kind != 6) {
        int i = 0;
        if (*first_base == -1)
            return;
        {
            int * volatile last = last_base;
            int *first = first_base;
            do {
                if (need_opponent_overlay(kind, i)) {
                    if (!has_set_overlay(sprite_sets[3], *first))
                        load_overlay_range(sprite_sets[3], 850, *first, *last, 2);
                    if (i == 0 && (kind == 0 || kind == 5 || kind == 6)) {
                        if (!opponent_overlay_one)
                            load_opponent_extra(1);
                        if (!opponent_overlay_two)
                            load_opponent_extra(2);
                    }
                } else {
                    release_overlay_range(sprite_sets[3], *first, *last);
                }
                ++last;
                ++i;
                ++first;
            } while (*first != -1);
        }
    }
}

typedef struct {
    unsigned flags, actor;
} RoomObjectInfo;

void far pascal preload_room_data(Byte room)
{
    int visited[64];
    SByte tile;
    volatile unsigned first;

    memset(visited, 0, sizeof(visited));
    tile = 0;
    first = (unsigned)(Byte)30 * room;
    do {
        int index = tile + first;
        unsigned object = room_objects[index];
        unsigned info = ((RoomObjectInfo *)room_object_info)[index].flags;
        int *flag = &visited[object];
        if (*flag == 0) {
            visited[object] = 1;
            switch (object) {
            case 2:
                load_scene_overlay(background_intro_group[0], background_intro_number[0]);
                break;
            case 10:
                if (((Byte)info & 0xe0) == 0xc0 && loaded_opponent != 9)
                    load_opponent_set(9);
                /* fall through */
            case 23:
                load_scene_overlay(background_intro_group[1], background_intro_number[1]);
                break;
            case 13:
                load_scene_overlay(background_intro_group[3], background_intro_number[3]);
                break;
            case 25:
                load_scene_overlay(background_intro_group[5], background_intro_number[5]);
                break;
            case 26:
                load_scene_overlay(background_intro_group[6], background_intro_number[6]);
                break;
            case 36:
                load_scene_overlay(background_intro_group[4], background_intro_number[4]);
                break;
            }
        }
    } while (++tile < 30);
    if (room_trigger_data[(unsigned)(Byte)116 * room])
        load_opponent_sound(opponent);
    if ((SByte)music_number > 0) {
        unsigned id;
        if ((SByte)music_number < 33)
            id = 10016;
        else
            id = 10000;
        load_typed_handle(id);
        if ((SByte)music_number < 31)
            load_typed_handle(10001);
    }
}

int far pascal load_resource_error(Handle resource)
{
    char path[120];
    char drive[4], directory[256], basename[256], extension[256];

    lock_handle(resource);
    copy_far_string(path, get_handle_data(resource) + 16);
    split_path(path, drive, directory, basename, extension);
    sprintf(path, resource_path_format, basename, extension);
    report_resource_path(path);
    return 1;
}

void far pascal report_resource_path(const char far *path)
{
    char message[100];

    sprintf(message, "Couldn't find resource file \"%s\"", path);
    report_error(1, message);
}

void far pascal set_kid_sword_kind(void)
{
    if (saved_kid)
        sword_kind = saved_kid->sword_kind;
    else if (level_number == 6)
        sword_kind = -1;
    else if (level_number == 7 ||
             (level_number == 8 && room_feature != 9 && saved_kid == 0))
        sword_kind = 2;
    else
        sword_kind = 1;
}

void far pascal unload_from(int first)
{
    int i;
    reset_sound_data();
    for (i = first; i < 5; ++i)
        unload_set(i);
    if (first <= 4 && sound_available)
        release_sound_data();
}

void far pascal unload_level(int first)
{
    int i;
    unload_from(first);
    for (i = 0; i < 8; ++i) {
        if (background_overlay_enabled[i])
            unload_scene_overlay(background_intro_group[i], background_intro_number[i]);
    }
    if (first <= 3 && opponent_frames) {
        if (opponent_frames) {
            release_handle(opponent_frames);
            opponent_frames = 0;
        }
        if (opponent_effect_frames) {
            release_tagged_handle(0L, 755);
            opponent_effect_frames = 0;
        }
        if (loaded_opponent != -1) {
            close_resource_file(opponent_resource_files[loaded_opponent]);
            loaded_opponent = -1;
        }
    }
    if (first <= 0 && sword_frames) {
        release_handle(sword_frames);
        sword_frames = 0;
        loaded_sword = 0;
    }
    unload_pieces_data();
    release_sound_data();
}

void far pascal unload_opponent_overlays(SByte previous, SByte current)
{
    if (sprite_sets[3] && opponent != 5 && opponent != 6) {
        int *last_base = opponent_overlay_last[previous];
        int *first_base = opponent_overlay_first[previous];
        if (first_base) {
            int i = 0;
            if (*first_base == -1)
                return;
            {
                int * volatile last = last_base;
                int *first = first_base;
                do {
                    if (current != previous || !need_opponent_overlay(current, i))
                        release_overlay_range(sprite_sets[3], *first, *last);
                    ++last;
                    ++i;
                    ++first;
                } while (*first != -1);
            }
        }
    }
}

void far pascal unload_pieces_data(void)
{
    if (level_pieces) {
        free((void *)level_pieces);
        level_pieces = 0;
    }
}

void far pascal unload_set(register int set)
{
    if (sprite_sets[set]) {
        release_sprite_set(sprite_sets[set]);
        sprite_sets[set] = 0;
    }
}
