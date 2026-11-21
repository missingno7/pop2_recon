extern const char far *far pascal skip_nul_strings(const char far *strings, unsigned count);

const char far *far pascal lookup_prefixed_string(const unsigned char far *strings, unsigned char index)
{
    if (strings && index && index <= *strings)
        return skip_nul_strings((const char far *)(strings + 1), index - 1);
    return 0;
}
