.MODEL MEDIUM
.DATA
system_hit_rectangle DW 0, 0, 0, 0
EXTRN BACKGROUND_ORIGIN_X:WORD
EXTRN BACKGROUND_ORIGIN_Y:WORD
EXTRN BACKING_PORT_DESCRIPTOR:DWORD
EXTRN CURRENT_PORT:WORD
EXTRN DRAW_RECORD_RANGE:WORD
EXTRN PORT_ORIGIN:BYTE
EXTRN WINDOW_DRAW_PORT:WORD
EXTRN WINDOW_HEAD:WORD
EXTRN WINDOW_LIST:BYTE
.CONST
; BIOS Alt-F1 and Alt-F2 command words.
SEND_WINDOW_BACK_COMMAND DW 6800h
MOVE_WINDOW_COMMAND DW 6900h
.CODE
EXTRN ALLOCATE_WINDOW_MEMORY:FAR
EXTRN CREATE_STRING_HANDLE:NEAR
EXTRN DISCARD_QUEUED_EVENTS:NEAR
EXTRN DISPOSE_HANDLE:NEAR
EXTRN DISPOSE_WINDOW_ITEM:NEAR
EXTRN EMPTYRECT:NEAR
EXTRN FREE_NEAR:FAR
EXTRN HGETRSRC:NEAR
EXTRN INIT_PORT:NEAR
EXTRN MOUSE_BUTTON_UNQUEUED:NEAR
EXTRN POINT_IN_RECTANGLE:NEAR
EXTRN POP_PORT_CLIP:NEAR
EXTRN POST_POINTER_EVENT:NEAR
EXTRN PUSH_PORT_CLIP:NEAR
EXTRN READ_MOUSE_EVENT_POSITION:NEAR
EXTRN READ_QUEUED_EVENT:NEAR
EXTRN RESOLVE_HANDLE_TO_FAR_PAYLOAD:NEAR
EXTRN SECTRECT:NEAR
EXTRN SET_RECORD_RANGE:NEAR
EXTRN SHOW_MOUSE_GRAPHICS:NEAR
EXTRN STROKE_PORT_RECT:NEAR
EXTRN STROKE_RECTANGLE:NEAR
EXTRN SUBTRACT_RECTANGLE_STRIP:NEAR
EXTRN UNIONRECT:NEAR
EXTRN UPDATE_MESSAGE_DISPLAY:NEAR
EXTRN adjust_mouse_point:NEAR
EXTRN allocate_port_buffer:NEAR
EXTRN draw_outline_rect:NEAR
EXTRN offset_rect:NEAR
EXTRN offset_window_rectangle:NEAR
EXTRN origin_relative_rectangle:NEAR
EXTRN release_port_buffer:NEAR
EXTRN subtract_points:NEAR
EXTRN translate_port_point:NEAR
PUBLIC PAINT_PENDING_WINDOW
PUBLIC BRING_WINDOW_FORWARD
PUBLIC CLOSE_WINDOW
PUBLIC HANDLE_WINDOW_MOVE_COMMAND
PUBLIC DRAG_WINDOW_FROM_POINT
PUBLIC RESTORE_WINDOW_CLIP
PUBLIC LOCATE_WINDOW_AT_POINT
PUBLIC DISPOSE_WINDOW
PUBLIC SET_WINDOW_EVENT_STATE
PUBLIC INITIALIZE_WINDOW_DRAW_PORT
PUBLIC INVALIDATE_WINDOW_BEHIND_PORT
PUBLIC INVALIDATE_WINDOW_RECTANGLE
PUBLIC MOVE_OUTLINE_BY_KEYBOARD
PUBLIC DRAG_OUTLINE_WITH_MOUSE
PUBLIC OFFSET_WINDOW_AND_REDRAW
PUBLIC CREATE_WINDOW
PUBLIC INITIALIZE_WINDOW
PUBLIC STROKE_OFFSET_EDGES
PUBLIC ACTIVATE_WINDOW
PUBLIC RELINK_WINDOW_AFTER
PUBLIC REPLACE_WINDOW_TITLE
PUBLIC TRACK_WINDOW_HIT
PUBLIC TRANSLATE_WINDOW_CLIP
PUBLIC TRIM_PENDING_WINDOW_UPDATES
PUBLIC RELEASE_BACKGROUND_PORT
PUBLIC ALLOCATE_BACKGROUND_PORT

; Paint pending updates and preserve the window clip across the drawing callback.
PAINT_PENDING_WINDOW PROC FAR
    pop ax
    pop es
    pop dx
    mov bx, dx
    add bx, 20h
    mov cx, 4
save_window_clip:
    sub bx, 2
    push WORD PTR [bx]
    loop save_window_clip
    push dx
    push es
    push ax
    push bp
    mov bp, sp
    push di
    push cs
    call UPDATE_MESSAGE_DISPLAY
    mov ax, WINDOW_DRAW_PORT
    xchg CURRENT_PORT, ax
    push ax
    mov di, [bp+6]
    mov cx, [di+46h]
    jcxz window_paint_done
    lea ax, [di+48h]
    push ds
    push ax
    push cs
    call PUSH_PORT_CLIP
    sub ax, ax
    mov [di+46h], ax
    push di
    mov dx, 2
    push dx
    push ax
    call DWORD PTR [di+2eh]
    push cs
    call POP_PORT_CLIP
    mov CURRENT_PORT, di
    lea ax, [di+18h]
    push ds
    push ax
    push ds
    push ax
    lea ax, [di+48h]
    push ds
    push ax
    push cs
    call SECTRECT
    lea ax, [di+10h]
    push ds
    push ax
    push cs
    call STROKE_PORT_RECT
window_paint_done:
    pop CURRENT_PORT
    pop di
    pop bp
    retf 2
PAINT_PENDING_WINDOW ENDP

BRING_WINDOW_FORWARD PROC FAR
    push bp
    mov bp, sp
    add sp, -12h
    push di
    push si
    mov WORD PTR [bp-2], 0
    mov di, [bp+6]
    mov si, OFFSET WINDOW_LIST
inspect_preceding_window:
    cmp di, [si+2ch]
    je relink_window
    mov si, [si+2ch]
    lea ax, [bp-12h]
    push ds
    push ax
    lea ax, [di+3eh]
    push ds
    push ax
    lea ax, [si+3eh]
    push ds
    push ax
    push cs
    call SECTRECT
    xchg cx, ax
    jcxz inspect_preceding_window
    push di
    push si
    lea di, [bp-0ah]
    lea si, [bp-12h]
    mov cx, 1
    xchg [bp-2], cx
    jcxz copy_first_overlap
    push ds
    push di
    push ds
    push di
    push ds
    push si
    push cs
    call UNIONRECT
    jmp SHORT restore_windows
copy_first_overlap:
    push ds
    pop es
    mov cx, 4
    rep movsw
restore_windows:
    pop si
    pop di
    jmp SHORT inspect_preceding_window
relink_window:
    mov ax, [di+2ch]
    mov [si+2ch], ax
    mov ax, di
    xchg WINDOW_HEAD, ax
    mov [di+2ch], ax
    mov cx, [bp-2]
    jcxz window_front_done
    lea ax, [bp-0ah]
    push ax
    push di
    push cs
    call NEAR PTR INVALIDATE_WINDOW_RECTANGLE
window_front_done:
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 2
BRING_WINDOW_FORWARD ENDP

CLOSE_WINDOW PROC FAR
    push bp
    mov bp, sp
    push di
    mov ax, OFFSET WINDOW_LIST
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
    mov cx, OFFSET WINDOW_LIST
close_next_window:
    mov bx, cx
    mov cx, [bx+2ch]
    jcxz notify_window
    cmp di, cx
    jne close_next_window
    mov cx, [di+2ch]
    mov [bx+2ch], cx
    jcxz invalidate_bounds
    cmp cx, WINDOW_HEAD
    jne invalidate_bounds
    push cx
    push cs
    call NEAR PTR ACTIVATE_WINDOW
invalidate_bounds:
    lea ax, [di+3eh]
    push ax
    sub ax, ax
    push ax
    push cs
    call NEAR PTR INVALIDATE_WINDOW_RECTANGLE
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

; Dispatch keyboard commands to move the active window or send it back.
HANDLE_WINDOW_MOVE_COMMAND PROC FAR
    push bp
    mov bp, sp
    add sp, -10h
    push di
    mov di, word ptr [bp + 6]
    cmp word ptr [di], 8
    je match_window_command
move_command_unhandled:
    sub ax, ax
move_command_return:
    pop di
    mov sp, bp
    pop bp
    retf 2
match_window_command:
    mov ax, word ptr [di + 0ah]
    mov di, word ptr WINDOW_HEAD
    test di, di
    je move_command_unhandled
    cmp ax, word ptr MOVE_WINDOW_COMMAND
    je begin_key_move
    cmp ax, word ptr SEND_WINDOW_BACK_COMMAND
    jne move_command_unhandled
    push di
    sub ax, ax
    push ax
    push cs
    call NEAR PTR RELINK_WINDOW_AFTER
move_command_handled:
    mov ax, 1
    jmp SHORT move_command_return
begin_key_move:
    mov ax, OFFSET WINDOW_LIST
    mov bx, ax
    xchg word ptr CURRENT_PORT, ax
    push ax
    mov ax, 7
    xchg word ptr [bx + 28h], ax
    push ax
    push bx
    lea ax, [bp - 8]
    push ds
    push ax
    push ds
    add bx, 18h
    push bx
    mov ax, 10h
    sub ax, word ptr [di + 44h]
    add ax, word ptr [di + 40h]
    push ax
    mov ax, 8
    sub ax, word ptr [di + 42h]
    add ax, word ptr [di + 3eh]
    push ax
    push cs
    call origin_relative_rectangle
    lea bx, [di + 3eh]
    push bx
    push ax
    push cs
    call NEAR PTR MOVE_OUTLINE_BY_KEYBOARD
    mov cx, ax
    or ax, dx
    je restore_move_port
    push di
    add cx, word ptr [di + 38h]
    push cx
    add dx, word ptr [di + 36h]
    push dx
    push cs
    call NEAR PTR OFFSET_WINDOW_AND_REDRAW
restore_move_port:
    pop bx
    pop word ptr [bx + 28h]
    pop word ptr CURRENT_PORT
    jmp SHORT move_command_handled
HANDLE_WINDOW_MOVE_COMMAND ENDP

; Build a drag rectangle that includes the starting point and poll the mouse.
DRAG_WINDOW_FROM_POINT PROC FAR
    push bp
    mov bp, sp
    add sp, -10h
    push di
    push si
    mov ax, word ptr WINDOW_DRAW_PORT
    mov bx, ax
    xchg word ptr CURRENT_PORT, ax
    push ax
    lea di, [bp - 8]
    push ds
    push di
    push ds
    add bx, 18h
    push bx
    mov ax, 4
    push ax
    mov ax, 2
    push ax
    push cs
    call origin_relative_rectangle
    mov si, word ptr [bp + 8]
    mov ax, word ptr [si + 2]
    cmp ax, word ptr [di + 2]
    jge test_drag_right
    mov word ptr [di + 2], ax
test_drag_right:
    cmp ax, word ptr [di + 6]
    jle test_drag_top
    mov word ptr [di + 6], ax
test_drag_top:
    mov ax, word ptr [si]
    cmp ax, word ptr [di]
    jge test_drag_bottom
    mov word ptr [di], ax
test_drag_bottom:
    cmp ax, word ptr [di + 4]
    jle begin_mouse_drag
    mov word ptr [di + 4], ax
begin_mouse_drag:
    mov bx, word ptr CURRENT_PORT
    mov ax, 7
    xchg word ptr [bx + 28h], ax
    push ax
    push bx
    push si
    push word ptr [bp + 6]
    xchg di, ax
    mov di, word ptr [bp + 0ah]
    lea bx, [di + 3eh]
    push bx
    push ax
    push ax
    push cs
    call NEAR PTR DRAG_OUTLINE_WITH_MOUSE
    pop bx
    pop word ptr [bx + 28h]
    mov cx, ax
    or ax, dx
    je restore_mouse_drag_port
    push di
    add cx, word ptr [di + 38h]
    push cx
    add dx, word ptr [di + 36h]
    push dx
    push cs
    call NEAR PTR OFFSET_WINDOW_AND_REDRAW
restore_mouse_drag_port:
    pop word ptr CURRENT_PORT
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 6
DRAG_WINDOW_FROM_POINT ENDP

RESTORE_WINDOW_CLIP PROC FAR
    push cs
    call SHOW_MOUSE_GRAPHICS
    pop ax
    pop es
    pop bx
    add bx, 18h
    mov cx, 4
restore_next:
    pop WORD PTR [bx]
    add bx, 2
    loop restore_next
    push es
    push ax
    retf
RESTORE_WINDOW_CLIP ENDP

LOCATE_WINDOW_AT_POINT PROC FAR
    push bp
    mov bp, sp
    push di
    push ds
    push WORD PTR [bp+8]
    mov ax, OFFSET system_hit_rectangle
    push ds
    push ax
    push cs
    call POINT_IN_RECTANGLE
    xchg cx, ax
    jcxz find_hit_window
    mov ax, 1
    sub di, di
    jmp SHORT store_hit_window
find_hit_window:
    mov di, OFFSET WINDOW_LIST
test_next_window:
    mov ax, 0
    mov di, [di+2ch]
    test di, di
    jz store_hit_window
    push di
    mov ax, 3
    push ax
    push WORD PTR [bp+8]
    call DWORD PTR [di+2eh]
    cmp ax, 0
    je test_next_window
store_hit_window:
    mov bx, [bp+6]
    mov [bx], di
    pop di
    pop bp
    retf 4
LOCATE_WINDOW_AT_POINT ENDP

DISPOSE_WINDOW PROC FAR
    push bp
    mov bp, sp
    push WORD PTR [bp+6]
    push cs
    call NEAR PTR CLOSE_WINDOW
    push WORD PTR [bp+6]
    call FREE_NEAR
    add sp, 2
    pop bp
    retf 2
DISPOSE_WINDOW ENDP

SET_WINDOW_EVENT_STATE PROC FAR
    push bp
    mov bp, sp
    push di
    mov di, [bp+8]
    mov cx, [bp+6]
    test cx, cx
    xchg [di+34h], cx
    jcxz check_previous_state
    jne state_done
    jmp SHORT notify_state_change
check_previous_state:
    jz state_done
notify_state_change:
    push di
    mov ax, 2
    push ax
    sub ax, ax
    push ax
    call DWORD PTR [di+2eh]
    mov bx, WINDOW_HEAD
    cmp bx, di
    je state_done
find_previous_window:
    cmp di, [bx+2ch]
    je invalidate_window
    mov bx, [bx+2ch]
    jmp SHORT find_previous_window
invalidate_window:
    lea ax, [di+3eh]
    push ax
    push bx
    push cs
    call NEAR PTR INVALIDATE_WINDOW_RECTANGLE
state_done:
    pop di
    pop bp
    retf 4
SET_WINDOW_EVENT_STATE ENDP

INITIALIZE_WINDOW_DRAW_PORT PROC FAR
    mov ax, WINDOW_DRAW_PORT
    mov CURRENT_PORT, ax
    push ax
    push ds
    mov ax, OFFSET PORT_ORIGIN
    push ax
    push cs
    call INIT_PORT
    push ds
    add ax, 10h
    push ax
    push DRAW_RECORD_RANGE
    push cs
    call SET_RECORD_RANGE
    retf
INITIALIZE_WINDOW_DRAW_PORT ENDP

; Intersect the rectangle with earlier windows before invalidating the current port.
INVALIDATE_WINDOW_BEHIND_PORT PROC FAR
    push bp
    mov bp, sp
    add sp, -8
    push di
    push si
    mov di, word ptr [bp + 6]
    lea si, [bp - 8]
    push ds
    push si
    push ds
    push di
    push cs
    call translate_port_point
    lea ax, [si + 4]
    push ds
    push ax
    lea ax, [di + 4]
    push ds
    push ax
    push cs
    call translate_port_point
    mov di, OFFSET WINDOW_LIST
scan_windows_behind:
    mov di, word ptr [di + 2ch]
    cmp di, word ptr CURRENT_PORT
    je invalidate_port_region
    push ds
    push si
    push ds
    push si
    lea ax, [di + 3eh]
    push ds
    push ax
    push cs
    call SUBTRACT_RECTANGLE_STRIP
    push ds
    push si
    push cs
    call EMPTYRECT
    xchg cx, ax
    jcxz scan_windows_behind
    jmp SHORT return_original_region
invalidate_port_region:
    push si
    push di
    push cs
    call NEAR PTR INVALIDATE_WINDOW_RECTANGLE
return_original_region:
    mov ax, word ptr [bp + 6]
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 2
INVALIDATE_WINDOW_BEHIND_PORT ENDP

INVALIDATE_WINDOW_RECTANGLE PROC FAR
    push bp
    mov bp, sp
    add sp, -8
    push di
    push si
    lea si, [bp-8]
    push ds
    push si
    mov ax, WINDOW_DRAW_PORT
    add ax, 10h
    push ds
    push ax
    push ds
    push WORD PTR [bp+8]
    push cs
    call SECTRECT
    cmp WORD PTR [bp+6], 0
    jne start_windows
    mov ax, WINDOW_DRAW_PORT
    xchg CURRENT_PORT, ax
    push ax
    push ds
    push si
    push DRAW_RECORD_RANGE
    push cs
    call SET_RECORD_RANGE
    pop CURRENT_PORT
start_windows:
    mov cx, WINDOW_HEAD
inspect_window:
    jcxz invalidate_done
    mov di, cx
    lea ax, [bp-8]
    lea si, [di+48h]
    mov cx, [di+46h]
    jcxz clip_window
    push ds
    push si
    push ds
    push si
    push ds
    push ax
    push cs
    call UNIONRECT
clip_window:
    push ds
    push si
    push ds
    push ax
    lea ax, [di+3eh]
    push ds
    push ax
    push cs
    call SECTRECT
    mov [di+46h], ax
    lea ax, [bp-8]
    push ds
    push ax
    push ds
    push ax
    push ds
    push si
    push cs
    call SUBTRACT_RECTANGLE_STRIP
    push ds
    push ax
    push cs
    call EMPTYRECT
    xchg cx, ax
    jcxz invalidate_next_window
    push WORD PTR [bp+8]
    push WORD PTR [di+2ch]
    push cs
    call NEAR PTR TRIM_PENDING_WINDOW_UPDATES
    jmp SHORT invalidate_done
invalidate_next_window:
    mov cx, [di+2ch]
    cmp di, [bp+6]
    jne inspect_window
invalidate_done:
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 4
INVALIDATE_WINDOW_RECTANGLE ENDP

; Track the outline with the arrow keys and return its displacement on Enter.
MOVE_OUTLINE_BY_KEYBOARD PROC FAR
    push bp
    mov bp, sp
    add sp, -1ch
    push di
    push si
    mov ax, 3
    mov bx, word ptr CURRENT_PORT
    xchg word ptr [bx + 26h], ax
    push ax
    push bx
    push cs
    call UPDATE_MESSAGE_DISPLAY
    mov cx, 8
    lea di, [bp - 0ah]
    mov si, word ptr [bp + 8]
    push ds
    pop es
    rep movsb
    mov word ptr [bp - 1ah], cx
    lea di, [bp - 0ah]
    push ds
    push di
    push cs
    call draw_outline_rect
wait_for_move_key:
    lea ax, [bp - 18h]
    push ax
    mov ax, 0ch
    push ax
    push cs
    call READ_QUEUED_EVENT
    xchg cx, ax
    jcxz wait_for_move_key
    cmp word ptr [bp - 18h], 8
    je handle_move_key
cancel_key_move:
    sub ax, ax
    sub dx, dx
finish_key_move:
    push ax
    push dx
    push ds
    push di
    push cs
    call draw_outline_rect
    push cs
    call SHOW_MOUSE_GRAPHICS
    pop dx
    pop ax
    pop bx
    pop word ptr [bx + 26h]
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 4
handle_move_key:
    mov bx, word ptr [bp - 2]
    mov dx, word ptr [bp - 12h]
    mov cx, dx
    xchg word ptr [bp - 1ch], cx
    sub dx, cx
    mov ax, word ptr [bp - 0eh]
    cmp ax, word ptr [bp - 1ah]
    mov word ptr [bp - 1ah], ax
    jne reset_key_repeat
    cmp dx, 3
    ja reset_key_repeat
    cmp bx, 0fh
    jae save_key_repeat
    inc bx
    jmp SHORT save_key_repeat
reset_key_repeat:
    sub bx, bx
save_key_repeat:
    mov word ptr [bp - 2], bx
    inc bx
    sub cx, cx
    sub dx, dx
    mov si, word ptr [bp + 6]
    cmp al, 0dh
    je confirm_key_move
    cmp al, 1bh
    je cancel_key_move
    cmp ax, 4b00h
    je move_outline_left
    cmp ax, 4d00h
    je move_outline_right
    cmp ax, 4800h
    je move_outline_up
    cmp ax, 5000h
    je move_outline_down
poll_move_keys:
    jmp NEAR PTR wait_for_move_key
confirm_key_move:
    mov ax, word ptr [di + 2]
    mov dx, word ptr [di]
    mov bx, word ptr [bp + 8]
    sub ax, word ptr [bx + 2]
    sub dx, word ptr [bx]
    jmp SHORT finish_key_move
move_outline_left:
    sub cx, bx
    jmp SHORT offset_key_outline
move_outline_right:
    add cx, bx
    jmp SHORT offset_key_outline
move_outline_up:
    sub dx, bx
    jmp SHORT offset_key_outline
move_outline_down:
    add dx, bx
offset_key_outline:
    push ds
    push di
    push ds
    push di
    push cx
    push dx
    push ds
    push di
    push cs
    call draw_outline_rect
    push cs
    call offset_rect
    mov cx, word ptr [si + 2]
    sub cx, word ptr [di + 2]
    jg clamp_vertical_offset
    mov cx, word ptr [si + 6]
    sub cx, word ptr [di + 6]
    jl clamp_vertical_offset
    sub cx, cx
clamp_vertical_offset:
    mov dx, word ptr [si]
    sub dx, word ptr [di]
    jg clamp_key_outline
    mov dx, word ptr [si + 4]
    sub dx, word ptr [di + 4]
    jl clamp_key_outline
    sub dx, dx
    jcxz redraw_key_outline
clamp_key_outline:
    push ds
    push di
    push ds
    push di
    push cx
    push dx
    push cs
    call offset_rect
redraw_key_outline:
    push ds
    push di
    push cs
    call draw_outline_rect
    jmp SHORT poll_move_keys
MOVE_OUTLINE_BY_KEYBOARD ENDP

; Draw the moving outline while the chosen mouse button remains down.
DRAG_OUTLINE_WITH_MOUSE PROC FAR
    push bp
    mov bp, sp
    add sp, -12h
    push di
    push si
    mov ax, 3
    mov bx, word ptr CURRENT_PORT
    xchg word ptr [bx + 26h], ax
    push ax
    push bx
    mov cx, 4
    lea di, [bp - 0ch]
    mov si, word ptr [bp + 0eh]
    push ds
    pop es
    rep movsb
    mov word ptr [bp - 12h], cx
    lea di, [bp - 8]
    lea si, [bp - 0ch]
test_drag_point:
    push ds
    push si
    push ds
    push word ptr [bp + 6]
    push cs
    call POINT_IN_RECTANGLE
    cmp di, word ptr [bp - 12h]
    xchg cx, ax
    jcxz drag_point_outside
    jne offset_mouse_outline
    push ds
    push di
    push cs
    call draw_outline_rect
offset_mouse_outline:
    lea ax, [bp - 10h]
    push ds
    push ax
    push ds
    push si
    push ds
    push word ptr [bp + 8]
    push cs
    call subtract_points
    xchg bx, ax
    mov cx, word ptr [bx + 2]
    mov dx, word ptr [bx]
    mov bx, word ptr [bp + 0eh]
    sub cx, word ptr [bx + 2]
    sub dx, word ptr [bx]
    push ds
    push di
    push ds
    push word ptr [bp + 0ah]
    push cx
    push dx
    push cs
    call offset_rect
    mov word ptr [bp - 12h], di
    jmp SHORT redraw_mouse_outline
drag_point_outside:
    jne poll_drag_button
    mov word ptr [bp - 12h], 0
redraw_mouse_outline:
    push ds
    push di
    push cs
    call draw_outline_rect
poll_drag_button:
    push word ptr [bp + 0ch]
    push cs
    call MOUSE_BUTTON_UNQUEUED
    xchg cx, ax
    jcxz finish_mouse_drag
    sub sp, 4
    mov ax, sp
    push ax
    push cs
    call READ_MOUSE_EVENT_POSITION
    push ds
    push ax
    push ds
    push ax
    push cs
    call adjust_mouse_point
    pop dx
    pop cx
    xchg word ptr [si + 2], cx
    xchg word ptr [si], dx
    cmp cx, word ptr [si + 2]
    jne test_new_drag_point
    cmp dx, word ptr [si]
    je poll_drag_button
test_new_drag_point:
    jmp NEAR PTR test_drag_point
finish_mouse_drag:
    sub ax, ax
    sub dx, dx
    cmp ax, word ptr [bp - 12h]
    je restore_drag_mode
    push ds
    push di
    push cs
    call draw_outline_rect
    mov ax, word ptr [di + 2]
    mov dx, word ptr [di]
    mov bx, word ptr [bp + 0ah]
    sub ax, word ptr [bx + 2]
    sub dx, word ptr [bx]
restore_drag_mode:
    pop bx
    pop word ptr [bx + 26h]
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 0ah
DRAG_OUTLINE_WITH_MOUSE ENDP

; Move the window and invalidate its old and new bounds.
OFFSET_WINDOW_AND_REDRAW PROC FAR
    push bp
    mov bp, sp
    add sp, -8
    push di
    push si
    mov ax, word ptr WINDOW_DRAW_PORT
    xchg word ptr CURRENT_PORT, ax
    push ax
    lea di, [bp - 8]
    push di
    sub ax, ax
    push ax
    mov bx, word ptr [bp + 0ah]
    mov cx, 8
    lea si, [bx + 3eh]
    push ds
    pop es
    rep movsb
    mov di, bx
    push di
    push word ptr [bp + 8]
    push word ptr [bp + 6]
    push cs
    call offset_window_rectangle
    push di
    mov ax, 1
    push ax
    push ax
    call DWORD PTR [di + 2eh]
    lea ax, [di + 3eh]
    push ax
    push di
    push cs
    call NEAR PTR INVALIDATE_WINDOW_RECTANGLE
    push cs
    call NEAR PTR INVALIDATE_WINDOW_RECTANGLE
    pop word ptr CURRENT_PORT
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 6
OFFSET_WINDOW_AND_REDRAW ENDP

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
    xchg CURRENT_PORT, ax
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
    call NEAR PTR INVALIDATE_WINDOW_RECTANGLE
    xchg di, ax
    pop CURRENT_PORT
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
    call NEAR PTR BRING_WINDOW_FORWARD
    pop di
    pop bp
    retf 2
ACTIVATE_WINDOW ENDP

; Unlink the window and insert it after the requested predecessor or at the tail.
RELINK_WINDOW_AFTER PROC FAR
    push bp
    mov bp, sp
    push di
    mov di, word ptr [bp + 8]
    cmp di, word ptr WINDOW_HEAD
    jne find_window_predecessor
    mov cx, word ptr [di + 2ch]
    jcxz window_relink_done
    push cx
    push cs
    call NEAR PTR ACTIVATE_WINDOW
find_window_predecessor:
    mov bx, OFFSET WINDOW_LIST
scan_predecessors:
    mov bx, word ptr [bx + 2ch]
    cmp di, word ptr [bx + 2ch]
    jne scan_predecessors
    mov ax, word ptr [di + 2ch]
    mov word ptr [bx + 2ch], ax
    xchg bx, ax
    mov bx, word ptr [bp + 6]
    test bx, bx
    jne insert_window_after
scan_to_window_tail:
    xchg bx, ax
    mov ax, word ptr [bx + 2ch]
    test ax, ax
    jne scan_to_window_tail
insert_window_after:
    mov cx, word ptr [bx + 2ch]
    mov word ptr [bx + 2ch], di
    mov word ptr [di + 2ch], cx
    lea ax, [di + 3eh]
    push ax
    push di
    push cs
    call NEAR PTR INVALIDATE_WINDOW_RECTANGLE
window_relink_done:
    pop di
    pop bp
    retf 4
RELINK_WINDOW_AFTER ENDP

; Release the old title handle, make a handle for the new string and invalidate.
REPLACE_WINDOW_TITLE PROC FAR
    push bp
    mov bp, sp
    push di
    mov di, word ptr [bp + 0ah]
    mov cx, word ptr [di + 5ah]
    jcxz load_title_text
    push cx
    push cs
    call DISPOSE_HANDLE
load_title_text:
    les bx, dword ptr [bp + 6]
    mov ax, es
    or ax, bx
    je store_title_handle
    push es
    push bx
    push cs
    call CREATE_STRING_HANDLE
store_title_handle:
    mov word ptr [di + 5ah], ax
    lea ax, [di + 3eh]
    push ax
    push di
    push cs
    call NEAR PTR INVALIDATE_WINDOW_RECTANGLE
    pop di
    pop bp
    retf 6
REPLACE_WINDOW_TITLE ENDP

; Poll the window callback and notify it when the hit state changes.
TRACK_WINDOW_HIT PROC FAR
    push bp
    mov bp, sp
    add sp, -6
    push di
    push si
    mov cx, 4
    lea di, [bp - 6]
    mov si, word ptr [bp + 8]
    push ds
    pop es
    rep movsb
    mov word ptr [bp - 2], cx
    mov di, word ptr [bp + 0ah]
    lea si, [bp - 6]
send_window_hit_test:
    push di
    mov ax, 3
    push ax
    push si
    call DWORD PTR [di + 2eh]
    sub cx, cx
    cmp ax, 4
    jne update_window_hit_state
    mov cx, 1
update_window_hit_state:
    xchg word ptr [bp - 2], cx
    cmp cx, word ptr [bp - 2]
    je poll_window_hit
    push di
    mov ax, 2
    push ax
    mov ax, 0ffffh
    push ax
    call DWORD PTR [di + 2eh]
poll_window_hit:
    push si
    push cs
    call READ_MOUSE_EVENT_POSITION
    push word ptr [bp + 6]
    push cs
    call MOUSE_BUTTON_UNQUEUED
    test ax, ax
    jne send_window_hit_test
    mov cx, word ptr [bp - 2]
    jcxz return_window_hit
    push di
    mov ax, 2
    push ax
    mov ax, 0ffffh
    push ax
    call DWORD PTR [di + 2eh]
    mov cx, 1
return_window_hit:
    xchg cx, ax
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 6
TRACK_WINDOW_HIT ENDP

; Translate both rectangle points to the current port before trimming updates.
TRANSLATE_WINDOW_CLIP PROC FAR
    push bp
    mov bp, sp
    add sp, -8
    lea ax, [bp - 8]
    push ax
    push word ptr CURRENT_PORT
    mov bx, word ptr [bp + 6]
    push ds
    push ax
    push ds
    push bx
    add ax, 4
    add bx, 4
    push ds
    push ax
    push ds
    push bx
    push cs
    call translate_port_point
    push cs
    call translate_port_point
    push cs
    call NEAR PTR TRIM_PENDING_WINDOW_UPDATES
    mov ax, word ptr [bp + 6]
    mov sp, bp
    pop bp
    retf 2
TRANSLATE_WINDOW_CLIP ENDP

TRIM_PENDING_WINDOW_UPDATES PROC FAR
    push bp
    mov bp, sp
    push di
    lea di, [bp-26h]
trim_next_window:
    mov di, [di+2ch]
    test di, di
    jz update_done
    mov cx, [di+46h]
    jcxz trim_next_window
    lea ax, [di+48h]
    push ds
    push ax
    push ds
    push ax
    push ds
    push WORD PTR [bp+8]
    push cs
    call SUBTRACT_RECTANGLE_STRIP
    push ds
    push ax
    push cs
    call EMPTYRECT
    neg ax
    inc ax
    mov [di+46h], ax
    jmp SHORT trim_next_window
update_done:
    pop di
    pop bp
    retf 4
TRIM_PENDING_WINDOW_UPDATES ENDP

; AX carries the saved background handle. Zero has no background to release.
RELEASE_BACKGROUND_PORT PROC NEAR
    xchg cx, ax
    jcxz release_background_done
    mov ax, word ptr WINDOW_DRAW_PORT
    xchg word ptr CURRENT_PORT, ax
    push ax
    push cx
    push cs
    call release_port_buffer
    pop word ptr CURRENT_PORT
release_background_done:
    ret
RELEASE_BACKGROUND_PORT ENDP

; AX points to the rectangle. Temporarily clear the backing origin for the capture.
ALLOCATE_BACKGROUND_PORT PROC NEAR
    xchg cx, ax
    mov ax, word ptr WINDOW_DRAW_PORT
    xchg word ptr CURRENT_PORT, ax
    push ax
    sub ax, ax
    cwd
    xchg word ptr BACKGROUND_ORIGIN_X, ax
    xchg word ptr BACKGROUND_ORIGIN_Y, dx
    push dx
    push ax
    sub ax, ax
    cwd
    les bx, dword ptr BACKING_PORT_DESCRIPTOR
    xchg word ptr es:[bx + 12h], ax
    xchg word ptr es:[bx + 14h], dx
    push dx
    push ax
    push ds
    push cx
    push cs
    call allocate_port_buffer
    les bx, dword ptr BACKING_PORT_DESCRIPTOR
    pop word ptr es:[bx + 12h]
    pop word ptr es:[bx + 14h]
    pop word ptr BACKGROUND_ORIGIN_X
    pop word ptr BACKGROUND_ORIGIN_Y
    pop word ptr CURRENT_PORT
    ret
ALLOCATE_BACKGROUND_PORT ENDP
END
