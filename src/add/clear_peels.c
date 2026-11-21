typedef struct {
    unsigned int handle;
    int x;
    int y;
} PeelEntry;
extern PeelEntry peel_entries[];
extern int peel_count;
extern void free_peel(unsigned int handle);

void clear_peels(void)
{
    int count = peel_count;
    PeelEntry *entry;
    if (count > 0) {
        entry = peel_entries + count;
        do {
            free_peel((--entry)->handle);
        } while (--count > 0);
    }
    peel_count = 0;
}
