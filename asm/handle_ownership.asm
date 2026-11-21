.MODEL MEDIUM
.CODE
PUBLIC CLEAR_HANDLE_OWNED, OWNER_OF_PAYLOAD

CLEAR_HANDLE_OWNED PROC FAR
    push bp
    mov bp, sp
    mov bp, [bp+6]
    test bp, bp
    jz clear_done
    and WORD PTR [bp], 7fffh
clear_done:
    xchg ax, bp
    pop bp
    retf 2
CLEAR_HANDLE_OWNED ENDP

OWNER_OF_PAYLOAD PROC FAR
    push bp
    mov bp, sp
    les bx, DWORD PTR [bp+6]
    mov ax, es
    or ax, bx
    jz owner_done
    sub ax, ax
    cmp WORD PTR es:[bx-6], 0c000h
    jb owner_done
    mov ax, es:[bx-2]
owner_done:
    pop bp
    retf 4
OWNER_OF_PAYLOAD ENDP
END
