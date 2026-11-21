.MODEL MEDIUM
EXTRN SOUND_DGROUP:BYTE
.DATA
EXTRN CALLBACK_STACK_TOP:BYTE

timer_rate DW 240
time_division DW 0
fractional_tick DW 0
sequence_step_low DW 0
sequence_step_high DW 0
driver_rate DW 0
driver_fraction DW 0
sequence_completion DD 0
DRIVER_STOP_CALLBACK DD 0
TIMER_DESCRIPTOR LABEL BYTE
    DW 0, 0
previous_timer DD 0
    DW 0
bios_tick_step_low DW 0
bios_tick_step_high DW 0
bios_tick_fraction DW 0
sequence_data DD 0
header_marker DB "MThd"
tracks LABEL BYTE
REPT 17
    DW 0, 0, 0
    DB 0
ENDM
track_marker DB "MTrk"
track_count DW 0
tempo_low DW 0a120h
tempo_high DW 7
    DW 0, 0, 0
    DB 34h, 0
filter_channel DW 0
channel_map DB 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15

.CODE
EXTRN CALCULATE_SOUND_TIMER_STEP:NEAR
EXTRN READ_SOUND_DELTA:NEAR
EXTRN PROCESS_MIDI_EVENTS:NEAR
EXTRN DIVIDE_TIMER_INTERVAL:NEAR
EXTRN INSTALL_TIMER_HOOK:NEAR
EXTRN REMOVE_TIMER_HOOK:NEAR
PUBLIC START_MIDI_SEQUENCE, STOP_DRIVER_TIMER, DRIVER_TIMER_INTERRUPT

START_MIDI_SEQUENCE PROC FAR
    push bp
    mov bp, sp
    add sp, -4
    push di
    push si
    les ax, [bp+8]
    mov WORD PTR DRIVER_STOP_CALLBACK, ax
    mov WORD PTR DRIVER_STOP_CALLBACK+2, es
    mov filter_channel, 0
    mov bx, OFFSET channel_map
    mov cx, 8
reset_channels:
    mov ax, [bx]
    and ax, 0f0fh
    mov [bx], ax
    inc bx
    inc bx
    loop reset_channels
    mov al, 4
    call DRIVER_STOP_CALLBACK

    mov cx, 4
    les di, [bp+10h]
    lea ax, [di+6]
    mov WORD PTR sequence_data, di
    mov WORD PTR sequence_data+2, es
    mov si, OFFSET header_marker
    repe cmpsb
    jne sequence_done
    xchg si, ax
    lods WORD PTR es:[si]
    xchg al, ah
    add ax, si
    mov [bp-2], ax
    lods WORD PTR es:[si]
    xchg al, ah
    cmp ax, 1
    ja sequence_done
    lods WORD PTR es:[si]
    xchg al, ah
    mov track_count, ax
    mov [bp-4], ax
    xchg cx, ax
    jcxz sequence_done
    lods WORD PTR es:[si]
    xchg al, ah
    test ah, 80h
    je open_tracks
sequence_done:
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 0eh

open_tracks:
    mov time_division, ax
    mov fractional_tick, 0
    mov ax, tempo_low
    mov dx, tempo_high
    call CALCULATE_SOUND_TIMER_STEP
    mov di, OFFSET tracks
search_track:
    xchg di, ax
    mov cx, 4
    mov di, [bp-2]
    mov si, OFFSET track_marker
    repe cmpsb
    pushf
    xchg di, ax
    mov si, [bp-2]
    add si, 6
    lods WORD PTR es:[si]
    xchg al, ah
    add ax, si
    mov [bp-2], ax
    popf
    jne search_track
    mov BYTE PTR [di+6], 0
    call READ_SOUND_DELTA
    mov [di+4], si
    mov [di], ax
    mov [di+2], dx
    or ax, dx
    jne advance_track
    push es
    call PROCESS_MIDI_EVENTS
    pop es
advance_track:
    add di, 7
    dec WORD PTR [bp-4]
    jne search_track

    mov ax, timer_rate
    mov cx, [bp+6]
    jcxz configure_timer
    cmp ax, cx
    jae convert_rate
    mov ax, cx
convert_rate:
    push ax
    sub dx, dx
    mov driver_fraction, dx
    push cx
    push dx
    push ax
    push dx
    push cs
    call DIVIDE_TIMER_INTERVAL
    xchg cx, ax
    pop ax
configure_timer:
    mov driver_rate, cx
    mov dx, OFFSET TIMER_DESCRIPTOR
    push ds
    push dx
    mov dx, OFFSET DRIVER_TIMER_INTERRUPT
    push cs
    push dx
    push ax
    push cs
    call INSTALL_TIMER_HOOK
    les ax, [bp+0ch]
    mov WORD PTR sequence_completion, ax
    mov WORD PTR sequence_completion+2, es
    jmp sequence_done
START_MIDI_SEQUENCE ENDP

STOP_DRIVER_TIMER PROC FAR
    pushf
    cli
    mov ax, OFFSET TIMER_DESCRIPTOR
    push ds
    push ax
    push cs
    call REMOVE_TIMER_HOOK
    mov al, 4
    call DRIVER_STOP_CALLBACK
    popf
    retf
STOP_DRIVER_TIMER ENDP

DRIVER_TIMER_INTERRUPT PROC FAR
    push ax
    push bx
    push cx
    push dx
    push ds
    mov ax, SEG SOUND_DGROUP
    mov ds, ax
    mov cx, driver_rate
    jcxz advance_clock
    add driver_fraction, cx
    jnc advance_clock
    mov al, 3
    call DRIVER_STOP_CALLBACK
advance_clock:
    mov ax, sequence_step_low
    add fractional_tick, ax
    mov ax, sequence_step_high
    adc ax, 0
    je chain_bios_timer

    push bp
    push di
    push si
    push es
    cld
    xchg si, ax
    mov bp, track_count
    mov di, OFFSET tracks
    mov es, WORD PTR sequence_data+2
next_track:
    sub [di], si
    sbb WORD PTR [di+2], 0
    jl send_events
    jg advance_track_slot
    cmp WORD PTR [di], 0
    jne advance_track_slot
send_events:
    call PROCESS_MIDI_EVENTS
    je finish_sequence
advance_track_slot:
    add di, 7
    dec bp
    jne next_track
    jmp short restore_track_registers

finish_sequence:
    mov di, ss
    mov si, sp
    mov ax, ds
    cmp ax, di
    je stop_sequence
    mov ss, ax
    mov sp, OFFSET CALLBACK_STACK_TOP
stop_sequence:
    push cs
    call NEAR PTR STOP_DRIVER_TIMER
    mov ax, WORD PTR sequence_completion
    or ax, WORD PTR sequence_completion+2
    je restore_interrupt_stack
    call sequence_completion
restore_interrupt_stack:
    mov ss, di
    mov sp, si
restore_track_registers:
    pop es
    pop si
    pop di
    pop bp
chain_bios_timer:
    sub cx, cx
    mov ax, bios_tick_step_low
    add bios_tick_fraction, ax
    adc cx, bios_tick_step_high
    je acknowledge
next_bios_tick:
    pushf
    call previous_timer
    loop next_bios_tick
    jmp short restore_registers
acknowledge:
    mov al, 20h
    out 20h, al
restore_registers:
    pop ds
    pop dx
    pop cx
    pop bx
    pop ax
    iret
DRIVER_TIMER_INTERRUPT ENDP
END
