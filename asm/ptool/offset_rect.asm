.MODEL MEDIUM
.CODE
PUBLIC OFFSETRECT

OFFSETRECT PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    push ds
    lds si, DWORD PTR [bp+10]
    les di, DWORD PTR [bp+14]
    push di

    ; Translate corresponding edges in top, left, bottom, right order.
    lodsw
    add ax, [bp+6]
    stosw
    lodsw
    add ax, [bp+8]
    stosw
    lodsw
    add ax, [bp+6]
    stosw
    lodsw
    add ax, [bp+8]
    stosw

    pop ax
    mov dx, es
    pop ds
    pop si
    pop di
    pop bp
    retf 12
OFFSETRECT ENDP
END
