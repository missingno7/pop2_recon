.MODEL MEDIUM
.CODE
EXTRN POINT_IN_RECTANGLE:NEAR
PUBLIC FIND_ITEM_AT_POINT

FIND_ITEM_AT_POINT PROC FAR
    push bp
    mov bp, sp
    push di
    mov di, [bp+8]
    add di, 50h
search_next_item:
    mov di, [di]
    test di, di
    jz search_done
    mov cx, [di+1ah]
    jcxz search_next_item
    mov cx, [di+1ch]
    jcxz search_next_item
    push ds
    push WORD PTR [bp+6]
    lea ax, [di+0ch]
    push ds
    push ax
    push cs
    call POINT_IN_RECTANGLE
    xchg cx, ax
    jcxz search_next_item
search_done:
    xchg di, ax
    pop di
    pop bp
    retf 4
FIND_ITEM_AT_POINT ENDP
END
