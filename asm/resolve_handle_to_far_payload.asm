.MODEL MEDIUM
.CODE
PUBLIC H2P

H2P PROC FAR
    push bp
    mov bp, sp
    mov bp, [bp+6]
    test bp, bp
    jz no_payload
    les ax, DWORD PTR [bp]
    test ax, 200h
    jnz no_payload
    and ax, 0fh
    mov dx, es
    or dx, ax
    jz done
    add ax, 6
    mov dx, es
done:
    pop bp
    retf 2
no_payload:
    sub ax, ax
    cwd
    jmp SHORT done
H2P ENDP
END
