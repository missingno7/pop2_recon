; DOS window and item dispatch, in original contribution order.
; Procedure names are descriptive. Public fill labels delimit original alignment bytes.
.MODEL MEDIUM

.DATA
EXTRN Graf:WORD
EXTRN screen_row_origin:WORD
EXTRN screen_x_origin:WORD
EXTRN CURRENT_GRAPHICS_FONT:WORD
EXTRN TIMER_RELOAD:WORD
EXTRN TIMER_TICKS:WORD
EXTRN WINDOW_HEAD:WORD
EXTRN window_draw_port:WORD

.CODE
EXTRN UPDATE_MESSAGE_DISPLAY:NEAR
EXTRN SHOW_MOUSE_GRAPHICS:NEAR
EXTRN RESOLVE_HANDLE_TO_FAR_PAYLOAD:NEAR
EXTRN DISPOSE_HANDLE:NEAR
EXTRN SAVE_HANDLE_FLAGS:NEAR
EXTRN FETCH_LOCKED_SHAPE_LIST:NEAR
EXTRN RESTORE_HANDLE_FLAGS:NEAR
EXTRN FINISH_PORT:NEAR
EXTRN PUSH_PORT_CLIP:NEAR
EXTRN POP_PORT_CLIP:NEAR
EXTRN DRAW_NUL_TERMINATED_TEXT:NEAR
EXTRN STROKE_PORT_RECT:NEAR
EXTRN FRAME_RECT:NEAR
EXTRN COPY_CURRENT_PORT_ORIGIN:NEAR
EXTRN INSETRECT:NEAR
EXTRN DRAW_AXIS_SEGMENT:NEAR
EXTRN MOVETO:NEAR
EXTRN SECTRECT:NEAR
EXTRN SET_RELATIVE_PEN_STATE:NEAR
EXTRN MEASURE_NUL_TERMINATED_TEXT:NEAR
EXTRN MEASURE_COUNTED_TEXT:NEAR
EXTRN RELEASE_BACKGROUND_PORT:NEAR
EXTRN ALLOCATE_BACKGROUND_PORT:NEAR
EXTRN DISPOSE_WINDOW:NEAR
EXTRN ACTIVATE_WINDOW:NEAR
EXTRN SET_RECORD_RANGE:NEAR
EXTRN HGETRSRC:NEAR
EXTRN PLAY_SOUND:NEAR
EXTRN PAINT_PENDING_WINDOW:NEAR
EXTRN RESTORE_WINDOW_CLIP:NEAR
EXTRN LOCATE_WINDOW_AT_POINT:NEAR
EXTRN CREATE_WINDOW:NEAR
EXTRN STROKE_OFFSET_EDGES:NEAR
EXTRN READ_QUEUED_EVENT:NEAR
EXTRN READ_MOUSE_EVENT_POSITION:NEAR
EXTRN MOUSE_BUTTON_UNQUEUED:NEAR
EXTRN UPPERCASE_EVENT_CHARACTER:NEAR
EXTRN DRAW_CHARACTER:NEAR
EXTRN ADD_PORT_ORIGIN:NEAR
EXTRN TOGGLE_RECT_EFFECT:NEAR
EXTRN APPLY_DEFAULT_PEN_STATE:NEAR
EXTRN POINT_IN_RECTANGLE:NEAR
EXTRN draw_text_handle:NEAR
EXTRN CREATE_STRING_HANDLE:NEAR
EXTRN measure_text_handle:NEAR
EXTRN __nfree:FAR
EXTRN __nmalloc:FAR
EXTRN __fstrcmp:FAR

PUBLIC DEFAULT_ITEM_HANDLER
PUBLIC DISPATCH_LINKED_ITEMS
PUBLIC SET_ITEM_ENABLED
PUBLIC NOTIFY_SELECTED_ITEM
PUBLIC SELECTED_ITEM_ORDINAL
PUBLIC DISPATCH_WINDOW_KEY
PUBLIC PAINT_WINDOW_ITEMS
PUBLIC FIND_ITEM_AT_POINT
PUBLIC DISPOSE_WINDOW_ITEM
PUBLIC RELEASE_WINDOW_ITEM_EFFECT
PUBLIC ITEM_BY_INDEX
PUBLIC LOAD_ITEM_DEFINITION
PUBLIC LOAD_ITEM_LIST
PUBLIC STEP_SELECTABLE_ITEM
PUBLIC SET_SELECTED_ITEM
PUBLIC SELECT_ITEM_BY_INDEX
PUBLIC SHOW_WINDOW_ITEM
PUBLIC DISPATCH_ITEM_CHARACTER
PUBLIC STROKE_ITEM_EDGES
PUBLIC FILL_BEFORE_MODAL_WINDOW
PUBLIC RUN_MODAL_RESOURCE_WINDOW
PUBLIC DISPATCH_WINDOW_EVENT
PUBLIC REPAINT_WINDOW_ITEM
PUBLIC CREATE_WINDOW_FROM_RESOURCE
PUBLIC WAIT_FOR_WINDOW_RESULT
PUBLIC CREATE_WINDOW_FROM_DEFINITION
PUBLIC FIND_WINDOW_AT_RECT
PUBLIC WINDOW_FRAME_ITEM_HANDLER
PUBLIC LIST_ITEM_HANDLER
PUBLIC SHOW_LIST_CURSOR
PUBLIC HIDE_LIST_CURSOR
PUBLIC TOGGLE_LIST_CURSOR
PUBLIC REPAINT_LIST_ITEM
PUBLIC REBUILD_LIST_TEXT_HANDLE
PUBLIC COPY_LIST_TEXT
PUBLIC FILL_BEFORE_GRID_HANDLER
PUBLIC GRID_ITEM_HANDLER
PUBLIC UPDATE_GRID_ITEM_BOUNDS

; Far item callback. BP+8 selects a command; BP+6 carries its payload.
; Creation joins the shared register restore without saving a port.
DEFAULT_ITEM_HANDLER PROC FAR
    push bp
    mov bp, sp
    add sp, -16h
    push di
    push si
    mov di, word ptr [bp+0ah]
    mov si, word ptr [bp+8]
    cmp si, 0
    je construct_item
    mov bx, word ptr [di+2]
    mov ax, CURRENT_GRAPHICS_FONT
    xchg word ptr [bx+2ah], ax
    xchg Graf, bx
    push ax
    push bx
    lea ax, [bp-0eh]
    push ds
    push ax
    push ds
    push ax
    push cs
    call COPY_CURRENT_PORT_ORIGIN
    push cs
    call APPLY_DEFAULT_PEN_STATE
    mov cx, word ptr [bp+6]
    xchg si, ax
    cmp ax, 2
    je paint_item
    cmp ax, 7
    je return_character
    cmp ax, 6
    je track_item
    cmp ax, 8
    je release_item_text
    cmp ax, 9
    je replace_item_text
ignore_item_command:
    sub ax, ax
restore_item_port:
    xchg si, ax
    push cs
    call SET_RELATIVE_PEN_STATE
    xchg si, ax
    pop bx
    xchg Graf, bx
    pop word ptr [bx+2ah]
return_item_result:
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 6
construct_item:
    jmp NEAR PTR allocate_item_record
track_item:
    jmp NEAR PTR track_item_press
return_character:
    xchg cx, ax
    jmp SHORT restore_item_port
release_item_text:
    push word ptr [di+1eh]
    push cs
    call DISPOSE_HANDLE
    jmp SHORT ignore_item_command
replace_item_text:
    push ds
    push cx
    push word ptr [di+1eh]
    push cs
    call DISPOSE_HANDLE
    push cs
    call CREATE_STRING_HANDLE
    mov word ptr [di+1eh], ax
    mov cx, word ptr [di+1ch]
    jcxz ignore_item_command
paint_item:
    mov bx, word ptr [di+2]
    mov ax, 7
    mov dx, word ptr [bx+20h]
    mov cx, word ptr [di+1ah]
    jcxz set_item_pen
    mov ax, word ptr [di+18h]
    mov dx, word ptr [di+16h]
set_item_pen:
    mov word ptr [bx+28h], ax
    lea si, [di+0ch]
    push ds
    push si
    push dx
    push cs
    call SET_RECORD_RANGE
    push ds
    push si
    push cs
    call FRAME_RECT
    lea ax, [bp-16h]
    push ds
    push ax
    push ds
    push si
    mov ax, 2
    push ax
    sub ax, ax
    push ax
    push cs
    call INSETRECT
    push ds
    push ax
    push cs
    call PUSH_PORT_CLIP
    mov cx, word ptr [di+1eh]
    jcxz finish_item_paint
    push cx
    push cs
    call measure_text_handle
    push ax
    mov bx, Graf
    push word ptr [bx+2ah]
    push cs
    call RESOLVE_HANDLE_TO_FAR_PAYLOAD
    xchg bx, ax
    mov es, dx
    pop ax
    mov dx, word ptr [bp-12h]
    mov cx, dx
    sub cx, word ptr [bp-16h]
    shr cx, 1
    sbb dx, cx
    mov cx, word ptr es:[bx+2]
    add cx, word ptr es:[bx+4]
    shr cx, 1
    adc dx, cx
    sub dx, word ptr es:[bx+4]
    mov bx, word ptr [bp-14h]
    mov cx, word ptr [bp-10h]
    sub cx, bx
    shr cx, 1
    add cx, bx
    shr ax, 1
    sub cx, ax
    push cx
    push dx
    push cs
    call MOVETO
    push word ptr [di+1eh]
    push cs
    call RESOLVE_HANDLE_TO_FAR_PAYLOAD
    xchg si, ax
    mov es, dx
    mov cx, word ptr [di+1ah]
    jcxz draw_remaining_text
    push es
    push word ptr [di+14h]
    push cs
    call UPPERCASE_EVENT_CHARACTER
    pop es
    xchg cx, ax
    jcxz draw_remaining_text
scan_mnemonic:
    lods BYTE PTR es:[si]
    test al, al
    je finish_item_paint
    mov dx, ax
    and al, 0dfh
    cmp al, cl
    je draw_mnemonic
    push es
    push dx
    push cs
    call DRAW_CHARACTER
    pop es
    jmp SHORT scan_mnemonic
finish_item_paint:
    push cs
    call POP_PORT_CLIP
    jmp NEAR PTR ignore_item_command
draw_mnemonic:
    mov di, Graf
    mov ax, 4
    xchg word ptr [di+28h], ax
    push ax
    push es
    push dx
    push cs
    call DRAW_CHARACTER
    pop es
    pop word ptr [di+28h]
draw_remaining_text:
    push es
    push si
    push cs
    call DRAW_NUL_TERMINATED_TEXT
    jmp SHORT finish_item_paint
track_item_press:
    mov word ptr [bp-2], 0
poll_item_press:
    lea si, [bp-6]
    push si
    push cs
    call READ_MOUSE_EVENT_POSITION
    push ds
    push si
    push ds
    push si
    push cs
    call ADD_PORT_ORIGIN
    push ds
    push si
    lea ax, [bp-16h]
    push ds
    push ax
    push ds
    push ax
    lea si, [di+0ch]
    push ds
    push si
    mov cx, word ptr [bp-2]
    jcxz inset_press_rect
    mov cx, 0fffeh
inset_press_rect:
    push cx
    push cx
    push cs
    call INSETRECT
    push cs
    call POINT_IN_RECTANGLE
    xchg word ptr [bp-2], ax
    cmp ax, word ptr [bp-2]
    je invoke_press_action
    push ds
    push si
    push cs
    call TOGGLE_RECT_EFFECT
invoke_press_action:
    mov ax, word ptr [di+8]
    or ax, word ptr [di+0ah]
    je check_button_release
    push di
    call DWORD PTR [di+8]
check_button_release:
    push word ptr [bp+6]
    push cs
    call MOUSE_BUTTON_UNQUEUED
    test ax, ax
    jne poll_item_press
    mov cx, word ptr [bp-2]
    jcxz return_press_result
    push ds
    push si
    push cs
    call TOGGLE_RECT_EFFECT
return_press_result:
    mov ax, word ptr [bp-2]
    jmp NEAR PTR restore_item_port
allocate_item_record:
    mov ax, 20h
    push ax
    call __nmalloc
    add sp, 2
    test ax, ax
    je finish_item_creation
    xchg dx, ax
    mov di, dx
    sub ax, ax
    push ds
    pop es
    mov cx, 20h
    rep stosb
    mov di, dx
    mov bx, word ptr [bp+6]
    lds si, [bx]
    lodsw
    mov cx, 8
    add di, 0ch
    rep movsb
    mov di, dx
    push ds
    push es
    pop ds
    pop es
    mov word ptr [di+4], OFFSET DEFAULT_ITEM_HANDLER
    mov word ptr [di+6], cs
    mov ax, 1
    mov word ptr [di+1ah], ax
    mov word ptr [di+1ch], ax
    mov word ptr [di+16h], 0eh
    lods WORD PTR es:[si]
    mov word ptr [di+14h], ax
    push es
    push si
    sub al, al
    mov cx, 0ffffh
    xchg di, si
    repne scasb
    mov word ptr [bx], di
    push cs
    call CREATE_STRING_HANDLE
    mov word ptr [si+1eh], ax
    xchg si, ax
finish_item_creation:
    jmp NEAR PTR return_item_result
DEFAULT_ITEM_HANDLER ENDP

DISPATCH_LINKED_ITEMS PROC FAR
    push bp
    mov bp, sp
    push di
    mov di, [bp+8]
    lea di, [di+52h]
next_item:
    mov di, [di]
    test di, di
    jz linked_items_done
    push di
    mov ax, 3
    push ax
    push WORD PTR [bp+6]
    call DWORD PTR [di+4]
    jmp SHORT next_item
linked_items_done:
    pop di
    pop bp
    retf 4
DISPATCH_LINKED_ITEMS ENDP

; Update the enable word and drop focus when a selected item is disabled.
SET_ITEM_ENABLED PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    mov di, word ptr [bp+8]
    mov si, word ptr [bp+6]
    xchg word ptr [di+1ah], si
    cmp si, word ptr [di+1ah]
    je return_enabled_item
    push di
    mov ax, 2
    push ax
    sub si, si
    push si
    call DWORD PTR [di+4]
    cmp si, word ptr [di+1ah]
    jne return_enabled_item
    mov bx, word ptr [di+2]
    cmp di, word ptr [bx+52h]
    jne return_enabled_item
    push bx
    push si
    push cs
    call NEAR PTR SET_SELECTED_ITEM
return_enabled_item:
    xchg di, ax
    pop si
    pop di
    pop bp
    retf 4
SET_ITEM_ENABLED ENDP

NOTIFY_SELECTED_ITEM PROC FAR
    push bp
    mov bp, sp
    mov bx, [bp+6]
    mov bx, [bx+52h]
    test bx, bx
    jz notify_done
    push bx
    mov ax, 5
    push ax
    sub ax, ax
    push ax
    call DWORD PTR [bx+4]
notify_done:
    pop bp
    retf 2
NOTIFY_SELECTED_ITEM ENDP

SELECTED_ITEM_ORDINAL PROC FAR
    push bp
    mov bp, sp
    sub ax, ax
    mov cx, [bp+6]
    jcxz ordinal_done
    mov bx, cx
    mov bx, [bx+2]
    lea bx, [bx+50h]
count_item:
    inc ax
    mov bx, [bx]
    cmp bx, cx
    jne count_item
ordinal_done:
    pop bp
    retf 2
SELECTED_ITEM_ORDINAL ENDP

DISPATCH_WINDOW_KEY PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    mov di, [bp+6]
    mov si, [bp+8]
    mov si, [si+52h]
    test si, si
    jz scan_accelerators
    push si
    mov ax, 7
    push ax
    push WORD PTR [di]
    call DWORD PTR [si+4]
    mov [di], ax
    cmp al, 0dh
    je window_key_done
    mov dx, 1
    cmp al, 9
    je step_selection
    neg dx
    cmp ax, 0f00h
    jne scan_accelerators
step_selection:
    push si
    push dx
    push cs
    call NEAR PTR STEP_SELECTABLE_ITEM
    push WORD PTR [si+2]
    push ax
    push cs
    call NEAR PTR SET_SELECTED_ITEM
    sub si, si
    mov [di], si
    jmp SHORT window_key_done
scan_accelerators:
    push WORD PTR [di]
    push cs
    call UPPERCASE_EVENT_CHARACTER
    mov [bp+6], ax
    mov si, [bp+8]
    add si, 50h
scan_next_item:
    mov si, [si]
    test si, si
    jz window_key_done
    mov cx, [si+1ah]
    jcxz scan_next_item
    mov cx, [si+1ch]
    jcxz scan_next_item
    mov cx, [si+14h]
    jcxz scan_next_item
    push cx
    push cs
    call UPPERCASE_EVENT_CHARACTER
    cmp ax, [bp+6]
    jne scan_next_item
    push si
    mov ax, 7
    push ax
    push WORD PTR [di]
    call DWORD PTR [si+4]
    mov [di], ax
window_key_done:
    xchg si, ax
    pop si
    pop di
    pop bp
    retf 4
DISPATCH_WINDOW_KEY ENDP

PAINT_WINDOW_ITEMS PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    push cs
    call UPDATE_MESSAGE_DISPLAY
    mov di, [bp+6]
    lea si, [di+50h]
item_next:
    mov si, [si]
    test si, si
    je paint_focus
    mov cx, [si+1ch]
    jcxz item_next
    push si
    mov ax, 2
    push ax
    sub ax, ax
    push ax
    call DWORD PTR [si+4]
    jmp short item_next
paint_focus:
    mov bx, [di+52h]
    test bx, bx
    je show_cursor
    call NEAR PTR STROKE_ITEM_EDGES
show_cursor:
    push cs
    call SHOW_MOUSE_GRAPHICS
    pop si
    pop di
    pop bp
    retf 2
PAINT_WINDOW_ITEMS ENDP

FIND_ITEM_AT_POINT PROC FAR
    push bp
    mov bp, sp
    push di
    mov di, [bp+8]
    add di, 50h
search_next_item:
    mov di, [di]
    test di, di
    jz search_done
    mov cx, [di+1ah]
    jcxz search_next_item
    mov cx, [di+1ch]
    jcxz search_next_item
    push ds
    push WORD PTR [bp+6]
    lea ax, [di+0ch]
    push ds
    push ax
    push cs
    call POINT_IN_RECTANGLE
    xchg cx, ax
    jcxz search_next_item
search_done:
    xchg di, ax
    pop di
    pop bp
    retf 4
FIND_ITEM_AT_POINT ENDP

DISPOSE_WINDOW_ITEM PROC FAR
    push bp
    mov bp, sp
    push di
    mov di, [bp+6]
    mov bx, [di+2]
    cmp di, [bx+52h]
    jne find_item_link
    push bx
    sub ax, ax
    push ax
    push cs
    call NEAR PTR SET_SELECTED_ITEM
find_item_link:
    mov cx, [di+2]
    add cx, 50h
next_item_link:
    mov bx, cx
    mov cx, [bx]
    jcxz redraw_bounds
    cmp cx, di
    jne next_item_link
    mov ax, [di]
    mov [bx], ax
redraw_bounds:
    mov ax, [di+2]
    xchg Graf, ax
    push ax
    lea ax, [di+0ch]
    push ds
    push ax
    push cs
    call STROKE_PORT_RECT
    pop Graf
    push di
    mov ax, 8
    push ax
    sub ax, ax
    push ax
    call DWORD PTR [di+4]
    push di
    call __nfree
    add sp, 2
    pop di
    pop bp
    retf 2
DISPOSE_WINDOW_ITEM ENDP

RELEASE_WINDOW_ITEM_EFFECT PROC FAR
    push bp
    mov bp, sp
    push di
    mov di, word ptr [bp+6]
    sub cx, cx
    xchg word ptr [di+1ch], cx
    jcxz return_released_item
    mov ax, word ptr [di+2]
    xchg Graf, ax
    push ax
    lea ax, [di+0ch]
    push ds
    push ax
    push cs
    call STROKE_PORT_RECT
    pop Graf
    mov bx, word ptr [di+2]
    sub di, word ptr [bx+52h]
    jne return_released_item
    push bx
    push di
    push cs
    call NEAR PTR SET_SELECTED_ITEM
return_released_item:
    xchg di, ax
    pop di
    pop bp
    retf 2
RELEASE_WINDOW_ITEM_EFFECT ENDP

ITEM_BY_INDEX PROC FAR
    push bp
    mov bp, sp
    sub ax, ax
    mov bx, [bp+8]
    lea bx, [bx+50h]
next_definition:
    mov bx, [bx]
    test bx, bx
    jz return_indexed_item
    inc ax
    cmp ax, [bp+6]
    jne next_definition
return_indexed_item:
    xchg bx, ax
    pop bp
    retf 4
ITEM_BY_INDEX ENDP

LOAD_ITEM_DEFINITION PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    sub di, di
    mov si, [bp+6]
    mov ax, 4344h
    push ax
    mov ax, 4546h
    push ax
    les bx, DWORD PTR [si]
    push WORD PTR es:[bx]
    push cs
    call HGETRSRC
    push ax
    push cs
    call RESOLVE_HANDLE_TO_FAR_PAYLOAD
    xchg bx, ax
    mov es, dx
    or dx, bx
    jz definition_done
    push di
    mov ax, 0
    push ax
    push si
    call DWORD PTR es:[bx]
    xchg di, ax
    test di, di
    jz definition_done
    mov bx, [bp+8]
    mov [di+2], bx
    add bx, 50h
find_last_item:
    mov cx, [bx]
    jcxz append_item
    mov bx, cx
    jmp SHORT find_last_item
append_item:
    mov [bx], di
    mov [di], cx
    push di
    mov ax, 1
    push ax
    push cx
    call DWORD PTR [di+4]
definition_done:
    xchg di, ax
    pop si
    pop di
    pop bp
    retf 4
LOAD_ITEM_DEFINITION ENDP

LOAD_ITEM_LIST PROC FAR
    push bp
    mov bp, sp
    push di
    les bx, DWORD PTR [bp+6]
    mov di, es:[bx]
    add WORD PTR [bp+6], 2
load_next_item:
    dec di
    jl item_list_done
    push WORD PTR [bp+10]
    lea ax, [bp+6]
    push ax
    push cs
    call NEAR PTR LOAD_ITEM_DEFINITION
    jmp SHORT load_next_item
item_list_done:
    pop di
    pop bp
    retf 6
LOAD_ITEM_LIST ENDP

STEP_SELECTABLE_ITEM PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    mov bx, [bp+8]
    test bx, bx
    jz return_item
    mov di, [bx+2]
    mov si, [bp+6]
check_remaining_steps:
    test si, si
    jl step_previous
    jg step_next
return_item:
    xchg bx, ax
    pop si
    pop di
    pop bp
    retf 4
step_next:
    mov bx, [bx]
    test bx, bx
    jnz check_next
    mov bx, [di+50h]
check_next:
    mov cx, [bx+1ah]
    jcxz step_next
    mov cx, [bx+1ch]
    jcxz step_next
    dec si
    jmp SHORT check_remaining_steps
step_previous:
    xchg bx, ax
    lea bx, [di+50h]
    cmp [bx], ax
    jne find_previous
    sub ax, ax
find_previous:
    cmp [bx], ax
    je check_previous
    mov bx, [bx]
    jmp SHORT find_previous
check_previous:
    mov cx, [bx+1ah]
    jcxz step_previous
    mov cx, [bx+1ch]
    jcxz step_previous
    inc si
    jmp SHORT check_remaining_steps
STEP_SELECTABLE_ITEM ENDP

SET_SELECTED_ITEM PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    mov bx, [bp+8]
    mov di, [bp+6]
    mov si, [bx+52h]
    cmp di, si
    je selected_item_ready
    test di, di
    jz test_previous_item
    mov cx, [di+1ah]
    jcxz selected_item_ready
    mov cx, [di+1ch]
    jcxz selected_item_ready
test_previous_item:
    test si, si
    jz store_selected_item
    mov bx, si
    call NEAR PTR STROKE_ITEM_EDGES
    push si
    mov ax, 4
    push ax
    sub ax, ax
    push ax
    call DWORD PTR [si+4]
store_selected_item:
    mov bx, [bp+8]
    mov [bx+52h], di
    test di, di
    jz selected_item_ready
    mov bx, di
    call NEAR PTR STROKE_ITEM_EDGES
    push di
    mov ax, 4
    push ax
    mov ax, 1
    push ax
    call DWORD PTR [di+4]
selected_item_ready:
    mov ax, [bx+52h]
    pop si
    pop di
    pop bp
    retf 4
SET_SELECTED_ITEM ENDP

SELECT_ITEM_BY_INDEX PROC FAR
    push bp
    mov bp, sp
    push WORD PTR [bp+8]
    push WORD PTR [bp+6]
    push cs
    call NEAR PTR ITEM_BY_INDEX
    push WORD PTR [bp+8]
    push ax
    push cs
    call NEAR PTR SET_SELECTED_ITEM
    pop bp
    retf 4
SELECT_ITEM_BY_INDEX ENDP

SHOW_WINDOW_ITEM PROC FAR
    push bp
    mov bp, sp
    push di
    mov di, word ptr [bp+6]
    mov ax, 1
    xchg word ptr [di+1ch], ax
    test ax, ax
    jne loc_8d03
    push di
    mov ax, 2
    push ax
    sub ax, ax
    push ax
    call DWORD PTR [di+4]
loc_8d03:
    xchg di, ax
    pop di
    pop bp
    retf 2
SHOW_WINDOW_ITEM ENDP

DISPATCH_ITEM_CHARACTER PROC FAR
    push bp
    mov bp, sp
    mov bx, [bp+8]
    mov cx, [bx+1ah]
    jcxz character_done
    mov cx, [bx+1ch]
    jcxz character_done
    push bx
    mov ax, 6
    push ax
    push WORD PTR [bp+6]
    call DWORD PTR [bx+4]
    xchg cx, ax
character_done:
    xchg cx, ax
    pop bp
    retf 4
DISPATCH_ITEM_CHARACTER ENDP

STROKE_ITEM_EDGES PROC NEAR
    mov ax, [bx+2]
    xchg Graf, ax
    push ax
    add bx, 0ch
    push bx
    mov ax, 2
    push ax
    mov ax, 1
    push ax
    mov ax, 3
    push ax
    mov ax, 7
    push ax
    push cs
    call STROKE_OFFSET_EDGES
    pop Graf
    ret
STROKE_ITEM_EDGES ENDP

FILL_BEFORE_MODAL_WINDOW LABEL BYTE
    db 0  ; Original alignment byte between routines.

RUN_MODAL_RESOURCE_WINDOW PROC FAR
    push bp
    mov bp, sp
    add sp, -4
    push di
    push si
    mov word ptr [bp-2], 0
    mov ax, window_draw_port
    xchg Graf, ax
    push ax
    push word ptr [bp+6]
    push cs
    call NEAR PTR CREATE_WINDOW_FROM_RESOURCE
    xchg di, ax
    test di, di
    je loc_8dbb
    lea bx, [di+3eh]
    push ds
    pop es
    mov ax, word ptr [di+2ch]
    call NEAR PTR FIND_WINDOW_AT_RECT
    cmc
    sbb ax, ax
    je loc_8d87
    lea ax, [di+3eh]
    call ALLOCATE_BACKGROUND_PORT
loc_8d87:
    mov word ptr [bp-4], ax
    mov ax, 0ffffh
    push ax
    push cs
    call PLAY_SOUND
    sub ax, ax
    push ax
    push ax
    push cs
    call NEAR PTR WAIT_FOR_WINDOW_RESULT
    mov word ptr [bp-2], ax
    mov cx, word ptr [bp-4]
    jcxz loc_8db6
    xchg cx, ax
    call RELEASE_BACKGROUND_PORT
    sub cx, cx
    xchg word ptr [di+2ch], cx
    mov WINDOW_HEAD, cx
    jcxz loc_8db6
    push cx
    push cs
    call ACTIVATE_WINDOW
loc_8db6:
    push di
    push cs
    call DISPOSE_WINDOW
loc_8dbb:
    pop Graf
    mov ax, word ptr [bp-2]
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 2
RUN_MODAL_RESOURCE_WINDOW ENDP

DISPATCH_WINDOW_EVENT PROC FAR
    push bp
    mov bp, sp
    add sp, -4
    push di
    push si
    mov ax, [bp+0ah]
    mov di, ax
    xchg Graf, ax
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
    pop Graf
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 6

idle_notify:
    push di
    push cs
    call NEAR PTR NOTIFY_SELECTED_ITEM
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
    call NEAR PTR DISPATCH_LINKED_ITEMS
    jmp short ignored
repaint_window:
    cmp di, [si+0ah]
    jne idle_notify
    push di
    push cs
    call PAINT_PENDING_WINDOW
    push di
    push cs
    call NEAR PTR PAINT_WINDOW_ITEMS
    push di
    push cs
    call RESTORE_WINDOW_CLIP
    jmp short ignored
handle_keyboard:
    push di
    lea ax, [si+0ah]
    push ax
    push cs
    call NEAR PTR DISPATCH_WINDOW_KEY
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
    call NEAR PTR STEP_SELECTABLE_ITEM
    push di
    push ax
    push cs
    call NEAR PTR SET_SELECTED_ITEM
ignored_long:
    jmp ignored
report_selected:
    push si
    push cs
    call NEAR PTR SELECTED_ITEM_ORDINAL
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
    call NEAR PTR FIND_ITEM_AT_POINT
    test ax, ax
    je ignored_long
    push ax
    push WORD PTR [si+0ah]
    xchg si, ax
    push cs
    call NEAR PTR DISPATCH_ITEM_CHARACTER
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
    call NEAR PTR SET_SELECTED_ITEM
    jmp short ignored_long
DISPATCH_WINDOW_EVENT ENDP

REPAINT_WINDOW_ITEM PROC FAR
    push bp
    mov bp, sp
    push di
    mov di, word ptr [bp+6]
    mov ax, di
    xchg Graf, ax
    push ax
    push di
    mov ax, 2
    push ax
    sub ax, ax
    push ax
    call DWORD PTR [di+2eh]
    lea ax, [di+10h]
    push ds
    push ax
    push cs
    call STROKE_PORT_RECT
    push di
    push cs
    call NEAR PTR PAINT_WINDOW_ITEMS
    pop Graf
    pop di
    pop bp
    retf 2
REPAINT_WINDOW_ITEM ENDP

CREATE_WINDOW_FROM_RESOURCE PROC FAR
    push bp
    mov bp, sp
    push di
    mov ax, 444ch
    push ax
    mov ax, 4f47h
    push ax
    push WORD PTR [bp+6]
    push cs
    call HGETRSRC
    test ax, ax
    jz done
    xchg di, ax
    push di
    push cs
    call SAVE_HANDLE_FLAGS
    push di
    push ax
    push di
    push cs
    call FETCH_LOCKED_SHAPE_LIST
    push dx
    push ax
    push cs
    call NEAR PTR CREATE_WINDOW_FROM_DEFINITION
    xchg di, ax
    push cs
    call RESTORE_HANDLE_FLAGS
    xchg di, ax
done:
    pop di
    pop bp
    retf 2
CREATE_WINDOW_FROM_RESOURCE ENDP

WAIT_FOR_WINDOW_RESULT PROC FAR
    push bp
    mov bp, sp
    add sp, -16h
    push di
    push si
    push Graf
    mov di, WINDOW_HEAD
read_event:
    mov Graf, di
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
    call NEAR PTR DISPATCH_WINDOW_EVENT
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
    pop Graf
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 4
WAIT_FOR_WINDOW_RESULT ENDP

CREATE_WINDOW_FROM_DEFINITION PROC FAR
    push bp
    mov bp, sp
    add sp, -0eh
    push di
    push Graf
    mov ax, -1
    push ax
    les bx, DWORD PTR [bp+6]
    lea ax, [bx]
    push es
    push ax
    sub ax, ax
    push ax
    push ax
    push ax
    push cs
    call CREATE_WINDOW
    xchg di, ax
    test di, di
    jz restore_window_port
    mov Graf, di
    mov ax, [di+1ah]
    xchg [di+1eh], ax
    push ax
    push di
    les bx, DWORD PTR [bp+6]
    push WORD PTR es:[bx+8]
    push di
    lea ax, [bx+0ah]
    push es
    push ax
    push cs
    call NEAR PTR LOAD_ITEM_LIST
    push cs
    call NEAR PTR SELECT_ITEM_BY_INDEX
    pop WORD PTR [di+1eh]
restore_window_port:
    pop Graf
    xchg di, ax
    pop di
    mov sp, bp
    pop bp
    retf 4
CREATE_WINDOW_FROM_DEFINITION ENDP

; Walk window links until a rectangle intersects. Return the hit in CF.
FIND_WINDOW_AT_RECT PROC NEAR
    push bp
    mov bp, sp
    add sp, -8
    push di
    xchg di, ax
loc_9049:
    test di, di
    je loc_906a
    mov cx, word ptr [di+46h]
    jcxz loc_906f
    push es
    push bx
    lea ax, [bp-8]
    push ds
    push ax
    lea ax, [di+48h]
    push ds
    push ax
    push es
    push bx
    push cs
    call SECTRECT
    xchg cx, ax
    pop bx
    pop es
    jcxz loc_906f
    stc
loc_906a:
    pop di
    mov sp, bp
    pop bp
    ret
loc_906f:
    mov di, word ptr [di+2ch]
    jmp SHORT loc_9049
FIND_WINDOW_AT_RECT ENDP

; Far callback for the window frame. Commands initialize, hit-test and paint.
WINDOW_FRAME_ITEM_HANDLER PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    mov ax, window_draw_port
    xchg Graf, ax
    push ax
    mov di, word ptr [bp+0ah]
    mov ax, word ptr [bp+8]
    cmp ax, 1
    je initialize_frame_item
    cmp ax, 3
    je hit_test_frame_item
    cmp ax, 2
    je paint_frame_item
    sub ax, ax
restore_frame_item_port:
    pop Graf
    pop si
    pop di
    pop bp
    retf 6
initialize_frame_item:
    mov bx, di
    mov cx, 8
    lea di, [bx+36h]
    lea si, [bx+10h]
    push ds
    pop es
    rep movsb
    mov cx, word ptr [bx+6]
    mov dx, word ptr [bx+4]
    sub word ptr [bx+38h], cx
    sub word ptr [bx+36h], dx
    sub word ptr [bx+3ch], cx
    sub word ptr [bx+3ah], dx
    mov cx, 8
    lea di, [bx+3eh]
    lea si, [bx+36h]
    rep movsb
    mov di, bx
    dec word ptr [di+40h]
    dec word ptr [di+3eh]
    add word ptr [di+44h], 3
    add word ptr [di+42h], 2
    push di
    push cs
    call FINISH_PORT
    lea ax, [di+18h]
    push ds
    push ax
    push ds
    push ax
    mov ax, window_draw_port
    add ax, 18h
    push ds
    push ax
    push cs
    call SECTRECT
    jmp SHORT restore_frame_item_port
hit_test_frame_item:
    push ds
    push word ptr [bp+6]
    lea ax, [di+36h]
    push ds
    push ax
    push cs
    call POINT_IN_RECTANGLE
    xchg cx, ax
    mov ax, 0
    jcxz return_frame_hit
    mov ax, 2
return_frame_hit:
    jmp SHORT restore_frame_item_port
paint_frame_item:
    cmp word ptr [bp+6], 0
    jne restore_frame_item_port
    mov bx, Graf
    mov ax, 0
    xchg word ptr [bx+28h], ax
    push ax
    push bx
    lea si, [di+3eh]
    sub word ptr [si+6], 2
    sub word ptr [si+4], 1
    push si
    mov ax, 2
    push ax
    mov ax, 1
    push ax
    mov ax, 0
    push ax
    push word ptr [bx+28h]
    push cs
    call STROKE_OFFSET_EDGES
    push ds
    push si
    push cs
    call FRAME_RECT
    add word ptr [si+6], 2
    add word ptr [si+4], 1
    pop bx
    pop word ptr [bx+28h]
    jmp NEAR PTR restore_frame_item_port
WINDOW_FRAME_ITEM_HANDLER ENDP

; Far list-item callback. Dispatch stubs keep the command compares short.
LIST_ITEM_HANDLER PROC FAR
    push bp
    mov bp, sp
    add sp, -0ch
    push di
    push si
    mov di, word ptr [bp+0ah]
    mov si, word ptr [bp+8]
    cmp si, 0
    je dispatch_list_create
    mov ax, word ptr [di+2]
    xchg Graf, ax
    push ax
    lea ax, [bp-0ch]
    push ds
    push ax
    push ds
    push ax
    push cs
    call COPY_CURRENT_PORT_ORIGIN
    push cs
    call APPLY_DEFAULT_PEN_STATE
    cmp word ptr [di+1ah], 0
    jne loc_918f
    mov bx, Graf
    mov word ptr [bx+28h], 7
loc_918f:
    mov cx, word ptr [bp+6]
    xchg si, ax
    cmp ax, 1
    je dispatch_list_initialize
    cmp ax, 2
    je dispatch_list_repaint
    cmp ax, 5
    je dispatch_list_blink
    cmp ax, 0
    je dispatch_list_create
    cmp ax, 7
    je dispatch_list_key
    cmp ax, 6
    je dispatch_list_pointer
    cmp ax, 8
    je dispatch_list_dispose
    cmp ax, 3
    je set_list_cursor_visibility
    cmp ax, 4
    je loc_91f7
    cmp ax, 9
    je loc_9218
ignore_list_command:
    sub ax, ax
restore_list_port:
    xchg si, ax
    push cs
    call SET_RELATIVE_PEN_STATE
    xchg si, ax
    pop Graf
return_list_result:
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 6
dispatch_list_initialize:
    jmp NEAR PTR initialize_list_item
dispatch_list_repaint:
    jmp SHORT repaint_list_command
dispatch_list_blink:
    jmp SHORT blink_list_cursor
dispatch_list_create:
    jmp SHORT allocate_list_item
dispatch_list_key:
    jmp NEAR PTR handle_list_key
dispatch_list_pointer:
    jmp NEAR PTR handle_list_pointer
dispatch_list_dispose:
    jmp NEAR PTR dispose_list_text
set_list_cursor_visibility:
    jcxz hide_list_cursor_now
loc_91ed:
    call NEAR PTR SHOW_LIST_CURSOR
    jmp SHORT ignore_list_command
hide_list_cursor_now:
    call NEAR PTR HIDE_LIST_CURSOR
    jmp SHORT ignore_list_command
loc_91f7:
    jcxz loc_9209
    call NEAR PTR REBUILD_LIST_TEXT_HANDLE
    sub ax, ax
    mov word ptr [di+20h], ax
    mov word ptr [di+22h], ax
    call NEAR PTR REPAINT_LIST_ITEM
    jmp SHORT loc_91ed
loc_9209:
    sub ax, ax
    xchg word ptr [di+32h], ax
    push ax
    push cs
    call DISPOSE_HANDLE
    call NEAR PTR REPAINT_LIST_ITEM
    jmp SHORT hide_list_cursor_now
loc_9218:
    xchg cx, ax
    mov dx, ds
    call NEAR PTR COPY_LIST_TEXT
    call NEAR PTR REBUILD_LIST_TEXT_HANDLE
    call NEAR PTR REPAINT_LIST_ITEM
    jmp SHORT ignore_list_command
repaint_list_command:
    lea si, [di+0ch]
    push ds
    push si
    push cs
    call FRAME_RECT
    call NEAR PTR REPAINT_LIST_ITEM
    jmp SHORT ignore_list_command
blink_list_cursor:
    mov ax, TIMER_TICKS
    sub ax, word ptr [di+24h]
    mov dx, TIMER_RELOAD
    shr dx, 1
    cmp ax, dx
    jb loc_9247
    call NEAR PTR TOGGLE_LIST_CURSOR
loc_9247:
    jmp NEAR PTR ignore_list_command
allocate_list_item:
    mov si, word ptr [bp+6]
    les bx, [si]
    mov ax, word ptr es:[bx+0ah]
    add ax, 39h
    push ax
    push ax
    call __nmalloc
    add sp, 2
    pop cx
    test ax, ax
    je loc_92a2
    xchg dx, ax
    sub ax, ax
    mov di, dx
    push ds
    pop es
    rep stosb
    mov di, dx
    lds si, [si]
    lodsw
    mov cx, 8
    add di, 0ch
    rep movsb
    mov di, dx
    push ds
    push es
    pop ds
    pop es
    mov word ptr [di+4], OFFSET LIST_ITEM_HANDLER
    mov word ptr [di+6], cs
    mov ax, 1
    mov word ptr [di+1ah], ax
    mov word ptr [di+1ch], ax
    mov word ptr [di+1eh], 0ffffh
    lods WORD PTR es:[si]
    mov word ptr [di+36h], ax
    mov bx, word ptr [bp+6]
    mov word ptr [bx], si
    xchg di, ax
loc_92a2:
    jmp NEAR PTR return_list_result
initialize_list_item:
    mov ax, word ptr [di+0eh]
    add ax, 2
    mov word ptr [di+2ch], ax
    mov ax, word ptr [di+12h]
    sub ax, 2
    mov word ptr [di+30h], ax
    mov bx, word ptr [di+2]
    push word ptr [bx+2ah]
    push cs
    call RESOLVE_HANDLE_TO_FAR_PAYLOAD
    xchg bx, ax
    mov es, dx
    mov dx, word ptr [di+10h]
    mov cx, dx
    sub cx, word ptr [di+0ch]
    shr cx, 1
    sbb dx, cx
    mov ax, word ptr es:[bx+2]
    add ax, word ptr es:[bx+4]
    mov cx, ax
    shr cx, 1
    adc dx, cx
    mov word ptr [di+2eh], dx
    sub dx, ax
    mov word ptr [di+2ah], dx
    jmp NEAR PTR ignore_list_command
loc_92e9:
    jcxz loc_9309
    sub cx, cx
    jmp SHORT loc_92ff
loc_92ef:
    cmp cx, word ptr [di+34h]
    jae loc_9309
    mov cx, word ptr [di+34h]
    jmp SHORT loc_92ff
loc_92f9:
    cmp cx, word ptr [di+34h]
    jae loc_9309
    inc cx
loc_92ff:
    mov word ptr [di+20h], cx
    jmp SHORT loc_936f
loc_9304:
    jcxz loc_9309
    dec cx
    jmp SHORT loc_92ff
loc_9309:
    jmp SHORT loc_9379
handle_list_key:
    push cx
    call NEAR PTR HIDE_LIST_CURSOR
    pop ax
    mov cx, word ptr [di+20h]
    cmp al, 8
    je loc_937e
    cmp ax, 5300h
    je loc_9383
    cmp al, 1bh
    je loc_939e
    cmp ax, 4b00h
    je loc_9304
    cmp ax, 4d00h
    je loc_92f9
    cmp ax, 4700h
    je loc_92e9
    cmp ax, 4f00h
    je loc_92ef
    cmp al, 7eh
    ja loc_9379
    cmp al, 20h
    jb loc_9379
    jne loc_9340
    jcxz loc_935f
loc_9340:
    mov dx, word ptr [di+26h]
    or dx, word ptr [di+28h]
    je loc_9351
    push di
    push ax
    call DWORD PTR [di+26h]
    test al, al
    je loc_9365
loc_9351:
    mov bx, word ptr [di+20h]
    cmp bx, word ptr [di+36h]
    jae loc_936f
    mov byte ptr [bx+di+38h], al
    lea cx, [bx+1]
loc_935f:
    mov word ptr [di+34h], cx
    mov word ptr [di+20h], cx
loc_9365:
    mov bx, word ptr [di+34h]
    mov byte ptr [bx+di+38h], 0
    call NEAR PTR REPAINT_LIST_ITEM
loc_936f:
    sub ax, ax
loc_9371:
    push ax
    call NEAR PTR SHOW_LIST_CURSOR
    pop ax
    jmp NEAR PTR restore_list_port
loc_9379:
    mov ax, word ptr [bp+6]
    jmp SHORT loc_9371
loc_937e:
    jcxz loc_9379
    dec cx
    jmp SHORT loc_935f
loc_9383:
    mov bx, word ptr [di+34h]
    sub bx, cx
    je loc_936f
    push di
    add di, word ptr [di+20h]
    add di, 38h
    lea si, [di+1]
    push ds
    pop es
    rep movsb
    pop di
    dec word ptr [di+34h]
    jmp SHORT loc_9365
loc_939e:
    mov cx, word ptr [di+32h]
    jcxz loc_9379
    push cx
    push cs
    call RESOLVE_HANDLE_TO_FAR_PAYLOAD
    push dx
    push ax
    lea ax, [di+38h]
    push ds
    push ax
    call __fstrcmp
    add sp, 8
    xchg cx, ax
    jcxz loc_9379
    push word ptr [di+32h]
    push cs
    call RESOLVE_HANDLE_TO_FAR_PAYLOAD
    call NEAR PTR COPY_LIST_TEXT
    jmp SHORT loc_9365
handle_list_pointer:
    lea si, [bp-4]
    push si
    push cs
    call READ_MOUSE_EVENT_POSITION
    push ds
    push si
    push ds
    push si
    push cs
    call ADD_PORT_ORIGIN
    push ds
    push si
    lea ax, [di+0ch]
    push ds
    push ax
    push cs
    call POINT_IN_RECTANGLE
    jmp NEAR PTR restore_list_port
dispose_list_text:
    push word ptr [di+32h]
    push cs
    call DISPOSE_HANDLE
    jmp NEAR PTR ignore_list_command
LIST_ITEM_HANDLER ENDP

; Tail-call the cursor toggle when the cursor is hidden.
SHOW_LIST_CURSOR PROC NEAR
    mov cx, word ptr [di+22h]
    jcxz TOGGLE_LIST_CURSOR
    ret
SHOW_LIST_CURSOR ENDP

; Tail-call the cursor toggle when the cursor is visible.
HIDE_LIST_CURSOR PROC NEAR
    cmp word ptr [di+22h], 0
    jne TOGGLE_LIST_CURSOR
    ret
HIDE_LIST_CURSOR ENDP

TOGGLE_LIST_CURSOR PROC NEAR
    push bp
    mov bp, sp
    add sp, -8
    push si
    mov si, word ptr [di+2]
    mov ax, 3
    xchg word ptr [si+26h], ax
    push ax
    mov ax, 0fh
    xchg word ptr [si+28h], ax
    push ax
    lea ax, [bp-8]
    push ds
    push ax
    lea ax, [di+0ch]
    push ds
    push ax
    mov ax, 1
    push ax
    push ax
    push cs
    call INSETRECT
    push ds
    push ax
    push cs
    call PUSH_PORT_CLIP
    lea ax, [di+38h]
    push ds
    push ax
    push word ptr [di+20h]
    push cs
    call MEASURE_COUNTED_TEXT
    add ax, word ptr [di+2ch]
    dec ax
    push ax
    push ax
    mov ax, word ptr [di+2ah]
    dec ax
    push ax
    push cs
    call MOVETO
    push word ptr [di+2eh]
    push cs
    call DRAW_AXIS_SEGMENT
    mov ax, TIMER_TICKS
    mov word ptr [di+24h], ax
    sub ax, ax
    cmp word ptr [di+22h], ax
    jne loc_945e
    mov ax, 1
loc_945e:
    mov word ptr [di+22h], ax
    push cs
    call POP_PORT_CLIP
    pop word ptr [si+28h]
    pop word ptr [si+26h]
    pop si
    mov sp, bp
    pop bp
    ret
TOGGLE_LIST_CURSOR ENDP

REPAINT_LIST_ITEM PROC NEAR
    push bp
    mov bp, sp
    add sp, -8
    push si
    lea ax, [bp-8]
    push ds
    push ax
    lea ax, [di+0ch]
    push ds
    push ax
    mov ax, 1
    push ax
    push ax
    push cs
    call INSETRECT
    push ds
    push ax
    push cs
    call PUSH_PORT_CLIP
    push ds
    push ax
    push cs
    call STROKE_PORT_RECT
    lea ax, [di+38h]
    push ds
    push ax
    push cs
    call MEASURE_NUL_TERMINATED_TEXT
    sub bx, bx
    mov cx, word ptr [di+2ch]
    mov dx, word ptr [di+30h]
    cmp bx, word ptr [di+32h]
    jne loc_94c3
    cmp bx, word ptr [di+1eh]
    jg loc_94c3
    jl loc_94bf
    sub dx, cx
    sar dx, 1
    add cx, dx
    sar ax, 1
    sub cx, ax
    jmp SHORT loc_94c3
loc_94bf:
    sub dx, ax
    mov cx, dx
loc_94c3:
    mov si, word ptr [di+2]
    mov word ptr [si+24h], cx
    push word ptr [si+2ah]
    push cs
    call RESOLVE_HANDLE_TO_FAR_PAYLOAD
    xchg bx, ax
    mov es, dx
    mov ax, word ptr es:[bx+2]
    add ax, word ptr [di+2ah]
    mov word ptr [si+22h], ax
    mov ax, 7
    mov cx, word ptr [di+1ah]
    jcxz loc_94e8
    mov ax, word ptr [di+18h]
loc_94e8:
    xchg word ptr [si+28h], ax
    push ax
    lea ax, [di+38h]
    push ds
    push ax
    push cs
    call DRAW_NUL_TERMINATED_TEXT
    pop word ptr [si+28h]
    push cs
    call POP_PORT_CLIP
    sub cx, cx
    xchg word ptr [di+22h], cx
    jcxz loc_9506
    call NEAR PTR TOGGLE_LIST_CURSOR
loc_9506:
    pop si
    mov sp, bp
    pop bp
    ret
REPAINT_LIST_ITEM ENDP

REBUILD_LIST_TEXT_HANDLE PROC NEAR
    lea ax, [di+38h]
    push ds
    push ax
    push cs
    call CREATE_STRING_HANDLE
    xchg word ptr [di+32h], ax
    push ax
    push cs
    call DISPOSE_HANDLE
    ret
REBUILD_LIST_TEXT_HANDLE ENDP

COPY_LIST_TEXT PROC NEAR
    push si
    sub bx, bx
    xchg si, ax
    mov es, dx
    or dx, si
    je loc_9538
loc_9527:
    cmp bx, word ptr [di+36h]
    je loc_9538
    lods BYTE PTR es:[si]
    test al, al
    je loc_9538
    mov byte ptr [bx+di+38h], al
    inc bx
    jmp SHORT loc_9527
loc_9538:
    mov word ptr [di+34h], bx
    sub ax, ax
    mov byte ptr [bx+di+38h], al
    mov word ptr [di+20h], ax
    pop si
    ret
COPY_LIST_TEXT ENDP

FILL_BEFORE_GRID_HANDLER LABEL BYTE
    db 0  ; Original alignment byte between routines.

; Far callback for the grid item. Rectangle calculations share the caller frame.
GRID_ITEM_HANDLER PROC FAR
    push bp
    mov bp, sp
    add sp, -18h
    push di
    push si
    mov ax, window_draw_port
    xchg Graf, ax
    push ax
    mov di, word ptr [bp+0ah]
    mov ax, word ptr [bp+8]
    cmp ax, 1
    je initialize_grid_item
    cmp ax, 2
    je dispatch_grid_paint
    cmp ax, 3
    je hit_test_grid_item
    sub ax, ax
restore_grid_item_port:
    pop Graf
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 6
dispatch_grid_paint:
    jmp NEAR PTR paint_grid_item
initialize_grid_item:
    mov bx, di
    mov cx, 8
    lea di, [bx+36h]
    lea si, [bx+10h]
    push ds
    pop es
    rep movsb
    mov cx, word ptr [bx+6]
    mov dx, word ptr [bx+4]
    sub word ptr [bx+38h], cx
    sub word ptr [bx+36h], dx
    sub word ptr [bx+3ch], cx
    sub word ptr [bx+3ah], dx
    mov cx, 8
    lea di, [bx+3eh]
    lea si, [bx+36h]
    rep movsb
    mov di, bx
    dec word ptr [di+40h]
    mov si, Graf
    push word ptr [si+2ah]
    push cs
    call RESOLVE_HANDLE_TO_FAR_PAYLOAD
    xchg bx, ax
    mov es, dx
    mov ax, word ptr es:[bx+2]
    add ax, word ptr es:[bx+4]
    add ax, 4
    sub word ptr [di+3eh], ax
    add word ptr [di+44h], 3
    add word ptr [di+42h], 2
    push di
    push cs
    call FINISH_PORT
    lea ax, [di+18h]
    push ds
    push ax
    push ds
    push ax
    lea ax, [si+18h]
    push ds
    push ax
    push cs
    call SECTRECT
    jmp SHORT restore_grid_item_port
hit_test_grid_item:
    call NEAR PTR UPDATE_GRID_ITEM_BOUNDS
    mov si, word ptr [bp+6]
    mov cx, word ptr [di+34h]
    jcxz loc_9605
    push ds
    push si
    lea ax, [bp-8]
    push ds
    push ax
    push cs
    call POINT_IN_RECTANGLE
    test ax, ax
    mov ax, 4
    jne loc_962b
loc_9605:
    push ds
    push si
    lea ax, [bp-18h]
    push ds
    push ax
    push cs
    call POINT_IN_RECTANGLE
    test ax, ax
    mov ax, 3
    jne loc_962b
    push ds
    push si
    lea ax, [di+36h]
    push ds
    push ax
    push cs
    call POINT_IN_RECTANGLE
    xchg cx, ax
    mov ax, 0
    jcxz loc_962b
    mov ax, 2
loc_962b:
    jmp NEAR PTR restore_grid_item_port
paint_grid_item:
    call NEAR PTR UPDATE_GRID_ITEM_BOUNDS
    mov cx, word ptr [bp+6]
    jcxz loc_9642
    lea ax, [bp-8]
    push ds
    push ax
    push cs
    call TOGGLE_RECT_EFFECT
    jmp NEAR PTR restore_grid_item_port
loc_9642:
    lea ax, [bp-10h]
    push ds
    push ax
    push ds
    push ax
    push cs
    call COPY_CURRENT_PORT_ORIGIN
    push cs
    call APPLY_DEFAULT_PEN_STATE
    lea si, [bp-18h]
    dec word ptr [si+4]
    push ds
    push si
    mov ax, 0fh
    push ax
    push cs
    call SET_RECORD_RANGE
    mov ax, word ptr [si+4]
    inc word ptr [si+4]
    xchg word ptr [si], ax
    push ax
    push ds
    push si
    mov bx, Graf
    mov ax, 0
    mov word ptr [bx+28h], ax
    push ax
    push cs
    call SET_RECORD_RANGE
    pop word ptr [si]
    mov ax, word ptr [di+3ah]
    inc ax
    xchg word ptr [si+4], ax
    push ax
    push ds
    push si
    push cs
    call FRAME_RECT
    push si
    mov ax, 2
    push ax
    mov ax, 1
    push ax
    mov ax, 0
    push ax
    mov ax, 0
    push ax
    push cs
    call STROKE_OFFSET_EDGES
    pop word ptr [si+4]
    push ds
    push si
    push ds
    push si
    mov ax, 2
    push ax
    push ax
    push cs
    call INSETRECT
    mov cx, word ptr [di+34h]
    jcxz loc_96fb
    mov ax, word ptr [si+2]
    xchg screen_x_origin, ax
    push ax
    mov ax, word ptr [si]
    xchg screen_row_origin, ax
    push ax
    push ds
    push si
    mov ax, 1
    push ax
    push cs
    call SET_RECORD_RANGE
    pop screen_row_origin
    pop screen_x_origin
    mov cx, word ptr [di+32h]
    jcxz loc_96fb
    lea bx, [bp-8]
    dec word ptr [bx+2]
    inc word ptr [bx+6]
    push ds
    push bx
    mov ax, 0fh
    push ax
    push cs
    call SET_RECORD_RANGE
    xchg bx, ax
    inc word ptr [bx+2]
    dec word ptr [bx+6]
    push ds
    push bx
    push cs
    call FRAME_RECT
loc_96fb:
    mov cx, word ptr [di+5ah]
    jcxz loc_975f
    push cx
    push cs
    call measure_text_handle
    mov dx, word ptr [si+6]
    sub dx, word ptr [si+2]
    sub dx, ax
    sar dx, 1
    sub dx, 2
    cmp dx, 10h
    jge loc_971a
    mov dx, 10h
loc_971a:
    add word ptr [si+2], dx
    sub word ptr [si+6], dx
    push ds
    push si
    mov ax, 0fh
    push ax
    push cs
    call SET_RECORD_RANGE
    add word ptr [si+2], 2
    sub word ptr [si+6], 2
    push ds
    push si
    push cs
    call PUSH_PORT_CLIP
    push word ptr [si+2]
    mov bx, Graf
    push word ptr [bx+2ah]
    push cs
    call RESOLVE_HANDLE_TO_FAR_PAYLOAD
    xchg bx, ax
    mov es, dx
    mov ax, word ptr es:[bx+2]
    add ax, word ptr [si]
    push ax
    push cs
    call MOVETO
    push word ptr [di+5ah]
    push cs
    call draw_text_handle
    push cs
    call POP_PORT_CLIP
loc_975f:
    push cs
    call SET_RELATIVE_PEN_STATE
    jmp NEAR PTR restore_grid_item_port
GRID_ITEM_HANDLER ENDP

; Near helper. The BP-relative outputs belong to the grid callback frame.
UPDATE_GRID_ITEM_BOUNDS PROC NEAR
    mov bx, window_draw_port
    push word ptr [bx+2ah]
    push cs
    call RESOLVE_HANDLE_TO_FAR_PAYLOAD
    xchg bx, ax
    mov es, dx
    mov ax, word ptr [di+3eh]
    mov word ptr [bp-18h], ax
    add ax, 2
    mov word ptr [bp-8], ax
    mov dx, word ptr es:[bx+2]
    add dx, word ptr es:[bx+4]
    add ax, dx
    mov word ptr [bp-4], ax
    add ax, 2
    mov word ptr [bp-14h], ax
    mov ax, word ptr [di+40h]
    mov word ptr [bp-16h], ax
    add ax, dx
    mov word ptr [bp-6], ax
    add ax, dx
    mov word ptr [bp-2], ax
    mov ax, word ptr [di+44h]
    sub ax, 2
    mov word ptr [bp-12h], ax
    ret
UPDATE_GRID_ITEM_BOUNDS ENDP

END
