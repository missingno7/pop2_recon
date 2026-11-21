.MODEL MEDIUM
.CODE
EXTRN MATERIALIZE_HANDLE:NEAR
PUBLIC DETACH_HANDLE_PAYLOAD

DETACH_HANDLE_PAYLOAD PROC FAR
    push bp
    mov bp, sp
    mov bp, [bp+6]
    push bp
    push cs
    call MATERIALIZE_HANDLE
    xchg ax, bx
    mov es, dx
    or dx, bx
    jz empty
    and WORD PTR es:[bx-6], 3fffh
    or WORD PTR es:[bx-6], 8000h
    and WORD PTR [bp], 0dc00h
    mov WORD PTR [bp+2], 0
    xchg ax, bx
    mov dx, es
done:
    pop bp
    retf 2
empty:
    sub ax, ax
    cwd
    jmp SHORT done
DETACH_HANDLE_PAYLOAD ENDP
END
