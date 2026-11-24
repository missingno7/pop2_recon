; Built-in window-library resources addressed by the resident catalog (catalog.asm): two sounds, dialog
; templates (DLOG) and one string list. Contents are original data bytes, in catalog order.

RESOURCE_DATA SEGMENT PARA PUBLIC 'FAR_DATA'
PUBLIC RES_SND_1
RES_SND_1 LABEL BYTE
    DB 0, 03Ch, 0, 0E8h, 3, 014h, 012h, 0
PUBLIC RES_SND_2
RES_SND_2 LABEL BYTE
    DB 0, 03Ch, 0, 096h, 0, 6, 0, 0, 6, 07Dh, 0, 6, 012h, 0
PUBLIC RES_DLOG_100
RES_DLOG_100 LABEL BYTE
    DB 032h, 0, 035h, 0, 067h, 0, 00Bh, 1, 3, 0, 4, 0, 0FFh, 0FFh, 024h, 0
    DB 4, 0, 031h, 0, 046h, 0, 041h, 0
    DB 'Abort'
    DB 0, 0FFh, 0FFh, 024h, 0, 04Ah, 0, 031h, 0, 08Ch, 0, 01Bh, 0
    DB 'Cancel '
    DB 01Bh, 0, 0FFh, 0FFh, 024h, 0, 090h, 0, 031h, 0, 0D2h, 0, 052h, 0
    DB 'Retry'
    DB 0, 0FBh, 0FFh, 4, 0, 4, 0, 022h, 0, 0D2h, 0, 0
PUBLIC RES_STRL_100
RES_STRL_100 LABEL BYTE
    DB 6
    DB 'Disk error on drive %c:'
    DB 0
    DB 'Drive %c: not ready.'
    DB 00Dh
    DB 'Please close the drive door.'
    DB 0
    DB 'Drive %c: is write protected.'
    DB 00Dh
    DB 'Please adjust the write protect tab.'
    DB 0
    DB 'Printer error.'
    DB 0
    DB 'Printer out of paper.'
    DB 0
    DB 'System error.'
    DB 0
PUBLIC RES_DLOG_200
RES_DLOG_200 LABEL BYTE
    DB 032h, 0, 050h, 0, 05Fh, 0, 0F0h, 0, 1, 0, 2, 0, 0FFh, 0FFh, 01Ch, 0
    DB 064h, 0, 029h, 0, 09Ch, 0, 04Fh, 0, 04Fh, 06Bh, 0, 0FBh, 0FFh, 4, 0, 4
    DB 0, 018h, 0, 09Ch, 0
    DB 'Invalid DOS path.'
    DB 0
PUBLIC RES_DLOG_201
RES_DLOG_201 LABEL BYTE
    DB 032h, 0, 050h, 0, 05Fh, 0, 0F0h, 0, 2, 0, 3, 0, 0FFh, 0FFh, 01Ch, 0
    DB 028h, 0, 029h, 0, 060h, 0, 059h, 0
    DB 'Yes'
    DB 0, 0FFh, 0FFh, 01Ch, 0, 064h, 0, 029h, 0, 09Ch, 0, 04Eh, 0, 04Eh, 06Fh, 0
    DB 0FBh, 0FFh, 4, 0, 4, 0, 018h, 0, 09Ch, 0
    DB 'File already exists.'
    DB 00Dh
    DB 'Replace it?'
    DB 0
PUBLIC RES_DLOG_202
RES_DLOG_202 LABEL BYTE
    DB 028h, 0, 019h, 0, 092h, 0, 027h, 1, 0, 0, 8, 0, 0FFh, 0FFh, 037h, 0
    DB 096h, 0, 044h, 0, 0F0h, 0, 4, 0
    DB 'Drive '
    DB 01Dh, 044h, 0, 0FFh, 0FFh, 048h, 0, 096h, 0, 055h, 0, 0F0h, 0, 00Fh, 0
    DB 'Okay '
    DB 01Dh, 04Fh, 0, 0FFh, 0FFh, 059h, 0, 096h, 0, 066h, 0, 0F0h, 0, 01Bh, 0
    DB 'Cancel '
    DB 01Bh, 0, 0FEh, 0FFh, 4, 0, 4, 0, 010h, 0, 00Ah, 1, 040h, 0, 0FCh, 0FFh
    DB 01Eh, 0, 069h, 0, 066h, 0, 078h, 0, 1, 0, 0FBh, 0FFh, 01Eh, 0, 07Ch, 0
    DB 032h, 0, 00Ah, 1, 0, 0FBh, 0FFh, 013h, 0, 4, 0, 01Ch, 0, 00Ah, 1, 0
    DB 0FBh, 0FFh, 01Fh, 0, 5, 0, 065h, 0, 069h, 0, 0
PUBLIC RES_DLOG_203
RES_DLOG_203 LABEL BYTE
    DB 028h, 0, 019h, 0, 06Fh, 0, 027h, 1, 0, 0, 7, 0, 0FFh, 0FFh, 036h, 0
    DB 012h, 0, 043h, 0, 062h, 0, 4, 0
    DB 'Drive '
    DB 01Dh, 044h, 0, 0FFh, 0FFh, 036h, 0, 066h, 0, 043h, 0, 0B6h, 0, 00Fh, 0
    DB 'Okay '
    DB 01Dh, 04Fh, 0, 0FFh, 0FFh, 036h, 0, 0BAh, 0, 043h, 0, 00Ah, 1, 01Bh, 0
    DB 'Cancel '
    DB 01Bh, 0, 0FEh, 0FFh, 4, 0, 4, 0, 010h, 0, 00Ah, 1, 040h, 0, 0FEh, 0FFh
    DB 01Eh, 0, 4, 0, 02Ah, 0, 068h, 0, 00Ch, 0, 0FBh, 0FFh, 01Eh, 0, 06Ch, 0
    DB 032h, 0, 00Ah, 1, 0, 0FBh, 0FFh, 013h, 0, 4, 0, 01Ch, 0, 00Ah, 1, 0
RESOURCE_DATA ENDS
END
