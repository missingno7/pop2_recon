.MODEL MEDIUM
.DATA
EXTRN HEAP_STATE:DWORD
.CODE
EXTRN GET_BLOCK_PAYLOAD_SIZE:NEAR
EXTRN COALESCE_FREE_BLOCKS:NEAR
EXTRN EVICT_MOVABLE_BLOCK:NEAR
EXTRN RESIZE_DOS_HEAP:NEAR
PUBLIC FIND_MOVABLE_PAYLOAD_SPACE

FIND_MOVABLE_PAYLOAD_SPACE PROC NEAR
    push bp
    mov bp, sp
    add sp, -12
    push di
    mov [bp-12], ax
    mov [bp-10], dx
    add ax, 7
    adc dx, 0
    jc size_invalid
    and al, 0feh
    cmp dx, 0fh
    jbe normalize_size
size_invalid:
    jmp NEAR PTR no_space
normalize_size:
    mov bx, ax
    and bx, 0fh
    mov cl, 4
    shr ax, cl
    ror dx, cl
    add ax, dx
    xchg ax, cx
    mov [bp-4], bx
    mov [bp-2], cx
    les ax, DWORD PTR [si]
    and ax, 7
    shl ax, 1
    mov dx, es
    cmp dx, cx
    ja split_payload
    jb examine_neighbors
    cmp ax, bx
    jb examine_neighbors
    je found
split_payload:
    les bx, DWORD PTR [bp-4]
    mov cx, es
    sub ax, bx
    jnc subtract_paragraphs
    and ax, 0fh
    dec dx
subtract_paragraphs:
    sub dx, cx
    ja write_remainder
    cmp ax, 6
    jae write_remainder
    add ax, bx
    and ax, 0fh
    jmp SHORT update_header
write_remainder:
    mov di, ds
    add di, cx
    mov es, di
    shr ax, 1
    mov es:[bx+si], ax
    mov es:[bx+si+2], dx
    xchg ax, bx
    mov [si+2], cx
update_header:
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
    and WORD PTR [si], 0ffc0h
    or [si], ax
found:
    stc
save_position:
    les di, DWORD PTR ss:[HEAP_STATE]
    mov es:[di+4], si
    mov es:[di+6], ds
    les ax, DWORD PTR [bp-12]
    mov dx, es
    pop di
    mov sp, bp
    pop bp
    ret
examine_neighbors:
    push ax
    push dx
    call GET_BLOCK_PAYLOAD_SIZE
    mov [bp-8], ax
    mov [bp-6], dx
    pop dx
    pop ax
merge_adjacent:
    call COALESCE_FREE_BLOCKS
    rcr bx, 1
    cmp dx, [bp-2]
    ja merge_success
    jb try_evict
    cmp ax, [bp-4]
    jb try_evict
merge_success:
    jmp NEAR PTR split_payload
try_evict:
    shl bx, 1
    jo check_heap_end
    push WORD PTR [si]
    and WORD PTR [si], 3fffh
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
    pop ax
    pop dx
    pop si
    pop ds
    pop WORD PTR [si]
    jc merge_adjacent
retry_original_size:
    les ax, DWORD PTR [bp-8]
    mov dx, es
    call FIND_MOVABLE_PAYLOAD_SPACE
no_space:
    clc
    jmp SHORT save_position
check_heap_end:
    jc retry_original_size
    push ax
    push dx
    les ax, DWORD PTR [bp-12]
    push es
    push ax
    push cs
    call RESIZE_DOS_HEAP
    or ax, dx
    pop dx
    pop ax
    je retry_original_size
    jmp SHORT merge_adjacent
FIND_MOVABLE_PAYLOAD_SPACE ENDP
END
