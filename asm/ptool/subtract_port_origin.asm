.MODEL MEDIUM
.DATA
EXTRN CURRENT_CONTEXT:WORD
.CODE
PUBLIC SUBTRACT_PORT_ORIGIN

SUBTRACT_PORT_ORIGIN PROC FAR
    push bp
    mov bp, sp
    push di
    mov bx, CURRENT_CONTEXT
    les di, DWORD PTR [bp+6]
    les ax, DWORD PTR es:[di]
    sub ax, [bx+4]
    mov cx, es
    sub cx, [bx+6]
    les di, DWORD PTR [bp+10]
    stosw
    xchg ax, cx
    stosw
    mov ax, [bp+10]
    mov dx, es
    pop di
    pop bp
    retf 8
SUBTRACT_PORT_ORIGIN ENDP
END
