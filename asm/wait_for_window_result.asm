.MODEL MEDIUM
.DATA
EXTRN CURRENT_PORT:WORD
EXTRN WINDOW_HEAD:WORD
.CODE
EXTRN READ_QUEUED_EVENT:NEAR
EXTRN PAINT_PENDING_WINDOW:NEAR
EXTRN DISPATCH_WINDOW_EVENT:NEAR
EXTRN RESTORE_WINDOW_CLIP:NEAR
PUBLIC WAIT_FOR_WINDOW_RESULT

WAIT_FOR_WINDOW_RESULT PROC FAR
    push bp
    mov bp, sp
    add sp, -16h
    push di
    push si
    push CURRENT_PORT
    mov di, WINDOW_HEAD
read_event:
    mov CURRENT_PORT, di
    sub ax, ax
    mov [bp-12h], ax
    mov [bp-14h], ax
    lea si, [bp-0eh]
    push si
    mov ax, 0ffefh
    push ax
    push cs
    call READ_QUEUED_EVENT
    mov ax, [si]
    cmp ax, 2
    je check_window
    cmp ax, 1
    jne check_idle
check_window:
    cmp di, [si+0ah]
    jne read_event
    jmp short dispatch_event
check_idle:
    cmp ax, 0
    jne dispatch_event
    mov cx, [di+46h]
    jcxz dispatch_event
    mov WORD PTR [si], 10h
    mov [si+0ah], di
    mov [bp-14h], di
    push di
    push cs
    call PAINT_PENDING_WINDOW
dispatch_event:
    mov ax, [bp+6]
    or ax, [bp+8]
    je default_dispatch
    push di
    push si
    lea ax, [bp-10h]
    push ax
    call DWORD PTR [bp+6]
    mov [bp-12h], ax
    test ax, ax
    jne finish_event
default_dispatch:
    push di
    push si
    lea ax, [bp-10h]
    push ax
    push cs
    call DISPATCH_WINDOW_EVENT
    mov [bp-12h], ax
finish_event:
    mov cx, [bp-14h]
    jcxz check_result
    push cx
    push cs
    call RESTORE_WINDOW_CLIP
check_result:
    mov cx, [bp-12h]
    jcxz read_event
    mov ax, [bp-10h]
    pop CURRENT_PORT
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 4
WAIT_FOR_WINDOW_RESULT ENDP
END
