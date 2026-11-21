.MODEL MEDIUM
.DATA
EXTRN CURRENT_PORT:WORD
EXTRN WINDOW_LIST_ANCHOR:BYTE
EXTRN WINDOW_HEAD:WORD
.CODE
EXTRN DISPOSE_HANDLE:NEAR
EXTRN DISPOSE_WINDOW_ITEM:NEAR
EXTRN ACTIVATE_WINDOW:NEAR
EXTRN INVALIDATE_WINDOW_RECTANGLE:NEAR
PUBLIC CLOSE_WINDOW

CLOSE_WINDOW PROC FAR
    push bp
    mov bp, sp
    push di
    mov ax, OFFSET WINDOW_LIST_ANCHOR
    xchg CURRENT_PORT, ax
    push ax
    mov di, [bp+6]
    mov ax, [di+1ah]
    xchg [di+1eh], ax
    push ax
    mov cx, [di+5ah]
    jcxz dispose_items
    push cx
    push cs
    call DISPOSE_HANDLE
dispose_items:
    mov cx, [di+50h]
    jcxz restore_window_flags
    push cx
    push cs
    call DISPOSE_WINDOW_ITEM
    jmp short dispose_items
restore_window_flags:
    pop WORD PTR [di+1eh]
    mov cx, OFFSET WINDOW_LIST_ANCHOR
next_window:
    mov bx, cx
    mov cx, [bx+2ch]
    jcxz notify_window
    cmp di, cx
    jne next_window
    mov cx, [di+2ch]
    mov [bx+2ch], cx
    jcxz invalidate_bounds
    cmp cx, WINDOW_HEAD
    jne invalidate_bounds
    push cx
    push cs
    call ACTIVATE_WINDOW
invalidate_bounds:
    lea ax, [di+3eh]
    push ax
    sub ax, ax
    push ax
    push cs
    call INVALIDATE_WINDOW_RECTANGLE
notify_window:
    push di
    mov ax, 4
    push ax
    push ax
    call DWORD PTR [di+2eh]
    pop CURRENT_PORT
    pop di
    pop bp
    retf 2
CLOSE_WINDOW ENDP
END
