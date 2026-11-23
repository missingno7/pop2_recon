EXTRN __nmalloc:FAR
EXTRN __nfree:FAR
EXTRN find_string_byte:FAR

GAMELIB_TEXT SEGMENT BYTE PUBLIC 'CODE'
GAMELIB_TEXT ENDS

; DOS window-library item handlers for menus, grids, sliders and text.
; The callback table holds the four far entries in their linked order.
; All entries have near assembler labels. Far-return entries are called
; within frame 1101 with PUSH CS before CALL.
_DATA SEGMENT WORD PUBLIC 'DATA'
EXTRN Graf:WORD
EXTRN window_draw_port:WORD
EXTRN menu_bounds_top:WORD
EXTRN menu_list:WORD
EXTRN item_state_2618:WORD

; Menu and indexed item state precedes the slider tables in DGROUP.
indexed_item_records       DW 0
indexed_item_state    DW 0
                          DW 0    ; Unreferenced word in the state block.
previous_menu_item        DW 0
menu_selection_row        DW 0
menu_selection_column     DW 0
current_menu_item         DW 0

; Horizontal and vertical slider descriptors select keys and arrow bitmaps.
slider_arrow_inset       DW 8, 12
slider_decrement_key     DW 4800h, 4B00h
slider_decrement_shape   DW OFFSET DGROUP:slider_up_arrow, OFFSET DGROUP:slider_left_arrow
slider_increment_key     DW 5000h, 4D00h
slider_increment_shape   DW OFFSET DGROUP:slider_down_arrow, OFFSET DGROUP:slider_right_arrow

slider_down_arrow        DW 4, 8, 1
                         DB 0FFh, 7Eh, 3Ch, 18h
slider_left_arrow        DW 5, 6, 1
                         DB 0Ch, 3Ch, 0FCh, 3Ch, 0Ch
slider_right_arrow       DW 5, 6, 1
                         DB 0C0h, 0F0h, 0FCh, 0F0h, 0C0h
slider_up_arrow          DW 4, 8, 1
                         DB 18h, 3Ch, 7Eh, 0FFh

_DATA ENDS
DGROUP GROUP _DATA

GAMELIB_TEXT SEGMENT BYTE PUBLIC 'CODE'
ASSUME CS:GAMELIB_TEXT, DS:DGROUP, SS:DGROUP
EXTRN point_in_rectangle:NEAR
EXTRN clamp_point_to_rect:NEAR
EXTRN mouse_button_unqueued:NEAR
EXTRN read_mouse_event_position:NEAR
EXTRN add_port_origin:NEAR
EXTRN scroll_context_rectangle:NEAR
EXTRN push_port_clip:NEAR
EXTRN paint_grid_items:NEAR
EXTRN pop_port_clip:NEAR
EXTRN copy_current_port_origin:NEAR
EXTRN apply_default_pen_state:NEAR
EXTRN SectRect:NEAR
EXTRN set_relative_pen_state:NEAR
EXTRN MEASURE_NUL_TERMINATED_TEXT:NEAR
EXTRN resolve_handle_to_far_payload:NEAR
EXTRN set_record_range:NEAR
EXTRN DRAW_NUL_TERMINATED_TEXT:NEAR
EXTRN toggle_rect_effect:NEAR
EXTRN MoveTo:NEAR
EXTRN update_message_display:NEAR
EXTRN UPPERCASE_EVENT_CHARACTER:NEAR
EXTRN draw_character:NEAR
EXTRN draw_relative_axis_segment:NEAR
EXTRN show_mouse_graphics:NEAR
EXTRN HGETRSRC:NEAR
EXTRN FETCH_LOCKED_SHAPE_LIST:NEAR
EXTRN dispose_handle:NEAR
EXTRN read_queued_event:NEAR
EXTRN RELEASE_BACKGROUND_PORT:NEAR
EXTRN invalidate_window_rectangle:NEAR
EXTRN ALLOCATE_BACKGROUND_PORT:NEAR
EXTRN FRAME_RECT:NEAR
EXTRN skip_nul_strings:NEAR
EXTRN allocate_payload_handle:NEAR
EXTRN release_near_buffer:NEAR
EXTRN CLEAR_HANDLE_OWNED:NEAR
EXTRN InsetRect:NEAR
EXTRN DRAW_NUL_TEXT_IN_RECT:NEAR
EXTRN subtract_port_origin:NEAR
EXTRN stroke_offset_edges:NEAR
EXTRN DRAG_OUTLINE_WITH_MOUSE:NEAR
EXTRN divide_timer_interval:NEAR
EXTRN multiply_timer_interval:NEAR
EXTRN convert_shape:NEAR
EXTRN paint_four_bit:NEAR
EXTRN create_string_handle:NEAR
EXTRN STROKE_PORT_RECT:NEAR
EXTRN draw_text_handle_in_rect:NEAR
EXTRN measure_character:NEAR
EXTRN draw_axis_segment:NEAR

PUBLIC navigate_grid_items
navigate_grid_items LABEL NEAR
    ; Far Pascal (grid, key). Enter returns the current index; arrows and Tab move selection.
    push bp
    mov bp, sp
    add sp, -2
    push di
    push si
    mov di, word ptr [bp + 8]
    mov ax, word ptr [di + 02eh]
    call grid_index_to_coordinates
    xchg cx, ax
    sub si, si
    mov ax, word ptr [bp + 6]
    cmp ax, 04800h
    je step_previous_row
    cmp ax, 05000h
    je step_next_row
    cmp ax, 04b00h
    je step_left_if_possible
    cmp ax, 04d00h
    je step_right_if_possible
    cmp al, 9
    je step_next_item
    cmp ax, 0f00h
    je step_previous_item
    cmp al, 0dh
    je return_current_index
navigation_no_result:
    sub ax, ax
navigation_return:
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 4
return_current_index:
    mov ax, word ptr [di + 02eh]
    jmp SHORT navigation_return
step_previous_row:
    sub si, word ptr [di + 024h]
    jmp SHORT start_navigation
step_next_row:
    mov si, word ptr [di + 024h]
    jmp SHORT start_navigation
step_left_if_possible:
    test dx, dx
    je start_navigation
step_previous_item:
    dec si
    jmp SHORT start_navigation
step_right_if_possible:
    inc dx
    cmp dx, word ptr [di + 024h]
    jge start_navigation
step_next_item:
    inc si
    jmp SHORT start_navigation
start_navigation:
    mov word ptr [bp - 2], 2
    mov cx, word ptr [di + 02eh]
    jcxz start_from_first_visible_item
try_next_grid_index:
    add cx, si
    jle check_navigation_wrap
    cmp cx, word ptr [di + 030h]
    jle check_grid_item_enabled
check_navigation_wrap:
    cmp word ptr [di + 01ch], 0
    je finish_navigation
    dec word ptr [bp - 2]
    je finish_navigation
    test cx, cx
    mov cx, word ptr [di + 030h]
    jle check_grid_item_enabled
    mov cx, 1
check_grid_item_enabled:
    mov ax, 5
    mul cx
    xchg bx, ax
    test byte ptr [bx + di + 02dh], 1
    jne try_next_grid_index
    push di
    push cx
    push cs
    call select_grid_item
finish_navigation:
    jmp SHORT navigation_no_result
start_from_first_visible_item:
    mov ax, word ptr [di + 02ah]
    mov dx, word ptr [di + 02ch]
    call grid_coordinates_to_index
    xchg cx, ax
    dec cx
    mov si, 1
    jmp SHORT try_next_grid_index

PUBLIC draw_grid_item
draw_grid_item LABEL NEAR
    ; Track the mouse against the grid, scrolling when it leaves the visible rectangle.
    push bp
    mov bp, sp
    add sp, -010h
    push di
    push si
    mov di, word ptr [bp + 0ah]
    mov ax, word ptr [di]
    xchg word ptr [Graf], ax
    push ax
    mov bx, word ptr [bp + 8]
    lea si, [bp - 010h]
    mov ax, word ptr [bx + 2]
    mov word ptr [si + 2], ax
    mov ax, word ptr [bx]
    mov word ptr [si], ax
loc_9914:
    push ds
    push si
    lea ax, [di + 8]
    push ds
    push ax
    push cs
    call point_in_rectangle
    mov word ptr [bp - 0ah], ax
    test ax, ax
    jne loc_994e
    sub cx, cx
    sub dx, dx
    mov ax, word ptr [si + 2]
    cmp ax, word ptr [di + 0ah]
    jge loc_9933
    dec cx
loc_9933:
    cmp ax, word ptr [di + 0eh]
    jl loc_9939
    inc cx
loc_9939:
    mov ax, word ptr [si]
    cmp ax, word ptr [di + 8]
    jge loc_9941
    dec dx
loc_9941:
    cmp ax, word ptr [di + 0ch]
    jl loc_9947
    inc dx
loc_9947:
    push di
    push cx
    push dx
    push cs
    call scroll_grid_origin
loc_994e:
    push ds
    push si
    push ds
    push si
    lea ax, [di + 8]
    push ds
    push ax
    push cs
    call clamp_point_to_rect
    sub ax, ax
loc_995d:
    sub dx, dx
loc_995f:
    push ax
    push dx
    add ax, word ptr [di + 02ah]
    add dx, word ptr [di + 02ch]
    lea bx, [bp - 8]
    call grid_cell_rectangle
    push ds
    push si
    push ds
    push bx
    push cs
    call point_in_rectangle
    test ax, ax
    pop dx
    pop ax
    jne loc_9987
    inc dx
    cmp dx, word ptr [di + 028h]
    jl loc_995f
    inc ax
    cmp ax, word ptr [di + 026h]
    jl loc_995d
loc_9987:
    add ax, word ptr [di + 02ah]
    add dx, word ptr [di + 02ch]
    call grid_coordinates_to_index
    mov word ptr [bp - 0ch], ax
    cmp ax, word ptr [di + 030h]
    mov cx, word ptr [di + 01ah]
    jcxz loc_99b1
    jle loc_99a2
    mov ax, word ptr [di + 030h]
    jmp SHORT loc_99ba
loc_99a2:
    xchg bx, ax
    mov ax, 5
    mul bx
    xchg bx, ax
    test byte ptr [bx + di + 02dh], 1
    je loc_99ba
    jmp SHORT loc_99c0
loc_99b1:
    mov cx, word ptr [bp - 0ah]
    jcxz loc_99b8
    jle loc_99ba
loc_99b8:
    sub ax, ax
loc_99ba:
    push di
    push ax
    push cs
    call select_grid_item
loc_99c0:
    mov ax, word ptr [di + 014h]
    or ax, word ptr [di + 016h]
    je loc_99cc
    push di
    call DWORD PTR [di + 014h]
loc_99cc:
    push word ptr [bp + 6]
    push cs
    call mouse_button_unqueued
    xchg cx, ax
    jcxz loc_99e6
    push si
    push cs
    call read_mouse_event_position
    push ds
    push si
    push ds
    push si
    push cs
    call add_port_origin
    jmp NEAR PTR loc_9914
loc_99e6:
    sub ax, ax
    cmp ax, word ptr [bp - 0ah]
    je loc_99f6
    mov cx, word ptr [di + 02eh]
    cmp cx, word ptr [bp - 0ch]
    jne loc_99f6
    xchg cx, ax
loc_99f6:
    pop word ptr [Graf]
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 6

PUBLIC assemble_packed_item
assemble_packed_item LABEL NEAR
    push bp
    mov bp, sp
    push di
    push si
    mov ax, 5
    mov bx, word ptr [bp + 0ah]
    mul word ptr [bx]
    add ax, 032h
    push ax
    push ax
    call FAR PTR __nmalloc
    add sp, 2
    pop cx
    xchg si, ax
    test si, si
    jne loc_9a29
loc_9a22:
    xchg si, ax
    pop si
    pop di
    pop bp
    retf 8
loc_9a29:
    sub ax, ax
    mov di, si
    push ds
    pop es
    rep stosb
    cwd
    mov di, word ptr [bp + 0ah]
    mov ax, word ptr [di]
    mov word ptr [si + 030h], ax
    mov cx, word ptr [di + 2]
    mov word ptr [si + 024h], cx
    div cx
    test dx, dx
    je loc_9a47
    inc ax
loc_9a47:
    mov word ptr [si + 022h], ax
    push di
    push si
    mov cx, 8
    lea di, [si + 8]
    mov si, word ptr [bp + 0ch]
    push ds
    pop es
    rep movsb
    pop si
    pop di
    dec cx
    mov word ptr [si + 018h], cx
    mov bx, word ptr [Graf]
    mov word ptr [si], bx
    mov ax, word ptr [bx + 02ah]
    mov word ptr [si + 2], ax
    mov ax, word ptr [bx + 020h]
    mov word ptr [si + 4], ax
    mov ax, word ptr [bx + 028h]
    mov word ptr [si + 6], ax
    mov ax, word ptr [si + 0eh]
    sub ax, word ptr [si + 0ah]
    mov cx, word ptr [di + 6]
    mov word ptr [si + 020h], cx
    sub dx, dx
    div cx
    test dx, dx
    je loc_9a8c
    inc ax
loc_9a8c:
    cmp ax, word ptr [si + 024h]
    jle loc_9a94
    mov ax, word ptr [si + 024h]
loc_9a94:
    mov word ptr [si + 028h], ax
    mov ax, word ptr [si + 0ch]
    sub ax, word ptr [si + 8]
    mov cx, word ptr [di + 4]
    mov word ptr [si + 01eh], cx
    sub dx, dx
    div cx
    test dx, dx
    je loc_9aac
    inc ax
loc_9aac:
    cmp ax, word ptr [si + 022h]
    jle loc_9ab4
    mov ax, word ptr [si + 022h]
loc_9ab4:
    mov word ptr [si + 026h], ax
    sub dx, dx
    les di, DWORD PTR [bp + 6]
loc_9abc:
    inc dx
    cmp dx, word ptr [si + 030h]
    jle loc_9ac5
    jmp NEAR PTR loc_9a22
loc_9ac5:
    push dx
    push es
    push si
    push dx
    push es
    push di
    call copy_packed_item_rects
    pop es
    pop dx
    sub al, al
    mov cx, 0ffffh
    repnz scasb
    jmp SHORT loc_9abc

PUBLIC scroll_grid_origin
scroll_grid_origin LABEL NEAR
    push bp
    mov bp, sp
    add sp, -0ch
    push di
    mov di, word ptr [bp + 0ah]
    mov ax, word ptr [bp + 6]
    mov dx, word ptr [bp + 8]
    add dx, word ptr [di + 02ch]
    jge loc_9af0
    sub dx, dx
loc_9af0:
    mov cx, word ptr [di + 028h]
    add cx, dx
    sub cx, word ptr [di + 024h]
    jle loc_9afc
    sub dx, cx
loc_9afc:
    add ax, word ptr [di + 02ah]
    jge loc_9b03
    sub ax, ax
loc_9b03:
    mov cx, word ptr [di + 026h]
    add cx, ax
    sub cx, word ptr [di + 022h]
    jle loc_9b0f
    sub ax, cx
loc_9b0f:
    sub ax, word ptr [di + 02ah]
    add word ptr [di + 02ah], ax
    mov word ptr [bp - 4], ax
    sub dx, word ptr [di + 02ch]
    add word ptr [di + 02ch], dx
    mov word ptr [bp - 2], dx
    or ax, dx
    je loc_9b62
    mov ax, word ptr [di]
    xchg word ptr [Graf], ax
    push ax
    lea ax, [di + 8]
    push ds
    push ax
    mov ax, word ptr [di + 020h]
    imul word ptr [bp - 2]
    neg ax
    push ax
    mov ax, word ptr [di + 01eh]
    imul word ptr [bp - 4]
    neg ax
    push ax
    lea ax, [bp - 0ch]
    push ds
    push ax
    push cs
    call scroll_context_rectangle
    lea ax, [bp - 0ch]
    push ds
    push ax
    push cs
    call push_port_clip
    push di
    push cs
    call paint_grid_items
    push cs
    call pop_port_clip
    pop word ptr [Graf]
loc_9b62:
    les ax, DWORD PTR [bp - 4]
    mov dx, es
    pop di
    mov sp, bp
    pop bp
    retf 6

PUBLIC select_grid_item
select_grid_item LABEL NEAR
    push bp
    mov bp, sp
    push di
    mov di, word ptr [bp + 8]
    mov bx, word ptr [di]
    mov ax, word ptr [di + 2]
    xchg word ptr [bx + 02ah], ax
    push ax
    xchg word ptr [Graf], bx
    push bx
    lea ax, [di + 8]
    push ds
    push ax
    push cs
    call push_port_clip
    mov cx, word ptr [bp + 6]
    jcxz loc_9ba7
    cmp cx, word ptr [di + 02eh]
    je loc_9be9
    cmp cx, word ptr [di + 030h]
    jg loc_9ba7
    mov ax, 5
    mul cx
    xchg bx, ax
    test byte ptr [bx + di + 02dh], 1
    je loc_9ba9
loc_9ba7:
    sub cx, cx
loc_9ba9:
    xchg word ptr [di + 02eh], cx
    jcxz loc_9bb2
    xchg cx, ax
    call invalidate_grid_cell
loc_9bb2:
    mov cx, word ptr [di + 02eh]
    jcxz loc_9be9
    xchg cx, ax
    call grid_index_to_coordinates
    sub dx, word ptr [di + 02ch]
    jl loc_9bc8
    inc dx
    sub dx, word ptr [di + 028h]
    jge loc_9bc8
    sub dx, dx
loc_9bc8:
    sub ax, word ptr [di + 02ah]
    jl loc_9bd5
    inc ax
    sub ax, word ptr [di + 026h]
    jge loc_9bd5
    sub ax, ax
loc_9bd5:
    sub cx, cx
    xchg word ptr [di + 02eh], cx
    push cx
    push di
    push dx
    push ax
    push cs
    call scroll_grid_origin
    pop ax
    mov word ptr [di + 02eh], ax
    call invalidate_grid_cell
loc_9be9:
    push cs
    call pop_port_clip
    pop bx
    xchg word ptr [Graf], bx
    pop word ptr [bx + 02ah]
    pop di
    pop bp
    retf 4

PUBLIC update_item_vector
update_item_vector LABEL NEAR
    push bp
    mov bp, sp
    push di
    mov di, word ptr [bp + 0ch]
    push di
    push word ptr [bp + 0ah]
    les ax, DWORD PTR [bp + 6]
    push es
    push ax
    call copy_packed_item_rects
    mov ax, word ptr [bp + 0ah]
    call grid_index_to_coordinates
    push di
    push dx
    push ax
    call draw_grid_items
    pop di
    pop bp
    retf 8

PUBLIC grid_coordinates_to_index
grid_coordinates_to_index LABEL NEAR
    ; AX = row * column_count + column + 1. DI = grid, AX = row, DX = column.
    mov cx, dx
    mul word ptr [di + 024h]
    add ax, cx
    inc ax
    ret

PUBLIC grid_cell_rectangle
grid_cell_rectangle LABEL NEAR
    ; Convert row and column to the cell rectangle at DS:BX.
    push dx
    sub ax, word ptr [di + 02ah]
    mul word ptr [di + 01eh]
    add ax, word ptr [di + 8]
    mov word ptr [bx], ax
    add ax, word ptr [di + 01eh]
    mov word ptr [bx + 4], ax
    pop ax
    sub ax, word ptr [di + 02ch]
    mul word ptr [di + 020h]
    add ax, word ptr [di + 0ah]
    mov word ptr [bx + 2], ax
    add ax, word ptr [di + 020h]
    mov word ptr [bx + 6], ax
    ret

PUBLIC draw_grid_items
draw_grid_items LABEL NEAR
    push bp
    mov bp, sp
    add sp, -016h
    push di
    push si
    mov di, word ptr [bp + 8]
    mov si, word ptr [di]
    mov ax, word ptr [di + 2]
    xchg word ptr [si + 02ah], ax
    push ax
    mov ax, si
    xchg word ptr [Graf], ax
    push ax
    lea ax, [bp - 0ah]
    push ds
    push ax
    push ds
    push ax
    push cs
    call copy_current_port_origin
    push cs
    call apply_default_pen_state
    lea bx, [bp - 012h]
    les ax, DWORD PTR [bp + 4]
    mov dx, es
    call grid_cell_rectangle
    push ds
    push bx
    push ds
    push bx
    push ds
    push bx
    lea ax, [di + 8]
    push ds
    push ax
    push cs
    call SectRect
    push cs
    call push_port_clip
    les ax, DWORD PTR [bp + 4]
    mov dx, es
    call grid_coordinates_to_index
    mov word ptr [bp - 2], ax
    cmp ax, word ptr [di + 030h]
    jle loc_9cbc
loc_9ca5:
    push cs
    call pop_port_clip
    push cs
    call set_relative_pen_state
    pop word ptr [Graf]
    pop word ptr [si + 02ah]
    pop si
    pop di
    mov sp, bp
    pop bp
    ret 6
loc_9cbc:
    mov cx, 5
    mul cx
    xchg bx, ax
    mov ax, word ptr [di + 6]
    test byte ptr [bx + di + 02dh], 1
    je loc_9cce
    mov ax, 7
loc_9cce:
    mov word ptr [si + 028h], ax
    les ax, DWORD PTR [bx + di + 02eh]
    mov word ptr [bp - 016h], ax
    mov word ptr [bp - 014h], es
    push es
    push ax
    push cs
    call MEASURE_NUL_TERMINATED_TEXT
    push ax
    push word ptr [di + 2]
    push cs
    call resolve_handle_to_far_payload
    xchg bx, ax
    mov es, dx
    pop ax
    mov dx, word ptr [bp - 0eh]
    mov cx, dx
    sub cx, word ptr [bp - 012h]
    shr cx, 1
    sbb dx, cx
    mov cx, word ptr es:[bx + 2]
    add cx, word ptr es:[bx + 4]
    shr cx, 1
    adc dx, cx
    sub dx, word ptr es:[bx + 4]
    mov bx, word ptr [bp - 010h]
    mov cx, word ptr [bp - 0ch]
    cmp word ptr [di + 018h], 0
    jl loc_9d22
    jg loc_9d29
    sub cx, bx
    shr cx, 1
    add cx, bx
    shr ax, 1
    sub cx, ax
    jmp SHORT loc_9d2e
loc_9d22:
    mov cx, bx
    add cx, 2
    jmp SHORT loc_9d2e
loc_9d29:
    sub cx, ax
    sub cx, 2
loc_9d2e:
    mov word ptr [si + 024h], cx
    mov word ptr [si + 022h], dx
    lea bx, [bp - 012h]
    mov ax, word ptr [di + 010h]
    or ax, word ptr [di + 012h]
    je loc_9d49
    push di
    push word ptr [bp - 2]
    push bx
    call DWORD PTR [di + 010h]
    jmp SHORT loc_9d5b
loc_9d49:
    push ds
    push bx
    push word ptr [di + 4]
    push cs
    call set_record_range
    les ax, DWORD PTR [bp - 016h]
    push es
    push ax
    push cs
    call DRAW_NUL_TERMINATED_TEXT
loc_9d5b:
    mov ax, word ptr [bp - 2]
    cmp ax, word ptr [di + 02eh]
    jne loc_9d66
    call invalidate_grid_cell
loc_9d66:
    jmp NEAR PTR loc_9ca5

PUBLIC invalidate_grid_cell
invalidate_grid_cell LABEL NEAR
    push bp
    mov bp, sp
    add sp, -8
    call grid_index_to_coordinates
    lea bx, [bp - 8]
    call grid_cell_rectangle
    push ds
    push bx
    push cs
    call toggle_rect_effect
    mov sp, bp
    pop bp
    ret

PUBLIC grid_index_to_coordinates
grid_index_to_coordinates LABEL NEAR
    ; AX = one-based item index. Return zero-based row in AX and column in DX.
    dec ax
    sub dx, dx
    div word ptr [di + 024h]
    ret

PUBLIC copy_packed_item_rects
copy_packed_item_rects LABEL NEAR
    push bp
    mov bp, sp
    mov bx, word ptr [bp + 0ah]
    mov cx, word ptr [bp + 8]
    jcxz loc_9da9
    cmp cx, word ptr [bx + 030h]
    jg loc_9da9
    mov ax, 5
    mul cx
    add bx, ax
    les ax, DWORD PTR [bp + 4]
    mov word ptr [bx + 02eh], ax
    mov word ptr [bx + 030h], es
loc_9da9:
    pop bp
    ret 8
PUBLIC align_9dad
align_9dad db 0   ; Inter-entry fill in the original image

PUBLIC redraw_dialog_scroll
redraw_dialog_scroll LABEL NEAR
    push bp
    mov bp, sp
    add sp, -8
    push di
    push si
    mov bx, word ptr [window_draw_port]
    sub ax, ax
    xchg word ptr [bx + 018h], ax
    push ax
    push word ptr [bx + 028h]
    xchg word ptr [Graf], bx
    push bx
    mov si, OFFSET DGROUP:menu_bounds_top
    push ds
    push si
    mov ax, 0fh
    push ax
    push cs
    call set_record_range
    mov ax, word ptr [si + 4]
    dec ax
    xchg word ptr [si], ax
    push ax
    push ds
    push si
    mov ax, 0
    push ax
    push cs
    call set_record_range
    pop word ptr [si]
    mov di, word ptr [menu_list]
    lea si, [bp - 8]
loc_9def:
    test di, di
    je loc_9e44
    call calculate_menu_bounds
    mov ax, 0
    test byte ptr [di + 014h], 1
    jne loc_9e02
    mov ax, 7
loc_9e02:
    mov bx, word ptr [Graf]
    mov word ptr [bx + 028h], ax
    push word ptr [bx + 02ah]
    push cs
    call resolve_handle_to_far_payload
    xchg bx, ax
    mov es, dx
    mov ax, word ptr [si + 2]
    add ax, 2
    push ax
    mov ax, word ptr [si]
    add ax, word ptr es:[bx + 2]
    inc ax
    push ax
    push cs
    call MoveTo
    mov bx, word ptr [di + 01ch]
    lea ax, [bx + 3]
    push ax
    push word ptr [bx + 1]
    push cs
    call draw_dialog_scroll_region
    cmp di, word ptr [current_menu_item]
    jne loc_9e40
    push ds
    push si
    push cs
    call toggle_rect_effect
loc_9e40:
    mov di, word ptr [di]
    jmp SHORT loc_9def
loc_9e44:
    pop bx
    xchg word ptr [Graf], bx
    pop word ptr [bx + 028h]
    pop word ptr [bx + 018h]
    pop si
    pop di
    mov sp, bp
    pop bp
    retf

PUBLIC draw_dialog_scroll_region
draw_dialog_scroll_region LABEL NEAR
    push bp
    mov bp, sp
    add sp, -2
    push di
    push si
    push cs
    call update_message_display
    sub ax, ax
    mov word ptr [bp - 2], ax
    mov al, byte ptr [bp + 6]
    push ax
    push cs
    call UPPERCASE_EVENT_CHARACTER
    mov word ptr [bp + 6], ax
    mov si, word ptr [bp + 8]
loc_9e74:
    sub ah, ah
    lodsb
    test ax, ax
    je loc_9ece
    push ax
    xchg di, ax
    push cs
    call UPPERCASE_EVENT_CHARACTER
    cmp ax, word ptr [bp + 6]
    jne loc_9ec4
    mov ax, word ptr [Graf]
    xchg di, ax
    mov dx, word ptr [di + 028h]
    cmp dx, 7
    je loc_9e97
    mov word ptr [di + 028h], 4
loc_9e97:
    push dx
    push ax
    push cs
    call draw_character
    add word ptr [bp - 2], ax
    push word ptr [di + 024h]
    push word ptr [di + 022h]
    dec word ptr [di + 024h]
    inc word ptr [di + 022h]
    neg ax
    push ax
    sub ax, ax
    push ax
    mov word ptr [bp + 6], ax
    push cs
    call draw_relative_axis_segment
    pop word ptr [di + 022h]
    pop word ptr [di + 024h]
    pop word ptr [di + 028h]
    jmp SHORT loc_9e74
loc_9ec4:
    push di
    push cs
    call draw_character
    add word ptr [bp - 2], ax
    jmp SHORT loc_9e74
loc_9ece:
    push cs
    call show_mouse_graphics
    mov ax, word ptr [bp - 2]
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 4

PUBLIC set_grid_item_location
set_grid_item_location LABEL NEAR
    push bp
    mov bp, sp
    push di
    mov di, word ptr [bp + 0ah]
    mov cx, word ptr [bp + 8]
    cmp cx, word ptr [di + 01ah]
    ja loc_9f21
    mov ax, 0fffeh
    cwd
    jcxz loc_9ef9
    stc
loc_9ef3:
    rcl ax, 1
    rcl dx, 1
    loop loc_9ef3
loc_9ef9:
    mov bx, word ptr [di + 014h]
    and bx, ax
    and word ptr [di + 016h], dx
    mov cx, word ptr [bp + 6]
    jcxz loc_9f0f
    not ax
    not dx
    or bx, ax
    or word ptr [di + 016h], dx
loc_9f0f:
    xchg word ptr [di + 014h], bx
    cmp bx, word ptr [di + 014h]
    je loc_9f21
    cmp word ptr [bp + 8], 0
    jne loc_9f21
    push cs
    call redraw_dialog_scroll
loc_9f21:
    pop di
    pop bp
    retf 6

PUBLIC load_menu_handle
load_menu_handle LABEL NEAR
    push bp
    mov bp, sp
    mov ax, 04d45h
    push ax
    mov ax, 04e55h
    push ax
    push word ptr [bp + 6]
    push cs
    call HGETRSRC
    test ax, ax
    je loc_9f5d
    push ax
    push ax
    push cs
    call FETCH_LOCKED_SHAPE_LIST
    xchg bx, ax
    mov es, dx
    push word ptr [bp + 6]
    sub ax, ax
    mov al, byte ptr es:[bx]
    push ax
    add bx, 1
    push es
    push bx
    push cs
    call draw_packed_item
    xchg bp, ax
    push cs
    call dispose_handle
    xchg bp, ax
loc_9f5d:
    pop bp
    retf 2

PUBLIC redraw_menu_items
redraw_menu_items LABEL NEAR
    push bp
    mov bp, sp
    add sp, -8
    push di
    push si
    sub ax, ax
    mov bx, word ptr [window_draw_port]
    xchg word ptr [bx + 018h], ax
    xchg word ptr [Graf], bx
    push ax
    push bx
    sub di, di
    lea si, [bp - 8]
    mov cx, word ptr [bp + 6]
    jcxz loc_9fa6
    mov di, word ptr [menu_list]
loc_9f86:
    test di, di
    je loc_9fa6
    cmp cx, word ptr [di + 8]
    je loc_9f93
    mov di, word ptr [di]
    jmp SHORT loc_9f86
loc_9f93:
    cmp di, word ptr [current_menu_item]
    je loc_9fba
    call calculate_menu_bounds
    push ds
    push si
    push cs
    call toggle_rect_effect
    mov word ptr [previous_menu_item], di
loc_9fa6:
    xchg word ptr [current_menu_item], di
    test di, di
    je loc_9fba
    call calculate_menu_bounds
    push ds
    push si
    push cs
    call toggle_rect_effect
    call close_tracking_window
loc_9fba:
    pop bx
    xchg word ptr [Graf], bx
    pop word ptr [bx + 018h]
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 2

PUBLIC initialize_menu_bounds
initialize_menu_bounds LABEL NEAR
    ; Copy the window bounds and extend the menu edge for the active port.
    push di
    push si
    mov bx, word ptr [window_draw_port]
    mov cx, 4
    lea si, [bx + 010h]
    mov di, OFFSET DGROUP:menu_bounds_top
    push ds
    pop es
    rep movsw
    mov di, bx
    push word ptr [di + 02ah]
    push cs
    call resolve_handle_to_far_payload
    xchg bx, ax
    mov es, dx
    mov ax, word ptr es:[bx + 2]
    add ax, word ptr es:[bx + 4]
    mov dx, 2
    add ax, dx
    sub dx, word ptr es:[bx + 4]
    inc ax
    jle loc_9fff
    add ax, dx
loc_9fff:
    add ax, word ptr [menu_bounds_top]
    mov word ptr [di + 018h], ax
    mov word ptr [indexed_item_state], ax
    push cs
    call redraw_dialog_scroll
    pop si
    pop di
    retf

PUBLIC set_indexed_item_position
set_indexed_item_position LABEL NEAR
    ; Link a menu item at the first matching item ID, or at the end of the list.
    push bp
    mov bp, sp
    push di
    push si
    mov cx, word ptr [bp + 6]
    mov si, OFFSET DGROUP:menu_list
loc_a01b:
    mov di, si
    mov si, word ptr [di]
    test si, si
    je loc_a02a
    jcxz loc_a01b
    cmp cx, word ptr [si + 8]
    jne loc_a01b
loc_a02a:
    mov bx, word ptr [bp + 8]
    mov word ptr [di], bx
    mov word ptr [bx], si
    call redraw_indexed_item
    pop si
    pop di
    pop bp
    retf 4

PUBLIC update_menu_tracking
update_menu_tracking LABEL NEAR
    push bp
    mov bp, sp
    add sp, -0eh
    push di
    push cs
    call update_message_display
    mov ax, word ptr [window_draw_port]
    xchg word ptr [Graf], ax
    push ax
    mov bx, word ptr [bp + 6]
    mov cx, word ptr [bx]
    jcxz loc_a095
    xchg cx, ax
    call open_tracking_window
    test di, di
    jne loc_a085
    xchg di, ax
    cwd
loc_a05e:
    push ax
    push dx
    call close_tracking_window
    push cs
    call show_mouse_graphics
    pop dx
    pop ax
    pop word ptr [Graf]
    pop di
    mov sp, bp
    pop bp
    retf 2
loc_a074:
    sub ax, ax
    push ax
    push cs
    call redraw_menu_items
    sub ax, ax
    cwd
    mov bx, word ptr [bp + 6]
    mov word ptr [bx], ax
    jmp SHORT loc_a05e
loc_a085:
    mov cx, word ptr [di + 018h]
    jcxz loc_a0a5
loc_a08a:
    mov cx, word ptr [di + 018h]
    jcxz loc_a10b
    xchg cx, ax
    mov dx, word ptr [di + 8]
    jmp SHORT loc_a05e
loc_a095:
    mov di, word ptr [previous_menu_item]
    test di, di
    jne loc_a0a5
    mov di, word ptr [menu_list]
    test di, di
    je loc_a074
loc_a0a5:
    mov ax, 1
    and ax, word ptr [di + 014h]
    je loc_a0c3
    mov cx, word ptr [di + 018h]
    jcxz loc_a0c6
    cwd
loc_a0b3:
    shl ax, 1
    rcl dx, 1
    loop loc_a0b3
    and ax, word ptr [di + 014h]
    jne loc_a0c6
    and dx, word ptr [di + 016h]
    jne loc_a0c6
loc_a0c3:
    mov word ptr [di + 018h], ax
loc_a0c6:
    push word ptr [di + 8]
    push cs
    call redraw_menu_items
    call refresh_tracking_item
loc_a0d0:
    lea ax, [bp - 0eh]
    push ax
    mov ax, 0ch
    push ax
    push cs
    call read_queued_event
    xchg cx, ax
    jcxz loc_a0d0
    cmp word ptr [bp - 0eh], 4
    je loc_a074
    mov ax, word ptr [bp - 4]
    call open_tracking_window
    test di, di
    jne loc_a085
    mov di, word ptr [current_menu_item]
    mov ax, word ptr [bp - 4]
    cmp al, 0dh
    je loc_a08a
    cmp al, 01bh
    jne loc_a101
    jmp NEAR PTR loc_a074
loc_a101:
    cmp ax, 04800h
    je loc_a13a
    cmp ax, 05000h
    jne loc_a15f
loc_a10b:
    mov cx, word ptr [di + 018h]
loc_a10e:
    inc cx
    cmp cx, word ptr [di + 01ah]
    ja loc_a138
    mov ax, 1
    test word ptr [di + 014h], ax
    je loc_a138
    cwd
    push cx
loc_a11e:
    shl ax, 1
    rcl dx, 1
    loop loc_a11e
    pop cx
    test word ptr [di + 014h], ax
    jne loc_a12f
    test word ptr [di + 016h], dx
    je loc_a10e
loc_a12f:
    push di
    mov ax, 4
    push ax
    push cx
    call DWORD PTR [di + 2]
loc_a138:
    jmp SHORT loc_a0d0
loc_a13a:
    mov cx, word ptr [di + 018h]
    jcxz loc_a12f
loc_a13f:
    dec cx
    je loc_a12f
    mov ax, 1
    test word ptr [di + 014h], ax
    je loc_a13f
    cwd
    push cx
loc_a14c:
    shl ax, 1
    rcl dx, 1
    loop loc_a14c
    pop cx
    test word ptr [di + 014h], ax
    jne loc_a12f
    test word ptr [di + 016h], dx
    jne loc_a12f
    jmp SHORT loc_a13f
loc_a15f:
    cmp al, 9
    je loc_a185
    cmp ax, 04d00h
    je loc_a185
    cmp ax, 0f00h
    je loc_a172
    cmp ax, 04b00h
    jne loc_a138
loc_a172:
    mov bx, OFFSET DGROUP:menu_list
loc_a175:
    cmp di, word ptr [bx]
    je loc_a17d
    mov bx, word ptr [bx]
    jmp SHORT loc_a175
loc_a17d:
    cmp bx, OFFSET DGROUP:menu_list
    jne loc_a18b
    jmp SHORT loc_a138
loc_a185:
    mov bx, word ptr [di]
    test bx, bx
    je loc_a138
loc_a18b:
    mov di, bx
    mov word ptr [di + 018h], 0
    jmp NEAR PTR loc_a0a5

PUBLIC draw_menu_item
draw_menu_item LABEL NEAR
    push bp
    mov bp, sp
    add sp, -0ch
    push di
    push si
    mov ax, word ptr [window_draw_port]
    xchg word ptr [Graf], ax
    push ax
    push ds
    pop es
    mov si, word ptr [bp + 8]
    lea di, [bp - 0ch]
    mov cx, 4
    rep movsb
loc_a1b2:
    mov di, word ptr [menu_list]
    lea si, [bp - 0ch]
loc_a1b9:
    test di, di
    je loc_a1eb
    push si
    lea si, [bp - 8]
    call calculate_menu_bounds
    xchg si, ax
    pop si
    push ds
    push si
    push ds
    push ax
    push cs
    call point_in_rectangle
    test ax, ax
    jne loc_a1d6
    mov di, word ptr [di]
    jmp SHORT loc_a1b9
loc_a1d6:
    cmp di, word ptr [current_menu_item]
    je loc_a1e8
    push word ptr [di + 8]
    push cs
    call redraw_menu_items
    mov word ptr [di + 018h], 0
loc_a1e8:
    call refresh_tracking_item
loc_a1eb:
    mov di, word ptr [current_menu_item]
    test di, di
    je loc_a202
    mov cx, word ptr [menu_selection_row]
    jcxz loc_a202
    push di
    mov ax, 3
    push ax
    push si
    call DWORD PTR [di + 2]
loc_a202:
    push word ptr [bp + 6]
    push cs
    call mouse_button_unqueued
    xchg cx, ax
    jcxz loc_a213
    push si
    push cs
    call read_mouse_event_position
    jmp SHORT loc_a1b2
loc_a213:
    call close_tracking_window
    mov bx, word ptr [current_menu_item]
    test bx, bx
    je loc_a22a
    mov ax, word ptr [bx + 018h]
    test ax, ax
    je loc_a22a
    mov dx, word ptr [bx + 8]
    jmp SHORT loc_a235
loc_a22a:
    sub ax, ax
    push ax
    push cs
    call redraw_menu_items
    sub ax, ax
    sub dx, dx
loc_a235:
    pop word ptr [Graf]
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 4

PUBLIC draw_packed_item
draw_packed_item LABEL NEAR
    push bp
    mov bp, sp
    push di
    push si
    mov ax, word ptr [bp + 0ah]
    cmp ax, 020h
    jbe loc_a251
    mov ax, 020h
loc_a251:
    mov si, ax
    dec si
    shl ax, 1
    je loc_a2c6
    add ax, 01ch
    push ax
    push ax
    call FAR PTR __nmalloc
    add sp, 2
    pop cx
    test ax, ax
    je loc_a2c6
    xchg di, ax
    push di
    sub ax, ax
    push ds
    pop es
    rep stosb
    pop di
    mov ax, word ptr [bp + 0ch]
    mov word ptr [di + 8], ax
    mov word ptr [di + 01ah], si
    dec cx
    mov word ptr [di + 014h], cx
    mov word ptr [di + 016h], cx
    mov ax, 04d44h
    push ax
    mov ax, 04546h
    push ax
    mov ax, 0
    push ax
    push cs
    call HGETRSRC
    push ax
    push cs
    call resolve_handle_to_far_payload
    xchg bx, ax
    mov es, dx
    les ax, DWORD PTR es:[bx]
    mov word ptr [di + 2], ax
    mov word ptr [di + 4], es
    sub ax, ax
    mov si, word ptr [bp + 6]
loc_a2a9:
    push di
    push ax
    push word ptr [bp + 8]
    push si
    xchg si, ax
    call replace_packed_item
    xchg si, ax
    inc ax
    cmp ax, word ptr [di + 01ah]
    jbe loc_a2a9
    push di
    mov ax, 0
    push ax
    sub ax, ax
    push ax
    call DWORD PTR [di + 2]
    xchg di, ax
loc_a2c6:
    pop si
    pop di
    pop bp
    retf 8

PUBLIC store_byte_in_indexed_slot
store_byte_in_indexed_slot LABEL NEAR
    ; Far Pascal (item, slot, value). Store the low byte if the slot is in range.
    push bp
    mov bp, sp
    push di
    mov di, word ptr [bp + 0ah]
    mov bx, word ptr [bp + 8]
    cmp bx, word ptr [di + 01ah]
    ja loc_a2e5
    shl bx, 1
    mov bx, word ptr [bx + di + 01ch]
    mov al, byte ptr [bp + 6]
    mov byte ptr [bx], al
loc_a2e5:
    pop di
    pop bp
    retf 6

PUBLIC set_indexed_item_value
set_indexed_item_value LABEL NEAR
    push bp
    mov bp, sp
    push di
    push si
    mov di, word ptr [bp + 0ch]
    mov ax, word ptr [bp + 0ah]
    cmp ax, word ptr [di + 01ah]
    ja loc_a31b
    push di
    push ax
    les ax, DWORD PTR [bp + 6]
    push es
    push ax
    call replace_packed_item
    mov cx, word ptr [bp + 0ah]
    jcxz loc_a314
    push di
    mov ax, 1
    push ax
    push ax
    call DWORD PTR [di + 2]
    jmp SHORT loc_a31b
loc_a314:
    call redraw_indexed_item
    push cs
    call redraw_dialog_scroll
loc_a31b:
    pop si
    pop di
    pop bp
    retf 8

PUBLIC redraw_indexed_item
redraw_indexed_item LABEL NEAR
    push di
    push si
    mov ax, word ptr [window_draw_port]
    xchg word ptr [Graf], ax
    push ax
    mov di, word ptr [menu_list]
    mov si, word ptr [indexed_item_records]
    add si, 4
loc_a336:
    test di, di
    je loc_a363
    mov word ptr [di + 0eh], si
    mov ax, word ptr [indexed_item_state]
    mov word ptr [di + 0ch], ax
    push di
    mov ax, 1
    push ax
    sub ax, ax
    push ax
    call DWORD PTR [di + 2]
    mov ax, word ptr [di + 01ch]
    add ax, 3
    push ds
    push ax
    push cs
    call MEASURE_NUL_TERMINATED_TEXT
    add ax, 6
    add si, ax
    mov di, word ptr [di]
    jmp SHORT loc_a336
loc_a363:
    pop word ptr [Graf]
    pop si
    pop di
    ret

PUBLIC calculate_menu_bounds
calculate_menu_bounds LABEL NEAR
    mov ax, word ptr [di + 01ch]
    add ax, 3
    push ds
    push ax
    push cs
    call MEASURE_NUL_TERMINATED_TEXT
    mov cx, word ptr [di + 0eh]
    mov word ptr [si + 2], cx
    add ax, cx
    add ax, 6
    mov word ptr [si + 6], ax
    mov ax, word ptr [menu_bounds_top]
    mov word ptr [si], ax
    mov ax, word ptr [indexed_item_state]
    dec ax
    mov word ptr [si + 4], ax
    ret

PUBLIC close_tracking_window
close_tracking_window LABEL NEAR
    sub cx, cx
    xchg word ptr [menu_selection_row], cx
    jcxz loc_a3b1
    sub ax, ax
    xchg word ptr [menu_selection_column], ax
    test ax, ax
    je loc_a3a8
    call RELEASE_BACKGROUND_PORT
    jmp SHORT loc_a3b1
loc_a3a8:
    add cx, 0ch
    push cx
    push ax
    push cs
    call invalidate_window_rectangle
loc_a3b1:
    ret

PUBLIC open_tracking_window
open_tracking_window LABEL NEAR
    push ax
    push cs
    call UPPERCASE_EVENT_CHARACTER
    mov di, word ptr [menu_list]
loc_a3bb:
    test di, di
    je loc_a434
    sub cx, cx
loc_a3c1:
    mov bx, cx
    shl bx, 1
    mov bx, word ptr [bx + di + 01ch]
    test ah, ah
    je loc_a3d5
    cmp ah, byte ptr [bx + 2]
    jne loc_a3d5
    jcxz loc_a3f0
    jmp SHORT loc_a3fc
loc_a3d5:
    test al, al
    je loc_a3e6
    cmp al, byte ptr [bx + 1]
    jne loc_a3e6
    jcxz loc_a3f0
    cmp di, word ptr [current_menu_item]
    je loc_a3fc
loc_a3e6:
    inc cx
    cmp cx, word ptr [di + 01ah]
    jbe loc_a3c1
loc_a3ec:
    mov di, word ptr [di]
    jmp SHORT loc_a3bb
loc_a3f0:
    test ah, ah
    jne loc_a418
    cmp cx, word ptr [current_menu_item]
    je loc_a3e6
    jmp SHORT loc_a418
loc_a3fc:
    mov bx, 1
    test word ptr [di + 014h], bx
    je loc_a3ec
    sub dx, dx
    push cx
loc_a407:
    shl bx, 1
    rcl dx, 1
    loop loc_a407
    pop cx
    test word ptr [di + 014h], bx
    jne loc_a418
    test word ptr [di + 016h], dx
    je loc_a3e6
loc_a418:
    cmp di, word ptr [menu_selection_row]
    jne loc_a42a
    push di
    mov ax, 4
    push ax
    push cx
    call DWORD PTR [di + 2]
    mov cx, word ptr [di + 018h]
loc_a42a:
    mov word ptr [di + 018h], cx
    push word ptr [di + 8]
    push cs
    call redraw_menu_items
loc_a434:
    ret

PUBLIC replace_packed_item
replace_packed_item LABEL NEAR
    ; Replace an indexed packed item. Preserve its three-byte header across free/allocate.
    push bp
    mov bp, sp
    add sp, -4
    push di
    push si
    push ds
    pop es
    mov cx, 3
    mov bx, word ptr [bp + 8]
    shl bx, 1
    add bx, word ptr [bp + 0ah]
    lea di, [bp - 4]
    mov si, word ptr [bx + 01ch]
    push si
    rep movsb
    call FAR PTR __nfree
    add sp, 2
    jmp SHORT loc_a460
    xchg si, ax
    rep stosb
loc_a460:
    mov di, word ptr [bp + 0ah]
    les si, DWORD PTR [bp + 4]
loc_a466:
    lods BYTE PTR es:[si]
    cmp al, 021h
    je loc_a48c
    cmp al, 02fh
    je loc_a493
    cmp al, 028h
    jne loc_a4a8
    mov ax, 0fffeh
    cwd
    mov cx, word ptr [bp + 8]
    jcxz loc_a484
    stc
loc_a47e:
    rcl ax, 1
    rcl dx, 1
    loop loc_a47e
loc_a484:
    and word ptr [di + 014h], ax
    and word ptr [di + 016h], dx
    jmp SHORT loc_a466
loc_a48c:
    lods BYTE PTR es:[si]
    mov byte ptr [bp - 4], al
    jmp SHORT loc_a466
loc_a493:
    push es
    lods BYTE PTR es:[si]
    sub ah, ah
    push ax
    push cs
    call UPPERCASE_EVENT_CHARACTER
    pop es
    mov byte ptr [bp - 3], al
    lods BYTE PTR es:[si]
    mov byte ptr [bp - 2], al
    jmp SHORT loc_a466
loc_a4a8:
    sub al, al
    mov cx, 0ffffh
    dec si
    mov di, si
    repnz scasb
    not cx
    add cx, 3
    push cx
    call FAR PTR __nmalloc
    add sp, 2
    mov bx, word ptr [bp + 8]
    shl bx, 1
    add bx, word ptr [bp + 0ah]
    mov word ptr [bx + 01ch], ax
    test ax, ax
    je loc_a4e7
    push si
    sub di, si
    push di
    xchg di, ax
    mov cx, 3
    lea si, [bp - 4]
    push ds
    pop es
    rep movsb
    pop cx
    pop si
    mov ds, word ptr [bp + 6]
    rep movsb
    push ss
    pop ds
loc_a4e7:
    xchg si, ax
    mov dx, word ptr [bp + 6]
    pop si
    pop di
    mov sp, bp
    pop bp
    ret 8

PUBLIC refresh_tracking_item
refresh_tracking_item LABEL NEAR
    mov ax, word ptr [window_draw_port]
    xchg word ptr [Graf], ax
    push ax
    mov cx, word ptr [menu_selection_row]
    cmp cx, di
    je loc_a520
    jcxz loc_a508
    call close_tracking_window
loc_a508:
    lea ax, [di + 0ch]
    call ALLOCATE_BACKGROUND_PORT
    mov word ptr [menu_selection_column], ax
    mov word ptr [menu_selection_row], di
    push di
    mov ax, 2
    push ax
    sub ax, ax
    push ax
    call DWORD PTR [di + 2]
loc_a520:
    pop word ptr [Graf]
    ret
PUBLIC align_a525
align_a525 db 0   ; Inter-entry fill in the original image

PUBLIC grid_item_callback
grid_item_callback LABEL NEAR
    ; Far callback installed by the preloaded item dispatch table.
    push bp
    mov bp, sp
    add sp, -030h
    push di
    push si
    mov di, word ptr [bp + 0ah]
    mov si, word ptr [bp + 8]
    cmp si, 0
    je loc_a59b
    mov bx, word ptr [di + 2]
    mov ax, word ptr [item_state_2618]
    xchg word ptr [bx + 02ah], ax
    xchg word ptr [Graf], bx
    push ax
    push bx
    lea ax, [bp - 01eh]
    push ds
    push ax
    push ds
    push ax
    push cs
    call copy_current_port_origin
    push cs
    call apply_default_pen_state
    cmp word ptr [di + 01ah], 0
    jne loc_a566
    mov bx, word ptr [Graf]
    mov word ptr [bx + 028h], 7
loc_a566:
    mov cx, word ptr [bp + 6]
    xchg si, ax
    cmp ax, 2
    je loc_a5b9
    cmp ax, 7
    je loc_a5c7
    cmp ax, 6
    je loc_a59e
    cmp ax, 8
    je loc_a5a1
    cmp ax, 9
    je loc_a5aa
loc_a583:
    sub ax, ax
loc_a585:
    xchg si, ax
    push cs
    call set_relative_pen_state
    xchg si, ax
    pop bx
    xchg word ptr [Graf], bx
    pop word ptr [bx + 02ah]
loc_a593:
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 6
loc_a59b:
    jmp NEAR PTR loc_a658
loc_a59e:
    jmp NEAR PTR loc_a629
loc_a5a1:
    push word ptr [di + 022h]
    push cs
    call dispose_handle
    jmp SHORT loc_a583
loc_a5aa:
    jcxz loc_a583
    cmp cx, word ptr [di + 020h]
    ja loc_a583
    mov word ptr [di + 01eh], cx
    mov cx, word ptr [di + 01ch]
    jcxz loc_a583
loc_a5b9:
    lea ax, [di + 0ch]
    push ds
    push ax
    push cs
    call FRAME_RECT
    call draw_grid_cell_frame
    jmp SHORT loc_a583
loc_a5c7:
    xchg cx, ax
    cmp al, 0dh
    je loc_a5de
    cmp al, 020h
    jne loc_a585
    mov cx, word ptr [di + 01eh]
    inc cx
    cmp cx, word ptr [di + 020h]
    jbe loc_a5aa
    mov cx, 1
    jmp SHORT loc_a5aa
loc_a5de:
    push cs
    call update_message_display
    call draw_grid_selection
    sub si, si
    mov cx, word ptr [bp - 012h]
    jcxz loc_a621
loc_a5ec:
    lea ax, [bp - 010h]
    push ax
    mov ax, 0ch
    push ax
    push cs
    call read_queued_event
    xchg cx, ax
    jcxz loc_a5ec
    cmp word ptr [bp - 010h], 4
    je loc_a61e
    mov bx, word ptr [bp - 012h]
    mov ax, word ptr [bp - 6]
    cmp al, 01bh
    je loc_a61e
    cmp al, 0dh
    je loc_a617
    push bx
    push ax
    push cs
    call navigate_grid_items
    jmp SHORT loc_a5ec
loc_a617:
    xchg si, ax
    mov ax, word ptr [bx + 02eh]
    mov word ptr [di + 01eh], ax
loc_a61e:
    call release_grid_item_records
loc_a621:
    push cs
    call show_mouse_graphics
    xchg si, ax
    jmp NEAR PTR loc_a585
loc_a629:
    call draw_grid_selection
    mov ax, 1
    mov cx, word ptr [bp - 012h]
    jcxz loc_a655
    lea si, [bp - 016h]
    push si
    push cs
    call read_mouse_event_position
    push word ptr [bp - 012h]
    push si
    push word ptr [bp + 6]
    push cs
    call draw_grid_item
    xchg cx, ax
    jcxz loc_a650
    mov word ptr [di + 01eh], cx
    mov cx, 1
loc_a650:
    push cx
    call release_grid_item_records
    pop ax
loc_a655:
    jmp NEAR PTR loc_a585
loc_a658:
    mov ax, 024h
    push ax
    call FAR PTR __nmalloc
    add sp, 2
    test ax, ax
    je loc_a6d5
    xchg dx, ax
    sub ax, ax
    mov cx, 024h
    mov di, dx
    push ds
    pop es
    rep stosb
    mov di, dx
    mov bx, word ptr [bp + 6]
    lds si, DWORD PTR [bx]
    lodsw
    mov cx, 8
    add di, 0ch
    rep movsb
    mov di, dx
    push ds
    push es
    pop ds
    pop es
    mov word ptr [di + 4], OFFSET grid_item_callback
    mov word ptr [di + 6], cs
    mov ax, 1
    mov word ptr [di + 01ah], ax
    mov word ptr [di + 01ch], ax
    mov word ptr [di + 016h], 0fh
    mov word ptr [di + 01eh], 1
    lods WORD PTR es:[si]
    mov word ptr [di + 020h], ax
    push es
    push es
    push si
    push ax
    push cs
    call skip_nul_strings
    mov bx, word ptr [bp + 6]
    mov word ptr [bx], ax
    sub ax, si
    push ax
    sub dx, dx
    push dx
    push ax
    push cs
    call allocate_payload_handle
    mov word ptr [di + 022h], ax
    push ax
    push cs
    call resolve_handle_to_far_payload
    pop cx
    pop ds
    xchg di, ax
    mov es, dx
    or dx, di
    je loc_a6d5
    rep movsb
loc_a6d5:
    push ss
    pop ds
    jmp NEAR PTR loc_a593

PUBLIC release_grid_item_records
release_grid_item_records LABEL NEAR
    ; Uses the callback stack frame. The empty-record path enters draw_grid_cell_frame.
    push word ptr [bp - 012h]
    push cs
    call release_near_buffer
    push word ptr [di + 022h]
    push cs
    call CLEAR_HANDLE_OWNED
    mov cx, word ptr [bp - 2]
    jcxz loc_a6f9
    xchg cx, ax
    call RELEASE_BACKGROUND_PORT
loc_a6f1:
    mov ax, word ptr [di + 2]
    mov word ptr [Graf], ax
    jmp SHORT draw_grid_cell_frame
loc_a6f9:
    lea ax, [bp - 030h]
    push ax
    push cx
    push cs
    call invalidate_window_rectangle
    jmp SHORT loc_a6f1

PUBLIC draw_grid_cell_frame
draw_grid_cell_frame LABEL NEAR
    push si
    lea si, [bp - 026h]
    push ds
    push si
    lea ax, [di + 0ch]
    push ds
    push ax
    mov ax, 1
    push ax
    push ax
    push cs
    call InsetRect
    mov bx, word ptr [di + 2]
    mov ax, word ptr [bx + 020h]
    mov dx, 7
    mov cx, word ptr [di + 01ah]
    jcxz loc_a72c
    mov ax, word ptr [di + 016h]
    mov dx, word ptr [di + 018h]
loc_a72c:
    xchg word ptr [bx + 028h], dx
    push dx
    push bx
    push ds
    push si
    push ax
    push cs
    call set_record_range
    push word ptr [di + 022h]
    push cs
    call resolve_handle_to_far_payload
    push dx
    push ax
    mov ax, word ptr [di + 01eh]
    dec ax
    push ax
    push cs
    call skip_nul_strings
    push ds
    push si
    sub cx, cx
    push cx
    push cx
    push dx
    push ax
    push cs
    call DRAW_NUL_TEXT_IN_RECT
    pop bx
    pop word ptr [bx + 028h]
    pop si
    ret

PUBLIC draw_grid_selection
draw_grid_selection LABEL NEAR
    push si
    lea ax, [bp - 030h]
    push ds
    push ax
    lea ax, [di + 0ch]
    push ds
    push ax
    mov ax, 1
    push ax
    push ax
    push cs
    call InsetRect
    mov word ptr [bp - 024h], 1
    mov bx, word ptr [di + 020h]
    mov word ptr [bp - 026h], bx
    mov ax, word ptr [bp - 02ah]
    sub ax, word ptr [bp - 02eh]
    mov word ptr [bp - 020h], ax
    mov ax, word ptr [bp - 02ch]
    sub ax, word ptr [bp - 030h]
    mov word ptr [bp - 022h], ax
    mov bx, word ptr [di + 2]
    mov si, word ptr [window_draw_port]
    mov word ptr [bp - 028h], 0
    mul word ptr [di + 01eh]
    mov cx, word ptr [bp - 02ch]
    sub cx, ax
    mov word ptr [bp - 030h], cx
    mov ax, word ptr [si + 018h]
    add ax, word ptr [bx + 4]
    sub ax, cx
    jle loc_a7cb
    sub dx, dx
    div word ptr [bp - 022h]
    test dx, dx
    je loc_a7b8
    inc ax
loc_a7b8:
    inc ax
    cmp ax, word ptr [di + 01eh]
    jbe loc_a7c1
    mov ax, word ptr [di + 01eh]
loc_a7c1:
    dec ax
    mov word ptr [bp - 028h], ax
    mul word ptr [bp - 022h]
    add word ptr [bp - 030h], ax
loc_a7cb:
    mov ax, word ptr [di + 020h]
    sub ax, word ptr [di + 01eh]
    mul word ptr [bp - 022h]
    add word ptr [bp - 02ch], ax
    mov cx, word ptr [si + 01ch]
    add cx, word ptr [bx + 4]
    mov ax, word ptr [bp - 02ch]
    sub ax, cx
    jle loc_a806
    sub dx, dx
    div word ptr [bp - 022h]
    test dx, dx
    je loc_a7ee
    inc ax
loc_a7ee:
    mov dx, word ptr [di + 020h]
    sub dx, ax
    cmp dx, word ptr [di + 01eh]
    jge loc_a7fb
    mov dx, word ptr [di + 01eh]
loc_a7fb:
    mov ax, word ptr [di + 020h]
    sub ax, dx
    mul word ptr [bp - 022h]
    sub word ptr [bp - 02ch], ax
loc_a806:
    push si
    lea si, [bp - 030h]
    push ds
    push si
    push ds
    push si
    push cs
    call subtract_port_origin
    lea ax, [si + 4]
    push ds
    push ax
    push ds
    push ax
    push cs
    call subtract_port_origin
    pop word ptr [Graf]
    push word ptr [di + 022h]
    push cs
    call FETCH_LOCKED_SHAPE_LIST
    push si
    lea bx, [bp - 026h]
    push bx
    push dx
    push ax
    push cs
    call assemble_packed_item
    mov word ptr [bp - 012h], ax
    test ax, ax
    jne loc_a83c
loc_a83a:
    pop si
    ret
loc_a83c:
    xchg bx, ax
    mov word ptr [bx + 018h], 0
    mov ax, word ptr [item_state_2618]
    mov word ptr [bx + 2], ax
    mov ax, word ptr [di + 016h]
    mov word ptr [bx + 4], ax
    mov ax, word ptr [di + 018h]
    mov word ptr [bx + 6], ax
    push ds
    push si
    push ds
    push si
    mov ax, 0ffffh
    push ax
    push ax
    push cs
    call InsetRect
    add word ptr [si + 6], 2
    add word ptr [si + 4], 1
    mov ax, si
    call ALLOCATE_BACKGROUND_PORT
    mov word ptr [bp - 2], ax
    sub word ptr [si + 6], 2
    sub word ptr [si + 4], 1
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
    call stroke_offset_edges
    add word ptr [si + 6], 2
    add word ptr [si + 4], 1
    mov cx, word ptr [bp - 028h]
    jcxz loc_a8ae
    push word ptr [bp - 012h]
    sub ax, ax
    push ax
    push cx
    push cs
    call scroll_grid_origin
    jmp SHORT loc_a8b5
loc_a8ae:
    push word ptr [bp - 012h]
    push cs
    call paint_grid_items
loc_a8b5:
    push word ptr [bp - 012h]
    push word ptr [di + 01eh]
    push cs
    call select_grid_item
    jmp NEAR PTR loc_a83a

PUBLIC slider_item_callback
slider_item_callback LABEL NEAR
    ; Far callback installed by the preloaded item dispatch table.
    push bp
    mov bp, sp
    add sp, -05eh
    push di
    push si
    mov di, word ptr [bp + 0ah]
    mov si, word ptr [bp + 8]
    cmp si, 0
    je loc_a929
    mov ax, word ptr [di + 2]
    xchg word ptr [Graf], ax
    push ax
    lea ax, [bp - 02ah]
    push ds
    push ax
    push ds
    push ax
    push cs
    call copy_current_port_origin
    push cs
    call apply_default_pen_state
    call initialize_slider_geometry
    mov cx, word ptr [bp + 6]
    xchg si, ax
    cmp ax, 2
    je loc_a91b
    cmp ax, 9
    je loc_a923
    cmp ax, 7
    je loc_a91d
    cmp ax, 6
    je loc_a920
loc_a907:
    sub ax, ax
loc_a909:
    xchg si, ax
    push cs
    call set_relative_pen_state
    xchg si, ax
    pop word ptr [Graf]
loc_a913:
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 6
loc_a91b:
    jmp SHORT loc_a985
loc_a91d:
    jmp NEAR PTR loc_a9c8
loc_a920:
    jmp NEAR PTR loc_a9f8
loc_a923:
    xchg cx, ax
    call update_slider_value
    jmp SHORT loc_a907
loc_a929:
    mov ax, 024h
    push ax
    call FAR PTR __nmalloc
    add sp, 2
    test ax, ax
    je loc_a913
    xchg dx, ax
    sub ax, ax
    mov cx, 024h
    mov di, dx
    push ds
    pop es
    rep stosb
    mov di, dx
    mov bx, word ptr [bp + 6]
    lds si, DWORD PTR [bx]
    lodsw
    mov cx, 8
    add di, 0ch
    rep movsb
    mov di, dx
    push ds
    push es
    pop ds
    pop es
    mov word ptr [di + 4], OFFSET slider_item_callback
    mov word ptr [di + 6], cs
    mov ax, 1
    mov word ptr [di + 01ah], ax
    mov word ptr [di + 01ch], ax
    mov word ptr [di + 016h], 7
    mov word ptr [di + 018h], 0fh
    mov word ptr [di + 022h], 1
    lods WORD PTR es:[si]
    mov word ptr [di + 01eh], ax
    mov word ptr [bx], si
    xchg di, ax
    jmp SHORT loc_a913
loc_a985:
    mov bx, word ptr [di + 2]
    mov ax, word ptr [bx + 020h]
    mov cx, word ptr [di + 01ah]
    jcxz loc_a993
    mov ax, word ptr [di + 016h]
loc_a993:
    lea bx, [bp - 020h]
    push ds
    push bx
    push ax
    push cs
    call set_record_range
    mov si, word ptr [bp - 2]
    mov ax, word ptr [si + slider_decrement_shape]
    lea bx, [bp - 0ah]
    call draw_slider_region
    mov ax, word ptr [si + slider_increment_shape]
    lea bx, [bp - 012h]
    call draw_slider_region
    mov cx, word ptr [di + 01ah]
    jcxz loc_a9bc
    call draw_slider_edges
loc_a9bc:
    lea ax, [di + 0ch]
    push ds
    push ax
    push cs
    call FRAME_RECT
    jmp NEAR PTR loc_a907
loc_a9c8:
    xchg cx, ax
    mov si, word ptr [bp - 2]
    mov dx, 1
    cmp ax, word ptr [si + slider_increment_key]
    je loc_a9ec
    neg dx
    cmp ax, word ptr [si + slider_decrement_key]
    je loc_a9ec
    mov dx, word ptr [di + 022h]
    cmp ax, 05100h
    je loc_a9ec
    neg dx
    cmp ax, 04900h
    jne loc_a9f5
loc_a9ec:
    xchg dx, ax
    add ax, word ptr [di + 020h]
    call update_slider_value
    sub ax, ax
loc_a9f5:
    jmp NEAR PTR loc_a909
loc_a9f8:
    sub ax, ax
    mov word ptr [bp - 014h], ax
    mov word ptr [bp - 022h], ax
loc_aa00:
    lea ax, [bp - 018h]
    push ax
    push cs
    call read_mouse_event_position
    push ds
    push ax
    push ds
    push ax
    push cs
    call add_port_origin
    push ds
    push ax
    lea si, [bp - 04eh]
    push ds
    push si
    push cs
    call point_in_rectangle
    xchg cx, ax
    jcxz loc_aa27
    cmp word ptr [bp - 022h], 0
    jne loc_aa89
    jmp NEAR PTR loc_aad9
loc_aa27:
    lea ax, [bp - 018h]
    push ds
    push ax
    lea si, [bp - 032h]
    push ds
    push si
    push cs
    call point_in_rectangle
    test ax, ax
    mov ax, 1
    mov dx, word ptr [di + 022h]
    jne loc_aa96
    lea ax, [bp - 018h]
    push ds
    push ax
    lea si, [bp - 03ah]
    push ds
    push si
    push cs
    call point_in_rectangle
    mov dx, word ptr [di + 022h]
    neg dx
    test ax, ax
    mov ax, 2
    jne loc_aa96
    lea ax, [bp - 018h]
    push ds
    push ax
    lea si, [bp - 0ah]
    push ds
    push si
    push cs
    call point_in_rectangle
    test ax, ax
    mov ax, 0fffeh
    mov dx, 0ffffh
    jne loc_aa96
    lea ax, [bp - 018h]
    push ds
    push ax
    lea si, [bp - 012h]
    push ds
    push si
    push cs
    call point_in_rectangle
    test ax, ax
    mov ax, 0ffffh
    mov dx, 1
    jne loc_aa96
loc_aa89:
    sub cx, cx
    xchg word ptr [bp - 014h], cx
    jcxz loc_aaba
    xchg cx, ax
    call convert_slider_bounds
    jmp SHORT loc_aaba
loc_aa96:
    mov cx, word ptr [bp - 022h]
    jcxz loc_aa9f
    cmp ax, cx
    jne loc_aa89
loc_aa9f:
    push dx
    mov word ptr [bp - 022h], ax
    test ax, ax
    jns loc_aab3
    cmp si, word ptr [bp - 014h]
    mov word ptr [bp - 014h], si
    je loc_aab3
    xchg si, ax
    call convert_slider_bounds
loc_aab3:
    pop ax
    add ax, word ptr [di + 020h]
    call update_slider_value
loc_aaba:
    call notify_slider_changed
    push word ptr [bp + 6]
    push cs
    call mouse_button_unqueued
    xchg cx, ax
    jcxz loc_aaca
    jmp NEAR PTR loc_aa00
loc_aaca:
    mov cx, word ptr [bp - 014h]
    jcxz loc_aad3
    xchg cx, ax
    call convert_slider_bounds
loc_aad3:
    mov ax, 1
    jmp NEAR PTR loc_a909
loc_aad9:
    lea ax, [bp - 046h]
    push ds
    push ax
    push ds
    push si
    mov ax, 1
    push ax
    push ax
    push cs
    call InsetRect
    mov cx, word ptr [bp - 016h]
    mov word ptr [bp - 054h], cx
    inc cx
    mov word ptr [bp - 050h], cx
    mov dx, word ptr [bp - 018h]
    mov word ptr [bp - 056h], dx
    inc dx
    mov word ptr [bp - 052h], dx
    mov si, word ptr [bp - 2]
    mov ax, word ptr [bp + si - 020h]
    add ax, word ptr [bp + si - 018h]
    sub ax, word ptr [bp + si - 04eh]
    mov word ptr [bp + si - 056h], ax
    mov ax, word ptr [bp + si - 01ch]
    add ax, word ptr [bp + si - 018h]
    sub ax, word ptr [bp + si - 04ah]
    inc ax
    mov word ptr [bp + si - 052h], ax
    mov bx, word ptr [di + 2]
    mov ax, 7
    xchg word ptr [bx + 028h], ax
    push ax
    push bx
    lea ax, [bp - 018h]
    push ax
    push word ptr [bp + 6]
    lea ax, [bp - 046h]
    push ax
    lea ax, [bp - 056h]
    push ax
    lea ax, [bp - 05eh]
    push ds
    push ax
    lea ax, [bp - 020h]
    push ds
    push ax
    mov ax, word ptr [bp - 01eh]
    sub ax, word ptr [bp - 01ah]
    push ax
    mov ax, word ptr [bp - 020h]
    sub ax, word ptr [bp - 01ch]
    push ax
    push cs
    call InsetRect
    push ax
    push cs
    call DRAG_OUTLINE_WITH_MOUSE
    mov word ptr [bp - 016h], ax
    mov word ptr [bp - 018h], dx
    pop bx
    pop word ptr [bx + 028h]
    mov ax, word ptr [bp + si - 018h]
    add ax, word ptr [bp + si - 04eh]
    sub ax, word ptr [bp + si - 020h]
    push ax
    sub ax, ax
    push ax
    push word ptr [bp - 03ch]
    push word ptr [bp - 03eh]
    push cs
    call divide_timer_interval
    test ax, ax
    jns loc_ab7a
    inc dx
loc_ab7a:
    push dx
    push cs
    call apply_default_pen_state
    pop ax
    call update_slider_value
    call notify_slider_changed
    jmp NEAR PTR loc_aad3

PUBLIC initialize_slider_geometry
initialize_slider_geometry LABEL NEAR
    ; Seed slider rectangles. The final path falls through into rebuild_slider_bounds.
    mov bx, di
    mov di, 8
loc_ab8e:
    dec di
    mov al, byte ptr [bx + di + 0ch]
    mov byte ptr [bp + di - 0ah], al
    mov byte ptr [bp + di - 012h], al
    mov byte ptr [bp + di - 020h], al
    jg loc_ab8e
    mov cx, word ptr [bx + 012h]
    sub cx, word ptr [bx + 0eh]
    mov dx, word ptr [bx + 010h]
    sub dx, word ptr [bx + 0ch]
    cmp cx, dx
    mov di, 2
    jae loc_abb3
    mov di, 0
loc_abb3:
    mov word ptr [bp - 2], di
    mov ax, word ptr [bx + di + 0ch]
    add ax, word ptr [di + slider_arrow_inset]
    mov word ptr [bp + di - 6], ax
    dec ax
    mov word ptr [bp + di - 020h], ax
    mov ax, word ptr [bx + di + 010h]
    sub ax, word ptr [di + slider_arrow_inset]
    mov word ptr [bp + di - 012h], ax
    inc ax
    mov word ptr [bp + di - 01ch], ax
    mov di, bx

PUBLIC rebuild_slider_bounds
rebuild_slider_bounds LABEL NEAR
    ; Independent near entry as well as the tail of initialize_slider_geometry.
    push si
    mov si, 8
loc_abd8:
    dec si
    mov al, byte ptr [bp + si - 020h]
    mov byte ptr [bp + si - 032h], al
    mov byte ptr [bp + si - 03ah], al
    mov byte ptr [bp + si - 04eh], al
    jg loc_abd8
    mov si, word ptr [bp - 2]
    mov ax, word ptr [bp + si - 01ch]
    sub ax, word ptr [bp + si - 020h]
    sub ax, word ptr [si + slider_arrow_inset]
    push ax
    sub ax, ax
    push ax
    mov dx, word ptr [di + 01eh]
    dec dx
    push dx
    push ax
    push cs
    call divide_timer_interval
    mov word ptr [bp - 03eh], ax
    mov word ptr [bp - 03ch], dx
    push dx
    push ax
    push word ptr [di + 020h]
    sub ax, ax
    push ax
    push cs
    call multiply_timer_interval
    test ax, ax
    jns loc_ac19
    inc dx
loc_ac19:
    add dx, word ptr [bp + si - 020h]
    mov word ptr [bp + si - 036h], dx
    mov word ptr [bp + si - 04eh], dx
    add dx, word ptr [si + slider_arrow_inset]
    mov word ptr [bp + si - 04ah], dx
    mov word ptr [bp + si - 032h], dx
    pop si
    ret

PUBLIC draw_slider_region
draw_slider_region LABEL NEAR
    push bp
    push si
    xchg si, ax
    push ds
    push bx
    mov bx, word ptr [di + 2]
    mov ax, word ptr [bx + 020h]
    mov cx, word ptr [di + 01ah]
    jcxz loc_ac41
    mov ax, word ptr [di + 018h]
loc_ac41:
    push ax
    push cs
    call set_record_range
    push ds
    push ax
    push cs
    call FRAME_RECT
    xchg bx, ax
    mov cx, word ptr [bx + 6]
    sub cx, word ptr [bx + 2]
    shr cx, 1
    adc cx, word ptr [bx + 2]
    mov ax, word ptr [si + 2]
    shr ax, 1
    sbb cx, ax
    mov dx, word ptr [bx + 4]
    sub dx, word ptr [bx]
    shr dx, 1
    adc dx, word ptr [bx]
    mov ax, word ptr [si]
    shr ax, 1
    sbb dx, ax
    push cx
    push dx
    push ds
    push si
    sub ax, ax
    push ax
    push ax
    dec ax
    push ax
    inc ax
    push cs
    call convert_shape
    xchg bp, ax
    push bp
    push cs
    call FETCH_LOCKED_SHAPE_LIST
    pop si
    pop cx
    push dx
    push ax
    push cx
    push si
    mov ax, 0
    push ax
    mov ax, 0
    push ax
    push cs
    call paint_four_bit
    push bp
    push cs
    call dispose_handle
    pop si
    pop bp
    ret

PUBLIC draw_slider_edges
draw_slider_edges LABEL NEAR
    mov bx, word ptr [di + 2]
    mov ax, word ptr [bx + 020h]
    mov cx, word ptr [di + 01ah]
    jcxz loc_acab
    mov ax, word ptr [di + 018h]
loc_acab:
    lea bx, [bp - 04eh]
    push ds
    push bx
    push ax
    push cs
    call set_record_range
    push ds
    push ax
    push cs
    call FRAME_RECT
    ret

PUBLIC convert_slider_bounds
convert_slider_bounds LABEL NEAR
    lea bx, [bp - 046h]
    push ds
    push bx
    push ds
    push ax
    mov ax, 1
    push ax
    push ax
    push cs
    call InsetRect
    push ds
    push ax
    push cs
    call toggle_rect_effect
    ret

PUBLIC notify_slider_changed
notify_slider_changed LABEL NEAR
    mov ax, word ptr [di + 8]
    or ax, word ptr [di + 0ah]
    je loc_acdf
    push di
    call DWORD PTR [di + 8]
loc_acdf:
    ret

PUBLIC update_slider_value
update_slider_value LABEL NEAR
    test ax, ax
    jge loc_ace6
    sub ax, ax
loc_ace6:
    cmp ax, word ptr [di + 01eh]
    jb loc_acef
    mov ax, word ptr [di + 01eh]
    dec ax
loc_acef:
    cmp ax, word ptr [di + 020h]
    mov word ptr [di + 020h], ax
    je loc_ad34
    mov cx, word ptr [di + 01ch]
    jcxz loc_ad34
    lea ax, [bp - 046h]
    push ds
    push ax
    lea ax, [bp - 020h]
    push ds
    push ax
    mov ax, 1
    push ax
    push ax
    push cs
    call InsetRect
    push ds
    push ax
    push cs
    call push_port_clip
    mov ax, 0fh
    mov cx, word ptr [di + 01ah]
    jcxz loc_ad20
    mov ax, 7
loc_ad20:
    lea bx, [bp - 04eh]
    push ds
    push bx
    push ax
    push cs
    call set_record_range
    push cs
    call pop_port_clip
    call rebuild_slider_bounds
    call draw_slider_edges
loc_ad34:
    ret
PUBLIC align_ad35
align_ad35 db 0   ; Inter-entry fill in the original image

PUBLIC text_item_callback
text_item_callback LABEL NEAR
    ; Far callback installed by the preloaded item dispatch table.
    push bp
    mov bp, sp
    add sp, -8
    push di
    push si
    mov di, word ptr [bp + 0ah]
    mov si, word ptr [bp + 8]
    cmp si, 0
    je loc_ad92
    mov bx, word ptr [di + 2]
    mov ax, word ptr [item_state_2618]
    xchg word ptr [bx + 02ah], ax
    xchg word ptr [Graf], bx
    push ax
    push bx
    lea ax, [bp - 8]
    push ds
    push ax
    push ds
    push ax
    push cs
    call copy_current_port_origin
    push cs
    call apply_default_pen_state
    mov cx, word ptr [bp + 6]
    xchg si, ax
    cmp ax, 2
    je loc_adbb
    cmp ax, 8
    je loc_ad94
    cmp ax, 9
    je loc_ad9d
loc_ad7a:
    sub ax, ax
    xchg si, ax
    push cs
    call set_relative_pen_state
    xchg si, ax
    pop bx
    xchg word ptr [Graf], bx
    pop word ptr [bx + 02ah]
loc_ad8a:
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 6
loc_ad92:
    jmp SHORT loc_ae07
loc_ad94:
    push word ptr [di + 026h]
    push cs
    call dispose_handle
    jmp SHORT loc_ad7a
loc_ad9d:
    push ds
    push cx
    push word ptr [di + 026h]
    push cs
    call dispose_handle
    push cs
    call create_string_handle
    mov word ptr [di + 026h], ax
    mov cx, word ptr [di + 01ch]
    jcxz loc_ad7a
    lea ax, [di + 0ch]
    push ds
    push ax
    push cs
    call STROKE_PORT_RECT
loc_adbb:
    mov si, word ptr [di + 2]
    mov ax, word ptr [di + 018h]
    mov word ptr [si + 028h], ax
    push word ptr [si + 02ah]
    push cs
    call resolve_handle_to_far_payload
    xchg bx, ax
    mov es, dx
    mov ax, word ptr es:[bx + 8]
    add ax, word ptr [di + 024h]
    xchg word ptr es:[bx + 8], ax
    push ax
    mov ax, word ptr es:[bx + 6]
    add ax, word ptr [di + 022h]
    xchg word ptr es:[bx + 6], ax
    push ax
    push es
    push bx
    lea ax, [di + 0ch]
    push ds
    push ax
    push word ptr [di + 020h]
    push word ptr [di + 01eh]
    push word ptr [di + 026h]
    push cs
    call draw_text_handle_in_rect
    pop bx
    pop es
    pop word ptr es:[bx + 6]
    pop word ptr es:[bx + 8]
    jmp NEAR PTR loc_ad7a
loc_ae07:
    mov ax, 028h
    push ax
    call FAR PTR __nmalloc
    add sp, 2
    test ax, ax
    je loc_ae5f
    xchg dx, ax
    sub ax, ax
    mov cx, 028h
    mov di, dx
    push ds
    pop es
    rep stosb
    mov di, dx
    mov bx, word ptr [bp + 6]
    lds si, DWORD PTR [bx]
    lodsw
    mov cx, 8
    add di, 0ch
    rep movsb
    mov di, dx
    push ds
    push es
    pop ds
    pop es
    mov word ptr [di + 4], OFFSET text_item_callback
    mov word ptr [di + 6], cs
    mov word ptr [di + 01ch], 1
    dec cx
    mov word ptr [di + 020h], cx
    mov word ptr [di + 01eh], cx
    push es
    push si
    sub ax, ax
    xchg di, si
    repnz scasb
    mov word ptr [bx], di
    push cs
    call create_string_handle
    mov word ptr [si + 026h], ax
    xchg si, ax
loc_ae5f:
    jmp NEAR PTR loc_ad8a

PUBLIC field_item_callback
field_item_callback LABEL NEAR
    ; Far callback installed by the preloaded item dispatch table.
    push bp
    mov bp, sp
    add sp, -010h
    push di
    push si
    push word ptr [Graf]
    mov ax, word ptr [window_draw_port]
    mov word ptr [Graf], ax
    mov di, word ptr [bp + 0ah]
    mov ax, word ptr [bp + 8]
    cmp ax, 2
    je loc_ae9a
    cmp ax, 3
    je loc_ae9d
    cmp ax, 4
    je loc_aea0
    cmp ax, 1
    je loc_aea3
loc_ae8e:
    pop word ptr [Graf]
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 6
loc_ae9a:
    jmp NEAR PTR loc_af7e
loc_ae9d:
    jmp NEAR PTR loc_af3f
loc_aea0:
    jmp NEAR PTR loc_af36
loc_aea3:
    mov word ptr [di + 012h], 0
    mov cx, word ptr [di + 01ah]
    jcxz loc_aef8
loc_aead:
    push cx
    mov bx, cx
    shl bx, 1
    mov si, word ptr [bx + di + 01ch]
    cmp byte ptr [si + 3], 02dh
    je loc_aef5
    lea ax, [si + 3]
    push ds
    push ax
    push cs
    call MEASURE_NUL_TERMINATED_TEXT
    add ax, 0bh
    mov cx, word ptr [si + 1]
    jcxz loc_aeed
    push di
    xchg di, ax
    push cx
    lea ax, [si + 3]
    push ax
    call FAR PTR find_string_byte
    add sp, 4
    test ax, ax
    jne loc_aeeb
    push word ptr [si + 1]
    push cs
    call measure_character
    add ax, 8
    add di, ax
loc_aeeb:
    xchg di, ax
    pop di
loc_aeed:
    cmp ax, word ptr [di + 012h]
    jbe loc_aef5
    xchg word ptr [di + 012h], ax
loc_aef5:
    pop cx
    loop loc_aead
loc_aef8:
    mov ax, word ptr [di + 0eh]
    add ax, 4
    add word ptr [di + 012h], ax
    mov bx, word ptr [Graf]
    push word ptr [bx + 02ah]
    push cs
    call resolve_handle_to_far_payload
    xchg bx, ax
    mov dx, es
    mov ax, word ptr es:[bx + 2]
    add ax, word ptr es:[bx + 4]
    mov dx, 2
    add ax, dx
    sub dx, word ptr es:[bx + 4]
    jle loc_af24
    add ax, dx
loc_af24:
    mov word ptr [di + 0ah], ax
    mul word ptr [di + 01ah]
    add ax, word ptr [di + 0ch]
    add ax, 2
    mov word ptr [di + 010h], ax
    jmp NEAR PTR loc_ae8e
loc_af36:
    mov ax, word ptr [bp + 6]
    call redraw_field_selection
    jmp NEAR PTR loc_ae8e
loc_af3f:
    mov si, word ptr [di + 01ah]
    inc si
loc_af43:
    dec si
    je loc_af58
    mov ax, si
    call field_slot_rectangle
    push ds
    push word ptr [bp + 6]
    push ds
    push bx
    push cs
    call point_in_rectangle
    xchg cx, ax
    jcxz loc_af43
loc_af58:
    mov ax, 1
    test word ptr [di + 014h], ax
    je loc_af75
    cwd
    mov cx, si
    jcxz loc_af6b
loc_af65:
    shl ax, 1
    rcl dx, 1
    loop loc_af65
loc_af6b:
    test word ptr [di + 014h], ax
    jne loc_af77
    test word ptr [di + 016h], dx
    jne loc_af77
loc_af75:
    sub si, si
loc_af77:
    xchg si, ax
    call redraw_field_selection
    jmp NEAR PTR loc_ae8e
loc_af7e:
    push cs
    call update_message_display
    lea ax, [bp - 010h]
    push ds
    push ax
    push ds
    push ax
    push cs
    call copy_current_port_origin
    lea si, [di + 0ch]
    push si
    mov ax, 2
    push ax
    sub word ptr [si + 6], ax
    mov ax, 1
    push ax
    sub word ptr [si + 4], ax
    mov ax, 0
    push ax
    mov ax, 0
    push ax
    push cs
    call stroke_offset_edges
    mov bx, word ptr [Graf]
    mov word ptr [bx + 028h], 0
    push ds
    push si
    push cs
    call FRAME_RECT
    add word ptr [si + 6], 2
    add word ptr [si + 4], 1
    mov ax, word ptr [di + 01ah]
    call field_slot_rectangle
    mov ax, word ptr [di + 0ch]
    mov word ptr [bx], ax
    push ds
    push bx
    mov ax, 0fh
    push ax
    push cs
    call set_record_range
    sub ax, ax
    lea si, [di + 01ch]
loc_afdc:
    inc ax
    cmp ax, word ptr [di + 01ah]
    jle loc_aff5
    sub ax, ax
    xchg word ptr [di + 018h], ax
    call redraw_field_selection
    push cs
    call set_relative_pen_state
    push cs
    call show_mouse_graphics
    jmp NEAR PTR loc_ae8e
loc_aff5:
    push ax
    mov bx, ax
    shl bx, 1
    mov si, word ptr [bx + di + 01ch]
    call field_slot_rectangle
    les ax, DWORD PTR [bx]
    mov dx, es
    cmp byte ptr [si + 3], 02dh
    jne loc_b022
    les bx, DWORD PTR [bx + 4]
    sub bx, ax
    sar bx, 1
    add ax, bx
    push es
    push ax
    push dx
    push ax
    push cs
    call MoveTo
    push cs
    call draw_axis_segment
    jmp NEAR PTR loc_b0e2
loc_b022:
    add dx, 9
    push dx
    inc ax
    push ax
    mov bx, word ptr [Graf]
    push word ptr [bx + 02ah]
    push cs
    call resolve_handle_to_far_payload
    xchg bx, ax
    mov es, dx
    pop ax
    add ax, word ptr es:[bx + 2]
    push ax
    push cs
    call MoveTo
    mov bx, word ptr [Graf]
    mov word ptr [bx + 028h], 7
    mov ax, 1
    test word ptr [di + 014h], ax
    je loc_b06b
    cwd
    pop cx
    push cx
    jcxz loc_b06b
loc_b056:
    shl ax, 1
    rcl dx, 1
    loop loc_b056
    test word ptr [di + 014h], ax
    jne loc_b066
    test word ptr [di + 016h], dx
    je loc_b06b
loc_b066:
    mov word ptr [bx + 028h], 0
loc_b06b:
    lea ax, [si + 3]
    push ax
    push word ptr [si + 1]
    push cs
    call draw_dialog_scroll_region
    mov al, byte ptr [si]
    test al, al
    je loc_b08c
    mov bx, word ptr [Graf]
    mov cx, word ptr [bp - 6]
    inc cx
    mov word ptr [bx + 024h], cx
    push ax
    push cs
    call draw_character
loc_b08c:
    mov cx, word ptr [si + 1]
    jcxz loc_b0e2
    push cx
    lea ax, [si + 3]
    push ax
    call FAR PTR find_string_byte
    add sp, 4
    test ax, ax
    jne loc_b0e2
    push word ptr [si + 1]
    push cs
    call measure_character
    mov dx, word ptr [bp - 2]
    sub dx, ax
    sub dx, 2
    mov bx, word ptr [Graf]
    xchg bx, si
    mov word ptr [si + 024h], dx
    mov ax, word ptr [si + 028h]
    push ax
    cmp ax, 7
    je loc_b0c8
    mov word ptr [si + 028h], 4
loc_b0c8:
    push word ptr [bx + 1]
    push cs
    call draw_character
    dec word ptr [si + 024h]
    inc word ptr [si + 022h]
    neg ax
    push ax
    sub ax, ax
    push ax
    push cs
    call draw_relative_axis_segment
    pop word ptr [si + 028h]
loc_b0e2:
    pop ax
    jmp NEAR PTR loc_afdc

PUBLIC field_slot_rectangle
field_slot_rectangle LABEL NEAR
    ; Build the field slot rectangle at BP-8 and return its address in BX.
    mul word ptr [di + 0ah]
    add ax, word ptr [di + 0ch]
    lea bx, [bp - 8]
    mov word ptr [bx + 4], ax
    sub ax, word ptr [di + 0ah]
    mov word ptr [bx], ax
    mov ax, word ptr [di + 0eh]
    inc ax
    mov word ptr [bx + 2], ax
    mov ax, word ptr [di + 012h]
    sub ax, 3
    mov word ptr [bx + 6], ax
    ret

PUBLIC redraw_field_selection
redraw_field_selection LABEL NEAR
    cmp ax, word ptr [di + 018h]
    je loc_b12c
    xchg word ptr [di + 018h], ax
    test ax, ax
    je loc_b11d
    call field_slot_rectangle
    push ds
    push bx
    push cs
    call toggle_rect_effect
loc_b11d:
    mov cx, word ptr [di + 018h]
    jcxz loc_b12c
    xchg cx, ax
    call field_slot_rectangle
    push ds
    push bx
    push cs
    call toggle_rect_effect
loc_b12c:
    ret
PUBLIC align_b12d
align_b12d db 0   ; Inter-entry fill in the original image

GAMELIB_TEXT ENDS
END
