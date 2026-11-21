.MODEL MEDIUM
.DATA
directory_pattern DB '?:\????????.???',0
.CODE
EXTRN ENUMERATE_DOS_DIRECTORY:NEAR
PUBLIC CHECK_DOS_DRIVE

CHECK_DOS_DRIVE PROC FAR
    push bp
    mov bp, sp
    add sp, -2
    les ax, DWORD PTR [bp+8]
    push es
    push ax
    lea bx, [bp-2]
    mov WORD PTR [bx], 12
    push bx
    mov al, [bp+6]
    mov bx, OFFSET directory_pattern
    mov [bx], al
    push bx
    mov ax, 8
    push ax
    push cs
    call ENUMERATE_DOS_DIRECTORY
    test ax, ax
    jne result_ready
    cmp ax, [bp-2]
    jne result_ready
    les bx, DWORD PTR [bp+8]
    mov es:[bx], al
result_ready:
    mov sp, bp
    pop bp
    retf 6
CHECK_DOS_DRIVE ENDP
END
