extern void far cdecl _nfree(void *pointer);

void far pascal RELEASE_NEAR_BUFFER(void *pointer)
{
    _nfree(pointer);
}
