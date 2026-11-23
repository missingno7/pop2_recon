.MODEL MEDIUM
.DATA
EXTRN Graf:WORD
.CODE
PUBLIC FINISH_PORT
EXTRN SECTRECT:NEAR
EXTRN OFFSETRECT:NEAR

FINISH_PORT PROC FAR
    push bp
    mov bp, sp
    mov ax, WORD PTR [bp+6]
    mov bx, ax
    xchg Graf, ax
    push ax

    ; Intersect the selected port rectangles, then translate to its origin.
    lea ax, [bx+18h]
    lea dx, [bx+10h]
    add bx, 4
    push ds
    push ax
    push ds
    push ax
    mov cx, WORD PTR [bx+2]
    neg cx
    push cx
    mov cx, WORD PTR [bx]
    neg cx
    push cx
    push ds
    push ax
    push ds
    push bx
    push ds
    push dx
    push cs
    call SECTRECT
    push cs
    call OFFSETRECT

    pop Graf
    pop bp
    retf 2
FINISH_PORT ENDP
END
