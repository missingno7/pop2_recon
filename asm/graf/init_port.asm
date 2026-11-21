.MODEL MEDIUM
.DATA
EXTRN DEFAULT_PORT:BYTE
.CODE
EXTRN FINISH_PORT:NEAR
PUBLIC init_port

init_port PROC FAR
    push bp
    mov bp, sp
    push di
    push si

    ; Inherit the current default port, then change its coordinate bounds.
    mov bx, WORD PTR [bp+10]
    mov cx, 2ch
    mov di, bx
    mov si, OFFSET DEFAULT_PORT
    push ds
    pop es
    rep movsb
    les di, DWORD PTR [bp+6]
    mov ax, es:[di+6]
    mov cx, es:[di+2]
    sub ax, cx
    mov [bx+16h], ax
    mov ax, es:[di+4]
    mov dx, es:[di]
    sub ax, dx
    mov [bx+14h], ax
    sub [bx+6], cx
    sub [bx+4], dx
    sub [bx+0ah], cx
    sub [bx+8], dx

    ; Preserve the port pointer for the return value across the helper.
    push bx
    push bx
    push cs
    call FINISH_PORT
    pop ax
    pop si
    pop di
    pop bp
    retf 6
init_port ENDP
END
