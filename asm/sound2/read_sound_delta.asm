.MODEL MEDIUM
.CODE
PUBLIC READ_SOUND_DELTA

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
END
