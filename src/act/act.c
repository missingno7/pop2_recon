extern int Pause;
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
    int field_34;                      /* 34 */
    int previous_mode;                 /* 36 */
    unsigned char behavior;            /* 38 */
    signed char opponent_index;        /* 39 */
    unsigned char field_3a;            /* 3a */
    unsigned char unknown_3b[5];        /* 3b..3f */
} CHARREC;
#pragma pack()
extern CHARREC Char;


extern int far pascal HGETRSRC(unsigned long type, int id);
extern int far pascal AddCharX(int column);
extern void far pascal reread_cells(void);

void far pascal reset_actor_alert(void)
{
    Pause = 0;
}

void far pascal JumpSeq(int sequence)
{
    if (HGETRSRC(0x53455153UL, sequence)) {
        Char.sequence_id = sequence;
        (*(int near *)&Char.sequence) = sequence;
        Char.sequence_position = 0;
        if ((Char.character_kind == 7 || Char.character_kind == 8) &&
            ((sequence >= 130 && sequence <= 139) || sequence == 166) &&
            Char.direction == 0) {
            Char.direction = -1;
            if ((*(int near *)&Char.frame) == 158 || (*(int near *)&Char.frame) == 150)
                Char.x = AddCharX(14);
            reread_cells();
        }
    }
}

extern int fall_scream_started;


extern int fall_floor_y[];
extern int fall_transition;

extern void far _fastcall AddKidScream(unsigned char room);
extern void far pascal AddSound(int effect);
extern void far pascal FallOn(void);
extern signed char far pascal GetRow(int y);
extern signed char far pascal get_under_foot(void);
extern int far pascal IsWall(signed char kind);
extern signed char far pascal inside_cell(void);
extern int far pascal is_six_kind(signed char kind);
extern void far pascal break_loose_floor(signed char speed);
extern void far pascal fall_spikes(void);
extern void far pascal shake_trapdoor(void);
extern int far pascal decrement_life_points(signed char amount);
extern void far pascal dispatch_sound(int delay);
void far pascal HitFloor(void);

void far pascal Falling(void)
{
    signed char tile;

    if (!fall_scream_started && Char.vertical_step >= 31) {
        if (Char.character_kind == 0) {
            AddKidScream(Char.room);
            fall_scream_started = 1;
        } else if (Char.character_kind == 2) {
            AddSound(25);
        }
    }
    if ((signed char)63 * Char.row + 56 > Char.y) {
        FallOn();
        if ((Char.character_kind == 7 || Char.character_kind == 8) && Char.vertical_step != 0) {
            Char.row = GetRow(Char.y);
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
                break_loose_floor(Char.vertical_step);
            else if (tile == 15)
                fall_spikes();
            else if (tile == 26)
                shake_trapdoor();
            if ((Char.vertical_step >= 33 || Char.slot != 10) && Char.character_kind != 1)
                decrement_life_points(100);
            Char.vertical_step /= (signed char)2;
            Char.y = fall_floor_y[Char.row] - 7;
            if (fall_transition == 1)
                fall_transition = -1;
            dispatch_sound(10001);
            fall_scream_started = 0;
            return;
        }
        Char.row++;
    }
}


extern signed char state_mode;
extern int landing_sound_pending;
extern void far pascal reset_char_y(void);
extern int far pascal get_edge_distance(void);
extern signed char far pascal get_cells_ahead(signed char direction);
extern int far pascal IsImpassable(signed char kind);
extern void far pascal do_lava_boil(void);
extern void far pascal bones_fall(void);
extern void far pascal set_actor_action(signed char action);
void far pascal PutDeadBodyOnSolidGround(void);
extern void far pascal align_to_floor(void);

/* The original calls to and from this draft are far returns through the ACT frame. */
void far pascal HitFloor(void)
{
    signed char tile;
    int sequence, hazard, fatal, hurt, sound, landed;
    int edge;

    hazard = 0; hurt = 0;
    fatal = 0; landed = 0;
    fall_scream_started = 0;
    sound = -1;

    reset_char_y();
    tile = get_under_foot();
    if (tile == 23 || tile == 24) {
        landed = 1;
    } else {
        edge = get_edge_distance(); tile = get_cells_ahead(1);
        if (Char.character_kind != 7 && Char.character_kind != 8 &&
            (is_six_kind(tile) || IsImpassable(tile)) && edge < 11)
            Char.x = AddCharX(edge - 13);
        if (Char.alive >= 0)
            fatal = 1;
    }

    tile = get_under_foot();
    if (landed || tile == 23 || tile == 24) {
        do_lava_boil(); hazard = 1;
    }
    if (!hazard) {
        if (Char.vertical_step < 22) {
            if ((Char.character_kind >= 2 || Char.stance == 1) && Char.character_kind != 6) {
                sequence = (*(int near *)&Char.sequence) == 186 ? 187 : 63; Char.stance = 1;
            } else {
                sequence = 17;
            }
            sound = Char.character_kind == 4 ? 76 : 17;
        } else if (Char.vertical_step >= 33 && Char.character_kind != 1) {
            fatal = 1;
        } else if (Char.slot != 10) {
            fatal = 1;
        } else if ((Char.character_kind != 1 && decrement_life_points(1)) ||
                   (Char.room == 11 && state_mode == 5 && Char.row >= 3)) {
            hurt = 1; decrement_life_points(100);
        } else {
            sound = 16; sequence = 20;
        }

        if (fatal) {
            decrement_life_points(100); hurt = 1;
        }
        if (hurt) {
            if (Char.character_kind == 4) {
                bones_fall(); sequence = 112;
            } else {
                sound = 0; sequence = 22;
            }
            set_actor_action(3);
            PutDeadBodyOnSolidGround();
        }
        if (Char.character_kind == 7 || Char.character_kind == 8) {
            Char.x = AddCharX(6);
            sequence = 155;
        }
        JumpSeq(sequence); align_to_floor(); Char.vertical_step = 0;
        if (sound != -1) {
            AddSound(sound);
            if (Char.character_kind == 0)
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
        Char.x = AddCharX(edge - 20);
        if (correction == 2 && Char.row >= 3) {
            if (edge - 20 < 0)
                edge = -1;
            else
                edge = 1;
            Char.x = AddCharX(18 * edge);
            if (tile == 26)
                Char.x = AddCharX(9 * edge);
        }
    }
}


extern unsigned char running_latch;
extern CHARREC Kid;
extern int far pascal is_dead_pos(unsigned int frame);
extern signed char far pascal choose_opponent(signed char mode);
extern void far pascal finish_shadow_control(void);
extern void far pascal play_guard_room_event(void);
extern void far pascal apply_guard_control(void);
extern void far pascal handle_frame_295(void);
extern void far pascal handle_frames_272_to_281(void);
extern unsigned char far pascal ClrAll(void);

extern void far pascal control_fighting(void);
extern void far pascal control_turning(void);
extern void far pascal control_starting(void);
extern void far pascal begin_standing_jump(void);
extern void far pascal control_running(void);
extern void far pascal control_hanging(void);
extern void far pascal control_drop(void);
extern void far pascal control_crouching(void);
extern void far pascal control_crawling(void);
extern void far pascal StairClimbing(void);
extern void far pascal control_dead(void);
extern void far pascal control_standing(void);

/* 25da:047e */
void far pascal GenCtrl(void)
{
    unsigned int frame = (*(int near *)&Char.frame);
    int state;

    if (Char.alive < 0) {
        state = Char.action;
        if (state == 5 || state == 4 || state == 9 ||
            (*(int near *)&Char.sequence) == 27 || (*(int near *)&Char.sequence) == 110)
            goto animate;
        if (Char.stance == 1) {
            control_fighting();
        } else if (Char.character_kind >= 2) {
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
        } else if (frame == 81 && (*(int near *)&Char.sequence) == 68) {
            control_drop();
        } else if (frame == 109) {
            control_crouching();
        } else if ((frame >= 246 && frame <= 261) ||
                   (frame >= 256 && frame <= 263)) {
            control_crawling();
        } else if (frame >= 217 && frame <= 226) {
            StairClimbing();
        } else if (is_dead_pos(frame) || (frame >= 179 && frame <= 183)) {
            goto dead;
        } else if ((*(int near *)&Char.frame) == 295) {
            handle_frame_295();
        } else if ((unsigned int)(*(int near *)&Char.frame) >= 272 && (unsigned int)(*(int near *)&Char.frame) <= 281) {
            handle_frames_272_to_281();
        } else if ((*(int near *)&Char.frame) == 44) {
            running_latch = 0;
            return;
        }
    } else {
        if (Char.character_kind != 0 && Char.slot == Kid.opponent_index)
            Kid.opponent_index = choose_opponent(1);
        if (Char.character_kind == 1) {
            if (Char.alive > 6) {
                finish_shadow_control();
                return;
            }
        } else if (Char.room == 3 && state_mode == 6 && Char.character_kind == 2) {
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
    ClrAll();
}


extern signed char direction_forward, direction_back, direction_up, direction_down;
extern signed char direction_action;

extern signed char tile_modifier;
extern unsigned char level_number;
extern int gate_override;
extern int ColEdgeX[20];
extern unsigned char far pascal ClrAll(void);
extern signed char far pascal ReadCell(signed char room, signed char column, signed char row);
extern signed char far pascal GetCellCol(void);
extern void far pascal ruin_check_crevices(void);
extern void far pascal trigger_crouch_event(void);
extern int far pascal crouch_obstructed(void);
extern int far pascal ScytheBarrToStandUp(void);
extern int far pascal try_pick_up(void);

/* 25da:0650 */
void far pascal control_crawling(void)
{
    signed char tile;
    int gap;

    if ((unsigned int)(*(int near *)&Char.frame) >= 246 && (unsigned int)(*(int near *)&Char.frame) <= 261) {
        if (state_mode == 4)
            ruin_check_crevices();
        tile = ReadCell(Char.room, Char.column, Char.row);
        if (tile != 7 && tile != 12 && tile != 13 &&
            get_cells_ahead(1) != 7 && get_cells_ahead(1) != 12 && get_cells_ahead(1) != 13 &&
            get_behind() != 7 && get_behind() != 12 && get_behind() != 13 &&
            (*(int near *)&Char.sequence) != 124) {
            JumpSeq(124);
            return;
        }
        tile = ReadCell(Char.room, Char.column, Char.row);
        if ((*(int near *)&Char.frame) == 251) {
            if (state_mode == 4)
                ruin_check_crevices();
            if (direction_up && tile != 7) {
                tile = get_behind();
                if (tile == 7) {
                    if (Char.direction == 0)
                        gap = Char.x - ColEdgeX[Char.column + 5] - 38;
                    else
                        gap = ColEdgeX[Char.column + 6] - Char.x - 10;
                } else if (tile == 12 || tile == 13) {
                    gap = ScytheBarrToStandUp() ? -32 : 32;
                } else {
                    gap = 32;
                }
                if (gap >= 0) {
                    JumpSeq(124);
                    direction_up = ClrAll();
                    if (gap < 6) {
                        Char.x = AddCharX(-(gap -= 6));
                    }
                }
            } else {
                if (direction_forward < 0 && Char.field_0f) {
                    ClrAll();
                    JumpSeq(122);
                } else if (direction_back < 0) {
                    ClrAll();
                    JumpSeq(123);
                }
            }
            if (direction_forward == 0 && Char.field_0f == 0) {
                Char.field_0f = 1;
                return;
            }
        } else {
            ClrAll();
        }
    }
}

/* 25da:0802 */
void far pascal control_crouching(void)
{
    signed char tile;
    int edge, gap, delta;
    int sequence;

    if (state_mode == 4)
        ruin_check_crevices();
    if (level_number == 5 && Char.room == 3 &&
        (*(int near *)&Char.sequence) != 17 && (*(int near *)&Char.sequence) != 20 &&
        (get_under_foot() == 18 || ReadCell(Char.room, Char.column - 1, Char.row) == 18)) {
        trigger_crouch_event();
        return;
    }
    if ((*(int near *)&Char.sequence) == 111 || try_pick_up()) {
        direction_action = ClrAll();
        goto done;
    }

    sequence = -1;
    if (direction_down == 0) {
        if (!crouch_obstructed() && (!gate_override || !ScytheBarrToStandUp())) {
            sequence = 49;
            goto apply;
        }
        if (direction_forward < 0)
            goto step;
    } else if (direction_forward < 0) {
        direction_forward = ClrAll();
        tile = get_cells_ahead(1);
        if (tile != 7 && tile != 12 && tile != 13)
            goto step;
        if (tile == 7 && (tile_modifier & 3) == 3)
            goto step;
        tile = GetCellCol();
        if (Char.direction == 0 && -((edge = ColEdgeX[tile + 5]) - Char.x) > 6) {
            gap = edge - Char.x;
            gap += 6;
        } else if (Char.direction == -1 && (delta = Char.x - ColEdgeX[tile + 6]) < 22) {
            gap = delta - 22;
        } else {
            gap = 0;
        }
        if (gap != 0)
            Char.x = AddCharX(gap);
        sequence = 125;
        goto apply;
step:
        sequence = 79;
    }
apply:
    if (direction_down != 0)
        direction_down = ClrAll();
    if (sequence != -1) {
        JumpSeq(sequence);
        return;
    }
done:
    return;
}


extern int far pascal GetShadowToKidDist(void);
extern void far pascal KidRiseFromShadowman(void);
extern void far pascal play_throne_room_nis(void);
extern void far pascal save_actor_slot(void);
extern void far pascal copy_kid_to_char(void);
extern void far pascal load_actor_slot(signed char slot);

/* 25da:0974 */
void far pascal control_dead(void)
{
    signed char slot;

    if (is_dead_pos((*(int near *)&Char.frame))) {
        if (state_mode == 1 && (*(int near *)&Char.frame) == 185 && Char.life_points) {
            decrement_life_points(Char.life_points);
        } else if (Char.character_kind == 1) {
            if (!Char.field_0f && (GetShadowToKidDist() < 0 ? -GetShadowToKidDist() : GetShadowToKidDist()) < 2) {
                KidRiseFromShadowman();
                return;
            }
        } else if (Char.room == 9 && level_number == 8 && (*(int near *)&Char.sequence) == 237) {
            play_throne_room_nis();
            return;
        }
    } else if (Char.character_kind == 0 && Char.slot != 10 && (*(int near *)&Char.frame) != 179 && (*(int near *)&Char.frame) != 183) {
        slot = Char.slot;
        save_actor_slot();
        copy_kid_to_char();
        if (!decrement_life_points(1))
            Char.life_initial--;
        Kid = Char;
        load_actor_slot(slot);
        decrement_life_points(1);
    }
}

extern CHARREC Opp;
extern signed char area_number, viewed_room, exit_column;
extern signed char ledge_input, actor_health_slot;
extern int gate_override;
extern int far pascal get_opp_dist(void);
extern signed char far pascal pick_hidden_actor(signed char direction);
extern void far pascal CheckTurnSeqs(void);
extern void far pascal engage_weapon(void);
extern void far pascal turn_actor(void);
extern void far pascal actor_up(void);
extern void far pascal actor_down(void);
extern void far pascal start_running(void);
extern void far pascal step_forward(int amount);
extern void far pascal standing_jump(void);
extern void far pascal jump_up(void);
extern int far pascal IsExitOpen(void);
extern void far pascal enter_stairs(void);
extern void far pascal crouch_actor(void);

/* 25da:0a68 */
void far pascal control_standing(void)
{
    int act_on_input;
    int gap;

    if (state_mode == 2 || state_mode == 6)
        CheckTurnSeqs();
    act_on_input = 1;
    if (direction_action == -1 && try_pick_up())
        goto acted;
    if (Char.slot == 10) {
        if ((Char.stance != -1 || (state_mode == 6 && Char.character_kind == 1)) &&
            direction_action == -2 && !direction_forward && !direction_back &&
            !direction_up && !direction_down) {
            direction_action = 2;
            ClrAll();
            engage_weapon();
            Char.opponent_index = pick_hidden_actor(Char.direction);
            if (Char.opponent_index == -1)
                Char.opponent_index = 0;
            goto acted;
        }
        if (Opp.alert_delay >= 2 && Char.character_kind != 1 && Char.sequence_group != 13) {
            gap = get_opp_dist();
            if (gap >= -10 && gap < 205 && gap < 0 && gap >= -14) {
                turn_actor();
                goto acted;
            }
        }
    } else if (direction_down < 0 && direction_forward < 0) {
        engage_weapon();
acted:
        act_on_input = 0;
    }

    if (act_on_input) {
        if (direction_action == -1 || direction_action == 1) {
            act_on_input = 0;
            if (direction_back < 0)
                turn_actor();
            else if (direction_up < 0)
                actor_up();
            else if (direction_down < 0)
                actor_down();
            else if (direction_forward < 0 && !direction_up && !direction_down)
                step_forward(0);
            else if (!direction_forward)
                act_on_input = 1;
        }
        if (act_on_input) {
            if (direction_forward < 0) {
                if (direction_up >= 0)
                    goto run;
jump:
                standing_jump();
            } else if (direction_back < 0) {
                turn_actor();
            } else if (direction_up < 0) {
                if (direction_forward < 0)
                    goto jump;
                actor_up();
            } else if (direction_down < 0) {
                actor_down();
            } else if (direction_forward) {
run:
                start_running();
            }
        }
    }

    if ((*(int near *)&Char.sequence) != 5 && Char.character_kind == 0 && Char.field_34)
        Char.field_34 = 0;
}

/* 25da:0c1e */
void far pascal actor_up(void)
{
    if (viewed_room != area_number && Char.character_kind == 0 &&
        (get_under_foot() == 17 || get_behind() == 17 || get_cells_ahead(1) == 17) &&
        IsExitOpen()) {
        enter_stairs();
        return;
    }
    if (direction_forward) {
        standing_jump();
        return;
    }
    jump_up();
}

extern unsigned int cell_modifier_word;
extern int far pascal check_ledge(signed char behind, signed char under, unsigned int behind_mod, unsigned int under_mod);

/* 25da:0c6c */
void far pascal actor_down(void)
{
    signed char behind, under;
    int crouch, edge;
    unsigned int modifier;

    crouch = 1;
    direction_down = 1;
    if (is_six_kind(get_cells_ahead(1)) && get_edge_distance() < 3) {
        Char.x = AddCharX(10);
        reread_cells();
        crouch = 0;
    } else if (is_six_kind(get_behind()) && get_edge_distance() >= 8) {
        behind = get_behind();
        modifier = cell_modifier_word;
        under = get_under_foot();
        if (check_ledge(behind, under, modifier, cell_modifier_word) &&
            (Char.direction == 0 || get_under_foot() != 4 || (cell_modifier_word & 0xfffc) >= 24)) {
            edge = get_edge_distance();
            Char.x = AddCharX(edge - (Char.direction == 0) * 4 - 19);
            JumpSeq(68);
            crouch = 0;
        }
    }
    if (crouch)
        crouch_actor();
}

/* 25da:0d42 */
void far pascal enter_stairs(void)
{
    Char.x = ColEdgeX[exit_column + 4] + 16;
    Char.direction = -1;
    JumpSeq(70);
}

extern unsigned char exit_room;
extern signed char exit_cell;
extern unsigned char room_tiles[][30];
extern signed char far pascal index_cell(signed char column, signed char row);
extern void far pascal AddLeaveLevelSong(void);
extern void far pascal redraw_exit(void);

/* 25da:0d62 */
void far pascal StairClimbing(void)
{
    signed char cell = index_cell(Char.column, Char.row);

    exit_room = Char.room;
    exit_cell = room_tiles[exit_room][cell] == 17 ? cell : cell + 1;
    if ((*(int near *)&Char.frame) == 224)
        AddLeaveLevelSong();
    redraw_exit();
}

/* 25da:0dba */
void far pascal control_turning(void)
{
    if (Char.slot == 10 && (*(int near *)&Char.sequence) == 5)
        CheckTurnSeqs();
    if ((*(int near *)&Char.frame) == 48 && direction_action >= 0 &&
        direction_forward && !direction_up && !direction_down) {
        JumpSeq(43);
        if (Char.character_kind == 0 && Char.field_34)
            Char.field_34 = 0;
    }
}

extern int far pascal CheckShadowmanBackIntoKid(void);

/* 25da:0e0e */
void far pascal crouch_actor(void)
{
    int sequence = Char.character_kind == 1 ? CheckShadowmanBackIntoKid() : -1;
    if (sequence == -1)
        sequence = 50;
    JumpSeq(sequence);
    direction_down = ClrAll();
}

extern int far pascal can_turn_on_gate(void);
extern void far pascal CheckShadowmanCreate(void);

/* 25da:0e3a */
void far pascal turn_actor(void)
{
    int edge, distance;
    direction_back = ClrAll();
    Char.field_0f = 1;
    edge = get_edge_distance();
    if (get_cells_ahead(1) == 4 && can_turn_on_gate()) {
        if (state_mode == 3)
            distance = 7;
        else if (viewed_room == 9 && level_number == 8)
            distance = -100;
        else
            distance = 4;
        if (distance > edge) {
            edge -= distance;
            Char.x = AddCharX(edge);
        }
    }
    if (state_mode == 2 || state_mode == 6)
        CheckShadowmanCreate();
    if ((*(int near *)&Char.sequence) != 71)
        JumpSeq(5);
}

extern signed char running_collision;
extern int far pascal get_forward_distance(void);

/* 25da:0eba */
void far pascal start_running(void)
{
    int distance = get_forward_distance();
    if (running_collision == 1 && distance < 18) {
        if (direction_forward < 0)
            step_forward(0);
    } else {
        JumpSeq(1);
    }
    direction_forward = ClrAll();
}

extern int far pascal run_jump(int sequence);

/* 25da:0ef0 */
void far pascal control_running(void)
{
    if (!direction_forward && !direction_back && ((*(int near *)&Char.frame) == 7 || (*(int near *)&Char.frame) == 11)) {
        direction_forward = ClrAll();
        JumpSeq(13);
    } else if (direction_back) {
        direction_back = ClrAll();
        JumpSeq(6);
    } else if (direction_up) {
        if (direction_up < 0)
            run_jump(4);
    } else if (direction_down < 0) {
        direction_down = ClrAll();
        JumpSeq(26);
    }
    if (direction_forward < 0)
        direction_forward = 1;
}

struct tile_snapshot {
    unsigned char bytes[11];
};
extern struct tile_snapshot current_tile;
extern int step_in_progress;

/* 25da:0f66 */
void far pascal step_forward(int distance)
{
    int sequence, remainder;
    signed char edge_marker;

    step_in_progress = 1;
    edge_marker = 1; direction_forward = edge_marker; direction_action = edge_marker;
    if (!distance)
        distance = get_forward_distance();
    if (running_collision == 1)
        distance--;
    if (distance > 0) {
        Char.field_0f = 1;
        if (distance < 28) {
            if (distance == 1)
                sequence = -1;
            else
                sequence = distance / 2 + 28;
            if ((remainder = distance % 2) == 0) {
                distance = sequence;
                goto play_sequence;
            }
            Char.x = AddCharX(1);
            distance = sequence;
        } else {
            distance = 42;
        }
    } else if (Char.field_0f &&
               (running_collision != 1 || current_tile.bytes[0] == 12 || current_tile.bytes[0] == 13)) {
        Char.field_0f = 0; distance = 44;
    } else {
        distance = 42;
    }
play_sequence:
    if (distance != -1)
        JumpSeq(distance); step_in_progress = 0;
}

extern int far pascal pick_it_up(void);
extern void far pascal RemoveObj(int portion);
extern void far pascal AddSong(int song);
extern void far pascal load_sword_set(unsigned char type);
extern unsigned char pickup_type;

int far pascal try_pick_up(void)
{
    struct tile_snapshot saved_tile;
    int here = 0;
    int result;
    signed char tile = get_under_foot();

    if (tile == 10 || tile == 22) {
        saved_tile = current_tile;
        here = 1;
        current_tile = saved_tile;
    }
    if (here || get_cells_ahead(1) == 10)
        goto pickup;
    result = here;
    if (get_cells_ahead(1) == 22) {
pickup:
        result = pick_it_up();
    }
    return result;
}

int far pascal pick_it_up(void)
{
    signed char item;
    int picked;
    signed char foot;
    int edge;
    unsigned char type;
    struct tile_snapshot saved_tile;

    item = current_tile.bytes[0];
    saved_tile = current_tile;
    type = (unsigned char)cell_modifier_word & 3;
    picked = 1;
    edge = get_edge_distance();
    foot = get_under_foot();

    if ((*(int near *)&Char.frame) != 109) {
        Char.x = AddCharX(foot == item
            ? (Char.direction == 0 ? edge - 25 : edge - 17)
            : (Char.direction == 0 ? edge + 1 : edge + 15)); crouch_actor();
    } else if ((*(int near *)&Char.sequence) == 50 ||
               ((*(int near *)&Char.sequence) != 20 && direction_action == -1)) {
        if (Char.direction == 0) {
            if (edge < 24 && foot != item)
                Char.x = AddCharX(edge - 4);
        } else {
            Char.x = AddCharX(edge - 22);
        }
        current_tile = saved_tile;
        if (item == 22) {
            RemoveObj(-1);
            if (Char.room == 9 && level_number == 8) {
                JumpSeq(237);
                AddSong(254);
            } else {
                JumpSeq(91);
                AddSong(151);
            }
            load_sword_set(type);
            pickup_type = type;
        } else if (item == 10) {
            RemoveObj((cell_modifier_word & 0xe0) >> 5);
            JumpSeq(78);
        }
    } else {
        picked = 0;
    }
    return picked;
}

/* 25da:11dc */
void far pascal control_starting(void)
{
    if (direction_up < 0 && direction_forward < 0) {
        unsigned char zero = ClrAll();
        direction_forward = zero;
        direction_up = zero;
        standing_jump();
    }
}

/* 25da:11fa */
void far pascal begin_standing_jump(void)
{
    if (direction_forward)
        standing_jump();
}

/* 25da:1206 */
void far pascal standing_jump(void)
{
    signed char held = 1;
    direction_forward = held;
    direction_up = held;
    JumpSeq(3);
}

extern signed char far pascal get_above(void);
extern signed char far pascal get_above_front(void);
extern signed char far pascal get_above_behind(void);
extern int far _fastcall probe_jump_anchor(signed char room, signed char row);
extern int abs(int);
#pragma intrinsic(abs)
extern void far pascal jump_hang(int back);
extern void far pascal jump_back(void);
extern void far pascal jump_edge(void);
extern void far pascal jump_high(void);
extern int far pascal FaceDx(int x);
extern int far pascal get_base_x(void);
extern signed char far pascal get_col(int x);

/* 25da:1218 */
void far pascal jump_up(void)
{
    signed char above, front;
    unsigned int modifier;
    int anchor;

    direction_up = ClrAll();
    above = get_above();
    modifier = cell_modifier_word;
    front = get_above_front();
    if (front == 4)
        cell_modifier_word = 200;
    anchor = probe_jump_anchor(Char.room, Char.row);
    if (anchor && abs(anchor - Char.x) > 32)
        anchor = 0;
    if (!anchor && check_ledge(above, front, modifier, cell_modifier_word)) {
        jump_hang(0);
        return;
    }

    above = get_above_behind();
    modifier = cell_modifier_word;
    front = get_above();
    if (front == 4)
        cell_modifier_word = 200;
    if (!anchor && check_ledge(above, front, modifier, cell_modifier_word)) {
        jump_back();
        return;
    }
    jump_high();
}

/* 25da:12dc */
void far pascal jump_back(void)
{
    int edge = get_edge_distance();

    probe_jump_anchor(Char.room, Char.row);
    if (edge < 16) {
        jump_high();
        return;
    }
    if (is_six_kind(get_behind())) {
        jump_edge();
        return;
    }
    Char.x = AddCharX(edge - 32);
    reread_cells();
    jump_hang(1);
}

/* 25da:132e */
void far pascal jump_edge(void)
{
    get_above();
    Char.x = AddCharX(get_edge_distance() - 24);
    JumpSeq(16);
}

/* 25da:134e */
void far pascal jump_high(void)
{
    signed char column;
    int edge;

    direction_up = ClrAll();
    edge = get_forward_distance();
    if (edge < 4 && running_collision == 1)
        Char.x = AddCharX(edge - 6);
    column = get_col((unsigned int)FaceDx(0) + get_base_x() - 6);
    if (is_six_kind(ReadCell(Char.room, column, Char.row - 1)) ||
        (state_mode == 5 && Char.room == 15) ||
        (Char.room == 7 && state_mode == 6 &&
         (Char.row != 2 || (Char.column != 2 && Char.column != 6))) ||
        (Char.room == 8 && state_mode == 6 &&
         (Char.row != 2 || Char.column == 3 || Char.column == 4)))
        JumpSeq(28);
    else
        JumpSeq(14);
}

extern int climb_delay;
extern signed char far pascal get_above(void);
extern int far pascal RoofHangingFromShip(void);
extern int far pascal probe_chain_floor(int direction);
extern void far pascal climb_up(void);
extern int far pascal IsCharSwingingFwd(void);
extern int far pascal is_walkable_tile(signed char tile);
extern int far pascal is_open_tile(signed char tile);

/* 25da:141e */
void far pascal control_hanging(void)
{
    signed char tile;
    if (Char.alive < 0) {
        if (!climb_delay && direction_up) {
            climb_up();
            return;
        }
        if (direction_action && Char.sequence_group != 11) {
            tile = get_under_foot();
            if ((Char.action != 6 && IsWall(tile)) ||
                (Char.direction == -1 && tile == 4) ||
                (state_mode == 5 && Char.room == 15)) {
                JumpSeq(25);
                return;
            }
            if (is_six_kind(get_above()) && (state_mode != 5 || !RoofHangingFromShip()))
                goto drop;
            goto done;
        } else {
            Char.sequence_group = 0;
        }
    }
drop:
    control_drop();
done:
    return;
}

/* 25da:14c4 */
void far pascal climb_up(void)
{
    int sequence;
    unsigned char zero;
    signed char above;

    if (level_number == 5 && Char.room == 10 && Char.stance != -1) {
        sequence = 236;
        Char.stance = -1;
        AddSound(19);
    } else {
        sequence = 10;
    }
    zero = ClrAll();
    direction_action = zero;
    direction_up = zero;
    above = get_above();
    if ((Char.character_kind != 1 && above == 4 && Char.direction == -1 &&
         (cell_modifier_word & 0xfffc) < 24) ||
        (state_mode == 2 && probe_chain_floor(0))) {
        sequence = 73;
    } else if (state_mode == 5 && RoofHangingFromShip()) {
        sequence = 59;
    }
    JumpSeq(sequence);
    if (Char.direction == 0 || state_mode != 3)
        Char.x = AddCharX(-2);
}

/* 25da:156a */
void far pascal control_drop(void)
{
    signed char under, behind;
    int adjustment, back_drop, front_drop, sequence;

    direction_down = ClrAll();
    adjustment = 0;
    behind = get_behind();
    under = get_under_foot();
    if (is_six_kind(behind) && is_six_kind(under)) {
        sequence = IsCharSwingingFwd() ? 99 : 23;
    } else {
        front_drop = is_walkable_tile(under);
        back_drop = is_walkable_tile(behind);
        if (!front_drop && !back_drop) {
            if (is_open_tile(behind) && IsWall(under))
                back_drop = 1;
            else if (is_open_tile(under) && IsWall(behind))
                front_drop = 1;
        }
        if (IsWall(under)) {
            adjustment = -14;
            sequence = back_drop ? 11 : 23;
        } else if (!front_drop && back_drop) {
            adjustment = -8;
            sequence = 11;
        } else if (front_drop && !back_drop) {
            adjustment = 6;
            sequence = 11;
        } else {
            sequence = front_drop && back_drop ? 11 : 23;
        }
        if (adjustment)
            Char.x = AddCharX(adjustment);
    }
    JumpSeq(sequence);
}

extern void far pascal advance_character(void);

int far pascal IsCharSwingingFwd(void)
{
    int old_repeat = (*(int near *)&Char.sequence);
    int old_position = Char.sequence_position;
    int old_sequence = Char.sequence_id;
    unsigned int old_frame = (*(int near *)&Char.frame);

    advance_character();
    Char.sequence_id = old_sequence;
    Char.sequence_position = old_position;
    (*(int near *)&Char.sequence) = old_repeat;
    if (old_frame >= (*(int near *)&Char.frame))
        return 1;
    return 0;
}

/* 25da:16d2 */
void far pascal jump_hang(int back)
{
    int edge = get_edge_distance();

    if (back && edge == 32)
        edge = 0;
    if (edge < 4 && get_forward_distance() < 4 && running_collision != 1) {
        Char.x = AddCharX(edge);
        JumpSeq(8);
    } else {
        if (Char.direction == -1)
            edge -= 7;
        else
            edge -= 9;
        Char.x = AddCharX(edge);
        JumpSeq(24);
    }
}

extern signed char stride_columns[];
extern signed char far pascal get_char_col(int x);
extern int far pascal get_edge_distance_at(int x);
extern int far pascal can_continue_run_jump(signed char tile, unsigned int modifier);

/* 25da:1738 */
int far pascal run_jump(int sequence)
{
    signed char tile, column;
    int reach, x, distance, gaps, scanning, found, result;

    if ((sequence == 4 && ((*(int near *)&Char.frame) == 7 || (*(int near *)&Char.frame) == 11)) ||
        (sequence == 100 && ((*(int near *)&Char.frame) == 192 || (*(int near *)&Char.frame) == 196))) {
        reach = sequence == 4 ? 11 : 9;
        x = AddCharX(reach);
        column = get_char_col(x);
        gaps = found = 0;
        scanning = 1;
        while (scanning) {
            tile = ReadCell(Char.room, column += stride_columns[Char.direction], Char.row);
            if (!is_six_kind(tile) && can_continue_run_jump(tile, cell_modifier_word)) {
                if (++gaps == 2)
                    found = 1;
            } else {
                scanning = 0;
            }
            if (found)
                break;
        }
        if (!scanning) {
            distance = get_edge_distance_at(x) + (gaps << 5);
            distance -= sequence == 4 ? 31 : 35;
            if (distance < -19 || distance >= 10) {
                if (distance < 0 && sequence == 4)
                    return 0;
                distance = -reach;
            }
            Char.x = AddCharX(distance + reach);
        }
        JumpSeq(sequence);
        direction_up = ClrAll();
        result = 1;
    } else {
        result = 0;
    }
    return result;
}

/* 25da:1870 */
void far pascal retreat_actor(void)
{
    int frame = (*(int near *)&Char.frame);
    if (Char.character_kind != 7 && Char.character_kind != 8 && Char.character_kind != 11 &&
        (frame == 158 || frame == 170 || frame == 171)) {
        int sequence;
        if (Char.character_kind == 10)
            sequence = 104;
        else
            sequence = 57;
        JumpSeq(sequence);
        direction_back = ClrAll();
    }
}

/* 25da:18ba */
void far pascal advance_actor(void)
{
    int frame = (*(int near *)&Char.frame);
    if (Char.character_kind != 7 && Char.character_kind != 8 && Char.character_kind != 11 &&
        (frame == 158 || frame == 170 || frame == 171)) {
        int sequence;
        if (Char.character_kind == 0 || Char.character_kind == 1)
            sequence = 56;
        else if (Char.character_kind == 10)
            sequence = 108;
        else
            sequence = 86;
        JumpSeq(sequence);
        direction_forward = ClrAll();
    }
}

extern int far pascal choose_weapon_draw(void);
extern int far pascal special_shadow_draw(void);
extern CHARREC RoomOpp[5];

/* 25da:1918 */
void far pascal engage_weapon(void)
{
    int sequence;

    direction_forward = ClrAll();
    direction_action = 2;
    Char.stance = 1;
    if (Char.character_kind == 0 || Char.character_kind == 1)
        sequence = choose_weapon_draw();
    else if (Char.character_kind == 7 || Char.character_kind == 8)
        sequence = -1;
    else if (Char.character_kind != 1)
        sequence = 90;
    if (sequence != -1)
        JumpSeq(sequence);
}

/* 25da:1978 */
int far pascal choose_weapon_draw(void)
{
    signed char tile, slot;
    int sequence, anchor, edge, adjustment;

    if (((unsigned int)(*(int near *)&Char.frame) >= 246 && (unsigned int)(*(int near *)&Char.frame) <= 261) ||
        ((Char.room == 1 || Char.room == 2) && state_mode == 14)) {
        sequence = -1;
    } else if (Char.character_kind == 1 && (Char.room == 7 || Char.room == 8) && state_mode == 6) {
        sequence = special_shadow_draw();
        slot = 0;
        while (RoomOpp[slot].character_kind != 0 && slot < 5)
            slot++;
        Char.stance = RoomOpp[slot].stance;
    } else {
        sequence = 55;
        adjustment = -1;
        anchor = probe_jump_anchor(Char.room, Char.row);
        if (anchor) {
            tile = get_char_col(anchor);
            if (abs(tile - Char.column) > 1)
                anchor = 0;
        }
        tile = Char.direction == 0 ? get_under_foot() : 0;
        if (tile == 4 && can_turn_on_gate()) {
            adjustment = -32;
        } else if (anchor && ((Char.direction == -1 && Char.x >= anchor) ||
                             (Char.direction == 0 && Char.x <= anchor))) {
            edge = get_edge_distance();
            adjustment = abs(anchor - Char.x) - edge;
            adjustment = (adjustment == -1 ? -2 : adjustment) - 4;
        } else {
            tile = get_cells_ahead(1);
            if (!is_open_tile(tile) || (tile == 4 && can_turn_on_gate()))
                adjustment = 0;
        }
        if (adjustment != -1) {
            tile = get_behind();
            if (is_open_tile(tile) && (tile != 4 || !can_turn_on_gate()) &&
                (!anchor || (Char.direction == -1 && Char.x >= anchor) ||
                 (Char.direction == 0 && anchor >= Char.x)) && Char.sequence_group != 4) {
                edge = get_edge_distance() + adjustment;
                if (Char.direction == 0 && edge < 21)
                    Char.x = AddCharX(edge - 21);
                else if (Char.direction == -1 && edge < 15)
                    Char.x = AddCharX(edge - 15);
            } else {
                Char.stance = 0;
                sequence = -1;
            }
        }
    }
    if (sequence != -1 && sequence != 242)
        AddSound(19);
    else if (Char.stance != -1)
        Char.stance = 0;
    return sequence;
}

struct fight_opponent {
    unsigned char payload[23];
};
struct fight_list {
    signed char count;
    struct fight_opponent opponent[5];
};
extern struct fight_list fight_lists[];
extern unsigned int next_opponent_toggle;
extern int room_event_pending;

extern void far pascal load_kid_with_opponent(signed char slot);
extern void far pascal on_alert(void);
extern void far pascal turn_engarde(void);
extern void far pascal sheathe_weapon(void);

/* 25da:1b92 */
void far pascal control_fighting(void)
{
    signed char target;
    int gap, react, eligible, out_of_range, opponent_can_react;

    if (Char.slot == 10) {
        Kid = Char;
        target = -1;
        if (Char.opponent_index != -1 && Char.opponent_index < fight_lists[(unsigned char)Char.room].count) {
            load_kid_with_opponent(Char.opponent_index);
            if (Char.life_points > 0)
                target = Char.opponent_index;
        }
        if (target == -1) {
            target = pick_hidden_actor(1);
            if (target == -1)
                target = 0;
            else
                Char.opponent_index = target;
        }
        load_kid_with_opponent(target);
    }
    if (Char.action < 2) {
        react = 1;
        {
            signed char tile = get_under_foot();
            eligible = tile == 11 || tile == 15 || Opp.alert_delay >= 2;
            if (eligible) {
                gap = get_opp_dist();
                out_of_range = gap < -10 || gap >= 205;
                if (out_of_range)
                    goto distant_opponent;
alert:
                on_alert();
reacted:
                react = 0;
            }
        }
after_reaction:
        if (Char.slot == 10 && direction_back < 0 &&
            (direction_action == -1 || direction_action == 1) &&
            (level_number != 5 || Char.room != 10 || room_event_pending <= 0)) {
            turn_engarde();
            direction_action = 2;
            direction_back = ClrAll();
            next_opponent_toggle = !next_opponent_toggle;
            Char.opponent_index = pick_hidden_actor(~Char.direction);
            return;
        }
        goto check_reaction;

distant_opponent:
        if (gap >= 0)
            goto after_reaction;
        if (gap >= -4)
            goto alert;
        if (gap >= -30) {
            switch (Opp.character_kind) {
            case 7:
            case 8:
                goto alert;
            }
        }
        if (Char.character_kind == 11)
            goto reacted;
        opponent_can_react = (Char.slot == 10 && !next_opponent_toggle) || Opp.character_kind == 6;
        if (opponent_can_react)
            goto alert;
        turn_engarde();
        goto reacted;

check_reaction:
        if (react) {
            if (Char.slot == 10 && direction_down < 0) {
                sheathe_weapon();
                return;
            }
            on_alert();
        }
    }
}
#pragma pack(1)
struct opponent {
    signed char reserved[17];
    signed char attitude;
    signed char pad[5];
};

struct opponent_list {
    signed char count;
    struct opponent opponents[5];
};


extern struct opponent_list opponent_lists[];

extern void far pascal copy_kid_to_char(void);
extern int far pascal is_dead_pos(unsigned int frame);
extern void far pascal load_actor_slot(signed char index);
extern struct opponent near * far pascal GetOppInfoIdx(int room, int index);

int far pascal OppToFight(void)
{
    CHARREC saved_actor;
    signed char index;
    int ready;
    struct opponent near *opponent;

    ready = 0;
    saved_actor = Char;
    copy_kid_to_char();
    if (Char.sequence_group != 13 && Char.character_kind != 1 &&
        Char.action != 3 && Char.action != 4 && !is_dead_pos((*(int near *)&Char.frame))) {
        index = 0;
        do {
            if (index >= opponent_lists[(*(unsigned char near *)&Kid.room)].count)
                break;
            load_actor_slot(index);
            if (Char.alive < 0 && Char.alert_delay > 0 && Char.character_kind != 11) {
                if (Char.character_kind == 7 || Char.character_kind == 8) {
                    opponent = GetOppInfoIdx((unsigned char)Char.room, Char.slot);
                    if (opponent->attitude != 1 && opponent->attitude != 3) {
                        ready = 0;
                        goto next_opponent;
                    }
                }
                ready = 1;
            }
next_opponent:
            index++;
        } while (!ready);
    }
    Char = saved_actor;
    return ready;
}


extern int strike_delay, sheathe_delay;
extern void far pascal unused_head_alert(void);
extern int far pascal is_head_locked_on(signed char room, int slot);
extern void far pascal turn_kid_engarde(void);
extern void far pascal strike_actor(void);
extern void far pascal block_actor(void);

/* 25da:1e08 */
void far pascal on_alert(void)
{
    int acted, frame, kind;

    if (Char.character_kind != 7 && Char.character_kind != 8) {
        acted = 0;
        frame = (*(int near *)&Char.frame);
        kind = Char.character_kind;
        if (frame == 161 && direction_action >= 0) {
            JumpSeq(57);
            acted = 1;
        } else if (direction_action == -2) {
            if (kind == 0 || kind == 1)
                strike_delay = 15;
            strike_actor();
            acted = 1;
        }
        if (!acted) {
            if (direction_down < 0) {
                if (frame == 158 || frame == 170 || frame == 171) {
                    sheathe_weapon();
                    return;
                }
            } else if (direction_up < 0) {
                block_actor();
                return;
            } else if (direction_forward < 0) {
                advance_actor();
                return;
            } else if (direction_back < 0 && !direction_action) {
                retreat_actor();
                return;
            }
        }
    } else {
        unused_head_alert();
    }
}

/* 25da:1ebc */
void far pascal sheathe_weapon(void)
{
    Char.stance = 0;
    direction_down = ClrAll();
    if (Char.character_kind == 0 || Char.character_kind == 1) {
        sheathe_delay = 9;
        if (OppToFight())
            JumpSeq(93);
        else
            JumpSeq(92);
    } else if (Char.character_kind == 10) {
        JumpSeq(90);
    } else {
        JumpSeq(77);
    }
}

/* 25da:1f08 */
void far pascal turn_engarde(void)
{
    if (Char.character_kind == 7 || Char.character_kind == 8) {
        if ((*(int near *)&Char.sequence) != 156 && (*(int near *)&Char.sequence) != 166 &&
            !is_head_locked_on(Char.room, Char.slot))
            JumpSeq(156);
    } else if (Char.character_kind == 0 || Char.character_kind == 1) {
        turn_kid_engarde();
        return;
    } else if (Char.character_kind != 10 || (unsigned int)(*(int near *)&Char.frame) < 213 || (unsigned int)(*(int near *)&Char.frame) > 217) {
        JumpSeq(60);
    }
}

/* 25da:1f72 */
void far pascal turn_kid_engarde(void)
{
    signed char tile;
    int offset, edge;

    offset = -1;
    tile = get_cells_ahead(1);
    if (is_open_tile(tile) && (tile != 4 || !can_turn_on_gate())) {
        tile = get_cells_ahead(2);
        if (!is_open_tile(tile) || (tile == 4 && can_turn_on_gate()))
            offset = 32;
    } else {
        offset = 0;
    }
    if ((*(int near *)&Char.sequence) == 94)
        offset += 18;
    if (offset != -1) {
        edge = get_edge_distance() + offset;
        if (edge < 56)
            Char.x = AddCharX(edge - 56);
    }
    JumpSeq(127);
}

/* 25da:2004 */
void far pascal strike_actor(void)
{
    int frame, sequence;
    if (Char.character_kind != 7 && Char.character_kind != 8) {
        frame = (*(int near *)&Char.frame);
        if (frame == 157 || frame == 158 || frame == 170 || frame == 171 || frame == 165) {
            switch (Char.character_kind) {
            case 1:
            case 0:
                sequence = Opp.character_kind == 11 ? 96 : 75;
                break;
            case 10:
                sequence = 109;
                break;
            default:
                sequence = 58;
                break;
            }
        } else if (frame == 150 || frame == 161) {
            sequence = 66;
        } else {
            sequence = -1;
        }
        if (sequence != -1) {
            JumpSeq(sequence);
            direction_action = 2;
            ClrAll();
        }
    }
}

/* 25da:2090 */
void far pascal block_actor(void)
{
    int threat_frame, kind, sequence, advance, frame;

    if (Char.character_kind != 7 && Char.character_kind != 8) {
        threat_frame = Opp.frame;
        kind = Char.character_kind;
        sequence = 62;
        advance = 0;
        frame = (*(int near *)&Char.frame);
        if (frame == 158 || frame == 170 || frame == 171 || frame == 168 || frame == 165) {
            if (kind == 0 || kind == 1) {
                if (threat_frame == 168)
                    sequence = -1;
                else if (threat_frame == 152 || threat_frame == 162) {
                    /* Keep the default block sequence. */
                } else if (threat_frame == 153) {
                    advance = 1;
                }
            } else if (get_opp_dist() >= 51) {
                retreat_actor();
                goto cannot_block;
            } else if (threat_frame != 152) {
                goto cannot_block;
            }
        } else if (frame == 167) {
            sequence = 61;
        } else {
cannot_block:
            sequence = -1;
        }
        if (sequence != -1) {
            direction_up = 1;
            JumpSeq(sequence);
            if (advance)
                advance_character();
        }
    }
}

/* 25da:2152 */
void far pascal crush_actor(void)
{
    signed char state = Char.action;
    int frame = (*(int near *)&Char.frame);

    if ((frame < 5 || frame >= 15) && frame != 109 &&
        (state < 2 || state == 7) && Char.character_kind != 1) {
        reset_char_y();
        if (decrement_life_points(Char.character_kind == 0 ? 1 : 100)) {
            if (Char.character_kind == 11)
                JumpSeq(169);
            else if (Char.character_kind == 7 || Char.character_kind == 8)
                JumpSeq(146);
            else if (Char.character_kind == 4) {
                bones_fall();
                return;
            } else {
                JumpSeq(22);
            }
        } else {
            if (frame == 109)
                return;
            if (is_six_kind(get_behind()))
                Char.x = AddCharX(-4);
            Char.sequence_group = 8;
            AddSound(10);
            JumpSeq(52);
        }
    }
}

#pragma pack(1)
struct crush_record {
    signed char bytes_00[5];
    int sequence, position;
    signed char byte_09;
    signed char index;
    signed char byte_0b;
    signed char enabled;
    signed char bytes_0d[2];
    signed char kind;
    signed char byte_10;
    int pending;
};
extern unsigned char crush_mode;
extern void far pascal RemoveOppFromRoom(unsigned char room, int slot);

/* 25da:2220 */
void far pascal CrushOppInfo(struct crush_record near *opponent, unsigned char room)
{
    if (crush_mode != 2)
        opponent->enabled = 0;
    if (crush_mode == 5) {
        if (opponent->kind == 8) {
            opponent->sequence = 169;
            opponent->position = 0;
        } else {
            RemoveOppFromRoom(room, opponent->index);
        }
    } else if (crush_mode == 2) {
        if (opponent->pending == 1)
            opponent->pending = 0;
    } else {
        opponent->sequence = 22;
        opponent->position = 0;
    }
}

/* 25da:227c */
void far pascal SquishChar(void)
{
    JumpSeq(118);
    decrement_life_points(100);
}
