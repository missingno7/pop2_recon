/* RestoreFChar: restore the 20-byte FChar record (FM Towns CTRL.C; anchored by SetupCarpet).
   saved_fchar is descriptive: its original name is not anchored. */
struct CopyRecord {
    char bytes[20];
};

extern struct CopyRecord FChar;
extern struct CopyRecord saved_fchar;

void RestoreFChar(void)
{
    FChar = saved_fchar;
}
