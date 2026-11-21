.MODEL MEDIUM
.CODE
PUBLIC SKIP_NUL_STRINGS

SKIP_NUL_STRINGS PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    sub ax, ax
    cwd
    les di, DWORD PTR [bp+8]
    add dx, [bp+6]
    jz done
skip_one:
    mov cx, 0ffffh
    repne scasb
    dec dx
    jnz skip_one
done:
    mov dx, es
    xchg ax, di
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 6
SKIP_NUL_STRINGS ENDP
EVEN
END
