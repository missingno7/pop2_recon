/* DIAG */

extern int frame_diagnostics;
extern int debug_option_1;
extern int debug_option_2;
extern int debug_option_3;
extern int show_reference_position;
extern int debug_check_1;
extern int debug_check_2;
extern int show_actor_position;
extern int debug_show_door_links;
extern int debug_show_meters;
extern int show_ruler;
extern int pause_on_frame;
extern POINT DebugKid;
extern int debug_position_1;
extern int debug_position_2;
extern int debug_position_3;
extern int debug_position_4;
extern char volume_format[];

int check_far_heap(void);
void check_heaps(const char *label);
int check_near_heap(void);
void check_null_area(const char *label);
void draw_frame_diagnostics(void);
void draw_ruler_marks(void);
void dump_heap(const char *name, const char far *heading, int summary_only);
void limit_frame_rate(void);
int null_area_matches(void);
void plot_actor_offset(int distance);
void plot_edge_offset(int distance);
void plot_position(int x, int color);
void preview_resource(Handle handle);
void report_ticks(const char *label);
void snapshot_null_area(void);
