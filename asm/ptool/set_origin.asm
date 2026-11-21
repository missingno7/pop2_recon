.MODEL MEDIUM
.DATA
EXTRN CURRENT_CONTEXT:WORD
.CODE
PUBLIC SET_ORIGIN

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
END
