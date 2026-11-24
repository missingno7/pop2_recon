; MCGA1 mirror entries, in their DOS order. Names follow the FM Towns publics.
; MIRRORMCGABLOCK is the already accepted entry, included to show the run.
; Pascal stack arguments are row length, row count and a far pixel pointer.
; MIRROR4BITBLOCK has the extra odd-width flag at [bp+8].

EXTRN __nmalloc:FAR
EXTRN __nfree:FAR

MCGA1_TEXT SEGMENT BYTE PUBLIC 'CODE'
ASSUME CS:MCGA1_TEXT

rle_scratch EQU WORD PTR [bp-2]
rle_source EQU WORD PTR [bp-4]
rle_row_size EQU WORD PTR [bp-6]

PUBLIC MIRRORMCGABLOCK
MIRRORMCGABLOCK PROC FAR
    push bp
    mov bp, sp
    push ds
    push si
    push di
    les di, DWORD PTR [bp+0Ah]
    push es
    pop ds
mcga_row:
    mov bx, di
    mov si, di
    mov cx, WORD PTR [bp+6]
    add si, cx
    dec si
    inc cx
    shr cx, 1
mcga_swap_pair:
    mov ah, BYTE PTR [di]
    mov al, BYTE PTR [si]
    stosb
    mov BYTE PTR [si], ah
    dec si
    loop mcga_swap_pair
    mov di, bx
    add di, WORD PTR [bp+6]
    dec WORD PTR [bp+8]
    jne mcga_row
    pop di
    pop si
    pop ds
    pop bp
    retf 8
MIRRORMCGABLOCK ENDP

PUBLIC MIRRORRLEBLOCK
MIRRORRLEBLOCK PROC FAR
    push bp
    mov bp, sp
    ; Three local words track scratch pointer, row start, and encoded size.
    add sp, -6
    push si
    push di
    pushf
    cld
    mov ax, WORD PTR [bp+6]
    shl ax, 1
    push ax
    call FAR PTR __nmalloc
    add sp, 2
    mov rle_scratch, ax
    test ax, ax
    je rle_done
    push ds
    pop es
    lds si, DWORD PTR [bp+0Ah]
    sub ch, ch
rle_row:
    mov dx, WORD PTR [bp+8]
    test dx, dx
    je rle_release
    dec dx
    mov WORD PTR [bp+8], dx
    mov di, rle_scratch
    lodsw
    mov rle_source, si
    mov rle_row_size, ax
    mov dx, ax
    dec ax
    add di, ax
; Reverse packet order and literal bytes into the scratch buffer.
rle_packet:
    lodsb
    or al, al
    jns rle_literal
    movsb
    sub di, 2
    stosb
    sub di, 2
    sub dx, 2
    jmp SHORT rle_packet_done
rle_literal:
    mov cl, al
    inc cl
rle_reverse_literal:
    movsb
    dec dx
    sub di, 2
    loop rle_reverse_literal
    stosb
    dec dx
    sub di, 2
rle_packet_done:
    test dx, dx
    jne rle_packet
    inc di
    mov cx, rle_row_size
    mov si, rle_source
    ; Copy the reversed row back. SS addresses the near heap here.
    xchg di, si
    push ds
    pop es
    push ss
    pop ds
    rep movsb BYTE PTR es:[di], BYTE PTR [si]
    mov si, di
    push es
    pop ds
    push ss
    pop es
    jmp SHORT rle_row
rle_release:
    push es
    pop ds
    mov ax, rle_scratch
    push ax
    call FAR PTR __nfree
    add sp, 2
rle_done:
    popf
    pop di
    pop si
    mov sp, bp
    pop bp
    retf 8
MIRRORRLEBLOCK ENDP

PUBLIC MIRROR4BITBLOCK
MIRROR4BITBLOCK PROC FAR
    push bp
    mov bp, sp
    push ds
    push si
    push di
    les di, DWORD PTR [bp+0Ch]
    push es
    pop ds
packed_row:
    mov bx, di
    mov si, di
    mov cx, WORD PTR [bp+6]
    add si, cx
    dec si
    mov ax, WORD PTR [bp+8]
    and ax, 1
    jne packed_odd_width
    inc cx
    shr cx, 1
packed_swap_pair:
    mov ah, BYTE PTR [di]
    rol ah, 1
    rol ah, 1
    rol ah, 1
    rol ah, 1
    mov al, BYTE PTR [si]
    rol al, 1
    rol al, 1
    rol al, 1
    rol al, 1
    stosb
    mov BYTE PTR [si], ah
    dec si
    loop packed_swap_pair
packed_next_row:
    mov di, bx
    add di, WORD PTR [bp+6]
    dec WORD PTR [bp+0Ah]
    jne packed_row
    jmp SHORT packed_done
; An odd pixel width requires moving nibbles across byte boundaries.
packed_odd_width:
    cmp cx, 1
    je packed_done
    sub dx, dx
    mov ah, BYTE PTR [di]
    mov al, BYTE PTR [si]
    mov dh, ah
    mov BYTE PTR [si], dh
    mov dl, al
    sub cx, 2
    jne packed_seed_nibbles
    and ah, 0Fh
    or dl, al
    mov BYTE PTR [di], dl
    jmp SHORT packed_next_row
packed_seed_nibbles:
    and dh, 0Fh
    and dl, 0F0h
packed_shift_pair:
    inc di
    dec si
    mov ah, BYTE PTR [di]
    mov al, BYTE PTR [si]
    push ax
    and ah, 0F0h
    or dh, ah
    mov BYTE PTR [si], dh
    and al, 0Fh
    or dl, al
    dec di
    mov BYTE PTR [di], dl
    inc di
    pop ax
    sub dx, dx
    and ah, 0Fh
    mov dh, ah
    and al, 0F0h
    sub cx, 2
    jg packed_continue
    jl packed_next_row
    or dh, al
    mov BYTE PTR [di], dh
    jmp SHORT packed_next_row
packed_continue:
    mov dl, al
    jmp SHORT packed_shift_pair
packed_done:
    pop di
    pop si
    pop ds
    pop bp
    retf 0Ah
MIRROR4BITBLOCK ENDP

MCGA1_TEXT ENDS
END
