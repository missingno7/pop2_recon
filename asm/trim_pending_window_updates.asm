.MODEL MEDIUM
.CODE
EXTRN SUBTRACT_RECTANGLE_STRIP:NEAR
EXTRN EMPTYRECT:NEAR
PUBLIC TRIM_PENDING_WINDOW_UPDATES

TRIM_PENDING_WINDOW_UPDATES PROC FAR
    push bp
    mov bp, sp
    push di
    lea di, [bp-26h]
next_window:
    mov di, [di+2ch]
    test di, di
    jz update_done
    mov cx, [di+46h]
    jcxz next_window
    lea ax, [di+48h]
    push ds
    push ax
    push ds
    push ax
    push ds
    push WORD PTR [bp+8]
    push cs
    call SUBTRACT_RECTANGLE_STRIP
    push ds
    push ax
    push cs
    call EMPTYRECT
    neg ax
    inc ax
    mov [di+46h], ax
    jmp SHORT next_window
update_done:
    pop di
    pop bp
    retf 4
TRIM_PENDING_WINDOW_UPDATES ENDP
END
