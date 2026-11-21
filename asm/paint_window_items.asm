.MODEL MEDIUM
.CODE
EXTRN UPDATE_MESSAGE_DISPLAY:NEAR
EXTRN DRAW_ITEM_FOCUS:NEAR
EXTRN SHOW_MOUSE_GRAPHICS:NEAR
PUBLIC PAINT_WINDOW_ITEMS

PAINT_WINDOW_ITEMS PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    push cs
    call UPDATE_MESSAGE_DISPLAY
    mov di, [bp+6]
    lea si, [di+50h]
item_next:
    mov si, [si]
    test si, si
    je paint_focus
    mov cx, [si+1ch]
    jcxz item_next
    push si
    mov ax, 2
    push ax
    sub ax, ax
    push ax
    call DWORD PTR [si+4]
    jmp short item_next
paint_focus:
    mov bx, [di+52h]
    test bx, bx
    je show_cursor
    call DRAW_ITEM_FOCUS
show_cursor:
    push cs
    call SHOW_MOUSE_GRAPHICS
    pop si
    pop di
    pop bp
    retf 2
PAINT_WINDOW_ITEMS ENDP
END
