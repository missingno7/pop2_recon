.MODEL MEDIUM
.DATA
EXTRN CURRENT_CONTEXT:WORD
AXIS_SEGMENT_CALLBACK DW 0
.CODE
EXTRN STROKE_RECTANGLE:NEAR
PUBLIC DRAW_AXIS_SEGMENT

DRAW_AXIS_SEGMENT PROC FAR
    push bp
    mov bp, sp
    mov bx, CURRENT_CONTEXT
    mov ax, [bx+22h]
    cmp ax, [bp+6]
    je shared_origin
    pop bp
    jmp WORD PTR AXIS_SEGMENT_CALLBACK
shared_origin:
    add sp, -8
    mov [bp-8], ax
    inc ax
    mov [bp-4], ax
    mov ax, [bp+8]
    mov dx, ax
    xchg ax, [bx+24h]
    cmp ax, dx
    jle ordered
    xchg ax, dx
ordered:
    mov [bp-6], ax
    inc dx
    mov [bp-2], dx
    lea ax, [bp-8]
    push ds
    push ax
    push WORD PTR [bx+26h]
    push WORD PTR [bx+28h]
    push cs
    call STROKE_RECTANGLE
    mov sp, bp
    pop bp
    retf 4
DRAW_AXIS_SEGMENT ENDP
END
