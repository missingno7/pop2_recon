.MODEL MEDIUM
.DATA
directory_pattern DB '?:\????????.???',0
.CODE
EXTRN ENUMERATE_DOS_DIRECTORY:NEAR
PUBLIC GET_CURRENT_DOS_DRIVE_LETTER, GET_CURRENT_DOS_DIRECTORY, CHECK_DOS_DRIVE

GET_CURRENT_DOS_DRIVE_LETTER PROC FAR
    mov ah, 19h
    int 21h
    add al, 'A'
    cbw
    retf
GET_CURRENT_DOS_DRIVE_LETTER ENDP

GET_CURRENT_DOS_DIRECTORY PROC FAR
    push bp
    mov bp, sp
    push si
    push ds
    push cs
    call NEAR PTR GET_CURRENT_DOS_DRIVE_LETTER
    lds si, DWORD PTR [bp+6]
    mov [si], al
    mov WORD PTR [si+1], 5c3ah
    add si, 3
    mov ah, 47h
    sub dl, dl
    int 21h
    jc get_directory_done
    sub ax, ax
get_directory_done:
    pop ds
    pop si
    pop bp
    retf 4
GET_CURRENT_DOS_DIRECTORY ENDP

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
