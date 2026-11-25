.MODEL MEDIUM
.DATA
EXTRN DEFAULT_PORT:BYTE
EXTRN Graf:WORD
.CODE
EXTRN FINISH_PORT:NEAR
EXTRN DRAW_AXIS_SEGMENT:FAR

PUBLIC InitPort

InitPort PROC FAR
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
InitPort ENDP
PUBLIC alignment_5029
alignment_5029 DB 0  ; Link-address alignment between routines.

PUBLIC INSETRECT

INSETRECT PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    push ds
    lds si, DWORD PTR [bp+10]
    les di, DWORD PTR [bp+14]
    push di

    ; Move the top and left edges inward and subtract from the opposite edges.
    lodsw
    add ax, WORD PTR [bp+6]
    stosw
    lodsw
    add ax, WORD PTR [bp+8]
    stosw
    lodsw
    sub ax, WORD PTR [bp+6]
    stosw
    lodsw
    sub ax, WORD PTR [bp+8]
    stosw

    pop ax
    mov dx, es
    pop ds
    pop si
    pop di
    pop bp
    retf 12
INSETRECT ENDP
PUBLIC alignment_5055
alignment_5055 DB 0  ; Link-address alignment between routines.

PUBLIC DRAW_RELATIVE_AXIS_SEGMENT

DRAW_RELATIVE_AXIS_SEGMENT PROC FAR
    push bp
    mov bp, sp
    mov bx, Graf
    mov ax, [bx+24h]
    add [bp+8], ax
    mov ax, [bx+22h]
    add [bp+6], ax
    pop bp
    jmp DRAW_AXIS_SEGMENT
DRAW_RELATIVE_AXIS_SEGMENT ENDP
END
