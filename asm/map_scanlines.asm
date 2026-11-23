GAMELIB_TEXT SEGMENT BYTE PUBLIC 'CODE'
ASSUME CS:GAMELIB_TEXT
PUBLIC MAP_ALTERNATING_SCANLINES

MAP_ALTERNATING_SCANLINES PROC FAR
    push bp
    mov bp, sp
    push si
    push di
    push ds
    lds si, DWORD PTR [bp+14]
    lodsw
    cmp ax, 0
    jle finished
    mov dx, ax
    add si, 2
    lodsw
    mov cx, ax
    lds si, DWORD PTR [bp+10]
    les di, DWORD PTR [bp+10]
next_row:
    push cx
    mov bx, WORD PTR [bp+8]
    test dx, 1
    jne next_pixel
    mov bx, WORD PTR [bp+6]
next_pixel:
    lodsb
    xlat BYTE PTR ss:[bx]
    stosb
    loop next_pixel
    pop cx
    dec dx
    jne next_row
finished:
    pop ds
    pop di
    pop si
    pop bp
    retf 12
MAP_ALTERNATING_SCANLINES ENDP
GAMELIB_TEXT ENDS
END
