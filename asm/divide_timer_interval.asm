.MODEL MEDIUM
.CODE
PUBLIC DIVIDE_TIMER_INTERVAL

DIVIDE_TIMER_INTERVAL PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    sub ax, ax
    sub bx, bx
    sub di, di
    mov si, 1
    test BYTE PTR [bp+9], 80h
    jz divisor_positive
    not ax
    neg WORD PTR [bp+6]
    adc [bp+8], di
    neg WORD PTR [bp+8]
divisor_positive:
    test BYTE PTR [bp+13], 80h
    jz dividend_positive
    not ax
    neg WORD PTR [bp+10]
    adc [bp+12], di
    neg WORD PTR [bp+12]
dividend_positive:
    push ax
divide_word:
    mov cx, 48
    sub ax, ax
    cwd
divide_bit:
    rcl WORD PTR [bp+10], 1
    rcl WORD PTR [bp+12], 1
    rcl bx, 1
    rcl ax, 1
    rcl dx, 1
    cmp dx, [bp+8]
    ja subtract_divisor
    jb rotate_quotient
    cmp ax, [bp+6]
    jb rotate_quotient
subtract_divisor:
    sub ax, [bp+6]
    sbb dx, [bp+8]
rotate_quotient:
    cmc
    loop divide_bit
    xchg di, [bp+10]
    xchg ax, [bp+12]
    xchg dx, bx
    rcl di, 1
    rcl ax, 1
    rcl dx, 1
    dec si
    jge divide_word
    xchg ax, di
    mov dx, [bp+10]
    pop cx
    jcxz return_quotient
    neg ax
    adc dx, 0
    neg dx
return_quotient:
    pop si
    pop di
    pop bp
    retf 8
DIVIDE_TIMER_INTERVAL ENDP
END
