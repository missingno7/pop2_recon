.MODEL MEDIUM
.CODE
PUBLIC POINT_IN_RECTANGLE

POINT_IN_RECTANGLE PROC FAR
    push bp
    mov bp, sp
    push ds
    lds bx, DWORD PTR [bp+10]
    mov cx, [bx+2]
    mov dx, [bx]
    lds bx, DWORD PTR [bp+6]
    sub ax, ax
    cmp cx, [bx+2]
    jl test_done
    cmp cx, [bx+6]
    jge test_done
    cmp dx, [bx]
    jl test_done
    cmp dx, [bx+4]
    jge test_done
    inc ax
test_done:
    pop ds
    pop bp
    retf 8
POINT_IN_RECTANGLE ENDP
END
