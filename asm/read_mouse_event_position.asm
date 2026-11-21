.MODEL MEDIUM
.DATA
EXTRN MOUSE_PRESENT:WORD
EXTRN SCREEN_WIDTH:WORD
.CODE
PUBLIC READ_MOUSE_EVENT_POSITION

READ_MOUSE_EVENT_POSITION PROC FAR
    push bp
    mov bp, sp
    sub dx, dx
    mov cx, MOUSE_PRESENT
    jcxz store_position
    sub cx, cx
    mov ax, 3
    int 33h
    cmp SCREEN_WIDTH, 320
    jne store_position
    shr cx, 1
store_position:
    mov bx, [bp+6]
    mov [bx+2], cx
    mov [bx], dx
    xchg bx, ax
    pop bp
    retf 2
READ_MOUSE_EVENT_POSITION ENDP
END
