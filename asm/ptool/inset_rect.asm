.MODEL MEDIUM
.CODE
PUBLIC INSETRECT

INSETRECT PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    push ds
    lds si, DWORD PTR [bp+10]
    les di, DWORD PTR [bp+14]
    push di

    ; Move the top and left edges inward and subtract from the opposite edges.
    lodsw
    add ax, WORD PTR [bp+6]
    stosw
    lodsw
    add ax, WORD PTR [bp+8]
    stosw
    lodsw
    sub ax, WORD PTR [bp+6]
    stosw
    lodsw
    sub ax, WORD PTR [bp+8]
    stosw

    pop ax
    mov dx, es
    pop ds
    pop si
    pop di
    pop bp
    retf 12
INSETRECT ENDP
END
