.MODEL MEDIUM
.CODE
PUBLIC ENUMERATE_DOS_DIRECTORY, QUALIFY_DOS_PATH

ENUMERATE_DOS_DIRECTORY PROC FAR
    push bp
    mov bp, sp
    add sp, -134
    push di
    push si
    mov ah, 2fh
    int 21h
    mov [bp-132], bx
    mov [bp-130], es
    mov ah, 1ah
    lea dx, [bp-128]
    int 21h
    sub cx, cx
    mov [bp-134], cx
    les di, DWORD PTR [bp+12]
    mov ah, 4eh
    mov cl, [bp+6]
    mov dx, [bp+8]
    int 21h
    jc search_result
    mov bx, [bp+10]
    mov dx, [bx]
    add dx, di
inspect_entry:
    mov al, [bp+6]
    xor al, [bp-107]
    test al, 18h
    jne search_next
    lea si, [bp-98]
    test BYTE PTR [bp-107], 10h
    jz copy_name
    cmp di, dx
    jae directory_full
    mov al, '['
    stosb
copy_name:
    cmp di, dx
    jae directory_full
    lodsb
    cmp al, '.'
    jne store_name_byte
    test BYTE PTR [bp-107], 8
    jne copy_name
store_name_byte:
    stosb
    test al, al
    jne copy_name
    test BYTE PTR [bp-107], 10h
    jz count_entry
    cmp di, dx
    jae directory_full
    mov BYTE PTR es:[di-1], ']'
    stosb
count_entry:
    inc WORD PTR [bp-134]
search_next:
    mov ah, 4fh
    int 21h
    jnc inspect_entry
search_result:
    cmp ax, 12h
    je directory_full
    xchg ax, di
    mov WORD PTR [bp-134], 0
    jmp SHORT restore_dta
directory_full:
    sub di, di
restore_dta:
    push ds
    mov ah, 1ah
    lds dx, DWORD PTR [bp-132]
    int 21h
    pop ds
    mov ax, [bp-134]
    mov bx, [bp+10]
    mov [bx], ax
    xchg ax, di
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 10
ENUMERATE_DOS_DIRECTORY ENDP

QUALIFY_DOS_PATH PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    push ds
    lds si, DWORD PTR [bp+6]
    les di, DWORD PTR [bp+10]
    lodsw
    cmp ah, ':'
    je write_drive
    sub si, 2
    mov ah, 19h
    int 21h
    add al, 'A'
    mov ah, ':'
write_drive:
    stosw
    and al, 0dfh
    sub al, 40h
    xchg ax, dx
    mov al, 5ch
    cmp [si], al
    je append_path
    stosb
    push ds
    push es
    pop ds
    xchg di, si
    mov ah, 47h
    int 21h
    xchg di, si
    pop ds
    jc qualify_done
    sub al, al
    mov cx, 0ffffh
    repne scasb
    dec di
    mov al, 5ch
    cmp es:[di-1], al
    je append_path
    stosb
append_path:
    sub ax, ax
    mov cx, [bp+10]
    add cx, 40h
    sub cx, di
    jbe path_too_long
copy_path:
    lodsb
    stosb
    test al, al
    loopne copy_path
    je qualify_done
path_too_long:
    mov ax, 3
qualify_done:
    pop ds
    pop si
    pop di
    pop bp
    retf 8
QUALIFY_DOS_PATH ENDP
END
