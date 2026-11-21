extern int repeat_sequence_id;
extern int sequence_position;
extern int sequence_id;
extern unsigned int actor_frame;
extern void far pascal advance_character(void);

int far pascal IsCharSwingingFwd(void)
{
    int old_repeat = repeat_sequence_id;
    int old_position = sequence_position;
    int old_sequence = sequence_id;
    unsigned int old_frame = actor_frame;

    advance_character();
    sequence_id = old_sequence;
    sequence_position = old_position;
    repeat_sequence_id = old_repeat;
    if (old_frame >= actor_frame)
        return 1;
    return 0;
}
