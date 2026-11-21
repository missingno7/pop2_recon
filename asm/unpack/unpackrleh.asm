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
    je finished
next_run:
    lodsb
    cbw
    mov cx, ax
    test cx, cx
    js repeat_run
    ; A nonnegative run code gives one plus its value in literal bytes.
    inc cx
    sub bp, cx
    jbe final_literal
    rep movsb
    jmp short next_run
final_literal:
    add cx, bp
    rep movsb
    jmp short finished
repeat_run:
    lodsb
    neg cx
    sub bp, cx
    jbe final_repeat
    rep stosb
    jmp short next_run
final_repeat:
    add cx, bp
    rep stosb
finished:
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
END
