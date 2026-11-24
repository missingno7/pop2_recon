; Resident DOS helpers following the interrupt-vector code in PRINCE_TEXT.
; Procedure names describe the observed operations.

_DATA SEGMENT WORD PUBLIC 'DATA'
EXTRN joystick_state:BYTE
_DATA ENDS
DGROUP GROUP _DATA

PRINCE_TEXT SEGMENT BYTE PUBLIC 'CODE'
ASSUME CS:PRINCE_TEXT, DS:DGROUP, SS:DGROUP

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
