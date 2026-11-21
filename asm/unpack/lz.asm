.MODEL MEDIUM
.CODE
EXTRN _MALLOC:FAR
EXTRN _FREE:FAR
PUBLIC UNPACKLZH, UNPACKLZV

ring_base EQU WORD PTR [bp-2]
flags_queue EQU WORD PTR [bp-4]
column_wrap EQU WORD PTR [bp-6]

; The horizontal decoder writes a linear destination.
UNPACKLZH PROC FAR
    push bp
    mov bp, sp
    add sp, -4
    push di
    push si
    push ds
    mov si, WORD PTR [bp+8]
    mov ax, 400h
    push ax
    call _MALLOC
    add sp, 2
    test ax, ax
    jne h_init
h_return:
    xchg ax, si
    sub ax, WORD PTR [bp+8]
    pop ds
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 10
h_init:
    mov ring_base, ax
    mov cx, 200h
    xchg ax, di
    sub ax, ax
    push ds
    pop es
    rep stosw
    lea bx, [di-42h]
    mov cx, WORD PTR [bp+6]
    mov dx, di
    lds si, DWORD PTR [bp+8]
    les di, DWORD PTR [bp+12]
    mov flags_queue, ax
h_next_token:
    mov ax, flags_queue
    shr ax, 1
    test ah, ah
    jne h_flags_ready
    lodsb
    not ah
h_flags_ready:
    mov flags_queue, ax
    shr ax, 1
    jae h_match

    ; A set bit consumes a literal and also adds it to the dictionary.
    lodsb
    stosb
    mov ss:[bx], al
    inc bx
    cmp bx, dx
    jb h_literal_done
    mov bx, ring_base
h_literal_done:
    loop h_next_token
h_finished:
    push ss
    pop ds
    push ring_base
    call _FREE
    add sp, 2
    les ax, DWORD PTR [bp+12]
    mov dx, es
    jmp short h_return

h_match:
    ; The ten-bit position and six-bit length address a dictionary run.
    lodsb
    mov ah, al
    lodsb
    push ds
    push si
    mov si, ss
    mov ds, si
    mov si, ax
    and si, 3ffh
    add si, ring_base
    shr ah, 1
    shr ah, 1
    add ah, 3
h_match_byte:
    lodsb
    cmp si, dx
    jb h_match_read
    mov si, ring_base
h_match_read:
    stosb
    mov [bx], al
    inc bx
    cmp bx, dx
    jb h_match_written
    mov bx, ring_base
h_match_written:
    dec cx
    je h_match_final
    dec ah
    jne h_match_byte
    pop si
    pop ds
    jmp short h_next_token
h_match_final:
    pop si
    pop ds
    jmp short h_finished
UNPACKLZH ENDP

; The vertical decoder advances the destination by a stride, then wraps columns.
UNPACKLZV PROC FAR
    push bp
    mov bp, sp
    add sp, -6
    push di
    push si
    push ds
    mov si, WORD PTR [bp+12]
    mov ax, 400h
    push ax
    call _MALLOC
    add sp, 2
    test ax, ax
    jne v_init
v_return:
    xchg ax, si
    sub ax, WORD PTR [bp+12]
    pop ds
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 14
v_init:
    mov ring_base, ax
    mov cx, 200h
    xchg ax, di
    sub ax, ax
    push ds
    pop es
    rep stosw
    lea bx, [di-42h]
    mov cx, WORD PTR [bp+6]
    mov dx, di
    lds si, DWORD PTR [bp+12]
    les di, DWORD PTR [bp+16]
    mov flags_queue, ax
    mov ax, WORD PTR [bp+10]
    dec ax
    mov column_wrap, ax
v_next_token:
    mov ax, flags_queue
    shr ax, 1
    test ah, ah
    jne v_flags_ready
    lodsb
    not ah
v_flags_ready:
    mov flags_queue, ax
    shr ax, 1
    jae v_match

    lodsb
    mov es:[di], al
    add di, WORD PTR [bp+8]
    dec cx
    jne v_literal_advance
    sub di, column_wrap
    mov cx, WORD PTR [bp+6]
v_literal_advance:
    mov ss:[bx], al
    inc bx
    cmp bx, dx
    jb v_literal_done
    mov bx, ring_base
v_literal_done:
    dec WORD PTR [bp+10]
    jne v_next_token
v_finished:
    push ss
    pop ds
    push ring_base
    call _FREE
    add sp, 2
    les ax, DWORD PTR [bp+16]
    mov dx, es
    jmp short v_return

v_match:
    lodsb
    mov ah, al
    lodsb
    push ds
    push si
    mov si, ss
    mov ds, si
    mov si, ax
    and si, 3ffh
    add si, ring_base
    shr ah, 1
    shr ah, 1
    add ah, 3
v_match_byte:
    lodsb
    cmp si, dx
    jb v_match_read
    mov si, ring_base
v_match_read:
    mov es:[di], al
    add di, WORD PTR [bp+8]
    dec cx
    jne v_match_advance
    sub di, column_wrap
    mov cx, WORD PTR [bp+6]
v_match_advance:
    mov [bx], al
    inc bx
    cmp bx, dx
    jb v_match_written
    mov bx, ring_base
v_match_written:
    dec WORD PTR [bp+10]
    je v_match_final
    dec ah
    jne v_match_byte
    pop si
    pop ds
    jmp near ptr v_next_token
v_match_final:
    pop si
    pop ds
    jmp short v_finished
UNPACKLZV ENDP
END
