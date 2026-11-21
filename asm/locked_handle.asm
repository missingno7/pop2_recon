.MODEL MEDIUM
.CODE
EXTRN MATERIALIZE_HANDLE:NEAR
PUBLIC FETCH_LOCKED_SHAPE_LIST, MARK_HANDLE_CONVERTED

FETCH_LOCKED_SHAPE_LIST PROC FAR
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
FETCH_LOCKED_SHAPE_LIST ENDP

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
