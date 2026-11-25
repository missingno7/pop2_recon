#include <string.h>
#include <malloc.h>
#include <dos.h>
#include <io.h>
#include "types.h"
#include "snd.h"


extern volatile int cdecl __argc;
extern char **cdecl __argv;


char *find_option(const char far *name)
{
    int index = 1;
    for (; index < __argc; ++index) {
        if (_fstrnicmp(__argv[index], name, _fstrlen(name)) == 0)
            return __argv[index];
    }
    return 0;
}


int load_sound_settings(sbyte *path)
{
    CONFIG *result = 0;
    int handle;
    int failed;
    int read_size;
    int buffer_size;
    word bytes_read;

    if (_dos_open(path, 0, &handle) == 0) {
        read_size = filelength(handle);
        buffer_size = read_size;
        if (buffer_size < 32)
            buffer_size = 32;
        result = _ncalloc(1, buffer_size);
        if (result) {
            if (_dos_read(handle, result, read_size, &bytes_read) == 0 &&
                bytes_read == read_size)
                failed = 0;
            else
                failed = 1;
        }
        _dos_close(handle);
    } else {
        failed = 1;
    }

    if (failed) {
        if (result)
            _nfree(result);
        return 0;
    }
    return configuration = result;
}
