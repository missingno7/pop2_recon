#pragma pack(1)
struct actor_state {
    signed char slot;
    signed char direction;
    int x;
    int y;
    signed char kind;
    int frame;
    signed char cell_row;
    signed char reserved_0a;
    signed char state_code;
    signed char reserved_0c[2];
    unsigned char room;
    signed char reserved_0f[2];
    signed char fall_latch;
    signed char reserved_12[17];
    signed char field_23;
    int animation_contact;
    signed char reserved_26[26];
};

struct opponent {
    signed char reserved[17];
    signed char attitude;
    signed char pad[5];
};

struct opponent_list {
    signed char count;
    struct opponent opponents[5];
};

extern struct actor_state actor;
extern unsigned char opponent_list_index;
extern struct opponent_list opponent_lists[];

extern void far pascal copy_kid_to_char(void);
extern int far pascal is_dead_pos(unsigned int frame);
extern void far pascal load_actor_slot(signed char index);
extern struct opponent near * far pascal GetOppInfoIdx(int room, int index);

int far pascal OppToFight(void)
{
    struct actor_state saved_actor;
    signed char index;
    int ready;
    struct opponent near *opponent;

    ready = 0;
    saved_actor = actor;
    copy_kid_to_char();
    if (actor.animation_contact != 13 && actor.kind != 1 &&
        actor.state_code != 3 && actor.state_code != 4 && !is_dead_pos(actor.frame)) {
        index = 0;
        do {
            if (index >= opponent_lists[opponent_list_index].count)
                break;
            load_actor_slot(index);
            if (actor.fall_latch < 0 && actor.field_23 > 0 && actor.kind != 11) {
                if (actor.kind == 7 || actor.kind == 8) {
                    opponent = GetOppInfoIdx(actor.room, actor.slot);
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
    actor = saved_actor;
    return ready;
}
