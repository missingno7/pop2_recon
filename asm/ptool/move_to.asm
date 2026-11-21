.MODEL MEDIUM
.DATA
EXTRN CURRENT_CONTEXT:WORD
.CODE
PUBLIC MOVETO

MOVETO PROC FAR
    push bp
    mov bp, sp
    mov bx, CURRENT_CONTEXT
    ; LES loads both coordinate words. No memory is accessed through ES.
    les ax, DWORD PTR [bp+6]
    mov [bx+22h], ax
    mov [bx+24h], es
    pop bp
    retf 4
MOVETO ENDP
END
