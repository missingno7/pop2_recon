.MODEL MEDIUM
.CODE
PUBLIC SET_CURRENT_DOS_DRIVE, SET_CURRENT_DOS_DIRECTORY

SET_CURRENT_DOS_DRIVE PROC FAR
    push bp
    mov bp, sp
    mov dl, [bp+6]
    and dl, 0dfh
    sub dl, 'A'
    mov ah, 0eh
    int 21h
    mov ah, 19h
    int 21h
    sub al, dl
    cbw
    jz drive_selected
    mov ax, 0fh
drive_selected:
    pop bp
    retf 2
SET_CURRENT_DOS_DRIVE ENDP

SET_CURRENT_DOS_DIRECTORY PROC FAR
    push bp
    mov bp, sp
    push ds
    mov ah, 3bh
    lds bx, DWORD PTR [bp+6]
    mov dx, bx
    int 21h
    jnc directory_changed
    cmp WORD PTR [bx+1], ':'
    jne directory_done
select_path_drive:
    push WORD PTR [bx]
    push ss
    pop ds
    push cs
    call NEAR PTR SET_CURRENT_DOS_DRIVE
    jmp SHORT directory_done
directory_changed:
    cmp BYTE PTR [bx+1], ':'
    je select_path_drive
    sub ax, ax
directory_done:
    pop ds
    pop bp
    retf 4
SET_CURRENT_DOS_DIRECTORY ENDP
END
