.MODEL MEDIUM
.CODE
EXTRN SECTRECT:NEAR
PUBLIC SUBTRACT_RECTANGLE_STRIP

cut_top EQU WORD PTR [bp-8]
cut_left EQU WORD PTR [bp-6]
cut_bottom EQU WORD PTR [bp-4]
cut_right EQU WORD PTR [bp-2]

SUBTRACT_RECTANGLE_STRIP PROC FAR
    push bp
    mov bp, sp
    add sp, -8
    push di
    push si
    push ds
    lea ax, cut_top
    push ds
    push ax
    les ax, DWORD PTR [bp+10]
    push es
    push ax
    les ax, DWORD PTR [bp+6]
    push es
    push ax
    push cs
    call SECTRECT
    lds si, DWORD PTR [bp+10]
    les di, DWORD PTR [bp+14]
    xchg cx, ax
    jcxz copy_source
    mov cx, 1
    mov ax, [si+2]
    mov bx, [si+6]
    mov dx, cut_right
    cmp ax, cut_left
    je check_right_cut
    xchg cut_left, ax
    mov cut_right, ax
    dec cx
check_right_cut:
    cmp bx, dx
    je check_top_cut
    jcxz copy_source
    xchg cut_right, bx
    mov cut_left, bx
    dec cx
check_top_cut:
    mov ax, [si]
    mov bx, [si+4]
    mov dx, cut_bottom
    cmp ax, cut_top
    je check_bottom_cut
    jcxz copy_source
    xchg cut_top, ax
    mov cut_bottom, ax
    dec cx
check_bottom_cut:
    cmp bx, dx
    je store_result
    jcxz copy_source
    xchg cut_bottom, bx
    mov cut_top, bx
    dec cx
store_result:
    jcxz copy_strip
    sub al, al
    mov cx, 8
    rep stosb
    jmp SHORT result_ready
copy_strip:
    lea si, cut_top
    push ss
    pop ds
copy_source:
    mov cx, 8
    rep movsb
result_ready:
    les ax, DWORD PTR [bp+14]
    mov dx, es
    pop ds
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 12
SUBTRACT_RECTANGLE_STRIP ENDP
END
