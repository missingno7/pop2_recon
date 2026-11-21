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
    je finished
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
next_row:
    mov cx, dx
    neg cl
    and cl, 7
    shr al, cl
    mov cl, dl
next_byte:
    mov ah, al
next_bit:
    shr ah, 1
    sbb al, al
    stosb
    dec cx
    test cl, 7
    jne next_bit
    lodsb
    or cx, cx
    jne next_byte
    dec bx
    jne next_row
finished:
    cld
    pop di
    pop si
    pop ds
    pop bp
    retf 8
MASKTOMCGA ENDP
END
