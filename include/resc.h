/* Resources: the .DAT files. */
typedef struct {
    char letters[4];
} RSRCTYPE;                             /* such as 'SHAP' */

typedef struct {
    int id;
    RSRCTYPE type;
} RSRCINFO;

extern Handle shape_metadata_list;
extern int (*catalog_callback)(Handle map);

void CloseMap(Handle map);
Handle hGetRsrc(unsigned long type, int id);
const byte far *load_resource_pinned(unsigned long type, int id);
RSRCINFO far *lookup_shape_metadata(RSRCINFO far *info, Handle handle);
void *nGetRsrc(unsigned long type, int id);
Handle OpenMap(const char far *name);
void ReadRsrc(unsigned long type, int id, void far *to, long bytes);
void release_tagged_resource(unsigned long type, word id);
void select_resource_file(const char *name);
void SetFont(int id);
const char far *skip_nul_strings(const char far *strings, word count);

void hFreeShpList(Handle list);
Handle hGetFont(int id);
Handle hGetShpList(int id);
Handle hMapShpList(Handle list);
Handle unlink_shape_list(Handle list);
