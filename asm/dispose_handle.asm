.MODEL MEDIUM
.CODE
EXTRN RELEASE_BACKING_RECORD:NEAR
EXTRN UNLINK_SHAPE_METADATA:NEAR
EXTRN FREE_NEAR_HEAP:FAR
PUBLIC RELEASE_HANDLE_STORAGE, DISPOSE_HANDLE

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
    call FREE_NEAR_HEAP
    add sp, 2
dispose_done:
    pop ds
    pop bp
    retf 2
DISPOSE_HANDLE ENDP
END
