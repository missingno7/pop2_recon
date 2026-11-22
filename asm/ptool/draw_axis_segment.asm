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
    je shared_origin_5070
    pop bp
    jmp WORD PTR AXIS_SEGMENT_CALLBACK
shared_origin_5070:
    add sp, -8
    mov [bp-8], ax
    inc ax
    mov [bp-4], ax
    mov ax, [bp+8]
    mov dx, ax
    xchg ax, [bx+24h]
    cmp ax, dx
    jle ordered_5070
    xchg ax, dx
ordered_5070:
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
PUBLIC alignment_50b7
alignment_50b7 DB 0  ; Link-address alignment between routines.

PUBLIC SUBTRACT_PORT_ORIGIN

SUBTRACT_PORT_ORIGIN PROC FAR
    push bp
    mov bp, sp
    push di
    mov bx, CURRENT_CONTEXT
    les di, DWORD PTR [bp+6]
    les ax, DWORD PTR es:[di]
    sub ax, [bx+4]
    mov cx, es
    sub cx, [bx+6]
    les di, DWORD PTR [bp+10]
    stosw
    xchg ax, cx
    stosw
    mov ax, [bp+10]
    mov dx, es
    pop di
    pop bp
    retf 8
SUBTRACT_PORT_ORIGIN ENDP
END
