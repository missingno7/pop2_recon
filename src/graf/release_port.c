typedef struct Port {
    void far *buffer;
    char reserved[10];
    void *row_offsets;
} Port;

extern Port CurrentPort;
extern void far pascal ReleaseBuffer(void far *buffer);
extern void far cdecl free(void *memory);

void release_port(Port *port)
{
    if (port->buffer != CurrentPort.buffer)
        ReleaseBuffer(port->buffer);
    if (port->row_offsets != CurrentPort.row_offsets)
        free(port->row_offsets);
}
