.MODEL MEDIUM
.DATA
EXTRN CURRENT_CONTEXT:WORD
.CODE
PUBLIC SET_ORIGIN, SET_RELATIVE_PEN_STATE

SET_ORIGIN PROC FAR
    push bp
    mov bp, sp
    les dx, DWORD PTR [bp+6]
    mov cx, es
    mov bx, CURRENT_CONTEXT
    sub cx, [bx+12h]
    sub dx, [bx+10h]
    add [bx+12h], cx
    add [bx+10h], dx
    add [bx+16h], cx
    add [bx+14h], dx
    add [bx+6], cx
    add [bx+4], dx
    add [bx+0ah], cx
    add [bx+8], dx
    pop bp
    retf 4
SET_ORIGIN ENDP

SET_RELATIVE_PEN_STATE PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    push ds
    mov cx, 8
    mov di, CURRENT_CONTEXT
    add di, 22h
    push ds
    pop es
    lds si, DWORD PTR [bp+6]
    rep movsb
    pop ds
    pop si
    pop di
    pop bp
    retf 4
SET_RELATIVE_PEN_STATE ENDP
END
