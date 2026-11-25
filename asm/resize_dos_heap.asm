.MODEL MEDIUM
.DATA
PUBLIC HEAP_STATE
HEAP_STATE DD 0
.CODE
PUBLIC RESIZE_DOS_HEAP, HEAPINIT
EXTRN ROUND_BYTES_TO_PARAGRAPHS:NEAR

PUBLIC HEAPCOMPACT
EXTRN COALESCE_FREE_BLOCKS:NEAR
EXTRN MOVE_HUGE_BYTES:NEAR

target_offset EQU WORD PTR [bp-8]
target_paragraphs EQU WORD PTR [bp-6]
largest_offset EQU WORD PTR [bp-4]
largest_paragraphs EQU WORD PTR [bp-2]

HEAPCOMPACT PROC FAR
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
    jc clamp_target
    and al, 0feh
    cmp dx, 0fh
    jbe normalize_target
clamp_target:
    mov ax, 0ffffh
    mov dx, 0fh
normalize_target:
    mov bx, ax
    and ax, 0fh
    mov cl, 4
    shr bx, cl
    ror dx, cl
    add dx, bx
    mov target_offset, ax
    mov target_paragraphs, dx
    mov largest_offset, 6
    mov largest_paragraphs, 0
    lds si, DWORD PTR ss:HEAP_STATE
    lds si, DWORD PTR [si]
    mov di, ds
inspect_block:
    les ax, DWORD PTR [si]
    mov dx, es
    and ax, 0c007h
    shl ax, 1
    jnc inspect_free_block
    and ax, 0fh
advance_block:
    add si, ax
    mov ax, si
    and si, 0fh
    REPT 4
        shr ax, 1
    ENDM
    add di, ax
    add di, dx
    mov ds, di
    jmp SHORT inspect_block
report_largest:
    les di, DWORD PTR ss:HEAP_STATE
    mov es:[di+4], si
    mov es:[di+6], ds
    les ax, DWORD PTR [bp-4]
    mov dx, es
    REPT 4
        rol dx, 1
    ENDM
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
inspect_free_block:
    js report_largest
merge_free_blocks:
    call COALESCE_FREE_BLOCKS
    rcr bx, 1
    cmp dx, largest_paragraphs
    ja record_largest
    jb test_adjacent_block
    cmp ax, largest_offset
    jbe test_adjacent_block
record_largest:
    mov largest_offset, ax
    mov largest_paragraphs, dx
    cmp dx, target_paragraphs
    ja report_largest
    jb test_adjacent_block
    cmp ax, target_offset
    jae report_largest
test_adjacent_block:
    shl bx, 1
    jnc report_largest
    jns skip_adjacent_block
    mov di, es
    xchg cx, di
    mov es, di
    xchg bx, ax
    mov di, es:[bx+si+4]
    test WORD PTR ss:[di], 8000h
    jz move_adjacent_block
    xchg bx, ax
skip_adjacent_block:
    mov di, ds
    jmp advance_block
move_adjacent_block:
    and ax, 0fh
    push dx
    push bx
    push cx
    push ax
    push ds
    push si
    add bx, si
    push es
    push bx
    REPT 4
        rol cx, 1
    ENDM
    mov dx, cx
    and dx, 0fff0h
    and cx, 0fh
    add ax, dx
    push cx
    push ax
    push cs
    call MOVE_HUGE_BYTES
    pop si
    pop bx
    add si, ax
    add bx, dx
    and WORD PTR ss:[di], 0fff0h
    or ss:[di], ax
    mov ss:[di+2], dx
    mov ax, si
    and si, 0fh
    REPT 4
        shr ax, 1
    ENDM
    add bx, ax
    mov ds, bx
    pop ax
    pop dx
    shr ax, 1
    mov [si], ax
    mov [si+2], dx
    shl ax, 1
    jmp merge_free_blocks
HEAPCOMPACT ENDP

grown     EQU WORD PTR [bp-2]
requested EQU WORD PTR [bp-4]

RESIZE_DOS_HEAP PROC FAR
    push bp
    mov bp, sp
    add sp, -4
    push di
    push si
    push ds
    sub ax, ax
    mov grown, ax
    lds di, DWORD PTR ss:HEAP_STATE
    cmp ax, [di+10h]
    jne add_to_heap_total
    les ax, DWORD PTR [bp+6]
    mov dx, es
    call ROUND_BYTES_TO_PARAGRAPHS
    mov requested, ax
    add ax, [di+0ah]
    jae resize_block
    mov ax, 0ffffh
resize_block:
    push ds
    pop es
    xchg ax, bx
    mov ah, 4ah
    int 21h
    jae count_growth
    mov ah, 4ah
    int 21h
count_growth:
    mov dx, es
    mov ds, dx
    mov ax, [di+0ah]
    sub bx, ax
    je expand_ems
    add grown, bx
    add [di+0ah], bx
    add dx, ax
    dec dx
    mov ds, dx
    mov di, 0ah
    sub ax, ax
    mov [di], ax
    mov [di+2], bx
    add dx, bx
    mov ds, dx
    mov WORD PTR [di], 4003h
    mov [di+2], ax
    lds di, DWORD PTR ss:HEAP_STATE
    mov ax, requested
    cmp ax, grown
    ja expand_ems
add_to_heap_total:
    mov ax, grown
    add [di+8], ax
    mov cl, 4
    rol ax, cl
    mov dx, ax
    and al, 0f0h
    and dx, 0fh
    pop ds
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 4

expand_ems:
    inc WORD PTR [di+10h]
    mov dx, [di+0ch]
    test dx, dx
    je add_to_heap_total
    mov ah, 4ch
    int 67h
    mov cl, 10
    mov si, bx
    shl si, cl
    add grown, si
    mov cx, bx
    sub bx, bx
map_page:
    mov al, bl
    mov ah, 44h
    int 67h
    inc bx
    loop map_page
    mov ah, 41h
    int 67h
    mov dx, ds
    add dx, [di+0ah]
    sub bx, dx
    dec dx
    mov ds, dx
    mov di, 0ah
    mov WORD PTR [di], 0a003h
    mov [di+2], bx
    sub di, di
    inc dx
    add dx, bx
    mov ds, dx
    mov ax, 5
    add si, -1
    mov [di], ax
    mov [di+2], si
    shl ax, 1
    add di, ax
    add dx, si
    mov ds, dx
    mov WORD PTR [di], 4003h
    mov WORD PTR [di+2], 0
    lds di, DWORD PTR ss:HEAP_STATE
    jmp short add_to_heap_total
RESIZE_DOS_HEAP ENDP

minimum_paragraphs EQU WORD PTR [bp-102h]

HEAPINIT PROC FAR
    push bp
    mov bp, sp
    add sp, -102h
    push di
    push si
    push ds
    sub ax, ax
    mov WORD PTR HEAP_STATE, ax
    mov WORD PTR HEAP_STATE+2, ax
    les ax, DWORD PTR [bp+6]
    mov dx, es
    call ROUND_BYTES_TO_PARAGRAPHS
    cmp ax, 3
    jae choose_heap_size
    mov ax, 3
choose_heap_size:
    mov minimum_paragraphs, ax
    xchg bx, ax
allocate_paragraphs:
    mov ah, 48h
    int 21h
    jnc check_heap_segment
    test bx, bx
    jnz try_available_paragraphs
    sub ax, ax
    jmp return_heap_size
try_available_paragraphs:
    mov ah, 48h
    int 21h
check_heap_segment:
    mov dx, ds
    cmp ax, dx
    ja initialize_descriptor
    mov es, ax
    mov ah, 49h
    int 21h
    inc bx
    jmp SHORT allocate_paragraphs
initialize_descriptor:
    mov WORD PTR HEAP_STATE+2, ax
    mov es, ax
    sub ax, ax
    mov cx, 11
    sub di, di
    rep stosw
    lds di, HEAP_STATE
    mov [di+8], bx
    mov [di+0ah], bx
    mov si, 6
    mov dx, ds
    add dx, 1
    mov [di+4], si
    mov [di], si
    mov [di+6], dx
    mov [di+2], dx
    mov ds, dx
    mov ax, 2
    add bx, -2
    mov [si], ax
    mov [si+2], bx
    shl ax, 1
    add si, ax
    add dx, bx
    mov ds, dx
    mov WORD PTR [si], 4003h
    mov WORD PTR [si+2], 0
    mov ax, 3567h
    int 21h
    mov cx, 8
    mov di, 0ah
    mov si, OFFSET ems_signature
    push cs
    pop ds
    repe cmpsb
    jne ensure_heap_size
    mov ah, 46h
    int 67h
    test ah, ah
    jne ensure_heap_size
    cmp al, 40h
    jb ensure_heap_size
    lea di, [bp-100h]
    push ss
    pop es
    mov ax, 5800h
    int 67h
    mov si, di
    push ss
    pop ds
look_for_mapping:
    lodsw
    xchg dx, ax
    lodsw
    test ax, ax
    loopne look_for_mapping
    inc cx
count_adjacent_pages:
    inc ax
    add dx, 400h
    cmp dx, [si]
    lea si, [si+4]
    loope count_adjacent_pages
    xchg bx, ax
    mov ah, 43h
    int 67h
    test ah, ah
    jne ensure_heap_size
    lds di, HEAP_STATE
    mov [di+0ch], dx
ensure_heap_size:
    lds di, DWORD PTR ss:[HEAP_STATE]
    mov ax, minimum_paragraphs
    sub ax, [di+8]
    jbe read_heap_size
    mov cl, 4
    rol ax, cl
    mov dx, ax
    and al, 0f0h
    and dx, 0fh
    push dx
    push ax
    push cs
    call NEAR PTR RESIZE_DOS_HEAP
read_heap_size:
    mov ax, [di+8]
return_heap_size:
    mov cl, 4
    rol ax, cl
    mov dx, ax
    and al, 0f0h
    and dx, 0fh
    pop ds
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 4

ems_signature DB 'EMSXXXX0'
HEAPINIT ENDP
END
