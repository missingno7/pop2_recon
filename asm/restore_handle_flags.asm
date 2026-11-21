.MODEL MEDIUM
.CODE
PUBLIC RESTORE_HANDLE_FLAGS

RESTORE_HANDLE_FLAGS PROC FAR
    push bp
    mov bp, sp
    mov ax, [bp+6]
    mov bp, [bp+8]
    test bp, bp
    jz done
    and ax, 0e000h
    and WORD PTR [bp], 1fffh
    or [bp], ax
done:
    xchg ax, bp
    pop bp
    retf 4
RESTORE_HANDLE_FLAGS ENDP
END
