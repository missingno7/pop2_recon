.MODEL MEDIUM
.CODE
EXTRN STROKE_RECTANGLE:NEAR
PUBLIC SET_RECORD_RANGE

SET_RECORD_RANGE PROC FAR
    push bp
    mov bp, sp
    les ax, DWORD PTR [bp+8]
    push es
    push ax
    mov ax, 0
    push ax
    push WORD PTR [bp+6]
    push cs
    call STROKE_RECTANGLE
    pop bp
    retf 6
SET_RECORD_RANGE ENDP
END
