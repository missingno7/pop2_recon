.MODEL MEDIUM
.DATA
EXTRN WINDOW_LIST:BYTE
EXTRN CURRENT_CONTEXT:WORD
EXTRN WINDOW_HEAD:WORD
.CODE
EXTRN ALLOCATE_WINDOW_MEMORY:FAR
EXTRN INIT_PORT:NEAR
EXTRN HGETRSRC:NEAR
EXTRN RESOLVE_HANDLE_TO_FAR_PAYLOAD:NEAR
EXTRN CREATE_STRING_HANDLE:NEAR
EXTRN INVALIDATE_WINDOW_RECTANGLE:NEAR
EXTRN STROKE_RECTANGLE:NEAR
EXTRN POST_POINTER_EVENT:NEAR
EXTRN DISCARD_QUEUED_EVENTS:NEAR
EXTRN BRING_WINDOW_FORWARD:NEAR
PUBLIC CREATE_WINDOW, INITIALIZE_WINDOW, STROKE_OFFSET_EDGES, ACTIVATE_WINDOW

CREATE_WINDOW PROC FAR
    push bp
    mov bp, sp
    mov ax, 5ch
    push ax
    call ALLOCATE_WINDOW_MEMORY
    add sp, 2
    test ax, ax
    jz create_window_done
    push ax
    push WORD PTR [bp+10h]
    les ax, DWORD PTR [bp+0ch]
    push es
    push ax
    push WORD PTR [bp+0ah]
    les ax, DWORD PTR [bp+6]
    push es
    push ax
    push cs
    call NEAR PTR INITIALIZE_WINDOW
create_window_done:
    pop bp
    retf 12
CREATE_WINDOW ENDP

INITIALIZE_WINDOW PROC FAR
    push bp
    mov bp, sp
    push di
    mov ax, OFFSET WINDOW_LIST
    xchg CURRENT_CONTEXT, ax
    push ax
    mov di, [bp+12h]
    push di
    les ax, DWORD PTR [bp+0ch]
    push es
    push ax
    sub ax, ax
    mov cx, 5ch
    push ds
    pop es
    rep stosb
    push cs
    call INIT_PORT
    xchg di, ax
    mov ax, 5744h
    push ax
    mov ax, 4546h
    push ax
    push WORD PTR [bp+10h]
    push cs
    call HGETRSRC
    push ax
    push cs
    call RESOLVE_HANDLE_TO_FAR_PAYLOAD
    xchg bx, ax
    mov es, dx
    les ax, DWORD PTR es:[bx]
    mov [di+2eh], ax
    mov [di+30h], es
    mov WORD PTR [di+34h], 1
    mov ax, [bp+0ah]
    mov [di+32h], ax
    les bx, DWORD PTR [bp+6]
    mov ax, es
    or ax, bx
    jz store_window_title
    push es
    push bx
    push cs
    call CREATE_STRING_HANDLE
store_window_title:
    mov [di+5ah], ax
    push di
    mov ax, 0
    push ax
    sub ax, ax
    push ax
    call DWORD PTR [di+2eh]
    push di
    mov ax, 1
    push ax
    sub ax, ax
    push ax
    call DWORD PTR [di+2eh]
    mov ax, OFFSET WINDOW_LIST
find_window_tail:
    xchg bx, ax
    mov ax, [bx+2ch]
    test ax, ax
    jne find_window_tail
    mov [bx+2ch], di
    mov [di+2ch], ax
    push di
    push cs
    call NEAR PTR ACTIVATE_WINDOW
    lea ax, [di+3eh]
    push ax
    push di
    push cs
    call INVALIDATE_WINDOW_RECTANGLE
    xchg di, ax
    pop CURRENT_CONTEXT
    pop di
    pop bp
    retf 14
INITIALIZE_WINDOW ENDP

STROKE_OFFSET_EDGES PROC FAR
    push bp
    mov bp, sp
    add sp, -8
    push di
    push si
    mov si, [bp+14]
    lea di, [bp-8]
    mov ax, [si+6]
    mov [di+2], ax
    add ax, [bp+12]
    mov [di+6], ax
    mov ax, [si]
    add ax, [bp+10]
    mov [di], ax
    mov ax, [si+4]
    add ax, [bp+10]
    mov [di+4], ax
    push ds
    push di
    push WORD PTR [bp+8]
    push WORD PTR [bp+6]
    push cs
    call STROKE_RECTANGLE
    mov ax, [si+2]
    mov cx, [bp+12]
    add ax, cx
    mov [di+2], ax
    sub [di+6], cx
    mov ax, [si+4]
    mov [di], ax
    push ds
    push di
    push WORD PTR [bp+8]
    push WORD PTR [bp+6]
    push cs
    call STROKE_RECTANGLE
    xchg si, ax
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 10
STROKE_OFFSET_EDGES ENDP

ACTIVATE_WINDOW PROC FAR
    push bp
    mov bp, sp
    push di
    mov ax, WINDOW_HEAD
    mov di, [bp+6]
    cmp ax, di
    je post_activation
    mov dx, 1
    push dx
    sub dx, dx
    push dx
    push ax
    push cs
    call POST_POINTER_EVENT
post_activation:
    mov ax, 2
    push ax
    push ax
    push cs
    call DISCARD_QUEUED_EVENTS
    sub ax, ax
    push ax
    push di
    push cs
    call POST_POINTER_EVENT
    push di
    push cs
    call BRING_WINDOW_FORWARD
    pop di
    pop bp
    retf 2
ACTIVATE_WINDOW ENDP
END
