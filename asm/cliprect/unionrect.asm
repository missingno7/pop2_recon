.MODEL MEDIUM
.CODE
PUBLIC UNIONRECT

UNIONRECT PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    push ds
    les di, DWORD PTR [bp+10]
    lds si, DWORD PTR [bp+6]

    ; Select the lesser near edges and the greater far edges.
    mov ax, es:[di]
    cmp ax, [si]
    jle keep_first_near
    mov ax, [si]
keep_first_near:
    mov bx, es:[di+2]
    cmp bx, [si+2]
    jle keep_second_near
    mov bx, [si+2]
keep_second_near:
    mov cx, es:[di+4]
    cmp cx, [si+4]
    jge keep_first_far
    mov cx, [si+4]
keep_first_far:
    mov dx, es:[di+6]
    cmp dx, [si+6]
    jge keep_second_far
    mov dx, [si+6]
keep_second_far:
    les di, DWORD PTR [bp+14]
    stosw
    xchg ax, bx
    stosw
    xchg ax, cx
    stosw
    xchg ax, dx
    stosw
    mov ax, [bp+14]
    mov dx, es
    pop ds
    pop si
    pop di
    pop bp
    retf 12
UNIONRECT ENDP
END
