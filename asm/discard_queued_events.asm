.MODEL MEDIUM
.DATA
EXTRN EVENT_QUEUE:BYTE
EXTRN EVENT_COUNT:WORD
EXTRN WINDOW_LIST:BYTE
.CODE
PUBLIC DISCARD_QUEUED_EVENTS

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
END
