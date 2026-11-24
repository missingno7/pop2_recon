GAMELIB_TEXT SEGMENT BYTE PUBLIC 'CODE'
GAMELIB_TEXT ENDS

_DATA SEGMENT WORD PUBLIC 'DATA'
EXTRN GRAPHICS_MODE:WORD
_DATA ENDS
DGROUP GROUP _DATA

GAMELIB_TEXT SEGMENT BYTE PUBLIC 'CODE'
ASSUME CS:GAMELIB_TEXT, DS:DGROUP, SS:DGROUP
PUBLIC READ_DAC_BLOCK, WRITE_DAC_BLOCK, WRITE_DAC_ENTRY, WAIT_DAC_WINDOW
PUBLIC WRITE_EGA_PALETTE, WRITE_TGA_PALETTE

DAC_PAUSE MACRO
    nop
    nop
    nop
    nop
ENDM

READ_DAC_BLOCK PROC FAR
    push bp
    mov bp, sp
    push es
    push di
    pushf
    cld
    mov cx, WORD PTR [bp+10]
    push cs
    call near ptr WAIT_DAC_WINDOW
    cli
    mov dx, 3c7h
    mov ax, WORD PTR [bp+12]
    out dx, al
    sub ah, ah
    les di, DWORD PTR [bp+6]
    mov dx, 3c9h
read_entry:
    in al, dx
    stosb
    DAC_PAUSE
    in al, dx
    stosb
    DAC_PAUSE
    in al, dx
    stosb
    DAC_PAUSE
    loop read_entry
    sti
    popf
    pop di
    pop es
    pop bp
    retf 8
READ_DAC_BLOCK ENDP

WRITE_DAC_BLOCK PROC FAR
    push bp
    mov bp, sp
    push ds
    push si
    cmp WORD PTR [bp+6], 0
    je select_entry
    push cs
    call near ptr WAIT_DAC_WINDOW
select_entry:
    mov cx, WORD PTR [bp+12]
    mov dx, 3c8h
    mov ax, WORD PTR [bp+14]
    out dx, al
    lds si, DWORD PTR [bp+8]
    mov dx, 3c9h
    mov ax, ds
    or ax, si
    jne write_entry
write_black:
    sub al, al
    out dx, al
    DAC_PAUSE
    out dx, al
    DAC_PAUSE
    out dx, al
    DAC_PAUSE
    loop write_black
    jmp short close_write
write_entry:
    lodsb
    out dx, al
    DAC_PAUSE
    lodsb
    out dx, al
    DAC_PAUSE
    lodsb
    out dx, al
    DAC_PAUSE
    loop write_entry
close_write:
    pop si
    pop ds
    pop bp
    retf 10
WRITE_DAC_BLOCK ENDP

; One DAC entry: index [bp+14], then red, green and blue.
WRITE_DAC_ENTRY PROC FAR
    push bp
    mov bp, sp
    cmp WORD PTR [bp+6], 0
    je select_single
    push cs
    call near ptr WAIT_DAC_WINDOW
select_single:
    mov dx, 3c8h
    mov ax, WORD PTR [bp+14]
    out dx, al
    mov dx, 3c9h
    mov ax, WORD PTR [bp+12]
    out dx, al
    DAC_PAUSE
    mov ax, WORD PTR [bp+10]
    out dx, al
    DAC_PAUSE
    mov ax, WORD PTR [bp+8]
    out dx, al
    pop bp
    retf 10
WRITE_DAC_ENTRY ENDP

WAIT_DAC_WINDOW PROC FAR
    mov dx, 3bah
    mov ah, 80h
    cmp WORD PTR ss:GRAPHICS_MODE, 2
    je wait_reset
    mov dx, 3dah
    mov ah, 8
wait_set:
    in al, dx
    test al, ah
    je wait_set
    retf
wait_reset:
    in al, dx
    test al, ah
    jne wait_reset
    retf
WAIT_DAC_WINDOW ENDP

; EGA attribute controller: register [bp+10] gets the value [bp+8].
WRITE_EGA_PALETTE PROC FAR
    push bp
    mov bp, sp
    push es
    cmp WORD PTR [bp+6], 0
    je ega_select
    push cs
    call near ptr WAIT_DAC_WINDOW
ega_select:
    mov ax, 40h
    mov es, ax
    mov dx, es:[63h]            ; BIOS CRT controller port
    add dl, 6                   ; its status register
    push dx
    in al, dx                   ; next write selects the index
    mov dl, 0c0h
    mov al, BYTE PTR [bp+10]
    out dx, al
    mov al, BYTE PTR [bp+8]
    out dx, al
    pop dx
    in al, dx
    mov dl, 0c0h
    mov al, 20h                 ; show the display again
    out dx, al
    pop es
    pop bp
    retf 6
WRITE_EGA_PALETTE ENDP

; Tandy video array: palette register [bp+10] gets the value [bp+8].
WRITE_TGA_PALETTE PROC FAR
    push bp
    mov bp, sp
    cmp WORD PTR [bp+6], 0
    je tga_select
    push cs
    call near ptr WAIT_DAC_WINDOW
tga_select:
    mov dx, 3dah
    in al, dx
    mov al, BYTE PTR [bp+10]
    add al, 10h
    out dx, al
    mov dx, 3deh
    mov al, BYTE PTR [bp+8]
    out dx, al
    mov dx, 3dah
    sub al, al
    out dx, al
    pop bp
    retf 6
WRITE_TGA_PALETTE ENDP
GAMELIB_TEXT ENDS
END
