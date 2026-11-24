.MODEL MEDIUM
.DATA
EXTRN Graf:WORD, DEFAULT_PORT:WORD
.CODE
EXTRN GET_BLOCK_PAYLOAD_SIZE:NEAR, SECTRECT:NEAR, DRAW_COUNTED_TEXT:NEAR
EXTRN STROKE_RECTANGLE:NEAR, OFFSETRECT:NEAR, MEASURE_TEXT:NEAR
EXTRN STROKE_PORT_RECT:NEAR, CHECK_CURSOR_OVERLAP:NEAR


PUBLIC QUERY_HANDLE_SIZE
QUERY_HANDLE_SIZE PROC FAR
    push bp
    mov bp, sp
    push si
    push ds
    mov bp, [bp+6]
    test bp, bp
    jz no_payload
    lds si, DWORD PTR [bp]
    test si, 200h
    jnz no_payload
    and si, 0fh
    mov ax, ds
    or ax, si
    jz no_payload
    call GET_BLOCK_PAYLOAD_SIZE
done:
    pop ds
    pop si
    pop bp
    retf 2
no_payload:
    sub ax, ax
    cwd
    jmp SHORT done
QUERY_HANDLE_SIZE ENDP

PUBLIC SUBTRACT_RECTANGLE_STRIP
cut_top EQU WORD PTR [bp-8]
cut_left EQU WORD PTR [bp-6]
cut_bottom EQU WORD PTR [bp-4]
cut_right EQU WORD PTR [bp-2]

SUBTRACT_RECTANGLE_STRIP PROC FAR
    push bp
    mov bp, sp
    add sp, -8
    push di
    push si
    push ds
    lea ax, cut_top
    push ds
    push ax
    les ax, DWORD PTR [bp+10]
    push es
    push ax
    les ax, DWORD PTR [bp+6]
    push es
    push ax
    push cs
    call SECTRECT
    lds si, DWORD PTR [bp+10]
    les di, DWORD PTR [bp+14]
    xchg cx, ax
    jcxz copy_source
    mov cx, 1
    mov ax, [si+2]
    mov bx, [si+6]
    mov dx, cut_right
    cmp ax, cut_left
    je check_right_cut
    xchg cut_left, ax
    mov cut_right, ax
    dec cx
check_right_cut:
    cmp bx, dx
    je check_top_cut
    jcxz copy_source
    xchg cut_right, bx
    mov cut_left, bx
    dec cx
check_top_cut:
    mov ax, [si]
    mov bx, [si+4]
    mov dx, cut_bottom
    cmp ax, cut_top
    je check_bottom_cut
    jcxz copy_source
    xchg cut_top, ax
    mov cut_bottom, ax
    dec cx
check_bottom_cut:
    cmp bx, dx
    je store_result
    jcxz copy_source
    xchg cut_bottom, bx
    mov cut_top, bx
    dec cx
store_result:
    jcxz copy_strip
    sub al, al
    mov cx, 8
    rep stosb
    jmp SHORT result_ready
copy_strip:
    lea si, cut_top
    push ss
    pop ds
copy_source:
    mov cx, 8
    rep movsb
result_ready:
    les ax, DWORD PTR [bp+14]
    mov dx, es
    pop ds
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 12
SUBTRACT_RECTANGLE_STRIP ENDP

; Recovered DOS graphics helpers in root order.
; The three zero bytes between entries are original unexecuted alignment fill.

public draw_character
draw_character proc far
    push bp
    mov bp, sp
    lea ax, [bp+6]
    push ds
    push ax
    mov ax, 1
    push ax
    push cs
    call draw_counted_text
    pop bp
    retf 2
draw_character endp



PUBLIC ADD_PORT_ORIGIN
ADD_PORT_ORIGIN PROC FAR
    push bp
    mov bp, sp
    push di
    mov bx, Graf
    les di, DWORD PTR [bp+6]
    les ax, DWORD PTR es:[di]
    add ax, [bx+4]
    mov cx, es
    add cx, [bx+6]
    les di, DWORD PTR [bp+10]
    stosw
    xchg cx, ax
    stosw
    mov ax, [bp+10]
    mov dx, es
    pop di
    pop bp
    retf 8
ADD_PORT_ORIGIN ENDP

public toggle_rect_effect
toggle_rect_effect proc far
    push bp
    mov bp, sp
    les ax, dword ptr [bp+6]
    push es
    push ax
    mov ax, 3
    push ax
    mov bx, Graf
    mov ax, 0fh
    push ax
    push cs
    call stroke_rectangle
    pop bp
    retf 4
toggle_rect_effect endp

public set_context_origin
set_context_origin proc far
    push bp
    mov bp, sp
    mov bx, [bp+10]
    sub cx, cx
    sub dx, dx
    sub cx, [bx+6]
    sub dx, [bx+4]
    sub cx, [bp+8]
    sub dx, [bp+6]
    lea ax, [bx+4]
    push ds
    push ax
    push ds
    push ax
    push cx
    push dx
    push cs
    call OFFSETRECT
    pop bp
    retf 6
set_context_origin endp

public origin_alignment
origin_alignment label byte
    db 0

public apply_default_pen_state
apply_default_pen_state proc far
    mov bx, Graf
    mov ax, DEFAULT_PORT+26h
    mov [bx+26h], ax
    mov ax, DEFAULT_PORT+28h
    mov [bx+28h], ax
    retf
apply_default_pen_state endp

public pen_alignment
pen_alignment label byte
    db 0



PUBLIC POINT_IN_RECTANGLE
POINT_IN_RECTANGLE PROC FAR
    push bp
    mov bp, sp
    push ds
    lds bx, DWORD PTR [bp+10]
    mov cx, [bx+2]
    mov dx, [bx]
    lds bx, DWORD PTR [bp+6]
    sub ax, ax
    cmp cx, [bx+2]
    jl test_done
    cmp cx, [bx+6]
    jge test_done
    cmp dx, [bx]
    jl test_done
    cmp dx, [bx+4]
    jge test_done
    inc ax
test_done:
    pop ds
    pop bp
    retf 8
POINT_IN_RECTANGLE ENDP

public scroll_context_rectangle
scroll_context_rectangle proc far
    push bp
    mov bp, sp
    add sp, -24
    push di
    push si
    lea si, [bp-24]              ; clipped source rectangle
    push ds
    push si
    push ds
    push si
    les ax, dword ptr [bp+14]
    push es
    push ax
    push ds
    push si
    mov di, Graf
    lea ax, [di+18h]
    push ds
    push ax
    lea bx, [si+4]
    push ds
    push bx
    add ax, 4
    push ds
    push ax
    push cs
    call near ptr add_port_origin
    push cs
    call near ptr add_port_origin
    push cs
    call SectRect
    test ax, ax
    jnz clip_found
    les di, dword ptr [bp+6]
    mov dx, es
    or dx, di
    jz return_source
    mov cx, 4
    rep stosw
return_source:
    les ax, dword ptr [bp+14]
    mov dx, es
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 12
clip_found:
    push di
    push di
    lea ax, [bp-8]               ; destination rectangle
    push ds
    push ax
    push ds
    push si
    mov ax, 0
    push ax
    mov si, 8
copy_rectangles:
    mov ax, [bp+si-26]
    mov [bp+si-18], ax
    mov [bp+si-10], ax
    mov ax, [bp+si-28]
    mov [bp+si-20], ax
    mov [bp+si-12], ax
    sub si, 4
    jnz copy_rectangles
    add si, [bp+12]
    jz shift_vertical
    jg shift_right
    sub [bp-22], si
    add [bp-2], si
    mov ax, [bp-2]
    mov [bp-14], ax
    jmp short shift_vertical
shift_right:
    sub [bp-18], si
    add [bp-6], si
    mov ax, [bp-6]
    mov [bp-10], ax
shift_vertical:
    mov dx, [bp+10]
    test dx, dx
    jz move_region
    jg shift_down
    sub [bp-24], dx
    add [bp-4], dx
    mov ax, [bp-4]
    mov [bp-16], ax
    jmp short move_region
shift_down:
    sub [bp-20], dx
    add [bp-8], dx
    mov ax, [bp-8]
    mov [bp-12], ax
move_region:
    push cs
    call MEASURE_TEXT
    les di, dword ptr [bp+6]
    mov dx, es
    or dx, di
    jz return_after_scroll
    lea si, [bp-16]              ; exposed rectangle
    push ds
    push si
    mov cx, 4
    rep movsw
    push cs
    call stroke_port_rect
return_after_scroll:
    jmp near ptr return_source
scroll_context_rectangle endp

public scroll_alignment
scroll_alignment label byte
    db 0

public check_context_cursor_overlap
check_context_cursor_overlap proc far
    push bp
    mov bp, sp
    add sp, -8
    lea ax, [bp-8]               ; local rectangle read through BP by the helper
    push ds
    push ax
    les ax, dword ptr [bp+6]
    push es
    push ax
    mov bx, Graf
    sub ax, ax
    cwd
    sub ax, [bx+6]
    sub dx, [bx+4]
    push ax
    push dx
    push cs
    call OFFSETRECT
    mov bx, Graf
    les ax, dword ptr [bx]
    call check_cursor_overlap
    mov sp, bp
    pop bp
    retf 4
check_context_cursor_overlap endp


END
