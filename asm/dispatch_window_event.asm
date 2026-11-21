.MODEL MEDIUM
EXTRN DEFAULT_ITEM_HANDLER:FAR
.DATA
EXTRN CURRENT_PORT:WORD
.CODE
EXTRN NOTIFY_SELECTED_ITEM:NEAR
EXTRN DISPATCH_LINKED_ITEMS:NEAR
EXTRN PAINT_PENDING_WINDOW:NEAR
EXTRN PAINT_WINDOW_ITEMS:NEAR
EXTRN RESTORE_WINDOW_CLIP:NEAR
EXTRN DISPATCH_WINDOW_KEY:NEAR
EXTRN PLAY_SOUND:NEAR
EXTRN IS_SELECTABLE_ITEM:NEAR
EXTRN SELECT_FOCUSED_ITEM:NEAR
EXTRN SELECTED_ITEM_ORDINAL:NEAR
EXTRN LOCATE_WINDOW_AT_POINT:NEAR
EXTRN ADD_PORT_ORIGIN:NEAR
EXTRN FIND_ITEM_AT_POINT:NEAR
EXTRN DISPATCH_ITEM_CHARACTER:NEAR
PUBLIC DISPATCH_WINDOW_EVENT

DISPATCH_WINDOW_EVENT PROC FAR
    push bp
    mov bp, sp
    add sp, -4
    push di
    push si
    mov ax, [bp+0ah]
    mov di, ax
    xchg CURRENT_PORT, ax
    push ax
    mov si, [bp+8]
    mov ax, [si]
    cmp ax, 2
    je deactivate_items
    cmp ax, 1
    je activate_items
    cmp ax, 8
    je handle_keyboard
    cmp ax, 10h
    je repaint_window
    cmp ax, 0
    je idle_notify
    cmp ax, 4
    jne ignored
    jmp pointer_event
ignored:
    sub ax, ax
finish:
    pop CURRENT_PORT
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 6

idle_notify:
    push di
    push cs
    call NOTIFY_SELECTED_ITEM
    jmp short ignored
deactivate_items:
    mov ax, 1
    jmp short update_items
activate_items:
    sub ax, ax
update_items:
    cmp di, [si+0ah]
    jne ignored
    push di
    push ax
    push cs
    call DISPATCH_LINKED_ITEMS
    jmp short ignored
repaint_window:
    cmp di, [si+0ah]
    jne idle_notify
    push di
    push cs
    call PAINT_PENDING_WINDOW
    push di
    push cs
    call PAINT_WINDOW_ITEMS
    push di
    push cs
    call RESTORE_WINDOW_CLIP
    jmp short ignored
handle_keyboard:
    push di
    lea ax, [si+0ah]
    push ax
    push cs
    call DISPATCH_WINDOW_KEY
    mov si, [si+0ah]
    xchg si, ax
    test si, si
    jne keyboard_item
    sub cx, cx
    cmp al, 0dh
    je keyboard_ignored
    cmp al, 1bh
    xchg cx, ax
    je report_selected
    jcxz ignored
beep:
    mov ax, 0fffeh
    push ax
    push cs
    call PLAY_SOUND
    jmp short ignored
keyboard_ignored:
    sub ax, ax
    jmp short finish
keyboard_item:
    cmp WORD PTR [si+4], OFFSET DEFAULT_ITEM_HANDLER
    jne handle_return
    cmp WORD PTR [si+6], SEG DEFAULT_ITEM_HANDLER
    je report_selected
handle_return:
    cmp al, 0dh
    jne ignored_long
    push si
    mov ax, 1
    push ax
    push cs
    call IS_SELECTABLE_ITEM
    push di
    push ax
    push cs
    call SELECT_FOCUSED_ITEM
ignored_long:
    jmp ignored
report_selected:
    push si
    push cs
    call SELECTED_ITEM_ORDINAL
    mov bx, [bp+6]
    mov [bx], ax
    mov ax, 1
    jmp finish

pointer_event:
    lea ax, [si+2]
    push ax
    lea ax, [bp+0ah]
    push ax
    push cs
    call LOCATE_WINDOW_AT_POINT
    cmp di, [bp+0ah]
    jne beep
    cmp ax, 2
    jne beep
    lea ax, [bp-4]
    push ds
    push ax
    lea ax, [si+2]
    push ds
    push ax
    push cs
    call ADD_PORT_ORIGIN
    push di
    push ax
    push cs
    call FIND_ITEM_AT_POINT
    test ax, ax
    je ignored_long
    push ax
    push WORD PTR [si+0ah]
    xchg si, ax
    push cs
    call DISPATCH_ITEM_CHARACTER
    xchg cx, ax
    jcxz ignored_long
    cmp WORD PTR [si+4], OFFSET DEFAULT_ITEM_HANDLER
    jne pointer_select
    cmp WORD PTR [si+6], SEG DEFAULT_ITEM_HANDLER
    je report_selected
pointer_select:
    push di
    push si
    push cs
    call SELECT_FOCUSED_ITEM
    jmp short ignored_long
DISPATCH_WINDOW_EVENT ENDP
END
