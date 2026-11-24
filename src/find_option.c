extern volatile int argument_count;
extern char **arguments;

extern unsigned far cdecl _fstrlen(const char far *text);
extern int far cdecl _fstrnicmp(const char far *left, const char far *right, unsigned count);

char *far pascal find_option(const char far *name)
{
    int index = 1;
    if (index < argument_count) {
        do {
            if (_fstrnicmp(arguments[index], name, _fstrlen(name)) == 0)
                return arguments[index];
            ++index;
        } while (index < argument_count);
    }
    return 0;
}

typedef struct SoundSettings SoundSettings;
extern SoundSettings *configuration;
extern unsigned far cdecl _dos_open(const char *path, unsigned mode, int *handle);
extern long far cdecl filelength(int handle);
extern void *far cdecl _ncalloc(unsigned count, unsigned size);
extern unsigned far cdecl dos_read(int handle, void far *buffer, unsigned size,
                                   unsigned *bytes_read);
extern unsigned far cdecl _dos_close(int handle);
extern void far cdecl _nfree(void *allocation);

SoundSettings *far pascal load_sound_settings(const char *path)
{
    SoundSettings *result = 0;
    int handle;
    int failed;
    int read_size;
    int buffer_size;
    unsigned bytes_read;

    if (_dos_open(path, 0, &handle) == 0) {
        read_size = (int)filelength(handle);
        buffer_size = read_size;
        if (buffer_size < 32)
            buffer_size = 32;
        result = _ncalloc(1, buffer_size);
        if (result) {
            if (dos_read(handle, result, read_size, &bytes_read) == 0 &&
                bytes_read == (unsigned)read_size)
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
