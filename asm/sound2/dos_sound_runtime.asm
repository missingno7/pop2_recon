; DOS sound driver dispatch, playback state and interrupt callbacks.
.MODEL MEDIUM
EXTRN SOUND_DGROUP:BYTE

.DATA
; Driver entry points supplied by the startup loader and shared engine state.
EXTRN FIRST_PROBE_DRIVER:DWORD
EXTRN FIRST_PROBE_SEGMENT:WORD
EXTRN SECOND_PROBE_DRIVER:DWORD
EXTRN SECOND_PROBE_SEGMENT:WORD
EXTRN MIDI_MODE:BYTE
EXTRN DRIVER_CONFIG:DWORD
EXTRN CALLBACK_STACK_TOP:BYTE

; DOS MIDI sequencer state and format markers.
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
sound_tempo_low DW 0
sound_tempo_high DW 0
    DW 0
    DB 34h
event_status DB 0
filter_channel DW 0
channel_map DB 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15

; Playback state shared by the three device routes.
EVEN
driver_status DB 0
ACTIVE_SOUND_CHANNELS DB 0
SOUND_LEVEL DB 0
SECOND_SOUND_BUSY DB 0
FIRST_SOUND_STATUS DW 0
FIRST_SOUND_FLAG LABEL WORD
DIGITAL_LOOP DW 0
FIRST_SOUND_PAYLOAD DW 0
FIRST_SOUND_SEGMENT DW 0
digital_completion DD 0
FIRST_SOUND_DRIVER DD 0
DIGITAL_LOOP_CAPABILITY DW 0
digital_rate DW 0
digital_type DW 0
SECOND_SOUND_STATUS DW 0
SECOND_SOUND_FLAG LABEL WORD
midi_repeat DW 0
SECOND_SOUND_PAYLOAD DW 0
SECOND_SOUND_SEGMENT DW 0
midi_completion DD 0
SECOND_SOUND_DRIVER DD 0
midi_clock_rate DW 0
SPEAKER_ACTIVE DW 0
SPEAKER_FLAG DW 0
SPEAKER_PAYLOAD DW 0
SPEAKER_SEGMENT DW 0
speaker_completion DD 0
timer_hooks DD 0, 0
sequence_marker DB "MSeq"
speaker_position DD 0
speaker_timer_state DD 0
speaker_previous_timer DD 0
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
EXTRN DIVIDE_TIMER_INTERVAL:NEAR
EXTRN INSTALL_TIMER_HOOK:NEAR
EXTRN REMOVE_TIMER_HOOK:NEAR
EXTRN INSTALL_TIMER_ENTRY:FAR
EXTRN REMOVE_TIMER_ENTRY:FAR

PUBLIC START_MIDI_SEQUENCE
PUBLIC STOP_DRIVER_TIMER
PUBLIC DRIVER_TIMER_INTERRUPT
PUBLIC CALCULATE_SOUND_TIMER_STEP
PUBLIC PROCESS_MIDI_EVENTS
PUBLIC READ_SOUND_DELTA
PUBLIC PROBE_FIRST_SOUND_DRIVER
PUBLIC PROBE_SECOND_SOUND_DRIVER
PUBLIC CONFIGURE_SOUND_DEVICES
PUBLIC RESET_SOUND_DRIVERS
PUBLIC FLAG_SOUND_PAYLOAD
PUBLIC DISPATCH_RESOURCE_POINTER
PUBLIC START_SOUND_PAYLOAD
PUBLIC SET_SOUND_LEVEL
PUBLIC QUERY_SOUND_PAYLOAD
PUBLIC DIGITAL_COMPLETION_INTERRUPT
PUBLIC MIDI_COMPLETION_INTERRUPT
PUBLIC CALL_SOUND_COMPLETION
PUBLIC RELEASE_FIRST_DEVICE_SOUND
PUBLIC RELEASE_SECOND_DEVICE_SOUND
PUBLIC RELEASE_SPEAKER_SOUND
PUBLIC START_DIGITAL_PAYLOAD
PUBLIC START_MIDI_PAYLOAD
PUBLIC START_SPEAKER_PAYLOAD
PUBLIC SPEAKER_TIMER_INTERRUPT

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
sequence_reset_channels:
    mov ax, [bx]
    and ax, 0f0fh
    mov [bx], ax
    inc bx
    inc bx
    loop sequence_reset_channels
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
    je sequence_acknowledge
next_bios_tick:
    pushf
    call previous_timer
    loop next_bios_tick
    jmp short sequence_restore_registers
sequence_acknowledge:
    mov al, 20h
    out 20h, al
sequence_restore_registers:
    pop ds
    pop dx
    pop cx
    pop bx
    pop ax
    iret
DRIVER_TIMER_INTERRUPT ENDP

CALCULATE_SOUND_TIMER_STEP PROC NEAR
    push di
    push si
    mov sound_tempo_low, ax
    mov sound_tempo_high, dx
    mov dh, dl
    mov dl, ah
    mov ah, al
    sub al, al
    push dx
    push ax
    push time_division
    sub ax, ax
    push ax
    push cs
    call DIVIDE_TIMER_INTERVAL
    xchg di, ax
    mov si, dx
    mov dx, 0f42h
    mov ax, 4000h
    push dx
    push ax
    push timer_rate
    sub ax, ax
    push ax
    push cs
    call DIVIDE_TIMER_INTERVAL
    push dx
    push ax
    push si
    push di
    push cs
    call DIVIDE_TIMER_INTERVAL
    mov sequence_step_low, ax
    mov sequence_step_high, dx
    pop si
    pop di
    ret
CALCULATE_SOUND_TIMER_STEP ENDP

PROCESS_MIDI_EVENTS PROC NEAR
    xchg [di+4], si
events_read_event:
    lods BYTE PTR es:[si]
    cmp al, 0ffh
    jne channel_event
    jmp meta_event

channel_event:
    mov event_status, al
    cmp al, 0f0h
    jae system_event
    test al, al
    js save_running_status
    dec si
    mov al, [di+6]
save_running_status:
    mov [di+6], al
    mov ah, al
    and ax, 0ff0h
    mov dl, es:[si]
    inc si
    cmp al, 0c0h
    je remap_channel
    cmp al, 0d0h
    je remap_channel
    mov dh, es:[si]
    inc si
remap_channel:
    push bx
    push ax
    mov al, ah
    mov bx, OFFSET channel_map
    xlat
    pop bx
    mov ah, al
    mov al, bl
    pop bx
    cmp al, 90h
    jne check_channel
    test ah, 80h
    jne next_delta
check_channel:
    and ah, 0fh
    cmp ah, 0fh
    jne dispatch_driver
    cmp filter_channel, 1
    jne dispatch_driver
    jmp short next_delta
dispatch_driver:
    call DRIVER_STOP_CALLBACK
next_delta:
    call READ_SOUND_DELTA
    add [di], ax
    adc [di+2], dx
    jl events_read_event
    jg events_finished
    cmp WORD PTR [di], 0
    je events_read_event
events_finished:
    xchg [di+4], si
    ret

system_event:
    call READ_SOUND_DELTA
    mov bx, si
    add si, ax
    xchg cx, ax
    xor ax, ax
    cmp es:[bx], ax
    jne pass_system_event
    cmp BYTE PTR es:[bx+2], 34h
    jne pass_system_event
    mov al, es:[bx+3]
    cmp al, 0
    je dispatch_command
    cmp al, MIDI_MODE
    je dispatch_command
pass_system_event:
    mov al, event_status
    jmp short dispatch_driver

dispatch_command:
    mov ax, es:[bx+4]
    cmp al, 0
    je forward_command
    cmp al, 1
    je mute_channel
    cmp al, 2
    je unmute_channel
    cmp al, 3
    je change_channel
    cmp al, 4
    je events_reset_channels
    cmp al, 5
    je mute_channel_range
    cmp al, 6
    je enable_filter
    cmp al, 7
    je disable_filter
command_done:
    jmp short next_delta
forward_command:
    mov al, 0f0h
    jmp short dispatch_driver
    jmp short command_done
unmute_channel:
    mov al, ah
    xor ah, ah
    mov bx, OFFSET channel_map
    add bx, ax
    and BYTE PTR [bx], 7fh
    jmp short command_done
mute_channel:
    mov al, ah
    xor ah, ah
    mov bx, OFFSET channel_map
    add bx, ax
    or BYTE PTR [bx], 80h
    jmp short command_done
change_channel:
    mov al, ah
    xor ah, ah
    mov cl, es:[bx+6]
    mov bx, OFFSET channel_map
    add bx, ax
    and BYTE PTR [bx], 80h
    or [bx], cl
    jmp short command_done

events_reset_channels:
    xor al, al
    mov cx, 0fh
    mov bx, OFFSET channel_map
reset_next:
    and BYTE PTR [bx], 80h
    or [bx], al
    inc al
    inc bx
    loop reset_next
    jmp short command_done

mute_channel_range:
    mov bx, OFFSET channel_map
    or ah, ah
    je mute_next
unmute_next:
    and BYTE PTR [bx], 7fh
    inc bx
    loop unmute_next
    jmp short command_done
mute_next:
    or BYTE PTR [bx], 80h
    inc bx
    loop mute_next
    jmp short command_done

enable_filter:
    mov filter_channel, 1
    jmp short command_done
disable_filter:
    mov filter_channel, 0
    jmp short command_done

meta_event:
    lods BYTE PTR es:[si]
    cmp al, 7
    je set_sequence_flag
    cmp al, 2fh
    je end_track
    cmp al, 51h
    je change_tempo
skip_meta_data:
    call READ_SOUND_DELTA
    add si, ax
    jmp next_delta
set_sequence_flag:
    call READ_SOUND_DELTA
    xchg dx, ax
    mov al, es:[si]
    mov SECOND_SOUND_BUSY, al
    add si, dx
    jmp next_delta
end_track:
    sub si, 2
    cmp di, OFFSET tracks
    jne finish_secondary_track
    jmp events_finished
finish_secondary_track:
    mov WORD PTR [di], 0ffffh
    mov WORD PTR [di+2], 7fffh
    jmp events_finished
change_tempo:
    cmp di, OFFSET tracks
    jne skip_meta_data
    sub dh, dh
    mov dl, es:[si+1]
    mov ax, es:[si+2]
    call CALCULATE_SOUND_TIMER_STEP
    jmp short skip_meta_data
PROCESS_MIDI_EVENTS ENDP

READ_SOUND_DELTA PROC NEAR
    sub ax, ax
    cwd
read_next_byte:
    mov dh, dl
    mov dl, ah
    mov ah, al
    lods BYTE PTR es:[si]
    shl al, 1
    jnc finish_delta
    shr dx, 1
    shr ax, 1
    jmp SHORT read_next_byte
finish_delta:
    shr dx, 1
    rcr ax, 1
    ret
READ_SOUND_DELTA ENDP

PROBE_FIRST_SOUND_DRIVER PROC FAR
    push bp
    mov bp, sp
    push si
    xor si, si
    mov ax, WORD PTR FIRST_PROBE_DRIVER
    or ax, FIRST_PROBE_SEGMENT
    jz first_probe_done
    les bx, DWORD PTR [bp+6]
    mov al, 0
    call FIRST_PROBE_DRIVER
    test ax, ax
    jl first_probe_done
    inc si
first_probe_done:
    mov ax, si
    pop si
    pop bp
    retf 4
PROBE_FIRST_SOUND_DRIVER ENDP

PROBE_SECOND_SOUND_DRIVER PROC FAR
    push bp
    mov bp, sp
    push si
    xor si, si
    mov ax, WORD PTR SECOND_PROBE_DRIVER
    or ax, SECOND_PROBE_SEGMENT
    jz second_probe_done
    les bx, DWORD PTR [bp+6]
    mov al, 0
    call SECOND_PROBE_DRIVER
    test ax, ax
    jl second_probe_done
    inc si
second_probe_done:
    mov ax, si
    pop si
    pop bp
    retf 4
PROBE_SECOND_SOUND_DRIVER ENDP

CONFIGURE_SOUND_DEVICES PROC FAR
    push bp
    mov bp, sp
    push di
    sub ax, ax
    mov cx, timer_hooks - driver_status
    mov di, OFFSET driver_status
    push ds
    pop es
    rep stosb
    mov driver_status, 10h

    mov di, es
    mov si, OFFSET timer_hooks
    mov ax, OFFSET INSTALL_TIMER_ENTRY
    mov [si], ax
    mov ax, SEG INSTALL_TIMER_ENTRY
    mov [si+2], ax
    mov ax, OFFSET REMOVE_TIMER_ENTRY
    mov [si+4], ax
    mov ax, SEG REMOVE_TIMER_ENTRY
    mov [si+6], ax

    xor bx, bx
    mov es, bx
    mov al, 1
    call SECOND_PROBE_DRIVER
    test ax, ax
    je digital_ok
    or driver_status, 4
    jmp short probe_midi
digital_ok:
    mov DIGITAL_LOOP_CAPABILITY, bx
    mov digital_rate, cx
    mov digital_type, dx
    les ax, SECOND_PROBE_DRIVER
    mov WORD PTR FIRST_SOUND_DRIVER, ax
    mov WORD PTR FIRST_SOUND_DRIVER+2, es
    or ACTIVE_SOUND_CHANNELS, 1

probe_midi:
    mov al, 1
    push ds
    lds bx, ss:DRIVER_CONFIG
    mov di, [bx+8]
    pop ds
    xor bx, bx
    mov es, bx
    call FIRST_PROBE_DRIVER
    test ax, ax
    jge midi_ok
    or driver_status, 8
    jmp short update_channels
midi_ok:
    mov midi_clock_rate, ax
    les ax, FIRST_PROBE_DRIVER
    mov WORD PTR SECOND_SOUND_DRIVER, ax
    mov WORD PTR SECOND_SOUND_DRIVER+2, es
    or ACTIVE_SOUND_CHANNELS, 2
    les bx, [bp+6]
    mov ax, es
    or ax, bx
    jz update_channels
    mov ah, es:[bx]
    inc bx
    mov al, 6
    call SECOND_SOUND_DRIVER

update_channels:
    mov ax, 0fh
    push ax
    push cs
    call NEAR PTR SET_SOUND_LEVEL
    mov al, ACTIVE_SOUND_CHANNELS
    xor ah, ah
    pop di
    pop bp
    retf 4
CONFIGURE_SOUND_DEVICES ENDP

RESET_SOUND_DRIVERS PROC FAR
    xor ax, ax
    push ax
    push ax
    push cs
    call NEAR PTR DISPATCH_RESOURCE_POINTER
    test ACTIVE_SOUND_CHANNELS, 1
    jz reset_second
    mov al, 2
    call FIRST_SOUND_DRIVER
reset_second:
    test ACTIVE_SOUND_CHANNELS, 2
    jz clear_reset_state
    mov al, 2
    call SECOND_SOUND_DRIVER
clear_reset_state:
    sub al, al
    mov driver_status, al
    mov ACTIVE_SOUND_CHANNELS, al
    retf
RESET_SOUND_DRIVERS ENDP

FLAG_SOUND_PAYLOAD PROC FAR
    push bp
    mov bp, sp
    les ax, DWORD PTR [bp+6]
    mov dx, es
    or dx, ax
    jz flag_all
    mov dx, es
    cmp ax, FIRST_SOUND_PAYLOAD
    jne flag_second
    cmp dx, FIRST_SOUND_SEGMENT
    jne flag_second
    mov FIRST_SOUND_FLAG, 1
    jmp SHORT flag_done
flag_second:
    cmp ax, SECOND_SOUND_PAYLOAD
    jne flag_speaker
    cmp dx, SECOND_SOUND_SEGMENT
    jne flag_speaker
    mov SECOND_SOUND_FLAG, 1
    jmp SHORT flag_done
flag_speaker:
    cmp ax, SPEAKER_PAYLOAD
    jne flag_done
    cmp dx, SPEAKER_SEGMENT
    jne flag_done
    mov SPEAKER_FLAG, 1
flag_done:
    pop bp
    retf 4
flag_all:
    mov ax, 1
    mov FIRST_SOUND_FLAG, ax
    mov SECOND_SOUND_FLAG, ax
    mov SPEAKER_FLAG, ax
    jmp SHORT flag_done
FLAG_SOUND_PAYLOAD ENDP

DISPATCH_RESOURCE_POINTER PROC FAR
    push bp
    mov bp, sp
    les ax, DWORD PTR [bp+6]
    call RELEASE_FIRST_DEVICE_SOUND
    les ax, DWORD PTR [bp+6]
    call RELEASE_SECOND_DEVICE_SOUND
    les ax, DWORD PTR [bp+6]
    call RELEASE_SPEAKER_SOUND
    pop bp
    retf 4
DISPATCH_RESOURCE_POINTER ENDP

START_SOUND_PAYLOAD PROC FAR
    push bp
    mov bp, sp
    les bx, [bp+0ah]
    mov ax, es
    or ax, bx
    jz dispatch_done
    mov al, es:[bx]
    and al, 7fh
    cmp al, 1
    je digital_payload
    cmp al, 2
    je midi_payload
    cmp al, 0
    je dispatch_speaker_payload
    xor ax, ax
dispatch_done:
    pop bp
    retf 8

dispatch_speaker_payload:
    push es
    push bx
    les ax, [bp+6]
    push es
    push ax
    call START_SPEAKER_PAYLOAD
    jmp short dispatch_done

digital_payload:
    push es
    push bx
    les ax, [bp+6]
    push es
    push ax
    call START_DIGITAL_PAYLOAD
    jmp short dispatch_done

midi_payload:
    push es
    push bx
    les ax, [bp+6]
    push es
    push ax
    call START_MIDI_PAYLOAD
    jmp short dispatch_done
START_SOUND_PAYLOAD ENDP

SET_SOUND_LEVEL PROC FAR
    push bp
    mov bp, sp
    mov al, [bp+6]
    cmp al, 15
    jle level_ready
    mov al, 15
level_ready:
    cmp al, SOUND_LEVEL
    mov SOUND_LEVEL, al
    je level_done
    test ACTIVE_SOUND_CHANNELS, 1
    jz second_driver
    mov ah, al
    mov al, 4
    call FIRST_SOUND_DRIVER
second_driver:
    test ACTIVE_SOUND_CHANNELS, 2
    jz update_speaker
    mov ah, SOUND_LEVEL
    mov al, 5
    call SECOND_SOUND_DRIVER
update_speaker:
    mov cx, SPEAKER_ACTIVE
    jcxz level_done
    in al, 61h
    and al, 0fch
    cmp SOUND_LEVEL, 0
    je speaker_write
    or al, 3
speaker_write:
    out 61h, al
level_done:
    pop bp
    retf 2
SET_SOUND_LEVEL ENDP

QUERY_SOUND_PAYLOAD PROC FAR
    push bp
    mov bp, sp
    les ax, DWORD PTR [bp+6]
    mov dx, es
    or dx, ax
    jz any_device
    mov dx, es
    cmp ax, FIRST_SOUND_PAYLOAD
    jne second_device
    cmp dx, FIRST_SOUND_SEGMENT
    jne second_device
    mov ax, FIRST_SOUND_STATUS
    jmp SHORT release_done
second_device:
    cmp ax, SECOND_SOUND_PAYLOAD
    jne speaker
    cmp dx, SECOND_SOUND_SEGMENT
    jne speaker
    mov ax, SECOND_SOUND_STATUS
    jmp SHORT release_done
speaker:
    cmp ax, SPEAKER_PAYLOAD
    jne inactive
    cmp dx, SPEAKER_SEGMENT
    jne inactive
    mov ax, SPEAKER_ACTIVE
    jmp SHORT release_done
inactive:
    sub ax, ax
    jmp SHORT release_done
any_device:
    mov ax, FIRST_SOUND_STATUS
    or ax, SECOND_SOUND_STATUS
    or ax, SPEAKER_ACTIVE
release_done:
    pop bp
    retf 4
QUERY_SOUND_PAYLOAD ENDP

DIGITAL_COMPLETION_INTERRUPT PROC FAR
    cld
    push ax
    push bx
    push cx
    push dx
    push di
    push si
    push ds
    push es
    mov ax, SEG SOUND_DGROUP
    mov ds, ax
    mov dx, ss
    mov bx, sp
    cmp ax, dx
    je digital_stack_ready
    mov ss, ax
    mov sp, OFFSET CALLBACK_STACK_TOP
digital_stack_ready:
    push dx
    push bx
    les bx, DWORD PTR FIRST_SOUND_PAYLOAD
    sub ax, ax
    test BYTE PTR es:[bx], 80h
    je digital_finished
    cmp ax, DIGITAL_LOOP_CAPABILITY
    je digital_finished
    cmp ax, DIGITAL_LOOP
    je repeat_intro
    jg digital_finished
    add ax, es:[bx+8]
    je digital_finished
    mov cx, es:[bx+4]
    sub cx, ax
    je digital_finished
    neg DIGITAL_LOOP
    jmp short submit_digital
repeat_intro:
    add ax, es:[bx+8]
    jne repeat_intro_length
    mov ax, es:[bx+4]
repeat_intro_length:
    xchg cx, ax
    mov ax, es:[bx+6]
    sub cx, ax
submit_digital:
    mov dx, es:[bx+1]
    add bx, ax
    add bx, 0ah
    mov di, SEG DIGITAL_COMPLETION_INTERRUPT
    mov si, OFFSET DIGITAL_COMPLETION_INTERRUPT
    mov al, 7
    call FIRST_SOUND_DRIVER
    jmp short digital_restore
digital_finished:
    add bx, es:[bx+4]
    mov ah, es:[bx+9]
    mov al, 6
    call FIRST_SOUND_DRIVER
    sub ax, ax
    mov bx, OFFSET FIRST_SOUND_STATUS
    mov [bx], ax
    call CALL_SOUND_COMPLETION
digital_restore:
    pop ax
    pop ss
    mov sp, ax
    pop es
    pop ds
    pop si
    pop di
    pop dx
    pop cx
    pop bx
    pop ax
    iret
DIGITAL_COMPLETION_INTERRUPT ENDP

MIDI_COMPLETION_INTERRUPT PROC FAR
    cld
    push ax
    push bx
    push cx
    push dx
    push ds
    push es
    mov ax, SEG SOUND_DGROUP
    mov ds, ax
    mov dx, ss
    mov bx, sp
    cmp ax, dx
    je midi_stack_ready
    mov ss, ax
    mov sp, OFFSET CALLBACK_STACK_TOP
midi_stack_ready:
    push dx
    push bx
    sub ax, ax
    les bx, DWORD PTR SECOND_SOUND_PAYLOAD
    test BYTE PTR es:[bx], 80h
    je midi_finished
    cmp ax, midi_repeat
    jne midi_finished
    add bx, 1
    push es
    push bx
    mov ax, OFFSET MIDI_COMPLETION_INTERRUPT
    push cs
    push ax
    les ax, SECOND_SOUND_DRIVER
    push es
    push ax
    push midi_clock_rate
    push cs
    call NEAR PTR STOP_DRIVER_TIMER
    push cs
    call NEAR PTR START_MIDI_SEQUENCE
    jmp short midi_restore
midi_finished:
    mov bx, OFFSET SECOND_SOUND_STATUS
    mov [bx], ax
    call CALL_SOUND_COMPLETION
midi_restore:
    pop ax
    pop ss
    mov sp, ax
    pop es
    pop ds
    pop dx
    pop cx
    pop bx
    pop ax
    iret
MIDI_COMPLETION_INTERRUPT ENDP

CALL_SOUND_COMPLETION PROC NEAR
    push di
    push si
    push ds
    mov dx, SEG SOUND_DGROUP
    mov ds, dx
    mov cx, [bx+8]
    or cx, [bx+10]
    jz callback_done
    mov di, ss
    mov si, sp
    cmp dx, di
    je invoke_callback
    mov ss, dx
    mov sp, OFFSET CALLBACK_STACK_TOP
invoke_callback:
    les dx, DWORD PTR [bx+4]
    push es
    push dx
    push ax
    call DWORD PTR [bx+8]
    mov ss, di
    mov sp, si
callback_done:
    pop ds
    pop si
    pop di
    ret
CALL_SOUND_COMPLETION ENDP

RELEASE_FIRST_DEVICE_SOUND PROC NEAR
    pushf
    cli
    mov dx, es
    or dx, ax
    jz stop_first
    cmp ax, FIRST_SOUND_PAYLOAD
    jne first_done
    mov dx, es
    cmp dx, FIRST_SOUND_SEGMENT
    jne first_done
stop_first:
    sub cx, cx
    xchg cx, FIRST_SOUND_STATUS
    jcxz first_done
    mov al, 3
    call FIRST_SOUND_DRIVER
    mov ax, 1
    mov bx, OFFSET FIRST_SOUND_STATUS
    call CALL_SOUND_COMPLETION
first_done:
    popf
    ret
RELEASE_FIRST_DEVICE_SOUND ENDP

RELEASE_SECOND_DEVICE_SOUND PROC NEAR
    mov dx, es
    or dx, ax
    jz stop_second
    cmp ax, SECOND_SOUND_PAYLOAD
    jne second_done
    mov dx, es
    cmp dx, SECOND_SOUND_SEGMENT
    jne second_done
stop_second:
    sub cx, cx
    xchg cx, SECOND_SOUND_STATUS
    jcxz second_done
    mov SECOND_SOUND_BUSY, 0
    push cs
    call NEAR PTR STOP_DRIVER_TIMER
    mov ax, 1
    mov bx, OFFSET SECOND_SOUND_STATUS
    call CALL_SOUND_COMPLETION
second_done:
    ret
RELEASE_SECOND_DEVICE_SOUND ENDP

RELEASE_SPEAKER_SOUND PROC NEAR
    pushf
    cli
    mov dx, es
    or dx, ax
    jz stop_speaker
    cmp ax, SPEAKER_PAYLOAD
    jne speaker_done
    mov dx, es
    cmp dx, SPEAKER_SEGMENT
    jne speaker_done
stop_speaker:
    sub cx, cx
    xchg cx, SPEAKER_ACTIVE
    jcxz speaker_done
    mov SECOND_SOUND_BUSY, 0
    mov ax, OFFSET speaker_timer_state
    push ds
    push ax
    push cs
    call REMOVE_TIMER_HOOK
    in al, 61h
    and al, 0fch
    out 61h, al
    mov ax, 1
    mov bx, OFFSET SPEAKER_ACTIVE
    call CALL_SOUND_COMPLETION
speaker_done:
    popf
    ret
RELEASE_SPEAKER_SOUND ENDP

START_DIGITAL_PAYLOAD PROC NEAR
    push bp
    mov bp, sp
    push di
    push si
    mov al, ACTIVE_SOUND_CHANNELS
    and al, 1
    jz release_finished
    xor ax, ax
    mov es, ax
    call RELEASE_FIRST_DEVICE_SOUND

    mov FIRST_SOUND_STATUS, 1
    mov DIGITAL_LOOP, 0
    les ax, [bp+4]
    mov WORD PTR digital_completion, ax
    mov WORD PTR digital_completion+2, es
    les bx, [bp+8]
    mov FIRST_SOUND_PAYLOAD, bx
    mov FIRST_SOUND_SEGMENT, es
    mov ah, es:[bx+0ah]
    mov al, 5
    call FIRST_SOUND_DRIVER

    les bx, [bp+8]
    test BYTE PTR es:[bx], 80h
    jz use_payload_length
    mov cx, DIGITAL_LOOP_CAPABILITY
    jcxz use_payload_length
    mov cx, es:[bx+8]
    test cx, cx
    jnz start_driver
use_payload_length:
    mov cx, es:[bx+4]
start_driver:
    mov dx, es:[bx+1]
    add bx, 0ah
    mov di, SEG DIGITAL_COMPLETION_INTERRUPT
    mov si, OFFSET DIGITAL_COMPLETION_INTERRUPT
    mov al, 7
    call FIRST_SOUND_DRIVER

    cmp DIGITAL_LOOP_CAPABILITY, 0
    jne release_finished
wait_for_driver:
    cmp FIRST_SOUND_STATUS, 0
    jne wait_for_driver
release_finished:
    mov ax, FIRST_SOUND_STATUS
    pop si
    pop di
    pop bp
    ret 8
START_DIGITAL_PAYLOAD ENDP

START_MIDI_PAYLOAD PROC NEAR
    push bp
    mov bp, sp
    add sp, -8
    push di
    push si
    sub ax, ax
    test ACTIVE_SOUND_CHANNELS, 2
    je midi_payload_done

    les bx, [bp+8]
    mov cx, 4
    lea di, [bx+1]
    mov si, OFFSET sequence_marker
    repe cmpsb
    je control_sequence
    mov SECOND_SOUND_BUSY, 0
    push es
    push bx
    sub ax, ax
    mov es, ax
    call RELEASE_SECOND_DEVICE_SOUND
    pop SECOND_SOUND_PAYLOAD
    pop SECOND_SOUND_SEGMENT
    les ax, [bp+4]
    mov WORD PTR midi_completion, ax
    mov WORD PTR midi_completion+2, es
    mov SECOND_SOUND_STATUS, 1
    mov midi_repeat, 0
    les ax, DWORD PTR SECOND_SOUND_PAYLOAD
    add ax, 1
    push es
    push ax
    push cs
    mov ax, OFFSET MIDI_COMPLETION_INTERRUPT
    push ax
    les ax, SECOND_SOUND_DRIVER
    push es
    push ax
    push midi_clock_rate
    push cs
    call NEAR PTR START_MIDI_SEQUENCE
    mov ax, SECOND_SOUND_STATUS
midi_payload_done:
    pop si
    pop di
    mov sp, bp
    pop bp
    ret 8

control_sequence:
    xchg di, si
    mov [bp-3], si
    mov [bp-1], cl
    mov [bp-7], cx
    mov [bp-5], cx
    pushf
    cli
    lea di, [bp-7]
    call PROCESS_MIDI_EVENTS
    popf
    mov ax, [bp+6]
    or ax, [bp+4]
    je midi_payload_done
    les ax, [bp+8]
    push es
    push ax
    xor ax, ax
    push ax
    call DWORD PTR [bp+4]
    sub ax, ax
    jmp short midi_payload_done
START_MIDI_PAYLOAD ENDP

START_SPEAKER_PAYLOAD PROC NEAR
    push bp
    mov bp, sp
    sub ax, ax
    mov es, ax
    call RELEASE_SPEAKER_SOUND
    mov cx, FIRST_SOUND_STATUS
    jcxz set_callback
    test ACTIVE_SOUND_CHANNELS, 1
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
    mov SPEAKER_PAYLOAD, bx
    mov SPEAKER_SEGMENT, es
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
    mov SPEAKER_ACTIVE, ax
    mov SPEAKER_FLAG, 0
    mov al, 0b6h
    out 43h, al
    int 8
    popf

    mov ax, SPEAKER_ACTIVE
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
    jz speaker_read_event

    sub ax, ax
    add al, sweep_length
    jne update_sweep
chain_timer:
    les ax, timer_increment
    mov cx, es
    add timer_fraction, ax
    adc cx, 0
    je speaker_acknowledge
call_previous:
    pushf
    call speaker_previous_timer
    loop call_previous
    jmp short speaker_restore_registers
speaker_acknowledge:
    mov al, 20h
    out 20h, al
speaker_restore_registers:
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
    xchg al, ah
    cbw
    add ax, speaker_divisor
    jmp short program_divisor

silence:
    in al, 61h
    and al, 0fch
    out 61h, al
    jmp short chain_timer

speaker_read_event:
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
    cmp SOUND_LEVEL, 0
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
    les bx, DWORD PTR SPEAKER_PAYLOAD
    test BYTE PTR es:[bx], 80h
    je stop_playback
    cmp SPEAKER_FLAG, 0
    jne stop_playback
    add bx, 3
    mov WORD PTR speaker_position, bx
    jmp short speaker_read_event

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
    jmp speaker_acknowledge
set_sweep:
    mov sweep_length, al
    jmp speaker_read_event
set_continuation:
    mov SECOND_SOUND_BUSY, al
    jmp speaker_read_event
SPEAKER_TIMER_INTERRUPT ENDP
END
