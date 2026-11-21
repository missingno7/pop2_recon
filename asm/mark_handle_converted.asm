.MODEL MEDIUM
.CODE
PUBLIC MARK_HANDLE_CONVERTED

MARK_HANDLE_CONVERTED PROC FAR
    push bp
    mov bp, sp
    mov bp, [bp+6]
    test bp, bp
    je done
    or WORD PTR [bp], 2000h
done:
    xchg ax, bp
    pop bp
    retf 2
MARK_HANDLE_CONVERTED ENDP
END
