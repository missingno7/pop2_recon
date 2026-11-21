.MODEL MEDIUM
EXTRN SOUND_DGROUP:BYTE
.DATA
EXTRN SPEAKER_TIMER:BYTE
EXTRN SOUND_CALLBACK_STACK_TOP:BYTE

    DB 0
ACTIVE_SOUND_CHANNELS DB 0
SOUND_LEVEL DB 0
SECOND_SOUND_BUSY DB 0
FIRST_SOUND_STATUS DW 0
DIGITAL_LOOP DW 0
FIRST_SOUND_PAYLOAD DW 0
FIRST_SOUND_SEGMENT DW 0
digital_completion DD 0
FIRST_SOUND_DRIVER DD 0
DIGITAL_LOOP_CAPABILITY DW 0
    DW 0, 0
SECOND_SOUND_STATUS DW 0
midi_repeat DW 0
SECOND_SOUND_PAYLOAD DW 0
SECOND_SOUND_SEGMENT DW 0
midi_completion DD 0
SECOND_SOUND_DRIVER DD 0
midi_clock_rate DW 0
SPEAKER_ACTIVE DW 0
    DW 0
SPEAKER_PAYLOAD DW 0
SPEAKER_SEGMENT DW 0
    DD 0, 0, 0
sequence_marker DB "MSeq"

.CODE
EXTRN STOP_DRIVER_TIMER:NEAR
EXTRN REMOVE_TIMER_HOOK:NEAR
EXTRN START_MIDI_SEQUENCE:NEAR
EXTRN PROCESS_MIDI_EVENTS:NEAR
PUBLIC SET_SOUND_LEVEL, QUERY_SOUND_PAYLOAD
PUBLIC DIGITAL_COMPLETION_INTERRUPT, MIDI_COMPLETION_INTERRUPT
PUBLIC CALL_SOUND_COMPLETION, RELEASE_FIRST_DEVICE_SOUND
PUBLIC RELEASE_SECOND_DEVICE_SOUND, RELEASE_SPEAKER_SOUND
PUBLIC START_DIGITAL_PAYLOAD, START_MIDI_PAYLOAD

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
    jmp SHORT done
second_device:
    cmp ax, SECOND_SOUND_PAYLOAD
    jne speaker
    cmp dx, SECOND_SOUND_SEGMENT
    jne speaker
    mov ax, SECOND_SOUND_STATUS
    jmp SHORT done
speaker:
    cmp ax, SPEAKER_PAYLOAD
    jne inactive
    cmp dx, SPEAKER_SEGMENT
    jne inactive
    mov ax, SPEAKER_ACTIVE
    jmp SHORT done
inactive:
    sub ax, ax
    jmp SHORT done
any_device:
    mov ax, FIRST_SOUND_STATUS
    or ax, SECOND_SOUND_STATUS
    or ax, SPEAKER_ACTIVE
done:
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
    mov sp, OFFSET SOUND_CALLBACK_STACK_TOP
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
    mov sp, OFFSET SOUND_CALLBACK_STACK_TOP
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
    call STOP_DRIVER_TIMER
    push cs
    call START_MIDI_SEQUENCE
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
    mov sp, OFFSET SOUND_CALLBACK_STACK_TOP
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
    call STOP_DRIVER_TIMER
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
    mov ax, OFFSET SPEAKER_TIMER
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
    jz finished
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
    jne finished
wait_for_driver:
    cmp FIRST_SOUND_STATUS, 0
    jne wait_for_driver
finished:
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
    call START_MIDI_SEQUENCE
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

END
