/* Windows, their items and the queue of events. */
typedef struct WINDOW {
    PORT port;
    byte unknown_2c[26];
    int needs_update;                   /* 46 */
    byte unknown_48[14];
    void (*action)(void *, int);   /* 56 */
} WINDOW;

typedef struct WINDOWITEM {
    int unknown_00[2];
    void (*action)(struct WINDOWITEM *, int, const void *);
    int unknown_08[2];
    Rect bounds;                        /* 0c */
    byte unknown_14[18];
    const void *argument;               /* 26 */
} WINDOWITEM;

typedef struct {
    word unknown_00[5];
    word key;
    word unknown_0c;
} EVENT;

extern WINDOW *window_head;
extern int draw_record_range;

PORT *create_window(int resource, const Rect far *rect, int style, const char far *title);
PORT *create_window_from_resource(int resource);
void dispose_window(PORT *window);
void initialize_window_draw_port(void);
WINDOWITEM *item_by_index(void *window, int index);
int read_queued_event(EVENT *event, int mask);
void set_window_event_state(PORT *window, int state);
int wait_for_window_result(void (*idle)(void));
