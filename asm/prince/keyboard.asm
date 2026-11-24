_DATA SEGMENT WORD PUBLIC 'DATA'
EXTRN KEY_DOWN:BYTE
EXTRN INTERRUPT_VECTORS_INSTALLED:WORD
; DOS reads the two words as one vector in LDS and indirect far CALL.
EXTRN SAVED_PRINT_SCREEN_VECTOR:DWORD
EXTRN SAVED_KEYBOARD_IRQ_VECTOR:DWORD
EXTRN SAVED_VIDEO_BIOS_VECTOR:DWORD
_DATA ENDS
DGROUP GROUP _DATA

PRINCE_TEXT SEGMENT BYTE PUBLIC 'CODE'
ASSUME CS:PRINCE_TEXT, DS:DGROUP, SS:DGROUP

PUBLIC largest_dos_memory_block
largest_dos_memory_block PROC FAR
    mov ah, 48h
    mov bx, 0FFFFh
    int 21h
    xchg ax, bx                 ; DOS returns the largest available block in BX
    retf
largest_dos_memory_block ENDP

PUBLIC install_interrupt_handlers
install_interrupt_handlers PROC FAR
    push es
    mov al, 05h
    mov ah, 35h
    int 21h
    mov WORD PTR SAVED_PRINT_SCREEN_VECTOR, bx
    mov WORD PTR SAVED_PRINT_SCREEN_VECTOR+2, es
    mov al, 09h
    mov ah, 35h
    int 21h
    mov WORD PTR SAVED_KEYBOARD_IRQ_VECTOR, bx
    mov WORD PTR SAVED_KEYBOARD_IRQ_VECTOR+2, es
    mov al, 10h
    mov ah, 35h
    int 21h
    mov WORD PTR SAVED_VIDEO_BIOS_VECTOR, bx
    mov WORD PTR SAVED_VIDEO_BIOS_VECTOR+2, es

    push ds
    push cs
    pop ds                      ; DOS takes the replacement vector in DS:DX
    mov al, 05h
    mov ah, 25h
    mov dx, OFFSET ignore_print_screen
    int 21h
    mov al, 09h
    mov ah, 25h
    mov dx, OFFSET track_keyboard_irq
    int 21h
    mov al, 10h
    mov ah, 25h
    mov dx, OFFSET filter_video_interrupt
    int 21h
    mov al, 23h
    mov ah, 25h
    mov dx, OFFSET ignore_ctrl_break
    int 21h
    pop ds
    mov INTERRUPT_VECTORS_INSTALLED, 1
    pop es
    retf
install_interrupt_handlers ENDP

PUBLIC RESTORE_INTERRUPT_VECTORS

RESTORE_INTERRUPT_VECTORS PROC FAR
    push ds
    mov ah, 30h
    int 21h
    mov al, 05h
    mov ah, 25h
    lds dx, DWORD PTR ss:[SAVED_PRINT_SCREEN_VECTOR]
    int 21h
    mov al, 09h
    mov ah, 25h
    lds dx, DWORD PTR ss:[SAVED_KEYBOARD_IRQ_VECTOR]
    int 21h
    mov al, 10h
    mov ah, 25h
    lds dx, DWORD PTR ss:[SAVED_VIDEO_BIOS_VECTOR]
    int 21h
    pop ds
    retf
RESTORE_INTERRUPT_VECTORS ENDP

PUBLIC ignore_print_screen
ignore_print_screen PROC FAR
    iret
ignore_print_screen ENDP

PUBLIC track_keyboard_irq
track_keyboard_irq PROC FAR
    push ds
    push ax
    push bx
    mov ax, DGROUP
    mov ds, ax
    in al, 60h
    mov bl, al
    rol al, 1
    and al, 1
    xor al, 1                   ; make code sets the entry, break code clears it
    sub bh, bh
    and bl, 7Fh
    cmp bl, 59h
    jae chain_keyboard_irq
    add bx, OFFSET DGROUP:KEY_DOWN
    mov [bx], al
chain_keyboard_irq:
    pop bx
    pop ax
    pushf
    call SAVED_KEYBOARD_IRQ_VECTOR
    pop ds
    iret
track_keyboard_irq ENDP

PUBLIC filter_video_interrupt
filter_video_interrupt PROC FAR
    push ds
    push ax
    mov ax, DGROUP
    mov ds, ax
    pop ax
    cmp ah, 0Eh                 ; skip BIOS teletype output
    je video_done
    pushf
    call SAVED_VIDEO_BIOS_VECTOR
video_done:
    pop ds
    iret
filter_video_interrupt ENDP

PUBLIC ignore_ctrl_break
ignore_ctrl_break PROC FAR
    iret
ignore_ctrl_break ENDP

PRINCE_TEXT ENDS
END
