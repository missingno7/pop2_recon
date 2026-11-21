extern unsigned int DEFAULT_HANDLE_TAG;
extern unsigned int far pascal ALLOCATE_TAGGED_HANDLE(void far *value, unsigned int tag);

unsigned int far pascal ALLOCATE_DEFAULT_HANDLE(void)
{
    return ALLOCATE_TAGGED_HANDLE((void far *)0, DEFAULT_HANDLE_TAG);
}
