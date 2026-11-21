.MODEL MEDIUM
.CODE
PUBLIC CLAMP_POINT_TO_RECT

CLAMP_POINT_TO_RECT PROC FAR
    push bp
    mov bp, sp
    push di
    push ds
    lds bx, DWORD PTR [bp+6]
    les di, DWORD PTR [bp+0ah]
    mov cx, es:[di+2]
    mov ax, es:[di]
    cmp cx, [bx+2]
    jge test_right_edge
    mov cx, [bx+2]
test_right_edge:
    cmp cx, [bx+6]
    jl test_top_edge
    mov cx, [bx+6]
    dec cx
test_top_edge:
    cmp ax, [bx]
    jge test_bottom_edge
    mov ax, [bx]
test_bottom_edge:
    cmp ax, [bx+4]
    jl write_clamped_point
    mov ax, [bx+4]
    dec ax
write_clamped_point:
    les di, DWORD PTR [bp+0eh]
    stosw
    xchg cx, ax
    stosw
    mov ax, [bp+0eh]
    mov dx, es
    pop ds
    pop di
    pop bp
    retf 12
CLAMP_POINT_TO_RECT ENDP
END
