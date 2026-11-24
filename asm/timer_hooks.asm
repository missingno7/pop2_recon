.MODEL MEDIUM
.DATA
timer_rate DW 0
timer_countdowns DW 0, 0, 0, 0
timer_ticks_low DW 0
timer_ticks_high DW 0
PUBLIC timer_chain_offset
timer_chain_offset DW 0
timer_chain_segment DW 0
default_timer_hook LABEL WORD
    DW 0, 0
original_timer_vector DD 0
    DW 0
timer_ratio_low DW 0
timer_ratio_high DW 0
timer_ratio_accumulator DW 0
pit_frequency DD 1193181
EXTRN _end:BYTE
.CODE
EXTRN DIVIDE_TIMER_INTERVAL:NEAR
EXTRN MULTIPLY_TIMER_INTERVAL:NEAR
PUBLIC INSTALL_DEFAULT_TIMER, INSTALL_TIMER_HOOK, RESET_TIMER_HOOK, REMOVE_TIMER_HOOK
PUBLIC DEFAULT_TIMER_INTERRUPT

INSTALL_DEFAULT_TIMER PROC FAR
    push bp
    mov bp, sp
    mov ax, OFFSET default_timer_hook
    push ds
    push ax
    mov ax, OFFSET DEFAULT_TIMER_INTERRUPT
    push cs
    push ax
    mov ax, [bp+6]
    push ax
    mov timer_rate, ax
    push cs
    call NEAR PTR INSTALL_TIMER_HOOK
    pop bp
    retf 2
INSTALL_DEFAULT_TIMER ENDP

INSTALL_TIMER_HOOK PROC FAR
    push bp
    mov bp, sp
    push di
    push ds
    mov ax, DGROUP
    mov ds, ax
    mov dx, ss
    mov bx, sp
    cmp ax, dx
    je install_on_stack
    mov ss, ax
    mov sp, OFFSET _end
install_on_stack:
    push dx
    push bx
    sub ax, ax
    mov cx, 16
    les di, DWORD PTR [bp+12]
    rep stosb
    mov ax, [bp+6]
    cmp ax, WORD PTR [pit_frequency+2]
    ja divide_frequency
    xchg ax, cx
    jmp SHORT save_timer_state
divide_frequency:
    les dx, pit_frequency
    push es
    push dx
    push ax
    push cx
    push cs
    call DIVIDE_TIMER_INTERVAL
save_timer_state:
    pushf
    cli
    les bx, DWORD PTR timer_chain_offset
    lds di, DWORD PTR [bp+12]
    mov [di+8], ax
    mov [di], bx
    mov [di+2], es
    mov dx, es
    or dx, bx
    jz old_rate_ready
    mov dx, es:[bx+8]
old_rate_ready:
    sub cx, cx
    cmp cx, ax
    cmc
    adc cx, cx
    push cx
    push ax
    sub cx, cx
    cmp cx, dx
    cmc
    adc cx, cx
    push cx
    push dx
    push cs
    call DIVIDE_TIMER_INTERVAL
    mov [di+10], ax
    mov [di+12], dx
    les ax, DWORD PTR [bp+8]
    mov dx, es
    sub bx, bx
    mov es, bx
    xchg ax, es:[20h]
    xchg dx, es:[22h]
    mov [di+4], ax
    mov [di+6], dx
    mov ss:[timer_chain_offset], di
    mov ss:[timer_chain_segment], ds
    mov al, 36h
    out 43h, al
    mov ax, [di+8]
    out 40h, al
    mov al, ah
    out 40h, al
    popf
    pop ax
    pop ss
    mov sp, ax
    pop ds
    pop di
    pop bp
    retf 10
INSTALL_TIMER_HOOK ENDP

RESET_TIMER_HOOK PROC FAR
    pushf
    cli
    mov ax, OFFSET default_timer_hook
    mov timer_chain_offset, ax
    mov timer_chain_segment, ds
    push ds
    push ax
    push cs
    call NEAR PTR REMOVE_TIMER_HOOK
    popf
    retf
RESET_TIMER_HOOK ENDP

REMOVE_TIMER_HOOK PROC FAR
    push bp
    mov bp, sp
    push di
    push ds
    pushf
    mov ax, DGROUP
    mov ds, ax
    mov dx, ss
    mov bx, sp
    cmp ax, dx
    je remove_on_stack
    mov ss, ax
    mov sp, OFFSET _end
remove_on_stack:
    push dx
    push bx
    cli
    mov di, OFFSET timer_chain_offset
find_hook:
    mov bx, di
    mov ax, ds
    mov es, ax
    or ax, bx
    jz remove_done
    lds di, DWORD PTR [di]
    cmp di, [bp+6]
    jne find_hook
    mov cx, ds
    cmp cx, [bp+8]
    jne find_hook
    mov ax, [di+4]
    mov dx, [di+6]
    cmp di, ss:[timer_chain_offset]
    jne remove_middle
    sub cx, ss:[timer_chain_segment]
    jz remove_first
remove_middle:
    mov es:[bx+4], ax
    mov es:[bx+6], dx
    mov ax, [di]
    mov es:[bx], ax
    mov ax, [di+2]
    mov es:[bx+2], ax
    push es
    push bx
    les ax, DWORD PTR es:[bx+10]
    push es
    push ax
    les ax, DWORD PTR [di+10]
    push es
    push ax
    push cs
    call MULTIPLY_TIMER_INTERVAL
    pop di
    pop ds
    mov [di+10], ax
    mov [di+12], dx
    jmp SHORT remove_done
remove_first:
    mov es, cx
    mov es:[20h], ax
    mov es:[22h], dx
    les bx, DWORD PTR [di]
    mov ss:[timer_chain_offset], bx
    mov ss:[timer_chain_segment], es
    mov ax, es
    or ax, bx
    jz program_period
    mov bx, es:[bx+8]
program_period:
    mov al, 36h
    out 43h, al
    xchg ax, bx
    out 40h, al
    mov al, ah
    out 40h, al
remove_done:
    pop ax
    pop ss
    mov sp, ax
    popf
    pop ds
    pop di
    pop bp
    retf 4
REMOVE_TIMER_HOOK ENDP

DEFAULT_TIMER_INTERRUPT PROC FAR
    push ds
    push ax
    push bx
    mov ax, DGROUP
    mov ds, ax
    mov bx, 8
    inc timer_ticks_low
    jnz decrement_countdowns
    inc timer_ticks_high
decrement_countdowns:
    cmp WORD PTR [bx+timer_rate], 0
    je next_countdown
    dec WORD PTR [bx+timer_rate]
next_countdown:
    sub bx, 2
    jnz decrement_countdowns
    mov ax, timer_ratio_low
    add timer_ratio_accumulator, ax
    adc bx, timer_ratio_high
    jnz invoke_old_timer
    mov al, 20h
    out 20h, al
    jmp SHORT timer_interrupt_done
invoke_old_timer:
    pushf
    call original_timer_vector
timer_interrupt_done:
    pop bx
    pop ax
    pop ds
    iret
DEFAULT_TIMER_INTERRUPT ENDP
END
