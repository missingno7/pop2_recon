.MODEL MEDIUM
.DATA
EXTRN CURRENT_CONTEXT:WORD
EXTRN HORIZONTAL_PIXELS_PER_BYTE:BYTE
paint_four_bit_callback DW 0
stroke_rectangle_callback DW 0
paint_plain_callback DW 0
.CODE
EXTRN BLIT_RECT:NEAR
EXTRN RELEASE_BUFFER:NEAR
EXTRN FREE_PORT_DESCRIPTOR:FAR
EXTRN ALLOCATE_BUFFER:NEAR
EXTRN ALLOCATE_PORT_DESCRIPTOR:FAR

PUBLIC MOVETO

MOVETO PROC FAR
    push bp
    mov bp, sp
    mov bx, CURRENT_CONTEXT
    ; LES loads both coordinate words. No memory is accessed through ES.
    les ax, DWORD PTR [bp+6]
    mov [bx+22h], ax
    mov [bx+24h], es
    pop bp
    retf 4
MOVETO ENDP

PUBLIC OFFSETRECT

OFFSETRECT PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    push ds
    lds si, DWORD PTR [bp+10]
    les di, DWORD PTR [bp+14]
    push di

    ; Translate corresponding edges in top, left, bottom, right order.
    lodsw
    add ax, [bp+6]
    stosw
    lodsw
    add ax, [bp+8]
    stosw
    lodsw
    add ax, [bp+6]
    stosw
    lodsw
    add ax, [bp+8]
    stosw

    pop ax
    mov dx, es
    pop ds
    pop si
    pop di
    pop bp
    retf 12
OFFSETRECT ENDP
PUBLIC alignment_5135
alignment_5135 DB 0  ; Link-address alignment between routines.

PUBLIC PAINT_FOUR_BIT

PAINT_FOUR_BIT PROC FAR
    jmp WORD PTR paint_four_bit_callback
PAINT_FOUR_BIT ENDP

PUBLIC PAINT_PLAIN

PAINT_PLAIN PROC FAR
    jmp WORD PTR paint_plain_callback
PAINT_PLAIN ENDP

PUBLIC CLAMP_POINT_TO_RECT

CLAMP_POINT_TO_RECT PROC FAR
    push bp
    mov bp, sp
    push di
    push ds
    lds bx, DWORD PTR [bp+6]
    les di, DWORD PTR [bp+0ah]
    mov cx, es:[di+2]
    mov ax, es:[di]
    cmp cx, [bx+2]
    jge test_right_edge_513e
    mov cx, [bx+2]
test_right_edge_513e:
    cmp cx, [bx+6]
    jl test_top_edge_513e
    mov cx, [bx+6]
    dec cx
test_top_edge_513e:
    cmp ax, [bx]
    jge test_bottom_edge_513e
    mov ax, [bx]
test_bottom_edge_513e:
    cmp ax, [bx+4]
    jl write_clamped_point_513e
    mov ax, [bx+4]
    dec ax
write_clamped_point_513e:
    les di, DWORD PTR [bp+0eh]
    stosw
    xchg cx, ax
    stosw
    mov ax, [bp+0eh]
    mov dx, es
    pop ds
    pop di
    pop bp
    retf 12
CLAMP_POINT_TO_RECT ENDP
PUBLIC alignment_5181
alignment_5181 DB 0  ; Link-address alignment between routines.

PUBLIC DISPOSE_BITMAP_PORT

DISPOSE_BITMAP_PORT PROC FAR
    push bp
    mov bp, sp
    mov bx, [bp+6]
    push bx
    les ax, DWORD PTR [bx]
    push es
    push ax
    push CURRENT_CONTEXT
    push bx
    add bx, 10h
    push ds
    push bx
    push ds
    push bx
    mov ax, 0
    push ax
    push cs
    call BLIT_RECT
    push cs
    call RELEASE_BUFFER
    call FREE_PORT_DESCRIPTOR
    add sp, 2
    pop bp
    retf 2
DISPOSE_BITMAP_PORT ENDP
PUBLIC alignment_51b1
alignment_51b1 DB 0  ; Link-address alignment between routines.

PUBLIC ALLOCATE_CLIPPED_PORT

ALLOCATE_CLIPPED_PORT PROC FAR
    push bp
    mov bp, sp
    add sp, -1ah
    push di
    push si
    mov bx, CURRENT_CONTEXT
    mov cx, 10h
    mov si, bx
    lea di, [bp-18h]
    push ds
    pop es
    rep movsb
    push ds
    mov cx, 4
    lds si, DWORD PTR [bp+6]
    lea di, [bp-8]
    rep movsw
    pop ds
    mov ax, [bp-6]
    sub ax, [bx+6]
    mov cl, HORIZONTAL_PIXELS_PER_BYTE
    dec cx
    and ax, cx
    mov di, ax
    add ax, [bp-2]
    sub ax, [bp-6]
    add ax, cx
    inc cx
    cwd
    div cx
    mov [bp-0ch], ax
    lea ax, [bp-14h]
    push ds
    push ax
    lea ax, [bp-8]
    push ds
    push ax
    neg di
    push di
    sub ax, ax
    push ax
    push cs
    call NEAR PTR OFFSETRECT
    sub [bp-0eh], di
    mov ax, [bp-4]
    sub ax, [bp-8]
    mov [bp-1ah], ax
    shl ax, 1
    add ax, 18h
    push ax
    call ALLOCATE_PORT_DESCRIPTOR
    add sp, 2
    test ax, ax
    jz port_ready_51b2
    mov cx, 18h
    mov di, ax
    lea si, [bp-18h]
    push ds
    pop es
    rep movsb
    xchg si, ax
    mov [si+0eh], di
    sub ax, ax
    mov cx, [bp-1ah]
    jcxz allocate_pixels_51b2
fill_scanline_offsets_51b2:
    stosw
    add ax, [si+0ch]
    loop fill_scanline_offsets_51b2
allocate_pixels_51b2:
    mov ax, [bp-1ah]
    mul WORD PTR [si+0ch]
    push dx
    push ax
    push cs
    call ALLOCATE_BUFFER
    mov [si], ax
    mov [si+2], dx
    or ax, dx
    jne finish_bitmap_51b2
    push si
    call FREE_PORT_DESCRIPTOR
    add sp, 2
    sub ax, ax
port_ready_51b2:
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 4
finish_bitmap_51b2:
    push si
    push CURRENT_CONTEXT
    lea ax, [si+10h]
    push ds
    push ax
    push ds
    push ax
    mov ax, 0
    push ax
    push cs
    call BLIT_RECT
    xchg si, ax
    jmp SHORT port_ready_51b2
ALLOCATE_CLIPPED_PORT ENDP
PUBLIC alignment_5283
alignment_5283 DB 0  ; Link-address alignment between routines.

PUBLIC SECTRECT

SECTRECT PROC FAR
    push bp
    mov bp, sp
    push di
    push ds
    les bx, DWORD PTR [bp+10]
    lds di, DWORD PTR [bp+6]

    ; Intersect left/right edges first. An empty intersection clears the output.
    mov ax, es:[bx+2]
    cmp ax, [di+2]
    jge have_left_5284
    mov ax, [di+2]
have_left_5284:
    mov dx, es:[bx+6]
    cmp dx, [di+6]
    jle have_right_5284
    mov dx, [di+6]
have_right_5284:
    cmp ax, dx
    jge clear_rect_5284
    lds di, DWORD PTR [bp+14]
    mov [di+2], ax
    mov [di+6], dx

    lds di, DWORD PTR [bp+6]
    mov ax, es:[bx]
    cmp ax, [di]
    jge have_top_5284
    mov ax, [di]
have_top_5284:
    mov dx, es:[bx+4]
    cmp dx, [di+4]
    jle have_bottom_5284
    mov dx, [di+4]
have_bottom_5284:
    cmp ax, dx
    jge clear_rect_5284
    lds di, DWORD PTR [bp+14]
    mov [di], ax
    mov [di+4], dx
    mov ax, 1
    jmp short done_5284
clear_rect_5284:
    sub ax, ax
    mov cx, 4
    les di, DWORD PTR [bp+14]
    rep stosw
done_5284:
    pop ds
    pop di
    pop bp
    retf 12
SECTRECT ENDP
END
