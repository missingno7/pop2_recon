/* Movable memory. A Handle points to a pair of words: flags and the paragraph of the block. */
typedef struct HEAPBLOCK {              /* in front of every block of far memory */
    word size;                          /* top bits 0000 free, 8000 fixed, c000 movable, 4000 the end */
    struct HEAPBLOCK near *resource;
    Handle handle;
} HEAPBLOCK;

typedef struct {
    HEAPBLOCK far *first;
    byte unknown_04[14];
    int (*release_discardable)(void);   /* when memory is short */
} HEAPSTATE;

extern HEAPSTATE far *heap_state;
extern int (*allocation_handler)(void); /* when there is not enough memory */

void far *allocate_pinned_payload(long bytes);
int attach_handle_payload(Handle handle, void far *payload);
word banked_pointer_capacity(const HEAPBLOCK far *block);
int check_heap_chain(void);
void clear_payload_header(const void far *payload);
void far *detach_handle_payload(Handle handle);
void far *h2p(Handle handle);
Handle hFree(Handle handle);
unsigned long hGetSize(Handle handle);
word hGetState(Handle handle);
void far *hLock(Handle handle);
Handle hNew(long bytes);
Handle hNoPurge(Handle handle);
Handle hPurge(Handle handle);
int hSetSize(Handle handle, unsigned long bytes);
Handle hSetState(Handle handle, word state);
Handle hUnlock(Handle handle);
unsigned long HeapCompact(unsigned long bytes);
void HeapInit(unsigned long bytes);
unsigned long HeapPurge(unsigned long bytes);
Handle mark_handle_converted(Handle handle);
void far *materialize_handle(Handle handle);
char far *move_huge_bytes(void far *to, const void far *from, unsigned long bytes);
HEAPBLOCK far *normalize_banked_pointer(HEAPBLOCK far *block);  /* the next one */
Handle p2h(const void far *payload);
unsigned long pGetSize(const void far *payload);
void release_heap_segment(void);
void relocate_handle_payload(Handle handle);
unsigned long resize_dos_heap(unsigned long bytes);

unsigned long ems_driver(void);
void initialize_backing_driver(unsigned long (*driver)(void));
unsigned long query_callback_u32(unsigned long (*driver)(void));
void shutdown_backing_driver(void);
unsigned long xms_driver(void);
