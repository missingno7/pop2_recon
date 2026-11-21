.MODEL MEDIUM
.CODE
PUBLIC NORMALIZE_BANKED_POINTER

NORMALIZE_BANKED_POINTER PROC FAR
    push bp
    mov bp, sp
    push es
    push ds
    push si
    lds si, DWORD PTR [bp+6]
    les ax, DWORD PTR [si]
    mov bx, es
    ; The low three bits select one of eight word offsets.
    and ax, 7
    shl ax, 1
    add si, ax
    mov ax, si
    and ax, 0fh
    shr si, 1
    shr si, 1
    shr si, 1
    shr si, 1
    mov dx, ds
    add dx, bx
    add dx, si
    pop si
    pop ds
    pop es
    pop bp
    retf 4
NORMALIZE_BANKED_POINTER ENDP
END
