.MODEL MEDIUM
.CODE
EXTRN GET_BLOCK_PAYLOAD_SIZE:NEAR
EXTRN ALLOCATE_BACKING_RECORD:NEAR
EXTRN RELEASE_BACKING_RECORD:NEAR
EXTRN TRANSFER_BACKING_RECORD:NEAR
PUBLIC SWAP_OUT_HEAP_BLOCK

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
