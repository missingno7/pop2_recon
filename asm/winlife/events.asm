.MODEL MEDIUM
EXTRN INTERRUPT_DGROUP:BYTE
.DATA
ERROR_CANCEL_HANDLER DD 0
ABORT_REQUESTED DW 0
ERROR_ACTION_LABEL DW 0
event_hook DD 0
saved_vectors DD 0, 0, 0, 0
LAST_INPUT_TICK DW 0
EVENT_QUEUE LABEL BYTE                  ; eight events of seven words
    DW 0, 0, 0, 0, 0, 0, 0
    DW 0, 0, 0, 0, 0, 0, 0
    DW 0, 0, 0, 0, 0, 0, 0
    DW 0, 0, 0, 0, 0, 0, 0
    DW 0, 0, 0, 0, 0, 0, 0
    DW 0, 0, 0, 0, 0, 0, 0
    DW 0, 0, 0, 0, 0, 0, 0
    DW 0, 0, 0, 0, 0, 0, 0
EVENT_COUNT DW 0
EXTRN Graf:WORD
EXTRN MOUSE_PRESENT:WORD
EXTRN SCREEN_WIDTH:WORD
EXTRN TIMER_TICKS:DWORD
EXTRN WINDOW_DRAW_PORT:WORD
EXTRN WINDOW_HEAD:WORD
EXTRN WINDOW_LIST:BYTE
.CODE
EXTRN ACTIVATE_WINDOW:NEAR
EXTRN ALLOCATE_BACKGROUND_PORT:NEAR
EXTRN CREATE_WINDOW_FROM_RESOURCE:NEAR
EXTRN DISPOSE_WINDOW:NEAR
EXTRN ITEM_BY_INDEX:NEAR
EXTRN _sprintf:FAR
EXTRN __NFREE:FAR
EXTRN RELEASE_BACKGROUND_PORT:NEAR
EXTRN LOAD_RESOURCE_COPY:NEAR
EXTRN HANDLE_MOUSE_CALLBACK:FAR
EXTRN LOOKUP_PREFIXED_STRING:NEAR
EXTRN PLAY_SOUND:NEAR
EXTRN POLL_DOS_KEY:FAR
EXTRN SET_ITEM_ENABLED:NEAR
EXTRN SET_WINDOW_EVENT_STATE:NEAR
EXTRN SHOW_MOUSE_GRAPHICS:NEAR
EXTRN FIND_WINDOW_AT_RECT:NEAR
EXTRN WAIT_FOR_WINDOW_RESULT:NEAR
PUBLIC MOUSE_BUTTON_DOWN
PUBLIC TEST_EVENT_PENDING
PUBLIC DISCARD_QUEUED_EVENTS
PUBLIC READ_QUEUED_EVENT
PUBLIC READ_MOUSE_EVENT_POSITION
PUBLIC INSTALL_DOS_VECTORS
PUBLIC LOWERCASE_EVENT_CHARACTER
PUBLIC POST_POINTER_EVENT
PUBLIC INSERT_QUEUED_EVENT
PUBLIC RESTORE_DOS_VECTORS
PUBLIC mouse_button_unqueued
PUBLIC UPPERCASE_EVENT_CHARACTER
PUBLIC FIND_EVENT_WINDOW
PUBLIC NOTIFY_EVENT_HOOK
PUBLIC VIDEO_INTERRUPT_FILTER
PUBLIC KEYBOARD_INTERRUPT_FILTER
PUBLIC DOS_CRITICAL_ERROR
PUBLIC DOS_MOUSE_EVENT
PUBLIC IGNORE_LEGACY_INTERRUPT

MOUSE_BUTTON_DOWN PROC FAR
    push bp
    mov bp, sp
    sub ax, ax
    cmp ax, mouse_present
    je mouse_button_done

    ; Mouse driver function 3 returns the current button bits in BX.
    mov ax, 3
    int 33h
    sub ax, ax
    stc
    mov cl, BYTE PTR [bp+6]
    rcl ax, cl
    and ax, bx
    je mouse_button_done
    mov ax, 1
mouse_button_done:
    pop bp
    retf 2
MOUSE_BUTTON_DOWN ENDP

; Test queued events, then check for keyboard input and pending redraws.
TEST_EVENT_PENDING PROC FAR
    push bp
    mov bp, sp
    mov ax, word ptr [bp + 6]
    mov bx, OFFSET EVENT_QUEUE
    mov cx, word ptr EVENT_COUNT
    jcxz check_pending_keyboard
scan_pending_events:
    test word ptr [bx], ax
    jne pending_event_found
    add bx, 0eh
    loop scan_pending_events
check_pending_keyboard:
    test ax, 8
    je check_pending_redraw
    mov ah, 0bh
    int 21h
    cmp al, 0ffh
    je pending_event_found
check_pending_redraw:
    test ax, 10h
    je no_pending_event
    mov bx, OFFSET WINDOW_LIST
scan_pending_windows:
    mov cx, word ptr [bx + 2ch]
    jcxz no_pending_event
    mov bx, cx
    mov cx, word ptr [bx + 46h]
    jcxz scan_pending_windows
pending_event_found:
    mov ax, 1
return_event_pending:
    pop bp
    retf 2
no_pending_event:
    sub ax, ax
    jmp SHORT return_event_pending
TEST_EVENT_PENDING ENDP

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
discard_next_window:
    mov bx, [bx+2ch]
    test bx, bx
    jz discard_done
    mov WORD PTR [bx+46h], 0
    jmp SHORT discard_next_window
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
    ; The scan advances one record before testing the first event.
    mov si, OFFSET saved_vectors+4
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
    call POLL_DOS_KEY
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
    call NEAR PTR NOTIFY_EVENT_HOOK
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
    call NEAR PTR FIND_EVENT_WINDOW
    jcxz no_event
    sub ax, ax
    jmp SHORT update_window
window_released:
    call NEAR PTR FIND_EVENT_WINDOW
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
    call NEAR PTR READ_MOUSE_EVENT_POSITION
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
    call NEAR PTR MOUSE_BUTTON_DOWN
    test ax, ax
    je unqueued_button_done
    mov cx, event_count
    jcxz unqueued_button_done
    mov bx, OFFSET EVENT_QUEUE
    mov dx, WORD PTR [bp+6]
next_record:
    cmp WORD PTR [bx], 4
    jne advance
    cmp dx, WORD PTR [bx+10]
    jne advance
    sub ax, ax
    jmp short unqueued_button_done
advance:
    add bx, 14
    loop next_record
unqueued_button_done:
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
find_next_event_window:
    mov bx, cx
    mov cx, [bx+2ch]
    jcxz lookup_done
    cmp cx, [di+0ah]
    jne find_next_event_window
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
    xchg Graf, ax
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
    call FIND_WINDOW_AT_RECT
    cmc
    sbb ax, ax
    je save_background
    lea ax, [di+3eh]
    call ALLOCATE_BACKGROUND_PORT
save_background:
    mov [bp-4ah], ax
    push di
    mov ax, 2
    push ax
    push cs
    call NEAR PTR ITEM_BY_INDEX
    push ax
    push ERROR_ACTION_LABEL
    push cs
    call SET_ITEM_ENABLED
    mov ax, WORD PTR ERROR_CANCEL_HANDLER
    or ax, WORD PTR ERROR_CANCEL_HANDLER+2
    jne classify_error
    push di
    mov ax, 1
    push ax
    push cs
    call NEAR PTR ITEM_BY_INDEX
    push ax
    sub ax, ax
    push ax
    push cs
    call SET_ITEM_ENABLED

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
    call LOAD_RESOURCE_COPY
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
    call _sprintf
    add sp, 6
    call __NFREE
    add sp, 2
    push di
    mov ax, 4
    push ax
    push cs
    call NEAR PTR ITEM_BY_INDEX
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
    call RELEASE_BACKGROUND_PORT
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
    pop Graf
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
    les ax, TIMER_TICKS
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
