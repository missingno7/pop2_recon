.MODEL MEDIUM
.CODE
PUBLIC TGATOMCGA

TGATOMCGA PROC FAR
    push bp
    mov bp, sp
    add sp, -2
    push ds
    push si
    push di
    lds si, DWORD PTR [bp+12]
    lodsw
    mov bx, ax
    lodsw
    mov cx, ax
    mov WORD PTR [bp-2], cx
    lodsw
    lds si, DWORD PTR [bp+8]
    les di, DWORD PTR [bp+8]
    or bx, bx
    je finished

    ; Work backward through the packed bytes and expanded pixels in place.
    mul bx
    add si, ax
    dec si
    mov ax, cx
    mul bx
    add di, ax
    dec di
    mov dh, BYTE PTR [bp+6]
    shl dh, 1
    shl dh, 1
    shl dh, 1
    shl dh, 1
    std
    lodsb
next_row:
    mov cx, WORD PTR [bp-2]
    test cl, 1
    jne high_nibble
low_nibble:
    mov ah, al
    and al, 0fh
    je store_low
    add al, dh
store_low:
    stosb
    dec cx
    mov al, ah
high_nibble:
    shr al, 1
    shr al, 1
    shr al, 1
    shr al, 1
    je store_high
    add al, dh
store_high:
    stosb
    lodsb
    loop low_nibble
    dec bx
    jne next_row
finished:
    cld
    pop di
    pop si
    pop ds
    mov sp, bp
    pop bp
    retf 10
TGATOMCGA ENDP
END
