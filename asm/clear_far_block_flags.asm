.MODEL MEDIUM
.CODE
PUBLIC CLEAR_FAR_BLOCK_FLAGS

CLEAR_FAR_BLOCK_FLAGS PROC FAR
    push bp
    mov bp, sp
    les bx, DWORD PTR [bp+6]
    mov ax, es
    or ax, bx
    je done
    and WORD PTR es:[bx-6], 3fffh
done:
    pop bp
    retf 4
CLEAR_FAR_BLOCK_FLAGS ENDP
END
