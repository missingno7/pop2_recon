/* Drawing: images in memory, ports and rectangles. */
typedef struct {                        /* an image in memory, or the screen */
    void far *pixels;
    Rect bounds;
    int row_bytes;
    int *row_offsets;
} BITMAP;

typedef struct {                        /* a part of a port kept by SaveRect */
    BITMAP map;
    Rect rect;
} SAVEDRECT;

typedef struct {                        /* where drawing goes */
    BITMAP map;
    Rect visible, clip;                 /* 10, 18 */
    int background;                     /* 20 */
    int pen_y, pen_x, pen_mode;         /* 22 */
    int foreground;                     /* 28 */
    Handle font;                           /* 2a */
} PORT;

extern PORT *Graf;                      /* the port drawn into */
extern PORT default_port;
extern sbyte pixels_per_byte;
extern int graphics_mode;
extern volatile int dither_seed;

void far *allocate_buffer(unsigned long bytes);
void clamp_point_to_rect(POINT far *out, const void far *point, const Rect far *rect);
void ClipRect(const Rect far *rect);    /* narrows the clip and leaves the old one on the stack for ClipRestore */
void ClipRestore(void);
void copy_current_port_origin(void far *pen);
void copy_dithered_port_region(void *to, void *from, const Rect far *to_rect, const Rect far *from_rect, int level);
void CopyBits(void *to, const void *from, const Rect far *to_rect, const Rect far *from_rect, int blit);
void draw_axis_segment(int dx, int dy);
void draw_relative_axis_segment(int dx, int dy);
int DrawString(const char far *text);
int EmptyRect(const Rect far *rect);
void EraseRect(const void far *rect);
void FillRect(const void far *rect, int color);
void Finish_Port(PORT *port);
void flip_rows(void far *pixels, word rows, word row_bytes);
void FrameRect(const Rect far *rect);
void initialize_graphics_mode(int mode);
PORT *InitPort(void *port, const void far *rect);
Rect far *InsetRect(Rect far *out, const Rect far *rect, int dx, int dy);
void MakeShapeRect(Rect far *out, int top, int left, const void far *shape);
int measure_character(byte character);
int measure_counted_text(const char far *text, word count);
int measure_nul_terminated_text(const char far *text);
void Mirror4BitBlock(byte far *pixels, int height, int width, int row_bytes);
void MirrorMCGABlock(byte far *pixels, int height, int row_bytes);
void MirrorRLEBlock(byte far *pixels, int height, int row_bytes);
void Move(int dx, int dy);
void MoveTo(int x, int y);
Rect far *OffsetRect(Rect far *out, const Rect far *rect, int dx, int dy);
void Paint4BitShape(const void far *shape, int x, int y, int blit, int page);
void paint_indexed_shape(const void far *shape, int x, int y, int blit, word flags);
void PaintRLEShape_asm(const void far *shape, int x, int y, int blit);
void PaintShape(const void far *shape, int x, int y, int blit);
void ReadDACBlock(int first, int count, void far *colors);
void release_buffer(void far *buffer);
void RestoreRect(SAVEDRECT *saved);
SAVEDRECT *SaveRect(const Rect far *rect);
int SectRect(Rect far *out, const Rect far *a, const Rect far *b);
int select_graphics_mode(int mode);
void set_relative_pen_state(void far *pen);
void SetDACBlock(int first, int count, const void far *colors, int sync);
void SetOrigin(int x, int y);
void shutdown_graphics_mode(void);
Rect far *StringRect(const Rect far *rect, int across, int down, const char far *text);
Rect far *UnionRect(Rect far *out, const Rect far *a, const Rect far *b);
void update_message_display(void);

PORT *BackPort(PORT *port, const Rect far *rect);
Handle create_string_handle(const char far *text);
int draw_text_handle(Handle handle);
Rect far *draw_text_handle_in_rect(Rect far *rect, int vertical_alignment, int horizontal_alignment, Handle handle);
void FreePort(PORT *port);
Handle hSetFont(Handle font);
int measure_text_handle(Handle handle);
