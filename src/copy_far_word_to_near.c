int * far pascal COPY_FAR_WORD_TO_NEAR(int *destination, const int far *source)
{
    *destination = *source;
    return destination;
}
