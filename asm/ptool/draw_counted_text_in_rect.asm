.MODEL MEDIUM
.DATA
EXTRN CURRENT_CONTEXT:WORD
.CODE
EXTRN COPY_CURRENT_PORT_ORIGIN:NEAR
EXTRN FIT_TEXT_PREFIX:NEAR
EXTRN RESOLVE_HANDLE_TO_FAR_PAYLOAD:NEAR
EXTRN MEASURE_COUNTED_TEXT:NEAR
EXTRN DRAW_COUNTED_TEXT:NEAR
EXTRN SET_RELATIVE_PEN_STATE:NEAR
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
measure_next_span:
    push WORD PTR [bp+0ah]
    push si
    push di
    push rect_width
    push WORD PTR [bp+0eh]
    push cs
    call FIT_TEXT_PREFIX
    test ax, ax
    jz compute_vertical_position
    push ax
    push si
    inc line_count
    add si, ax
    sub di, ax
    ja measure_next_span
compute_vertical_position:
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
    jl apply_vertical_position
    jg align_bottom
    mov cx, rect_height
    shr cx, 1
    adc dx, cx
    shr ax, 1
    sbb dx, ax
    jmp SHORT apply_vertical_position
align_bottom:
    add dx, rect_height
    sub dx, ax
apply_vertical_position:
    add dx, es:[di+2]
    mov [si+22h], dx
    mov di, span_stack_base
draw_next_span:
    cmp di, sp
    je restore_pen
    sub di, 4
    les bx, DWORD PTR [di]
    mov dx, es
    mov es, [bp+0ah]
    cmp WORD PTR [bp+0eh], 0
    jge measure_line_width
    cmp bx, [bp+8]
    je measure_line_width
    mov ax, es:[bx-1]
    cmp ah, ' '
    jne measure_line_width
    cmp al, 0dh
    je measure_line_width
    inc bx
    dec dx
    je measure_line_width
    cmp BYTE PTR es:[bx], ' '
    jne measure_line_width
    cmp al, '.'
    jne measure_line_width
    inc bx
    dec dx
measure_line_width:
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
    jl draw_line
    jg align_right
    mov bx, rect_width
    shr bx, 1
    add cx, bx
    shr ax, 1
    sub cx, ax
    jmp SHORT draw_line
align_right:
    add cx, rect_width
    sub cx, ax
draw_line:
    mov [si+24h], cx
    push cs
    call DRAW_COUNTED_TEXT
    mov ax, line_height
    add [si+22h], ax
    jmp SHORT draw_next_span
restore_pen:
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
END
