.MODEL MEDIUM
.CODE
EXTRN MATERIALIZE_HANDLE:NEAR
PUBLIC HLOCK, MARK_HANDLE_CONVERTED

HLOCK PROC FAR
    push bp
    mov bp, sp
    mov bp, [bp+6]
    push bp
    push cs
    call MATERIALIZE_HANDLE
    mov cx, ax
    or cx, dx
    jz resolved
    or WORD PTR [bp], 81f0h
resolved:
    pop bp
    retf 2
HLOCK ENDP

MARK_HANDLE_CONVERTED PROC FAR
    push bp
    mov bp, sp
    mov bp, [bp+6]
    test bp, bp
    jz done
    or WORD PTR [bp], 2000h
done:
    xchg ax, bp
    pop bp
    retf 2
MARK_HANDLE_CONVERTED ENDP
END
