/* copy_kid_to_char (descriptive name): Char = Kid, the inverse of the copies in SaveKidwOpp.
   Char/Kid are the FM Towns globals at DOS DS:5c00/DS:5c80. */
struct CopyRecord {
    char bytes[64];
};

extern struct CopyRecord Char;
extern struct CopyRecord Kid;

void copy_kid_to_char(void)
{
    Char = Kid;
}
