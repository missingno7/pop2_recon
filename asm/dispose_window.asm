.MODEL MEDIUM
.CODE
EXTRN CLOSE_WINDOW:NEAR
EXTRN FREE_NEAR:FAR
PUBLIC DISPOSE_WINDOW

DISPOSE_WINDOW PROC FAR
    push bp
    mov bp, sp
    push WORD PTR [bp+6]
    push cs
    call CLOSE_WINDOW
    push WORD PTR [bp+6]
    call FREE_NEAR
    add sp, 2
    pop bp
    retf 2
DISPOSE_WINDOW ENDP
END
