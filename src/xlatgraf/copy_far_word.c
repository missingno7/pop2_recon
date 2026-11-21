unsigned *far pascal copy_far_word(unsigned *out, const unsigned far *in)
{
    *out = *in;
    return out;
}
