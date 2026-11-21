.MODEL MEDIUM
.DATA
sound_tempo_low DW 0
sound_tempo_high DW 0
EXTRN SOUND_TIME_DIVISION:WORD
EXTRN SOUND_TIMER_RATE:WORD
EXTRN SOUND_STEP_LOW:WORD
EXTRN SOUND_STEP_HIGH:WORD
.CODE
EXTRN DIVIDE_TIMER_INTERVAL:NEAR
PUBLIC CALCULATE_SOUND_TIMER_STEP

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
    push SOUND_TIME_DIVISION
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
    push SOUND_TIMER_RATE
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
    mov SOUND_STEP_LOW, ax
    mov SOUND_STEP_HIGH, dx
    pop si
    pop di
    ret
CALCULATE_SOUND_TIMER_STEP ENDP
END
