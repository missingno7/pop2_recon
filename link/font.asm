; Built-in system font (resident catalog record FONT 0, 5F7h bytes).

FONT_FAR SEGMENT PARA PUBLIC 'FAR_DATA'
PUBLIC SYSTEM_FONT
SYSTEM_FONT LABEL BYTE
    DB 01Bh, 07Eh, 7, 0, 2, 0, 1, 0, 1, 0, 0D2h, 0, 0F0h, 0, 4, 1
    DB 022h, 1, 02Fh, 1, 03Ch, 1, 042h, 1, 04Fh, 1, 058h, 1, 064h, 1, 071h, 1
    DB 07Eh, 1, 08Bh, 1, 094h, 1, 0A1h, 1, 0AEh, 1, 0BAh, 1, 0C6h, 1, 0D4h, 1
    DB 0DEh, 1, 0EBh, 1, 0F8h, 1, 5, 2, 012h, 2, 01Fh, 2, 02Ch, 2, 039h, 2
    DB 046h, 2, 053h, 2, 060h, 2, 06Dh, 2, 07Ah, 2, 086h, 2, 094h, 2, 0A1h, 2
    DB 0ACh, 2, 0B9h, 2, 0C6h, 2, 0D3h, 2, 0E0h, 2, 0EDh, 2, 0FAh, 2, 7, 3
    DB 014h, 3, 021h, 3, 02Eh, 3, 03Bh, 3, 048h, 3, 055h, 3, 062h, 3, 06Fh, 3
    DB 07Ch, 3, 089h, 3, 096h, 3, 0A3h, 3, 0B0h, 3, 0BDh, 3, 0CAh, 3, 0D7h, 3
    DB 0E4h, 3, 0F1h, 3, 0FEh, 3, 00Bh, 4, 018h, 4, 025h, 4, 032h, 4, 03Fh, 4
    DB 04Ch, 4, 055h, 4, 063h, 4, 06Ch, 4, 079h, 4, 086h, 4, 093h, 4, 0A0h, 4
    DB 0ADh, 4, 0BAh, 4, 0C9h, 4, 0D6h, 4, 0E3h, 4, 0F2h, 4, 0FFh, 4, 00Ch, 5
    DB 019h, 5, 026h, 5, 033h, 5, 042h, 5, 051h, 5, 05Eh, 5, 06Bh, 5, 078h, 5
    DB 085h, 5, 092h, 5, 09Fh, 5, 0ACh, 5, 0BBh, 5, 0C8h, 5, 0D5h, 5, 0E2h, 5
    DB 0EFh, 5, 8, 0, 011h, 0, 3, 0, 07Fh, 0FFh, 0, 0C6h, 073h, 080h, 0DDh, 0ADh
    DB 080h, 0CEh, 0EFh, 080h, 0DFh, 06Fh, 080h, 0DDh, 0ADh, 080h, 0C6h, 073h, 080h, 07Fh, 0FFh, 0
    DB 8, 0, 010h, 0, 2, 0, 07Fh, 0FEh, 0CDh, 0C7h, 0B5h, 0EFh, 0B5h, 0EFh, 085h, 0EFh
    DB 0B5h, 0EFh, 0B4h, 06Fh, 8, 0, 013h, 0, 3, 0, 07Fh, 0FFh, 0C0h, 0CCh, 046h, 0E0h
    DB 0B6h, 0DAh, 0E0h, 0BEh, 0DAh, 0E0h, 0BEh, 0C6h, 0E0h, 0B6h, 0DAh, 0E0h, 0CEh, 0DAh, 020h, 07Fh
    DB 0FFh, 0C0h, 7, 0, 7, 0, 1, 0, 0, 6, 00Ch, 0D8h, 0F0h, 0E0h, 0C0h, 7
    DB 0, 7, 0, 1, 0, 0, 0, 070h, 0C4h, 0CCh, 08Ch, 038h, 0, 0, 3, 0
    DB 0, 0, 7, 0, 2, 0, 1, 0, 0C0h, 0C0h, 0C0h, 0C0h, 0C0h, 0, 0C0h, 3
    DB 0, 5, 0, 1, 0, 0D8h, 0D8h, 0D8h, 6, 0, 7, 0, 1, 0, 0, 06Ch
    DB 0FEh, 06Ch, 0FEh, 06Ch, 7, 0, 7, 0, 1, 0, 010h, 07Ch, 0D0h, 07Ch, 016h, 07Ch
    DB 010h, 7, 0, 8, 0, 1, 0, 0C3h, 0C6h, 00Ch, 018h, 030h, 063h, 0C3h, 7, 0
    DB 8, 0, 1, 0
    DB '8l8z'
    DB 0CCh, 0CEh, 07Bh, 3, 0, 3, 0, 1, 0, 060h, 060h, 0C0h, 7, 0, 4, 0
    DB 1, 0, 030h, 060h, 0C0h, 0C0h, 0C0h, 060h, 030h, 7, 0, 4, 0, 1, 0, 0C0h
    DB '`000`'
    DB 0C0h, 6, 0, 7, 0, 1, 0, 0, 06Ch, 038h, 0FEh, 038h, 06Ch, 6, 0, 6
    DB 0, 1, 0, 0, 030h, 030h, 0FCh, 030h, 030h, 8, 0, 3, 0, 1, 0, 0
    DB 0, 0, 0, 0, 060h, 060h, 0C0h, 4, 0, 4, 0, 1, 0, 0, 0, 0
    DB 0F0h, 7, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0C0h, 0C0h, 7, 0
    DB 8, 0, 1, 0, 3, 6, 00Ch, 018h, 030h, 060h, 0C0h, 7, 0, 6, 0, 1
    DB 0, 078h, 0CCh, 0CCh, 0CCh, 0CCh, 0CCh, 078h, 7, 0, 6, 0, 1, 0, 030h, 070h
    DB 0F0h
    DB '000'
    DB 0FCh, 7, 0, 6, 0, 1, 0, 078h, 0CCh, 00Ch, 018h, 030h, 060h, 0FCh, 7, 0
    DB 6, 0, 1, 0, 078h, 0CCh, 00Ch, 018h, 00Ch, 0CCh, 078h, 7, 0, 7, 0, 1
    DB 0, 01Ch, 03Ch, 06Ch, 0CCh, 0FEh, 00Ch, 00Ch, 7, 0, 6, 0, 1, 0, 0F8h, 0C0h
    DB 0C0h, 0F8h, 00Ch, 00Ch, 0F8h, 7, 0, 6, 0, 1, 0, 078h, 0C0h, 0C0h, 0F8h, 0CCh
    DB 0CCh, 078h, 7, 0, 6, 0, 1, 0, 0FCh, 00Ch, 018h
    DB '0000'
    DB 7, 0, 6, 0, 1, 0, 078h, 0CCh, 0CCh, 078h, 0CCh, 0CCh, 078h, 7, 0, 6
    DB 0, 1, 0, 078h, 0CCh, 0CCh, 07Ch, 00Ch, 0CCh, 078h, 6, 0, 2, 0, 1, 0
    DB 0, 0C0h, 0C0h, 0, 0C0h, 0C0h, 8, 0, 3, 0, 1, 0, 0, 060h, 060h, 0
    DB 0, 060h, 060h, 0C0h, 7, 0, 5, 0, 1, 0, 018h, 030h, 060h, 0C0h, 060h, 030h
    DB 018h, 5, 0, 4, 0, 1, 0, 0, 0, 0F0h, 0, 0F0h, 7, 0, 5, 0
    DB 1, 0, 0C0h, 060h, 030h, 018h, 030h, 060h, 0C0h, 7, 0, 6, 0, 1, 0, 078h
    DB 0CCh, 00Ch, 018h, 030h, 0, 030h, 7, 0, 6, 0, 1, 0, 078h, 0CCh, 0DCh, 0DCh
    DB 0D8h, 0C0h, 078h, 7, 0, 6, 0, 1, 0, 078h, 0CCh, 0CCh, 0FCh, 0CCh, 0CCh, 0CCh
    DB 7, 0, 6, 0, 1, 0, 0F8h, 0CCh, 0CCh, 0F8h, 0CCh, 0CCh, 0F8h, 7, 0, 6
    DB 0, 1, 0, 078h, 0CCh, 0C0h, 0C0h, 0C0h, 0CCh, 078h, 7, 0, 6, 0, 1, 0
    DB 0F8h, 0CCh, 0CCh, 0CCh, 0CCh, 0CCh, 0F8h, 7, 0, 5, 0, 1, 0, 0F8h, 0C0h, 0C0h
    DB 0F0h, 0C0h, 0C0h, 0F8h, 7, 0, 5, 0, 1, 0, 0F8h, 0C0h, 0C0h, 0F0h, 0C0h, 0C0h
    DB 0C0h, 7, 0, 6, 0, 1, 0, 078h, 0CCh, 0C0h, 0DCh, 0CCh, 0CCh, 078h, 7, 0
    DB 6, 0, 1, 0, 0CCh, 0CCh, 0CCh, 0FCh, 0CCh, 0CCh, 0CCh, 7, 0, 4, 0, 1
    DB 0, 0F0h
    DB '`````'
    DB 0F0h, 7, 0, 6, 0, 1, 0, 00Ch, 00Ch, 00Ch, 00Ch, 00Ch, 0CCh, 078h, 7, 0
    DB 7, 0, 1, 0, 0C6h, 0CCh, 0D8h, 0F0h, 0D8h, 0CCh, 0C6h, 7, 0, 5, 0, 1
    DB 0, 0C0h, 0C0h, 0C0h, 0C0h, 0C0h, 0C0h, 0F8h, 7, 0, 8, 0, 1, 0, 0C3h, 0E7h
    DB 0FFh, 0DBh, 0C3h, 0C3h, 0C3h, 7, 0, 6, 0, 1, 0, 0CCh, 0CCh, 0ECh, 0FCh, 0DCh
    DB 0CCh, 0CCh, 7, 0, 6, 0, 1, 0, 078h, 0CCh, 0CCh, 0CCh, 0CCh, 0CCh, 078h, 7
    DB 0, 6, 0, 1, 0, 0F8h, 0CCh, 0CCh, 0F8h, 0C0h, 0C0h, 0C0h, 7, 0, 6, 0
    DB 1, 0, 078h, 0CCh, 0CCh, 0CCh, 0CCh, 0D8h, 06Ch, 7, 0, 6, 0, 1, 0, 0F8h
    DB 0CCh, 0CCh, 0F8h, 0D8h, 0CCh, 0CCh, 7, 0, 6, 0, 1, 0, 078h, 0CCh, 0C0h, 078h
    DB 00Ch, 0CCh, 078h, 7, 0, 6, 0, 1, 0, 0FCh
    DB '000000'
    DB 7, 0, 6, 0, 1, 0, 0CCh, 0CCh, 0CCh, 0CCh, 0CCh, 0CCh, 07Ch, 7, 0, 6
    DB 0, 1, 0, 0CCh, 0CCh, 0CCh, 0CCh, 0CCh, 078h, 030h, 7, 0, 8, 0, 1, 0
    DB 0C3h, 0C3h, 0C3h, 0DBh, 0FFh, 0E7h, 0C3h, 7, 0, 6, 0, 1, 0, 0CCh, 0CCh
    DB 'x0x'
    DB 0CCh, 0CCh, 7, 0, 6, 0, 1, 0, 0CCh, 0CCh, 0CCh
    DB 'x000'
    DB 7, 0, 8, 0, 1, 0, 0FFh, 6, 00Ch, 018h, 030h, 060h, 0FFh, 7, 0, 4
    DB 0, 1, 0, 0F0h, 0C0h, 0C0h, 0C0h, 0C0h, 0C0h, 0F0h, 7, 0, 8, 0, 1, 0
    DB 0C0h, 060h, 030h, 018h, 00Ch, 6, 3, 7, 0, 4, 0, 1, 0, 0F0h
    DB '00000'
    DB 0F0h, 3, 0, 6, 0, 1, 0, 030h, 078h, 0CCh, 8, 0, 6, 0, 1, 0
    DB 0, 0, 0, 0, 0, 0, 0, 0FCh, 3, 0, 4, 0, 1, 0, 0C0h, 060h
    DB 030h, 7, 0, 6, 0, 1, 0, 0, 0, 078h, 00Ch, 07Ch, 0CCh, 07Ch, 7, 0
    DB 6, 0, 1, 0, 0C0h, 0C0h, 0F8h, 0CCh, 0CCh, 0CCh, 0F8h, 7, 0, 6, 0, 1
    DB 0, 0, 0, 078h, 0CCh, 0C0h, 0CCh, 078h, 7, 0, 6, 0, 1, 0, 00Ch, 00Ch
    DB 07Ch, 0CCh, 0CCh, 0CCh, 07Ch, 7, 0, 6, 0, 1, 0, 0, 0, 078h, 0CCh, 0FCh
    DB 0C0h, 07Ch, 7, 0, 5, 0, 1, 0, 038h, 060h, 0F8h
    DB '````'
    DB 9, 0, 6, 0, 1, 0, 0, 0, 078h, 0CCh, 0CCh, 0CCh, 07Ch, 00Ch, 078h, 7
    DB 0, 6, 0, 1, 0, 0C0h, 0C0h, 0F8h, 0CCh, 0CCh, 0CCh, 0CCh, 7, 0, 2, 0
    DB 1, 0, 0C0h, 0, 0C0h, 0C0h, 0C0h, 0C0h, 0C0h, 9, 0, 4, 0, 1, 0, 030h
    DB 0
    DB '000000'
    DB 0E0h, 7, 0, 6, 0, 1, 0, 0C0h, 0C0h, 0CCh, 0D8h, 0F0h, 0D8h, 0CCh, 7, 0
    DB 2, 0, 1, 0, 0C0h, 0C0h, 0C0h, 0C0h, 0C0h, 0C0h, 0C0h, 7, 0, 8, 0, 1
    DB 0, 0, 0, 0FEh, 0DBh, 0DBh, 0DBh, 0DBh, 7, 0, 6, 0, 1, 0, 0, 0
    DB 0F8h, 0CCh, 0CCh, 0CCh, 0CCh, 7, 0, 6, 0, 1, 0, 0, 0, 078h, 0CCh, 0CCh
    DB 0CCh, 078h, 9, 0, 6, 0, 1, 0, 0, 0, 0F8h, 0CCh, 0CCh, 0CCh, 0F8h, 0C0h
    DB 0C0h, 9, 0, 6, 0, 1, 0, 0, 0, 078h, 0CCh, 0CCh, 0CCh, 07Ch, 00Ch, 00Ch
    DB 7, 0, 6, 0, 1, 0, 0, 0, 078h, 0CCh, 0C0h, 0C0h, 0C0h, 7, 0, 6
    DB 0, 1, 0, 0, 0, 078h, 0C0h, 078h, 00Ch, 0F8h, 7, 0, 5, 0, 1, 0
    DB 060h, 060h, 0F8h
    DB '```8'
    DB 7, 0, 6, 0, 1, 0, 0, 0, 0CCh, 0CCh, 0CCh, 0CCh, 07Ch, 7, 0, 6
    DB 0, 1, 0, 0, 0, 0CCh, 0CCh, 0CCh, 078h, 030h, 7, 0, 8, 0, 1, 0
    DB 0, 0, 0C3h, 0C3h, 0DBh, 0FFh, 066h, 7, 0, 6, 0, 1, 0, 0, 0, 0CCh
    DB 'x0x'
    DB 0CCh, 9, 0, 6, 0, 1, 0, 0, 0, 0CCh, 0CCh, 0CCh, 0CCh, 07Ch, 00Ch, 078h
    DB 7, 0, 6, 0, 1, 0, 0, 0, 0FCh, 018h, 030h, 060h, 0FCh, 7, 0, 4
    DB 0, 1, 0
    DB '0``'
    DB 0C0h
    DB '``0'
    DB 7, 0, 2, 0, 1, 0, 0C0h, 0C0h, 0C0h, 0, 0C0h, 0C0h, 0C0h, 7, 0, 4
    DB 0, 1, 0, 0C0h
    DB '``0``'
    DB 0C0h, 2, 0, 7, 0, 1, 0, 076h, 0DCh
FONT_FAR ENDS
END
