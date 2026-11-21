.MODEL MEDIUM
.CODE

PUBLIC MASKTOMCGA

MASKTOMCGA PROC FAR
    push bp
    mov bp, sp
    push ds
    push si
    push di
    lds si, DWORD PTR [bp+10]
    lodsw
    or ax, ax
    je mask_finished
    mov bx, ax
    lodsw
    mov cx, ax
    lodsw
    lds si, DWORD PTR [bp+6]
    les di, DWORD PTR [bp+6]

    ; Expand in place from the end so the output cannot overwrite input bits.
    mul bx
    add si, ax
    dec si
    mov ax, cx
    mul bx
    add di, ax
    dec di
    std
    lodsb
    mov dx, cx
mask_next_row:
    mov cx, dx
    neg cl
    and cl, 7
    shr al, cl
    mov cl, dl
mask_next_byte:
    mov ah, al
mask_next_bit:
    shr ah, 1
    sbb al, al
    stosb
    dec cx
    test cl, 7
    jne mask_next_bit
    lodsb
    or cx, cx
    jne mask_next_byte
    dec bx
    jne mask_next_row
mask_finished:
    cld
    pop di
    pop si
    pop ds
    pop bp
    retf 8
MASKTOMCGA ENDP

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
    je pixels_finished

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
pixels_next_row:
    mov cx, WORD PTR [bp-2]
    test cl, 1
    jne pixels_high_nibble
pixels_low_nibble:
    mov ah, al
    and al, 0fh
    je pixels_store_low
    add al, dh
pixels_store_low:
    stosb
    dec cx
    mov al, ah
pixels_high_nibble:
    shr al, 1
    shr al, 1
    shr al, 1
    shr al, 1
    je pixels_store_high
    add al, dh
pixels_store_high:
    stosb
    lodsb
    loop pixels_low_nibble
    dec bx
    jne pixels_next_row
pixels_finished:
    cld
    pop di
    pop si
    pop ds
    mov sp, bp
    pop bp
    retf 10
TGATOMCGA ENDP
END
