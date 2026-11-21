.MODEL MEDIUM
.CODE
EXTRN ALLOCATE_TAGGED_HANDLE:NEAR
EXTRN GET_HANDLE_FLAGS:NEAR
EXTRN GET_LOCKED_PAYLOAD:NEAR
EXTRN CREATE_WINDOW_FROM_DEFINITION:NEAR
EXTRN SET_HANDLE_FLAGS:NEAR
PUBLIC CREATE_WINDOW_FROM_RESOURCE

CREATE_WINDOW_FROM_RESOURCE PROC FAR
    push bp
    mov bp, sp
    push di
    mov ax, 444ch
    push ax
    mov ax, 4f47h
    push ax
    push WORD PTR [bp+6]
    push cs
    call ALLOCATE_TAGGED_HANDLE
    test ax, ax
    jz done
    xchg di, ax
    push di
    push cs
    call GET_HANDLE_FLAGS
    push di
    push ax
    push di
    push cs
    call GET_LOCKED_PAYLOAD
    push dx
    push ax
    push cs
    call CREATE_WINDOW_FROM_DEFINITION
    xchg di, ax
    push cs
    call SET_HANDLE_FLAGS
    xchg di, ax
done:
    pop di
    pop bp
    retf 2
CREATE_WINDOW_FROM_RESOURCE ENDP
END
