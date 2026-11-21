.MODEL MEDIUM
.CODE
EXTRN QUERY_BACKING_RECORD_SIZE:NEAR
EXTRN ALLOCATE_BACKING_SPACE_GUARDED:NEAR
EXTRN TRANSFER_BACKING_RECORD:NEAR
EXTRN RELEASE_BACKING_RECORD:NEAR
PUBLIC MATERIALIZE_HANDLE

MATERIALIZE_HANDLE PROC FAR
    push bp
    mov bp, sp
    push si
    push ds
    mov bp, [bp+6]
    test bp, bp
    jz empty
read_handle:
    les ax, DWORD PTR [bp]
    mov dx, es
    test ax, 200h
    jnz from_backing
    and ax, 0fh
    or dx, ax
    jz done
    or WORD PTR [bp], 01f0h
    add ax, 6
    mov dx, es
done:
    pop ds
    pop si
    pop bp
    retf 2
empty:
    sub ax, ax
    cwd
    jmp SHORT done
from_backing:
    and ax, 01ffh
    mov cl, 7
    ror dx, cl
    mov cx, dx
    and cx, 0fe00h
    or ax, cx
    and dx, 01ffh
    push dx
    push ax
    push dx
    push ax
    push cs
    call QUERY_BACKING_RECORD_SIZE
    call ALLOCATE_BACKING_SPACE_GUARDED
    pop bx
    pop cx
    jnc empty
    or WORD PTR [si], 0c000h
    mov [si+4], bp
    and WORD PTR [bp], 0fc00h
    or [bp], si
    mov [bp+2], ds
    push cx
    push bx
    add si, 6
    push ds
    push si
    push cx
    push bx
    push dx
    push ax
    mov ax, 0ffffh
    push ax
    push cs
    call TRANSFER_BACKING_RECORD
    push cs
    call RELEASE_BACKING_RECORD
    jmp SHORT read_handle
MATERIALIZE_HANDLE ENDP
END
