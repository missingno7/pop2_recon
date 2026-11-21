.MODEL MEDIUM
EXTRN SOUND_DGROUP:BYTE
.DATA
EXTRN FIRST_SOUND_PAYLOAD:WORD
EXTRN FIRST_SOUND_SEGMENT:WORD
EXTRN FIRST_SOUND_STATUS:WORD
EXTRN FIRST_SOUND_DRIVER:DWORD
EXTRN SECOND_SOUND_PAYLOAD:WORD
EXTRN SECOND_SOUND_SEGMENT:WORD
EXTRN SECOND_SOUND_STATUS:WORD
EXTRN SECOND_SOUND_BUSY:BYTE
EXTRN SPEAKER_PAYLOAD:WORD
EXTRN SPEAKER_SEGMENT:WORD
EXTRN SPEAKER_ACTIVE:WORD
EXTRN SPEAKER_TIMER:BYTE
EXTRN SOUND_CALLBACK_STACK_TOP:BYTE
.CODE
EXTRN STOP_DRIVER_TIMER:NEAR
EXTRN REMOVE_TIMER_HOOK:NEAR
PUBLIC CALL_SOUND_COMPLETION, RELEASE_FIRST_DEVICE_SOUND
PUBLIC RELEASE_SECOND_DEVICE_SOUND, RELEASE_SPEAKER_SOUND

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
END
