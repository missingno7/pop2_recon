; DOS cursor and bitmap frontend (root frame 1129).
; The draft public delimits the internal alignment byte.
.MODEL MEDIUM
.DATA
EXTRN Graf:WORD
EXTRN MOUSE_PRESENT:WORD
EXTRN SCREEN_WIDTH:WORD
EXTRN PIXELS_PER_BYTE:BYTE
; Default-port members are exported by the graphics-mode group.
EXTRN DEFAULT_PORT:BYTE
EXTRN DEFAULT_PORT_SEGMENT:WORD
EXTRN DEFAULT_PORT_ORIGIN:DWORD
EXTRN DEFAULT_PORT_LIMITS:DWORD
EXTRN DEFAULT_PORT_MOUSE_BOUNDS:BYTE
EXTRN DEFAULT_PORT_MOUSE_MAX_X:WORD
EXTRN MOUSE_MAX_Y:WORD
EXTRN MOUSE_STACK_TOP:BYTE

cursor_plane_mask DW 0fh
CURSOR_SHOW_COUNT DW -1
CURSOR_Y DW 0
CURSOR_X DW 0

CURSOR_DATA SEGMENT WORD PUBLIC 'DATA'
DGROUP GROUP CURSOR_DATA
cursor_image_handle DW 0
cursor_hot_y DW 0
cursor_hot_x DW 0
cursor_mask_handle DW 0
cursor_background DD 0
cursor_background_segment EQU WORD PTR cursor_background+2
cursor_y0 LABEL DWORD
    DW 0
cursor_x0 DW 0
cursor_y1 LABEL DWORD
    DW 0
cursor_x1 DW 0
cursor_row_bytes DW 0
cursor_bitmap_flags DW 0
cursor_row_offsets DW 0, 0, 0, 0, 0, 0, 0, 0
                   DW 0, 0, 0, 0, 0, 0, 0, 0
cursor_pending DW 0
cursor_callback_depth DW -1
CURSOR_DATA ENDS

.CODE
EXTRN MEASURE_TEXT:NEAR
EXTRN SAVE_HANDLE_FLAGS:NEAR
EXTRN FETCH_LOCKED_SHAPE_LIST:NEAR
EXTRN convert_shape:NEAR
EXTRN DISPOSE_HANDLE:NEAR
EXTRN RESTORE_HANDLE_FLAGS:NEAR
EXTRN RELOCATE_HANDLE_PAYLOAD:NEAR
EXTRN ALLOCATE_BUFFER:NEAR
EXTRN CLAMP_POINT_TO_RECT:NEAR
EXTRN RELEASE_BUFFER:NEAR
EXTRN HGETRSRC:NEAR
EXTRN RESOLVE_HANDLE_TO_FAR_PAYLOAD:NEAR
EXTRN PAINT_FOUR_BIT:NEAR

PUBLIC UPDATE_MESSAGE_DISPLAY

UPDATE_MESSAGE_DISPLAY PROC FAR
    mov ax, CURSOR_SHOW_COUNT
    test ax, ax
    jl decrease_only
    jz swap_context
    neg ax
swap_context:
    dec ax
    mov CURSOR_SHOW_COUNT, ax
    sub ax, ax
    xchg ax, Graf
    push ax
    mov ax, OFFSET DEFAULT_PORT
    push ax
    mov ax, OFFSET cursor_background
    push ax
    add ax, 4
    push ds
    push ax
    push ds
    push ax
    mov ax, 0
    push ax
    push cs
    call MEASURE_TEXT
    pop Graf
    retf
decrease_only:
    dec ax
    mov CURSOR_SHOW_COUNT, ax
    retf
UPDATE_MESSAGE_DISPLAY ENDP

PUBLIC BUILD_CURSOR_IMAGES

BUILD_CURSOR_IMAGES PROC FAR
    push bp
    mov bp, sp
    add sp, -2ah
    push di
    push si
    mov di, [bp+6]
    push di
    push cs
    call SAVE_HANDLE_FLAGS
    push di
    push ax
    push di
    push cs
    call FETCH_LOCKED_SHAPE_LIST
    mov [bp-4], ax
    mov [bp-2], dx
    or ax, dx
    jz release_cursor_resource

    ; Translate the 16 by 16 mask and inverted image from resource byte order.
    mov ax, 10h
    mov [bp-28h], ax
    mov [bp-2ah], ax
    mov WORD PTR [bp-26h], 0
    push ds
    mov cx, 10h
    lds si, DWORD PTR [bp-4]
    add si, 24h
    lea di, [bp-24h]
    push ss
    pop es
read_cursor_mask:
    lodsw
    xchg al, ah
    stosw
    loop read_cursor_mask
    pop ds
    lea ax, [bp-2ah]
    push ds
    push ax
    sub ax, ax
    push ax
    push ax
    inc ax
    push ax
    push cs
    call convert_shape
    xchg cx, ax
    jcxz release_cursor_resource
    push cx
    push ds
    mov cx, 10h
    lds si, DWORD PTR [bp-4]
    add si, 4
    lea di, [bp-24h]
    push ss
    pop es
read_cursor_image:
    lodsw
    xchg al, ah
    not ax
    stosw
    loop read_cursor_image
    pop ds
    lea ax, [bp-2ah]
    push ds
    push ax
    sub ax, ax
    push ax
    push ax
    inc ax
    push ax
    push cs
    call convert_shape
    test ax, ax
    jnz install_cursor_images
    push cs
    call DISPOSE_HANDLE
release_cursor_resource:
    push cs
    call RESTORE_HANDLE_FLAGS
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 2

install_cursor_images:
    push ax
    add di, CURSOR_SHOW_COUNT
    jl dispose_old_images
    push cs
    call near ptr UPDATE_MESSAGE_DISPLAY
dispose_old_images:
    mov cx, cursor_image_handle
    jcxz dispose_old_mask
    push cx
    push cs
    call DISPOSE_HANDLE
dispose_old_mask:
    mov cx, cursor_mask_handle
    jcxz install_new_images
    push cx
    push cs
    call DISPOSE_HANDLE
install_new_images:
    push cs
    call RELOCATE_HANDLE_PAYLOAD
    mov cursor_mask_handle, ax
    push ax
    push cs
    call FETCH_LOCKED_SHAPE_LIST
    push cs
    call RELOCATE_HANDLE_PAYLOAD
    mov cursor_image_handle, ax
    push ax
    push cs
    call FETCH_LOCKED_SHAPE_LIST
    les bx, DWORD PTR [bp-4]
    les dx, DWORD PTR es:[bx]
    mov cx, es
    mov cursor_hot_x, cx
    mov cursor_hot_y, dx
    les ax, DWORD PTR CURSOR_Y
    sub ax, dx
    mov WORD PTR cursor_y0, ax
    add ax, 10h
    mov WORD PTR cursor_y1, ax
    mov ax, es
    sub ax, cx
    mov cursor_x0, ax
    add ax, 10h
    mov cursor_x1, ax
    test di, di
    jl release_cursor_resource
    push cs
    call near ptr SHOW_MOUSE_GRAPHICS
    jmp short release_cursor_resource
BUILD_CURSOR_IMAGES ENDP

PUBLIC INITIALIZE_MOUSE_GRAPHICS

INITIALIZE_MOUSE_GRAPHICS PROC FAR
    push di
    push si
    ; Get the mouse interrupt vector before querying the driver.
    mov ax, 3533h
    int 21h
    mov ax, es
    or ax, bx
    jz initialize_cursor_state
    sub ax, ax
    int 33h
initialize_cursor_state:
    mov MOUSE_PRESENT, ax
    mov ax, -1
    mov CURSOR_SHOW_COUNT, ax
    mov cursor_callback_depth, ax
    mov ax, 2
    int 33h

    les ax, DEFAULT_PORT_LIMITS
    shr ax, 1
    mov CURSOR_Y, ax
    mov ax, es
    shr ax, 1
    mov CURSOR_X, ax
    mov cx, 0eh
    mov si, OFFSET DEFAULT_PORT
    mov di, OFFSET cursor_background
    push ds
    pop es
    rep movsb
    mov ax, OFFSET cursor_row_offsets
    stosw
    xchg di, ax
    mov ax, 10h
    mov WORD PTR cursor_y1, ax
    mov cursor_x1, ax
    div PIXELS_PER_BYTE
    cbw
    mov cursor_row_bytes, ax
    mov cx, WORD PTR cursor_y1
    mul cx
    push dx
    push ax
    sub ax, ax
    mov cursor_pending, ax
initialize_cursor_rows:
    stosw
    add ax, cursor_row_bytes
    loop initialize_cursor_rows
    push cs
    call ALLOCATE_BUFFER
    mov WORD PTR cursor_background, ax
    mov cursor_background_segment, dx
    mov ax, 0
    push ax
    push cs
    call near ptr LOAD_CURSOR_RESOURCE
    mov cx, MOUSE_PRESENT
    jcxz mouse_init_done

    ; Set the hardware mouse limits and install the movement callback.
    sub cx, cx
    mov dx, DEFAULT_PORT_MOUSE_MAX_X
    cmp SCREEN_WIDTH, 140h
    jne set_mouse_x_limit
    shl dx, 1
set_mouse_x_limit:
    mov ax, 7
    int 33h
    sub cx, cx
    mov dx, MOUSE_MAX_Y
    mov ax, 8
    int 33h
    mov cx, 1
    push cs
    pop es
    mov dx, OFFSET HANDLE_MOUSE_CALLBACK
    mov ax, 0ch
    int 33h
    les dx, DWORD PTR MOUSE_MAX_Y
    mov ax, es
    sar ax, 1
    push ax
    sar dx, 1
    push dx
    push cs
    call near ptr POSITION_MOUSE_CURSOR
mouse_init_done:
    pop si
    pop di
    retf
INITIALIZE_MOUSE_GRAPHICS ENDP

PUBLIC POSITION_MOUSE_CURSOR

POSITION_MOUSE_CURSOR PROC FAR
    push bp
    mov bp, sp
    mov ax, OFFSET CURSOR_Y
    push ds
    push ax
    lea ax, [bp+6]
    push ds
    push ax
    mov ax, OFFSET DEFAULT_PORT_MOUSE_BOUNDS
    push ds
    push ax
    push cs
    call CLAMP_POINT_TO_RECT
    cmp CURSOR_SHOW_COUNT, 0
    jl set_mouse_position
    mov ax, CURSOR_X
    sub ax, cursor_hot_x
    cmp ax, cursor_x0
    jne repaint_moved_cursor
    mov ax, CURSOR_Y
    sub ax, cursor_hot_y
    cmp ax, WORD PTR cursor_y0
    je mouse_position_done
repaint_moved_cursor:
    push cs
    call near ptr UPDATE_MESSAGE_DISPLAY
    push cs
    call near ptr SHOW_MOUSE_GRAPHICS
set_mouse_position:
    mov cx, MOUSE_PRESENT
    jcxz mouse_position_done
    mov ax, 4
    les dx, DWORD PTR CURSOR_Y
    mov cx, es
    cmp SCREEN_WIDTH, 140h
    jne driver_set_position
    shl cx, 1
driver_set_position:
    int 33h
mouse_position_done:
    pop bp
    retf 4
POSITION_MOUSE_CURSOR ENDP

PUBLIC SHUTDOWN_MOUSE_GRAPHICS

SHUTDOWN_MOUSE_GRAPHICS PROC FAR
    cmp CURSOR_SHOW_COUNT, 0
    jl release_mouse_storage
    push cs
    call near ptr UPDATE_MESSAGE_DISPLAY
release_mouse_storage:
    sub ax, ax
    cwd
    xchg dx, cursor_background_segment
    xchg ax, WORD PTR cursor_background
    push dx
    push ax
    push cs
    call RELEASE_BUFFER
    push cursor_image_handle
    push cs
    call DISPOSE_HANDLE
    push cursor_mask_handle
    push cs
    call DISPOSE_HANDLE
    mov cx, MOUSE_PRESENT
    jcxz mouse_done
    sub ax, ax
    int 33h
mouse_done:
    retf
SHUTDOWN_MOUSE_GRAPHICS ENDP

PUBLIC LOAD_CURSOR_RESOURCE

LOAD_CURSOR_RESOURCE PROC FAR
    push bp
    mov bp, sp
    ; Resource type RSCU, passed as two Pascal word arguments.
    mov ax, 'CU'
    push ax
    mov ax, 'RS'
    push ax
    mov ax, [bp+6]
    push ax
    push cs
    call HGETRSRC
    push ax
    push cs
    call near ptr BUILD_CURSOR_IMAGES
    pop bp
    retf 2
LOAD_CURSOR_RESOURCE ENDP

PUBLIC SHOW_MOUSE_GRAPHICS

SHOW_MOUSE_GRAPHICS PROC FAR
    mov cx, CURSOR_SHOW_COUNT
    jcxz mouse_show_done
    cmp cx, -1
    jl increment_show_count
    je repaint_cursor
    dec cx
    mov CURSOR_SHOW_COUNT, cx
    jne mouse_show_done
    xchg cursor_pending, cx
    jcxz mouse_show_done
    push cs
    call near ptr UPDATE_MESSAGE_DISPLAY
    jmp SHORT repaint_cursor
increment_show_count:
    inc CURSOR_SHOW_COUNT
mouse_show_done:
    retf
repaint_cursor:
    mov bx, OFFSET DEFAULT_PORT
    mov ax, bx
    xchg Graf, ax
    push ax
    mov ax, CURSOR_X
    sub ax, cursor_hot_x
    add ax, [bx+6]
    mov cursor_x0, ax
    add ax, 10h
    mov cursor_x1, ax
    mov ax, CURSOR_Y
    sub ax, cursor_hot_y
    add ax, [bx+4]
    mov WORD PTR cursor_y0, ax
    add ax, 10h
    mov WORD PTR cursor_y1, ax
    mov ax, OFFSET cursor_background
    push ax
    push bx
    add ax, 4
    push ds
    push ax
    push ds
    push ax
    mov ax, 0
    push ax
    push cs
    call MEASURE_TEXT
    push cursor_mask_handle
    push cs
    call RESOLVE_HANDLE_TO_FAR_PAYLOAD
    push dx
    push ax
    les ax, DWORD PTR cursor_y0
    push es
    push ax
    mov ax, 1
    push ax
    mov ax, -1
    push ax
    push cs
    call PAINT_FOUR_BIT
    push cursor_image_handle
    push cs
    call RESOLVE_HANDLE_TO_FAR_PAYLOAD
    push dx
    push ax
    les ax, DWORD PTR cursor_y0
    push es
    push ax
    mov ax, 3
    push ax
    push cursor_plane_mask
    push cs
    call PAINT_FOUR_BIT
    pop Graf
    jmp increment_show_count
SHOW_MOUSE_GRAPHICS ENDP

PUBLIC HANDLE_MOUSE_CALLBACK

HANDLE_MOUSE_CALLBACK PROC FAR
    push ax
    push cx
    push ds
    cmp BYTE PTR cs:mouse_callback_busy, 0
    jne mouse_callback_exit
    mov BYTE PTR cs:mouse_callback_busy, 1
    test al, 1
    jz mouse_callback_exit
    mov ax, DGROUP
    mov ds, ax
    cmp SCREEN_WIDTH, 140h
    jne compare_mouse_position
    sar cx, 1
compare_mouse_position:
    cmp cx, CURSOR_X
    jne save_mouse_position
    cmp dx, CURSOR_Y
    je mouse_callback_exit
save_mouse_position:
    mov CURSOR_X, cx
    mov CURSOR_Y, dx
    cmp CURSOR_SHOW_COUNT, 0
    jl mouse_callback_exit
    jg defer_cursor_repaint
    inc cursor_callback_depth
    jne reset_callback_depth

    ; DOS can invoke the callback on its own stack. Switch to the
    ; game's stack for the graphics calls, then restore the caller's.
    push bx
    push dx
    push es
    push di
    push si
    mov di, ss
    mov si, sp
    cmp ax, di
    je repaint_from_callback
    mov ss, ax
    mov sp, OFFSET MOUSE_STACK_TOP
repaint_from_callback:
    cld
    push cs
    call near ptr UPDATE_MESSAGE_DISPLAY
    push cs
    call near ptr SHOW_MOUSE_GRAPHICS
    mov ss, di
    mov sp, si
    pop si
    pop di
    pop es
    pop dx
    pop bx
reset_callback_depth:
    dec cursor_callback_depth
mouse_callback_exit:
    mov BYTE PTR cs:mouse_callback_busy, 0
    pop ds
    pop cx
    pop ax
    retf

mouse_callback_busy DB 0
defer_cursor_repaint:
    mov cursor_pending, 1
    jmp short mouse_callback_exit
HANDLE_MOUSE_CALLBACK ENDP

PUBLIC CHECK_CURSOR_OVERLAP

CHECK_CURSOR_OVERLAP PROC NEAR
    cmp CURSOR_SHOW_COUNT, 0
    jge check_port
    dec CURSOR_SHOW_COUNT
    ret
check_port:
    push ax
    push bx
    push cx
    push dx
    push es
    cmp ax, WORD PTR DEFAULT_PORT
    jne increment_cursor_count
    mov ax, es
    cmp ax, DEFAULT_PORT_SEGMENT
    jne increment_cursor_count
    les dx, DEFAULT_PORT_ORIGIN
    mov cx, es
    les ax, cursor_y1
    sub ax, dx
    cmp ax, [bp-8]
    jle increment_cursor_count
    mov ax, es
    sub ax, cx
    cmp ax, [bp-6]
    jle increment_cursor_count
    les ax, cursor_y0
    sub ax, dx
    cmp ax, [bp-4]
    jge increment_cursor_count
    mov ax, es
    sub ax, cx
    cmp ax, [bp-2]
    jge increment_cursor_count
    push cs
    call near ptr UPDATE_MESSAGE_DISPLAY
    jmp SHORT overlap_done
increment_cursor_count:
    inc CURSOR_SHOW_COUNT
overlap_done:
    pop es
    pop dx
    pop cx
    pop bx
    pop ax
    ret
CHECK_CURSOR_OVERLAP ENDP

PUBLIC ALIGNMENT_0729
ALIGNMENT_0729 LABEL BYTE
    DB 0

PUBLIC PAINT_INDEXED_SHAPE, CHECK_CURSOR_OVERLAP_WRAPPER

ink_prefix EQU WORD PTR [bp-18h]
rows_left EQU WORD PTR [bp-16h]
pixels_wide EQU WORD PTR [bp-14h]
source_pitch EQU WORD PTR [bp-12h]
source_offset EQU WORD PTR [bp-10h]
odd_alignment EQU WORD PTR [bp-0eh]
port_pitch EQU WORD PTR [bp-0ch]
port_offset EQU WORD PTR [bp-0ah]
shape_clip_top EQU WORD PTR [bp-8]
shape_clip_left EQU WORD PTR [bp-6]
shape_clip_bottom EQU WORD PTR [bp-4]
shape_clip_right EQU WORD PTR [bp-2]

PAINT_INDEXED_SHAPE PROC FAR
    push bp
    mov bp, sp
    add sp, -18h
    push di
    push si
    mov bx, Graf
    les si, DWORD PTR [bp+0eh]
    mov ax, [bp+0ch]
    sub ax, [bx+6]
    sub cx, cx
    mov odd_alignment, cx
    mov di, ax
    sub di, [bx+1ah]
    jge store_left
    mov ax, [bx+1ah]
    sub cx, di
    test cx, 1
    jz store_left
    mov odd_alignment, 1
store_left:
    mov shape_clip_left, ax
    mov di, ax
    add di, es:[si+2]
    sub di, cx
    cmp di, [bx+1eh]
    jle store_right
    mov di, [bx+1eh]
store_right:
    mov shape_clip_right, di
    sub di, ax
    mov pixels_wide, di
    jle shape_ready
    mov di, [bp+0ah]
    sub di, [bx+4]
    sub dx, dx
    mov ax, di
    sub ax, [bx+18h]
    jge store_top
    mov di, [bx+18h]
    sub dx, ax
store_top:
    mov shape_clip_top, di
    mov source_offset, dx
    mov ax, di
    add ax, es:[si]
    sub ax, dx
    cmp ax, [bx+1ch]
    jle store_bottom
    mov ax, [bx+1ch]
store_bottom:
    mov shape_clip_bottom, ax
    sub ax, di
    jg start_painting
shape_ready:
    les ax, DWORD PTR [bp+0eh]
    mov dx, es
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 12
start_painting:
    mov rows_left, ax
    sub dx, dx
    mov ax, [bp+6]
    shr ax, 1
    jz save_ink_prefix
next_ink_bank:
    add dx, 10h
    shr ax, 1
    jnz next_ink_bank
save_ink_prefix:
    mov ink_prefix, dx
    les ax, DWORD PTR [bx]
    call near ptr CHECK_CURSOR_OVERLAP
    add ax, shape_clip_left
    shl di, 1
    add di, [bx+0eh]
    add ax, [di]
    mov port_offset, ax
    mov ax, [bx+0ch]
    mov port_pitch, ax
    mov ds, [bp+10h]
    mov ax, [si+4]
    mov source_pitch, ax
    mul source_offset
    mov bx, ax
    and ax, 0fh
    REPT 4
        shr dx, 1
        rcr bx, 1
    ENDM
    mov dx, ds
    add dx, bx
    mov ds, dx
    shr cx, 1
    add ax, cx
    add ax, 6
    add ax, si
    mov source_offset, ax
    mov dx, ink_prefix
    jmp SHORT draw_scanline
advance_scanline:
    dec rows_left
    jz restore_mouse
    mov cx, source_pitch
    add cx, source_offset
    mov ax, cx
    and cx, 0fh
    mov source_offset, cx
    mov cl, 4
    shr ax, cl
    mov cx, ds
    add ax, cx
    mov ds, ax
    mov ax, port_pitch
    add port_offset, ax
draw_scanline:
    mov cx, pixels_wide
    mov di, port_offset
    mov si, source_offset
    test odd_alignment, 1
    jnz skip_left_nibble
    jmp SHORT draw_nibble_pair
restore_mouse:
    push ss
    pop ds
    push cs
    call near ptr SHOW_MOUSE_GRAPHICS
    jmp shape_ready
skip_left_nibble:
    dec di
    lodsb
    jmp SHORT draw_right_nibble
draw_nibble_pair:
    dec cx
    lodsb
    mov ah, al
    and ah, 0f0h
    jz draw_right_nibble
    REPT 4
        shr ah, 1
    ENDM
    or ah, dl
    mov es:[di], ah
draw_right_nibble:
    inc di
    jcxz scanline_done
    and al, 0fh
    jz next_pair
    or al, dl
    mov es:[di], al
next_pair:
    inc di
    loop draw_nibble_pair
scanline_done:
    jmp SHORT advance_scanline
PAINT_INDEXED_SHAPE ENDP

CHECK_CURSOR_OVERLAP_WRAPPER PROC FAR
    call near ptr CHECK_CURSOR_OVERLAP
    retf
CHECK_CURSOR_OVERLAP_WRAPPER ENDP
END
