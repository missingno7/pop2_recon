.MODEL MEDIUM
.CODE
PUBLIC BANKED_POINTER_CAPACITY

BANKED_POINTER_CAPACITY PROC FAR
    push bp
    mov bp, sp
    push si
    push ds
    push es
    lds si, DWORD PTR [bp+6]
    les ax, DWORD PTR [si]
    mov dx, es
    and ax, 7
    shl ax, 1
    shl dx, 1
    shl dx, 1
    shl dx, 1
    shl dx, 1
    add ax, dx
    sub ax, 6
    pop es
    pop ds
    pop si
    pop bp
    retf 4
BANKED_POINTER_CAPACITY ENDP
END
