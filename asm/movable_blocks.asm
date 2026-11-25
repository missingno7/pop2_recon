.MODEL MEDIUM
.DATA
EXTRN HEAP_STATE:DWORD
.CODE
EXTRN RESIZE_DOS_HEAP:NEAR
EXTRN HEAPPURGE:NEAR
EXTRN HEAPCOMPACT:NEAR
EXTRN MOVE_HUGE_BYTES:NEAR
EXTRN ALLOCATE_BACKING_RECORD:NEAR
EXTRN RELEASE_BACKING_RECORD:NEAR
EXTRN TRANSFER_BACKING_RECORD:NEAR
PUBLIC ROUND_BYTES_TO_PARAGRAPHS, ALLOCATE_BACKING_SPACE, ALLOCATE_BACKING_SPACE_GUARDED
PUBLIC ALLOCATE_MOVABLE_SLOT, GET_BLOCK_PAYLOAD_SIZE, COALESCE_FREE_BLOCKS
PUBLIC EVICT_MOVABLE_BLOCK, SWAP_OUT_HEAP_BLOCK

; DX:AX holds a byte count. Return the rounded, saturated paragraph count in AX.
ROUND_BYTES_TO_PARAGRAPHS PROC NEAR
    cmp dx, 0fh
    ja saturated
    mov bx, ax
    mov cl, 4
    shr ax, cl
    ror dx, cl
    test bl, 0fh
    je add_high_word
    inc ax
add_high_word:
    add ax, dx
    jae rounding_done
saturated:
    mov ax, 0ffffh
rounding_done:
    ret
ROUND_BYTES_TO_PARAGRAPHS ENDP

request_low EQU WORD PTR [bp-12]
request_high EQU WORD PTR [bp-10]
rounded_low EQU WORD PTR [bp-8]
rounded_high EQU WORD PTR [bp-6]
request_offset EQU WORD PTR [bp-4]
request_paragraphs EQU WORD PTR [bp-2]
remaining_lists EQU WORD PTR [bp-14]

ALLOCATE_BACKING_SPACE PROC NEAR
    push bp
    mov bp, sp
    add sp, -14
    push di
    mov request_low, ax
    mov request_high, dx
    add ax, 7
    adc dx, 0
    jc invalid_request
    and al, 0feh
    cmp dx, 0fh
    jbe normalize_request
invalid_request:
    jmp backing_unavailable
normalize_request:
    mov rounded_low, ax
    mov rounded_high, dx
    mov bx, ax
    and ax, 0fh
    mov cl, 4
    shr bx, cl
    ror dx, cl
    add dx, bx
    mov request_offset, ax
    mov request_paragraphs, dx
restart_backing_search:
    mov remaining_lists, 2
    lds si, DWORD PTR ss:[HEAP_STATE]
    lds si, DWORD PTR [si+4]
load_block_segment:
    mov di, ds
inspect_backing_block:
    les ax, DWORD PTR [si]
    mov dx, es
    and ax, 0c007h
    shl ax, 1
    jnc examine_backing_free
    and ax, 0fh
advance_backing_block:
    add si, ax
    mov ax, si
    and si, 0fh
    REPT 4
        shr ax, 1
    ENDM
    add di, ax
    add di, dx
    mov ds, di
    jmp SHORT inspect_backing_block
examine_backing_free:
    js next_backing_list
    call COALESCE_FREE_BLOCKS
    les bx, DWORD PTR [bp-4]
    mov cx, es
    cmp dx, cx
    ja fit_backing_block
    jb advance_backing_block
    cmp ax, bx
    jb advance_backing_block
fit_backing_block:
    sub ax, bx
    jnc subtract_request_paragraphs
    and ax, 0fh
    dec dx
subtract_request_paragraphs:
    sub dx, cx
    ja split_backing_block
    cmp ax, 6
    jae split_backing_block
    add ax, bx
    and ax, 0fh
    jmp SHORT record_backing_allocation
next_backing_list:
    dec remaining_lists
    jz compact_backing_heap
    lds di, DWORD PTR ss:[HEAP_STATE]
    lds si, DWORD PTR [di]
    jmp SHORT load_block_segment
split_backing_block:
    add di, cx
    mov es, di
    shr ax, 1
    mov es:[bx+si], ax
    mov es:[bx+si+2], dx
    xchg bx, ax
    mov [si+2], cx
record_backing_allocation:
    mov dx, ax
    shr ax, 1
    mov cx, request_low
    and cx, 0fh
    sub dx, cx
    sub dx, 6
    REPT 3
        shl dx, 1
    ENDM
    and dx, 38h
    or ax, dx
    mov [si], ax
    les di, DWORD PTR ss:[HEAP_STATE]
    mov es:[di+4], si
    mov es:[di+6], ds
    stc
finish_backing_request:
    les ax, DWORD PTR [bp-12]
    mov dx, es
    pop di
    mov sp, bp
    pop bp
    ret
grow_backing_heap:
    push si
    push di
    push cs
    call RESIZE_DOS_HEAP
    cmp dx, si
    ja retry_backing_search
    jb reclaim_backing_heap
    cmp ax, di
    jae retry_backing_search
reclaim_backing_heap:
    push si
    push di
    push cs
    call HEAPPURGE
    cmp dx, si
    ja retry_backing_search
    jb compact_backing_heap
    cmp ax, di
    jae retry_backing_search
compact_backing_heap:
    les di, DWORD PTR [bp-8]
    mov si, es
    push si
    push di
    push cs
    call HEAPCOMPACT
    cmp dx, si
    ja retry_backing_search
    jb check_backing_retry
    cmp ax, di
    jae retry_backing_search
check_backing_retry:
    mov cx, remaining_lists
    inc remaining_lists
    jcxz grow_backing_heap
    les di, DWORD PTR ss:[HEAP_STATE]
    mov ax, es:[di+12h]
    or ax, es:[di+14h]
    je backing_unavailable
    push ss
    pop ds
    call DWORD PTR es:[di+12h]
    xchg cx, ax
    jcxz backing_unavailable
retry_backing_search:
    jmp restart_backing_search
backing_unavailable:
    clc
    jmp SHORT finish_backing_request
ALLOCATE_BACKING_SPACE ENDP

ALLOCATE_BACKING_SPACE_GUARDED PROC NEAR
    les bx, DWORD PTR ss:[HEAP_STATE]
    inc WORD PTR es:[bx+0eh]
    call ALLOCATE_BACKING_SPACE
    les bx, DWORD PTR ss:[HEAP_STATE]
    dec WORD PTR es:[bx+0eh]
    ret
ALLOCATE_BACKING_SPACE_GUARDED ENDP

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

SWAP_OUT_HEAP_BLOCK PROC NEAR
    push bp
    mov bp, [si+4]
    test WORD PTR [bp], 0c000h
    jle swap_done
    call GET_BLOCK_PAYLOAD_SIZE
    push dx
    push ax
    push dx
    push ax
    push cs
    call ALLOCATE_BACKING_RECORD
    test dx, 0fe00h
    jz store_record_address
    push dx
    push ax
    push cs
    call RELEASE_BACKING_RECORD
    sub ax, ax
    cwd
store_record_address:
    push dx
    push ax
    mov cx, ax
    mov bx, 1ffh
    and ax, bx
    and dx, bx
    not bx
    and cx, bx
    or dx, cx
    mov cl, 7
    rol dx, cl
    mov [bp+2], dx
    and [bp], bx
    or [bp], ax
    or ax, dx
    pop ax
    pop dx
    pop bx
    pop cx
    jnz store_payload
    and WORD PTR [bp], 0dfffh
release_heap_block:
    and WORD PTR [si], 3fffh
    stc
swap_done:
    pop bp
    ret
store_payload:
    or WORD PTR [bp], 200h
    push dx
    push ax
    lea ax, [si+6]
    push ds
    push ax
    push cx
    push bx
    mov ax, 1
    push ax
    push cs
    call TRANSFER_BACKING_RECORD
    jmp SHORT release_heap_block
SWAP_OUT_HEAP_BLOCK ENDP
END
