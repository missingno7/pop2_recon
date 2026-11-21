.MODEL MEDIUM
.CODE
PUBLIC NORMALIZEPTR

NORMALIZEPTR PROC FAR
    ; DS addresses the stack on entry. BX locates the far pointer.
    mov bx, sp
    mov ax, [bx+4]
    mov dx, ax
    and ax, 0fh
    mov cl, 4
    shr dx, cl
    add dx, [bx+6]
    retf 4
NORMALIZEPTR ENDP
END

