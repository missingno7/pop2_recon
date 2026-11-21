.MODEL MEDIUM
.CODE
PUBLIC FLIP_ROWS

FLIP_ROWS PROC FAR
    push bp
    mov bp, sp
    add sp, -324
    push ds
    push si
    push di
    lds si, DWORD PTR [bp+10]
    mov [bp-322], si
    mov [bp-324], si
    mov bx, [bp+6]
    mov cx, [bp+8]
    mov ax, cx
    dec ax
    mul bx
    add [bp-324], ax
    shr cx, 1
swap_rows:
    push cx
    ; Save the first row in the stack buffer.
    mov si, [bp-322]
    mov ax, ss
    mov es, ax
    lea di, [bp-320]
    mov cx, bx
    rep movsb
    ; Move the last row over the first.
    mov si, [bp-324]
    push ds
    pop es
    mov di, [bp-322]
    mov cx, bx
    rep movsb
    ; Restore the saved row into the last row's old position.
    push ds
    mov ax, ss
    mov ds, ax
    lea si, [bp-320]
    pop es
    mov di, [bp-324]
    mov cx, bx
    rep movsb
    push es
    pop ds
    add [bp-322], bx
    sub [bp-324], bx
    pop cx
    loop swap_rows
    pop di
    pop si
    pop ds
    mov sp, bp
    pop bp
    retf 8
FLIP_ROWS ENDP
END
