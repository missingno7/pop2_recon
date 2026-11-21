.MODEL MEDIUM
.CODE
EXTRN GET_BLOCK_PAYLOAD_SIZE:NEAR
PUBLIC QUERY_HANDLE_SIZE

QUERY_HANDLE_SIZE PROC FAR
    push bp
    mov bp, sp
    push si
    push ds
    mov bp, [bp+6]
    test bp, bp
    jz no_payload
    lds si, DWORD PTR [bp]
    test si, 200h
    jnz no_payload
    and si, 0fh
    mov ax, ds
    or ax, si
    jz no_payload
    call GET_BLOCK_PAYLOAD_SIZE
done:
    pop ds
    pop si
    pop bp
    retf 2
no_payload:
    sub ax, ax
    cwd
    jmp SHORT done
QUERY_HANDLE_SIZE ENDP
END
