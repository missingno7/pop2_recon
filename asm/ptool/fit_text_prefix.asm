.MODEL MEDIUM
.CODE
EXTRN MEASURE_CHARACTER:NEAR
PUBLIC FIT_TEXT_PREFIX

FIT_TEXT_PREFIX PROC FAR
    push bp
    mov bp, sp
    add sp, -4
    push di
    push si
    sub di, di
    mov [bp-2], di
    mov [bp-4], di
    les si, DWORD PTR [bp+12]
next_char:
    cmp di, [bp+10]
    je count_read
    push es
    push WORD PTR es:[si]
    push cs
    call MEASURE_CHARACTER
    pop es
    add ax, [bp-4]
    mov [bp-4], ax
    cmp ax, [bp+8]
    ja exceeds_width
    inc di
    lods BYTE PTR es:[si]
    cmp al, 0dh
    je count_read
    cmp al, '-'
    je save_break
    cmp WORD PTR [bp+6], 0
    jg before_next_word
    cmp al, ' '
    je save_break
    cmp BYTE PTR es:[si], ' '
    je save_break
    jmp SHORT next_char
before_next_word:
    cmp BYTE PTR es:[si], ' '
    jne next_char
    cmp al, ' '
    je next_char
save_break:
    mov [bp-2], di
    jmp SHORT next_char
exceeds_width:
    mov ax, [bp-2]
    test ax, ax
    jnz done
count_read:
    xchg ax, di
done:
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 10
FIT_TEXT_PREFIX ENDP
END
