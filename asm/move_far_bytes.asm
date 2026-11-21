.MODEL MEDIUM
.CODE
PUBLIC MOVE_FAR_BYTES

MOVE_FAR_BYTES PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    push ds

    ; Normalize both far pointers to paragraph and low nibble.
    mov cl, 4
    mov dx, 0fh
    and [bp+8], dx
    lds si, DWORD PTR [bp+10]
    mov ax, si
    and si, dx
    shr ax, cl
    mov bx, ds
    add bx, ax
    mov ds, bx
    les di, DWORD PTR [bp+14]
    mov ax, di
    and di, dx
    shr ax, cl
    mov dx, es
    add dx, ax
    mov [bp+14], di
    mov [bp+16], dx

    cmp dx, bx
    ja check_overlap
    jb forward
    cmp di, si
    ja check_overlap

forward:
    mov bx, ds
forward_chunk:
    sub ax, ax
    mov cx, 0fff0h
    sub [bp+6], cx
    sbb [bp+8], ax
    jae forward_copy
    add cx, [bp+6]
    je finished
    mov [bp+6], ax
    mov [bp+8], ax
forward_copy:
    mov ax, si
    and si, 0fh
    shr ax, 1
    shr ax, 1
    shr ax, 1
    shr ax, 1
    add bx, ax
    mov ds, bx
    mov ax, di
    and di, 0fh
    shr ax, 1
    shr ax, 1
    shr ax, 1
    shr ax, 1
    add dx, ax
    mov es, dx
    shr cx, 1
    rep movsw
    jae forward_chunk
    movsb
    jmp short forward_chunk

finished:
    cld
    les ax, DWORD PTR [bp+14]
    mov dx, es
    pop ds
    pop si
    pop di
    pop bp
    retf 12

check_overlap:
    ; Compare the destination start to the source end.
    push dx
    mov dx, [bp+8]
    ror dx, cl
    mov ax, [bp+6]
    add ax, si
    adc bx, dx
    mov dx, ax
    and ax, 0fh
    shr dx, cl
    add bx, dx
    pop dx
    cmp dx, bx
    ja forward
    jb backward
    cmp di, ax
    jae forward

backward:
    ; Offset both end pointers to prepare for descending word copies.
    add ax, 0eh
    dec bx
    xchg ax, si
    mov ax, [bp+8]
    ror ax, cl
    add di, [bp+6]
    adc dx, ax
    add di, 0eh
    dec dx
    std
backward_chunk:
    sub ax, ax
    mov cx, 0ffe0h
    sub [bp+6], cx
    sbb [bp+8], ax
    jae backward_copy
    add cx, [bp+6]
    je finished
    mov [bp+6], ax
    mov [bp+8], ax
backward_copy:
    mov ax, si
    and si, 0fh
    shr ax, 1
    shr ax, 1
    shr ax, 1
    shr ax, 1
    add bx, ax
    sub si, 20h
    sub bx, 0ffeh
    mov ds, bx
    mov ax, di
    and di, 0fh
    shr ax, 1
    shr ax, 1
    shr ax, 1
    shr ax, 1
    add dx, ax
    sub di, 20h
    sub dx, 0ffeh
    mov es, dx
    shr cx, 1
    rep movsw
    jae backward_chunk
    inc di
    inc si
    movsb
    jmp short backward_chunk
MOVE_FAR_BYTES ENDP
END
