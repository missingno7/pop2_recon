.MODEL MEDIUM
.DATA
first_driver DD 0
second_driver DD 0
EXTRN EVENT_STATUS:BYTE
EXTRN MIDI_MODE:BYTE
EXTRN CHANNEL_MAP:BYTE
EXTRN FILTER_CHANNEL:WORD
EXTRN MIDI_DRIVER:DWORD
EXTRN SEQUENCE_FLAG:BYTE
EXTRN TRACKS:BYTE
.CODE
EXTRN CALCULATE_SOUND_TIMER_STEP:NEAR
PUBLIC PROCESS_MIDI_EVENTS
PUBLIC READ_SOUND_DELTA, PROBE_FIRST_SOUND_DRIVER, PROBE_SECOND_SOUND_DRIVER

PROCESS_MIDI_EVENTS PROC NEAR
    xchg [di+4], si
read_event:
    lods BYTE PTR es:[si]
    cmp al, 0ffh
    jne channel_event
    jmp meta_event

channel_event:
    mov EVENT_STATUS, al
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
    mov bx, OFFSET CHANNEL_MAP
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
    cmp FILTER_CHANNEL, 1
    jne dispatch_driver
    jmp short next_delta
dispatch_driver:
    call MIDI_DRIVER
next_delta:
    call READ_SOUND_DELTA
    add [di], ax
    adc [di+2], dx
    jl read_event
    jg finished
    cmp WORD PTR [di], 0
    je read_event
finished:
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
    mov al, EVENT_STATUS
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
    je reset_channels
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
    mov bx, OFFSET CHANNEL_MAP
    add bx, ax
    and BYTE PTR [bx], 7fh
    jmp short command_done
mute_channel:
    mov al, ah
    xor ah, ah
    mov bx, OFFSET CHANNEL_MAP
    add bx, ax
    or BYTE PTR [bx], 80h
    jmp short command_done
change_channel:
    mov al, ah
    xor ah, ah
    mov cl, es:[bx+6]
    mov bx, OFFSET CHANNEL_MAP
    add bx, ax
    and BYTE PTR [bx], 80h
    or [bx], cl
    jmp short command_done

reset_channels:
    xor al, al
    mov cx, 0fh
    mov bx, OFFSET CHANNEL_MAP
reset_next:
    and BYTE PTR [bx], 80h
    or [bx], al
    inc al
    inc bx
    loop reset_next
    jmp short command_done

mute_channel_range:
    mov bx, OFFSET CHANNEL_MAP
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
    mov FILTER_CHANNEL, 1
    jmp short command_done
disable_filter:
    mov FILTER_CHANNEL, 0
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
    mov SEQUENCE_FLAG, al
    add si, dx
    jmp next_delta
end_track:
    sub si, 2
    cmp di, OFFSET TRACKS
    jne finish_secondary_track
    jmp finished
finish_secondary_track:
    mov WORD PTR [di], 0ffffh
    mov WORD PTR [di+2], 7fffh
    jmp finished
change_tempo:
    cmp di, OFFSET TRACKS
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
    mov ax, WORD PTR first_driver
    or ax, WORD PTR [first_driver+2]
    jz first_probe_done
    les bx, DWORD PTR [bp+6]
    mov al, 0
    call first_driver
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
    mov ax, WORD PTR second_driver
    or ax, WORD PTR [second_driver+2]
    jz second_probe_done
    les bx, DWORD PTR [bp+6]
    mov al, 0
    call second_driver
    test ax, ax
    jl second_probe_done
    inc si
second_probe_done:
    mov ax, si
    pop si
    pop bp
    retf 4
PROBE_SECOND_SOUND_DRIVER ENDP
END
