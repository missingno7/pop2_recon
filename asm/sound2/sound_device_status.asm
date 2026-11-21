.MODEL MEDIUM
.DATA
EXTRN ACTIVE_SOUND_CHANNELS:BYTE
EXTRN SOUND_LEVEL:BYTE
EXTRN FIRST_SOUND_DRIVER:DWORD
EXTRN SECOND_SOUND_DRIVER:DWORD
EXTRN SPEAKER_ACTIVE:WORD
EXTRN FIRST_SOUND_PAYLOAD:WORD
EXTRN FIRST_SOUND_SEGMENT:WORD
EXTRN FIRST_SOUND_STATUS:WORD
EXTRN SECOND_SOUND_PAYLOAD:WORD
EXTRN SECOND_SOUND_SEGMENT:WORD
EXTRN SECOND_SOUND_STATUS:WORD
EXTRN SPEAKER_PAYLOAD:WORD
EXTRN SPEAKER_SEGMENT:WORD
.CODE
PUBLIC SET_SOUND_LEVEL

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

PUBLIC QUERY_SOUND_PAYLOAD

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
END
