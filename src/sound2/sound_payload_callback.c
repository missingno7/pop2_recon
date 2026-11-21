typedef unsigned Handle;

extern Handle far pascal owner_of_payload(const void far *payload);
extern void far pascal clear_handle_owned(Handle handle);

void far pascal sound_payload_callback(void far *payload, unsigned status)
{
    clear_handle_owned(owner_of_payload(payload));
}
