.MODEL MEDIUM
.CODE
PUBLIC MAKESHAPERECT

MAKESHAPERECT PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    pushf
    cld
    les di, DWORD PTR [bp+14]
    lds si, DWORD PTR [bp+6]
    mov ax, [bp+12]
    stosw
    mov bx, ax
    mov ax, [bp+10]
    stosw
    push ax
    mov ax, bx
    add ax, [si]
    stosw
    pop ax
    add ax, [si+2]
    stosw
    les ax, DWORD PTR [bp+14]
    mov dx, es
    push ss
    pop ds
    popf
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 12
MAKESHAPERECT ENDP
END
