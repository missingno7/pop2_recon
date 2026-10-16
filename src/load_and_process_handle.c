/* Forward the handle through the typed loader and processor. */
extern unsigned int far pascal LOAD_TYPED_HANDLE(unsigned int handle);
extern unsigned int far pascal PROCESS_HANDLE(unsigned int handle);

unsigned int far pascal LOAD_AND_PROCESS_HANDLE(unsigned int handle)
{
    return PROCESS_HANDLE(LOAD_TYPED_HANDLE(handle));
}
