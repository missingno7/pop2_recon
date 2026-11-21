.MODEL MEDIUM
.DATA
first_driver DD 0
second_driver DD 0
.CODE
PUBLIC READ_SOUND_DELTA, PROBE_FIRST_SOUND_DRIVER, PROBE_SECOND_SOUND_DRIVER

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
