; Alert dialog templates in DGROUP (resident catalog records DLOG -20, -30 and -40).

DGROUP GROUP _DATA
_DATA SEGMENT WORD PUBLIC 'DATA'
ASSUME DS:DGROUP
PUBLIC ALERT_20
ALERT_20 LABEL BYTE
    DB 032h, 0, 032h, 0, 096h, 0, 00Eh, 1, 1, 0, 3, 0, 0FFh, 0FFh, 04Eh, 0
    DB 00Fh, 0, 05Fh, 0, 041h, 0, 04Fh, 0, 04Fh, 06Bh, 0, 0FFh, 0FFh, 04Eh, 0, 09Bh
    DB 0, 05Fh, 0, 0CDh, 0, 043h, 0
    DB 'Cancel'
    DB 0, 0FBh, 0FFh, 2, 0, 5, 0, 04Dh, 0, 0DAh, 0, 0
PUBLIC ALERT_30
ALERT_30 LABEL BYTE
    DB 032h, 0, 032h, 0, 096h, 0, 00Eh, 1, 1, 0, 2, 0, 0FFh, 0FFh, 04Eh, 0
    DB 05Fh, 0, 05Fh, 0, 07Dh, 0, 04Fh, 0, 04Fh, 06Bh, 0, 0FBh, 0FFh, 2, 0, 00Ah
    DB 0, 0C5h, 0, 0DAh, 0, 0
PUBLIC ALERT_40
ALERT_40 LABEL BYTE
    DB 03Ch, 0, 032h, 0, 082h, 0, 00Eh, 1, 1, 0, 3, 0, 0FFh, 0FFh, 030h, 0
    DB 00Fh, 0, 041h, 0, 037h, 0, 059h, 0
    DB 'Yes'
    DB 0, 0FFh, 0FFh, 030h, 0, 0A5h, 0, 041h, 0, 0CDh, 0, 04Eh, 0, 04Eh, 06Fh, 0
    DB 0FBh, 0FFh, 2, 0, 5, 0, 02Fh, 0, 0DAh, 0, 0
_DATA ENDS
END
