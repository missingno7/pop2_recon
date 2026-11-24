; Graphics mode descriptors by mode number; only VGA is installed. The saved BIOS state follows.
GAMELIB_TEXT SEGMENT BYTE PUBLIC 'CODE'
EXTRN vga_mode_descriptor:NEAR
; Nothing uses these names, but the modules that define them are in the original image: some library module
; declares them.
EXTRN banked16_mode_descriptor:NEAR, map_alternating_scanlines:FAR
GAMELIB_TEXT ENDS
EXTRN _strdup:FAR

DGROUP GROUP _DATA
_DATA SEGMENT WORD PUBLIC 'DATA'
ASSUME DS:DGROUP
PUBLIC MODE_TABLE, SAVED_BIOS_MODE, SAVED_BIOS_EQUIPMENT
MODE_TABLE DW 6, 0, 0, 0, 0, OFFSET vga_mode_descriptor, 0
SAVED_BIOS_MODE DB 0
SAVED_BIOS_EQUIPMENT DW 0
_DATA ENDS
END
