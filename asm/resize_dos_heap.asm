.MODEL MEDIUM
.DATA
EXTRN heap_descriptor:DWORD
.CODE
PUBLIC RESIZE_DOS_HEAP
EXTRN ROUND_BYTES_TO_PARAGRAPHS:NEAR

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
    lds di, DWORD PTR ss:heap_descriptor
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
    lds di, DWORD PTR ss:heap_descriptor
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
    lds di, DWORD PTR ss:heap_descriptor
    jmp short add_to_heap_total
RESIZE_DOS_HEAP ENDP
END
