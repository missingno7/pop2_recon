.MODEL MEDIUM
.DATA
EXTRN CURRENT_CONTEXT:WORD
.CODE
EXTRN CREATE_WINDOW:NEAR
EXTRN LOAD_ITEM_LIST:NEAR
EXTRN SELECT_ITEM_BY_INDEX:NEAR
PUBLIC CREATE_WINDOW_FROM_DEFINITION

CREATE_WINDOW_FROM_DEFINITION PROC FAR
    push bp
    mov bp, sp
    add sp, -0eh
    push di
    push CURRENT_CONTEXT
    mov ax, -1
    push ax
    les bx, DWORD PTR [bp+6]
    lea ax, [bx]
    push es
    push ax
    sub ax, ax
    push ax
    push ax
    push ax
    push cs
    call CREATE_WINDOW
    xchg di, ax
    test di, di
    jz restore_window_port
    mov CURRENT_CONTEXT, di
    mov ax, [di+1ah]
    xchg [di+1eh], ax
    push ax
    push di
    les bx, DWORD PTR [bp+6]
    push WORD PTR es:[bx+8]
    push di
    lea ax, [bx+0ah]
    push es
    push ax
    push cs
    call LOAD_ITEM_LIST
    push cs
    call SELECT_ITEM_BY_INDEX
    pop WORD PTR [di+1eh]
restore_window_port:
    pop CURRENT_CONTEXT
    xchg di, ax
    pop di
    mov sp, bp
    pop bp
    retf 4
CREATE_WINDOW_FROM_DEFINITION ENDP
END
