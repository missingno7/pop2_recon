.MODEL MEDIUM
.CODE
PUBLIC SAVE_HANDLE_FLAGS

SAVE_HANDLE_FLAGS PROC FAR
    push bp
    mov bp, sp
    mov bp, [bp+6]
    mov ax, [bp]
    test bp, bp
    jnz done
    xchg ax, bp
done:
    pop bp
    retf 2
SAVE_HANDLE_FLAGS ENDP
END
