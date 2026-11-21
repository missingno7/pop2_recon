.MODEL MEDIUM
.CODE
EXTRN SHOW_MOUSE_GRAPHICS:NEAR
PUBLIC RESTORE_WINDOW_CLIP

RESTORE_WINDOW_CLIP PROC FAR
    push cs
    call SHOW_MOUSE_GRAPHICS
    pop ax
    pop es
    pop bx
    add bx, 18h
    mov cx, 4
restore_next:
    pop WORD PTR [bx]
    add bx, 2
    loop restore_next
    push es
    push ax
    retf
RESTORE_WINDOW_CLIP ENDP
END
