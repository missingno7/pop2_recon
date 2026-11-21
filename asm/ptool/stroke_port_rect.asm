.MODEL MEDIUM
.DATA
EXTRN CURRENT_CONTEXT:WORD
.CODE
EXTRN STROKE_RECTANGLE:NEAR
PUBLIC STROKE_PORT_RECT

STROKE_PORT_RECT PROC FAR
    push bp
    mov bp, sp
    les ax, DWORD PTR [bp+6]
    push es
    push ax
    mov ax, 0
    push ax
    mov bx, CURRENT_CONTEXT
    push WORD PTR [bx+20h]
    push cs
    call STROKE_RECTANGLE
    pop bp
    retf 4
STROKE_PORT_RECT ENDP
END
