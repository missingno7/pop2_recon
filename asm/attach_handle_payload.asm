.MODEL MEDIUM
.CODE
EXTRN RELEASE_HANDLE_STORAGE:NEAR
PUBLIC ATTACH_HANDLE_PAYLOAD

ATTACH_HANDLE_PAYLOAD PROC FAR
    push bp
    mov bp, sp
    push WORD PTR [bp+10]
    push cs
    call RELEASE_HANDLE_STORAGE
    test ax, ax
    jnz done
    les bx, DWORD PTR [bp+6]
    mov ax, es
    or ax, bx
    jz done
    mov bp, [bp+10]
    sub bx, 6
    and WORD PTR es:[bx], 3fffh
    or WORD PTR es:[bx], 0c000h
    mov es:[bx+4], bp
    or [bp], bx
    mov [bp+2], es
    sub ax, ax
done:
    pop bp
    retf 6
ATTACH_HANDLE_PAYLOAD ENDP
END
