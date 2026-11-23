.MODEL MEDIUM
.DATA
EXTRN HEAP_STATE:DWORD
.CODE
EXTRN RELEASE_BACKING_RECORD:NEAR
EXTRN UNLINK_SHAPE_METADATA:NEAR
EXTRN __nfree:FAR
PUBLIC RELEASE_HANDLE_STORAGE, DISPOSE_HANDLE

PUBLIC CHECK_HEAP_CHAIN
CHECK_HEAP_CHAIN PROC FAR
    push di
    push si
    push ds
    lds si, DWORD PTR ss:[HEAP_STATE]
    lds si, DWORD PTR [si]
    mov di, ds
inspect_block:
    les ax, DWORD PTR [si]
    mov dx, es
    and ax, 0c007h
    shl ax, 1
    jnc check_last_block
    and ax, 0fh
advance_block:
    add si, ax
    or ax, dx
    jz chain_bad
    mov ax, si
    and si, 0fh
    REPT 4
        shr ax, 1
    ENDM
    add di, ax
    jc chain_bad
    add di, dx
    jc chain_bad
    mov ds, di
    jmp SHORT inspect_block
check_last_block:
    jns advance_block
    sub ax, ax
chain_done:
    pop ds
    pop si
    pop di
    retf
chain_bad:
    mov ax, 1
    jmp SHORT chain_done
CHECK_HEAP_CHAIN ENDP

RELEASE_HANDLE_STORAGE PROC FAR
    push bp
    mov bp, sp
    push ds
    sub ax, ax
    mov bp, [bp+6]
    test bp, bp
    jz release_status
    mov bx, [bp]
    test bx, 8000h
    jnz release_status
    and WORD PTR [bp], 0dc00h
    xchg ax, [bp+2]
    test bx, 200h
    jnz release_backing
    mov es, ax
    and bx, 0fh
    or ax, bx
    jz released
    and WORD PTR es:[bx], 3fffh
released:
    stc
release_status:
    cmc
    sbb ax, ax
    neg ax
    pop ds
    pop bp
    retf 2
release_backing:
    and bx, 01ffh
    mov cl, 7
    ror ax, cl
    mov cx, ax
    and cx, 0fe00h
    or bx, cx
    and ax, 01ffh
    push ax
    push bx
    push cs
    call RELEASE_BACKING_RECORD
    jmp SHORT released
RELEASE_HANDLE_STORAGE ENDP

DISPOSE_HANDLE PROC FAR
    push bp
    mov bp, sp
    push ds
    mov bx, [bp+6]
    test bx, bx
    jz dispose_done
    push ss
    pop ds
    push bx
    push bx
    and WORD PTR [bx], 7fffh
    test WORD PTR [bx], 1000h
    jz dispose_storage
    push bx
    push cs
    call UNLINK_SHAPE_METADATA
dispose_storage:
    push cs
    call NEAR PTR RELEASE_HANDLE_STORAGE
    call __nfree
    add sp, 2
dispose_done:
    pop ds
    pop bp
    retf 2
DISPOSE_HANDLE ENDP
END
