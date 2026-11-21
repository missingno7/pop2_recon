.MODEL MEDIUM
.DATA
EXTRN CURRENT_CONTEXT:WORD
.CODE
PUBLIC COPY_CURRENT_PORT_ORIGIN

COPY_CURRENT_PORT_ORIGIN PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    mov cx, 8
    mov si, CURRENT_CONTEXT
    add si, 22h
    les di, DWORD PTR [bp+6]
    rep movsb
    pop si
    pop di
    pop bp
    retf 4
COPY_CURRENT_PORT_ORIGIN ENDP
END
