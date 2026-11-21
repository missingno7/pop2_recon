.MODEL MEDIUM
.CODE
PUBLIC MULTIPLY_TIMER_INTERVAL

MULTIPLY_TIMER_INTERVAL PROC FAR
    push bp
    mov bp, sp
    add sp, -10
    push di
    push si
    sub ax, ax
    mov cx, 5
    lea di, [bp-10]
    push ss
    pop es
    rep stosw
    mov bx, 2
    sub si, si
    test BYTE PTR [bp+9], 80h
    jz right_positive
    not ax
    neg WORD PTR [bp+6]
    adc [bp+8], si
    neg WORD PTR [bp+8]
right_positive:
    test BYTE PTR [bp+13], 80h
    jz left_positive
    not ax
    neg WORD PTR [bp+10]
    adc [bp+12], si
    neg WORD PTR [bp+12]
left_positive:
    push ax
multiply_input:
    sub di, di
multiply_word:
    push di
    mov ax, [bp+di+10]
    mul WORD PTR [bp+si+6]
    add di, si
    add [bp+di-10], ax
    adc [bp+di-8], dx
    adc [bp+di-6], cx
    pop di
    add di, bx
    cmp di, bx
    je multiply_word
    add si, bx
    cmp si, bx
    je multiply_input
    les ax, DWORD PTR [bp-8]
    mov dx, es
    pop cx
    jcxz return_product
    neg ax
    adc dx, 0
    neg dx
return_product:
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 8
MULTIPLY_TIMER_INTERVAL ENDP
END
