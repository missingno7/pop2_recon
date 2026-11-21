.MODEL MEDIUM
.DATA
EXTRN CURRENT_CONTEXT:WORD
.CODE
EXTRN DRAW_COUNTED_TEXT_IN_RECT:NEAR
EXTRN MEASURE_COUNTED_TEXT:NEAR
PUBLIC SET_ORIGIN, SET_RELATIVE_PEN_STATE
PUBLIC DRAW_NUL_TEXT_IN_RECT, MEASURE_NUL_TERMINATED_TEXT

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

DRAW_NUL_TEXT_IN_RECT PROC FAR
    push bp
    mov bp, sp
    push di
    les ax, DWORD PTR [bp+0eh]
    push es
    push ax
    push WORD PTR [bp+0ch]
    push WORD PTR [bp+0ah]
    les di, DWORD PTR [bp+6]
    push es
    push di
    sub ax, ax
    mov cx, 0ffffh
    repne scasb
    not cx
    dec cx
    push cx
    push cs
    call DRAW_COUNTED_TEXT_IN_RECT
    pop di
    pop bp
    retf 12
DRAW_NUL_TEXT_IN_RECT ENDP

MEASURE_NUL_TERMINATED_TEXT PROC FAR
    push bp
    mov bp, sp
    push di
    les di, DWORD PTR [bp+6]
    push es
    push di
    sub ax, ax
    mov cx, 0ffffh
    repne scasb
    not cx
    dec cx
    push cx
    push cs
    call MEASURE_COUNTED_TEXT
    pop di
    pop bp
    retf 4
MEASURE_NUL_TERMINATED_TEXT ENDP
END
