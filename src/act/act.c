extern int actor_alert;
extern int sequence_id;
extern int repeat_sequence_id;
extern int sequence_position;
extern signed char actor_kind;
extern signed char actor_direction;
extern int actor_frame;
extern int actor_x;

extern int far pascal allocate_tagged_handle(unsigned long type, int id);
extern int far pascal AddCharX(int column);
extern void far pascal reread_cells(void);

void far pascal reset_actor_alert(void)
{
    actor_alert = 0;
}

void far pascal JumpSeq(int sequence)
{
    if (allocate_tagged_handle(0x53455153UL, sequence)) {
        sequence_id = sequence;
        repeat_sequence_id = sequence;
        sequence_position = 0;
        if ((actor_kind == 7 || actor_kind == 8) &&
            ((sequence >= 130 && sequence <= 139) || sequence == 166) &&
            actor_direction == 0) {
            actor_direction = -1;
            if (actor_frame == 158 || actor_frame == 150)
                actor_x = AddCharX(14);
            reread_cells();
        }
    }
}

extern int fall_scream_started;
extern signed char falling_speed;
extern signed char actor_room;
extern signed char actor_row;
extern int actor_y;
extern int fall_floor_y[];
extern int fall_transition;
extern signed char actor_character;
extern void far _fastcall AddKidScream(unsigned char room);
extern void far pascal AddSound(int effect);
extern void far pascal FallOn(void);
extern signed char far pascal GetRow(int y);
extern signed char far pascal get_under_foot(void);
extern int far pascal IsWall(signed char kind);
extern signed char far pascal inside_cell(void);
extern int far pascal is_six_kind(signed char kind);
extern void far pascal break_floor(signed char speed);
extern void far pascal fall_spikes(void);
extern void far pascal fall_blades(void);
extern int far pascal decrement_life_points(signed char amount);
extern void far pascal animation_timer(int delay);
void far pascal HitFloor(void);

void far pascal Falling(void)
{
    signed char tile;

    if (!fall_scream_started && falling_speed >= 31) {
        if (actor_kind == 0) {
            AddKidScream(actor_room);
            fall_scream_started = 1;
        } else if (actor_kind == 2) {
            AddSound(25);
        }
    }
    if ((signed char)63 * actor_row + 56 > actor_y) {
        FallOn();
        if ((actor_kind == 7 || actor_kind == 8) && falling_speed != 0) {
            actor_row = GetRow(actor_y);
            return;
        }
    } else {
        tile = get_under_foot();
        if (IsWall(tile))
            tile = inside_cell();
        if (!is_six_kind(tile)) {
            if (tile != 11 && tile != 15 && tile != 26) {
                HitFloor();
                return;
            }
            if (tile == 11)
                break_floor(falling_speed);
            else if (tile == 15)
                fall_spikes();
            else if (tile == 26)
                fall_blades();
            if ((falling_speed >= 33 || actor_character != 10) && actor_kind != 1)
                decrement_life_points(100);
            falling_speed /= (signed char)2;
            actor_y = fall_floor_y[actor_row] - 7;
            if (fall_transition == 1)
                fall_transition = -1;
            animation_timer(10001);
            fall_scream_started = 0;
            return;
        }
        actor_row++;
    }
}

extern signed char fall_latch;
extern signed char floor_contact;
extern signed char state_mode;
extern int landing_sound_pending;
extern void far pascal reset_char_y(void);
extern int far pascal get_edge_distance(void);
extern signed char far pascal get_cells_ahead(signed char direction);
extern int far pascal IsImpassable(signed char kind);
extern void far pascal land_on_hazard(void);
extern void far pascal play_guard_landing(void);
extern void far pascal set_actor_action(signed char action);
void far pascal PutDeadBodyOnSolidGround(void);
extern void far pascal align_to_floor(void);

/* The original calls to and from this draft are far returns through the ACT frame. */
void far pascal HitFloor(void)
{
    signed char tile;
    int sequence, hazard, fatal, hurt, sound, landed;
    int edge;

    hazard = 0;
    hurt = 0;
    fatal = 0;
    landed = 0;
    fall_scream_started = 0;
    sound = -1;

    reset_char_y();
    tile = get_under_foot();
    if (tile == 23 || tile == 24) {
        landed = 1;
    } else {
        edge = get_edge_distance();
        tile = get_cells_ahead(1);
        if (actor_kind != 7 && actor_kind != 8 &&
            (is_six_kind(tile) || IsImpassable(tile)) && edge < 11)
            actor_x = AddCharX(edge - 13);
        if (fall_latch >= 0)
            fatal = 1;
    }

    tile = get_under_foot();
    if (landed || tile == 23 || tile == 24) {
        land_on_hazard();
        hazard = 1;
    }
    if (!hazard) {
        if (falling_speed < 22) {
            if ((actor_kind >= 2 || floor_contact == 1) && actor_kind != 6) {
                sequence = repeat_sequence_id == 186 ? 187 : 63;
                floor_contact = 1;
            } else {
                sequence = 17;
            }
            sound = actor_kind == 4 ? 76 : 17;
        } else if (falling_speed >= 33 && actor_kind != 1) {
            fatal = 1;
        } else if (actor_character != 10) {
            fatal = 1;
        } else if ((actor_kind != 1 && decrement_life_points(1)) ||
                   (actor_room == 11 && state_mode == 5 && actor_row >= 3)) {
            hurt = 1;
            decrement_life_points(100);
        } else {
            sound = 16;
            sequence = 20;
        }

        if (fatal) {
            decrement_life_points(100);
            hurt = 1;
        }
        if (hurt) {
            if (actor_kind == 4) {
                play_guard_landing();
                sequence = 112;
            } else {
                sound = 0;
                sequence = 22;
            }
            set_actor_action(3);
            PutDeadBodyOnSolidGround();
        }
        if (actor_kind == 7 || actor_kind == 8) {
            actor_x = AddCharX(6);
            sequence = 155;
        }
        JumpSeq(sequence);
        align_to_floor();
        falling_speed = 0;
        if (sound != -1) {
            AddSound(sound);
            if (actor_kind == 0)
                landing_sound_pending = 1;
        }
    }
}

extern signed char far pascal get_behind(void);
extern int far pascal IsMobFloor(signed char kind);

/* Same-file helper at 25da:03d4. */
void far pascal PutDeadBodyOnSolidGround(void)
{
    signed char tile;
    int edge, correction;

    edge = get_edge_distance();
    if (edge < 20)
        tile = get_cells_ahead(1);
    else if (edge > 20)
        tile = get_behind();
    else
        tile = 1;

    if (is_six_kind(tile))
        correction = 1;
    else
        correction = IsMobFloor(tile) ? 2 : 0;
    if (correction) {
        actor_x = AddCharX(edge - 20);
        if (correction == 2 && actor_row >= 3) {
            if (edge - 20 < 0)
                edge = -1;
            else
                edge = 1;
            actor_x = AddCharX(18 * edge);
            if (tile == 26)
                actor_x = AddCharX(9 * edge);
        }
    }
}

extern signed char state_code;
extern unsigned char running_latch;
extern signed char previous_opponent;
extern int far pascal is_dead_pos(unsigned int frame);
extern signed char far pascal choose_opponent(signed char mode);
extern void far pascal finish_shadow_control(void);
extern void far pascal play_guard_room_event(void);
extern void far pascal apply_guard_control(void);
extern void far pascal handle_frame_295(void);
extern void far pascal handle_frames_272_to_281(void);
extern void far pascal continue_animation(void);

extern void far pascal control_fighting(void);
extern void far pascal control_turning(void);
extern void far pascal control_starting(void);
extern void far pascal begin_standing_jump(void);
extern void far pascal control_running(void);
extern void far pascal control_hanging(void);
extern void far pascal control_drop(void);
extern void far pascal control_crouching(void);
extern void far pascal control_crawling(void);
extern void far pascal control_stair_climbing(void);
extern void far pascal control_dead(void);
extern void far pascal control_standing(void);

/* Dispatch draft. The later same-file targets still need their definitions. */
void far pascal GenCtrl(void)
{
    unsigned int frame = actor_frame;
    int state;

    if (fall_latch < 0) {
        state = state_code;
        if (state == 5 || state == 4 || state == 9 ||
            repeat_sequence_id == 27 || repeat_sequence_id == 110)
            goto animate;
        if (floor_contact == 1) {
            control_fighting();
        } else if (actor_kind < 2) {
            apply_guard_control();
        } else if (frame == 15 || (frame >= 50 && frame <= 52)) {
            goto standing;
        } else if (frame >= 45 && frame <= 49) {
            control_turning();
        } else if (frame >= 1 && frame < 4) {
            control_starting();
        } else if (frame >= 67 && frame < 70) {
            begin_standing_jump();
        } else if (frame < 15) {
            control_running();
        } else if (frame >= 87 && frame <= 99) {
            if (state_mode != 1)
                control_hanging();
        } else if (frame == 81 && repeat_sequence_id == 68) {
            control_drop();
        } else if (frame == 109) {
            control_crouching();
        } else if ((frame >= 246 && frame <= 261) ||
                   (frame >= 256 && frame <= 263)) {
            control_crawling();
        } else if (frame >= 217 && frame <= 226) {
            control_stair_climbing();
        } else if (is_dead_pos(frame) || (frame >= 179 && frame <= 183)) {
            goto dead;
        } else if (actor_frame == 295) {
            handle_frame_295();
        } else if ((unsigned int)actor_frame >= 272 && (unsigned int)actor_frame <= 281) {
            handle_frames_272_to_281();
        } else if (actor_frame == 44) {
            running_latch = 0;
            return;
        }
    } else {
        if (actor_kind != 0 && actor_character == previous_opponent)
            previous_opponent = choose_opponent(1);
        if (actor_kind == 1) {
            if (fall_latch > 6) {
                finish_shadow_control();
                return;
            }
        } else if (actor_room == 3 && state_mode == 6 && actor_kind == 2) {
            play_guard_room_event();
        }
    }
    return;

dead:
    control_dead();
    return;
standing:
    control_standing();
    return;
animate:
    continue_animation();
}
