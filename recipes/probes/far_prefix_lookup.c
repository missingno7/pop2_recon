/* Resident name-prefix lookup hypothesis; original names and language unknown. */
extern int entry_count;
extern char **entry_table;
extern unsigned far resident_strlen(char far *text);
extern int far resident_strnicmp(char far *left, char far *right, unsigned count);

char * far pascal lookup_prefix(char far *text)
{
    register int index;
    for (index = 1; index < entry_count; ++index)
        if (resident_strnicmp(entry_table[index], text, resident_strlen(text)) == 0)
            return entry_table[index];
    return 0;
}
