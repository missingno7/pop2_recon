; DOS clipped RLE raster loop. The FM Towns PAINTRLE ASM public anchors the name.
; The caller passes a far shape pointer, x/y coordinates, and a transparency word.
; DX:AX returns the original far shape pointer.

EXTRN CHECK_CURSOR_OVERLAP_WRAPPER:FAR
EXTRN SHOW_MOUSE_GRAPHICS:FAR

_DATA SEGMENT WORD PUBLIC 'DATA'
EXTRN Graf:WORD
_DATA ENDS
DGROUP GROUP _DATA

PAINTRLE_TEXT SEGMENT BYTE PUBLIC 'CODE'
ASSUME CS:PAINTRLE_TEXT, DS:DGROUP, SS:DGROUP

right_edge EQU WORD PTR [bp-2]
transparent EQU WORD PTR [bp+6]
shape_y EQU WORD PTR [bp+8]
shape_x EQU WORD PTR [bp+0Ah]
shape_pointer EQU DWORD PTR [bp+0Ch]
shape_segment EQU WORD PTR [bp+0Eh]

bottom_edge EQU WORD PTR [bp-4]
left_edge EQU WORD PTR [bp-6]
top_edge EQU WORD PTR [bp-8]
destination_row EQU WORD PTR [bp-0Ah]
destination_stride EQU WORD PTR [bp-0Ch]
source_segment EQU WORD PTR [bp-0Eh]
source_offset EQU WORD PTR [bp-10h]
source_row EQU DWORD PTR [bp-10h]
left_skip EQU WORD PTR [bp-12h]
visible_width EQU WORD PTR [bp-14h]
rows_remaining EQU WORD PTR [bp-16h]

PUBLIC PAINTRLESHAPE_ASM
PAINTRLESHAPE_ASM PROC FAR
    push bp
    mov bp, sp
    add sp, -16h
    push di
    push si
    ; Intersect the shape bounds with the current graphics port clip rectangle.
    mov bx, Graf
    les si, shape_pointer
    mov ax, shape_x
    sub ax, WORD PTR [bx+6]
    sub cx, cx
    mov di, ax
    sub di, WORD PTR [bx+1Ah]
    jge x_start_ready
    mov ax, WORD PTR [bx+1Ah]
    sub cx, di
x_start_ready:
    mov left_edge, ax
    mov left_skip, cx
    mov di, ax
    add di, WORD PTR es:[si+2]
    sub di, cx
    cmp di, WORD PTR [bx+1Eh]
    jle x_end_ready
    mov di, WORD PTR [bx+1Eh]
x_end_ready:
    mov right_edge, di
    sub di, ax
    mov visible_width, di
    jle return_source_shape
    mov di, shape_y
    sub di, WORD PTR [bx+4]
    sub dx, dx
    mov ax, di
    sub ax, WORD PTR [bx+18h]
    jge y_start_ready
    mov di, WORD PTR [bx+18h]
    sub dx, ax
y_start_ready:
    mov top_edge, di
    mov ax, di
    add ax, WORD PTR es:[si]
    sub ax, dx
    cmp ax, WORD PTR [bx+1Ch]
    jle y_end_ready
    mov ax, WORD PTR [bx+1Ch]
y_end_ready:
    mov bottom_edge, ax
    sub ax, di
    jg prepare_paint
return_source_shape:
    les ax, shape_pointer
    mov dx, es
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 0Ah
prepare_paint:
    mov rows_remaining, ax
    les ax, DWORD PTR [bx]
    ; Call the mouse-overlap hook with the destination in ES:AX.
    call FAR PTR CHECK_CURSOR_OVERLAP_WRAPPER
    add ax, left_edge
    shl di, 1
    add di, WORD PTR [bx+0Eh]
    add ax, WORD PTR [di]
    mov destination_row, ax
    mov ax, WORD PTR [bx+0Ch]
    mov destination_stride, ax
    mov bx, shape_segment
    mov ds, bx
    add si, 6
    or dx, dx
    je save_row_pointer
    mov cl, 4
; Decode row lengths and normalize the huge source pointer as rows are skipped.
skip_source_rows:
    lodsw
    add ax, si
    mov si, ax
    and si, 0Fh
    shr ax, cl
    add bx, ax
    mov ds, bx
    dec dx
    jne skip_source_rows
save_row_pointer:
    mov source_offset, si
    mov source_segment, bx
    ; BX selects the packet loop used after left-edge clipping.
    mov bx, OFFSET opaque_draw_packet
    cmp transparent, 0
    je read_source_row
    mov bx, OFFSET transparent_draw_packet
    jmp SHORT read_source_row
advance_destination_row:
    dec rows_remaining
    je finish_paint
    mov ax, destination_stride
    add destination_row, ax
read_source_row:
    lds si, source_row
    lodsw
    add ax, si
    mov cx, ax
    and cx, 0Fh
    mov source_offset, cx
    mov cl, 4
    shr ax, cl
    mov cx, ds
    add ax, cx
    mov source_segment, ax
    mov di, destination_row
    sub dh, dh
    mov cx, left_skip
    jcxz read_first_visible_packet
; Consume complete packets to the left of the clipped destination.
clip_left_packet:
    lodsb
    mov dl, al
    and dl, 7Fh
    inc dl
    test al, al
    js clip_left_repeat
    add si, dx
    sub cx, dx
    ja clip_left_packet
    je read_first_visible_packet
    add si, cx
    mov ah, BYTE PTR [si]
    inc si
    neg cx
    jmp SHORT draw_first_visible_packet
clip_left_repeat:
    inc si
    sub cx, dx
    ja clip_left_packet
    je read_first_visible_packet
    neg cx
    mov ah, BYTE PTR [si-1]
    jmp SHORT draw_first_visible_packet
read_first_visible_packet:
    lodsw
    mov cl, al
    and cl, 7Fh
    inc cl
draw_first_visible_packet:
    mov dx, visible_width
    jmp bx
finish_paint:
    push ss
    pop ds
    call FAR PTR SHOW_MOUSE_GRAPHICS
    jmp NEAR PTR return_source_shape
; Opaque packets alternate literal copies and runs of a single byte.
opaque_next_packet:
    lodsw
    mov cl, al
    and cl, 7Fh
    inc cl
opaque_draw_packet:
    test al, al
    js opaque_repeat
    sub dx, cx
    jbe opaque_last_literal
    dec si
    shr cx, 1
    rep movsw WORD PTR es:[di], WORD PTR [si]
    rcl cx, 1
    rep movsb BYTE PTR es:[di], BYTE PTR [si]
    jmp SHORT opaque_next_packet
opaque_last_literal:
    add cx, dx
    dec si
    shr cx, 1
    rep movsw WORD PTR es:[di], WORD PTR [si]
    rcl cx, 1
    rep movsb BYTE PTR es:[di], BYTE PTR [si]
    jmp NEAR PTR advance_destination_row
opaque_repeat:
    sub dx, cx
    jbe opaque_last_repeat
    mov al, ah
    shr cx, 1
    rep stosw WORD PTR es:[di], ax
    rcl cx, 1
    rep stosb BYTE PTR es:[di], al
    jmp SHORT opaque_next_packet
opaque_last_repeat:
    add cx, dx
    mov al, ah
    shr cx, 1
    rep stosw WORD PTR es:[di], ax
    rcl cx, 1
    rep stosb BYTE PTR es:[di], al
    jmp NEAR PTR advance_destination_row
; In transparent mode skip runs whose repeated pixel is zero.
transparent_next_packet:
    lodsw
    mov cl, al
    and cl, 7Fh
    inc cl
transparent_draw_packet:
    test al, al
    js transparent_repeat
    sub dx, cx
    jbe transparent_last_literal
    dec si
    shr cx, 1
    rep movsw WORD PTR es:[di], WORD PTR [si]
    rcl cx, 1
    rep movsb BYTE PTR es:[di], BYTE PTR [si]
    jmp SHORT transparent_next_packet
transparent_last_literal:
    add cx, dx
    dec si
    shr cx, 1
    rep movsw WORD PTR es:[di], WORD PTR [si]
    rcl cx, 1
    rep movsb BYTE PTR es:[di], BYTE PTR [si]
    jmp NEAR PTR advance_destination_row
transparent_repeat:
    sub dx, cx
    jbe transparent_last_repeat
    test ah, ah
    je transparent_skip_repeat
    mov al, ah
    shr cx, 1
    rep stosw WORD PTR es:[di], ax
    rcl cx, 1
    rep stosb BYTE PTR es:[di], al
    jmp SHORT transparent_next_packet
transparent_skip_repeat:
    add di, cx
    jmp SHORT transparent_next_packet
transparent_last_repeat:
    add cx, dx
    test ah, ah
    je transparent_skip_tail
    mov al, ah
    shr cx, 1
    rep stosw WORD PTR es:[di], ax
    rcl cx, 1
    rep stosb BYTE PTR es:[di], al
    jmp NEAR PTR advance_destination_row
transparent_skip_tail:
    add di, cx
    jmp NEAR PTR advance_destination_row
PAINTRLESHAPE_ASM ENDP

PAINTRLE_TEXT ENDS
END
