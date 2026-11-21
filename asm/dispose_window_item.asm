.MODEL MEDIUM
.DATA
EXTRN CURRENT_PORT:WORD
.CODE
EXTRN SELECT_FOCUSED_ITEM:NEAR
EXTRN STROKE_PORT_RECT:NEAR
EXTRN FREE_NEAR:FAR
PUBLIC DISPOSE_WINDOW_ITEM

DISPOSE_WINDOW_ITEM PROC FAR
    push bp
    mov bp, sp
    push di
    mov di, [bp+6]
    mov bx, [di+2]
    cmp di, [bx+52h]
    jne find_item_link
    push bx
    sub ax, ax
    push ax
    push cs
    call SELECT_FOCUSED_ITEM
find_item_link:
    mov cx, [di+2]
    add cx, 50h
next_item_link:
    mov bx, cx
    mov cx, [bx]
    jcxz redraw_bounds
    cmp cx, di
    jne next_item_link
    mov ax, [di]
    mov [bx], ax
redraw_bounds:
    mov ax, [di+2]
    xchg CURRENT_PORT, ax
    push ax
    lea ax, [di+0ch]
    push ds
    push ax
    push cs
    call STROKE_PORT_RECT
    pop CURRENT_PORT
    push di
    mov ax, 8
    push ax
    sub ax, ax
    push ax
    call DWORD PTR [di+4]
    push di
    call FREE_NEAR
    add sp, 2
    pop di
    pop bp
    retf 2
DISPOSE_WINDOW_ITEM ENDP
END
