.MODEL MEDIUM
.DATA
EXTRN CURRENT_CONTEXT:WORD
EXTRN RELEASE_BUFFER_CALLBACK:WORD
EXTRN ALLOCATE_BUFFER_CALLBACK:WORD
.CODE
EXTRN CLEAR_FAR_BLOCK_FLAGS:NEAR
EXTRN ALLOCATE_FAR_BLOCK:NEAR
EXTRN COPY_CURRENT_PORT_ORIGIN:NEAR
EXTRN FIT_TEXT_PREFIX:NEAR
EXTRN RESOLVE_HANDLE_TO_FAR_PAYLOAD:NEAR
EXTRN MEASURE_COUNTED_TEXT:NEAR
EXTRN DRAW_COUNTED_TEXT:NEAR
EXTRN SET_RELATIVE_PEN_STATE:NEAR
EXTRN STROKE_RECTANGLE:NEAR

PUBLIC UNIONRECT

UNIONRECT PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    push ds
    les di, DWORD PTR [bp+10]
    lds si, DWORD PTR [bp+6]

    ; Select the lesser near edges and the greater far edges.
    mov ax, es:[di]
    cmp ax, [si]
    jle keep_first_near_649c
    mov ax, [si]
keep_first_near_649c:
    mov bx, es:[di+2]
    cmp bx, [si+2]
    jle keep_second_near_649c
    mov bx, [si+2]
keep_second_near_649c:
    mov cx, es:[di+4]
    cmp cx, [si+4]
    jge keep_first_far_649c
    mov cx, [si+4]
keep_first_far_649c:
    mov dx, es:[di+6]
    cmp dx, [si+6]
    jge keep_second_far_649c
    mov dx, [si+6]
keep_second_far_649c:
    les di, DWORD PTR [bp+14]
    stosw
    xchg ax, bx
    stosw
    xchg ax, cx
    stosw
    xchg ax, dx
    stosw
    mov ax, [bp+14]
    mov dx, es
    pop ds
    pop si
    pop di
    pop bp
    retf 12
UNIONRECT ENDP
PUBLIC alignment_64eb
alignment_64eb DB 0  ; Link-address alignment between routines.

PUBLIC RELEASE_BUFFER

RELEASE_BUFFER PROC FAR
    mov cx, release_buffer_callback
    jcxz default_release_64ec
    jmp cx

default_release_64ec:
    push bp
    mov bp, sp
    les ax, DWORD PTR [bp+6]
    push es
    push ax
    push cs
    call CLEAR_FAR_BLOCK_FLAGS
    pop bp
    retf 4
RELEASE_BUFFER ENDP

PUBLIC ALLOCATE_BUFFER

ALLOCATE_BUFFER PROC FAR
    mov cx, allocate_buffer_callback
    jcxz default_allocation_6504
    jmp cx

default_allocation_6504:
    push bp
    mov bp, sp
    les ax, DWORD PTR [bp+6]
    push es
    push ax
    push cs
    call ALLOCATE_FAR_BLOCK
    pop bp
    retf 4
ALLOCATE_BUFFER ENDP

PUBLIC DRAW_COUNTED_TEXT_IN_RECT

span_stack_base EQU WORD PTR [bp-14h]
rect_top EQU WORD PTR [bp-12h]
rect_height EQU WORD PTR [bp-10h]
rect_width EQU WORD PTR [bp-0eh]
line_count EQU WORD PTR [bp-4]
line_height EQU WORD PTR [bp-2]

DRAW_COUNTED_TEXT_IN_RECT PROC FAR
    push bp
    mov bp, sp
    add sp, -14h
    push di
    push si
    lea ax, [bp-0ch]
    push ss
    push ax
    push ss
    push ax
    push cs
    call COPY_CURRENT_PORT_ORIGIN
    les di, DWORD PTR [bp+10h]
    mov ax, di
    mov dx, es
    mov es, dx
    mov ax, es:[di+2]
    mov cx, es:[di+6]
    sub cx, ax
    mov rect_width, cx
    mov ax, es:[di]
    mov rect_top, ax
    mov cx, es:[di+4]
    sub cx, ax
    mov rect_height, cx
    mov line_count, 0
    mov di, [bp+6]
    mov si, [bp+8]
    mov span_stack_base, sp
measure_next_span_651c:
    push WORD PTR [bp+0ah]
    push si
    push di
    push rect_width
    push WORD PTR [bp+0eh]
    push cs
    call FIT_TEXT_PREFIX
    test ax, ax
    jz compute_vertical_position_651c
    push ax
    push si
    inc line_count
    add si, ax
    sub di, ax
    ja measure_next_span_651c
compute_vertical_position_651c:
    mov si, CURRENT_CONTEXT
    push WORD PTR [si+2ah]
    push cs
    call RESOLVE_HANDLE_TO_FAR_PAYLOAD
    xchg di, ax
    mov es, dx
    mov ax, es:[di+2]
    add ax, es:[di+4]
    add ax, es:[di+6]
    mov line_height, ax
    mul line_count
    sub ax, es:[di+6]
    mov dx, rect_top
    cmp WORD PTR [bp+0ch], 0
    jl apply_vertical_position_651c
    jg align_bottom_651c
    mov cx, rect_height
    shr cx, 1
    adc dx, cx
    shr ax, 1
    sbb dx, ax
    jmp SHORT apply_vertical_position_651c
align_bottom_651c:
    add dx, rect_height
    sub dx, ax
apply_vertical_position_651c:
    add dx, es:[di+2]
    mov [si+22h], dx
    mov di, span_stack_base
draw_next_span_651c:
    cmp di, sp
    je restore_pen_651c
    sub di, 4
    les bx, DWORD PTR [di]
    mov dx, es
    mov es, [bp+0ah]
    cmp WORD PTR [bp+0eh], 0
    jge measure_line_width_651c
    cmp bx, [bp+8]
    je measure_line_width_651c
    mov ax, es:[bx-1]
    cmp ah, ' '
    jne measure_line_width_651c
    cmp al, 0dh
    je measure_line_width_651c
    inc bx
    dec dx
    je measure_line_width_651c
    cmp BYTE PTR es:[bx], ' '
    jne measure_line_width_651c
    cmp al, '.'
    jne measure_line_width_651c
    inc bx
    dec dx
measure_line_width_651c:
    push es
    push bx
    push dx
    push es
    push bx
    push dx
    push cs
    call MEASURE_COUNTED_TEXT
    les bx, DWORD PTR [bp+10h]
    mov cx, es:[bx+2]
    cmp WORD PTR [bp+0eh], 0
    jl draw_line_651c
    jg align_right_651c
    mov bx, rect_width
    shr bx, 1
    add cx, bx
    shr ax, 1
    sub cx, ax
    jmp SHORT draw_line_651c
align_right_651c:
    add cx, rect_width
    sub cx, ax
draw_line_651c:
    mov [si+24h], cx
    push cs
    call DRAW_COUNTED_TEXT
    mov ax, line_height
    add [si+22h], ax
    jmp SHORT draw_next_span_651c
restore_pen_651c:
    mov sp, span_stack_base
    push cs
    call SET_RELATIVE_PEN_STATE
    les ax, DWORD PTR [bp+10h]
    mov dx, es
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 14
DRAW_COUNTED_TEXT_IN_RECT ENDP
PUBLIC alignment_664f
alignment_664f DB 0  ; Link-address alignment between routines.

PUBLIC SET_RECORD_RANGE

SET_RECORD_RANGE PROC FAR
    push bp
    mov bp, sp
    les ax, DWORD PTR [bp+8]
    push es
    push ax
    mov ax, 0
    push ax
    push WORD PTR [bp+6]
    push cs
    call STROKE_RECTANGLE
    pop bp
    retf 6
SET_RECORD_RANGE ENDP
END
