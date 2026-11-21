.MODEL MEDIUM
EXTRN SOUND_DGROUP:BYTE

.DATA
EXTRN ACTIVE_DRIVERS:BYTE
EXTRN DIGITAL_ACTIVE:WORD
EXTRN SPEAKER_ENABLED:BYTE
EXTRN SPEAKER_CONTINUATION:BYTE
EXTRN CALLBACK_STACK_TOP:BYTE

speaker_active DW 0
speaker_repeat DW 0
speaker_payload DD 0
speaker_completion DD 0
                 DD 0, 0
sequence_marker  DB "MSeq"
speaker_position DD 0
speaker_timer_state DD 0
previous_timer DD 0
                 DW 0
timer_increment DD 0
timer_fraction DW 0
speaker_divisor DW 0
sweep_count DW 0
sweep_delta DW 0
ticks_to_event DB 0
sweep_length DB 0
sweep_fraction DB 0

.CODE
EXTRN RELEASE_SPEAKER_SOUND:NEAR
EXTRN RELEASE_FIRST_DEVICE_SOUND:NEAR
EXTRN INSTALL_TIMER_HOOK:NEAR
PUBLIC START_SPEAKER_PAYLOAD, SPEAKER_TIMER_INTERRUPT

START_SPEAKER_PAYLOAD PROC NEAR
    push bp
    mov bp, sp
    sub ax, ax
    mov es, ax
    call RELEASE_SPEAKER_SOUND
    mov cx, DIGITAL_ACTIVE
    jcxz set_callback
    test ACTIVE_DRIVERS, 1
    jne set_callback
    sub ax, ax
    mov es, ax
    call RELEASE_FIRST_DEVICE_SOUND

set_callback:
    les ax, [bp+4]
    mov WORD PTR speaker_completion, ax
    mov WORD PTR speaker_completion+2, es
    pushf
    cli
    les bx, [bp+8]
    mov WORD PTR speaker_payload, bx
    mov WORD PTR speaker_payload+2, es
    lea ax, [bx+3]
    mov WORD PTR speaker_position, ax
    mov WORD PTR speaker_position+2, es

    mov ax, OFFSET speaker_timer_state
    push ds
    push ax
    mov ax, OFFSET SPEAKER_TIMER_INTERRUPT
    push cs
    push ax
    push WORD PTR es:[bx+1]
    push cs
    call INSTALL_TIMER_HOOK
    sub ax, ax
    mov sweep_length, al
    inc ax
    mov ticks_to_event, al
    mov speaker_active, ax
    mov speaker_repeat, 0
    mov al, 0b6h
    out 43h, al
    int 8
    popf

    mov ax, speaker_active
    pop bp
    ret 8
START_SPEAKER_PAYLOAD ENDP

SPEAKER_TIMER_INTERRUPT PROC FAR
    push ax
    push bx
    push cx
    push dx
    push ds
    push es
    mov ax, SEG SOUND_DGROUP
    mov ds, ax
    dec ticks_to_event
    jz read_event

    sub ax, ax
    add al, sweep_length
    jne update_sweep
chain_timer:
    les ax, timer_increment
    mov cx, es
    add timer_fraction, ax
    adc cx, 0
    je acknowledge
call_previous:
    pushf
    call previous_timer
    loop call_previous
    jmp short restore_registers
acknowledge:
    mov al, 20h
    out 20h, al
restore_registers:
    pop es
    pop ds
    pop dx
    pop cx
    pop bx
    pop ax
    iret

update_sweep:
    dec sweep_count
    jne advance_sweep
    shl ax, 1
    mov sweep_count, ax
    neg sweep_delta
advance_sweep:
    mov ax, sweep_delta
    add sweep_fraction, al
    adc ah, 0
    je chain_timer
    xchg ah, al
    cbw
    add ax, speaker_divisor
    jmp short program_divisor

silence:
    in al, 61h
    and al, 0fch
    out 61h, al
    jmp short chain_timer

read_event:
    les bx, speaker_position
    add WORD PTR speaker_position, 3
    mov cx, es:[bx]
    mov al, es:[bx+2]
    mov ticks_to_event, al
    jcxz silence
    cmp cx, 13h
    jb control_event

    in al, 61h
    and al, 0fch
    cmp SPEAKER_ENABLED, 0
    je write_control
    or al, 3
write_control:
    out 61h, al
    mov dx, 12h
    mov ax, 34dch
    div cx
    sub dx, dx
    add dl, sweep_length
    je program_divisor
    mov sweep_count, dx
    mov bx, ax
    mov cl, 7
    shr ax, cl
    div dl
    mov BYTE PTR sweep_delta+1, al
    sub al, al
    mov sweep_fraction, al
    div dl
    mov BYTE PTR sweep_delta, al
    xchg bx, ax

program_divisor:
    mov speaker_divisor, ax
    out 42h, al
    mov al, ah
    out 42h, al
    jmp chain_timer

control_event:
    dec cx
    je set_sweep
    dec cx
    je set_continuation
    les bx, speaker_payload
    test BYTE PTR es:[bx], 80h
    je stop_playback
    cmp speaker_repeat, 0
    jne stop_playback
    add bx, 3
    mov WORD PTR speaker_position, bx
    jmp short read_event

stop_playback:
    push di
    push si
    mov di, ss
    mov si, sp
    mov ax, ds
    cmp ax, di
    je release_payload
    mov ss, ax
    mov sp, OFFSET CALLBACK_STACK_TOP
release_payload:
    sub ax, ax
    mov es, ax
    call RELEASE_SPEAKER_SOUND
    mov ss, di
    mov sp, si
    pop si
    pop di
    jmp acknowledge
set_sweep:
    mov sweep_length, al
    jmp read_event
set_continuation:
    mov SPEAKER_CONTINUATION, al
    jmp read_event
SPEAKER_TIMER_INTERRUPT ENDP
END
