.MODEL MEDIUM
.DATA
EXTRN CURRENT_CONTEXT:WORD
.CODE
EXTRN STROKE_OFFSET_EDGES:NEAR
PUBLIC DISPATCH_ITEM_CHARACTER, STROKE_ITEM_EDGES

DISPATCH_ITEM_CHARACTER PROC FAR
    push bp
    mov bp, sp
    mov bx, [bp+8]
    mov cx, [bx+1ah]
    jcxz character_done
    mov cx, [bx+1ch]
    jcxz character_done
    push bx
    mov ax, 6
    push ax
    push WORD PTR [bp+6]
    call DWORD PTR [bx+4]
    xchg cx, ax
character_done:
    xchg cx, ax
    pop bp
    retf 4
DISPATCH_ITEM_CHARACTER ENDP

STROKE_ITEM_EDGES PROC NEAR
    mov ax, [bx+2]
    xchg CURRENT_CONTEXT, ax
    push ax
    add bx, 0ch
    push bx
    mov ax, 2
    push ax
    mov ax, 1
    push ax
    mov ax, 3
    push ax
    mov ax, 7
    push ax
    push cs
    call STROKE_OFFSET_EDGES
    pop CURRENT_CONTEXT
    ret
STROKE_ITEM_EDGES ENDP
END
