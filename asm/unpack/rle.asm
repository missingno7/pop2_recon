.MODEL MEDIUM
.CODE

PUBLIC UNPACKRLEH

UNPACKRLEH PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    push ds
    push bp
    lds si, DWORD PTR [bp+8]
    les di, DWORD PTR [bp+12]
    mov bp, WORD PTR [bp+6]
    test bp, bp
    je rleh_finished
rleh_next_run:
    lodsb
    cbw
    mov cx, ax
    test cx, cx
    js rleh_repeat_run
    ; A nonnegative run code gives one plus its value in literal bytes.
    inc cx
    sub bp, cx
    jbe rleh_final_literal
    rep movsb
    jmp short rleh_next_run
rleh_final_literal:
    add cx, bp
    rep movsb
    jmp short rleh_finished
rleh_repeat_run:
    lodsb
    neg cx
    sub bp, cx
    jbe rleh_final_repeat
    rep stosb
    jmp short rleh_next_run
rleh_final_repeat:
    add cx, bp
    rep stosb
rleh_finished:
    pop bp
    xchg ax, si
    sub ax, WORD PTR [bp+8]
    pop ds
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 10
UNPACKRLEH ENDP

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
rlev_next_run:
    test dx, dx
    je rlev_finished
    lodsb
    cbw
    mov cx, ax
    test cx, cx
    js rlev_repeat_run
    inc cx
    sub dx, cx
    jbe rlev_clamp_literal
rlev_literal:
    movsb
    add di, WORD PTR [bp+8]
    dec bx
    loopne rlev_literal
    jne rlev_next_run
    ; At the end of a column, advance to the next column start.
    sub di, WORD PTR [bp+10]
    mov bx, WORD PTR [bp+6]
    jcxz rlev_next_run
    jmp short rlev_literal
rlev_clamp_literal:
    add cx, dx
    sub dx, dx
    jmp short rlev_literal
rlev_repeat_run:
    lodsb
    neg cx
    sub dx, cx
    jbe rlev_clamp_repeat
rlev_repeat_byte:
    stosb
    add di, WORD PTR [bp+8]
    dec bx
    loopne rlev_repeat_byte
    jne rlev_next_run
    sub di, WORD PTR [bp+10]
    mov bx, WORD PTR [bp+6]
    jcxz rlev_next_run
    jmp short rlev_repeat_byte
rlev_clamp_repeat:
    add cx, dx
    sub dx, dx
    jmp short rlev_repeat_byte
rlev_finished:
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
