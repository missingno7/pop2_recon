/* TOP.C candidate, DOS frame 0e56. Definitions follow original order.
 * Address comments identify the observed operands behind external declarations. */
#pragma pack(1)
struct box { int left, top, right, bottom; };
struct character {
    char frame, direction;
    int x, y;
    char action;
    unsigned sequence;
    signed char column, row, floor, strength, fall, room, type;
    signed char skill, alive, max_hp, hp_delta, pushed;
    int hurt, previous_x, alternate_x;
    struct box bounds;
    char alert;
    int collision;
    char unused26[6];
    struct box previous_bounds;
    int previous_sequence_x;
    char unused36[3];
    signed char opponent;
    char unused3a[6];
};
struct room_state { char prefix[0x34]; signed char guard_count; char suffix[0x3f]; };
struct room_links { unsigned char left, right, above, below; };
struct rebirth_point { signed char room, cell; };
struct guard_record { char prefix[7]; int active; };
typedef void (far pascal *frame_proc)(void);

extern struct character Char;                     /* ds:5c00 */
extern struct character FChar;                    /* ds:5c40 */
extern struct character Kid;                      /* ds:5c80 */
extern struct character shadows[];                /* ds:5cc0 */
extern struct room_state room_states[];            /* ds:44c1 */

extern const struct rebirth_point rebirth_points[2]; /* ds:53e9 */
extern frame_proc level_frame_procs[];             /* ds:06d4 */
extern int far *rebirth_data;                      /* ds:5bfc */
extern unsigned char room_count;                   /* ds:4542 */
extern unsigned char stage_set;                    /* ds:4547 */
extern signed char stage_number;                   /* ds:4549 */
extern signed char entry_room;                     /* ds:4562 */
extern signed char entry_cell;                     /* ds:4563 */
extern signed char entry_facing;                   /* ds:4564 */
extern signed char guard_start_delay;             /* ds:5c63 */
extern signed char end_room;                       /* ds:4568 */
extern signed char level;                          /* ds:0a18 */
extern signed char game_request;                   /* ds:6ccc */
extern signed char room_to_draw;                   /* ds:6ccd */
extern signed char starting_health;                /* ds:6cd1 */
extern int resume_mode;                            /* ds:5e00 */
extern signed char entrance_mode;                  /* ds:5e03 */
extern signed char start_facing;                   /* ds:5e04 */
extern signed char death_countdown;                /* ds:5e05 */
extern int mirror_state;                           /* ds:5e08 */
extern int room_sound_offset;                      /* ds:5e0c */
extern int room_changed;                           /* ds:5e0e */
extern int reload_room;                            /* ds:5e18 */
extern int redraw_frame;                           /* ds:5e1a */
extern int draw_interlude;                         /* ds:5e1c */
extern int queued_sound;                           /* ds:5e22 */
extern unsigned remaining_minutes;                 /* ds:5e2a */
extern int restart_request;                        /* ds:5e2c */
extern int skip_frame;                             /* ds:5e2e */
extern unsigned char current_room;                 /* ds:5e34 */
extern int freeze_ticks;                           /* ds:5e3e */
extern int sequence_delay;                    /* ds:5e40 */
extern int pending_sequence;                       /* ds:5e42 */
extern int remaining_ticks;                        /* ds:5e44 */
extern int next_level;                             /* ds:5e46 */
extern int cut_required;                           /* ds:5e5e */
extern int frame_count;                             /* ds:5e60 */
extern int message_frames;                         /* ds:5e8e */
extern int block_ticks;                       /* ds:5e90 */
extern volatile int tick_delay;                             /* ds:264e */
extern int script_status;                          /* ds:2cf6 */
extern int recording_mode;                         /* ds:2cfa */
extern int stage_state;                            /* ds:2d00 */
extern int draw_pass;                              /* ds:6a4a */
extern int input_enabled;                          /* ds:6266 */
extern int queued_track;                           /* ds:25c0 */
extern int sound_enabled;                          /* ds:0228 */
extern int special_sequence;                       /* ds:0a16 */
extern int resource_index;                         /* ds:0902 */
extern int last_sound;                             /* ds:08fe */
extern int last_track;                             /* ds:0900 */
extern int option_flags;                           /* ds:1142 */
extern int preview_mode;                           /* ds:1504 */
extern int view_retry;                             /* ds:6296 */
extern int shadows_processed;                      /* ds:6298 */
extern int action_timer;                           /* ds:629a */
extern int guard_timer;                            /* ds:629c */
extern int control_lock;                           /* ds:629e */
extern int collision_pending;                      /* ds:62de */
extern int transition_pending;                     /* ds:67cc */
extern int fade_ticks;                              /* ds:67bc */
extern char key_result;                            /* ds:01d6 */
extern unsigned char cell_flags;                   /* ds:6271 */
extern signed char sweep_column;                   /* ds:6275 */
extern signed char sweep_left;                     /* ds:6277 */
extern signed char sweep_right;                    /* ds:6278 */
extern signed char sweep_row;                      /* ds:6279 */
extern signed char previous_sweep_row;             /* ds:627a */
extern int column_x[];                              /* ds:0d86 */
extern char room_render_flags[10];                  /* ds:6248 */
extern unsigned char *room_kinds;                   /* ds:6294 */
extern unsigned int entry_cells_a[];                /* ds:3faa */
extern unsigned int entry_cells_b[];                /* ds:328e */
extern char interlude_name[];                       /* ds:09fe */
extern char loading_name[];                         /* ds:1fe0 */

extern void check_graphics_port(void);              /* far:0a5c:004e */
extern int find_option(char far *);                  /* far:1101:29aa */
extern int check_level_change(int, int);             /* far:0284:000e */
extern char select_preview_level(void);             /* far:0000:0210 */
extern void load_level(char);                       /* far:0a5c:01b4 */
extern void load_level_minimum(char);               /* far:0a5c:02f4 */
extern void initialize_guard_controls(void);        /* far:2802:0848 */
extern void InitInput(void);                        /* far:02cc:1404 */
extern void start_recording(void);                  /* far:0da4:0252 */
extern void finish_recording(void);                 /* far:0da4:0122 */
extern void finish_level(void);                     /* far:0169:0794 */
extern void reset_vector_a(void);                   /* far:2203:0d21 */
extern void reset_vector_b(void);                   /* far:2203:0d0d */
extern void reset_vector_c(void);                   /* far:2203:0d17 */
extern void reset_vector_d(void);                   /* far:2203:0d2b */
extern int starting_sequence(void);                 /* far:052d:1234 */
extern void start_sequence(int);                    /* far:25da:0016 */
extern void advance_character(void);               /* far:02cc:0394 */
extern void reset_velocity(void);                   /* far:02cc:07a2 */
extern void face_joystick(void);                    /* far:02cc:0944 */
extern void select_room(char);                       /* far:0f77:0002 */
extern void open_entrance(char, char);               /* far:0b41:16fa */
extern void play_sound(int);                        /* far:1101:807a */
extern int check_frame_input(int);                  /* far:0000:0dd4 */
extern int allow_fade(void);                         /* far:0284:0096 */
extern void start_named_effect(char far *);          /* far:1101:4d90 */
extern void update_display(int);                    /* far:078a:213a */
extern void update_health(int, int);                /* far:078a:24ee */
extern void wait_refresh(void);                     /* far:0000:125a */
extern void show_interlude(char far *, int);         /* far:1101:6650 */
extern void start_room_pass(void);                  /* far:078a:0000 */
extern void init_room_view(void);                   /* far:04a6:003e */
extern void finish_room_view(void);                 /* far:078a:13b2 */
extern void set_room_view_mode(int);                 /* far:04a6:0784 */
extern void draw_room(void);                        /* far:078a:12e4 */
extern void play_room_sound(char far *);            /* far:1ead:01a2 */
extern void finish_overlay_pass(void);              /* far:2344:0f4e */
extern void show_message(char far *);               /* far:0000:1268 */
extern void update_interlude(void);                 /* far:078a:293a */
extern int sound_busy(void);                         /* far:0dd7:02ce */
extern int start_level_track(int);                  /* far:1101:8444 */
extern void stop_sound(int);                        /* far:1101:83f0 */
extern void finish_level_scene(void);                /* far:052d:109e */
extern void set_frame_wait(int);                    /* far:1de8:015a */
extern void limit_frame_rate(void);                 /* far:107d:000c */
extern void start_floor_frame(void);                /* far:0b41:1a42 */
extern void update_floors(void);                    /* far:0b41:000c */
extern void level_special_floor(void);              /* far:2c3e:0f6c */
extern void select_guards_in_room(unsigned char);   /* far:2344:0a52 */
extern void advance_guard_frame(void);              /* far:2344:1f0e */
extern void check_guard_changes(void);              /* far:2344:198e */
extern void update_sound(void);                     /* far:0dd7:016c */
extern void advance_screen_logic(void);              /* far:0000:0f70 */
extern void update_guard_frame(void);               /* far:2344:1088 */
extern void wait_overlay_frame(void);               /* far:2344:0fae */
extern void check_clock(void);                      /* far:0000:0cbc */
extern void load_kid(void);                         /* far:02cc:1d94 */
extern int test_character_control(void);            /* far:02cc:0eb6 */
extern signed char find_opponent(signed char);       /* far:2344:08f0 */
extern void set_opponent(signed char);               /* far:02cc:07fc */
extern void update_character_cells(void);           /* far:02cc:027e */
extern int control_kid(void);                       /* far:02cc:116e */
extern void update_position(void);                  /* far:02cc:08d6 */
extern void update_fall(void);                       /* far:02cc:090e */
extern void check_controls(void);                   /* far:0169:09ee */
extern void check_collisions(void);                 /* far:2802:09ca */
extern void collide_floor(void);                    /* far:2802:000e */
extern void collide_walls(void);                    /* far:2802:0402 */
extern void collide_objects(void);                  /* far:2802:0d56 */
extern void process_floor(void);                    /* far:0b41:06f8 */
extern void process_gate(void);                     /* far:0b41:075e */
extern void prepare_room_set(void);                 /* far:101f:01ae */
extern void update_room_set_a(void);                /* far:2a4e:0e44 */
extern void finish_room_set(void);                  /* far:101f:00a0 */
extern void update_room_set_b(void);                /* far:29cf:00b2 */
extern void touch_room_set_b(void);                 /* far:29cf:0118 */
extern void update_room_set_c(void);                /* far:29cf:01f4 */
extern void touch_room_set_c(void);                 /* far:29cf:06e8 */
extern void schedule_sound(int);                    /* far:0dd7:01ce */
extern void release_character_control(void);        /* far:02cc:0dda */
extern void load_shadow(char);                      /* far:02cc:1d42 */
extern void set_shadow_opponent(char);              /* far:02cc:086a */
extern void control_shadow(void);                   /* far:02cc:12e4 */
extern void change_view_room(void);                 /* far:078a:0024 */
extern void shadow_bump(void);                      /* far:2c3e:0052 */
extern void collide_shadow_floor(void);              /* far:2802:0e82 */
extern void collide_shadow_walls(void);              /* far:2802:0c94 */
extern void update_room_set_d(void);                 /* far:29cf:1b60 */
extern char sequence_row(int);                      /* far:02cc:1102 */
extern void check_shadow_exit(void);                 /* far:2c3e:1674 */
extern void save_shadow(void);                       /* far:02cc:1cf6 */
extern void remove_shadow(unsigned char, int);       /* far:2344:07bc */
extern void select_room_view(char);                  /* far:04a6:02b0 */
extern void update_room_screen(void);               /* far:078a:2590 */
extern void render_screen(void);                    /* far:078a:218e */
extern void check_message_end(void);                 /* far:0000:12de */
extern void step_sound(void);                       /* far:0dd7:04d0 */
extern void finish_queued_sound(int *);              /* far:0dd7:03cc */
extern void update_flash(void);                     /* far:078a:20a8 */
extern void set_sound_flags(int);                    /* far:1101:842c */
extern void draw_floor_foreground(void far *, struct box *, char); /* far:0b41:0f62 */
extern void floor_foreground_resource(void);        /* far:0b41:0f1e */
extern int get_cell_index(char, char);               /* far:02cc:020a */
extern void draw_single_floor(int);                 /* far:0b41:0e60 */
extern void scan_floor(int, struct box *);           /* far:0b41:0e1c */
extern char first_in_row(char);                     /* far:02cc:07c6 */
extern void reach_rebirth_point(int);               /* far:052d:0fc4 */
extern void scroll_guard_view(char, char);            /* far:0b41:190c */
extern int distance_to_guard(void);                  /* far:02cc:1cac */
extern int guard_start_x(int);                     /* far:02cc:0360 */
extern struct guard_record *guard_record(int, int); /* far:2344:08b4 */
extern void init_room_set_c(void);                  /* far:29cf:08c6 */
extern void init_room_set_c_level5(void);            /* far:29cf:0c1c */
extern char read_cell(char, char, char);              /* far:02cc:00f0 */
extern int blocks_guard_view(char);                  /* far:078a:28fe */
extern int blocks_guard_floor(char);                 /* far:078a:2830 */
extern int obscures_guard_floor(char);               /* far:078a:28c8 */
extern void reset_level_display(void);               /* far:078a:29a6 */
extern int lose_game_dialog(int);                   /* far:0284:0226 */
extern void *memset(void *, int, unsigned);
#pragma intrinsic(memset)
int cdecl abs(int);
#pragma intrinsic(abs)
int restart_level(signed char requested);
void reset_level_overlays(void);
void start_kid(void);
void finish_start_kid(void);
void special_entrance(void);
void open_start_gate(void);
int first_frame(signed char requested);
void clean_cut(void);
int run_frames(void);
int next_frame(void);
int run_kid(void);
void run_shadows(void);
int advance_frame(void);
void reset_frame_vars(void);
void quick_foreground(void);
void quick_floor(void);
void check_rebirth_points(void);
void adjust_guard_view(void);
void tick_misc_timers(void);
void face_first_guard(void);
void reset_guard_records(void);
void init_level_specific(void);
void check_alert(void);
void call_level_frame_proc(void);
signed char reset_level(void);
void lose_game(void);


int start_game(signed char requested)
{
    check_graphics_port();
    reset_level_overlays();
    skip_frame = freeze_ticks = sequence_delay = pending_sequence = 0;
    if (!resume_mode) {
        remaining_minutes = 75;
        remaining_ticks = 719;
        starting_health = 3;
        if (option_flags && find_option("LEVEL")) {
            signed char health = stage_number;
            health = health < 3 ? 3 : health;
            starting_health = health > 12 ? 12 : health;
        }
    }
    return requested = restart_level(requested);
}

int restart_level(signed char requested)
{
    int reload;
    signed char previous_request;

    if (requested) do {
        if (requested == -1 || requested > 14)
            break;
        if (!resume_mode) {
            reload = check_level_change(level, requested);
            if (preview_mode) {
                previous_request = game_request;
                game_request = requested;
                requested = select_preview_level();
                game_request = previous_request;
                reload = 1;
            }
            if ((signed char)(requested ^ level))
                reload = 1;
        } else {
            reload = 1;
        }

        if (reload) {
            reset_level_overlays();
            load_level(requested);
        } else {
            load_level_minimum(level);
        }
        reset_guard_records();
        initialize_guard_controls();
        InitInput();
        init_level_specific();
        current_room = 0;
        special_sequence = fade_ticks = control_lock = transition_pending = collision_pending = 0;
        last_track = last_sound = -1;
        Kid.pushed = 0;
        shadows[0].direction = 0x56;
        start_kid();
        open_start_gate();
        requested = first_frame(requested);
        if (requested != -1) {
            if (recording_mode)
                start_recording();
            requested = run_frames();
            if (requested != -1)
                finish_level();
            if (recording_mode) {
                finish_recording();
                requested = level = 0;
                next_level = 0;
            }
        }
    } while (requested);
    return requested;
}

void reset_level_overlays(void)
{
    reset_vector_a();
    reset_vector_b();
    reset_vector_c();
    reset_vector_d();
}

void start_kid(void)
{
    int seq;
    signed char repeat;

    Char.action = 0;
    if (stage_number == 7)
        special_entrance();
    room_to_draw = Char.room = entry_room;
    Char.column = entry_cell % 10;
    Char.row = entry_cell / 10;
    Char.direction = ~entry_facing;
    seq = starting_sequence();
    Char.x = column_x[Char.column] + 14;
    Char.hp_delta = Char.max_hp = level ? starting_health : 4;
    repeat = 0;
    if (seq == -1) {
        if (stage_set == 5 && entry_room == 4) {
            seq = 4;
            repeat = 9;
        } else if (stage_set == 1) {
            seq = 124;
        } else if (entry_room == 27 && stage_number == 6) {
            seq = 2;
            Char.x = 240;
        } else if (entry_room == 22 && stage_number == 10) {
            seq = 2;
            Char.x = 310;
        } else if (stage_number == 14 && Char.room == 1 && stage_number == 14) {
            seq = 2;
            Char.x = 306;
        } else {
            seq = 5;
        }
    }
    start_sequence(seq);
    if (repeat) {
        do {
            advance_character();
        } while (--repeat);
    }
    finish_start_kid();
    death_countdown = -1;
    mirror_state = 0;
    room_changed = 1;
}

void finish_start_kid(void)
{
    Char.frame = 10;
    Char.action = 0;
    Char.alert = 3;
    *(volatile int *)&Char.collision = 0;
    *(volatile int *)&Char.previous_sequence_x = 0;
    Char.opponent = Char.alive = -1;
    reset_velocity();
    face_joystick();
    action_timer = guard_timer = block_ticks = view_retry = 0;
    Char.skill = start_facing == -1 ? -1 : 0;
    advance_character();
    Kid = Char;
}

void special_entrance(void)
{
    if (entrance_mode == 2) {
        entry_room = 4;
        entry_cell = 25;
        entry_cells_a[0] = (unsigned) ((unsigned char *)entry_cells_a)[1] << 8;
    } else {
        entry_room = 32;
        entry_cell = 24;
        entry_cells_b[0] = (unsigned) ((unsigned char *)entry_cells_b)[1] << 8;
    }
}

void open_start_gate(void)
{
    signed char cell;

    if (Kid.room == entry_room) {
        select_room(Kid.room);
        cell = 0;
        do {
            if (room_kinds[cell] == 17)
                goto found_gate;
        } while (++cell < 30);
        return;
found_gate:
        open_entrance(Kid.room, cell);
        play_sound(10009);
    }
}

int first_frame(signed char requested)
{
    int result;
    room_to_draw = Kid.room;
    result = check_frame_input(requested != -2 ? 1 : 0);
    if (result == -2) {
        if (!allow_fade())
            start_named_effect(loading_name);
        else
            update_display(1);
        sequence_delay = pending_sequence = 0;
        clean_cut();
        update_health(Kid.max_hp, Kid.hp_delta);
        tick_delay = 5;
        if (tick_delay) {
            do {
                wait_refresh();
            } while (tick_delay);
        }
    }
    return result;
}

void clean_cut(void)
{
    cut_required = 0;
    if (draw_interlude) {
        show_interlude(interlude_name, 12);
    } else {
        input_enabled = 1;
        wait_refresh();
        start_room_pass();
        wait_refresh();
        queued_track = room_sound_offset;
        init_room_view();
        finish_room_view();
        wait_refresh();
        if (sound_enabled)
            set_room_view_mode(0);
        wait_refresh();
        draw_interlude = input_enabled = 0;
        memset(room_render_flags, 0, sizeof(room_render_flags));
        draw_room();
        wait_refresh();
        finish_room_view();
        wait_refresh();
        if (message_frames)
            play_room_sound((char *)room_sound_offset);
        wait_refresh();
        finish_overlay_pass();
        show_message(interlude_name);
        update_interlude();
        wait_refresh();
        if (message_frames)
            play_room_sound((char *)room_sound_offset);
        wait_refresh();
    }
    draw_pass = 2;
}

int run_frames(void)
{
    signed char request;

    do {
        reset_frame_vars();
        request = next_frame();
        if (request == -2) {
            if (restart_request) {
                request = reset_level();
            } else if (level != next_level && !sound_busy() &&
                       (!start_level_track(resource_index + 10000) ||
                        resource_index == 4 || resource_index == 7 || resource_index == 54)) {
                if (next_level == -1 || (stage_number != 9 && stage_number != 5))
                    stop_sound(0);
                starting_health = Kid.hp_delta;
                request = next_level;
                finish_level_scene();
            } else {
                request = advance_frame();
                if (request == -2) {
                    if (tick_delay) {
                        script_status = 0;
                        set_frame_wait(1);
                    } else {
                        script_status = 1;
                    }
                }
            }
        }
        limit_frame_rate();
    } while (request == -2);
    return request;
}

int next_frame(void)
{
    int result;

    start_floor_frame();
    update_floors();
    if (end_room == 2 || shadows[0].action == 10)
        level_special_floor();
    select_guards_in_room(current_room);
    check_alert();
    result = run_kid();
    if (!result) {
        run_shadows();
        if (*(volatile int *)&freeze_ticks == result) {
            advance_guard_frame();
            check_guard_changes();
        }
        update_sound();
        check_rebirth_points();
        call_level_frame_proc();
        advance_screen_logic();
        update_guard_frame();
        result = check_frame_input(0);
        if (result == -2) {
            wait_overlay_frame();
            check_clock();
            if (!remaining_minutes) {
                result = -1;
                lose_game();
                game_request = 0;
                return result;
            }
        }
    } else if (result != -1) {
        result = -2;
        run_shadows();
        call_level_frame_proc();
        advance_screen_logic();
    }
    return result;
}


int run_kid(void)
{
    volatile signed char opponent;
    int saved_result;
    int result;

    load_kid();
    if (!test_character_control()) {
        opponent = Char.opponent;
        if (opponent == -1) {
            opponent = find_opponent(1);
            if (opponent == -1)
                opponent = 0;
        }
        set_opponent(opponent);
        update_character_cells();
        result = control_kid();
        if (result == -1)
            goto save_result;
        if (restart_request)
            goto restart;
        advance_character();
        if (!Char.sequence)
            goto normal_result;
        update_position();
        update_fall();
        check_controls();
        update_character_cells();
        check_collisions();
        face_first_guard();
        collide_floor();
        collide_walls();
        collide_objects();
        process_floor();
        process_gate();
        switch (stage_set) {
        case 2:
            prepare_room_set();
            update_room_set_a();
            if (!freeze_ticks)
                finish_room_set();
            /* fall through */
        case 4:
            update_room_set_b();
            if (!freeze_ticks)
                touch_room_set_b();
            break;
        case 3:
            goto room_set_c;
        }
finish_room_set_update:
        adjust_guard_view();
normal_result:
        result = 0;
        goto save_result;
room_set_c:
        prepare_room_set();
        if (!freeze_ticks) {
            update_room_set_c();
            finish_room_set();
            touch_room_set_c();
        }
        goto finish_room_set_update;
restart:
        result = 1;
save_result:
        saved_result = result;
        if (death_countdown >= 0) {
            if (!death_countdown)
                schedule_sound(26);
            --death_countdown;
        }
    } else {
        saved_result = control_kid();
        release_character_control();
    }
    Kid = Char;
    return saved_result;
}

void run_shadows(void)
{
    signed char index = 0;
    signed char count = room_states[current_room].guard_count;
    unsigned char old_room;

    if (count > 0) do {
        load_shadow(index);
        if (Char.direction == 0x56 || test_character_control() ||
            (Char.y > 254 && !((struct room_links *)room_states)[(unsigned char)Char.room].left)) {
            release_character_control();
        } else {
            set_shadow_opponent(index);
            update_character_cells();
            control_shadow();
            old_room = current_room;
            if (old_room != (unsigned char)Char.room) {
                current_room = Char.room;
                change_view_room();
            }
            advance_character();
            if (Char.action == 2)
                shadow_bump();
            if ((Char.x >= 34 && Char.x <= 545) ||
                (stage_number == 5 && Char.row == 1 &&
                 (Char.room == 10 || Char.room == 7 || Char.room == 12)) ||
                ((Char.room == 7 || Char.room == 8) && stage_set == 6)) {
                update_position();
                update_fall();
                check_controls();
                update_character_cells();
                check_collisions();
                if (Char.floor != 9) {
                    collide_shadow_floor();
                    collide_shadow_walls();
                    process_floor();
                    process_gate();
                    switch (stage_set) {
                    case 2:
                        prepare_room_set();
                        update_room_set_a();
                        finish_room_set();
                        /* fall through */
                    case 4:
                        update_room_set_b();
                        touch_room_set_b();
                        break;
                    case 3:
                        prepare_room_set();
                        update_room_set_c();
                        finish_room_set();
                        touch_room_set_c();
                        break;
                    case 6:
                        update_room_set_d();
                        break;
                    }
                } else {
                    Char.row = sequence_row(Char.y);
                }
            }
            if (Char.action == 12)
                check_shadow_exit();
            {
                unsigned char new_room = Char.room;
                if (old_room != new_room) {
                    current_room = old_room;
                    change_view_room();
                }
            }
        }
        save_shadow();
    } while (++index < count);

    index = 0;
    if (count > 0) do {
        if (shadows[index].direction == 0x56) {
            load_shadow(index);
            remove_shadow(current_room, index);
            if (Kid.opponent > Char.frame)
                --Kid.opponent;
            else if (Kid.opponent == Char.frame)
                Kid.opponent = find_opponent(Char.direction);
            --index;
            --count;
        }
    } while (++index < count);
    shadows_processed = 0;
}

int advance_frame(void)
{
    int result, phase;

    ++frame_count;
    result = -2;
    if (redraw_frame) {
        clean_cut();
        redraw_frame = 0;
    } else if (cut_required) {
        current_room = room_to_draw;
        clean_cut();
    } else if (reload_room) {
        reload_room = 0;
        select_room_view(current_room);
        clean_cut();
        update_room_screen();
    } else {
        memset(room_render_flags, 0, sizeof(room_render_flags));
        draw_room();
        finish_room_view();
        if (draw_interlude)
            start_named_effect(interlude_name);
        if (message_frames)
            play_room_sound((char *)room_sound_offset);
        render_screen();
        if (message_frames) {
            play_room_sound((char *)room_sound_offset);
            if (message_frames == 1)
                check_message_end();
            else if (Kid.alive < 0)
                --message_frames;
        }
    }
    step_sound();
    finish_queued_sound(&queued_sound);
    if (sequence_delay == 1) {
        if (pending_sequence == 36 || pending_sequence == 600) {
            game_request = 0;
            result = -1;
        } else {
            update_display(1);
        }
        return result;
    }
    if (sequence_delay && pending_sequence != 1188) {
        if (special_sequence != -1)
            --sequence_delay;
        if (pending_sequence == 600) {
            if (current_room == 4 && stage_number == 13 && !special_sequence)
                special_sequence = 1;
            if ((unsigned)sequence_delay < 120) {
                phase = (unsigned)sequence_delay % 12;
                if (phase > 3) {
                    update_display(0);
                    return result;
                }
                if (phase == 3) {
                    update_flash();
                    set_sound_flags(-2);
                }
            }
        }
    }
    return result;
}

void reset_frame_vars(void)
{
    signed char index, count;

    tick_delay = Kid.skill == 1 ? 6 : 5;
    Kid.pushed = 0;
    Kid.previous_bounds = Kid.bounds;
    if (Kid.collision == 8 && (Kid.previous_x != 52 || Kid.hurt))
        Kid.collision = 0;
    index = 0;
    count = room_states[current_room].guard_count;
    if (count > 0) do {
        load_shadow(index);
        Char.pushed = 0;
        Char.previous_bounds = Char.bounds;
        if (Char.collision == 8)
            Char.collision = 0;
        save_shadow();
    } while (++index < count);
    tick_misc_timers();
}

void quick_foreground(void)
{
    if (Char.frame == 10)
        draw_floor_foreground((void far *)floor_foreground_resource, &Char.bounds, -1);
}

void quick_floor(void)
{
    int floor = Char.floor;
    unsigned seq = Char.sequence;

    if (seq >= 135 && seq < 145 && !Char.direction) {
        draw_single_floor(get_cell_index(sweep_right, sweep_row));
        return;
    }
    if (floor == 2 || floor == 3 || floor == 4 || floor == 6 || seq == 79 || seq == 81)
        goto normal_sweep;
    if ((Char.action == 7 || Char.action == 8) &&
               (Char.alternate_x == 137 || Char.alternate_x == 138 || Char.alternate_x == 143 ||
                Char.alternate_x <= 144 || Char.alternate_x == 143 || Char.alternate_x == 144)) {
        if ((sweep_column = sweep_right) >= sweep_left) do {
            scan_floor(get_cell_index(sweep_column, sweep_row), &Char.bounds);
        } while (sweep_left <= --sweep_column);
        return;
    }
    return;
normal_sweep:
        if ((sweep_column = sweep_right) >= sweep_left) do {
            if (floor != 2)
                scan_floor(get_cell_index(sweep_column, previous_sweep_row), &Char.bounds);
            if (previous_sweep_row != sweep_row &&
                !(Char.alternate_x == 28 && (Char.room == 7 || Char.room == 8) && stage_set == 6))
                scan_floor(get_cell_index(sweep_column, sweep_row), &Char.bounds);
        } while (sweep_left <= --sweep_column);
}


void check_rebirth_points(void)
{
    int index;
    const struct rebirth_point *point;

    if (Kid.room && Kid.floor == 4 && Kid.alive < 0 && abs(Kid.pushed) < Kid.max_hp &&
        Kid.action != 1 && (Kid.room != 9 || stage_number != 8 || start_facing == 1)) {
        index = 0;
        point = rebirth_points;
        do {
            if (Kid.room == point->room && first_in_row(Kid.row) + Kid.column == point->cell &&
                (!rebirth_data || *rebirth_data - index != 1))
                reach_rebirth_point(index + 1);
            ++index;
        } while (++point < rebirth_points + 2);
    }
}

void adjust_guard_view(void)
{
    if (view_retry) {
        scroll_guard_view(Char.room, (signed char)(Char.row - (view_retry >= 1 ? 1 : 0)));
        view_retry = 0;
    }
}

void tick_misc_timers(void)
{
    if (fade_ticks > 0 && --fade_ticks == 0)
        --fade_ticks;
    if (freeze_ticks > 0)
        --freeze_ticks;
    if (block_ticks)
        --block_ticks;
}

void face_first_guard(void)
{
    if (FChar.direction != 0x56 && guard_start_delay >= 2 && Char.skill != 1 &&
        FChar.skill && FChar.floor < 2 && FChar.direction != Char.direction &&
        FChar.action != 11 && FChar.action != 7 && FChar.action != 8 &&
        !(FChar.action == 12 && FChar.sequence >= 179 && FChar.sequence <= 235) &&
        !(FChar.action == 4 && FChar.sequence == 206) &&
        !(FChar.action == 2 && FChar.alternate_x == 110) &&
        Char.alive < 0 && abs(distance_to_guard()) <= 15) {
        reset_velocity();
        Char.fall = 0;
        if (Char.sequence >= 246 && Char.sequence <= 261) {
            Char.x = guard_start_x(-6);
            return;
        }
        start_sequence(47);
        advance_character();
    }
}

void reset_guard_records(void)
{
    unsigned char room, limit;
    int index;
    struct guard_record *guard;

    room = 1;
    if (room_count >= 1) {
        do {
            index = 0;
            do {
                guard = guard_record(room, index++);
                if (guard)
                    guard->active = 0;
            } while (guard);
            limit = room_count;
            ++room;
        } while (room <= limit);
    }
}

void init_level_specific(void)
{
    if (stage_set == 3) {
        init_room_set_c();
        if (stage_number == 5)
            init_room_set_c_level5();
    } else if (stage_set == 4) {
        stage_state = 0;
    }
}

void check_alert(void)
{
    signed char left, right, kind, index, swap, second, first, count;
    unsigned int sequence;

    if (Kid.room) {
        sequence = Kid.sequence;
        first = find_opponent(0);
        second = find_opponent(-1);
        count = room_states[(unsigned char)Kid.room].guard_count;
        index = 0;
        if (count > 0) do {
            load_shadow(index);
            if (!sequence || (sequence >= 217 && sequence < 226) || Kid.alive >= 0 || Char.alive >= 0 ||
                Char.direction == 0x56 || (Kid.row != Char.row && Char.action != 7 && Char.action != 8) ||
                (Char.action == 11 && Char.collision == 6) || (Kid.action == 1 && stage_set == 2)) {
                Char.alert = 0;
            } else {
                Char.alert = 3;
                left = Kid.column;
                right = Char.column;
                if (left > right) {
                    swap = left;
                    left = right;
                    right = swap;
                }
                if (read_cell(Kid.room, right, Kid.row) == 4)
                    --right;
                if (left < right) {
                    if (left <= right) do {
                        if (Char.alert != 3)
                            break;
                        kind = read_cell(Kid.room, left, Kid.row);
                        if (blocks_guard_view(kind) &&
                            !(Char.action == 11 && kind == 7 && (cell_flags & 0x80))) {
                            Char.alert = 0;
                        } else if ((kind == 4 && cell_flags < 0x70) ||
                                   (blocks_guard_floor(kind) && Char.action != 10) ||
                                   (obscures_guard_floor(kind) && Char.action != 10 &&
                                    Char.action != 7 && Char.action != 8)) {
                            Char.alert = 1;
                        }
                    } while (right >= ++left);
                }
                if (Char.alert == 3 && Char.action != 11) {
                    if (index != second && index != first)
                        Char.alert = 2;
                    else if (Char.action == 10)
                        Char.skill = 1;
                }
            }
            save_shadow();
        } while (++index < count);
    }
}

void call_level_frame_proc(void)
{
    unsigned long address = (unsigned long)level_frame_procs[stage_set];
    if (address)
        ((frame_proc)address)();
}

signed char reset_level(void)
{
    restart_request = 0;
    if (level == next_level) {
        reset_level_display();
        start_named_effect(interlude_name);
        if (game_request)
            return level;
        return 0;
    }
    return next_level;
}

void lose_game(void)
{
    int result;

    stop_sound(0);
    result = lose_game_dialog(28);
    remaining_minutes = 0;
    key_result = 8;
    if (result == 2)
        reset_level_overlays();
}


