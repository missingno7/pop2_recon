; Dither a source port into the current graphics port.
.MODEL MEDIUM
.DATA
EXTRN Graf:WORD
EXTRN DITHER_SEED:WORD
.CODE
EXTRN CHECK_CURSOR_OVERLAP:NEAR
EXTRN SHOW_MOUSE_GRAPHICS:NEAR

PUBLIC COPY_DITHERED_PORT_REGION

COPY_DITHERED_PORT_REGION PROC FAR
    push bp
    mov bp, sp
    add sp, -1ch
    push di
    push si

    ; Clip the source rectangle and remember the portion removed.
    mov bx, [bp+10h]
    mov ax, [bx+0ch]
    mov [bp-1ch], ax
    les si, [bp+8]
    sub dx, dx
    lods WORD PTR es:[si]
    sub ax, [bx+4]
    jge source_top
    xchg dx, ax
source_top:
    mov [bp-14h], ax
    sub cx, cx
    lods WORD PTR es:[si]
    sub ax, [bx+6]
    jge source_left
    xchg cx, ax
source_left:
    mov [bp-12h], ax
    sub di, di
    lods WORD PTR es:[si]
    sub ax, [bx+8]
    jle source_bottom
    xchg di, ax
source_bottom:
    mov [bp-10h], di
    sub di, di
    lods WORD PTR es:[si]
    sub ax, [bx+0ah]
    jle source_right
    xchg di, ax
source_right:
    mov [bp-0eh], di

    ; Clip the destination to its port and adjust the source to match.
    mov bx, [bp+12h]
    mov ax, [bx+0ch]
    mov [bp-4], ax
    les si, [bp+0ch]
    lods WORD PTR es:[si]
    sub ax, dx
    sub ax, [bx+4]
    jge destination_top
    sub [bp-14h], ax
    sub ax, ax
destination_top:
    mov [bp-0ch], ax
    lods WORD PTR es:[si]
    sub ax, cx
    sub ax, [bx+6]
    jge destination_left
    sub [bp-12h], ax
    sub ax, ax
destination_left:
    mov [bp-0ah], ax
    lods WORD PTR es:[si]
    sub ax, [bp-10h]
    cmp ax, [bx+8]
    jle destination_bottom
    mov ax, [bx+8]
destination_bottom:
    sub ax, [bx+4]
    xchg dx, ax
    lods WORD PTR es:[si]
    sub ax, [bp-0eh]
    cmp ax, [bx+0ah]
    jle destination_right
    mov ax, [bx+0ah]
destination_right:
    sub ax, [bx+6]
    xchg cx, ax

    cmp bx, Graf
    jne measure_region
    mov ax, [bx+18h]
    sub ax, [bp-0ch]
    jle clip_left
    add [bp-0ch], ax
    add [bp-14h], ax
clip_left:
    mov ax, [bx+1ah]
    sub ax, [bp-0ah]
    jle clip_bottom
    add [bp-0ah], ax
    add [bp-12h], ax
clip_bottom:
    cmp dx, [bx+1ch]
    jle clip_right
    mov dx, [bx+1ch]
clip_right:
    cmp cx, [bx+1eh]
    jle measure_region
    mov cx, [bx+1eh]
measure_region:
    mov [bp-8], dx
    mov [bp-6], cx
    sub dx, [bp-0ch]
    jle copy_done
    sub cx, [bp-0ah]
    jg prepare_rows
copy_done:
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 0eh

prepare_rows:
    mov [bp-16h], cx
    mov [bp-18h], dx
    add cx, [bp-12h]
    mov [bp-0eh], cx
    add dx, [bp-14h]
    mov [bp-10h], dx
    mov si, bp
    mov di, [si+10h]
    les ax, [di]
    lea bp, [si-0ch]
    call near ptr CHECK_CURSOR_OVERLAP
    les ax, [bx]
    lea bp, [si-4]
    call near ptr CHECK_CURSOR_OVERLAP
    mov bp, si

    mov ax, [bp-0ah]
    mov dx, [bp-12h]
    les cx, [di]
    mov di, [bp-0ch]
    mov si, [bp-14h]
    cmp cx, [bx]
    jne compute_addresses
    mov cx, es
    cmp cx, [bx+2]
    jne compute_addresses
    cmp di, si
    jle choose_direction
    mov cx, [bp-18h]
    dec cx
    add di, cx
    add si, cx
    neg WORD PTR [bp-4]
    neg WORD PTR [bp-1ch]
choose_direction:
    cmp ax, dx
    jle compute_addresses
    mov cx, [bp-16h]
    dec cx
    add ax, cx
    add dx, cx
    std
compute_addresses:
    les cx, [bx]
    add ax, cx
    shl di, 1
    add di, [bx+0eh]
    add ax, [di]
    mov [bp-2], ax

    mov bx, [bp+10h]
    lds cx, [bx]
    add dx, cx
    shl si, 1
    add si, ss:[bx+0eh]
    add dx, ss:[si]
    mov [bp-1ah], dx
    push ds
    mov dx, ss
    mov ds, dx
    mov dx, DITHER_SEED
    pop ds
    mov bx, [bp+6]
    sub bh, bh
    jmp short load_row

restore_cursor:
    cld
    push ss
    pop ds
    mov DITHER_SEED, dx
    push cs
    call near ptr SHOW_MOUSE_GRAPHICS
    push cs
    call near ptr SHOW_MOUSE_GRAPHICS
    jmp copy_done
advance_row:
    dec WORD PTR [bp-18h]
    je restore_cursor
    mov ax, [bp-4]
    add [bp-2], ax
    mov ax, [bp-1ch]
    add [bp-1ah], ax
load_row:
    mov cx, [bp-16h]
    mov di, [bp-2]
    mov si, [bp-1ah]
next_pixel:
    test bh, bh
    je low_seed_byte
    cmp bh, 1
    je high_seed_byte
    mov ax, dx
    mov dx, 5
    mul dx
    inc ax
    mov dx, ax
    sub bh, bh
low_seed_byte:
    mov al, dl
    jmp short apply_threshold
high_seed_byte:
    mov al, dh
apply_threshold:
    inc bh
    cmp al, bl
    ja skip_pixel
    movsb
    jmp short pixel_done
skip_pixel:
    inc si
    inc di
pixel_done:
    loop next_pixel
    jmp short advance_row
COPY_DITHERED_PORT_REGION ENDP
END
