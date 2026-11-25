/* A line of text that can be edited. */
typedef struct EDITFIELD {
    PORT *port;                         /* 00 */
    Handle font;
    Rect bounds, clip;                  /* 04, 0c */
    int style;                          /* 14 */
    word selection_start, selection_end;
    word caret;                         /* 1a */
    int y, x;
    int visible;                        /* 20 */
    unsigned long last_blink;
    char far *text;                     /* 26 */
    word capacity, length;
    void (*paint)(struct EDITFIELD far *);  /* 2e */
} EDITFIELD;

void blink_edit_caret(EDITFIELD far *field);
void clear_flags_of_far_block(void far *block);
void click_edit_field(EDITFIELD far *field, void *point);
EDITFIELD far * create_edit_field(Rect far *bounds, Rect far *clip, char far *text, word capacity);
void delete_edit_character(EDITFIELD far *field, word index);
void frame_edit_field(EDITFIELD far *field, word mode);
int handle_edit_key(EDITFIELD far *field, int key);
void hide_edit_caret(EDITFIELD far *field);
void paint_edit_caret(EDITFIELD far *field, int visible);
void redraw_edit_text(EDITFIELD far *field);
void replace_edit_text(EDITFIELD far *field, char far *text);
void show_edit_caret(EDITFIELD far *field);
void update_edit_caret(EDITFIELD far *field);
