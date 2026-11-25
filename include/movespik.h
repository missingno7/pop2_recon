/* MOVESPIK */

void ATMSpikes(void);
void CheckImpale(void);
void CheckSpikes(void);
void CheckSpikesJammed(byte room, int index);
void DoImpale(void);
void DrawObjSpikes(byte object);
int GetSpikeIdx(word state);
MOBREC *GetSpikeMobRoomRow(byte room, sbyte row);
int GetSpikes(void);
void MobSpikes(void);
void TriggerSpikes(byte room, int index);
