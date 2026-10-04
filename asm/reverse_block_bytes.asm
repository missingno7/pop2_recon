; Semantic name hypothesis only: original symbol and purpose are unknown.
; This procedure reproduces the complete reviewed 51-byte resident component.
.MODEL LARGE
.CODE
PUBLIC reverse_block_bytes
reverse_block_bytes PROC FAR
    push bp
    mov bp, sp
    push ds
    push si
    push di

    les di, DWORD PTR [bp+0Ah]
    push es
    pop ds

; Static edge observation: this do-while DEC/JNE runs 65536 passes when count starts at zero.
outer_block:
    mov bx, di
    mov si, di
    mov cx, WORD PTR [bp+6]
    add si, cx
    dec si
    inc cx
    shr cx, 1

; Static edge observation: CX is rounded in 16-bit arithmetic; LOOP runs even when CX starts at zero.
; STOSB follows DF. There is no CLD, so matching bytes alone does not prove the caller clears DF.
reverse_pair:
    mov ah, BYTE PTR [di]
    mov al, BYTE PTR [si]
    stosb
    mov BYTE PTR [si], ah
    dec si
    loop reverse_pair

    mov di, bx
    add di, WORD PTR [bp+6]
    dec WORD PTR [bp+8]
    jne outer_block

    pop di
    pop si
    pop ds
    pop bp
    retf 8
reverse_block_bytes ENDP
END
