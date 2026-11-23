typedef struct Port {
    void far *buffer;
    char reserved[10];
    void *row_offsets;
} Port;

extern Port CurrentPort;
extern void far pascal RELEASE_BUFFER(void far *buffer);
extern void far cdecl _nfree(void *memory);

void release_port(Port *port)
{
    if (port->buffer != CurrentPort.buffer)
        RELEASE_BUFFER(port->buffer);
    if (port->row_offsets != CurrentPort.row_offsets)
        _nfree(port->row_offsets);
}
