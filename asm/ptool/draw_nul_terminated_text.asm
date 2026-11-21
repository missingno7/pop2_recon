.MODEL MEDIUM
.CODE
EXTRN DRAW_COUNTED_TEXT:NEAR
PUBLIC DRAW_NUL_TERMINATED_TEXT

DRAW_NUL_TERMINATED_TEXT PROC FAR
    push bp
    mov bp, sp
    push di
    les di, DWORD PTR [bp+6]
    push es
    push di
    sub ax, ax
    mov cx, 0ffffh
    repne scasb
    not cx
    dec cx
    push cx
    push cs
    call DRAW_COUNTED_TEXT
    pop di
    pop bp
    retf 4
DRAW_NUL_TERMINATED_TEXT ENDP
END
