/* SaveSelect: remember the current selection (FM Towns CTRL.C public; anchored by PlayerCtrl
   call sites in both builds). Dir/SelDir are the FM Towns globals at DOS DS:628c/DS:626a. */
struct CopyRecord {
    char bytes[5];
};

extern struct CopyRecord SelDir;
extern struct CopyRecord Dir;

void SaveSelect(void)
{
    SelDir = Dir;
}
