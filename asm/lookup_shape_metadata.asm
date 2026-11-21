.MODEL MEDIUM
.CODE
PUBLIC LOOKUP_SHAPE_METADATA
EXTRN FIND_SHAPE_METADATA:NEAR

LOOKUP_SHAPE_METADATA PROC FAR
    push bp
    mov bp, sp
    push es
    push ds
    push di
    push si
    pushf
    cld
    mov ax, WORD PTR [bp+6]
    call FIND_SHAPE_METADATA
    test ax, ax
    je no_shape
    mov si, di
    les di, DWORD PTR [bp+8]
    movsw
    mov si, bx
    mov cx, 2
    rep movsw
    mov dx, es
    mov ax, WORD PTR [bp+8]
    jmp short done
no_shape:
    sub dx, dx
done:
    popf
    pop si
    pop di
    pop ds
    pop es
    pop bp
    retf 6
LOOKUP_SHAPE_METADATA ENDP
END
