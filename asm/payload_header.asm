.MODEL MEDIUM
.CODE
EXTRN GET_BLOCK_PAYLOAD_SIZE:NEAR
PUBLIC CLEAR_PAYLOAD_HEADER, PGETSIZE

CLEAR_PAYLOAD_HEADER PROC FAR
    push bp
    mov bp, sp
    les bx, DWORD PTR [bp+6]
    mov ax, es
    or ax, bx
    jz clear_done
    and WORD PTR es:[bx-6], 3fffh
clear_done:
    pop bp
    retf 4
CLEAR_PAYLOAD_HEADER ENDP

PGETSIZE PROC FAR
    push bp
    mov bp, sp
    push si
    push ds
    lds si, DWORD PTR [bp+6]
    mov ax, ds
    or ax, si
    cwd
    jz size_done
    sub si, 6
    call GET_BLOCK_PAYLOAD_SIZE
size_done:
    pop ds
    pop si
    pop bp
    retf 4
PGETSIZE ENDP

END
