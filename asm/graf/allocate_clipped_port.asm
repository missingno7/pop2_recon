.MODEL MEDIUM
.DATA
EXTRN CURRENT_CONTEXT:WORD
EXTRN HORIZONTAL_PIXELS_PER_BYTE:BYTE
.CODE
EXTRN OFFSETRECT:NEAR
EXTRN ALLOCATE_BUFFER:NEAR
EXTRN BLIT_RECT:NEAR
EXTRN ALLOCATE_PORT_DESCRIPTOR:FAR
EXTRN FREE_PORT_DESCRIPTOR:FAR
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
    call OFFSETRECT
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
    jz port_ready
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
    jcxz allocate_pixels
fill_scanline_offsets:
    stosw
    add ax, [si+0ch]
    loop fill_scanline_offsets
allocate_pixels:
    mov ax, [bp-1ah]
    mul WORD PTR [si+0ch]
    push dx
    push ax
    push cs
    call ALLOCATE_BUFFER
    mov [si], ax
    mov [si+2], dx
    or ax, dx
    jne finish_bitmap
    push si
    call FREE_PORT_DESCRIPTOR
    add sp, 2
    sub ax, ax
port_ready:
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 4
finish_bitmap:
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
    jmp SHORT port_ready
ALLOCATE_CLIPPED_PORT ENDP
END
