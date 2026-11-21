.MODEL MEDIUM
.CODE
PUBLIC MARK_FIXED, MARK_PURGEABLE

MARK_FIXED PROC FAR
    push bp
    mov bp, sp
    mov bp, [bp+6]
    test bp, bp
    jz fixed_done
    and WORD PTR [bp], 0bfffh
fixed_done:
    xchg ax, bp
    pop bp
    retf 2
MARK_FIXED ENDP

MARK_PURGEABLE PROC FAR
    push bp
    mov bp, sp
    mov bp, [bp+6]
    test bp, bp
    jz purgeable_done
    or WORD PTR [bp], 4000h
purgeable_done:
    xchg ax, bp
    pop bp
    retf 2
MARK_PURGEABLE ENDP
END
