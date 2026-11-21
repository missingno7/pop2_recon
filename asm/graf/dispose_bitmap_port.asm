.MODEL MEDIUM
.DATA
EXTRN CURRENT_CONTEXT:WORD
.CODE
EXTRN BLIT_RECT:NEAR
EXTRN RELEASE_BUFFER:NEAR
EXTRN FREE_PORT_DESCRIPTOR:FAR
PUBLIC DISPOSE_BITMAP_PORT

DISPOSE_BITMAP_PORT PROC FAR
    push bp
    mov bp, sp
    mov bx, [bp+6]
    push bx
    les ax, DWORD PTR [bx]
    push es
    push ax
    push CURRENT_CONTEXT
    push bx
    add bx, 10h
    push ds
    push bx
    push ds
    push bx
    mov ax, 0
    push ax
    push cs
    call BLIT_RECT
    push cs
    call RELEASE_BUFFER
    call FREE_PORT_DESCRIPTOR
    add sp, 2
    pop bp
    retf 2
DISPOSE_BITMAP_PORT ENDP
END
