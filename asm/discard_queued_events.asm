.MODEL MEDIUM
.DATA
EXTRN EVENT_QUEUE:BYTE
EXTRN EVENT_COUNT:WORD
EXTRN WINDOW_LIST:BYTE
EXTRN QUEUE_SCAN_ORIGIN:BYTE
EXTRN LAST_INPUT_TICK:WORD
EXTRN TIMER_TICKS:DWORD
EXTRN MOUSE_PRESENT:WORD
EXTRN SCREEN_WIDTH:WORD
.CODE
EXTRN READ_INPUT:FAR
EXTRN NOTIFY_EVENT_HOOK:NEAR
EXTRN FIND_EVENT_WINDOW:NEAR
EXTRN SET_WINDOW_EVENT_STATE:NEAR
PUBLIC DISCARD_QUEUED_EVENTS, READ_QUEUED_EVENT, READ_MOUSE_EVENT_POSITION

DISCARD_QUEUED_EVENTS PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    pushf
    cli
    mov di, OFFSET EVENT_QUEUE
    mov cx, EVENT_COUNT
scan_events:
    jcxz finish_queue
    dec cx
    mov ax, [di]
    test [bp+6], ax
    jnz remove_event
    add di, 14
    jmp SHORT scan_events
remove_event:
    push cx
    push di
    mov ax, 14
    mul cx
    xchg cx, ax
    lea si, [di+14]
    push ds
    pop es
    rep movsb
    pop di
    pop cx
    dec EVENT_COUNT
    jmp SHORT scan_events
finish_queue:
    popf
    test WORD PTR [bp+6], 8
    jz test_windows
    mov ax, 0c06h
    mov dl, 0ffh
    int 21h
test_windows:
    test WORD PTR [bp+6], 10h
    jz discard_done
    mov bx, OFFSET WINDOW_LIST
next_window:
    mov bx, [bx+2ch]
    test bx, bx
    jz discard_done
    mov WORD PTR [bx+46h], 0
    jmp SHORT next_window
discard_done:
    pop si
    pop di
    pop bp
    retf 2
DISCARD_QUEUED_EVENTS ENDP

READ_QUEUED_EVENT PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    mov ax, [bp+6]
    mov di, [bp+8]
    pushf
    cli
    mov cx, EVENT_COUNT
    jcxz poll_event_sources
    mov bx, 14
    mov si, OFFSET QUEUE_SCAN_ORIGIN
scan_for_match:
    add si, bx
    test [si], ax
    loope scan_for_match
    je poll_event_sources
    push ds
    pop es
    xchg cx, ax
    mov cx, bx
    rep movsb
    dec EVENT_COUNT
    mov di, [bp+8]
    mul bx
    xchg cx, ax
    jcxz queue_read
    lea di, [si-14]
    rep movsb
queue_read:
    popf
    mov di, [bp+8]
    jmp SHORT dispatch_event
poll_event_sources:
    popf
    mov [di+0ah], cx
    mov [di+0ch], cx
    test ax, 8
    jz poll_windows
    mov ax, WORD PTR TIMER_TICKS
    xchg LAST_INPUT_TICK, ax
    cmp ax, LAST_INPUT_TICK
    je poll_windows
    call READ_INPUT
    xchg cx, ax
    jcxz poll_windows
    mov WORD PTR [di], 8
    mov [di+0ah], cx
    jmp SHORT finish_polled_event
poll_windows:
    test WORD PTR [bp+6], 10h
    jz no_event
    sub ax, ax
    mov si, OFFSET WINDOW_LIST
find_pending_window:
    mov si, [si+2ch]
    test si, si
    jz check_pending_window
    mov cx, [si+46h]
    jcxz find_pending_window
    mov ax, si
    jmp SHORT find_pending_window
check_pending_window:
    xchg cx, ax
    jcxz no_event
    mov WORD PTR [di], 10h
    mov [di+0ah], cx
    jmp SHORT finish_polled_event
no_event:
    mov WORD PTR [di], 0
finish_polled_event:
    les ax, TIMER_TICKS
    mov [di+8], es
    mov [di+6], ax
    lea ax, [di+2]
    push ax
    push cs
    call NEAR PTR READ_MOUSE_EVENT_POSITION
dispatch_event:
    call NOTIFY_EVENT_HOOK
    sub ax, ax
    cmp WORD PTR [di], 0
    je read_event_done
    cmp WORD PTR [di], 1
    je window_pressed
    cmp WORD PTR [di], 2
    je window_released
event_ready:
    mov ax, 1
read_event_done:
    pop si
    pop di
    pop bp
    retf 4
window_pressed:
    call FIND_EVENT_WINDOW
    jcxz no_event
    sub ax, ax
    jmp SHORT update_window
window_released:
    call FIND_EVENT_WINDOW
    jcxz no_event
    mov ax, 1
update_window:
    push WORD PTR [di+0ah]
    push ax
    push cs
    call SET_WINDOW_EVENT_STATE
    jmp SHORT event_ready
READ_QUEUED_EVENT ENDP

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
