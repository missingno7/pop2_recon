.MODEL MEDIUM
.DATA
EXTRN HEAP_STATE:DWORD
.CODE
EXTRN ALLOCATE_BACKING_SPACE:NEAR
EXTRN MOVE_HUGE_BYTES:NEAR
PUBLIC ALLOCATE_MOVABLE_SLOT, GET_BLOCK_PAYLOAD_SIZE, COALESCE_FREE_BLOCKS, EVICT_MOVABLE_BLOCK

ALLOCATE_MOVABLE_SLOT PROC NEAR
    push bp
    mov bp, sp
    add sp, -12
    push di
    mov [bp-12], ax
    mov [bp-10], dx
    sub cx, cx
    mov [bp-2], cx
    add ax, 7
    adc dx, cx
    jc allocation_failed
    and al, 0feh
    cmp dx, 0fh
    jbe normalize_size
allocation_failed:
    clc
    jmp NEAR PTR slot_done
normalize_size:
    mov bx, ax
    and ax, 0fh
    mov cl, 4
    shr bx, cl
    ror dx, cl
    add dx, bx
    mov [bp-8], ax
    mov [bp-6], dx

    lds si, DWORD PTR ss:[HEAP_STATE]
    lds si, DWORD PTR [si+4]
    mov di, ds
slot_scan:
    les ax, DWORD PTR [si]
    mov dx, es
    and ax, 0c007h
    shl ax, 1
    jnc test_free_block
    and ax, 0fh
next_block:
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
    jmp SHORT slot_scan

test_free_block:
    js final_candidate
    call COALESCE_FREE_BLOCKS
    les bx, DWORD PTR [bp-8]
    mov cx, es
    cmp dx, cx
    ja remember_block
    jb next_block
    cmp ax, bx
    jb next_block
remember_block:
    mov [bp-4], si
    mov [bp-2], ds
    jmp SHORT next_block

final_candidate:
    mov cx, [bp-2]
    jcxz slot_done
    lds si, DWORD PTR [bp-4]
    les ax, DWORD PTR [si]
    shl ax, 1
    and ax, 0fh
    mov dx, es
    les bx, DWORD PTR [bp-8]
    mov di, es
    sub ax, bx
    jnc subtract_paragraphs
    and ax, 0fh
    dec dx
subtract_paragraphs:
    sub dx, di
    ja split_block
    cmp ax, 6
    jae split_block
    add ax, bx
    and ax, 0fh
    jmp SHORT set_block_header
split_block:
    shr ax, 1
    mov [si], ax
    mov [si+2], dx
    shl ax, 1
    add si, ax
    mov ax, si
    and si, 0fh
    shr ax, 1
    shr ax, 1
    shr ax, 1
    shr ax, 1
    add cx, ax
    add cx, dx
    mov ds, cx
    xchg ax, bx
    mov [si+2], di
set_block_header:
    mov dx, ax
    shr ax, 1
    mov cx, [bp-12]
    and cx, 0fh
    sub dx, cx
    sub dx, 6
    shl dx, 1
    shl dx, 1
    shl dx, 1
    and dx, 38h
    or ax, dx
    mov [si], ax
    les di, DWORD PTR ss:[HEAP_STATE]
    mov es:[di+4], si
    mov es:[di+6], ds
    stc
slot_done:
    les ax, DWORD PTR [bp-12]
    mov dx, es
    pop di
    mov sp, bp
    pop bp
    ret
ALLOCATE_MOVABLE_SLOT ENDP

GET_BLOCK_PAYLOAD_SIZE PROC NEAR
    les ax, DWORD PTR [si]
    mov bx, ax
    and ax, 7
    shl ax, 1
    mov dx, es
    rol dx, 1
    rol dx, 1
    rol dx, 1
    rol dx, 1
    mov cx, dx
    and cx, 0fff0h
    and dx, 0fh
    add ax, cx
    and bx, 38h
    shr bx, 1
    shr bx, 1
    shr bx, 1
    add bx, 6
    sub ax, bx
    sbb dx, 0
    ret
GET_BLOCK_PAYLOAD_SIZE ENDP

COALESCE_FREE_BLOCKS PROC NEAR
merge_next:
    mov bx, ax
    mov cx, ds
    add cx, dx
    mov es, cx
    les bx, DWORD PTR es:[bx+si]
    and bx, 0c007h
    shl bx, 1
    jc done
    js done
    mov cx, es
    add ax, bx
    mov bx, ax
    and ax, 0fh
    shr bx, 1
    shr bx, 1
    shr bx, 1
    shr bx, 1
    add dx, bx
    add dx, cx
    shr ax, 1
    and WORD PTR [si], 0fff8h
    or [si], ax
    mov [si+2], dx
    shl ax, 1
    jmp SHORT merge_next
done:
    ret
COALESCE_FREE_BLOCKS ENDP

EVICT_MOVABLE_BLOCK PROC NEAR
    push bp
    mov bp, [si+4]
    mov ax, [bp]
    test ax, 8000h
    jnz evict_done
    push ax
    or WORD PTR [bp], 8000h
    push ds
    push si
    call GET_BLOCK_PAYLOAD_SIZE
    les bx, DWORD PTR ss:[HEAP_STATE]
    mov es:[bx+4], si
    mov es:[bx+6], ds
    call ALLOCATE_MOVABLE_SLOT
    jc allocated
    call ALLOCATE_BACKING_SPACE
allocated:
    push ds
    pop es
    mov bx, si
    pop si
    pop ds
    pop WORD PTR [bp]
    jnc evict_done
    mov cx, [si]
    and WORD PTR [si], 3fffh
    and cx, 0ffc0h
    and WORD PTR es:[bx], 3fh
    or es:[bx], cx
    mov es:[bx+4], bp
    and WORD PTR [bp], 0fff0h
    or [bp], bx
    mov [bp+2], es
    add bx, 6
    push es
    push bx
    lea cx, [si+6]
    push ds
    push cx
    push dx
    push ax
    push cs
    call MOVE_HUGE_BYTES
    stc
evict_done:
    pop bp
    ret
EVICT_MOVABLE_BLOCK ENDP
END
