/* CHECKNIS */

extern sbyte quote_index;

int CheckPlayNIS(int level, int previous_level);
void DrawDeadKid(int install_palette);
void DrawRoomFromNIS(sbyte room, sbyte level);
int EraseScreenAfterNIS(int x);
int EraseScreenBeforeNIS(int sequence);
int IsLevelStartWithNISScreen(void);
int IsTimeForNIS(int level, int previous_level);
int PlayNIS(int sequence);
