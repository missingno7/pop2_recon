.MODEL MEDIUM
.CODE
PUBLIC UNPACKRLEV

UNPACKRLEV PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    push ds
    mov bx, WORD PTR [bp+6]
    lds si, DWORD PTR [bp+12]
    les di, DWORD PTR [bp+16]
    mov dx, WORD PTR [bp+10]
    dec WORD PTR [bp+10]
    dec WORD PTR [bp+8]
next_run:
    test dx, dx
    je finished
    lodsb
    cbw
    mov cx, ax
    test cx, cx
    js repeat_run
    inc cx
    sub dx, cx
    jbe clamp_literal
literal:
    movsb
    add di, WORD PTR [bp+8]
    dec bx
    loopne literal
    jne next_run
    ; At the end of a column, advance to the next column start.
    sub di, WORD PTR [bp+10]
    mov bx, WORD PTR [bp+6]
    jcxz next_run
    jmp short literal
clamp_literal:
    add cx, dx
    sub dx, dx
    jmp short literal
repeat_run:
    lodsb
    neg cx
    sub dx, cx
    jbe clamp_repeat
repeat_byte:
    stosb
    add di, WORD PTR [bp+8]
    dec bx
    loopne repeat_byte
    jne next_run
    sub di, WORD PTR [bp+10]
    mov bx, WORD PTR [bp+6]
    jcxz next_run
    jmp short repeat_byte
clamp_repeat:
    add cx, dx
    sub dx, dx
    jmp short repeat_byte
finished:
    xchg ax, si
    sub ax, WORD PTR [bp+12]
    pop ds
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 14
UNPACKRLEV ENDP
END
