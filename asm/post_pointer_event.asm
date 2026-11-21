.MODEL MEDIUM
.DATA
EXTRN TIMER_TICKS:DWORD
EXTRN EVENT_QUEUE:BYTE
EXTRN EVENT_COUNT:WORD
.CODE
EXTRN READ_MOUSE_EVENT_POSITION:NEAR
PUBLIC LOWERCASE_EVENT_CHARACTER, POST_POINTER_EVENT, INSERT_QUEUED_EVENT

LOWERCASE_EVENT_CHARACTER PROC FAR
    push bp
    mov bp, sp
    mov ax, [bp+6]
    cmp al, 'A'
    jb lowercase_done
    cmp al, 'Z'
    ja lowercase_done
    or al, 20h
lowercase_done:
    pop bp
    retf 2
LOWERCASE_EVENT_CHARACTER ENDP

POST_POINTER_EVENT PROC FAR
    push bp
    mov bp, sp
    les ax, DWORD PTR [bp+6]
    push es
    push ax
    les ax, TIMER_TICKS
    push es
    push ax
    sub sp, 4
    mov ax, sp
    push WORD PTR [bp+10]
    push ax
    push cs
    call READ_MOUSE_EVENT_POSITION
    push cs
    call NEAR PTR INSERT_QUEUED_EVENT
    pop bp
    retf 6
POST_POINTER_EVENT ENDP

INSERT_QUEUED_EVENT PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    mov ax, 14
    mov bx, [bp+6]
    sub dx, dx
    mov di, OFFSET EVENT_QUEUE
    push ds
    pop es
    pushf
    cli
    mov cx, EVENT_COUNT
    jcxz store_event
seek_event_position:
    cmp bx, [di]
    jb make_room
    inc dx
    add di, ax
    loop seek_event_position
    cmp dx, 8
    jb store_event
    popf
    xchg cx, ax
return_inserted:
    pop si
    pop di
    pop bp
    retf 14
make_room:
    add dx, cx
    cmp dx, 8
    jb shift_events
    dec cx
    dec EVENT_COUNT
shift_events:
    push di
    dec di
    mov si, di
    add di, ax
    mul cx
    add di, ax
    add si, ax
    xchg cx, ax
    std
    rep movsb
    cld
    pop di
store_event:
    push ds
    mov cx, 14
    lea si, [bp+6]
    push ss
    pop ds
    rep movsb
    pop ds
    popf
    inc EVENT_COUNT
    mov ax, 1
    jmp SHORT return_inserted
INSERT_QUEUED_EVENT ENDP
END
