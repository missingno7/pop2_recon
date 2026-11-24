; DOS interrupt and game port helpers of the resident module.

DGROUP GROUP _DATA
_DATA SEGMENT WORD PUBLIC 'DATA'
ASSUME DS:DGROUP
PUBLIC joystick_state, KEY_DOWN, INTERRUPT_VECTORS_INSTALLED
joystick_state DW 0, 0, 0, 0            ; y, x, centre y, centre x
    DB 0                                ; button bits
KEY_DOWN DB 0                           ; 59h scan codes
    DB 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    DB 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    DB 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    DB 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    DB 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    DB 0, 0, 0, 0, 0, 0, 0, 0
INTERRUPT_VECTORS_INSTALLED DW 0
saved_print_screen_vector DD 0
saved_keyboard_irq_vector DD 0
saved_video_bios_vector DD 0
_DATA ENDS

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

GAME_PORT EQU 201h
AXIS_WAIT_COUNT EQU 4000

PUBLIC POLL_GAME_PORT
POLL_GAME_PORT PROC FAR
    mov bl, 3
    mov cx, AXIS_WAIT_COUNT
    mov dx, GAME_PORT
wait_axes_idle:
    in al, dx
    test al, bl
    loopne wait_axes_idle
    jcxz axes_timeout
    mov ah, 3
    mov cx, AXIS_WAIT_COUNT
    pushf
    cli
    out dx, al
    ; Six executed NOPs delay the first port read after triggering the timer.
    nop
    nop
    nop
    nop
    nop
    nop
sample_axes:
    in al, dx
    test al, 1
    jne x_still_high
    test bl, 1
    je test_y_axis
    and bl, 0FEh
    push ax
    push bx
    mov ax, AXIS_WAIT_COUNT
    sub ax, cx
    mov bx, OFFSET DGROUP:joystick_state
    mov WORD PTR [bx+2], ax
    pop bx
    pop ax
    jmp SHORT test_y_axis
; Preserve the original flag tests and branches to the next instruction.
x_still_high:
    test bl, 1
    jne test_y_axis
test_y_axis:
    test al, 2
    jne y_still_high
    test bl, 2
    je poll_remaining_axes
    and bl, 0FDh
    push ax
    push bx
    mov ax, AXIS_WAIT_COUNT
    sub ax, cx
    mov bx, OFFSET DGROUP:joystick_state
    mov WORD PTR [bx], ax
    pop bx
    pop ax
    jmp SHORT poll_remaining_axes
y_still_high:
    test bl, 1
    jne poll_remaining_axes
poll_remaining_axes:
    test bl, 3
    loopne sample_axes
    popf
    jcxz axes_timeout
    sub ah, ah
    not al
    mov cl, 4
    shr al, cl
    mov bx, OFFSET DGROUP:joystick_state
    mov BYTE PTR [bx+8], al
joystick_done:
    retf
axes_timeout:
    mov ax, 0FFFFh
    mov bx, OFFSET DGROUP:joystick_state
    mov WORD PTR [bx+2], ax
    mov WORD PTR [bx], ax
    jmp SHORT joystick_done
POLL_GAME_PORT ENDP

PUBLIC CHECK_ROM_SIGNATURE
CHECK_ROM_SIGNATURE PROC FAR
    push es
    ; Read the two signature bytes from the same real-mode ROM window.
    mov ax, 0FC00h
    mov es, ax
    mov ax, 0
    cmp BYTE PTR es:[3FFEh], 0FFh
    jne rom_done
    cmp BYTE PTR es:[0], 21h
    jne rom_done
    mov ax, 1
rom_done:
    pop es
    retf
CHECK_ROM_SIGNATURE ENDP

PUBLIC POLL_DOS_KEY
POLL_DOS_KEY PROC FAR
    sub bx, bx
    mov ah, 6
    mov dl, 0FFh
    int 21h
    je input_done
    mov bl, al
    test bl, bl
    jne input_done
    ; A zero first byte denotes a DOS extended keystroke.
    mov ah, 6
    mov dl, 0FFh
    int 21h
    mov bh, al
input_done:
    xchg bx, ax
    retf
POLL_DOS_KEY ENDP

PRINCE_TEXT ENDS
END
