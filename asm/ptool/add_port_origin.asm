.MODEL MEDIUM
.DATA
EXTRN CURRENT_CONTEXT:WORD
.CODE
PUBLIC ADD_PORT_ORIGIN

ADD_PORT_ORIGIN PROC FAR
    push bp
    mov bp, sp
    push di
    mov bx, CURRENT_CONTEXT
    les di, DWORD PTR [bp+6]
    les ax, DWORD PTR es:[di]
    add ax, [bx+4]
    mov cx, es
    add cx, [bx+6]
    les di, DWORD PTR [bp+10]
    stosw
    xchg cx, ax
    stosw
    mov ax, [bp+10]
    mov dx, es
    pop di
    pop bp
    retf 8
ADD_PORT_ORIGIN ENDP
END
