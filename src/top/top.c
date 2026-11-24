/* TOP.C candidate, DOS frame 0e56. Definitions follow original order.
 * Address comments identify the observed operands behind external declarations. */
extern signed char LevelIdx;                          /* ds:0a18 */
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
    volatile int field_34;                      /* 34 */
    int previous_mode;                 /* 36 */
    unsigned char behavior;            /* 38 */
    signed char opponent_index;        /* 39 */
    unsigned char field_3a;            /* 3a */
    unsigned char unknown_3b[5];        /* 3b..3f */
} CHARREC;
struct GetOppInfoIdx { char prefix[7]; int active; };
typedef void (far pascal *frame_proc)(void);

extern CHARREC Opp;                    /* ds:5c40 */
extern CHARREC Char;                     /* ds:5c00 */
extern CHARREC RoomOpp[5];                /* ds:5cc0 */
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

extern frame_proc level_frame_procs[];             /* ds:06d4 */
extern int far *rebirth_data;                      /* ds:5bfc */
extern signed char game_request;                   /* ds:6ccc */
extern signed char CutRoom;                   /* ds:6ccd */
extern signed char starting_health;                /* ds:6cd1 */
extern int resume_mode;                            /* ds:5e00 */
extern struct { signed char argument, entrance_mode; } entrance_state;
extern struct {
    signed char facing;
    signed char death_countdown;
} player_start;
extern int mirror_state;                           /* ds:5e08 */
extern int room_sound_offset;                      /* ds:5e0c */
extern int room_changed;                           /* ds:5e0e */
extern int gameplay_dialog_active;                            /* ds:5e18 */
extern int debug_reload_room;                           /* ds:5e1a */
extern int draw_interlude;                         /* ds:5e1c */
typedef struct {
    unsigned char enabled;
    signed char cue;
    int song;
    int delay;
    int reserved;
} AmbientState;
extern AmbientState ambient_state;                           /* ds:5e22 */
extern unsigned remaining_minutes;                 /* ds:5e2a */
extern int restart_request;                        /* ds:5e2c */
extern int skip_frame;                             /* ds:5e2e */
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
extern LEVELREC Level;
extern int freeze_ticks;                           /* ds:5e3e */
extern int sequence_delay;                    /* ds:5e40 */
extern int pending_sequence;                       /* ds:5e42 */
extern int remaining_ticks;                        /* ds:5e44 */
extern int next_level;                             /* ds:5e46 */
extern int cut_required;                           /* ds:5e5e */
extern unsigned long FrameNum;                             /* ds:5e60 */
extern int Invert;                         /* ds:5e8e */
extern int block_ticks;                       /* ds:5e90 */
extern volatile int tick_delay;                             /* ds:264e */
extern int fade_active;
extern int recording_mode;                         /* ds:2cfa */
extern int plateau_room_loaded;
extern int plateau_state_1;
extern int plateau_state_2;
extern unsigned int escher_room_eight_seen;
extern int draw_pass;                              /* ds:6a4a */
extern int input_enabled;                          /* ds:6266 */
extern int Graf;                           /* ds:25c0 */
extern int sound_enabled;                          /* ds:0228 */
extern int special_sequence;                       /* ds:0a16 */
extern int resource_index;                         /* ds:0902 */
extern int last_sound;                             /* ds:08fe */
extern int last_track;                             /* ds:0900 */
extern int option_flags;                           /* ds:1142 */
extern int nis_skip_requested;                           /* ds:1504 */
extern int view_retry;                             /* ds:6296 */
extern int kid_step;                      /* ds:6298 */
extern int action_timer;                           /* ds:629a */
extern int guard_timer;                            /* ds:629c */
extern int control_lock;                           /* ds:629e */
extern int collision_pending;                      /* ds:62de */
extern int transition_pending;                     /* ds:67cc */
extern char timer_disabled;                            /* ds:01d6 */
#pragma pack(1)
typedef struct {
    signed char kind;                                 /* 00 */
    unsigned int modifier;                            /* 01 */
    signed char index;                                /* 03 */
    unsigned char room;                               /* 04 */
    signed char column, row;                          /* 05-06 */
    signed char first_col, last_col;                   /* 07-08 */
    unsigned char top_row, bottom_row;                 /* 09-0a */
} CELLREC;
#pragma pack()
extern CELLREC Cell;
extern int ColEdgeX[20];
extern char room_render_flags[10];                  /* ds:6248 */
extern unsigned char *room_kinds;                   /* ds:6294 */
static char level_option[] = "LEVEL";
struct char_bounds GamePlayRect = { 0, 0, 192, 320 };
extern char loading_name[];                         /* ds:1fe0 */

extern void CheckBPort(void);              /* far:0a5c:004e */
extern int find_option(char far *);                  /* far:1101:29aa */
extern int CheckPlayNIS(int, int);             /* far:0284:000e */
extern char poll_and_clear_pending_state(void);             /* far:0000:0210 */
extern void load_level(char value);                       /* far:0a5c:01b4 */
extern void load_minimum_level(char);               /* far:0a5c:02f4 */
extern void InitCollisionDetection(void);        /* far:2802:0848 */
extern void InitInput(void);                        /* far:02cc:1404 */
extern void reset_recorded_playback(void);                  /* far:0da4:0252 */
extern void CheckRecordedGamePlayer(void);                 /* far:0da4:0122 */
extern void clear_peels(void);                     /* far:0169:0794 */
extern void reset_vector_a(void);                   /* far:2203:0d21 */
extern void reset_vector_b(void);                   /* far:2203:0d0d */
extern void reset_vector_c(void);                   /* far:2203:0d17 */
extern void reset_vector_d(void);                   /* far:2203:0d2b */
extern int SetRebirthPtKidStatus(void);                 /* far:052d:1234 */
extern void JumpSeq(int);                    /* far:25da:0016 */
extern void AnimChar(void);               /* far:02cc:0394 */
extern void reset_char_y(void);                   /* far:02cc:07a2 */
extern void ClearCharVel(void);                    /* far:02cc:0944 */
extern void select_room_arrays(char);                       /* far:0f77:0002 */
extern void close_exit(char, char);               /* far:0b41:16fa */
extern void hGetSnd(int);                        /* far:1101:807a */
extern int PrepCut(int);                  /* far:0000:0dd4 */
extern int IsLevelStartWithNISScreen(void);                         /* far:0284:0096 */
extern void STROKE_PORT_RECT(char far *);          /* far:1101:4d90 */
extern void clear_message(int);                    /* far:078a:213a */
extern void DrawKidMeter(int, int);                /* far:078a:24ee */
extern void clear_keyboard(void);                     /* far:0000:125a */
extern void SET_RECORD_RANGE(char far *, int);         /* far:1101:6650 */
extern void full_redraw(void);                  /* far:078a:0000 */
extern void CheckRoomSpecific(void);                   /* far:04a6:003e */
extern void draw_all(void);                 /* far:078a:13b2 */
extern void UnloadCustomRoomShapes(int);                 /* far:04a6:0784 */
extern void full_redraw_fast(void);                        /* far:078a:12e4 */
extern void InvertBitMap(char far *);            /* far:1ead:01a2 */
extern void load_opponent_banks(void);              /* far:2344:0f4e */
extern void PrinceCopy(char far *);               /* far:0000:1268 */
extern void restore_game_play_palette(void);                 /* far:078a:293a */
extern int is_leave_level_music_on(void);                         /* far:0dd7:02ce */
extern int query_sound(int);                  /* far:1101:8444 */
extern void dispatch_sound(int);                        /* far:1101:83f0 */
extern void RemoveRebirthPt(void);                /* far:052d:109e */
extern void wait_timer_no_poll(int);                    /* far:1de8:015a */
extern void limit_frame_rate(void);                 /* far:107d:000c */
extern void anim_mobs(void);                /* far:0b41:1a42 */
extern void AnimTrobs(void);                    /* far:0b41:000c */
extern void raise_nearby_opponents(void);              /* far:2c3e:0f6c */
extern void check_generator_points(unsigned char);   /* far:2344:0a52 */
extern void dispatch_guard_reaction(void);              /* far:2344:1f0e */
extern void check_stab(void);              /* far:2344:198e */
extern void add_actor_sfx(void);                     /* far:0dd7:016c */
extern void ChangeMeters(void);              /* far:0000:0f70 */
extern void check_cut(void);               /* far:2344:1088 */
extern void cut_room_opponents(void);               /* far:2344:0fae */
extern void KeepTime(void);                      /* far:0000:0cbc */
extern void copy_kid_to_char(void);                         /* far:02cc:1d94 */
extern int is_undefined_room(void);            /* far:02cc:0eb6 */
extern signed char pick_hidden_actor(signed char);       /* far:2344:08f0 */
extern void load_kid_with_opponent(signed char);               /* far:02cc:07fc */
extern void reread_cells(void);           /* far:02cc:027e */
extern int ctrl_player(void);                       /* far:02cc:116e */
extern void Gravity(void);                  /* far:02cc:08d6 */
extern void add_char_velocity(void);                       /* far:02cc:090e */
extern void setup_char(void);                   /* far:0169:09ee */
extern void get_char_edges(void);                 /* far:2802:09ca */
extern void CheckBarr(void);                    /* far:2802:000e */
extern void collide_with_barriers(void);                    /* far:2802:0402 */
extern void check_gate_for_player(void);                  /* far:2802:0d56 */
extern void check_floor(void);                    /* far:0b41:06f8 */
extern void check_press(void);                     /* far:0b41:075e */
extern void CheckSpikes(void);                 /* far:101f:01ae */
extern void check_trapdoor(void);                /* far:2a4e:0e44 */
extern void CheckImpale(void);                  /* far:101f:00a0 */
extern void CheckScythe(void);                /* far:29cf:00b2 */
extern void CheckScytheHit(void);                 /* far:29cf:0118 */
extern void check_dart_hit(void);                /* far:29cf:01f4 */
extern void check_lava_boil(void);                 /* far:29cf:06e8 */
extern void AddSound(int);                    /* far:0dd7:01ce */
extern void FallingInUndefinedRoom(void);        /* far:02cc:0dda */
extern void load_actor_slot(char);                      /* far:02cc:1d42 */
extern void load_shadow_with_opponent(char);              /* far:02cc:086a */
extern void shad_ctrl(void);                   /* far:02cc:12e4 */
extern void get_rooms(void);                 /* far:078a:0024 */
extern void check_multiple_dead_bodies(void);                      /* far:2c3e:0052 */
extern void collide_enemy_with_barrier(void);              /* far:2802:0e82 */
extern void check_gate_for_opponent(void);              /* far:2802:0c94 */
extern void check_fireball_hit_character(void);                 /* far:29cf:1b60 */
extern char GetRow(int);                      /* far:02cc:1102 */
extern void check_jinnee_column(void);                 /* far:2c3e:1674 */
extern void save_actor_slot(void);                       /* far:02cc:1cf6 */
extern void RemoveOppFromRoom(unsigned char, int);       /* far:2344:07bc */
extern void LoadCustomRoomData(char);                  /* far:04a6:02b0 */
extern void draw_meters(void);               /* far:078a:2590 */
extern void copy_dirty_rectangles(void);                    /* far:078a:218e */
extern void ToggleInvertState(void);                 /* far:0000:12de */
extern void play_sounds(void);                       /* far:0dd7:04d0 */
extern void play_ambient(AmbientState *);              /* far:0dd7:03cc */
extern void draw_restart_message(void);                     /* far:078a:20a8 */
extern int play_sound(unsigned);                    /* far:1101:842c */
extern void mark_bufs_for_rect(void far *, struct char_bounds *, char); /* far:0b41:0f62 */
extern void mark_foreground(void);        /* far:0b41:0f1e */
extern int index_cell(char, char);               /* far:02cc:020a */
extern void mark_half(int);                 /* far:0b41:0e60 */
extern void mark_floor(int, struct char_bounds *);           /* far:0b41:0e1c */
extern char FirstInRow(char);                     /* far:02cc:07c6 */
extern void SaveRebirthPtStatus(int);               /* far:052d:0fc4 */
extern void shake_row(char, char);            /* far:0b41:190c */
extern int get_opp_dist(void);                  /* far:02cc:1cac */
extern int AddCharX(int);                     /* far:02cc:0360 */
extern struct GetOppInfoIdx *GetOppInfoIdx(int, int); /* far:2344:08b4 */
extern void clear_lava_slots(void);                  /* far:29cf:08c6 */
extern void initialize_bridge(void);            /* far:29cf:0c1c */
extern char ReadCell(char, char, char);              /* far:02cc:00f0 */
extern int IsWall(char);                  /* far:078a:28fe */
extern int IsImpassable(char);                 /* far:078a:2830 */
extern int IS_SIX_KIND(char);               /* far:078a:28c8 */
extern void save_game_play_palette(void);               /* far:078a:29a6 */
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
extern CHARREC Kid;                      /* ds:5c80 */
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
    CheckBPort();
    reset_level_overlays();
    skip_frame = freeze_ticks = sequence_delay = pending_sequence = 0;
    if (!resume_mode) {
        remaining_minutes = 75;
        remaining_ticks = 719;
        starting_health = 3;
        if (option_flags && find_option(level_option)) {
            signed char health = Level.level_number;
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
            reload = CheckPlayNIS(LevelIdx, requested);
            if (nis_skip_requested) {
                previous_request = game_request;
                game_request = requested;
                requested = poll_and_clear_pending_state();
                game_request = previous_request;
                reload = 1;
            }
            if (requested != LevelIdx)
                reload = 1;
        } else {
            reload = 1;
        }

        if (reload) {
            reset_level_overlays();
            load_level(requested);
        } else {
            load_minimum_level(LevelIdx);
        }
        reset_guard_records();
        InitCollisionDetection();
        InitInput();
        init_level_specific();
        Room.number = 0;
        special_sequence = control_lock = transition_pending = collision_pending = 0;
        last_track = last_sound = -1;
        Kid.life_delta = 0;
        RoomOpp[0].direction = 0x56;
        start_kid();
        open_start_gate();
        requested = first_frame(requested);
        if (requested != -1) {
            if (recording_mode)
                reset_recorded_playback();
            requested = run_frames();
            if (requested != -1)
                clear_peels();
            if (recording_mode) {
                CheckRecordedGamePlayer();
                requested = LevelIdx = 0;
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

    Char.character_kind = 0;
    if (Level.level_number == 7)
        special_entrance();
    CutRoom = Char.room = Level.entry_room;
    Char.column = Level.entry_cell % 10;
    Char.row = Level.entry_cell / 10;
    Char.direction = ~Level.entry_facing;
    seq = SetRebirthPtKidStatus();
    Char.x = ColEdgeX[Char.column + 5] + 14;
    Char.life_initial = Char.life_points = starting_health;
    repeat = 0;
    if (seq == -1) {
        if (Level.environment_kind == 5 && Level.entry_room == 4) {
            seq = 4;
            repeat = 9;
        } else if (Level.environment_kind == 1) {
            seq = 124;
        } else if (Level.entry_room == 27 && Level.level_number == 6) {
            seq = 2;
            Char.x = 240;
        } else if (Level.entry_room == 22 && Level.level_number == 10) {
            seq = 2;
            Char.x = 310;
        } else if (Level.level_number == 14 && Char.room == 1 && Level.level_number == 14) {
            seq = 2;
            Char.x = 306;
        } else {
            seq = 5;
        }
    }
    JumpSeq(seq);
    if (repeat) {
        do {
            AnimChar();
        } while (--repeat);
    }
    finish_start_kid();
    player_start.death_countdown = -1;
    mirror_state = 0;
    room_changed = 1;
}

void finish_start_kid(void)
{
    Char.slot = 10;
    Char.character_kind = 0;
    Char.alert_delay = 3;
    Char.sequence_group = 0;
    Char.field_34 = 0;
    Char.opponent_index = Char.alive = -1;
    reset_char_y();
    ClearCharVel();
    action_timer = guard_timer = block_ticks = view_retry = 0;
    Char.stance = player_start.facing == -1 ? -1 : 0;
    AnimChar();
    Kid = Char;
}

void special_entrance(void)
{
    if (entrance_state.entrance_mode == 2) {
        Level.entry_room = 4;
        Level.entry_cell = 25;
        Level.cell_info[31][24].modifier = (unsigned) ((unsigned char *)&Level.cell_info[31][24].modifier)[1] << 8;
    } else {
        Level.entry_room = 32;
        Level.entry_cell = 24;
        Level.cell_info[3][25].modifier = (unsigned) ((unsigned char *)&Level.cell_info[3][25].modifier)[1] << 8;
    }
}

void open_start_gate(void)
{
    signed char cell;

    if (Kid.room == Level.entry_room || Level.level_number == 7 || Level.level_number == 6) {
        select_room_arrays(Kid.room);
        cell = 0;
        while (room_kinds[cell] != 17) {
            if (++cell >= 30)
                return;
        }
        close_exit(Kid.room, cell);
        hGetSnd(10009);
    }
}

int first_frame(signed char requested)
{
    int result;
    CutRoom = Kid.room;
    result = PrepCut(requested != -2 ? 1 : 0);
    if (result == -2) {
        if (!IsLevelStartWithNISScreen())
            STROKE_PORT_RECT(loading_name);
        else
            clear_message(1);
        sequence_delay = pending_sequence = 0;
        clean_cut();
        DrawKidMeter(Kid.life_points, Kid.life_initial);
        tick_delay = 5;
        if (tick_delay) {
            do {
                clear_keyboard();
            } while (tick_delay);
        }
    }
    return result;
}

void clean_cut(void)
{
    cut_required = 0;
    if (draw_interlude) {
        SET_RECORD_RANGE((char *)&GamePlayRect, 12);
    } else {
        input_enabled = 1;
        clear_keyboard();
        full_redraw();
        clear_keyboard();
        Graf = room_sound_offset;
        CheckRoomSpecific(); draw_all();
        clear_keyboard();
        if (sound_enabled)
            UnloadCustomRoomShapes(0);
        clear_keyboard();
        draw_interlude = input_enabled = 0;
        memset(room_render_flags, 0, sizeof(room_render_flags));
        full_redraw_fast();
        clear_keyboard();
        draw_all();
        clear_keyboard();
        if (Invert)
            InvertBitMap((char *)room_sound_offset);
        clear_keyboard();
        load_opponent_banks();
        PrinceCopy((char *)&GamePlayRect);
        restore_game_play_palette();
        clear_keyboard();
        if (Invert)
            InvertBitMap((char *)room_sound_offset);
        clear_keyboard();
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
            } else if (LevelIdx != next_level && !is_leave_level_music_on() &&
                       (!query_sound(resource_index + 10000) ||
                        resource_index == 4 || resource_index == 7 || resource_index == 54)) {
                if (next_level == -1 || (Level.level_number != 9 && Level.level_number != 5))
                    dispatch_sound(0);
                starting_health = Kid.life_initial;
                request = next_level;
                RemoveRebirthPt();
            } else {
                request = advance_frame();
                if (request == -2) {
                    if (tick_delay) {
                        if (fade_active)
                            --fade_active;
                        wait_timer_no_poll(1);
                    } else if (fade_active < 20) {
                        ++fade_active;
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

    anim_mobs();
    AnimTrobs();
    if (((signed char)Level.preferred_opponent) == 2 || RoomOpp[0].character_kind == 10)
        raise_nearby_opponents();
    check_generator_points(Room.number);
    check_alert();
    result = run_kid();
    if (!result) {
        run_shadows();
        if (!freeze_ticks && Room.number) {
            dispatch_guard_reaction();
            check_stab();
        }
        add_actor_sfx();
        check_rebirth_points();
        call_level_frame_proc();
        ChangeMeters();
        check_cut();
        result = PrepCut(0);
        if (result == -2) {
            cut_room_opponents();
            KeepTime();
            if (!remaining_minutes) {
                lose_game();
                game_request = 0;
                result = -1;
            }
            return result;
        }
    } else if (result != -1) {
        result = -2;
        run_shadows();
        call_level_frame_proc();
        ChangeMeters();
    }
    return result;
}


int run_kid(void)
{
    signed char opponent;
    int saved_result;
    int result;

    copy_kid_to_char();
    if (!is_undefined_room()) {
        if ((opponent = Char.opponent_index) == -1) {
            if ((opponent = pick_hidden_actor(1)) == -1)
                opponent = 0;
        }
        load_kid_with_opponent(opponent);
        reread_cells();
        result = ctrl_player();
        if (result != -1) {
            if (!restart_request) {
                AnimChar();
                if (!Char.frame)
                    goto normal_result;
                Gravity();
                add_char_velocity();
                setup_char();
                reread_cells();
                get_char_edges();
                face_first_guard();
                CheckBarr();
                collide_with_barriers();
                check_gate_for_player();
                check_floor();
                check_press();
                switch (Level.environment_kind) {
                case 2:
                    CheckSpikes();
                    check_trapdoor();
                    if (!freeze_ticks)
                        CheckImpale();
                    /* fall through */
                case 4:
                    CheckScythe();
                    if (!freeze_ticks)
                        CheckScytheHit();
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
                CheckSpikes();
                if (!freeze_ticks) {
                    check_dart_hit();
                    CheckImpale();
                    check_lava_boil();
                }
                goto finish_room_set_update;
            } else {
                result = 1;
            }
        }
save_result:
        saved_result = result;
        if (player_start.death_countdown >= 0) {
            if (!player_start.death_countdown && Kid.alive < 0 && Kid.action != 3 && Kid.action != 4)
                AddSound(26);
            --player_start.death_countdown;
        }
    } else {
        saved_result = ctrl_player();
        FallingInUndefinedRoom();
    }
    Kid = Char;
    {
        int *status = &saved_result;
        return *status;
    }
}

void run_shadows(void)
{
    signed char index, count;

    if (!Room.number)
        return;
    index = 0;
    count = Level.room_objects[Room.number - 1].opponent_count;

    if (count > 0) do {
        load_actor_slot(index);
        if (Char.direction == 0x56 || is_undefined_room() ||
            (Char.y > 254 && !Level.room_links[(unsigned char)Char.room - 1].below)) {
            FallingInUndefinedRoom();
        } else {
            unsigned char old_room;
            load_shadow_with_opponent(index);
            reread_cells();
            shad_ctrl();
            old_room = Room.number;
            if (old_room != (unsigned char)Char.room) {
                Room.number = Char.room;
                get_rooms();
            }
            AnimChar();
            if (Char.character_kind == 2)
                check_multiple_dead_bodies();
            if ((Char.x >= 34 && Char.x <= 545) ||
                (Char.row && Level.level_number == 5 &&
                 (Char.room == 10 || Char.room == 7 || Char.room == 12)) ||
                ((Char.room == 7 || Char.room == 8) && Level.environment_kind == 6)) {
                Gravity(); add_char_velocity();
                setup_char(); reread_cells();
                get_char_edges();
                if (Char.action != 9) {
                    collide_enemy_with_barrier(); check_gate_for_opponent();
                    check_floor(); check_press();
                    switch (Level.environment_kind) {
                    case 2:
                        CheckSpikes();
                        check_trapdoor();
                        CheckImpale();
                        /* fall through */
                    case 4:
                        CheckScythe();
                        CheckScytheHit();
                        break;
                    case 3:
                        CheckSpikes();
                        check_dart_hit();
                        CheckImpale();
                        check_lava_boil();
                        break;
                    case 6:
                        check_fireball_hit_character();
                        break;
                    }
                } else {
                    Char.row = GetRow(Char.y);
                }
            }
            if (Char.character_kind == 12)
                check_jinnee_column();
            if (old_room != (unsigned char)Char.room) {
                Room.number = old_room;
                get_rooms();
            }
        }
        if (Level.environment_kind == 6 && count != Level.room_objects[Room.number - 1].opponent_count) {
            --count;
            --index;
        } else {
            save_actor_slot();
        }
    } while (++index < count);

    index = 0;
    if (count > 0) do {
        if (RoomOpp[index].direction == 0x56) {
            load_actor_slot(index);
            RemoveOppFromRoom(Room.number, index);
            if (Char.slot < Kid.opponent_index)
                --Kid.opponent_index;
            else if (Char.slot == Kid.opponent_index)
                Kid.opponent_index = pick_hidden_actor(Char.direction);
            --index;
            --count;
        }
    } while (++index < count);
    kid_step = 0;
}

int advance_frame(void)
{
    int result, phase;

    ++FrameNum;
    result = -2;
    if (debug_reload_room) {
        clean_cut();
        debug_reload_room = 0;
    } else if (cut_required) {
        Room.number = CutRoom;
        clean_cut();
    } else if (gameplay_dialog_active) {
        gameplay_dialog_active = 0;
        LoadCustomRoomData(Room.number);
        clean_cut();
        draw_meters();
    } else {
        memset(room_render_flags, 0, sizeof(room_render_flags)); full_redraw_fast(); draw_all();
        if (draw_interlude) STROKE_PORT_RECT((char *)&GamePlayRect);
        if (Invert)
            InvertBitMap((char *)room_sound_offset);
        copy_dirty_rectangles();
        if (Invert) {
            InvertBitMap((char *)room_sound_offset);
            if (Invert == 1)
                ToggleInvertState();
            else if (Kid.alive < 0)
                --Invert;
        }
    }
    play_sounds();
    play_ambient(&ambient_state);
    if (sequence_delay == 1) {
        if (pending_sequence == 36 || pending_sequence == 600) {
            game_request = 0;
            result = -1;
        } else {
            clear_message(1);
        }
    } else if (sequence_delay && pending_sequence != 1188) {
        if (special_sequence != -1)
            --sequence_delay;
        if (pending_sequence == 600) {
            if (Room.number == 4 && Level.level_number == 13 && !special_sequence)
                special_sequence = 1;
            if ((unsigned)sequence_delay < 120) {
                phase = (unsigned)sequence_delay % 12;
                if (phase > 3) {
                    clear_message(0);
                } else if (phase == 3) {
                    draw_restart_message();
                    play_sound(-2);
                }
            }
        }
    }
    return result;
}

void reset_frame_vars(void)
{
    signed char index, count;

    tick_delay = Kid.stance == 1 ? 6 : 5;
    Kid.life_delta = 0;
    Kid.previous_bounds = Kid.bounds;
    if (Kid.sequence_group == 8 && (Kid.sequence_id != 52 || Kid.sequence_position))
        Kid.sequence_group = 0;
    index = 0;
    count = Level.room_objects[Room.number - 1].opponent_count;
    if (count > 0) do {
        load_actor_slot(index);
        Char.life_delta = 0;
        Char.previous_bounds = Char.bounds;
        if (Char.sequence_group == 8)
            Char.sequence_group = 0;
        save_actor_slot();
    } while (++index < count);
    tick_misc_timers();
}

void quick_foreground(void)
{
    if (Char.slot == 10)
        mark_bufs_for_rect((void far *)mark_foreground, &Char.bounds, -1);
}

void quick_floor(void)
{
    signed char left_edge;
    int floor = Char.action;
    unsigned seq = Char.frame;

    if (seq >= 135 && seq < 145 && !Char.direction) {
        mark_half(index_cell(Cell.last_col, ((signed char)Cell.top_row)));
        return;
    }
    if (floor != 2 && floor != 3 && floor != 4 && floor != 6 && seq != 79 && seq != 81) {
        if ((Char.character_kind == 7 || Char.character_kind == 8) &&
            (Char.sequence == 137 || Char.sequence == 138 || Char.sequence == 143 ||
             (int)Char.sequence <= 144 || Char.sequence == 143 || Char.sequence == 144)) {
            if ((Cell.column = Cell.last_col) >= Cell.first_col) do {
                mark_floor(index_cell(Cell.column, ((signed char)Cell.top_row)), &Char.bounds);
                left_edge = Cell.first_col;
                --Cell.column;
            } while (Cell.column >= left_edge);
            return;
        }
        return;
    }
    if ((Cell.column = Cell.last_col) >= Cell.first_col) do {
        if (floor != 2)
            mark_floor(index_cell(Cell.column, ((signed char)Cell.bottom_row)), &Char.bounds);
        if (((signed char)Cell.bottom_row) != ((signed char)Cell.top_row) &&
            !(Char.sequence == 28 && (Char.room == 7 || Char.room == 8) && Level.environment_kind == 6))
            mark_floor(index_cell(Cell.column, ((signed char)Cell.top_row)), &Char.bounds);
        left_edge = Cell.first_col;
        --Cell.column;
    } while (Cell.column >= left_edge);
}


void check_rebirth_points(void)
{
    int index;

    if (Kid.room && Kid.action != 4 && Kid.alive < 0 && abs(Kid.life_delta) < Kid.life_points &&
        Kid.character_kind != 1 && (Kid.room != 9 || Level.level_number != 8 || player_start.facing == 1)) {
        for (index = 0; index < 2; ++index) {
            if (Kid.room == Level.rebirth_points[index].room && FirstInRow(Kid.row) + Kid.column == Level.rebirth_points[index].cell &&
                (!rebirth_data || *rebirth_data - index != 1))
                SaveRebirthPtStatus(index + 1);
        }
    }
}

void adjust_guard_view(void)
{
    if (view_retry) {
        shake_row(Char.room, (signed char)(Char.row - (view_retry >= 1 ? 1 : 0)));
        view_retry = 0;
    }
}

void tick_misc_timers(void)
{
    if (freeze_ticks > 0)
        --freeze_ticks;
    if (block_ticks)
        --block_ticks;
}

void face_first_guard(void)
{
    if (Opp.direction != 0x56 && Opp.alert_delay >= 2 && Char.stance != 1 &&
        Opp.direction != Char.direction && Opp.stance && Opp.action < 2 &&
        Opp.character_kind != 11 && Opp.character_kind != 7 && Opp.character_kind != 8 &&
        !(Opp.character_kind == 12 && Opp.frame >= 179 && Opp.frame <= 235) &&
        !(Opp.character_kind == 4 && Opp.frame == 206) &&
        !(Opp.character_kind == 2 && Opp.sequence == 110) &&
        Char.alive < 0 && abs(get_opp_dist()) <= 15) {
        reset_char_y();
        Char.vertical_step = 0;
        if (Char.frame >= 246 && Char.frame <= 261) {
            Char.x = AddCharX(-6);
            return;
        }
        JumpSeq(47);
        AnimChar();
    }
}

void reset_guard_records(void)
{
    unsigned char room, limit;
    int index;
    struct GetOppInfoIdx *guard;

    room = 1;
    if (Level.room_count >= 1) {
        do {
            index = 0;
            do {
                guard = GetOppInfoIdx(room, index++);
                if (guard)
                    guard->active = 0;
            } while (guard);
            limit = Level.room_count;
            ++room;
        } while (room <= limit);
    }
}

void init_level_specific(void)
{
    if (Level.environment_kind == 3) {
        clear_lava_slots();
        if (Level.level_number == 5)
            initialize_bridge();
    } else if (Level.environment_kind == 4) {
        plateau_state_2 = plateau_state_1 = plateau_room_loaded = 0;
    } else if (Level.environment_kind == 6) {
        escher_room_eight_seen = 0;
    }
}

void check_alert(void)
{
    signed char swap, index, left, right, kind, second, first, count;
    unsigned int sequence;

    if (Kid.room) {
        sequence = Kid.frame;
        first = pick_hidden_actor(0);
        second = pick_hidden_actor(-1);
        count = Level.room_objects[(unsigned char)Kid.room - 1].opponent_count;
        index = 0;
        if (count > 0) do {
            load_actor_slot(index);
            if (!sequence || (sequence >= 217 && sequence < 226) || Kid.alive >= 0 || Char.alive >= 0 ||
                Char.direction == 0x56 || (Kid.row != Char.row && Char.character_kind != 7 && Char.character_kind != 8) ||
                (Char.character_kind == 11 && Char.sequence_group == 6) || (Kid.character_kind == 1 && Level.environment_kind == 2)) {
                Char.alert_delay = 0;
            } else {
                Char.alert_delay = 3;
                left = Kid.column;
                right = Char.column;
                if (left > right) {
                    swap = left;
                    left = right;
                    right = swap;
                }
                if (ReadCell(Kid.room, right, Kid.row) == 4)
                    --right;
                if (left < right) {
                    if (left <= right) do {
                        if (Char.alert_delay != 3)
                            break;
                        kind = ReadCell(Kid.room, left, Kid.row);
                        if (IsWall(kind) &&
                            !(Char.character_kind == 11 && kind == 7 && ((*(unsigned char *)&Cell.modifier) & 0x80))) {
                            Char.alert_delay = 0;
                        } else if ((kind == 4 && (*(unsigned char *)&Cell.modifier) < 0x70) ||
                                   (IsImpassable(kind) && Char.character_kind != 10) ||
                                   (IS_SIX_KIND(kind) && Char.character_kind != 10 &&
                                    Char.character_kind != 7 && Char.character_kind != 8)) {
                            Char.alert_delay = 1;
                        }
                    } while (right >= ++left);
                }
                if (Char.alert_delay == 3 && Char.character_kind != 11) {
                    if (index != second && index != first)
                        Char.alert_delay = 2;
                    else if (Char.character_kind == 10)
                        Char.stance = 1;
                }
            }
            save_actor_slot();
        } while (++index < count);
    }
}

void call_level_frame_proc(void)
{
    unsigned long address = (unsigned long)level_frame_procs[Level.environment_kind];
    if (address)
        ((frame_proc)address)();
}

signed char reset_level(void)
{
    restart_request = 0;
    if (LevelIdx == next_level) {
        save_game_play_palette();
        STROKE_PORT_RECT((char *)&GamePlayRect);
        if (game_request)
            return LevelIdx;
        return 0;
    }
    return next_level;
}

extern int play_nis(int);                   /* far:0284:0226 */

void lose_game(void)
{
    int result;

    dispatch_sound(0);
    result = play_nis(28);
    remaining_minutes = 0;
    timer_disabled = 8;
    if (result == 2)
        reset_level_overlays();
}

const int TopRowRect[4] = { 0, 0, 3, 320 };
