extern void far cdecl NEAR_HEAP_FREE(void *pointer);

void far pascal RELEASE_NEAR_BUFFER(void *pointer)
{
    NEAR_HEAP_FREE(pointer);
}
