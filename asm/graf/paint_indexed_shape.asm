.MODEL MEDIUM
.DATA
EXTRN CURRENT_CONTEXT:WORD
.CODE
EXTRN CHECK_CURSOR_OVERLAP:NEAR
EXTRN SHOW_MOUSE_GRAPHICS:NEAR
PUBLIC PAINT_INDEXED_SHAPE, CHECK_CURSOR_OVERLAP_WRAPPER

ink_prefix EQU WORD PTR [bp-18h]
rows_left EQU WORD PTR [bp-16h]
pixels_wide EQU WORD PTR [bp-14h]
source_pitch EQU WORD PTR [bp-12h]
source_offset EQU WORD PTR [bp-10h]
odd_alignment EQU WORD PTR [bp-0eh]
port_pitch EQU WORD PTR [bp-0ch]
port_offset EQU WORD PTR [bp-0ah]
clip_top EQU WORD PTR [bp-8]
clip_left EQU WORD PTR [bp-6]
clip_bottom EQU WORD PTR [bp-4]
clip_right EQU WORD PTR [bp-2]

PAINT_INDEXED_SHAPE PROC FAR
    push bp
    mov bp, sp
    add sp, -18h
    push di
    push si
    mov bx, CURRENT_CONTEXT
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
    mov clip_left, ax
    mov di, ax
    add di, es:[si+2]
    sub di, cx
    cmp di, [bx+1eh]
    jle store_right
    mov di, [bx+1eh]
store_right:
    mov clip_right, di
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
    mov clip_top, di
    mov source_offset, dx
    mov ax, di
    add ax, es:[si]
    sub ax, dx
    cmp ax, [bx+1ch]
    jle store_bottom
    mov ax, [bx+1ch]
store_bottom:
    mov clip_bottom, ax
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
    call CHECK_CURSOR_OVERLAP
    add ax, clip_left
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
    call SHOW_MOUSE_GRAPHICS
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
    call CHECK_CURSOR_OVERLAP
    retf
CHECK_CURSOR_OVERLAP_WRAPPER ENDP
END
