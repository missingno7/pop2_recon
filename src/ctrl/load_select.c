/* LoadSelect: restore the remembered selection before input polling (FM Towns CTRL.C). */
struct CopyRecord {
    char bytes[5];
};

extern struct CopyRecord Dir;
extern struct CopyRecord SelDir;

void LoadSelect(void)
{
    Dir = SelDir;
}
