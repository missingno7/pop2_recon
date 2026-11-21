.MODEL MEDIUM
.CODE
PUBLIC READ_HANDLE_FLAGS

READ_HANDLE_FLAGS PROC FAR
    push bp
    mov bp, sp
    mov bp, [bp+6]
    ; BP selects SS, including for the speculative load at a null handle.
    mov ax, [bp]
    test bp, bp
    jne have_handle
    xchg ax, bp
have_handle:
    pop bp
    retf 2
READ_HANDLE_FLAGS ENDP
END
