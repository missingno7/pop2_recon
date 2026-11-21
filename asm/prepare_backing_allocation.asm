.MODEL MEDIUM
.DATA
EXTRN HEAP_STATE:DWORD
.CODE
EXTRN RESIZE_DOS_HEAP:NEAR
EXTRN COALESCE_FREE_BLOCKS:NEAR
EXTRN EVICT_MOVABLE_BLOCK:NEAR
PUBLIC PREPARE_BACKING_ALLOCATION, MOVE_HUGE_BYTES


PUBLIC RELEASE_HEAP_SEGMENT
RELEASE_HEAP_SEGMENT PROC FAR
    les bx, HEAP_STATE
    mov dx, es:[0ch]
    test dx, dx
    jz release_dos_block
    mov ah, 45h
    int 67h
release_dos_block:
    mov ah, 49h
    int 21h
    retf
RELEASE_HEAP_SEGMENT ENDP

PREPARE_BACKING_ALLOCATION PROC FAR
    push bp
    mov bp, sp
    add sp, -8
    push di
    push si
    push ds

    les ax, DWORD PTR [bp+6]
    mov dx, es
    add ax, 7
    adc dx, 0
    jc max_size
    and al, 0feh
    cmp dx, 0fh
    jbe normalize_size
max_size:
    mov ax, 0ffffh
    mov dx, 0fh
normalize_size:
    mov bx, ax
    and ax, 0fh
    mov cl, 4
    shr bx, cl
    ror dx, cl
    add dx, bx
    mov [bp-8], ax
    mov [bp-6], dx
    mov WORD PTR [bp-4], 6
    mov WORD PTR [bp-2], 0
restart_search:
    lds si, DWORD PTR ss:[HEAP_STATE]
    lds si, DWORD PTR [si]
    mov di, ds
inspect_block:
    les ax, DWORD PTR [si]
    mov dx, es
    and ax, 0c007h
    shl ax, 1
    jnc free_or_boundary
    js movable_block
advance_block:
    add si, ax
    mov ax, si
    and si, 0fh
    shr ax, 1
    shr ax, 1
    shr ax, 1
    shr ax, 1
    add di, ax
    add di, dx
    mov ds, di
    jmp SHORT inspect_block
free_or_boundary:
    jns coalesce
try_resize:
    les ax, DWORD PTR [bp+6]
    push es
    push ax
    push cs
    call RESIZE_DOS_HEAP
    or ax, dx
    jne restart_search

save_best:
    les di, DWORD PTR ss:[HEAP_STATE]
    mov es:[di+4], si
    mov es:[di+6], ds
    les ax, DWORD PTR [bp-4]
    mov dx, es
    rol dx, 1
    rol dx, 1
    rol dx, 1
    rol dx, 1
    mov cx, dx
    and cx, 0fff0h
    and dx, 0fh
    add ax, cx
    sub ax, 6
    sbb dx, 0
    pop ds
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 4
movable_block:
    and ax, 0fh
    push ax
    push dx
    call EVICT_MOVABLE_BLOCK
    pop dx
    pop ax
    jnc advance_block
coalesce:
    call COALESCE_FREE_BLOCKS
    rcr bx, 1
    cmp dx, [bp-2]
    ja update_best
    jb inspect_flags
    cmp ax, [bp-4]
    jbe inspect_flags
update_best:
    mov [bp-4], ax
    mov [bp-2], dx
    cmp dx, [bp-6]
    ja save_best
    jb inspect_flags
    cmp ax, [bp-8]
    jae save_best
inspect_flags:
    shl bx, 1
    jnc try_resize
    jns advance_after_search
    or WORD PTR [si], 8000h
    push ds
    push si
    push dx
    push ax
    add si, ax
    mov ax, si
    and si, 0fh
    shr ax, 1
    shr ax, 1
    shr ax, 1
    shr ax, 1
    add ax, cx
    mov ds, ax
    call EVICT_MOVABLE_BLOCK
    cmc
    sbb cx, cx
    pop ax
    pop dx
    pop si
    pop ds
    and WORD PTR [si], 3fffh
    jcxz coalesce
advance_after_search:
    jmp NEAR PTR advance_block
PREPARE_BACKING_ALLOCATION ENDP

; Choose the copy direction from normalized source and destination ranges.
; Large ranges are copied in chunks that keep both offsets in their windows.
MOVE_HUGE_BYTES PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    push ds
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
    jb copy_forward
    cmp di, si
    ja check_overlap
copy_forward:
    mov bx, ds
forward_chunk:
    sub ax, ax
    mov cx, 0fff0h
    sub [bp+6], cx
    sbb [bp+8], ax
    jnc forward_normalize
    add cx, [bp+6]
    jz move_done
    mov [bp+6], ax
    mov [bp+8], ax
forward_normalize:
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
    jnc forward_chunk
    movsb
    jmp SHORT forward_chunk
move_done:
    cld
    les ax, DWORD PTR [bp+14]
    mov dx, es
    pop ds
    pop si
    pop di
    pop bp
    retf 12
check_overlap:
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
    ja copy_forward
    jb prepare_backward
    cmp di, ax
    jae copy_forward
prepare_backward:
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
    jnc backward_normalize
    add cx, [bp+6]
    jz move_done
    mov [bp+6], ax
    mov [bp+8], ax
backward_normalize:
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
    jnc backward_chunk
    inc di
    inc si
    movsb
    jmp SHORT backward_chunk
MOVE_HUGE_BYTES ENDP
END
