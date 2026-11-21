.MODEL MEDIUM
.CODE
PUBLIC SECTRECT

SECTRECT PROC FAR
    push bp
    mov bp, sp
    push di
    push ds
    les bx, DWORD PTR [bp+10]
    lds di, DWORD PTR [bp+6]

    ; Intersect left/right edges first. An empty intersection clears the output.
    mov ax, es:[bx+2]
    cmp ax, [di+2]
    jge have_left
    mov ax, [di+2]
have_left:
    mov dx, es:[bx+6]
    cmp dx, [di+6]
    jle have_right
    mov dx, [di+6]
have_right:
    cmp ax, dx
    jge clear_rect
    lds di, DWORD PTR [bp+14]
    mov [di+2], ax
    mov [di+6], dx

    lds di, DWORD PTR [bp+6]
    mov ax, es:[bx]
    cmp ax, [di]
    jge have_top
    mov ax, [di]
have_top:
    mov dx, es:[bx+4]
    cmp dx, [di+4]
    jle have_bottom
    mov dx, [di+4]
have_bottom:
    cmp ax, dx
    jge clear_rect
    lds di, DWORD PTR [bp+14]
    mov [di], ax
    mov [di+4], dx
    mov ax, 1
    jmp short done
clear_rect:
    sub ax, ax
    mov cx, 4
    les di, DWORD PTR [bp+14]
    rep stosw
done:
    pop ds
    pop di
    pop bp
    retf 12
SECTRECT ENDP
END
