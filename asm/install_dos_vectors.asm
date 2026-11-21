.MODEL MEDIUM
EXTRN INTERRUPT_DGROUP:BYTE
.DATA
event_hook DD 0
saved_vectors DD 0, 0, 0, 0
EXTRN TIMER_TICKS:DWORD
EXTRN EVENT_QUEUE:BYTE
EXTRN EVENT_COUNT:WORD
EXTRN MOUSE_PRESENT:WORD
EVENT_RECORDS EQU EVENT_QUEUE
EXTRN WINDOW_LIST:BYTE
EXTRN CURRENT_PORT:WORD
EXTRN WINDOW_DRAW_PORT:WORD
EXTRN ERROR_CANCEL_HANDLER:DWORD
EXTRN ERROR_ACTION_LABEL:WORD
EXTRN WINDOW_HEAD:WORD
EXTRN ABORT_REQUESTED:WORD
EXTRN MOUSE_EVENT_TARGET:DWORD
EXTRN SCREEN_WIDTH:WORD

.CODE
EXTRN READ_MOUSE_EVENT_POSITION:NEAR
EXTRN MOUSE_BUTTON_DOWN:NEAR
EXTRN SHOW_MOUSE_GRAPHICS:NEAR
EXTRN CREATE_WINDOW_FROM_RESOURCE:NEAR
EXTRN TEST_BACKGROUND_PORT:NEAR
EXTRN FIND_EVENT_ITEM:NEAR
EXTRN ALLOCATE_WINDOW_DRAW_PORT:NEAR
EXTRN SET_ITEM_LABEL:NEAR
EXTRN GET_TAGGED_RESOURCE:NEAR
EXTRN LOOKUP_PREFIXED_STRING:NEAR
EXTRN FORMAT_ERROR_DETAIL:FAR
EXTRN FREE_NEAR:FAR
EXTRN PLAY_SOUND:NEAR
EXTRN WAIT_FOR_WINDOW_RESULT:NEAR
EXTRN FREE_WINDOW_DRAW_PORT:NEAR
EXTRN ACTIVATE_WINDOW:NEAR
EXTRN DISPOSE_WINDOW:NEAR
EXTRN HANDLE_MOUSE_CALLBACK:FAR
PUBLIC INSTALL_DOS_VECTORS, VIDEO_INTERRUPT_FILTER, KEYBOARD_INTERRUPT_FILTER
PUBLIC DOS_CRITICAL_ERROR, DOS_MOUSE_EVENT, IGNORE_LEGACY_INTERRUPT
PUBLIC LOWERCASE_EVENT_CHARACTER, POST_POINTER_EVENT, INSERT_QUEUED_EVENT
PUBLIC RESTORE_DOS_VECTORS, MOUSE_BUTTON_UNQUEUED
PUBLIC UPPERCASE_EVENT_CHARACTER, FIND_EVENT_WINDOW, NOTIFY_EVENT_HOOK

INSTALL_DOS_VECTORS PROC FAR
    mov ax, 3505h
    int 21h
    mov WORD PTR saved_vectors, bx
    mov WORD PTR saved_vectors+2, es
    mov ax, 3510h
    int 21h
    mov WORD PTR saved_vectors+4, bx
    mov WORD PTR saved_vectors+6, es
    mov ax, 3516h
    int 21h
    mov WORD PTR saved_vectors+8, bx
    mov WORD PTR saved_vectors+10, es
    mov ax, 3524h
    int 21h
    mov WORD PTR saved_vectors+12, bx
    mov WORD PTR saved_vectors+14, es

    push ds
    push cs
    pop ds
    mov dx, OFFSET IGNORE_LEGACY_INTERRUPT
    mov ax, 2505h
    int 21h
    mov dx, OFFSET VIDEO_INTERRUPT_FILTER
    mov ax, 2510h
    int 21h
    mov dx, OFFSET KEYBOARD_INTERRUPT_FILTER
    mov ax, 2516h
    int 21h
    mov dx, OFFSET IGNORE_LEGACY_INTERRUPT
    mov ax, 2523h
    int 21h
    mov dx, OFFSET DOS_CRITICAL_ERROR
    mov ax, 2524h
    int 21h
    pop ds

    mov cx, MOUSE_PRESENT
    jcxz installed
    push cs
    pop es
    mov cx, 1fh
    mov dx, OFFSET DOS_MOUSE_EVENT
    mov ax, 0ch
    int 33h
    push cs
    call SHOW_MOUSE_GRAPHICS
installed:
    retf
INSTALL_DOS_VECTORS ENDP

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

RESTORE_DOS_VECTORS PROC FAR
    push ds
    lds dx, saved_vectors
    mov ax, 2510h
    int 21h
    lds dx, DWORD PTR ss:[saved_vectors+4]
    mov ax, 2510h
    int 21h
    lds dx, DWORD PTR ss:[saved_vectors+8]
    mov ax, 2516h
    int 21h
    lds dx, DWORD PTR ss:[saved_vectors+12]
    mov ax, 2524h
    int 21h
    mov cx, ss:[MOUSE_PRESENT]
    jcxz vectors_done
    sub ax, ax
    int 33h
vectors_done:
    pop ds
    retf
RESTORE_DOS_VECTORS ENDP

mouse_button_unqueued PROC FAR
    push bp
    mov bp, sp
    push WORD PTR [bp+6]
    push cs
    call mouse_button_down
    test ax, ax
    je done
    mov cx, event_count
    jcxz done
    mov bx, OFFSET event_records
    mov dx, WORD PTR [bp+6]
next_record:
    cmp WORD PTR [bx], 4
    jne advance
    cmp dx, WORD PTR [bx+10]
    jne advance
    sub ax, ax
    jmp short done
advance:
    add bx, 14
    loop next_record
done:
    pop bp
    retf 2
mouse_button_unqueued ENDP

UPPERCASE_EVENT_CHARACTER PROC FAR
    push bp
    mov bp, sp
    mov ax, [bp+6]
    cmp al, 'a'
    jb character_ready
    cmp al, 'z'
    ja character_ready
    and al, 0dfh
character_ready:
    pop bp
    retf 2
UPPERCASE_EVENT_CHARACTER ENDP

FIND_EVENT_WINDOW PROC NEAR
    mov cx, OFFSET WINDOW_LIST
next_window:
    mov bx, cx
    mov cx, [bx+2ch]
    jcxz lookup_done
    cmp cx, [di+0ah]
    jne next_window
lookup_done:
    ret
FIND_EVENT_WINDOW ENDP

NOTIFY_EVENT_HOOK PROC NEAR
    mov ax, WORD PTR event_hook
    or ax, WORD PTR [event_hook+2]
    jz notification_done
    push di
    call event_hook
notification_done:
    ret
NOTIFY_EVENT_HOOK ENDP

VIDEO_INTERRUPT_FILTER PROC FAR
    push ds
    push ax
    mov ax, SEG INTERRUPT_DGROUP
    mov ds, ax
    pop ax
    cmp ah, 0eh
    je video_done
    pushf
    call DWORD PTR saved_vectors+4
video_done:
    pop ds
    iret
VIDEO_INTERRUPT_FILTER ENDP

KEYBOARD_INTERRUPT_FILTER PROC FAR
    push ds
    push ax
    mov ax, SEG INTERRUPT_DGROUP
    mov ds, ax
    pop ax
    test ah, ah
    je read_key
    cmp ah, 0ah
    jne chain_keyboard
read_key:
    push ax
    or ah, 1
    pushf
    call DWORD PTR saved_vectors+8
    mov ax, 0
    je keyboard_done
    pop ax
    push ax
    pushf
    call DWORD PTR saved_vectors+8
keyboard_done:
    add sp, 2
    pop ds
    iret
chain_keyboard:
    pop WORD PTR cs:saved_keyboard_ds
    push WORD PTR saved_vectors+10
    push WORD PTR saved_vectors+8
    mov ds, cs:saved_keyboard_ds
    retf
saved_keyboard_ds DW 0
KEYBOARD_INTERRUPT_FILTER ENDP

DOS_CRITICAL_ERROR PROC FAR
    sti
    push ds
    push es
    push bx
    push cx
    push dx
    push di
    push si
    push ax
    mov dx, bp
    push bp
    mov bp, sp
    add sp, -4ah
    push ss
    pop ds
    mov [bp-48h], ax
    mov [bp-46h], di
    mov [bp-44h], si
    mov [bp-42h], dx
    mov ax, WINDOW_DRAW_PORT
    xchg CURRENT_PORT, ax
    push ax
open_error_window:
    mov ax, 0ff9ch
    push ax
    push cs
    call CREATE_WINDOW_FROM_RESOURCE
    xchg di, ax
    lea bx, [di+3eh]
    push ds
    pop es
    mov ax, [di+2ch]
    call TEST_BACKGROUND_PORT
    cmc
    sbb ax, ax
    je save_background
    lea ax, [di+3eh]
    call ALLOCATE_WINDOW_DRAW_PORT
save_background:
    mov [bp-4ah], ax
    push di
    mov ax, 2
    push ax
    push cs
    call NEAR PTR FIND_EVENT_ITEM
    push ax
    push ERROR_ACTION_LABEL
    push cs
    call SET_ITEM_LABEL
    mov ax, WORD PTR ERROR_CANCEL_HANDLER
    or ax, WORD PTR ERROR_CANCEL_HANDLER+2
    jne classify_error
    push di
    mov ax, 1
    push ax
    push cs
    call NEAR PTR FIND_EVENT_ITEM
    push ax
    sub ax, ax
    push ax
    push cs
    call SET_ITEM_LABEL

classify_error:
    mov al, [bp-46h]
    test BYTE PTR [bp-47h], 80h
    je disk_error
    les bx, [bp-44h]
    test BYTE PTR es:[bx+5], 80h
    je character_device
    cmp al, 9
    je unsupported_device
    mov si, 4
    jmp short load_error_resource
unsupported_device:
    mov si, 5
    jmp short load_error_resource
character_device:
    mov si, 6
    jmp short load_error_resource
disk_error:
    cmp al, 0
    je disk_protected
    cmp al, 2
    je disk_missing
    mov si, 1
    jmp short load_error_resource
disk_missing:
    mov si, 2
    jmp short load_error_resource
disk_protected:
    mov si, 3
load_error_resource:
    mov ax, 5354h
    push ax
    mov ax, 524ch
    push ax
    mov ax, 0ff9ch
    push ax
    push cs
    call GET_TAGGED_RESOURCE
    push ax
    push ds
    push ax
    push si
    push cs
    call LOOKUP_PREFIXED_STRING
    mov dx, 41h
    add dl, [bp-48h]
    push dx
    push ax
    lea si, [bp-40h]
    push si
    call FORMAT_ERROR_DETAIL
    add sp, 6
    call FREE_NEAR
    add sp, 2
    push di
    mov ax, 4
    push ax
    push cs
    call NEAR PTR FIND_EVENT_ITEM
    xchg bx, ax
    push bx
    mov ax, 9
    push ax
    push si
    call DWORD PTR [bx+4]
wait_for_action:
    mov ax, 0ffffh
    push ax
    push cs
    call PLAY_SOUND
    sub ax, ax
    push ax
    push ax
    push cs
    call WAIT_FOR_WINDOW_RESULT
    xchg cx, ax
    jcxz wait_for_action
    push cx
    mov cx, [bp-4ah]
    jcxz close_error_window
    xchg cx, ax
    call FREE_WINDOW_DRAW_PORT
    sub cx, cx
    xchg [di+2ch], cx
    mov WINDOW_HEAD, cx
    jcxz close_error_window
    push cx
    push cs
    call ACTIVATE_WINDOW
close_error_window:
    push di
    push cs
    call DISPOSE_WINDOW
    pop ax
    cmp ax, 1
    je retry_callback
    mov dl, 1
    cmp ax, 2
    jne return_status
    mov dl, 3
    mov ABORT_REQUESTED, 1
return_status:
    pop CURRENT_PORT
    mov sp, bp
    pop bp
    pop ax
    mov al, dl
    pop si
    pop di
    pop dx
    pop cx
    pop bx
    pop es
    pop ds
    iret
retry_callback:
    mov ah, 30h
    int 21h
    call ERROR_CANCEL_HANDLER
    jmp open_error_window
DOS_CRITICAL_ERROR ENDP

DOS_MOUSE_EVENT PROC FAR
    call HANDLE_MOUSE_CALLBACK
    test al, 0ah
    je mouse_event_done
    push ds
    push es
    push ax
    push bx
    push cx
    push dx
    cld
    mov bx, SEG INTERRUPT_DGROUP
    mov ds, bx
    mov bx, 1
    test al, 2
    jne enqueue_mouse_event
    inc bx
enqueue_mouse_event:
    sub ax, ax
    push ax
    push bx
    les ax, MOUSE_EVENT_TARGET
    push es
    push ax
    cmp SCREEN_WIDTH, 140h
    jne add_mouse_position
    shr cx, 1
add_mouse_position:
    push cx
    push dx
    mov ax, 4
    push ax
    push cs
    call NEAR PTR INSERT_QUEUED_EVENT
    pop dx
    pop cx
    pop bx
    pop ax
    pop es
    pop ds
mouse_event_done:
    retf
DOS_MOUSE_EVENT ENDP

IGNORE_LEGACY_INTERRUPT PROC FAR
    iret
IGNORE_LEGACY_INTERRUPT ENDP
END
