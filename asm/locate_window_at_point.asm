.MODEL MEDIUM
.DATA
EXTRN WINDOW_LIST:BYTE
system_hit_rectangle DW 0, 0, 0, 0
.CODE
EXTRN POINT_IN_RECTANGLE:NEAR
PUBLIC LOCATE_WINDOW_AT_POINT

LOCATE_WINDOW_AT_POINT PROC FAR
    push bp
    mov bp, sp
    push di
    push ds
    push WORD PTR [bp+8]
    mov ax, OFFSET system_hit_rectangle
    push ds
    push ax
    push cs
    call POINT_IN_RECTANGLE
    xchg cx, ax
    jcxz find_hit_window
    mov ax, 1
    sub di, di
    jmp SHORT store_hit_window
find_hit_window:
    mov di, OFFSET WINDOW_LIST
test_next_window:
    mov ax, 0
    mov di, [di+2ch]
    test di, di
    jz store_hit_window
    push di
    mov ax, 3
    push ax
    push WORD PTR [bp+8]
    call DWORD PTR [di+2eh]
    cmp ax, 0
    je test_next_window
store_hit_window:
    mov bx, [bp+6]
    mov [bx], di
    pop di
    pop bp
    retf 4
LOCATE_WINDOW_AT_POINT ENDP
END
