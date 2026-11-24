; 512 bytes between the end of the data (_end) and the C stack. The interrupt routines switch to _end+100h (sound)
; and _end+200h (mouse), so the two 256-byte stacks lie here. The declaring module is unknown.

INTERRUPT_STACK SEGMENT WORD PUBLIC 'STACK'
    DB 512 DUP (?)
INTERRUPT_STACK ENDS
DGROUP GROUP INTERRUPT_STACK
END
