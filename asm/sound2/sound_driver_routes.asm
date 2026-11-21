.MODEL MEDIUM
.DATA
EXTRN RESET_STATUS:BYTE
EXTRN ACTIVE_SOUND_CHANNELS:BYTE
EXTRN FIRST_SOUND_DRIVER:DWORD
EXTRN SECOND_SOUND_DRIVER:DWORD
EXTRN FIRST_SOUND_PAYLOAD:WORD
EXTRN FIRST_SOUND_SEGMENT:WORD
EXTRN FIRST_SOUND_FLAG:WORD
EXTRN SECOND_SOUND_PAYLOAD:WORD
EXTRN SECOND_SOUND_SEGMENT:WORD
EXTRN SECOND_SOUND_FLAG:WORD
EXTRN SPEAKER_PAYLOAD:WORD
EXTRN SPEAKER_SEGMENT:WORD
EXTRN SPEAKER_FLAG:WORD
.CODE
EXTRN RELEASE_FIRST_DEVICE_SOUND:NEAR
EXTRN RELEASE_SECOND_DEVICE_SOUND:NEAR
EXTRN RELEASE_SPEAKER_SOUND:NEAR
PUBLIC RESET_SOUND_DRIVERS, FLAG_SOUND_PAYLOAD, DISPATCH_RESOURCE_POINTER

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
    mov RESET_STATUS, al
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
END
